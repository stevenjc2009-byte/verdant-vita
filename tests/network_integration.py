"""Run in Linux beside a running headless emulator. Exercises actual guest NAT,
incoming SSH with a generated test key, atomic downloads and VNC framebuffer.
"""
import sys, socket, threading, subprocess, tempfile, time, struct
from pathlib import Path
from http.server import HTTPServer,BaseHTTPRequestHandler
from guest_integration import request,base
route=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);route.connect(('192.0.2.1',9));host=route.getsockname()[0];route.close()
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200);self.end_headers();self.wfile.write(b'VERDANT_REAL_DOWNLOAD\n')
    def log_message(self,*a):pass
http=HTTPServer(('0.0.0.0',0),Handler);threading.Thread(target=http.serve_forever,daemon=True).start()
download_path='/root/network-download-'+str(time.time_ns())+'.txt'
request('download',f'http://{host}:{http.server_port}/fixture',download_path);jid=str(__import__('guest_integration').serial)
for _ in range(100):
    status=request('job',jid)
    if status.startswith('exit='):break
    time.sleep(.1)
assert status.startswith('exit=0'),status
assert request('read',download_path)=='VERDANT_REAL_DOWNLOAD\n'
http.shutdown();print('PASS guest HTTP download through NAT',flush=True)
with tempfile.TemporaryDirectory() as t:
    key=Path(t)/'key';subprocess.run(['ssh-keygen','-q','-t','ed25519','-N','','-f',str(key)],check=True)
    request('mkdir','/root/.ssh');request('write','/root/.ssh/authorized_keys',Path(str(key)+'.pub').read_text())
    result=subprocess.run(['ssh','-i',str(key),'-p','2222','-o','BatchMode=yes','-o','StrictHostKeyChecking=no','-o','UserKnownHostsFile=/dev/null','-o','ConnectTimeout=15','root@127.0.0.1','printf VERDANT_INCOMING_SSH_OK'],capture_output=True,text=True,timeout=45)
    assert result.returncode==0,(result.stdout,result.stderr)
    assert result.stdout=='VERDANT_INCOMING_SSH_OK',result.stdout
    print('PASS incoming SSH forwarding and key authentication',flush=True)
def exact(s,n):
    result=b''
    while len(result)<n:
        b=s.recv(n-len(result));assert b;result+=b
    return result
listener=socket.socket();listener.bind(('0.0.0.0',0));listener.listen(1);errors=[]
def vnc_server():
    try:
        s,_=listener.accept();s.settimeout(60);s.sendall(b'RFB 003.008\n');assert exact(s,12)==b'RFB 003.008\n';s.sendall(b'\1\1');assert exact(s,1)==b'\1';s.sendall(b'\0'*4);assert exact(s,1)==b'\1'
        s.sendall(struct.pack('>HH',2,2)+b'\0'*16+struct.pack('>I',4)+b'test');exact(s,20);exact(s,8);exact(s,10)
        s.sendall(b'\0\0\0\1'+struct.pack('>HHHHi',0,0,2,2,0)+bytes([0,0,255,0,0,255,0,0,255,0,0,0,255,255,255,0]));exact(s,10);exact(s,16);s.close()
    except Exception as e:errors.append(e)
thread=threading.Thread(target=vnc_server,daemon=True);thread.start()
request('vnc',f'{host}:{listener.getsockname()[1]}','');jid=str(__import__('guest_integration').serial)
frame=base/f'vnc-{jid}.ppm';deadline=time.monotonic()+60
while not frame.exists():
    if time.monotonic()>deadline:raise TimeoutError((base/f'vnc-{jid}.status').read_text() if (base/f'vnc-{jid}.status').exists() else 'VNC worker did not start')
    time.sleep(.2)
assert frame.read_bytes()==b'P6\n2 2\n255\n'+bytes([255,0,0,0,255,0,0,0,255,255,255,255])
(base/f'vnc-{jid}.events').write_text('key 97\n');thread.join(20);assert not thread.is_alive() and not errors,errors
listener.close();request('cancel',jid)
print('PASS real guest VNC worker, framebuffer and keyboard through NAT',flush=True)
request('shutdown')
print('Network integration and clean shutdown passed.',flush=True)

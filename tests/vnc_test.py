"""Protocol fixtures test fragmentation, authentication and pixel/input bytes."""
import importlib.util, sys, socket, threading, struct, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'guest'))
spec=importlib.util.spec_from_file_location('vnc',root/'guest/verdant-vnc.py');vnc=importlib.util.module_from_spec(spec);spec.loader.exec_module(vnc)
assert vnc.pyDes.des(bytes.fromhex('133457799BBCDFF1')).encrypt(bytes.fromhex('0123456789ABCDEF')).hex()=='85e813540f0ab405'
def exact(s,n):
    b=b''
    while len(b)<n:
        data=s.recv(n-len(b));assert data;b+=data
    return b
for minor,password in [(8,''),(8,'sample'),(7,''),(3,'')]:
    listener=socket.socket();listener.bind(('127.0.0.1',0));listener.listen(1);port=listener.getsockname()[1];errors=[]
    def server():
        try:
            s,_=listener.accept();s.settimeout(5);banner=f'RFB 003.{minor:03d}\n'.encode()
            for c in banner:s.sendall(bytes([c]))
            assert exact(s,12)==banner
            security=2 if password else 1
            if minor==3:s.sendall(struct.pack('>I',security))
            else:s.sendall(bytes([1,security]));assert exact(s,1)==bytes([security])
            if password:
                challenge=bytes(range(16));s.sendall(challenge);assert exact(s,16)==vnc.auth_response(password,challenge)
            if password or minor>=8:s.sendall(b'\0'*4)
            assert exact(s,1)==b'\1'
            s.sendall(struct.pack('>HH',2,2)+b'\0'*16+struct.pack('>I',4)+b'test')
            pixel=exact(s,20);assert pixel[:4]==b'\0'*4 and pixel[4:8]==bytes([32,24,0,1])
            assert exact(s,8)==struct.pack('>BBHi',2,0,1,0)
            assert exact(s,10)==struct.pack('>BBHHHH',3,0,0,0,2,2)
            frame=b'\0\0\0\1'+struct.pack('>HHHHi',0,0,2,2,0)+bytes([0,0,255,0,0,255,0,0,255,0,0,0,255,255,255,0])
            for c in frame:s.sendall(bytes([c]))
            assert exact(s,10)==struct.pack('>BBHHHH',3,1,0,0,2,2)
            assert exact(s,16)==struct.pack('>BBHI',4,1,0,ord('a'))+struct.pack('>BBHI',4,0,0,ord('a'))
            assert exact(s,6)==struct.pack('>BBHH',5,1,1,0)
            s.close()
        except Exception as e:errors.append(e)
    thread=threading.Thread(target=server);thread.start()
    with tempfile.TemporaryDirectory() as temp:
        c=vnc.Client('127.0.0.1',port,password,temp,'1');c.update()
        assert (Path(temp)/'vnc-1.ppm').read_bytes()==b'P6\n2 2\n255\n'+bytes([255,0,0,0,255,0,0,0,255,255,255,255])
        (Path(temp)/'vnc-1.events').write_text('key 97\nptr 1 0 1\n');c.events();thread.join(6);assert not thread.is_alive();assert not errors,errors;c.socket.close()
    listener.close();print(f'PASS RFB 3.{minor}, password={bool(password)}')
print('VNC protocol and DES vector tests passed.')

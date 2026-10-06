#!/usr/bin/env python3
"""Small RFB 3.3/3.7/3.8 client. Raw encoding, bounded framebuffer, keyboard
and pointer events. Classic VNC authentication is supported; TLS is not.
Use a private network or a guest SSH tunnel. Passwords are not saved to disk.
"""
import os, sys, socket, struct, select, time
from pathlib import Path
import pyDes

def auth_response(password,challenge):
    raw=password.encode('latin-1')[:8].ljust(8,b'\0')
    key=bytes(int(f'{v:08b}'[::-1],2) for v in raw)
    return pyDes.des(key).encrypt(challenge)

class Client:
    def __init__(self,host,port,password,base,jid):
        self.base=Path(base);self.jid=jid;self.offset=0
        self.socket=socket.create_connection((host,int(port)),15)
        self.socket.settimeout(15)
        version=self.read(12)
        if not version.startswith(b'RFB 003.'):raise ValueError('Unsupported RFB protocol')
        minor=int(version[8:11]);self.minor=8 if minor>=8 else 7 if minor>=7 else 3
        self.socket.sendall(f'RFB 003.{self.minor:03d}\n'.encode())
        if self.minor==3:security=struct.unpack('>I',self.read(4))[0]
        else:
            n=self.read(1)[0]
            if not n:raise ValueError('VNC server refused connection')
            offered=self.read(n)
            security=2 if password and 2 in offered else 1 if not password and 1 in offered else 0
            if not security:raise ValueError('Set a classic VNC password; VeNCrypt/TLS is unsupported')
            self.socket.sendall(bytes([security]))
        if security==2:
            if not password:raise ValueError('VNC password required')
            self.socket.sendall(auth_response(password,self.read(16)))
        elif security!=1:raise ValueError('Unsupported VNC security type')
        if security==2 or self.minor>=8:
            if struct.unpack('>I',self.read(4))[0]:raise ValueError('VNC authentication failed')
        self.socket.sendall(b'\x01')
        init=self.read(24);self.width,self.height=struct.unpack('>HH',init[:4])
        if not (1<=self.width<=640 and 1<=self.height<=480):raise ValueError('Configure the server desktop to at most 640 x 480')
        name_len=struct.unpack('>I',init[20:24])[0]
        if name_len>16384:raise ValueError('Invalid server name')
        self.read(name_len)
        self.pixels=bytearray(self.width*self.height*3)
        fmt=struct.pack('>BBBBHHHBBBxxx',32,24,0,1,255,255,255,16,8,0)
        self.socket.sendall(b'\0\0\0\0'+fmt)
        self.socket.sendall(struct.pack('>BBHi',2,0,1,0))
        self.request(False)
        (self.base/f'vnc-{jid}.status').write_text('Connected\n')
    def read(self,n):
        result=bytearray()
        while len(result)<n:
            chunk=self.socket.recv(min(65536,n-len(result)))
            if not chunk:raise EOFError('VNC connection closed')
            result.extend(chunk)
        return bytes(result)
    def request(self,incremental):
        self.socket.sendall(struct.pack('>BBHHHH',3,int(incremental),0,0,self.width,self.height))
    def key(self,key):
        self.socket.sendall(struct.pack('>BBHI',4,1,0,key)+struct.pack('>BBHI',4,0,0,key))
    def events(self):
        p=self.base/f'vnc-{self.jid}.events'
        if not p.exists():return
        with p.open('rb') as f:f.seek(self.offset);data=f.read(4096)
        end=data.rfind(b'\n')
        if end<0:return
        self.offset+=end+1
        for line in data[:end].splitlines():
            parts=line.decode().split()
            if parts[0]=='ptr':
                x,y,mask=map(int,parts[1:]);x=max(0,min(self.width-1,x));y=max(0,min(self.height-1,y))
                self.socket.sendall(struct.pack('>BBHH',5,mask,x,y))
            elif parts[0]=='key':
                value=int(parts[1]);keys={13:0xff0d,9:0xff09,27:0xff1b,127:0xff08,8:0xff08}
                if 1<=value<=26 and value not in keys:
                    self.socket.sendall(struct.pack('>BBHI',4,1,0,0xffe3));self.key(value+96);self.socket.sendall(struct.pack('>BBHI',4,0,0,0xffe3))
                else:self.key(keys.get(value,value))
            elif parts[0]=='special':self.key(int(parts[1]))
    def update(self):
        kind=self.read(1)[0]
        if kind==0:
            _,count=struct.unpack('>BH',self.read(3))
            if count>1024:raise ValueError('Too many rectangles')
            for _ in range(count):
                x,y,w,h,encoding=struct.unpack('>HHHHi',self.read(12))
                if encoding!=0 or x+w>self.width or y+h>self.height:raise ValueError('Unsupported or invalid VNC rectangle')
                for row in range(h):
                    raw=self.read(w*4);rgb=bytearray(w*3)
                    rgb[0::3]=raw[2::4];rgb[1::3]=raw[1::4];rgb[2::3]=raw[0::4]
                    start=((y+row)*self.width+x)*3;self.pixels[start:start+w*3]=rgb
            p=self.base/f'vnc-{self.jid}.ppm';temp=p.with_suffix('.part')
            with temp.open('wb') as f:f.write(f'P6\n{self.width} {self.height}\n255\n'.encode());f.write(self.pixels)
            os.replace(temp,p);self.request(True)
        elif kind==2:pass # Bell
        elif kind==3:
            header=self.read(7);n=struct.unpack('>I',header[3:])[0]
            if n>65536:raise ValueError('Clipboard too large')
            self.read(n) # Incoming clipboard intentionally bounded and discarded.
        else:raise ValueError('Unsupported VNC message')
    def run(self):
        while True:
            self.events()
            if select.select([self.socket],[],[],.1)[0]:self.update()

if __name__=='__main__':
    host,port,password,base,jid=sys.argv[1:]
    try:Client(host,port,password,base,jid).run()
    except Exception as e:
        (Path(base)/f'vnc-{jid}.status').write_text(str(e)+'\n')
        print(str(e),file=sys.stderr);sys.exit(1)

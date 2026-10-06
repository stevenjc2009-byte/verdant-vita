"""Validate Vita LiveArea PNG resource format before VPK packaging."""
import struct,sys,zlib,zipfile
from pathlib import Path
SIZES={'sce_sys/icon0.png':(128,128),'sce_sys/livearea/contents/bg.png':(840,500),'sce_sys/livearea/contents/startup.png':(280,158)}
def check(data,name):
    if data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError(name+': invalid PNG')
    offset=8;chunks=[];header=None;palette=None;ended=False
    while offset<len(data):
        if offset+12>len(data):raise ValueError(name+': truncated chunk')
        size,=struct.unpack_from('>I',data,offset);kind=data[offset+4:offset+8];payload=data[offset+8:offset+8+size]
        if len(payload)!=size or offset+12+size>len(data):raise ValueError(name+': truncated payload')
        crc,=struct.unpack_from('>I',data,offset+8+size)
        if zlib.crc32(kind+payload)&0xffffffff!=crc:raise ValueError(name+': corrupt PNG chunk')
        chunks.append(kind)
        if kind==b'IHDR':
            if header is not None or size!=13 or offset!=8:raise ValueError(name+': invalid header')
            header=struct.unpack('>IIBBBBB',payload)
        if kind==b'PLTE':palette=payload
        offset+=size+12
        if kind==b'IEND':ended=True;break
    if not ended or offset!=len(data) or header is None:raise ValueError(name+': incomplete PNG')
    w,h,depth,colour,compression,filtering,interlace=header
    if (w,h)!=SIZES[name]:raise ValueError(name+': wrong size')
    if (depth,colour,compression,filtering,interlace)!=(8,3,0,0,0):raise ValueError(name+': requires non-interlaced 8-bit indexed PNG (0x8010113D prevention)')
    if not palette or len(palette)%3 or len(palette)>768 or b'IDAT' not in chunks:raise ValueError(name+': invalid palette/image data')
    if name!='sce_sys/livearea/contents/startup.png' and b'tRNS' in chunks:raise ValueError(name+': transparency is unsupported')
def main():
    root=Path(__file__).resolve().parents[1]
    if len(sys.argv)>1:
        with zipfile.ZipFile(sys.argv[1]) as z:
            if z.testzip():raise ValueError('Corrupt VPK')
            for n in SIZES:check(z.read(n),n)
    else:
        for n,p in zip(SIZES,[root/'assets/vita-icon0.png',root/'assets/vita-livearea/bg.png',root/'assets/vita-livearea/startup.png']):check(p.read_bytes(),n)
    print('Vita LiveArea PNG dimensions, 8-bit indexed colour, CRC and transparency checks passed.')
if __name__=='__main__':main()

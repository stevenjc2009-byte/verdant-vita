#!/usr/bin/env python3
"""Patch an upstream combined Image offline using Linux debugfs, then rebundle.
Run on Linux/WSL. Never operate on a console's mounted/live disk image.
"""
import argparse, gzip, struct, subprocess, tempfile
from pathlib import Path

def main():
    p = argparse.ArgumentParser()
    p.add_argument('upstream', type=Path); p.add_argument('output', type=Path)
    args = p.parse_args()
    project = Path(__file__).resolve().parents[1]
    data = args.upstream.read_bytes()
    if data[-8:] != b'3DSCLIRF': raise SystemExit('Input must be a combined upstream Image')
    gzlen, rawlen = struct.unpack('<II', data[-16:-8])
    kernel = data[:-16-gzlen]; raw = gzip.decompress(data[-16-gzlen:-16])
    if len(raw) != rawlen: raise SystemExit('Invalid rootfs length')
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)/'rootfs.ext2'; root.write_bytes(raw)
        commands = ['mkdir /usr/local', 'mkdir /usr/local/bin', 'rm /etc/default/dropbear']
        for src,dst,mode in [(project/'guest/verdant-agent.py','/usr/local/bin/verdant-agent.py','0100755'),
                             (project/'guest/verdant_platform.py','/usr/local/bin/verdant_platform.py','0100644'),
                             (project/'guest/verdant-vnc.py','/usr/local/bin/verdant-vnc.py','0100755'),
                             (project/'guest/pyDes.py','/usr/local/bin/pyDes.py','0100644'),
                             (project/'guest/dropbear-default','/etc/default/dropbear','0100644'),
                             (project/'guest/S99verdant','/etc/init.d/S99verdant','0100755')]:
            commands += [f'write "{src}" {dst}', f'set_inode_field {dst} mode {mode}']
        commands += ['write /dev/null /etc/default/verdant']
        script = Path(tmp)/'commands'; script.write_text('\n'.join(commands)+'\n')
        subprocess.run(['debugfs','-w','-f',str(script),str(root)], check=True)
        # Verify both files survived image editing; debugfs can exit 0 on errors.
        for dst in ['/usr/local/bin/verdant-agent.py','/etc/init.d/S99verdant','/usr/bin/python3']:
            check = subprocess.run(['debugfs','-R','stat '+dst,str(root)], capture_output=True,text=True)
            if 'Inode:' not in check.stdout: raise SystemExit('Missing runtime file: '+dst)
        blob = gzip.compress(root.read_bytes(), compresslevel=9, mtime=0)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_bytes(kernel+blob+struct.pack('<II',len(blob),rawlen)+b'3DSCLIRF')
    print('Created',args.output,'with the Verdant guest service')

if __name__ == '__main__': main()

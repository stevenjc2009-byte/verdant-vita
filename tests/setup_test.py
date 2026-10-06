import hashlib,subprocess,sys,tempfile
from pathlib import Path
binary=Path(sys.argv[1]).resolve()
def write(p,data):p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
def run(root,ok=True):
    p=subprocess.run([str(binary)],cwd=root,capture_output=True,text=True)
    assert (p.returncode==0)==ok,p.stdout+p.stderr
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);files={'verdant/Image':b'kernel bundle'*10000,'verdant/guest/verdant-agent.py':b'agent','verdant/guest/github-ca.pem':b'certs'}
    plan='0.2.5\n'+''.join(hashlib.sha256(v).hexdigest()+' '+n+'\n' for n,v in files.items())
    for n,v in files.items():write(root/'app0:/setup-files'/n,v)
    write(root/'app0:/setup-files/manifest.txt',plan.encode())
    write(root/'verdant/rootfs.ext2',b'preserved Linux disk');write(root/'verdant/preferences.cfg',b'preserved settings')
    write(root/'verdant/Image.setup-part',b'interrupted partial copy')
    run(root)
    for n,v in files.items():assert (root/n).read_bytes()==v
    assert (root/'verdant/rootfs.ext2').read_bytes()==b'preserved Linux disk'
    assert (root/'verdant/preferences.cfg').read_bytes()==b'preserved settings'
    assert (root/'verdant/setup-version.txt').read_text().strip()=='0.2.5'
    run(root)
    # Missing service file is repaired on a later launch.
    (root/'verdant/guest/verdant-agent.py').unlink();run(root)
    assert (root/'verdant/guest/verdant-agent.py').read_bytes()==b'agent'
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);write(root/'verdant/preferences.cfg',b'user settings')
    write(root/'app0:/setup-files/manifest.txt',b'0.2.5\n'+b'0'*64+b' verdant/Image\n')
    write(root/'app0:/setup-files/verdant/Image',b'corrupt image')
    run(root,False);assert not (root/'verdant/Image').exists()
    assert (root/'verdant/preferences.cfg').read_bytes()==b'user settings'
print('First-launch install, partial-copy recovery, repair, corrupt-bundle rejection and user-data preservation passed.')

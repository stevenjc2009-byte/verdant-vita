import hashlib,subprocess,sys,tempfile
from pathlib import Path
binary=Path(sys.argv[1]).resolve()
def write(p,data):p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
def stage(root,platform='3ds',bad=False):
    files={'verdant/Image':b'new image','3ds/verdant/verdant.3dsx':b'new executable'}
    pending=root/'verdant/update-pending'
    for n,d in files.items():write(pending/n,d)
    lines=''.join(('0'*64 if bad else hashlib.sha256(d).hexdigest())+' '+n+'\n' for n,d in files.items())
    write(pending/'plan.txt',(platform+'\n0.2.1\n'+lines).encode())
def run(root,ok):
    p=subprocess.run([str(binary)],cwd=root,capture_output=True,text=True)
    assert (p.returncode==0)==ok,p.stdout+p.stderr
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp)
    write(root/'verdant/Image',b'old image');write(root/'3ds/verdant/verdant.3dsx',b'old executable')
    write(root/'verdant/rootfs.ext2',b'user disk');write(root/'verdant/preferences.cfg',b'user preferences')
    stage(root,'vita');run(root,False);assert (root/'verdant/Image').read_bytes()==b'old image'
    stage(root,bad=True);run(root,False);assert (root/'verdant/Image').read_bytes()==b'old image'
    stage(root)
    # Simulate a power loss after the Image rename, before the executable rename.
    backup=root/'verdant/update-backup/0.2.1/verdant/Image';backup.parent.mkdir(parents=True)
    (root/'verdant/Image').rename(backup)
    (root/'verdant/update-pending/verdant/Image').rename(root/'verdant/Image')
    run(root,True)
    assert (root/'verdant/Image').read_bytes()==b'new image'
    assert (root/'3ds/verdant/verdant.3dsx').read_bytes()==b'new executable'
    assert backup.read_bytes()==b'old image'
    assert (root/'verdant/update-backup/0.2.1/3ds/verdant/verdant.3dsx').read_bytes()==b'old executable'
    assert (root/'verdant/rootfs.ext2').read_bytes()==b'user disk'
    assert (root/'verdant/preferences.cfg').read_bytes()==b'user preferences'
    assert not (root/'verdant/update-pending').exists()
    run(root,True)
print('Native hash/platform rejection, interrupted-update recovery and user-data preservation passed.')

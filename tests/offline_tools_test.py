"""Check update integrity, user-data preservation and recoverable offline backups."""
import hashlib,json,subprocess,sys,tempfile,zipfile
from pathlib import Path
project=Path(__file__).resolve().parents[1]
def run(tool,*args,ok=True):
    result=subprocess.run([sys.executable,str(project/'tools'/tool),*map(str,args)],capture_output=True,text=True)
    assert (result.returncode==0)==ok,result.stdout+result.stderr
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);sd=root/'sd';runtime=sd/'verdant';runtime.mkdir(parents=True)
    (runtime/'Image').write_bytes(b'old program')
    (runtime/'rootfs.ext2').write_bytes(b'user filesystem')
    (runtime/'preferences.cfg').write_bytes(b'user settings')
    (runtime/'bridge').mkdir();(runtime/'bridge'/'1.req').write_text('transient')
    package=root/'update.zip'
    def make(content,digest):
        with zipfile.ZipFile(package,'w') as z:
            z.writestr('verdant/Image',content)
            z.writestr('manifest.json',json.dumps({'platform':'vita','sha256':{'verdant/Image':digest}}))
    make(b'bad',hashlib.sha256(b'good').hexdigest())
    run('update_install.py',package,sd,ok=False)
    assert (runtime/'Image').read_bytes()==b'old program'
    make(b'new program',hashlib.sha256(b'new program').hexdigest())
    run('update_install.py',package,sd)
    assert (runtime/'Image').read_bytes()==b'new program'
    assert (runtime/'rootfs.ext2').read_bytes()==b'user filesystem'
    assert (runtime/'preferences.cfg').read_bytes()==b'user settings'
    assert next((runtime/'updates-backup').rglob('Image')).read_bytes()==b'old program'
    backup=root/'backup.zip';run('manage_install.py','backup',sd,'--output',backup)
    with zipfile.ZipFile(backup) as z:
        assert z.read('verdant/rootfs.ext2')==b'user filesystem'
        assert all('/bridge/' not in n for n in z.namelist())
        assert z.testzip() is None
    run('manage_install.py','recover-settings',sd)
    assert next(runtime.glob('preferences.cfg.saved-*')).read_bytes()==b'user settings'
    run('manage_install.py','reset-linux',sd)
    assert next(runtime.glob('rootfs.ext2.saved-*')).read_bytes()==b'user filesystem'
print('Offline update rejection/preservation, backup, settings recovery and disk reset passed.')

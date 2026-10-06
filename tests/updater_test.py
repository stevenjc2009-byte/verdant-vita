"""Reject cross-console/tampered/path-traversal updates; stage only permitted files."""
import hashlib,importlib.util,json,tempfile,zipfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('updater',Path(__file__).resolve().parents[1]/'guest/verdant-updater.py')
u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
assert u.version('v0.2.10')>u.version('v0.2.9')
for url in ['http://github.com/a','https://evil.example/x','https://github.com@evil.example/x']:
    try:u.safe_url(url)
    except ValueError:pass
    else:raise AssertionError('Unexpected URL accepted')
assert not u.allowed('verdant/guest/../../rootfs.ext2','3ds')
assert not u.allowed('verdant.vpk','3ds')
assert not u.allowed('cias/verdant.cia','vita')
with tempfile.TemporaryDirectory() as t:
    root=Path(t);u.RUNTIME=root/'runtime';u.RUNTIME.mkdir()
    def package(platform,broken=False):
        files={'verdant/Image':b'new image','verdant/guest/verdant-updater.py':b'updater','verdant/guest/github-ca.pem':b'certs','3ds/verdant/verdant.3dsx':b'new application'}
        m={'platform':platform,'version':'0.2.1','sha256':{n:hashlib.sha256(v).hexdigest() for n,v in files.items()}}
        if broken:m['sha256']['verdant/Image']='0'*64
        p=root/'update.zip'
        with zipfile.ZipFile(p,'w') as z:
            for n,v in files.items():z.writestr(n,v)
            z.writestr('manifest.json',json.dumps(m))
        return p
    for platform,broken in [('vita',False),('3ds',True)]:
        try:u.stage(package(platform,broken),'3ds','v0.2.1')
        except ValueError:pass
        else:raise AssertionError('Bad package accepted')
        assert not (u.RUNTIME/'update-pending').exists()
    u.stage(package('3ds'),'3ds','v0.2.1')
    plan=(u.RUNTIME/'update-pending/plan.txt').read_text()
    assert plan.startswith('3ds\n0.2.1\n')
    assert (u.RUNTIME/'update-pending/verdant/Image').read_bytes()==b'new image'
    assert not (u.RUNTIME/'Image').exists()
print('Updater platform, version, HTTPS host, traversal, corruption and staging checks passed.')

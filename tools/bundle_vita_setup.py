"""Produce a self-contained VPK while keeping the online update VPK small."""
import hashlib,zipfile
from pathlib import Path
from check_vita_assets import check,SIZES
project=Path(__file__).resolve().parents[1]
version='0.3.1'
source=project/'dist/vita/verdant.vpk'
dest=project/'dist/vita/verdant-setup.vpk'
files={'verdant/Image':project/'Image'}
for p in (project/'guest').iterdir():
    if p.suffix in {'.py','.txt','.pem'}:files['verdant/guest/'+p.name]=p
manifest=version+'\n'+''.join(hashlib.sha256(p.read_bytes()).hexdigest()+' '+n+'\n' for n,p in files.items())
with zipfile.ZipFile(source) as src,zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED) as out:
    for name in src.namelist():out.writestr(name,src.read(name))
    for name,path in files.items():out.write(path,'setup-files/'+name)
    out.writestr('setup-files/manifest.txt',manifest)
with zipfile.ZipFile(dest) as z:
    assert z.testzip() is None
    for n in SIZES:check(z.read(n),n)
    for n,p in files.items():assert hashlib.sha256(z.read('setup-files/'+n)).hexdigest()==hashlib.sha256(p.read_bytes()).hexdigest()
print('Validated self-contained VPK:',dest,dest.stat().st_size,'bytes')

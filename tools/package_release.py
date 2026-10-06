"""Create an install ZIP with a file-integrity manifest and a source archive."""
import argparse, hashlib, json, zipfile
from pathlib import Path
project=Path(__file__).resolve().parents[1]
out=project.parent
def package(platform):
    files={'verdant/Image':project/'Image'}
    for f in (project/'guest').iterdir():
        if f.suffix in ('.py','.txt','.pem'):files['verdant/guest/'+f.name]=f
    if platform=='3ds':
        files.update({'3ds/verdant/verdant.3dsx':project/'verdant.3dsx','3ds/verdant/verdant.smdh':project/'verdant.smdh','cias/verdant.cia':project/'verdant.cia'})
    else:files['verdant.vpk']=project/'dist/vita/verdant.vpk'
    for name in ('README.md','FEATURE-STATUS.md','TEST-RESULTS.md','RESEARCH.md','SOURCE-VERSIONS.md','LICENSE','VITA.md','UPDATES.md'):
        if (project/name).exists():files['docs/'+name]=project/('README-VITA.md' if platform=='vita' and name=='README.md' else name)
    for name in ('manage_install.py','update_install.py'):files['tools/'+name]=project/'tools'/name
    manifest={'version':'0.2.6','platform':platform,'sha256':{n:hashlib.sha256(p.read_bytes()).hexdigest() for n,p in files.items()}}
    dest=out/f'verdant-{platform}-update.zip'
    with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED) as z:
        for name,path in files.items():z.write(path,name)
        z.writestr('manifest.json',json.dumps(manifest,indent=2)+'\n')
    with zipfile.ZipFile(dest) as z:
        assert z.testzip() is None
        for name,digest in manifest['sha256'].items():assert hashlib.sha256(z.read(name)).hexdigest()==digest
    print('Validated install ZIP:',dest)
def source():
    dest=out/'verdant-source-0.2.6.zip'
    excluded={'.git','build','build-vita','dist','__pycache__','.github'}
    with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED) as z:
        for f in project.rglob('*'):
            rel=f.relative_to(project)
            if f.is_file() and not any(part in excluded for part in rel.parts) and f.suffix not in ('.elf','.3dsx','.cia','.smdh','.map','.o','.d','.pyc') and f.name not in ('Image','native-renderer-preview.png'):z.write(f,Path('verdant-desktop')/rel)
    print('Application source ZIP:',dest)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('platform',choices=('3ds','vita','source'));a=p.parse_args()
    source() if a.platform=='source' else package(a.platform)

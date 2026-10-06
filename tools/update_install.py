"""Apply a Verdant install ZIP offline, checking its SHA-256 manifest first.
Preserves Linux disk, settings, keys, history and user documents. CIA installation
still takes place in FBI on the console. Run only with Verdant closed.
"""
import argparse, hashlib, json, shutil, tempfile, zipfile
from pathlib import Path,PurePosixPath
from datetime import datetime
def permitted(name):
    p=PurePosixPath(name)
    if '\\' in name or ':' in name or '..' in p.parts or p.is_absolute():return False
    if name in {'verdant/Image','3ds/verdant/verdant.3dsx','3ds/verdant/verdant.smdh','cias/verdant.cia','verdant.vpk'}:return True
    return len(p.parts)==3 and p.parts[:2]==('verdant','guest') and p.suffix in {'.py','.txt','.pem'}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('package',type=Path);parser.add_argument('sd',type=Path);a=parser.parse_args()
    sd=a.sd.resolve();runtime=(sd/'verdant').resolve()
    if runtime.parent!=sd or not runtime.is_dir():raise SystemExit('Expected an existing verdant installation directly inside the SD root')
    with zipfile.ZipFile(a.package) as z:
        metadata=json.loads(z.read('manifest.json'))
        manifest=metadata['sha256']
        names=[name for name in manifest if permitted(name)]
        if not names or 'verdant/Image' not in names:raise SystemExit('Invalid package manifest')
        # Resolve every target before staging anything; reject traversal/symlinks.
        for name in names:
            target=(sd/name).resolve()
            if sd not in target.parents:raise SystemExit('Target escapes SD root')
            digest=hashlib.sha256()
            with z.open(name) as f:
                while block:=f.read(1024*1024):digest.update(block)
            if digest.hexdigest()!=manifest[name]:raise SystemExit('Hash mismatch: '+name)
        stamp=datetime.now().strftime('%Y%m%d-%H%M%S');backup=runtime/'updates-backup'/stamp
        if backup.exists():raise SystemExit('Backup name already exists')
        with tempfile.TemporaryDirectory(prefix='update-stage-',dir=runtime) as t:
            stage=Path(t).resolve()
            if runtime not in stage.parents:raise SystemExit('Invalid staging directory')
            for name in names:
                staged=stage/name;staged.parent.mkdir(parents=True,exist_ok=True)
                with z.open(name) as source,staged.open('wb') as out:shutil.copyfileobj(source,out)
                if hashlib.sha256(staged.read_bytes()).hexdigest()!=manifest[name]:raise SystemExit('Staged hash mismatch')
            for name in names:
                target=sd/name;target.parent.mkdir(parents=True,exist_ok=True)
                if target.exists():
                    saved=backup/name;saved.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,saved)
                (stage/name).replace(target)
        print('Updated program, image and guest service. Linux disk and preferences preserved.')
        print('Previous program files:',backup)
        if metadata.get('platform')=='vita':
            print('Vita users: reinstall verdant.vpk using VitaShell, then relaunch.')
        else:
            print('CIA users: reinstall cias/verdant.cia using FBI. 3DSX users: relaunch.')
if __name__=='__main__':main()

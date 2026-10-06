"""Offline backups and recovery. Run only after exiting Verdant and removing SD.
No recursive deletion; recovery and reset preserve old files by renaming them.
"""
import argparse, hashlib, json, shutil, zipfile
from pathlib import Path
from datetime import datetime
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('operation',choices=['backup','recover-settings','reset-linux'])
    p.add_argument('sd',type=Path,help='Mounted SD root, e.g. E:/')
    p.add_argument('--output',type=Path,help='Required backup destination, outside the SD runtime')
    a=p.parse_args();sd=a.sd.resolve();runtime=(sd/'verdant').resolve()
    if runtime.parent!=sd or not runtime.is_dir():raise SystemExit('Expected a verdant folder immediately inside the SD root')
    stamp=datetime.now().strftime('%Y%m%d-%H%M%S')
    if a.operation=='backup':
        if not a.output:raise SystemExit('Use --output PATH.zip')
        output=a.output.resolve()
        if runtime==output or runtime in output.parents:raise SystemExit('Choose a destination outside the verdant directory')
        if output.exists():raise SystemExit('Output already exists')
        output.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(output,'x',zipfile.ZIP_DEFLATED) as z:
            for f in runtime.rglob('*'):
                if f.is_file() and not f.is_symlink() and 'bridge' not in f.relative_to(runtime).parts and f.name not in ('swap.img','debug.log','console.log'):
                    z.write(f,Path('verdant')/f.relative_to(runtime))
        print('Offline backup:',output)
    else:
        names=('preferences.cfg','engine.cfg') if a.operation=='recover-settings' else ('rootfs.ext2',)
        for name in names:
            source=runtime/name
            if source.exists():
                target=runtime/(name+'.saved-'+stamp)
                if target.exists():raise SystemExit('Saved file already exists')
                source.rename(target);print('Preserved',target)
        print('Settings will use defaults.' if a.operation=='recover-settings' else 'Next boot extracts a fresh Linux disk from Image. Existing Linux files remain in the saved disk.')
if __name__=='__main__':main()

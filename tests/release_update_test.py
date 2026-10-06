"""Stage and apply the actual release ZIPs on disposable host storage."""
import importlib.util,json,subprocess,sys,tempfile,zipfile
from pathlib import Path
project=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('updater',project/'guest/verdant-updater.py')
u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
for platform,binary in zip(('3ds','vita'),sys.argv[1:3]):
    executable=Path(binary).resolve()
    with tempfile.TemporaryDirectory() as temp:
        sd=Path(temp);u.RUNTIME=sd/'verdant';u.RUNTIME.mkdir()
        (u.RUNTIME/'rootfs.ext2').write_bytes(b'user disk');(u.RUNTIME/'preferences.cfg').write_bytes(b'user settings')
        package=project.parent/('verdant-'+platform+'-update.zip')
        with zipfile.ZipFile(package) as z:version=json.loads(z.read('manifest.json'))['version']
        u.stage(package,platform,'v'+version)
        r=subprocess.run([str(executable)],cwd=sd,capture_output=True,text=True)
        assert r.returncode==0,r.stdout+r.stderr
        assert (u.RUNTIME/'rootfs.ext2').read_bytes()==b'user disk'
        assert (u.RUNTIME/'preferences.cfg').read_bytes()==b'user settings'
        assert (u.RUNTIME/'installed-version.txt').read_text().strip()==version
        app=sd/('app/VRDT00001/eboot.bin' if platform=='vita' else '3ds/verdant/verdant.3dsx')
        assert app.stat().st_size>100000
        print('PASS actual release staging/native application on host storage:',platform,flush=True)

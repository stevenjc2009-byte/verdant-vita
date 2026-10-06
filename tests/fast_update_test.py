"""Small updates reuse the runtime only after native hash validation."""
import hashlib,importlib.util,json,subprocess,sys,tempfile,zipfile,io
from pathlib import Path
project=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('updater',project/'guest/verdant-updater.py');u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
binary=Path(sys.argv[1]).resolve()
image=b'existing unchanged Linux Image'
with tempfile.TemporaryDirectory() as temp:
 root=Path(temp);vpk=io.BytesIO()
 with zipfile.ZipFile(vpk,'w') as z:z.writestr('eboot.bin',b'new executable');z.writestr('sce_sys/param.sfo',b'VRDT00001')
 files={'verdant.vpk':vpk.getvalue(),'verdant/guest/verdant-updater.py':b'updater','verdant/guest/github-ca.pem':b'ca'}
 meta={'platform':'vita','version':'0.4.0','sha256':{n:hashlib.sha256(data).hexdigest() for n,data in files.items()},'reuse':{'verdant/Image':hashlib.sha256(image).hexdigest()}}
 package=root/'fast.zip'
 with zipfile.ZipFile(package,'w') as z:
  for name,data in files.items():z.writestr(name,data)
  z.writestr('manifest.json',json.dumps(meta))
 for valid in (False,True):
  sd=root/str(valid);u.RUNTIME=sd/'verdant';u.RUNTIME.mkdir(parents=True);(u.RUNTIME/'Image').write_bytes(image if valid else b'wrong Image')
  (u.RUNTIME/'rootfs.ext2').write_bytes(b'user Linux disk');target=sd/'app/VRDT00001/eboot.bin';target.parent.mkdir(parents=True);target.write_bytes(b'old executable')
  u.stage(package,'vita','v0.4.0',u.hash_file(package));assert not (u.RUNTIME/'update-pending/verdant/Image').exists()
  result=subprocess.run([str(binary)],cwd=sd,capture_output=True,text=True)
  assert (result.returncode==0)==valid,result.stdout+result.stderr
  assert target.read_bytes()==(b'new executable' if valid else b'old executable')
  assert (u.RUNTIME/'rootfs.ext2').read_bytes()==b'user Linux disk'
print('Small update: matching runtime reuses Image; mismatched runtime is rejected BEFORE replacing the app; Linux disk preserved.')

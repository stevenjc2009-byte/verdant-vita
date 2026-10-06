"""Exercise the real running guest via its SD bridge, not mocked shell output."""
from pathlib import Path
import sys, time
base=Path(sys.argv[1]); serial=1000
def request(op,*args):
    global serial
    serial+=1
    p=base/f'{serial}.part'
    p.write_text(op+'\n'+''.join(str(a).encode().hex()+'\n' for a in args))
    p.replace(base/f'{serial}.req')
    response=base/f'{serial}.res'
    deadline=time.monotonic()+45
    while not response.exists():
        if time.monotonic()>deadline:raise TimeoutError(op)
        time.sleep(.1)
    data=response.read_text();response.unlink()
    assert data.startswith('OK\n'),data
    print('PASS',op, data[3:80].replace('\n',' '),flush=True)
    return data[3:]
prefix='/root/verdant-test-'+str(time.time_ns())
request('write',prefix+'.txt','A real Linux file\nsecond line\n')
assert request('read',prefix+'.txt')=='A real Linux file\nsecond line\n'
request('copy',prefix+'.txt',prefix+'-copy.txt')
assert Path(prefix+'-copy.txt').name in request('list','/root')
request('move',prefix+'-copy.txt',prefix+'-moved.txt')
request('trash',prefix+'-moved.txt')
request('tasks')
request('terminal','0')
with (base/'term-0.in').open('ab') as f:f.write(b"printf '\\nVERDANT_PTY_OK\\n'; command -v bash vim nano htop tree wget busybox ssh scp; uname -r\n")
deadline=time.monotonic()+45
while time.monotonic()<deadline:
    output=(base/'term-0.out').read_text(errors='replace')
    if '6.6.' in output and '/usr/bin/wget' in output:break
    time.sleep(.2)
else:raise AssertionError(output)
print(output,flush=True)
request('close','0')
print('Real guest integration passed.',flush=True)

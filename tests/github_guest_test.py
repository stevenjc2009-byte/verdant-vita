"""Live, read-only GitHub HTTPS checks through the actual RV32 guest NAT."""
import sys,time
from pathlib import Path
base=Path(sys.argv[1]);serial=4000
def request(op,*args):
    global serial
    serial+=1;p=base/(str(serial)+'.part')
    p.write_text(op+'\n'+''.join(str(a).encode().hex()+'\n' for a in args));p.rename(base/(str(serial)+'.req'))
    response=base/(str(serial)+'.res');end=time.monotonic()+90
    while not response.exists():
        if time.monotonic()>end:raise TimeoutError(op)
        time.sleep(.2)
    data=response.read_text();response.unlink();assert data.startswith('OK\n'),data
    return data[3:]
for platform in ('3ds','vita'):
    request('sysupdate','check',platform,'0.2.0');job=str(serial);end=time.monotonic()+240
    while time.monotonic()<end:
        output=request('job',job)
        if output.startswith('exit='):break
        time.sleep(1)
    assert output.startswith('exit=0') and 'Up to date: 0.2.0' in output,output
    print('PASS real guest verified GitHub HTTPS and correct release channel:',platform,flush=True)
request('shutdown')

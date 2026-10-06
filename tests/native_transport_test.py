"""Native transport handoff: cancellation drains old ownership before a new request."""
import importlib.util,tempfile,threading,time
from pathlib import Path
spec=importlib.util.spec_from_file_location('updater',Path(__file__).resolve().parents[1]/'guest/verdant-updater.py')
u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
with tempfile.TemporaryDirectory() as temp:
    u.RUNTIME=Path(temp);bridge=u.RUNTIME/'bridge';bridge.mkdir();base=bridge/'host-http'
    def path(ext):return base.with_name(base.name+ext)
    path('.busy').write_text('old transfer')
    failures=[]
    def worker():
        try:
            deadline=time.monotonic()+5
            while not path('.cancel').exists():
                assert time.monotonic()<deadline;time.sleep(.01)
            assert not path('.req').exists()
            path('.data').write_bytes(b'OLD RESPONSE')
            path('.res').write_text('OK\n');path('.busy').unlink()
            while not path('.req').exists():
                assert time.monotonic()<deadline;time.sleep(.01)
            assert path('.req').read_text().startswith('https://api.github.com/')
            path('.req').unlink();path('.data').write_bytes(b'NEW RESPONSE');path('.res').write_text('OK\n')
        except Exception as e:failures.append(e)
    thread=threading.Thread(target=worker);thread.start()
    assert u.native_fetch('https://api.github.com/repos/test/test/releases/latest')==b'NEW RESPONSE'
    thread.join();assert not failures,failures
    assert not path('.data').exists() and not path('.res').exists()
print('Cancelled native transfer drains before a new request; stale response is rejected by ownership.')

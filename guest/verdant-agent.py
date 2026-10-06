#!/usr/bin/env python3
"""Local SD mailbox bridge. Runs inside Linux, never on a network port.
Requests are atomically renamed *.req files: operation, then hex UTF-8 arguments.
Real terminals use PTYs; minimising/moving a window never restarts its process.
"""
import os, sys, time, json, pty, subprocess, selectors, signal, shutil, fcntl, termios, struct
from pathlib import Path

from verdant_platform import configure
BASE, HARDWARE = configure()
HOME = Path(os.environ.get('HOME', '/root'))
MAX_SESSIONS = 4
sessions = {}
jobs = {}
selector = selectors.DefaultSelector()

def atomic(path, data):
    path = Path(path)
    temp = path.with_name(path.name + '.part')
    with open(temp, 'w', encoding='utf-8') as f:
        f.write(data)
        f.flush()
    os.replace(temp, path)

def check_path(s):
    path = Path(s).resolve()
    # The emulator owns these disk images while Linux is running.
    sd = BASE.parent.parent.resolve()
    runtimes = {BASE.parent, Path('/mnt/3ds/sd/verdant')}
    if path in {root / name for root in runtimes for name in ('Image', 'rootfs.ext2', 'swap.img')}:
        raise ValueError('Runtime images must be managed while the app is closed')
    if any(path == root / 'bridge' or root / 'bridge' in path.parents for root in runtimes):
        raise ValueError('The service mailbox is reserved')
    return path

def check_mutable(s):
    path=check_path(s)
    if path in BASE.parents or path in {Path(p) for p in ('/','/root','/bin','/sbin','/usr','/etc','/lib','/dev','/proc','/sys','/mnt')}:
        raise ValueError('This directory contains a running system or reserved mailbox')
    return path

def terminal(sid, command='/bin/bash -l'):
    if sid in sessions:
        return 'Session already running'
    if len(sessions) >= MAX_SESSIONS:
        raise ValueError('Four-session limit reached')
    pid, master = pty.fork()
    if pid == 0:
        os.chdir(HOME)
        os.environ.update(TERM='xterm-256color', HISTFILE=str(HOME / '.bash_history'))
        if command == '/bin/bash -l':
            rc=Path(__file__).with_name('verdant-bashrc.txt')
            if rc.is_file():os.execv('/bin/bash',['bash','--noprofile','--rcfile',str(rc),'-i'])
        os.execv('/bin/sh', ['sh', '-c', command])
    os.set_blocking(master, False)
    fcntl.ioctl(master,termios.TIOCSWINSZ,struct.pack('HHHH',30,80,0,0))
    out = BASE / f'term-{sid}.out'
    out.write_bytes(b'')
    (BASE / f'term-{sid}.in').write_bytes(b'')
    sessions[sid] = {'pid': pid, 'fd': master, 'offset': 0, 'out': out}
    selector.register(master, selectors.EVENT_READ, sid)
    return 'Terminal started'

def stop_session(sid):
    session = sessions.pop(sid, None)
    if session:
        selector.unregister(session['fd'])
        os.close(session['fd'])
        try:
            os.killpg(session['pid'], signal.SIGHUP)
        except ProcessLookupError:
            pass
        try:
            os.waitpid(session['pid'], os.WNOHANG)
        except ChildProcessError:
            pass

def launch_job(jid, command):
    if jid in jobs and jobs[jid]['process'].poll() is None:
        raise ValueError('Job already running')
    out = open(BASE / f'job-{jid}.out', 'wb')
    proc = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT,
                            start_new_session=True)
    jobs[jid] = {'process': proc, 'out': out}
    return f'Job {jid} started'

def dispatch(op, a, rid):
    if op=='currency':
        script=BASE.parent/'guest'/'verdant-currency.py'
        if not script.is_file():raise ValueError('Currency service is absent; install the full update')
        if any((j.get('updater') or j.get('currency')) and j['process'].poll() is None for j in jobs.values()):raise ValueError('Another verified download is running')
        result=launch_job(rid,[sys.executable,'-u',str(script)])
        jobs[rid].update(currency=True,started=time.monotonic(),timeout=90)
        return result
    if op=='sysupdate':
        action,platform,current=a[:3]
        if action not in ('check','stage') or platform not in ('3ds','vita'):raise ValueError('Invalid updater request')
        script=BASE.parent/'guest'/'verdant-updater.py'
        if not script.is_file():raise ValueError('Updater is absent from SD; install the complete release')
        if any((j.get('updater') or j.get('currency')) and j['process'].poll() is None for j in jobs.values()):raise ValueError('An updater is already running')
        result=launch_job(rid,[sys.executable,'-u',str(script),action,platform,current]);jobs[rid]['updater']=True;jobs[rid]['started']=time.monotonic();jobs[rid]['timeout']=900 if action=='stage' else 90;return result
    if op == 'shutdown':
        for sid in list(sessions):stop_session(sid)
        subprocess.run(['sync'],check=True)
        subprocess.Popen(['poweroff'])
        return 'Linux is shutting down'
    if op == 'terminal':
        return terminal(a[0], a[1] if len(a) > 1 and a[1] else '/bin/bash -l')
    if op == 'close':
        stop_session(a[0]); return 'Session closed'
    if op == 'list':
        path = check_path(a[0])
        trash=HOME/'.local/share/Trash/files'
        if path==trash:trash.mkdir(parents=True,exist_ok=True)
        rows = sorted(path.iterdir(), key=lambda p: (not p.is_dir(), p.name.lower()))
        if path==trash:rows=[p for p in rows if not p.name.endswith('.origin')]
        return '\n'.join(('D ' if p.is_dir() else 'F ') + p.name.replace('\n', '?')
                         for p in rows[:128])
    if op == 'read':
        path=check_path(a[0])
        if path.stat().st_size>15000:raise ValueError('Text editor limit is 15000 bytes; use Vim or Nano for larger files')
        raw=path.read_bytes()
        if b'\0' in raw:raise ValueError('Binary file; use the image viewer or a terminal')
        return raw.decode('utf-8')
    if op == 'image':
        path=check_path(a[0])
        if path.stat().st_size>4*1024*1024:raise ValueError('Image file limit is 4 MB')
        name=f'image-{rid}.bin'
        shutil.copyfile(path,BASE/name)
        return name
    if op == 'write':
        atomic(check_mutable(a[0]), a[1]); return 'Saved'
    if op in ('copy', 'move'):
        src, dst = check_mutable(a[0]), check_mutable(a[1])
        if dst.exists():
            raise ValueError('Destination exists; choose a different name')
        if op == 'copy':
            shutil.copytree(src, dst) if src.is_dir() else shutil.copy2(src, dst)
        else:
            shutil.move(str(src), str(dst))
        return 'Complete'
    if op == 'trash':
        src = check_mutable(a[0])
        trash = HOME / '.local/share/Trash/files'
        trash.mkdir(parents=True, exist_ok=True)
        target = trash / (str(time.time_ns()) + '-' + src.name[:210])
        shutil.move(str(src), target)
        atomic(str(target) + '.origin', str(src))
        return 'Moved to trash'
    if op == 'restore':
        src = check_path(a[0]); origin = Path(str(src) + '.origin')
        target = check_mutable(origin.read_text())
        if target.exists(): raise ValueError('Original path already exists')
        shutil.move(str(src), target); origin.unlink(); return 'Restored'
    if op == 'mkdir':
        check_mutable(a[0]).mkdir(exist_ok=True); return 'Folder ready'
    if op == 'search':
        root = check_path(a[0]); result = []
        for directory, folders, files in os.walk(root, followlinks=False):
            folders[:] = [n for n in folders if not n.startswith('.')]
            for name in folders + files:
                if a[1].lower() in name.lower():
                    path=Path(directory)/name;result.append(('D ' if path.is_dir() else 'F ')+str(path))
                if len(result) >= 128: return '\n'.join(result)
        return '\n'.join(result) or 'No matches'
    if op == 'performance':
        import importlib
        return importlib.import_module('verdant-performance').sample()
    if op == 'tasks':
        result = subprocess.run(['ps', '-eo', 'pid,comm,rss'], capture_output=True, text=True)
        return Path('/proc/meminfo').read_text().split('\n')[0] + '\n' + result.stdout[:15000]
    if op == 'download':
        if not a[0].startswith(('https://', 'http://')): raise ValueError('Use HTTP or HTTPS')
        target=check_mutable(a[1])
        if target.exists():raise ValueError('Destination exists; choose a new path')
        temp=target.with_name(target.name+'.verdant-'+rid+'.part')
        result=launch_job(rid,['wget','-O',str(temp),a[0]])
        jobs[rid]['target']=target;jobs[rid]['temp']=temp
        return result
    if op == 'vnc':
        host,port=a[0].rsplit(':',1)
        if not host or not 1<=int(port)<=65535:raise ValueError('Enter host:port')
        return launch_job(rid,[sys.executable,str(Path(__file__).with_name('verdant-vnc.py')),host,port,a[1],str(BASE),rid])
    if op == 'job':
        job = jobs.get(a[0])
        if not job: raise ValueError('Unknown job')
        status = 'running' if job['process'].poll() is None else f"exit={job['process'].returncode}"
        log = BASE / f'job-{a[0]}.out'
        with log.open('rb') as f:
            f.seek(max(0, log.stat().st_size - 12000))
            tail = f.read().decode(errors='replace')
        return status + '\n' + tail
    if op == 'cancel':
        if jobs.get(a[0],{}).get('updater') or jobs.get(a[0],{}).get('currency'):(BASE/'host-http.cancel').write_text('1')
        job = jobs.get(a[0])
        if job and job['process'].poll() is None:
            os.killpg(job['process'].pid, signal.SIGTERM)
        return 'Cancelled'
    if op == 'shell':
        return terminal(a[0], a[1])
    if op == 'ssh':
        import shlex
        host, user, key = a[1:4]
        if not host or host.startswith('-') or not user or user.startswith('-'):
            raise ValueError('Enter a host and username')
        command = ['ssh']
        if key: command += ['-i', str(check_path(key))]
        command += [user + '@' + host]
        return terminal(a[0], shlex.join(command))
    if op == 'scp':
        # Destination is supplied as user@host:path; options cannot be injected.
        if a[1].startswith('-'): raise ValueError('Invalid destination')
        return launch_job(rid, ['scp', str(check_path(a[0])), a[1]])
    if op == 'smb':
        import shlex
        if not shutil.which('smbclient'): raise ValueError('smbclient is absent from this image')
        return terminal(a[0], shlex.join(['smbclient', a[1], '-U', a[2]]))
    if op == 'packages':
        return terminal(a[0], '/bin/sh -c "opkg list-installed; exec /bin/bash -l"')
    if op == 'pkginstall':
        import shlex
        path=check_path(a[1])
        if not path.is_file() or path.suffix!='.ipk':raise ValueError('Type the path to a compatible RV32 .ipk file')
        return terminal(a[0],shlex.join(['opkg','install',str(path)])+'; exec /bin/bash -l')
    if op == 'status':
        hw = HARDWARE
        values = []
        for name in ('battery', 'charging', 'wifi', 'network'):
            try: values.append(name + '=' + (hw/name).read_text().strip())
            except OSError: pass
        v = os.statvfs('/'); values.append('linux_free=' + str(v.f_bavail * v.f_frsize))
        return '\n'.join(values)
    raise ValueError('Unsupported operation: ' + op)

def tick():
    active=False
    # Native HTTPS has its own host-http.req mailbox in this directory.
    requests=(p for p in BASE.glob('*.req') if p.stem.isascii() and p.stem.isdigit())
    for req in sorted(requests, key=lambda p: int(p.stem))[:8]:
        active=True
        rid = req.stem
        try:
            lines = req.read_text().splitlines()
            op = lines[0]
            args = [bytes.fromhex(s).decode('utf-8') for s in lines[1:]]
            response = dispatch(op, args, rid)
            atomic(BASE / (rid + '.res'), 'OK\n' + response)
        except Exception as e:
            atomic(BASE / (rid + '.res'), 'ERROR\n' + str(e))
        finally:
            req.unlink(missing_ok=True)
    for sid, session in list(sessions.items()):
        path = BASE / ('term-' + sid + '.in')
        try:
            with open(path, 'rb') as f:
                f.seek(session['offset']); data = f.read(4096)
            if data:
                active=True
                sent = os.write(session['fd'], data)
                session['offset'] += sent
        except (FileNotFoundError, BlockingIOError): pass
        except OSError: stop_session(sid)
    for key, _ in selector.select(0):
        session = sessions.get(key.data)
        if not session: continue
        try:
            data = os.read(key.fd, 4096)
            if not data: stop_session(key.data); continue
            active=True
            with open(session['out'], 'ab') as f: f.write(data)
        except BlockingIOError: pass
        except OSError: stop_session(key.data)
    for job in jobs.values():
        if (job.get('updater') or job.get('currency')) and job['process'].poll() is None and time.monotonic()-job['started']>job['timeout']:
            os.killpg(job['process'].pid,signal.SIGTERM)
            job['out'].write(b'\nUpdate timed out. Check Vita Wi-Fi and system date/time.\n');job['out'].flush()
            (BASE/'host-http.cancel').write_text('1')
        if job['process'].poll() is not None and not job['out'].closed:
            job['out'].close()
            if 'target' in job and job['process'].returncode==0:
                if job['target'].exists():
                    with open(BASE / f"job-{next(k for k,v in jobs.items() if v is job)}.out",'ab') as f:f.write(b'\nDestination appeared; downloaded .part retained\n')
                else:job['temp'].rename(job['target'])

    return active

def main():
    BASE.mkdir(parents=True, exist_ok=True)
    # Old mailbox inputs must never be replayed into a new shell.
    for p in BASE.glob('term-*.in'): p.unlink()
    atomic(BASE / 'ready', 'Verdant bridge 1\n')
    try:
        while True:
            # Quiet shells do not need a permanent 50 Hz Python polling loop.
            time.sleep(0.02 if tick() else 0.10)
    finally:
        for sid in list(sessions): stop_session(sid)
        (BASE / 'ready').unlink(missing_ok=True)

if __name__ == '__main__': main()

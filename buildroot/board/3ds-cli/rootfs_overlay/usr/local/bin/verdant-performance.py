"""Measured Linux guest metrics. Physical console metrics come from native APIs."""
import os,time
from pathlib import Path
_last=None
_roles={}

def process_role(name, command):
    """Describe the actual program/arguments, never infer a role from a PID."""
    if 'verdant-agent.py' in command:return 'Desktop service: terminals and Linux files'
    if 'verdant-updater.py' in command:return 'Updater: checks, downloads and verifies'
    if 'verdant-currency.py' in command:return 'Calculator: downloads currency rates'
    if 'verdant-vnc.py' in command:return 'Remote desktop connection'
    if name.startswith('jbd2/'):return 'Filesystem journal: commits disk writes'
    if name.startswith('kworker'):return 'Kernel background work'
    return {'ext4lazyinit':'Filesystem initialization in the background','syslogd':'System log collector','klogd':'Kernel log collector',
            'init':'Starts services and cleans up exited processes',
            'bash':'Interactive terminal shell','sh':'Shell or startup script',
            'dropbear':'SSH server or connection','sshd':'SSH server',
            'ssh':'Connection to another computer','wget':'File download',
            'udhcpc':'Obtains a network address','getty':'Console login prompt',
            'python3':'Python program (script not identified)',
            'python':'Python program (script not identified)'}.get(name,'Linux program or kernel worker')

def sample():
    global _last,_roles
    now=time.monotonic()
    ticks=os.sysconf('SC_CLK_TCK'); page=os.sysconf('SC_PAGE_SIZE')//1024
    cpu=list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:9]))
    memory={}
    for line in Path('/proc/meminfo').read_text().splitlines():
        name,value,*_=line.split();memory[name.rstrip(':')]=int(value)
    net=[0,0]
    for line in Path('/proc/net/dev').read_text().splitlines()[2:]:
        name,data=line.split(':',1)
        if name.strip()!='lo':
            values=data.split();net[0]+=int(values[0]);net[1]+=int(values[8])
    disk=[0,0]
    for line in Path('/proc/diskstats').read_text().splitlines():
        v=line.split()
        if v[2]=='vda':disk=[int(v[5])*512,int(v[9])*512]
    processes={}
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():continue
        try:
            stat=(entry/'stat').read_text();end=stat.rfind(')');v=stat[end+2:].split()
            name=stat[stat.find('(')+1:end].replace('|','?').replace('\n','?')
            pid=int(entry.name);identity=(int(v[19]),name)
            cached=_roles.get(pid)
            if not cached or cached[0]!=identity:
                try:command=(entry/'cmdline').read_bytes()[:1024].replace(b'\0',b' ').decode(errors='replace')
                except OSError:command=''
                cached=(identity,process_role(name,command));_roles[pid]=cached
            processes[pid]=(int(v[11])+int(v[12]),max(0,int(v[21]))*page,name,cached[1])
        except (OSError,ValueError,IndexError):pass
    _roles={pid:role for pid,role in _roles.items() if pid in processes}
    usage=-1; rates=[-1]*4
    if _last:
        before=_last;dt=now-before['time'];total=sum(cpu)-sum(before['cpu'])
        if total>0:usage=max(0,min(100,100-(cpu[3]+cpu[4]-before['cpu'][3]-before['cpu'][4])*100/total))
        if dt>0:rates=[max(0,(value-old)/dt/1024) for value,old in zip(net+disk,before['net']+before['disk'])]
    total=memory.get('MemTotal',0);available=memory.get('MemAvailable',memory.get('MemFree',0))
    fs=os.statvfs('/')
    rows=['CPU|%.1f'%usage,'MEM|%d|%d'%(total,max(0,total-available)),
          'NET|%.1f|%.1f'%tuple(rates[:2]),'DISK|%.1f|%.1f|%d|%d'%(rates[2],rates[3],fs.f_blocks*fs.f_frsize,fs.f_bavail*fs.f_frsize),
          'PROCESSES|%d'%len(processes)]
    scored=[]
    for pid,(clock,rss,name,role) in processes.items():
        percent=-1
        if _last and pid in _last['processes'] and now>_last['time']:
            percent=max(0,(clock-_last['processes'][pid][0])*100/ticks/(now-_last['time']))
        scored.append((rss,pid,percent,name,role))
    for rss,pid,percent,name,role in sorted(scored,key=lambda row:(row[2],row[0]),reverse=True)[:100]:
        rows.append('PROC|%d|%.1f|%d|%s|%s'%(pid,percent,rss,name,role))
    _last=dict(time=now,cpu=cpu,net=net,disk=disk,processes=processes)
    return '\n'.join(rows)

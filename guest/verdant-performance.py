"""Measured Linux guest metrics. Physical console metrics come from native APIs."""
import os,time
from pathlib import Path
_last=None

def sample():
    global _last
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
            processes[int(entry.name)]=(int(v[11])+int(v[12]),max(0,int(v[21]))*page,stat[stat.find('(')+1:end].replace('|','?').replace('\n','?'))
        except (OSError,ValueError,IndexError):pass
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
    for pid,(clock,rss,name) in processes.items():
        percent=-1
        if _last and pid in _last['processes'] and now>_last['time']:
            percent=max(0,(clock-_last['processes'][pid][0])*100/ticks/(now-_last['time']))
        scored.append((rss,pid,percent,name))
    for rss,pid,percent,name in sorted(scored,reverse=True)[:100]:
        rows.append('PROC|%d|%.1f|%d|%s'%(pid,percent,rss,name))
    _last=dict(time=now,cpu=cpu,net=net,disk=disk,processes=processes)
    return '\n'.join(rows)

#!/usr/bin/env python3
"""Verified GitHub release staging. Active guest disk/programs are never replaced here."""
import os,re,sys,time
from pathlib import Path,PurePosixPath

VERSION='0.3.0'
REPOS={'3ds':'stevenjc2009-byte/verdant-3ds','vita':'stevenjc2009-byte/verdant-vita'}
LIMIT=160*1024*1024
RUNTIME=Path(os.environ.get('VERDANT_RUNTIME',str(Path(__file__).resolve().parent.parent)))

def atom(path,data):
    tmp=path.with_name(path.name+'.part')
    with tmp.open('w') as f:f.write(data);f.flush();os.fsync(f.fileno())
    tmp.replace(path)

def version(v):
    m=re.fullmatch(r'v?(\d+)\.(\d+)\.(\d+)',v)
    if not m:raise ValueError('Stable release must have a vMAJOR.MINOR.PATCH tag')
    return tuple(map(int,m.groups()))

def safe_url(url):
    # Exact authorities only: no credentials, custom ports or ambiguous hosts.
    if not re.fullmatch(r'https://(api.github.com|github.com|release-assets.githubusercontent.com|objects.githubusercontent.com)/[^\x00-\x20]*',url):
        raise ValueError('Unexpected HTTPS release host')
    return url

def opener():
    import ssl,urllib.request
    class GithubRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self,req,fp,code,msg,headers,newurl):
            safe_url(newurl)
            return super().redirect_request(req,fp,code,msg,headers,newurl)
    context=ssl.create_default_context(cafile=str(RUNTIME/'guest'/'github-ca.pem'))
    return urllib.request.build_opener(GithubRedirect(),urllib.request.HTTPSHandler(context=context))

def native_fetch(url,target=None,limit=1024*1024):
    bridge=RUNTIME/'bridge';base=bridge/'host-http'
    def path(ext):return base.with_name(base.name+ext)
    if path('.req').exists() or path('.busy').exists():
        path('.cancel').write_text('1')
        print('Waiting for the previous HTTPS transfer to stop...',flush=True)
        deadline=time.monotonic()+35
        while path('.req').exists() or path('.busy').exists():
            if time.monotonic()>deadline:raise TimeoutError('Previous HTTPS transfer is still stopping; retry shortly')
            time.sleep(.2)
    for ext in ('.res','.cancel','.progress','.data'):path(ext).unlink(missing_ok=True)
    atom(path('.req'),safe_url(url)+'\n')
    print('Connecting to GitHub using Vita HTTPS...',flush=True)
    started=time.monotonic();last=''
    while not path('.res').exists():
        if time.monotonic()-started>650:
            path('.cancel').write_text('1')
            raise TimeoutError('Native HTTPS timed out. Check Vita Wi-Fi and date/time.')
        try:progress=path('.progress').read_text()
        except FileNotFoundError:progress=''
        if progress and progress!=last:print(progress,flush=True);last=progress
        time.sleep(.2)
    response=path('.res').read_text()
    if not response.startswith('OK\n'):raise RuntimeError(response.removeprefix('ERROR\n'))
    if path('.data').stat().st_size>limit:raise ValueError('Release exceeds size limit')
    if target:
        with path('.data').open('rb') as source:
            while block:=source.read(65536):target.write(block)
        result=b''
    else:result=path('.data').read_bytes()
    path('.data').unlink();path('.res').unlink()
    return result

def fetch_once(client,url,target=None,limit=1024*1024):
    import urllib.request
    request=urllib.request.Request(safe_url(url),headers={'User-Agent':'Verdant-Updater/'+VERSION,'Accept':'application/vnd.github+json'})
    with client.open(request,timeout=45) as response:
        safe_url(response.url)
        count=0;data=bytearray();last=0
        while True:
            block=response.read(32768)
            if not block:break
            count+=len(block)
            if count>limit:raise ValueError('Release asset exceeds size limit')
            if target:target.write(block)
            else:data.extend(block)
            if target and time.monotonic()-last>3:
                print('Downloaded %d KiB'%(count//1024),flush=True);last=time.monotonic()
        return bytes(data)

def fetch(client,url,target=None,limit=1024*1024):
    if (RUNTIME/'bridge/host-http.enabled').exists():return native_fetch(url,target,limit)
    import ssl,urllib.error
    for attempt in range(3):
        try:
            if target:target.seek(0);target.truncate()
            return fetch_once(client,url,target,limit)
        except urllib.error.HTTPError as e:
            if e.code<500 or attempt==2:raise
        except (urllib.error.URLError,TimeoutError,ConnectionError,ssl.SSLError) as e:
            reason=getattr(e,'reason',e)
            if isinstance(reason,ssl.SSLCertVerificationError) or attempt==2:raise
        print('Connection interrupted; retrying verified HTTPS',flush=True)
        time.sleep(2**(attempt+1))

def allowed(name,platform):
    if '\\' in name or ':' in name or '..' in PurePosixPath(name).parts:return False
    binaries={'3ds':{'3ds/verdant/verdant.3dsx','3ds/verdant/verdant.smdh','cias/verdant.cia'},'vita':{'verdant.vpk'}}
    return name=='verdant/Image' or name in binaries[platform] or bool(re.fullmatch(r'verdant/guest/[A-Za-z0-9_.-]+\.(py|txt|pem)',name))

def hash_file(path):
    import hashlib
    h=hashlib.sha256()
    with path.open('rb') as f:
        while block:=f.read(32768):h.update(block)
    return h.hexdigest()

def stage(package,platform,tag,asset_digest=None):
    import hashlib,json,zipfile
    if asset_digest and hash_file(package)!=asset_digest:raise ValueError('GitHub asset SHA-256 mismatch')
    pending=RUNTIME/'update-pending'
    if pending.exists():
        if (pending/'plan.txt').exists():raise ValueError('An update is already staged; relaunch or use recovery first')
        pending.rename(RUNTIME/('update-consumed-'+str(time.time_ns())))
    # Incomplete directories are ignored. A cancelled download never creates pending.
    temp=RUNTIME/('update-stage-'+str(time.time_ns()));temp.mkdir()
    records=[]
    with zipfile.ZipFile(package) as z:
        if len(z.infolist())>160 or sum(i.file_size for i in z.infolist())>LIMIT:raise ValueError('Oversized update contents')
        if len(set(z.namelist()))!=len(z.namelist()):raise ValueError('Duplicate archive paths')
        meta=json.loads(z.read('manifest.json'))
        if meta.get('platform')!=platform or meta.get('version')!=tag.lstrip('v'):raise ValueError('Wrong platform/version package')
        manifest=meta['sha256']
        selected=[n for n in manifest if allowed(n,platform)]
        required={'verdant/Image','verdant/guest/verdant-updater.py','verdant/guest/github-ca.pem'}
        required.add('verdant.vpk' if platform=='vita' else '3ds/verdant/verdant.3dsx')
        if not required.issubset(selected):raise ValueError('Incomplete update package')
        for n in selected:
            if not re.fullmatch('[0-9a-f]{64}',manifest[n]):raise ValueError('Invalid digest')
            out=temp/n;out.parent.mkdir(parents=True,exist_ok=True)
            h=hashlib.sha256()
            with z.open(n) as source,out.open('wb') as dest:
                while block:=source.read(32768):dest.write(block);h.update(block)
                dest.flush();os.fsync(dest.fileno())
            if h.hexdigest()!=manifest[n]:raise ValueError('Hash mismatch: '+n)
            records.append((h.hexdigest(),n))
    if platform=='vita':
        # Update this application's own installed files only; preserve the bubble identity.
        with zipfile.ZipFile(temp/'verdant.vpk') as vpk:
            if len(vpk.infolist())>50 or sum(i.file_size for i in vpk.infolist())>16*1024*1024:raise ValueError('Oversized VPK')
            if len(set(vpk.namelist()))!=len(vpk.namelist()):raise ValueError('Duplicate VPK files')
            if b'VRDT00001' not in vpk.read('sce_sys/param.sfo'):raise ValueError('Wrong Vita application identity')
            for name in vpk.namelist():
                if not re.fullmatch(r'(eboot\.bin|sce_sys/(param\.sfo|icon0\.png|livearea/contents/(bg\.png|startup\.png|template\.xml)))',name):raise ValueError('Unexpected VPK file')
                n='app/VRDT00001/'+name;out=temp/n;out.parent.mkdir(parents=True,exist_ok=True)
                with vpk.open(name) as source,out.open('wb') as dest:
                    while block:=source.read(32768):dest.write(block)
                    dest.flush();os.fsync(dest.fileno())
                records.append((hash_file(out),n))
    atom(temp/'plan.txt',platform+'\n'+tag.lstrip('v')+'\n'+''.join(h+' '+n+'\n' for h,n in records))
    os.sync();temp.rename(pending);os.sync()
    print('Verified '+tag+'. Relaunch Verdant to apply. User files and Linux disk are preserved.',flush=True)

def main():
    action,platform,current=sys.argv[1:4]
    if platform not in REPOS:raise ValueError('Unsupported console')
    version(current)
    print('Checking GitHub releases for '+platform+' (current '+current+')...',flush=True)
    import json
    client=None if (RUNTIME/'bridge/host-http.enabled').exists() else opener()
    release=json.loads(fetch(client,'https://api.github.com/repos/'+REPOS[platform]+'/releases/latest'))
    tag=release['tag_name']
    if release.get('prerelease') or release.get('draft'):raise ValueError('Unstable release rejected')
    if version(tag)<=version(current):print('Up to date: '+current,flush=True);return
    print('Update available: '+tag+'\nRepository: '+REPOS[platform],flush=True)
    if action=='check':return
    if action!='stage':raise ValueError('Unknown updater action')
    expected='verdant-'+platform+'-update.zip'
    asset=next((a for a in release['assets'] if a['name']==expected),None)
    if not asset:raise ValueError('Release is missing '+expected)
    if asset['size']>LIMIT:raise ValueError('Update exceeds size limit')
    url=asset['browser_download_url'];safe_url(url)
    if not url.startswith('https://github.com/'+REPOS[platform]+'/releases/download/'):raise ValueError('Asset belongs to another repository')
    if shutil_free()<asset['size']*3+32*1024*1024:raise ValueError('Insufficient free storage for update and backup')
    package=RUNTIME/'github-update.zip.part'
    with package.open('wb') as f:fetch(client,url,f,LIMIT);f.flush();os.fsync(f.fileno())
    digest=asset.get('digest','')
    if not re.fullmatch(r'sha256:[0-9a-f]{64}',digest):raise ValueError('Release requires a GitHub SHA-256 digest')
    stage(package,platform,tag,digest[7:]);package.unlink();os.sync()

def shutil_free():
    import shutil
    return shutil.disk_usage(RUNTIME).free

if __name__=='__main__':
    try:main()
    except Exception as e:print('Update failed: '+str(e),flush=True);sys.exit(1)

#!/usr/bin/env python3
"""ECB daily reference-rate retrieval. Rates are dated and cached, never invented."""
import os,re,sys,time,math,importlib.util
from pathlib import Path
URL='https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml'
RUNTIME=Path(os.environ['VERDANT_RUNTIME'])
def validate(url):
    if url!=URL:raise ValueError('Unexpected currency rate URL')
    return url

def parse(data):
    text=data.decode('utf-8')
    stamp=re.search(r"<Cube\s+time=['\"](\d{4}-\d{2}-\d{2})['\"]",text)
    if not stamp:raise ValueError('ECB response has no reference date')
    rates={'EUR':1.0}
    for code,value in re.findall(r"<Cube\s+currency=['\"]([A-Z]{3})['\"]\s+rate=['\"]([0-9.]+)['\"]",text):
        number=float(value)
        if not math.isfinite(number) or number<=0:raise ValueError('Invalid ECB rate')
        rates[code]=number
    if not {'EUR','GBP','USD','JPY','AUD','CAD','CHF','CNY'}.issubset(rates):raise ValueError('Incomplete ECB reference rates')
    return 'RATES|'+stamp.group(1)+'\n'+''.join('%s|%.12g\n'%(code,value) for code,value in rates.items())

def main():
    print('Downloading dated ECB reference rates...',flush=True)
    if (RUNTIME/'bridge/host-http.enabled').exists():
        spec=importlib.util.spec_from_file_location('transport',Path(__file__).with_name('verdant-updater.py'))
        transport=importlib.util.module_from_spec(spec);spec.loader.exec_module(transport)
        data=transport.native_fetch(URL,limit=65536,validator=validate)
    else:
        import ssl,urllib.request
        context=ssl.create_default_context(cafile=str(RUNTIME/'guest/github-ca.pem'))
        with urllib.request.urlopen(URL,context=context,timeout=30) as response:
            validate(response.url);data=response.read(65537)
        if len(data)>65536:raise ValueError('Oversized ECB response')
    rates=parse(data)
    part=RUNTIME/'currency-rates.part';part.write_text(rates);part.replace(RUNTIME/'currency-rates.txt')
    print(rates,flush=True)
if __name__=='__main__':
    try:main()
    except Exception as e:print('Currency refresh failed: '+str(e),flush=True);sys.exit(1)

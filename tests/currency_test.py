"""Currency parsing rejects missing/invalid rates and retains a provider date."""
import importlib.util,os,tempfile
from pathlib import Path
with tempfile.TemporaryDirectory() as temp:
 os.environ['VERDANT_RUNTIME']=temp
 spec=importlib.util.spec_from_file_location('currency',Path(__file__).resolve().parents[1]/'guest/verdant-currency.py')
 c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
 xml=b"<Cube time='2026-10-05'>"+b''.join(("<Cube currency='%s' rate='1.25'/>"%code).encode() for code in ['GBP','USD','JPY','AUD','CAD','CHF','CNY'])+b'</Cube>'
 result=c.parse(xml);assert result.startswith('RATES|2026-10-05\nEUR|1\n') and 'GBP|1.25' in result
 for invalid in [b'<Cube/>',xml.replace(b'1.25',b'0'),xml.replace(b"currency='GBP'",b"currency='ZZZ'")]:
  try:c.parse(invalid)
  except ValueError:pass
  else:raise AssertionError('Invalid rates accepted')
 try:c.validate(c.URL+'?redirect=evil')
 except ValueError:pass
 else:raise AssertionError('Unexpected URL accepted')
print('Dated currency parsing, completeness, positive-rate and URL restrictions passed.')

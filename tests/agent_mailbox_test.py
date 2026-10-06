"""Keep native transport requests separate from numeric desktop commands."""
import importlib.util,sys,tempfile,types
from pathlib import Path
with tempfile.TemporaryDirectory() as temp:
 bridge=Path(temp);platform=types.ModuleType('verdant_platform');platform.configure=lambda:(bridge,bridge/'hw')
 sys.modules['verdant_platform']=platform
 spec=importlib.util.spec_from_file_location('agent',Path(__file__).resolve().parents[1]/'guest/verdant-agent.py')
 agent=importlib.util.module_from_spec(spec);spec.loader.exec_module(agent)
 native=bridge/'host-http.req';native.write_text('https://api.github.com/native-request\n')
 (bridge/'not-a-number.req').write_text('ignored')
 (bridge/'１２.req').write_text('ignored')
 for i in range(3):
  request=bridge/f'{i+1}.req';request.write_text('unsupported-op\n');agent.tick()
  assert not request.exists() and (bridge/f'{i+1}.res').read_text().startswith('ERROR\nUnsupported operation')
  assert native.read_text()=='https://api.github.com/native-request\n'
 assert (bridge/'not-a-number.req').exists() and (bridge/'１２.req').exists()
print('Native HTTPS requests stay untouched while numeric desktop commands complete across service ticks.')

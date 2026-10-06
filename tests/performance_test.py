"""Read real /proc counters; first rates are unknown until a second sample."""
import importlib.util,time
from pathlib import Path
spec=importlib.util.spec_from_file_location('metrics',Path(__file__).resolve().parents[1]/'guest/verdant-performance.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
a=m.sample();assert 'CPU|-1.0' in a and 'NET|-1.0|-1.0' in a
assert 'PROC|' in a and 'MEM|' in a and 'DISK|' in a
time.sleep(.03)
b=m.sample();assert 'CPU|-1.0' not in b
cpu=float(next(line.split('|')[1] for line in b.splitlines() if line.startswith('CPU|')))
assert 0<=cpu<=100
memory=next(line.split('|') for line in b.splitlines() if line.startswith('MEM|'))
assert int(memory[1])>=int(memory[2])>=0
print('Measured Linux CPU, memory, processes, network and disk counters passed.')

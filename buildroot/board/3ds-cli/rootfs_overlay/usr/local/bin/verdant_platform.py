"""Select console-specific guest paths while preserving older Linux disks."""
import os
import subprocess
from pathlib import Path

def bind(source, target):
    source, target = Path(source), Path(target)
    if not source.is_dir():
        raise RuntimeError('Console storage mount is unavailable: ' + str(source))
    target.mkdir(parents=True, exist_ok=True)
    mounted = any(line.split()[4] == str(target)
                  for line in Path('/proc/self/mountinfo').read_text().splitlines())
    if not mounted:
        subprocess.run(['mount', '--bind', str(source), str(target)], check=True)

def configure(legacy=Path('/mnt/3ds'), mount=bind):
    # VERDANT_BRIDGE overrides are used by disposable host tests.
    override = os.environ.get('VERDANT_BRIDGE')
    if override:
        return Path(override), Path(os.environ.get('VERDANT_HW', str(legacy / 'hw')))
    bridge = legacy / 'sd/verdant/bridge'
    try:
        platform = (bridge / 'platform.txt').read_text().strip()
    except FileNotFoundError:
        platform = '3ds'
    if platform not in ('3ds', 'vita'):
        raise RuntimeError('Unsupported console profile')
    storage, hw = legacy / 'sd', legacy / 'hw'
    if platform == 'vita':
        storage, hw = Path('/mnt/vita/ux0'), Path('/mnt/vita/hw')
        mount(legacy / 'sd', storage)
        mount(legacy / 'hw', hw)
        # Older persistent disks retain the upstream init script. Change only
        # its known status messages; its mount protocol and user files stay intact.
        init = Path('/etc/init.d/S02passthrough')
        if init.is_file():
            text = init.read_text()
            lines = text.splitlines(keepends=True)
            for i, line in enumerate(lines):
                if line.strip().startswith('echo "3DS passthrough on $DIR:'):
                    lines[i] = '\t\techo "PS Vita storage bridge ready (ux0:)"\n'
                elif line.strip().startswith('echo "3DS passthrough: unavailable'):
                    lines[i] = line.replace('3DS passthrough', 'PS Vita storage bridge')
            updated = ''.join(lines)
            if updated != text:
                part = init.with_name(init.name + '.verdant-part')
                part.write_text(updated)
                part.chmod(init.stat().st_mode & 0o777)
                part.replace(init)
    bridge = storage / 'verdant/bridge'
    os.environ.update(VERDANT_PLATFORM=platform, VERDANT_BRIDGE=str(bridge),
                      VERDANT_RUNTIME=str(storage / 'verdant'), VERDANT_HW=str(hw))
    return bridge, hw

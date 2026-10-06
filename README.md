# Verdant Desktop for PS Vita — experimental 0.3.0

Install the full **Verdant-PS-Vita-v0.3.0.vpk** with VitaShell, replacing the existing Verdant application. Launch its bubble and wait for automatic first-launch setup. No separate ZIP extraction is required. Keep at least 500 MB free, preferably 1 GB. Existing Linux files, preferences and shell history are preserved. Local Buildroot login: **root**, empty password unless you changed it.

Version 0.3.0 adds a 150% default desktop size, saved global scaling from 100% to 200%, graphical Settings cards, and a Processes/Performance task manager. Its Vita hardware pages use measured kernel idle clocks, current CPU/GPU clock queries, free memory pools, heap usage, Wi-Fi state/signal and ux0 capacity. Linux process/RAM/network/disk counters are separate. Unsupported GPU utilization, memory clock/type, base frequency and physical drive/link speeds show unavailable.

Version 0.3.0 reduces pointer-only redraws to the cursor area, accelerates rectangle fills, removes long terminal rendering locks and pipelines frame transfer on a separate CPU core. UI/input use core 0, display/media core 1 and Linux emulation core 2. The fourth core remains reserved for the system; the interpreted guest still has one virtual CPU. Host pixel/cursor and concurrent handoff tests pass, but physical Vita performance needs confirmation.

Version 0.2.6 supplies Vita-specific storage paths, controls, keyboard help, sensor identification and hardware settings. The earlier startup display failure and desktop flashing fixes remain included. Console testing of the new profile is still needed.

- Native storage: **ux0:/verdant/**; title ID: **VRDT00001**.
- Linux view of ux0: **/mnt/vita/ux0/**. Hardware files: **/mnt/vita/hw/**.
- Cross confirms/clicks; Circle closes keyboard/menu; Square switches windows; Triangle moves windows between desktop regions; left stick moves the pointer.
- Select opens the menu; Select+Square toggles the keyboard; Select+Triangle changes workspaces; Start requests Linux shutdown.
- Vita uses its 960×544 display, front touchscreen, Vita motion/power/network APIs and ux0: storage. Nintendo NAND/TWL settings are excluded.
- Built-in updates use **stevenjc2009-byte/verdant-vita**. The update ZIP and full standalone VPK are separate release assets.

Older Linux disks mount a shared internal 9p transport under /mnt/3ds. A real bind mount gives Vita its own paths without resetting the disk. Existing scripts using the old paths continue to work, and saved bookmarks migrate to the Vita path. The filename and internal transport are compatibility details of the shared RV32 Linux runtime.

This is a Vita homebrew application with an interpreted RV32 Linux guest and a native lightweight desktop. Read [VITA.md](VITA.md) for installation/build details, [FEATURE-STATUS.md](FEATURE-STATUS.md) for implemented features and limitations, [UPDATES.md](UPDATES.md) for update/recovery behavior and [TEST-RESULTS.md](TEST-RESULTS.md) for validation.

Rebuild with VitaSDK: `make -f mk/vita.mk -j4`, then `python tools/bundle_vita_setup.py` with the patched Image present. The resulting full installer is `dist/vita/verdant-setup.vpk`; its release name is `verdant.vpk`.

The Vita updater now uses verified native HTTPS on a worker thread. It prints connection/download progress and reports timeouts. The update ZIP remains compatible with the 0.2.6 staging/apply format. If the old updater cannot connect on your Vita, installing the full VPK over the same title is the fallback; Linux files are preserved.

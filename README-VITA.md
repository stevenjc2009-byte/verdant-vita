# Verdant Desktop for PS Vita — experimental 0.2.6

Install the full **verdant.vpk** with VitaShell, replacing the existing Verdant application. Launch its bubble and wait for automatic first-launch setup. No separate ZIP extraction is required. Keep at least 500 MB free, preferably 1 GB. Existing Linux files, preferences and shell history are preserved. Local Buildroot login: **root**, empty password unless you changed it.

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

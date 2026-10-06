# Verdant for PlayStation Vita

This repository publishes the **vita** update channel. Release 0.2.2 corrects VitaShell installation error 0x8010113D by using indexed LiveArea PNGs. Releases use `verdant-vita-update.zip`. See [VITA.md](VITA.md) and [UPDATES.md](UPDATES.md).

# Verdant Desktop â€” experimental 0.2.2 (Vita image-format fix)

An original dark-green desktop for real Linux inside a homebrew application, based on 3DS-CLI 5.3. **This is an experimental build, not a complete Linux Mint port. No physical console has been tested.** See FEATURE-STATUS.md and TEST-RESULTS.md.

The 3DS release includes both CIA and 3DSX builds. The subsequent PS Vita release includes a VPK; see VITA.md for its installation, controls and research.

## Install on 3DS/2DS

1. Extract the 3DS install ZIP to the SD root, retaining its directory structure.
2. Launch `3ds/verdant/verdant.3dsx` in Homebrew Launcher, or install `cias/verdant.cia` with FBI and open its Home Menu icon. Both use the same Linux disk.
3. Allow first boot to extract the approximately 192 MB Linux disk. Leave at least **500 MB free**, preferably more. Older models can boot and execute commands slowly.
4. Configure Wi-Fi in the console's own settings. The guest interface is configured automatically.

It installs an application and SD files; it does not replace Luma3DS, Nintendo firmware or Home Menu. Its runtime is under `sdmc:/verdant/`, separate from upstream 3DS-CLI's SD-root files. CIA title ID is `000400000F912200`, distinct from upstream. NAND/TWL access is disabled. All Old/New 3DS and 2DS models are architectural targets of the ARMv6 build, not a physically verified compatibility list.

Linux files persist in `verdant/rootfs.ext2` (an ext4 filesystem despite the inherited name). The actual SD card appears at `/mnt/3ds/sd`. Never manipulate the active disk/swap images while running.

## Controls

| Input | Action |
|---|---|
| Touch | Point/click on lower screen; Settings Pad enables relative touchpad control of either screen |
| Circle Pad + A | Move pointer/click; hold A to drag titles or select terminal text |
| X / Y | Switch window / move focused window to the other screen without restarting |
| Select + Y | Swap window screen assignments |
| Select | Application launcher |
| Select + Right / ZL | Next of four workspaces |
| Select + Circle Pad | Pan current workspace's windows |
| ZR / terminal Key button | Keyboard; older models can click Key using Circle Pad + A |
| B | Hide keyboard/menu |
| L / R | Terminal zoom |
| D-pad | Terminal arrows, editor cursor, or text-panel scrolling |
| Start | Sync/poweroff Linux and return; second press forces exit |
| Hold B when launching | Recovery console with default engine settings |

Title controls: `v` move screen, `_` minimize, `[]` maximize/restore, `X` close. Drag lower-right corner to resize. Joined mode spans the two-screen coordinates with missing lower-screen width clipped, never stretched. Join in Settings constrains dragging to a screen. Terminals preserve their running process when moved or minimized; closing sends SIGHUP.

Tap SYM/ABC for punctuation. Control/Alt/Shift, Tab/Escape/Enter/Backspace and navigation are included. Terminal clipboard is application-local. The editor supports cursor navigation and Ctrl+C/V/S, with a **15,000-byte UTF-8** limit; use Vim/Nano for larger files.

Files: Open/Up/Copy/Paste/Trash; Cut/Name/Find/Mark/SD/Root. **More** exposes New folder, Restore, Bin, Path and bookmark access. Name asks for a full destination path. Find searches below the current directory. Mark saves one shared bookmark. Trash stores original-path metadata under `/root/.local/share/Trash/files`.

## Network and applications

SSH opens a real terminal. Key selects an optional guest private key; Save keeps one connection profile. SCP transfers work in terminals. Downloads uses wget jobs with output/progress and cancellation. Existing destinations are not overwritten; failed/cancelled downloads retain `.part` files.

Incoming SSH forwards port **2222** on the console's LAN address. **Blank-password network login is disabled**. Use `passwd` or install `/root/.ssh/authorized_keys` before connecting: `ssh -p 2222 root@CONSOLE_IP`. The local console starts as root with a blank password; network authentication is separate.

Remote desktop uses VNC/RFB 3.3â€“3.8, raw pixels and classic VNC password authentication. Set server desktop to **640Ã—480 or smaller**; enter `host:5900`, optionally Pass, then Connect. Touch image to left-click; Keys opens keyboard. Linux/Windows VNC servers are supported in principle; real desktop-server interoperability remains untested. No Microsoft RDP, TLS/VeNCrypt, incoming clipboard, right-click or remote dragging. Classic VNC is unencrypted; use a private LAN or guest SSH tunnel. Passwords are not saved in preferences.

PNG/JPEG/BMP viewing is limited to 640Ã—480. Media Cam/Inner captures a 3DS snapshot. Rec/Stop saves `verdant/recording.wav`; Play plays that recording. Arbitrary music/video formats are not implemented. Physical media hardware is untested.

Bash, Vim, Nano, htop, tree, wget, BusyBox, SSH and SCP were verified in the guest. **winget is a Windows tool**; compatible Linux packages use opkg. Packages offers local RV32 ILP32 `.ipk` installation and configured-feed commands; there is no maintained Verdant feed or Ubuntu/Mint package compatibility. Local X11/Wayland and ordinary Linux GUI applications remain unimplemented. Use a remote computer for those applications.

## Backup, update, recovery

**Built-in GitHub updates:** open Settings â†’ Upd (or System update). Check queries your console's repository; Update downloads and verifies the latest stable release; Auto+ enables automatic staging of newer releases. Exit cleanly and relaunch to apply, then launch again to run the new executable. See UPDATES.md for repository links, recovery, verification and installed-title permission limitations. Install this 0.2.0 build once to gain the updater.

Backups makes a timestamped `/root` tar archive on SD. Close the app for a complete offline backup or update:

```text
python tools/manage_install.py backup E:/ --output C:/Backups/verdant.zip
python tools/manage_install.py recover-settings E:/
python tools/manage_install.py reset-linux E:/
python tools/update_install.py NEW-INSTALL.zip E:/
```

Replace E:/ with the SD root. Recovery/reset preserve old files by renaming them. Reset extracts a fresh Linux disk next boot; old documents remain in the saved disk. Offline updates check SHA-256 hashes, back up replaced program files and preserve the Linux disk/preferences. CIA users reinstall offline updates in FBI. Boot can use updated scripts in `verdant/guest/`, allowing service updates without resetting Linux.

## Build and credits

Use devkitPro devkitARM, libctru, zlib/curl/mbedTLS 3DS portlibs, bannertool and makerom. Set DEVKITPRO/DEVKITARM. Run `make -j4`, then **separately** `make cia`. `python tools/check3dsx.py verdant.3dsx` validates relocations. On Linux/WSL, `python3 tools/patch_image.py UPSTREAM-5.3-Image Image` patches the image; the Buildroot overlay contains the same service. UPSTREAM-README.md retains upstream runtime build instructions.

Application sources and vendored dependencies are included; full Linux/Buildroot package source downloads are separate upstream packages. Runtime source configuration pins and dependencies are in SOURCE-VERSIONS.md.

Credits: [3DS-CLI](https://github.com/cmdada/3DS-CLI), [mini-rv32ima-mmu](https://github.com/cmdada/mini-rv32ima-mmu), [ctr-osk-rt](https://github.com/cmdada/ctr-osk-rt), [libctru](https://github.com/devkitPro/libctru), stb_image, [pyDes](https://github.com/twhiteman/pyDes) (MIT notice included). This modified application is GPL-3.0 under LICENSE; dependency notices are retained. Unaffiliated with Nintendo, Sony or Linux Mint.

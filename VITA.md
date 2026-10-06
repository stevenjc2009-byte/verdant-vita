# Verdant Desktop for PS Vita — experimental 0.2.5

Version 0.2.5 renders into cached memory and copies completed frames into alternating CDRAM display buffers, switching on vertical blank. This addresses the reported screen flashing during desktop/cursor refreshes. Analog pointer movement is faster and uses elapsed time with fractional pixels, so its speed remains consistent across redraw rates. Rendering and pointer tests pass on the host; physical smoothness needs confirmation.

At the Buildroot console login, enter **root**. The bundled local account has an empty password; press Enter if prompted. Existing user-set passwords remain unchanged.

Version 0.2.4 corrects the startup display failure recorded in the console boot log: 0x80290006 (invalid framebuffer update timing). Display initialization now schedules the framebuffer on the next frame, then waits for vertical blank before drawing. Physical launch after this fix still needs confirmation.

Version 0.2.3 provides a **self-contained, one-click setup VPK**. It contains the Linux image, service scripts and HTTPS certificates. First launch copies and verifies these files, then extracts the Linux filesystem and boots it automatically. No separate ZIP extraction or manual folder creation is required. The earlier thin VPKs did not include the runtime.

Version 0.2.2 fixes the reported VitaShell installation error **0x8010113D**. All three LiveArea PNGs are now non-interlaced, 8-bit indexed images. The build checks resource dimensions, PNG checksums, palette format and transparency before and after VPK packaging. Versions 0.2.1 and earlier used RGB images and should be replaced with this build for installation.

Reference: [Vita LiveArea asset format guide](https://gist.github.com/hammerill/64411eebf071b93396b7d310ba8d6776), [developer error-code notes](https://gist.github.com/devnoname120/a565bea1b7f38393f220ec34f82ed6ba). The original RGB archive was reproduced as a validation failure; the indexed replacement passes. Actual installation still needs confirmation on the console.

The 3DS build was developed and packaged first. This second target shares the desktop, guest service and RV32 Linux runtime, with a VitaSDK platform adapter. It is a compiled homebrew application, not a replacement for Vita firmware and not native Linux Mint. No physical Vita has been tested.

## Installation

1. Transfer the standalone `verdant.vpk` from the latest GitHub release to your homebrew-enabled Vita. The full VPK is approximately 57 MB.
2. Install it with VitaShell, accepting replacement of the existing Verdant application if requested. Launch the **Verdant Desktop** bubble (title ID `VRDT00001`).
3. Leave it running through automatic runtime setup and extraction of the approximately 192 MB Linux disk. Keep at least 500 MB free, preferably 1 GB. First launch can take several minutes; progress is displayed.
4. Configure networking in the Vita's settings. Linux uses the same NAT bridge and startup configuration as the 3DS build.

The runtime is `ux0:/verdant/`. For compatibility with the shared guest image, this storage appears inside Linux at **`/mnt/3ds/sd`**, including on Vita. Linux files and shell history live in the persistent `rootfs.ext2` disk. Read README.md for SSH authentication, downloads, VNC, packages, backup and recovery instructions.

To rebuild, install VitaSDK and its required portlibs, set `VITASDK` and put its `bin` directory on PATH, then run `make -f mk/vita.mk -j4`. This produces the thin application VPK at `dist/vita/verdant.vpk`. With the patched `Image` present, run `python tools/bundle_vita_setup.py` to produce the standalone installer at `dist/vita/verdant-setup.vpk`. The GitHub standalone asset is named `verdant.vpk`; the online update ZIP intentionally retains a thin VPK because it already supplies runtime files separately. Do not bundle the toolchain into the application.

Setup preserves existing Linux disks, preferences and user documents. Partial copies are verified before installation and can retry on relaunch. Program files from a previous setup are preserved under `verdant/setup-backup/`. The startup diagnostic log is `ux0:/verdant/boot.log`. A boot error waits for a new Start press after the launch button has been released, rather than consuming the launch press as an exit command. The native heap is capped at 128 MB and guest RAM at 96 MB in this conservative build.

## Screen and controls

The Vita uses one 960×544 canvas. It has upper and lower logical window regions; the lower region also hosts the touchscreen keyboard. There are no two physical screens to swap. Moving a terminal between regions preserves its process. Four workspaces, window focus, resizing, minimizing and maximizing use the shared desktop implementation.

| Control | Action |
|---|---|
| Front touch | Pointer and desktop buttons; optional relative touchpad mode |
| Left stick + Cross | Pointer movement and click/drag |
| Square | Switch window |
| Triangle | Move focused window between logical regions |
| Select | Launcher |
| Select + Square | Keyboard toggle |
| Select + Triangle / Select + Right | Next workspace |
| Select + left stick | Workspace panning |
| L / R | Terminal zoom |
| Circle | Hide keyboard/menu |
| Start | Request Linux sync and shutdown; second press forces exit |

Rear touch is not integrated. Physical controls, sleep/resume and LiveArea transitions require device testing.

## Media implementation and limits

Camera snapshots use the Vita camera API, convert a 320×240 frame to RGB565 and letterbox it within the shared viewer. Microphone recording uses the 16 kHz capture API and resamples into the shared WAV format. Audio output runs on a separate thread and resamples into the Vita's 48 kHz stereo output. These adapters compile but their physical operation is unverified. Camera/microphone are disabled for the PlayStation TV model; arbitrary music/video playback remains unfinished.

The guest is interpreted RV32 Linux rather than native ARM Linux. Expect substantial performance limits. There is no X11/Wayland environment or general Linux graphical-application support in this build. The native desktop supplies its own small graphical applications. The feature-status table applies to both targets unless a console-specific qualification is stated.

## Research

- [VitaSDK](https://vitasdk.org/) provides homebrew compilation and VPK packaging. This build uses channel 2026.08. The VPK installs a normal homebrew bubble and contains the executable and LiveArea assets.
- [xerpi's bare-metal Linux loader](https://github.com/xerpi/vita-baremetal-linux-loader) is a different approach: it expects a separately built kernel and device tree under `ux0:/linux`, with UART debugging instructions. Its repository does not supply the complete requested desktop distribution. Porting a modern kernel and hardware drivers would be a separate project.
- [vita-moonlight](https://github.com/xyzz/vita-moonlight) documents release 0.13.2 and Sunshine streaming support. It is an existing separate application worth evaluating for a smoother remote desktop; it is not bundled or integrated into Verdant.
- The camera, microphone and audio implementation follows the [VitaSDK API documentation](https://docs.vitasdk.org/) and its [camera sample](https://github.com/vitasdk/samples/tree/master/camera).

Neither the native Linux loader nor Moonlight can simply supply missing local Linux graphics drivers to the shared emulated guest. The practical near-term work is hardware validation, interface refinement and improving the application/runtime integration.

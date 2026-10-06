# Verdant Desktop for PS Vita — experimental 0.3.1

Version 0.3.1 optimizes interactive rendering: pointer-only movement restores/redraws a 7x10 cursor area, rectangle fills clip once per surface and write bulk rows, and terminal rendering uses a brief locked snapshot. The UI polls every 2 ms between frames. Work is split across core 0 (UI/input), core 1 (frame transfer/display and media) and core 2 (the single-vCPU Linux interpreter). Frame ownership is synchronized, the display queue cannot accumulate old frames, and worker-creation failure falls back to synchronous presentation. A fourth Vita core is system-reserved. This does not turn the guest into SMP Linux. Physical drag smoothness remains to be measured.

Version 0.2.6 adds a Vita-specific profile: real Linux bind mounts at **/mnt/vita/ux0** and **/mnt/vita/hw**, ux0: file-manager/storage labels, Vita keyboard and Cross/Circle/left-stick help, PS Vita sensor identification, Vita RAM choices through 96 MB, and hidden Nintendo NAND/TWL settings. Backups and updater workers use the Vita runtime path. Saved 3DS-named bookmarks are migrated. Existing root disks, preferences and shell history are preserved.

The updater is pinned to stevenjc2009-byte/verdant-vita, the installed title is VRDT00001, and the native filesystem adapter uses ux0:. The old /mnt/3ds transport remains internally for persistent-disk compatibility; it is an alias for the same host files. Old boot status messages are adjusted on the next startup. No Nintendo NAND or TWL is exported on Vita.

Version 0.2.5 renders into cached memory and copies completed frames into alternating CDRAM display buffers, switching on vertical blank. This addresses the reported screen flashing during desktop/cursor refreshes. Analog pointer movement is faster and uses elapsed time with fractional pixels, so its speed remains consistent across redraw rates. Rendering and pointer tests pass on the host; physical smoothness needs confirmation.

At the Buildroot console login, enter **root**. The bundled local account has an empty password; press Enter if prompted. Existing user-set passwords remain unchanged.

Version 0.2.4 corrects the startup display failure recorded in the console boot log: 0x80290006 (invalid framebuffer update timing). Display initialization now schedules the framebuffer on the next frame, then waits for vertical blank before drawing. Physical launch after this fix still needs confirmation.

Version 0.2.3 provides a **self-contained, one-click setup VPK**. It contains the Linux image, service scripts and HTTPS certificates. First launch copies and verifies these files, then extracts the Linux filesystem and boots it automatically. No separate ZIP extraction or manual folder creation is required. The earlier thin VPKs did not include the runtime.

Version 0.2.2 fixes the reported VitaShell installation error **0x8010113D**. All three LiveArea PNGs are now non-interlaced, 8-bit indexed images. The build checks resource dimensions, PNG checksums, palette format and transparency before and after VPK packaging. Versions 0.2.1 and earlier used RGB images and should be replaced with this build for installation.

Reference: [Vita LiveArea asset format guide](https://gist.github.com/hammerill/64411eebf071b93396b7d310ba8d6776), [developer error-code notes](https://gist.github.com/devnoname120/a565bea1b7f38393f220ec34f82ed6ba). The original RGB archive was reproduced as a validation failure; the indexed replacement passes. Actual installation still needs confirmation on the console.

The 3DS build was developed and packaged first. This second target shares the desktop, guest service and RV32 Linux runtime, with a VitaSDK platform adapter. It is a compiled homebrew application, not a replacement for Vita firmware and not native Linux Mint. The user confirmed the application reaches the Vita desktop; hardware functions and performance have not been independently tested.

## Installation

1. Transfer the standalone `verdant.vpk` from the latest GitHub release to your homebrew-enabled Vita. The full VPK is approximately 57 MB.
2. Install it with VitaShell, accepting replacement of the existing Verdant application if requested. Launch the **Verdant Desktop** bubble (title ID `VRDT00001`).
3. Leave it running through automatic runtime setup and extraction of the approximately 192 MB Linux disk. Keep at least 500 MB free, preferably 1 GB. First launch can take several minutes; progress is displayed.
4. Configure networking in the Vita's settings. Linux uses the same NAT bridge and startup configuration as the 3DS build.

The runtime is `ux0:/verdant/`. Inside Linux this storage appears at **`/mnt/vita/ux0`**. The shared internal `/mnt/3ds/sd` transport remains as a compatibility alias. Linux files and shell history live in the persistent `rootfs.ext2` disk. Read README.md for SSH authentication, downloads, VNC, packages, backup and recovery instructions.

To rebuild, install VitaSDK and its required portlibs, set `VITASDK` and put its `bin` directory on PATH, then run `make -f mk/vita.mk -j4`. This produces the thin application VPK at `dist/vita/verdant.vpk`. With the patched `Image` present, run `python tools/bundle_vita_setup.py` to produce the standalone installer at `dist/vita/verdant-setup.vpk`. The GitHub standalone asset is named `verdant.vpk`; the online update ZIP intentionally retains a thin VPK because it already supplies runtime files separately. Do not bundle the toolchain into the application.

Setup preserves existing Linux disks, preferences and user documents. Partial copies are verified before installation and can retry on relaunch. Program files from a previous setup are preserved under `verdant/setup-backup/`. The startup diagnostic log is `ux0:/verdant/boot.log`. A boot error waits for a new Start press after the launch button has been released, rather than consuming the launch press as an exit command. The native heap is capped at 128 MB and guest RAM at 96 MB in this conservative build.

## Screen and controls

The Vita uses one 960×544 canvas. It has upper and lower logical window regions; the lower region also hosts the touchscreen keyboard. There are no two physical screens to swap. Moving a terminal between regions preserves its process. Four workspaces, window focus, resizing, minimizing and maximizing use the shared desktop implementation.

| Control | Action |
|---|---|
| Front touch | Direct application input; optional relative pad on empty desktop space |
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

## New in Vita 0.3.2

Vita finger input targets application buttons directly, independently of the joystick cursor. Drag/resize also uses finger coordinates. The optional relative pad works on empty desktop space; app controls, launcher and keyboard stay direct touch. The cursor hides during a touch. The left stick has one-third of its former maximum speed and a quadratic precision curve near center.

The Terminal toolbar now has a **Keyboard** button. Its replacement keyboard uses a larger QWERTY layout, wide Shift/Enter/Space/Backspace keys, numbers, punctuation, Ctrl/Alt/Escape and navigation keys, without function keys. New terminals start Bash directly with a small startup file, preserved history and user `.bashrc`, rather than automatically running neofetch. `neofetch` and `bash -l` remain available manually.

Task Manager puts process names on the left, then CPU %, RAM MiB and PID, with an explanation derived from the actual program and script. Per-process GPU/network throughput is labeled unavailable. Core 2 runs the one-vCPU RV32 interpreter; the native desktop is C code, while Python handles Linux bridge/background services. Physical core percentages and guest process CPU percentages measure different work.

The display worker tries CPU 3 (CapUnlocker) and falls back to CPU 1 when denied. Task Manager reports its selected core. The CPU clock policy preserves clocks at 444 MHz or higher, including a user-selected 500 MHz clock. It does not force an unsupported overclock. Independent native work uses multiple physical cores; the guest remains one virtual CPU.

Supported ux0 file listing, small text open/save, mkdir, file copy and move/rename run on an asynchronous native worker. Linux-only paths, directory copy, search/trash and other complex operations still use the guest. Document saves retain the previous file before committing because Vita rename is not an atomic replacement. An interrupted `.verdant-save-backup` or `.verdant-save-part` remains recoverable.

Idle half-second redraws are removed. Covered windows and unchanged job output do not trigger unnecessary painting. Cursor-only transfer/scaling uses damage regions across both alternating buffers. Graph samples and scale maps are cached. Quiet guest services poll at 10 Hz rather than keeping every idle terminal at 50 Hz. Native HTTPS retains DNS/TLS/connection caches. Networking starts without a 15-second Wi-Fi association wait.

The release retains the full compatible `verdant-vita-update.zip`. An installed 0.3.1 updater uses that package once. Once 0.3.2 is running, later same-runtime releases can use `verdant-vita-fast-update.zip` and reuse Image only after native hash validation, avoiding another roughly 56 MB runtime download. The standalone VPK remains self-contained.

VitaSDK builds and sanitized host/regression tests pass. The actual RV32 Linux guest starts an interactive fast terminal and returns process roles on the host fixture. The prompt appeared in 0.414 seconds on that PC fixture, not a measured Vita time. Physical FPS, touch accuracy, post-update CPU load and CapUnlocker behavior still require console testing.

## Vita 0.3.3 input, Notepad and games

Keyboard key dimensions and layout are unchanged. Touch highlights a candidate, permits sliding to correct it, retains the candidate through small boundary jitter and types on release. Leaving the keys cancels. Joystick clicks stay immediate. The cursor is now a white arrow with a black outline; damage-based cursor restoration remains enabled.

Notepad has File (New, Open, Save, Save As) and Edit (Select All, Copy, Cut, Paste) menus, select-all highlighting/replacement and a clipboard large enough for a supported small document. Open/Save As use full-path keyboard entry. Arbitrary range selection, undo/redo, a graphical file picker and unsaved-change confirmation are not yet implemented.

The desktop's Terminal, Files, Notepad, Calculator and Games shortcuts are omitted from Start. Games contains native Snake, Falling Blocks and Brick Breaker, touch/D-pad input, pause/restart and persistent high scores. Game launch markers are created in `ux0:/verdant/games/` and open through Files. They run within Verdant without a Linux game process. Hidden/minimized games pause their updates. Doom is not bundled; see [FEATURE-LIST-VITA.md](FEATURE-LIST-VITA.md) for future options and native engine research.

CPU charts identify each physical core's role. The Linux interpreter remains on Core 2 with one virtual CPU; this release does not claim equal utilization or a fix for the reported 81% plateau. VitaSDK and sanitized host tests cover keyboard correction/release/cancellation, cursor restoration, Notepad menus/save/clipboard, native game board/collision/rotation limits and layouts. Physical Vita accuracy and speed still require testing.

## Vita 0.4.0 usability and diagnostics

Adds five-target touch-offset calibration, a taller default keyboard with Compact/Larger setting, Notepad selection and bounded undo/redo, Find/Replace, View controls, unsaved-change prompts and graphical Open/Save As dialogs with existence/overwrite checks. New Notepad documents default to Vita storage for native operation before Linux is ready.

Fast outline dragging avoids rendering window contents on each movement. Native queued interactive reads/saves receive bounded priority without reordering dependent paths. Quiet guest mailbox polling and completed-job processing are reduced. Diagnostics expose measured emulator execution/poll/wait time and native worker state/core/clock counters; process rows prioritize measured CPU activity. Core 2 still runs one emulated CPU. No equal-load or on-device FPS claim is made.

Games adds Minesweeper, Pong, 2048 and one-card Klondike Solitaire to the existing three native games; held controls, paddle dragging, difficulty for real-time games and visible high scores are included. Doom remains outside this release.

Updater feedback includes elapsed time, rate/ETA, stalled-output indication, retry and cancellation during service acceptance. Backups exports/restores verified settings and retains previous document copies. Restore is confirmed and applies after relaunch; full Linux disk backups remain offline. See [FEATURE-LIST-VITA.md](FEATURE-LIST-VITA.md) for usage, measurements and practical limits.

The Vita linker reserves SCE metadata headroom between load segments so code growth does not cause converter overlap failures. Validation includes sanitized actual desktop/models/native storage, real RV32 Linux operation and installer/previous-updater archive compatibility. Physical Vita verification is still required.

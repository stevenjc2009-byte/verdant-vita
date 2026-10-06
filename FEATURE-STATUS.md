# Requested feature status

Implemented describes source/binary behavior, not physical device certification. This is an experimental foundation; the complete requested feature set is unfinished.

| Requested feature | State |
|---|---|
| Green theme, wallpapers/icons, settings, start menu, taskbar, clock, launcher | Implemented, original artwork |
| Windows: focus/minimize/maximize/close/switch/resize | Implemented; state/render tests pass |
| Separate/joined screens, movement without restart, swap, workspaces | Implemented; four workspaces and clipped joined canvas; live terminal movement tested |
| Modifier/symbol/navigation touchscreen keyboard | Implemented; ASCII layout |
| Touchpad/Circle Pad pointer, fonts/zoom/panning | Implemented; physical input untested; terminal font/zoom and workspace panning |
| Colored terminal, scrollback, selection, copy/paste | Implemented; ANSI/xterm, 200 scrollback rows, application clipboard |
| Multiple terminals/background commands | Four PTYs plus boot console; background jobs; movement preserves processes |
| Bash/Vim/Nano/htop/tree/wget/BusyBox | Present and verified in actual guest |
| winget | Unavailable Windows tool; opkg is provided for compatible Linux packages |
| Files, preferences, shell history persistence | Implemented; clean shutdown tested |
| SD/Linux graphical file manager | Copy/cut/move/rename/search/bookmark/new-folder/trash/restore implemented; core file operations tested |
| Graphical editor/calculator/image viewer | Implemented; 15 KB editor, arithmetic calculator, bounded PNG/JPEG/BMP |
| Processes/memory task manager | Implemented via real ps and /proc/meminfo; tested |
| Outbound Wi-Fi networking/SSH | NAT and auto eth0 configuration; host HTTP/incoming SSH tests pass; console Wi-Fi untested |
| SSH profiles/key authentication | One saved GUI profile and optional key; incoming public-key authentication tested; multiple named profiles pending |
| Downloads/progress/cancel | wget jobs and atomic .part handling; HTTP success tested; broad HTTPS compatibility unvalidated |
| SSH file transfer | Real SCP in terminals; graphical transfer browser pending |
| SMB | Optional service operation exists, but smbclient is absent; not usable in this release |
| Battery/charging/storage/network indicators | Console accessor implementations; physical readings untested |
| Camera/mic/audio | 3DS and Vita snapshots and WAV recording/playback adapters implemented; physical tests and general music/video support pending |
| Standard local Linux GUI applications | **Pending:** no X11/Wayland/toolkit in guest; native ARM drivers cannot be merged into the RV32 guest |
| Package install/update | Local compatible .ipk and opkg feed commands; no curated repository/general Mint packages |
| Linux/Windows remote desktop | VNC raw client tested through guest NAT; no RDP/TLS or broad real-server interoperability tests |
| Incoming SSH server | Port 2222, password/public-key setup; blank network passwords disabled |
| Suspend/resume/lid close | libctru APT support and wake clock rebasing; **physical lid, SD and Wi-Fi restoration tests pending** |
| Software updates | Built-in GitHub check/download/auto-stage and native apply on relaunch, per-console channels, hash checks/backups; CIA/Vita installed-application update paths need hardware testing; offline updater retained |
| Backups/recovery | /root tar, offline runtime backup, settings recovery, preserved-disk reset, boot recovery console |
| All 3DS/2DS models | ARMv6/adaptive-memory target; all physical models unverified |
| PS Vita | VitaSDK VPK compiled, original LiveArea assets, one physical desktop canvas; see VITA.md; physical testing pending |

Major unfinished work: local Linux GUI runtime, package repository, graphical SCP/SMB, richer remote controls and actual device testing.

## Vita 0.3.0 desktop and performance

Global saved desktop scaling (100/125/150/175/200 percent, default 150) covers fonts, controls, taskbar, windows, pointer and keyboard. Settings has touchable graphical cards. Task Manager supplies processes and performance categories with CPU core history and guest RAM/network graphs. Vita CPU/GPU current clocks, core utilization (if kernel query succeeds), user/CDRAM free pools, application heap, Wi-Fi state/signal and ux0 capacity use native APIs. Linux guest CPU, RAM, processes and network/disk rates use /proc. GPU utilization, memory type/speed, base CPU frequency and physical link/storage throughput are not exposed and are labeled unavailable. The four physical core charts include system activity; from 0.3.2 the display worker can also use CPU 3 when unlocked; the guest remains single-vCPU.

## Vita 0.3.1 additions

- Native touchscreen/mouse calculator: standard arithmetic, percentages, repeat equals, memory/history; scientific functions; single-function graph plotting with zoom/pan; exact unsigned programmer operations; Gregorian date difference/add/subtract; 12 unit categories; dated ECB currency conversion with cache and refresh.
- Notepad: renamed existing graphical editor, retaining file open/edit/save behavior.
- Native GitHub release check without guest Python, bounded timeout/cancellation, graphical updater states and download progress, fresh metadata reuse. Installation/staging preserves the existing recovery protocol.

This is a lightweight native application set, not Windows Calculator or the Windows Notepad executable. Console interaction and download speeds still need physical Vita validation.

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

# Verdant Desktop for PS Vita — experimental 0.4.0

Install the full **Verdant-PS-Vita-v0.4.0.vpk** with VitaShell, replacing the existing Verdant application. Launch its bubble and wait for automatic first-launch setup. No separate ZIP extraction is required. Keep at least 500 MB free, preferably 1 GB. Existing Linux files, preferences and shell history are preserved. Local Buildroot login: **root**, empty password unless you changed it.

Version 0.3.0 added a 150% default desktop size, saved global scaling from 100% to 200%, graphical Settings cards, and a Processes/Performance task manager. Its Vita hardware pages use measured kernel idle clocks, current CPU/GPU clock queries, free memory pools, heap usage, Wi-Fi state/signal and ux0 capacity. Linux process/RAM/network/disk counters are separate. Unsupported GPU utilization, memory clock/type, base frequency and physical drive/link speeds show unavailable.

Version 0.3.0 reduces pointer-only redraws to the cursor area, accelerates rectangle fills, removes long terminal rendering locks and pipelines frame transfer on a separate CPU core. UI/input use core 0, display/media core 1 and Linux emulation core 2. From 0.3.2 the display worker also tries CPU 3 when unlocked; the interpreted guest still has one virtual CPU. Host pixel/cursor and concurrent handoff tests pass, but physical Vita performance needs confirmation.

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

## New in 0.3.1

Calculator now has actual mouse/touch buttons, memory and history, with Standard, Scientific, Graphing, Programmer, Date calculation, Unit converter and Currency modes. Scientific functions support degrees/radians; graphing plots one function of x with zoom/pan and point readout. Programmer operations use exact unsigned 8/16/32/64-bit arithmetic and hexadecimal/decimal/octal/binary bases. Dates use Gregorian leap-year rules. Twelve unit categories include temperature, length, mass and data. Currency refresh uses dated [ECB reference rates](https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html), cached for offline use; it requires Wi-Fi and does not invent rates. Supported currencies: EUR, GBP, USD, CAD, JPY, AUD, CHF, CNY. Rate refresh still starts a Linux helper and can take time. At large UI sizes scientific/programmer keys use switchable pages. Tap Mode to change modes. The existing text editor is now named **Notepad**.

**System update → Check now** starts the native verified HTTPS worker directly, without waiting for Linux boot or Python initialization. It has connection/download/error feedback, immediate cancellation and a 45-second check deadline. Download/staging still uses the Linux service for archive/hash checks; fresh check metadata is reused to avoid a second GitHub request. The download bar shows byte progress when the server provides a length. Existing user files are preserved.

An installed 0.3.0 updater uses its old code until this update is applied. Use Download in that version once, or install the full version-named VPK in VitaShell if the old updater stalls. Exit with Start, relaunch to apply, and launch again after the update message.

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

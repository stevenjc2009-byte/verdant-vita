# Validation — experimental 0.3.0

- devkitARM ARMv6: 3DSX/CIA generated; check3dsx relocation validation passes.
- VitaSDK channel 2026.08: native ARM executable, SELF and VPK generated, including camera/mic/audio adapters and original LiveArea assets. Compilation does not establish physical operation.
- GCC address/undefined-behaviour sanitizers at both 3DS and Vita screen dimensions: windows, editor insertion/navigation/save response, file selection/clipboard, calculator, symbols, live-terminal movement, session limits and clipped native rendering pass.
- Vita VPK archive integrity, required executable/LiveArea assets and param.sfo title/title ID checks pass.
- Actual bundled RV32 guest boots in the headless backend; patched Python service starts. File read/write/copy/move/trash/list, memory/process inspection, real Bash PTY and listed tools pass.
- Actual guest NAT: HTTP download, incoming SSH with a generated key on port 2222, outgoing VNC frame/keyboard and Linux sync/unmount/poweroff pass.
- RFB fixtures: 3.3/3.7/3.8, password authentication, fragmented transport, pixels, input and a DES vector pass.
- Offline tools: corrupt update rejection, preservation of Linux disk/preferences, backup contents, settings recovery and recoverable disk reset pass on disposable files.
- GitHub updater: platform/version/HTTPS-host/path rejection, package corruption rejection and verified staging pass. Native apply tests cover interrupted-rename recovery and preservation of disk/settings. Physical CIA self-install and Vita installed-file permissions are untested.
- Live GitHub checks from the actual RV32 guest pass for both repositories with CA verification. A TCP bridge defect was diagnosed and corrected: premature SYN acknowledgement, duplicate/partial guest writes, RX-queue backpressure and repeated FIN transmission. The updater also retries interrupted HTTPS requests without bypassing certificate checks.
- Complete published 0.2.1 install ZIPs for both platforms pass staging and native file-application tests on disposable host storage, including extraction/application of the real Vita VPK. This does not exercise physical CIA installation or Vita access permissions. HTTP/SSH/VNC guest regression checks also pass after the TCP change.

Network testing uncovered the upstream image's inactive eth0 and 1970 clock. Startup configuration and console-derived time were added; tests passed on a fresh disk. Dropbear's blank-password network option was removed.

No physical console was tested. Host speed/memory and networking do not validate console Wi-Fi, input, CIA launch, camera/mic/audio, battery values, low-memory performance, lid close or Home Menu transitions. VNC uses protocol fixtures, not real desktop-server interoperability tests.

The user reported Vita installation error 0x8010113D on 0.2.1. Inspection found all LiveArea assets were RGB PNGs (colour type 2). Version 0.2.2 converts them to 8-bit indexed PNGs (colour type 3) with unchanged dimensions and no icon/background transparency. The new asset guard rejects the old VPK and validates the fixed assets. Physical installation success has not yet been confirmed.

The user subsequently reported an immediate return to LiveArea after installing only the thin VPK and confirmed that no runtime ZIP was extracted. Version 0.2.3 adds a standalone bundled setup VPK, native file-copy/hash verification, automatic first-launch setup, conservative Vita heap/RAM limits and startup logging. Sanitized installer tests cover fresh installation, repeat launches, partial-copy recovery, missing-file repair, corrupt-bundle rejection and preservation of existing Linux disk/settings. A physical Vita boot remains unverified.

The user's 0.2.3 boot.log confirms successful entry into main and framebuffer allocation/base lookup, followed by display setup error 0x80290006. Version 0.2.4 replaces immediate framebuffer updates with next-frame scheduling and waits for vertical blank. This corrects the observed failing call; subsequent physical boot remains unverified.

Version 0.2.5 responds to the user's confirmed desktop boot and reported redraw flashes/slow analog pointer. The Vita renderer uses a persistent cached drawing buffer plus two CDRAM scanout buffers, copying only completed frames and switching at vertical blank. Pointer tests verify equal distance at different polling rates, fractional slow movement, neutral reset and capped movement after stalls. Physical flicker/performance still requires confirmation. The bundled /etc/shadow was inspected: local root has an empty password.

Version 0.2.6 audits the Vita profile: Linux storage/hardware mounts, file-manager defaults and shortcuts, backup path, storage indicators, keyboard/control help, sensor name/vendor, native thread names, model/storage APIs, supported settings/RAM and pinned Vita update channel. Sanitized desktop tests pass with Vita defaults. The actual RV32 guest passes canonical Vita bind mounts, file I/O, backward-compatible aliases, runtime image guards and shell environment checks on disposable storage. This confirms guest integration, not physical media hardware or Vita filesystem permissions.

Version 0.3.0 passes sanitized pixel-equivalence tests for clipped/panned 24-bit and 32-bit surfaces (including alpha preservation), cursor-background restoration without trails, desktop behavior and time-based pointer movement. The actual production display worker is tested with host semaphore/thread bindings: immutable scanout during the next draw, queue backpressure, requested core affinity, shutdown with a flip in flight, restart, partial semaphore failure, thread-create/start failure and synchronous fallback all pass. In the sanitized host fill benchmark, 80 full-screen scalar fills took 509452 us versus 79121 us for the bulk routine (~6.4x faster for that primitive). These are host timings, not Vita FPS or an end-to-end performance promise. The VitaSDK binary compiles with UI core 0/display core 1/guest core 2 assignment; boot.log records affinity/worker initialization results. Physical scheduling and drag/cursor smoothness need console confirmation.

## Vita 0.3.0 UI and updater

Sanitized native desktop tests cover scale limits, saved 150% default geometry, graphical Settings hit testing, all five Performance pages, process rendering and parser/history state. Pixel output previews at 150% were inspected. The production frame transfer also passes a 200% nearest-neighbour mapping test across the two logical regions. The Linux metrics collector passes live /proc tests, with first-sample rates explicitly unknown.

The real RV32 Linux guest reached the live GitHub releases API through the same production native HTTPS implementation bound to host threads/curl. It reported connection progress and completed the check. The guest fallback transport also completed a live channel check. Host tests do not verify the Vita network stack, hardware query permissions, physical touch accuracy or console frame rate. GPU utilization and memory clocks/types remain unavailable rather than estimated.

The exact 0.2.6 guest agent/updater also completed a check against the live Vita release channel through the interpreted guest NAT. It remained silent while running and printed its result on exit, consistent with the reported UI. This confirms protocol/connectivity on the host test fixture, not on the user's Vita Wi-Fi.

## Vita 0.3.1 calculator and native update check

The VitaSDK build passes. Address/undefined-behavior sanitized tests cover calculator precedence/domain errors, scientific functions and graph evaluation, exact 64-bit programmer arithmetic and wrapping/shift limits, Gregorian leap-century date handling, unit conversions, memory, percent semantics and repeated equals. Actual button-center clicks pass through the desktop hit-testing path. All seven calculator modes and compact scientific/programmer key pages fit at 100%, 125%, 150%, 175% and 200%; rendered previews were inspected. Notepad keeps the existing file-edit/save tests.

The native release-check state machine passes strict metadata/version parsing, start/cache, timeout and cancellation fixtures without booting Linux. The production native HTTPS worker completed live GitHub checks in 0.42–0.52 seconds on the host fixture without a Linux/Python instance. This measures the new architecture on a PC, not Vita network speed. Live ECB XML retrieval passes the same verified native transport. The real interpreted RV32 guest passes a currency refresh job, dated rate caching, Vita paths and the existing GitHub check protocol.

Installer and actual full VPK extraction tests pass, including user-disk preservation. Update corruption/platform/path checks, stale-transfer cancellation, interrupted apply recovery, offline backup/recovery, Linux metrics and production frame ownership tests pass. Physical Vita touch accuracy, speed and self-update permissions remain unverified.

The 0.3.1 integration probe exposed a service-crash race: `host-http.req` was included in the desktop agent's numeric request sort. The agent now accepts only ASCII numeric command filenames and leaves native transport files alone. A regression fixture holds the native request in place across multiple service ticks while valid desktop commands complete.

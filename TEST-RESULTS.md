# Validation — experimental 0.2.1

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

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

Global saved desktop scaling (100/125/150/175/200 percent, default 150) covers fonts, controls, taskbar, windows, pointer and keyboard. Settings has touchable graphical cards. Task Manager supplies processes and performance categories with CPU core history and guest RAM/network graphs. Vita CPU/GPU current clocks, core utilization (if kernel query succeeds), user/CDRAM free pools, application heap, Wi-Fi state/signal and ux0 capacity use native APIs. Linux guest CPU, RAM, processes and network/disk rates use /proc. GPU utilization, memory type/speed, base CPU frequency and physical link/storage throughput are not exposed and are labeled unavailable. The four physical core charts include the system core for observation only; the guest remains single-vCPU.

# Research checked 6 October 2026

**3DS-CLI 5.3** is the practical foundation here: CIA/3DSX homebrew hosting RV32 Linux, SD passthrough, network NAT and hardware interfaces. The checked-out source/tag is 5.3; the actual guest tested is Linux **6.6.63** / Buildroot **2024.02.9**. A recent application release does not mean the newest kernel. It runs alongside Luma/Home Menu. Sources: [project](https://github.com/cmdada/3DS-CLI), [release](https://github.com/cmdada/3DS-CLI/releases/tag/5.3).

**Native 3DS-Linux/Void** offers ARM Linux and X11. Its official site advertises kernel 7.1, Old-family support and Wi-Fi, while listing New-family support as untested and sound absent. It therefore does not establish compatibility with the user's New 2DS XL. A .firm boot payload has a different execution model from a CIA application; loading it and permanently replacing firmware are separate actions. Verdant installs no such payload. Source: [official site](https://3dslinux.org/).

**3ds-fbge** is an author's framebuffer desktop work in progress with incomplete features/stability concerns, not a finished Mint desktop to merge wholesale. Source: [repository](https://github.com/AtexBg/3ds-fbge).

Verdant uses native UI drawing and Horizon services, while shells/files/network clients run in Linux through an SD mailbox. The emulator has substantial overhead; ARM/x86/Mint packages are not RV32-compatible. Cinnamon is not a practical local desktop target for this architecture/memory budget. VNC can expose full GUI applications running on a computer.

Implementation references: [libctru](https://github.com/devkitPro/libctru), [RFB specification](https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst), [pyDes](https://github.com/twhiteman/pyDes). FEATURE-STATUS.md distinguishes executed tests from upstream claims and remaining work.

# Source snapshot

- 3DS application built with devkitARM GCC 16 and libctru 2.7. Vita application built with VitaSDK channel 2026.08; toolchain binaries are not included in the source archive.

- 3DS-CLI tag 5.3: `ac170da1151eb4ecabaa50eed7e2ead235d9697f` (GPL-3.0).
- mini-rv32ima-mmu: `b2f0d1ea36174c1fd0d8ad7e165076a77c356b83` (vendored source/notice retained).
- ctr-osk-rt: `05e5e0c48ca8fd0bede2e38a5c8cd33ea7f3adb3` (vendored source/notice retained).
- Kernel in downloaded 5.3 combined Image: Linux 6.6.63; userspace Buildroot 2024.02.9, RV32IMA/Sv32, ILP32/glibc.
- Source URLs: https://github.com/cmdada/3DS-CLI/tree/5.3 ; https://github.com/cmdada/mini-rv32ima-mmu ; https://github.com/cmdada/ctr-osk-rt ; https://buildroot.org/downloads/buildroot-2024.02.9.tar.xz ; https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.63.tar.xz .
- Rebuilding the full guest requires the upstream Buildroot source packages; the application source archive does not include every Linux userspace dependency's source tarball. Use Buildroot's source and legal-info targets when preparing redistribution of a rebuilt runtime.
- pyDes 2.0.1 by Todd Whiteman: MIT notice in guest/pyDes-LICENSE.txt. stb_image includes its public-domain/MIT notices in source/core/stb_image.h.
- Original Verdant desktop/service/branding modifications are supplied under the application's GPL-3.0 license. The original upstream documentation is preserved in UPSTREAM-README.md.

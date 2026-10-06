# mini-rv32ima-mmu

A fork of [cnlohr/mini-rv32ima](https://github.com/cnlohr/mini-rv32ima), a
tiny single-file RV32IMA emulator core, adding:

- Sv32 virtual memory (page-table walking, S/U-mode access checks)
- Supervisor mode (S-mode) alongside the original Machine mode
- Trap delegation (`medeleg`/`mideleg`) between M-mode and S-mode
- A minimal built-in SBI (Supervisor Binary Interface) implementation, so a
  standard MMU-enabled Linux kernel can boot in S-mode without a separate
  firmware layer like OpenSBI

Upstream `mini-rv32ima` intentionally only supports the M-mode/NOMMU boot
path (`CONFIG_MMU=n`), which most mainline device userspace toolchains
(glibc, musl) don't cleanly support on RISC-V, and which the NOMMU-specific
ELF-FDPIC loader constrains to certain binary layouts. This fork exists to
support normal MMU'd Linux userspace instead.

## Usage

Same single-header integration model as upstream: define the required
macros, then `#include "mini-rv32ima.h"` with `MINIRV32_IMPLEMENTATION`
defined in exactly one translation unit. See the header's own comment block
for the full macro list.

## License

Same triple license as upstream (BSD-2-Clause / MIT / CC0-1.0) — see
`LICENSE`.

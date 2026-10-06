// Copyright 2022 Charles Lohr, you may use this file or any portions herein under any of the BSD, MIT, or CC0 licenses.
//
// This is a fork of cnlohr/mini-rv32ima adding Sv32 virtual memory,
// Supervisor mode, trap delegation, and a minimal built-in SBI so a normal
// MMU-enabled Linux kernel can boot in S-mode without a separate firmware
// layer (OpenSBI etc). See README.md for details.
//
// Because the CPU always starts up already in S-mode (see MiniRV32IMAState's
// extraflags comment) rather than going through a real M-mode boot ROM,
// there is no actual M-mode "firmware" code ever executed by the guest —
// the embedding application's MINIRV32_POSTEXEC hook is expected to
// service SBI ecalls (cause 9, environment-call-from-S-mode, which is
// never delegated) and the real machine timer interrupt (which also can't
// be delegated, since there's no Sstc-style hardware STIP wire) entirely
// in host code, exactly the way MMIO is already serviced by
// MINIRV32_HANDLE_MEM_*_CONTROL. See MINIRV32_POSTEXEC's two call sites
// below (retval == 0x80000007 identifies the machine timer interrupt).

#ifndef _MINI_RV32IMAH_H
#define _MINI_RV32IMAH_H

/**
    To use mini-rv32ima.h for the bare minimum, the following:

	#define MINI_RV32_RAM_SIZE ram_amt
	#define MINIRV32_IMPLEMENTATION

	#include "mini-rv32ima.h"

	Though, that's not _that_ interesting. You probably want I/O!


	Notes:
		* There is a dedicated CLNT at 0x10000000.
		* There is free MMIO from there to 0x12000000.
		* You can put things like a UART, or whatever there.
		* Feel free to override any of the functionality with macros.
*/

#ifndef MINIRV32WARN
	#define MINIRV32WARN( x... );
#endif

#ifndef MINIRV32_DECORATE
	#define MINIRV32_DECORATE static
#endif

#ifndef MINIRV32_RAM_IMAGE_OFFSET
	#define MINIRV32_RAM_IMAGE_OFFSET  0x80000000
#endif

#ifndef MINIRV32_MMIO_RANGE
	#define MINIRV32_MMIO_RANGE(n)  (0x10000000 <= (n) && (n) < 0x12000000)
#endif

#ifndef MINIRV32_POSTEXEC
	#define MINIRV32_POSTEXEC(...);
#endif

#ifndef MINIRV32_HANDLE_MEM_STORE_CONTROL
	#define MINIRV32_HANDLE_MEM_STORE_CONTROL(...);
#endif

#ifndef MINIRV32_HANDLE_MEM_LOAD_CONTROL
	#define MINIRV32_HANDLE_MEM_LOAD_CONTROL(...);
#endif

#ifndef MINIRV32_OTHERCSR_WRITE
	#define MINIRV32_OTHERCSR_WRITE(...);
#endif

#ifndef MINIRV32_OTHERCSR_READ
	#define MINIRV32_OTHERCSR_READ(...);
#endif

#ifndef MINIRV32_CUSTOM_MEMORY_BUS
	#define MINIRV32_STORE4( ofs, val ) *(uint32_t*)(image + ofs) = val
	#define MINIRV32_STORE2( ofs, val ) *(uint16_t*)(image + ofs) = val
	#define MINIRV32_STORE1( ofs, val ) *(uint8_t*)(image + ofs) = val
	#define MINIRV32_LOAD4( ofs ) *(uint32_t*)(image + ofs)
	#define MINIRV32_LOAD2( ofs ) *(uint16_t*)(image + ofs)
	#define MINIRV32_LOAD1( ofs ) *(uint8_t*)(image + ofs)
	#define MINIRV32_LOAD2_SIGNED( ofs ) *(int16_t*)(image + ofs)
	#define MINIRV32_LOAD1_SIGNED( ofs ) *(int8_t*)(image + ofs)
#endif

// sstatus/sie/sip are masked *views* onto the same physical mstatus/mie/mip
// storage, per the RISC-V privileged spec (there is only one physical
// mstatus register; sstatus just exposes a subset of its bits to S-mode).
#define MINIRV32_SSTATUS_MASK ( (1u<<1) | (1u<<5) | (1u<<8) | (1u<<18) | (1u<<19) ) // SIE, SPIE, SPP, SUM, MXR
#define MINIRV32_SIE_MASK     ( (1u<<1) | (1u<<5) | (1u<<9) )                       // SSIE, STIE, SEIE
#define MINIRV32_SIP_WMASK    ( (1u<<1) )                                          // Only SSIP is software-writable via sip.

// Access kinds passed to MiniRV32IMATranslate(), also used to pick which
// page-fault cause to report.
#define MINIRV32_ACCESS_FETCH 0
#define MINIRV32_ACCESS_LOAD  1
#define MINIRV32_ACCESS_STORE 2

// Direct-mapped translation cache for MiniRV32IMATranslate(). Sv32 has no
// concept of ASIDs in this minimal implementation, so the cache is flushed
// wholesale on every satp write and on SFENCE.VMA (see both call sites
// below) rather than tagged per address space - correct because the guest
// (Linux) is required by the RISC-V spec to sfence.vma after modifying its
// own page tables, so there's never a window where a stale entry here
// disagrees with what a fresh walk would find. Sized as a power of two so
// the index is a plain mask, not a division. One shared array (not split
// I/D) since most of a Linux address space is dual R+X anyway; perm bits
// are re-checked against current priv/SUM on every hit, only the RAM walk
// itself is skipped, so toggling mstatus.SUM without a fence still works.
#define MINIRV32_TLB_SIZE 1024
struct MiniRV32IMATLBEntry
{
	uint32_t tag; // Virtual page number (va>>12), or all-ones if invalid.
	// Packed translation result. Bits 31..12 are the page's byte offset from
	// the start of the RAM image - not its physical address, so the
	// subtraction of MINIRV32_RAM_IMAGE_OFFSET is folded in here once at fill
	// time instead of being redone on every access. Bits 3..0 are the PTE
	// permission bits (R,W,X,U), which fit for free in the low bits a
	// page-aligned offset always leaves zero.
	//
	// An entry is only ever filled for a page lying wholly inside RAM (see
	// MiniRV32IMATranslateSlow), which is what lets a hit skip the RAM bounds
	// check as well - and is also what keeps MMIO, which has to reach the
	// load/store control hooks, from ever being served out of here.
	//
	// Two words rather than three: at 8 bytes the array index is a single
	// shift instead of a multiply-by-three, and the whole table is 8KB rather
	// than 12KB, which matters on a core with as little data cache as ARM11.
	uint32_t data;
};
#define MINIRV32_TLB_PERM_MASK 0xfu

// As a note: We quouple-ify these, because in HLSL, we will be operating with
// uint4's.  We are going to uint4 data to/from system RAM.
//
// We're going to try to keep the full processor state to 12 x uint4.
//
// NOTE: this fork's Sv32/S-mode additions below break that HLSL-friendly
// sizing goal - they're plain additional uint32_t fields with no attempt to
// keep the struct GPU-shader-portable. The TLB below is host-side
// bookkeeping, not architectural state - it never needs to be inspected or
// restored, only invalidated - but it lives in the struct (rather than a
// file-scope static) so each MiniRV32IMAState instance gets its own cache.
struct MiniRV32IMAState
{
	uint32_t regs[32];

	uint32_t pc;
	uint32_t mstatus;
	uint32_t cyclel;
	uint32_t cycleh;

	uint32_t timerl;
	uint32_t timerh;
	uint32_t timermatchl;
	uint32_t timermatchh;

	uint32_t mscratch;
	uint32_t mtvec;
	uint32_t mie;
	uint32_t mip;

	uint32_t mepc;
	uint32_t mtval;
	uint32_t mcause;

	// Supervisor-mode CSRs (S-mode support added by this fork). sstatus,
	// sie and sip are masked views onto mstatus/mie/mip above, not
	// separate storage - see the MINIRV32_S*_MASK defines.
	uint32_t satp;
	uint32_t medeleg;
	uint32_t mideleg;
	uint32_t sscratch;
	uint32_t stvec;
	uint32_t sepc;
	uint32_t scause;
	uint32_t stval;

	// Note: only a few bits are used.  (Machine = 3, Supervisor = 1, User = 0)
	// Bits 0..1 = privilege.
	// Bit 2 = WFI (Wait for interrupt)
	// Bit 3+ = Load/Store reservation LSBs.
	//
	// Unlike upstream mini-rv32ima, the CPU is expected to start already in
	// S-mode (extraflags = 1, not 3) with a0/a1 pre-loaded with hartid/dtb
	// pointer, mirroring the register state a real firmware handoff (e.g.
	// OpenSBI's `mret` into the kernel) would produce - there's no actual
	// M-mode boot code that runs to get here. See the file header comment.
	uint32_t extraflags;

	// Single-entry instruction-fetch translation cache, sitting in front of
	// the TLB below. Instruction fetch is overwhelmingly sequential, so
	// consecutive fetches nearly always land in the same 4KB page, and a hit
	// here costs two compares - no TLB index, no permission re-check, no RAM
	// bounds check. See the fetch path in MiniRV32IMAStep for why each of
	// those is safe to skip.
	//
	// fetch_ctx is what makes the elided permission check sound: it pins the
	// entry to the privilege level and page-table generation it was validated
	// under, and fetch permission depends on nothing else (mstatus.SUM and
	// MXR are load/store concerns - see MiniRV32IMACheckPerm). Any change to
	// either invalidates every entry at once, with no scanning.
	uint32_t fetch_tag; // (va>>12) of the cached page, or all-ones if invalid.
	uint32_t fetch_ctx; // privilege | (tlb_gen << 2) the entry was filled under.
	uint32_t fetch_ofs; // The page's byte offset from the start of the RAM image.

	// Deliberately fetch-only. The equivalent load and store caches were
	// tried and measured about 2.5% *slower*: data accesses have nothing like
	// the page locality instruction fetch does, so the entry thrashed, and
	// the context value a data-side hit needs (privilege, mstatus.SUM and the
	// page-table generation - SUM matters for data but not for fetch) costs
	// more to assemble on every load and store than the occasional hit saved.
	// The shared TLB below already handles the data side well enough.

	// Bumped by MiniRV32IMATLBFlush. Lets the fetch cache above be
	// invalidated by a single increment rather than being cleared alongside
	// the main TLB.
	uint32_t tlb_gen;

	// See MiniRV32IMATLBEntry above. Lazily populated by
	// MiniRV32IMATranslate(); starts zeroed (tag 0, i.e. "valid for VPN 0")
	// along with the rest of a fresh ram_image, but that's harmless - the
	// first satp write (bare -> paged, or any subsequent address-space
	// switch) always flushes this before paging can actually be in effect,
	// so no explicit init is required here.
	struct MiniRV32IMATLBEntry tlb[MINIRV32_TLB_SIZE];
};

#ifndef MINIRV32_STEPPROTO
MINIRV32_DECORATE int32_t MiniRV32IMAStep( struct MiniRV32IMAState * state, uint8_t * image, uint32_t vProcAddress, uint32_t elapsedUs, int count );
#endif

#ifdef MINIRV32_IMPLEMENTATION

#include <string.h> // memset(), for MiniRV32IMATLBFlush().

// Steady-state guest execution is almost entirely: paging on, TLB hits,
// in-RAM accesses, no trap. These hints just tell the compiler which side
// of each of those checks to lay out as the straight-line path - cheap
// insurance on an in-order core (ARM11/ARMv6k on 3DS) with no branch
// prediction sophisticated enough to learn this on its own from history.
#if defined(__GNUC__) || defined(__clang__)
#define MINIRV32_LIKELY(x)   __builtin_expect(!!(x), 1)
#define MINIRV32_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MINIRV32_LIKELY(x)   (x)
#define MINIRV32_UNLIKELY(x) (x)
#endif

#ifndef MINIRV32_CUSTOM_INTERNALS
#define CSR( x ) state->x
#define SETCSR( x, val ) { state->x = val; }
#define REG( x ) state->regs[x]
#define REGSET( x, val ) { state->regs[x] = val; }
#endif

// Wholesale-invalidates state's translation cache. Must be called on every
// satp write and on SFENCE.VMA - see MINIRV32_TLB_SIZE's comment above for
// why that's sufficient (no per-entry/ASID tracking needed).
static void MiniRV32IMATLBFlush( struct MiniRV32IMAState * state )
{
	memset( state->tlb, 0xff, sizeof( state->tlb ) ); // tag = 0xffffffff (invalid) for every entry.
	// Invalidates the fetch cache too: its entries are tagged with this
	// counter, so nothing filled before this point can match again. The
	// explicit tag reset is belt-and-braces for the counter wrapping, which
	// would take 2^30 flushes but costs nothing to rule out here.
	state->tlb_gen++;
	state->fetch_tag = 0xffffffffu;
}

// Permission check shared by the TLB-hit fast path and the walk-on-miss
// slow path: identical logic, just fed from tlb-cached bits in one case
// and freshly-read PTE bits in the other (see the two call sites).
static inline uint32_t MiniRV32IMACheckPerm( uint32_t perm, int access, uint32_t priv, uint32_t mstatus, uint32_t fault_trap )
{
	uint32_t pte_r = perm & 1, pte_w = perm & 2, pte_x = perm & 4, pte_u = perm & 8;
	int is_fetch = ( access == MINIRV32_ACCESS_FETCH );
	int is_store = ( access == MINIRV32_ACCESS_STORE );

	if( is_fetch && !pte_x ) return fault_trap;
	if( is_store && !pte_w ) return fault_trap;
	if( !is_fetch && !is_store && !pte_r ) return fault_trap; // Load needs R.

	if( priv == 0 ) // U-mode: can only access pages marked user-accessible.
	{
		if( !pte_u ) return fault_trap;
	}
	else // S-mode.
	{
		if( pte_u && is_fetch ) return fault_trap; // Never execute from a U page.
		if( pte_u && !is_fetch && !( mstatus & (1u<<18) /*SUM*/ ) ) return fault_trap;
	}
	return 0;
}

// Walks the page table on a TLB miss, faults exactly as MiniRV32IMATranslate
// (see below) documents, and populates the TLB entry on success. Kept as a
// real out-of-line function (unlike the fast path) so the rare, large walk
// body doesn't get duplicated at every one of MiniRV32IMATranslate's four
// call sites (fetch/load/store/amo) just to inline the common hit case.
static uint32_t MiniRV32IMATranslateSlow( struct MiniRV32IMAState * state, uint8_t * image, uint32_t va, int access,
	uint32_t priv, uint32_t satp, struct MiniRV32IMATLBEntry * tlbe, uint32_t va_vpn, uint32_t fault_trap,
	uint32_t ram_size, uint32_t * ofs_out )
{
	uint32_t root = ( satp & 0x3fffff ) << 12;
	uint32_t vpn0 = ( va >> 12 ) & 0x3ff;
	uint32_t vpn1 = ( va >> 22 ) & 0x3ff;
	uint32_t pte = 0;
	uint32_t ptaddr = root;
	int level;
	for( level = 1; level >= 0; level-- )
	{
		uint32_t vpn = ( level == 1 ) ? vpn1 : vpn0;
		uint32_t pteaddr = ptaddr + vpn * 4;
		uint32_t ofs = pteaddr - MINIRV32_RAM_IMAGE_OFFSET;
		if( ofs >= ram_size - 3 )
			return fault_trap;
		pte = MINIRV32_LOAD4( ofs );
		if( !( pte & 1 ) || ( !( pte & 2 ) && ( pte & 4 ) ) ) // Not valid, or W-without-R (reserved encoding).
			return fault_trap;
		if( pte & 0xe ) break; // Leaf (R, W, or X set).
		ptaddr = ( pte >> 10 ) << 12;
	}
	if( !( pte & 0xe ) ) // Ran out of levels without ever finding a leaf.
		return fault_trap;

	uint32_t pte_r = pte & 2, pte_w = pte & 4, pte_x = pte & 8, pte_u = pte & 0x10;
	uint32_t perm = ( pte_r ? 1 : 0 ) | ( pte_w ? 2 : 0 ) | ( pte_x ? 4 : 0 ) | ( pte_u ? 8 : 0 );
	uint32_t perm_trap = MiniRV32IMACheckPerm( perm, access, priv, CSR( mstatus ), fault_trap );
	if( perm_trap ) return perm_trap;

	uint32_t pa = ( level == 1 )
		? ( ( pte >> 20 ) << 22 ) | ( va & 0x3fffff )   // Superpage: low bits come from the VA, not the PTE.
		: ( ( pte >> 10 ) << 12 ) | ( va & 0xfff );
	*ofs_out = pa - MINIRV32_RAM_IMAGE_OFFSET;

	// Cache this translation at 4KB granularity even if it came from a
	// superpage PTE - a superpage is just many consecutive VPNs that all
	// happen to resolve linearly, each gets its own entry lazily as touched,
	// same as how hardware TLBs commonly handle huge pages internally.
	//
	// Only pages lying wholly within RAM get an entry, so that a later hit
	// can skip the bounds check entirely. The unsigned comparison does double
	// duty: a physical address below the RAM base wraps to a huge offset and
	// so is rejected by the same test, which is what keeps MMIO out of the
	// cache and routed to the load/store control hooks. (ram_size is always
	// several MB here - see the allocation loop in the embedding app - so the
	// subtraction below cannot underflow.)
	uint32_t page_ofs = ( pa & ~0xfffu ) - MINIRV32_RAM_IMAGE_OFFSET;
	if( page_ofs <= ram_size - 4096u )
	{
		tlbe->tag = va_vpn;
		tlbe->data = page_ofs | perm;
	}

	return 0;
}

// Sv32 address translation. Returns 0 and writes the address's byte offset
// from the start of the RAM image to *ofs_out on success, or this core's
// trap-numbering-convention fault value (cause+1, e.g. 13 for instruction
// page fault) on a page fault - directly assignable to the `trap` variable in
// MiniRV32IMAStep.
//
// An offset rather than a physical address: every caller wants to index the
// RAM image with it, and the ones that don't (MMIO) can add the base back on
// the rare path where they need it. Note a successful return does NOT imply
// the offset is within RAM - it can be an out-of-range value that the caller
// is expected to bounds-check and route to MMIO, exactly as before.
//
// Superpages (4MB, leaf at level 1) are supported. Misaligned-superpage
// and A/D-bit-not-yet-set faults are deliberately NOT implemented: A and D
// are treated as always-set (a spec-legal simplification many minimal
// software MMUs make - real hardware MAY set them automatically instead of
// faulting), and a misaligned superpage PTE is treated the same as a
// well-formed one rather than being rejected, on the assumption no real
// kernel page-table code ever constructs one.
//
// Deliberately kept small and `inline`: everything past the TLB-hit check
// is out-of-line in MiniRV32IMATranslateSlow, so this is cheap enough for
// the compiler to actually inline at all four call sites in MiniRV32IMAStep
// (fetch/load/store/amo) - the common TLB-hit case then costs no function
// call at all, just the CSR reads and a tag compare.
static inline uint32_t MiniRV32IMATranslate( struct MiniRV32IMAState * state, uint8_t * image, uint32_t va, int access, uint32_t ram_size, uint32_t * ofs_out )
{
	uint32_t satp = CSR( satp );
	uint32_t priv = CSR( extraflags ) & 3;

	// Bare mode (no paging), or M-mode (this fork doesn't implement MPRV,
	// so M-mode - which never actually runs any guest code in this
	// design, see the file header - is always physical).
	if( MINIRV32_UNLIKELY( !( satp & 0x80000000 ) || priv == 3 ) )
	{
		*ofs_out = va - MINIRV32_RAM_IMAGE_OFFSET;
		return 0;
	}

	uint32_t fault_trap = ( access == MINIRV32_ACCESS_FETCH ) ? 13 : ( access == MINIRV32_ACCESS_LOAD ? 14 : 16 );

	uint32_t va_vpn = va >> 12;
	struct MiniRV32IMATLBEntry * tlbe = &state->tlb[ va_vpn & ( MINIRV32_TLB_SIZE - 1 ) ];
	if( MINIRV32_LIKELY( tlbe->tag == va_vpn ) )
	{
		uint32_t data = tlbe->data;
		uint32_t perm_trap = MiniRV32IMACheckPerm( data & MINIRV32_TLB_PERM_MASK, access, priv, CSR( mstatus ), fault_trap );
		if( perm_trap ) return perm_trap;
		*ofs_out = ( data & ~0xfffu ) | ( va & 0xfff );
		return 0;
	}

	return MiniRV32IMATranslateSlow( state, image, va, access, priv, satp, tlbe, va_vpn, fault_trap, ram_size, ofs_out );
}

#ifndef MINIRV32_STEPPROTO
MINIRV32_DECORATE int32_t MiniRV32IMAStep( struct MiniRV32IMAState * state, uint8_t * image, uint32_t vProcAddress, uint32_t elapsedUs, int count )
#else
MINIRV32_STEPPROTO
#endif
{
	uint32_t new_timer = CSR( timerl ) + elapsedUs;
	if( new_timer < CSR( timerl ) ) CSR( timerh )++;
	CSR( timerl ) = new_timer;

	// Handle Timer interrupt.
	if( ( CSR( timerh ) > CSR( timermatchh ) || ( CSR( timerh ) == CSR( timermatchh ) && CSR( timerl ) > CSR( timermatchl ) ) ) && ( CSR( timermatchh ) || CSR( timermatchl ) ) )
	{
		CSR( extraflags ) &= ~4; // Clear WFI
		CSR( mip ) |= 1<<7; //MTIP of MIP // https://stackoverflow.com/a/61916199/2926815  Fire interrupt.
	}
	else
		CSR( mip ) &= ~(1<<7);

	// Clear WFI if a pending external interrupt (any of MEIP/SEIP/STIP/SSIP) is enabled.
	if( ( CSR( mip ) & CSR( mie ) & ( (1<<11) | (1<<9) | (1<<5) | (1<<1) ) ) )
		CSR( extraflags ) &= ~4;

	// If WFI, don't run processor.
	if( CSR( extraflags ) & 4 )
		return 1;

	uint32_t trap = 0;
	uint32_t rval = 0;
	uint32_t pc = CSR( pc );
	uint32_t cycle = CSR( cyclel );
	uint32_t priv_for_irq = CSR( extraflags ) & 3;
	// MINI_RV32_RAM_SIZE is typically a global in the embedding app, which
	// means every bounds check below would otherwise reload it from memory -
	// the compiler can't cache it in a register across the MMIO callbacks,
	// which for all it knows might change it. It is fixed for the lifetime of
	// the machine, so read it once here and compare against a register.
	const uint32_t ram_size = MINI_RV32_RAM_SIZE;

	if( ( CSR( mip ) & (1<<7) ) && ( CSR( mie ) & (1<<7) /*mtie*/ ) && ( CSR( mstatus ) & 0x8 /*mie*/) )
	{
		// Machine Timer Interrupt. Can't be delegated (no Sstc STIP wire) -
		// the embedding app's MINIRV32_POSTEXEC hook is expected to
		// absorb this (retval == 0x80000007) and forward it to S-mode
		// itself, e.g. by asserting mip.STIP, the way real SBI firmware
		// does. If the hook doesn't absorb it (returns the value
		// unchanged), it falls through to a real (M-mode) trap below.
		trap = 0x80000007;
		pc -= 4;
		MINIRV32_POSTEXEC( pc, 0, trap );
		if( !trap ) pc += 4; // Absorbed: undo the pre-decrement, resume normally.
	}
	// NOTE: these are deliberately separate `if`s, not `else if`. If the MTI
	// above was absorbed (trap reset to 0), the interrupt it was forwarded as
	// (e.g. STIP) may now be pending+enabled, and must be checked in this
	// same call - otherwise the guest never gets scheduled to run again
	// (nothing else will ever un-assert the underlying MTI condition), and
	// every subsequent step just re-absorbs the same MTI forever.
	if( !trap && ( CSR( mip ) & (1<<11) ) && ( CSR( mie ) & (1<<11) /*meie*/ ) && ( CSR( mstatus ) & 0x8 /*mie*/) )
	{
		// Machine External Interrupt.
		trap = 0x8000000b;
		pc -= 4;
	}
	if( !trap && priv_for_irq != 3 && ( CSR( mip ) & (1<<5) ) && ( CSR( mie ) & (1<<5) /*stie*/ ) && ( CSR( mstatus ) & 0x2 /*sstatus.SIE*/) )
	{
		// Supervisor Timer Interrupt (asserted by firmware/host code, see above).
		trap = 0x80000005;
		pc -= 4;
	}
	if( !trap && priv_for_irq != 3 && ( CSR( mip ) & (1<<9) ) && ( CSR( mie ) & (1<<9) /*seie*/ ) && ( CSR( mstatus ) & 0x2 ) )
	{
		// Supervisor External Interrupt - driven by virtio-blk/net/rng completions via the PLIC's S-mode context.
		trap = 0x80000009;
		pc -= 4;
	}
	if( !trap && priv_for_irq != 3 && ( CSR( mip ) & (1<<1) ) && ( CSR( mie ) & (1<<1) /*ssie*/ ) && ( CSR( mstatus ) & 0x2 ) )
	{
		// Supervisor Software Interrupt.
		trap = 0x80000001;
		pc -= 4;
	}
	if( !trap ) // No interrupt?  Execute a bunch of instructions.
	for( int icount = 0; icount < count; icount++ )
	{
		uint32_t ir = 0;
		rval = 0;
		cycle++;

		// Fetch-cache fast path (see fetch_tag/fetch_ctx/fetch_ofs in
		// MiniRV32IMAState). On a hit this is the whole of address
		// translation for this instruction: two compares and an OR.
		//
		// The permission check is skipped because fetch permission is a
		// function of only the PTE bits and the current privilege, and fctx
		// pins both - the page tables cannot have changed without a flush
		// bumping tlb_gen. The RAM bounds check is skipped because an entry
		// is only filled for a page lying wholly inside RAM, so every offset
		// this can produce is in range.
		uint32_t fctx = ( CSR( extraflags ) & 3 ) | ( CSR( tlb_gen ) << 2 );
		uint32_t ofs_pc;
		if( MINIRV32_LIKELY( CSR( fetch_tag ) == ( pc >> 12 ) && CSR( fetch_ctx ) == fctx ) )
		{
			ofs_pc = CSR( fetch_ofs ) | ( pc & 0xfff );
		}
		else
		{
			uint32_t xlate_trap = MiniRV32IMATranslate( state, image, pc, MINIRV32_ACCESS_FETCH, ram_size, &ofs_pc );
			if( MINIRV32_UNLIKELY( xlate_trap ) )
			{
				trap = xlate_trap;
				rval = pc;
				break;
			}

			if( MINIRV32_UNLIKELY( ofs_pc >= ram_size ) )
			{
				trap = 1 + 1;  // Handle access violation on instruction read.
				break;
			}

			// Same wholly-inside-RAM rule the main TLB uses, for the same
			// reason: it is what lets a hit skip the bounds check above.
			uint32_t page_ofs = ofs_pc & ~0xfffu;
			if( page_ofs <= ram_size - 4096u )
			{
				SETCSR( fetch_tag, pc >> 12 );
				SETCSR( fetch_ctx, fctx );
				SETCSR( fetch_ofs, page_ofs );
			}
		}

		if( MINIRV32_UNLIKELY( ofs_pc & 3 ) )
		{
			trap = 1 + 0;  //Handle PC-misaligned access
			break;
		}
		else
		{
			ir = MINIRV32_LOAD4( ofs_pc );
			uint32_t rdid = (ir >> 7) & 0x1f;

			switch( ir & 0x7f )
			{
				case 0x37: // LUI (0b0110111)
					rval = ( ir & 0xfffff000 );
					break;
				case 0x17: // AUIPC (0b0010111)
					rval = pc + ( ir & 0xfffff000 );
					break;
				case 0x6F: // JAL (0b1101111)
				{
					int32_t reladdy = ((ir & 0x80000000)>>11) | ((ir & 0x7fe00000)>>20) | ((ir & 0x00100000)>>9) | ((ir&0x000ff000));
					if( reladdy & 0x00100000 ) reladdy |= 0xffe00000; // Sign extension.
					rval = pc + 4;
					pc = pc + reladdy - 4;
					break;
				}
				case 0x67: // JALR (0b1100111)
				{
					uint32_t imm = ir >> 20;
					int32_t imm_se = imm | (( imm & 0x800 )?0xfffff000:0);
					rval = pc + 4;
					pc = ( (REG( (ir >> 15) & 0x1f ) + imm_se) & ~1) - 4;
					break;
				}
				case 0x63: // Branch (0b1100011)
				{
					uint32_t immm4 = ((ir & 0xf00)>>7) | ((ir & 0x7e000000)>>20) | ((ir & 0x80) << 4) | ((ir >> 31)<<12);
					if( immm4 & 0x1000 ) immm4 |= 0xffffe000;
					int32_t rs1 = REG((ir >> 15) & 0x1f);
					int32_t rs2 = REG((ir >> 20) & 0x1f);
					immm4 = pc + immm4 - 4;
					rdid = 0;
					switch( ( ir >> 12 ) & 0x7 )
					{
						// BEQ, BNE, BLT, BGE, BLTU, BGEU
						case 0: if( rs1 == rs2 ) pc = immm4; break;
						case 1: if( rs1 != rs2 ) pc = immm4; break;
						case 4: if( rs1 < rs2 ) pc = immm4; break;
						case 5: if( rs1 >= rs2 ) pc = immm4; break; //BGE
						case 6: if( (uint32_t)rs1 < (uint32_t)rs2 ) pc = immm4; break;   //BLTU
						case 7: if( (uint32_t)rs1 >= (uint32_t)rs2 ) pc = immm4; break;  //BGEU
						default: trap = (2+1);
					}
					break;
				}
				case 0x03: // Load (0b0000011)
				{
					uint32_t rs1 = REG((ir >> 15) & 0x1f);
					uint32_t imm = ir >> 20;
					int32_t imm_se = imm | (( imm & 0x800 )?0xfffff000:0);
					uint32_t rsval = rs1 + imm_se; // Virtual address.

					uint32_t ld_ofs;
					uint32_t ld_xlate_trap = MiniRV32IMATranslate( state, image, rsval, MINIRV32_ACCESS_LOAD, ram_size, &ld_ofs );
					if( MINIRV32_UNLIKELY( ld_xlate_trap ) )
					{
						trap = ld_xlate_trap;
						rval = rsval;
						break;
					}
					rsval = ld_ofs;

					if( rsval >= ram_size-3 )
					{
						rsval += MINIRV32_RAM_IMAGE_OFFSET;
						if( MINIRV32_MMIO_RANGE( rsval ) )  // UART, CLNT
						{
							MINIRV32_HANDLE_MEM_LOAD_CONTROL( rsval, rval );
						}
						else
						{
							trap = (5+1);
							rval = rsval;
						}
					}
					else
					{
						switch( ( ir >> 12 ) & 0x7 )
						{
							//LB, LH, LW, LBU, LHU
							case 0: rval = MINIRV32_LOAD1_SIGNED( rsval ); break;
							case 1: rval = MINIRV32_LOAD2_SIGNED( rsval ); break;
							case 2: rval = MINIRV32_LOAD4( rsval ); break;
							case 4: rval = MINIRV32_LOAD1( rsval ); break;
							case 5: rval = MINIRV32_LOAD2( rsval ); break;
							default: trap = (2+1);
						}
					}
					break;
				}
				case 0x23: // Store 0b0100011
				{
					uint32_t rs1 = REG((ir >> 15) & 0x1f);
					uint32_t rs2 = REG((ir >> 20) & 0x1f);
					uint32_t addy = ( ( ir >> 7 ) & 0x1f ) | ( ( ir & 0xfe000000 ) >> 20 );
					if( addy & 0x800 ) addy |= 0xfffff000;
					addy += rs1; // Virtual address.
					rdid = 0;

					uint32_t st_ofs;
					uint32_t st_xlate_trap = MiniRV32IMATranslate( state, image, addy, MINIRV32_ACCESS_STORE, ram_size, &st_ofs );
					if( MINIRV32_UNLIKELY( st_xlate_trap ) )
					{
						trap = st_xlate_trap;
						rval = addy;
						break;
					}
					addy = st_ofs;

					if( addy >= ram_size-3 )
					{
						addy += MINIRV32_RAM_IMAGE_OFFSET;
						if( MINIRV32_MMIO_RANGE( addy ) )
						{
							MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, rs2 );
						}
						else
						{
							trap = (7+1); // Store access fault.
							rval = addy;
						}
					}
					else
					{
						switch( ( ir >> 12 ) & 0x7 )
						{
							//SB, SH, SW
							case 0: MINIRV32_STORE1( addy, rs2 ); break;
							case 1: MINIRV32_STORE2( addy, rs2 ); break;
							case 2: MINIRV32_STORE4( addy, rs2 ); break;
							default: trap = (2+1);
						}
					}
					break;
				}
				case 0x13: // Op-immediate 0b0010011
				case 0x33: // Op           0b0110011
				{
					uint32_t imm = ir >> 20;
					imm = imm | (( imm & 0x800 )?0xfffff000:0);
					uint32_t rs1 = REG((ir >> 15) & 0x1f);
					uint32_t is_reg = !!( ir & 0x20 );
					uint32_t rs2 = is_reg ? REG(imm & 0x1f) : imm;

					if( is_reg && ( ir & 0x02000000 ) )
					{
						switch( (ir>>12)&7 ) //0x02000000 = RV32M
						{
							case 0: rval = rs1 * rs2; break; // MUL
#ifndef CUSTOM_MULH // If compiling on a system that doesn't natively, or via libgcc support 64-bit math.
							case 1: rval = ((int64_t)((int32_t)rs1) * (int64_t)((int32_t)rs2)) >> 32; break; // MULH
							case 2: rval = ((int64_t)((int32_t)rs1) * (uint64_t)rs2) >> 32; break; // MULHSU
							case 3: rval = ((uint64_t)rs1 * (uint64_t)rs2) >> 32; break; // MULHU
#else
							CUSTOM_MULH
#endif
							case 4: if( rs2 == 0 ) rval = -1; else rval = ((int32_t)rs1 == INT32_MIN && (int32_t)rs2 == -1) ? rs1 : ((int32_t)rs1 / (int32_t)rs2); break; // DIV
							case 5: if( rs2 == 0 ) rval = 0xffffffff; else rval = rs1 / rs2; break; // DIVU
							case 6: if( rs2 == 0 ) rval = rs1; else rval = ((int32_t)rs1 == INT32_MIN && (int32_t)rs2 == -1) ? 0 : ((uint32_t)((int32_t)rs1 % (int32_t)rs2)); break; // REM
							case 7: if( rs2 == 0 ) rval = rs1; else rval = rs1 % rs2; break; // REMU
						}
					}
					else
					{
						switch( (ir>>12)&7 ) // These could be either op-immediate or op commands.  Be careful.
						{
							case 0: rval = (is_reg && (ir & 0x40000000) ) ? ( rs1 - rs2 ) : ( rs1 + rs2 ); break;
							case 1: rval = rs1 << (rs2 & 0x1F); break;
							case 2: rval = (int32_t)rs1 < (int32_t)rs2; break;
							case 3: rval = rs1 < rs2; break;
							case 4: rval = rs1 ^ rs2; break;
							case 5: rval = (ir & 0x40000000 ) ? ( ((int32_t)rs1) >> (rs2 & 0x1F) ) : ( rs1 >> (rs2 & 0x1F) ); break;
							case 6: rval = rs1 | rs2; break;
							case 7: rval = rs1 & rs2; break;
						}
					}
					break;
				}
				case 0x0f: // 0b0001111
					rdid = 0;   // fencetype = (ir >> 12) & 0b111; We ignore fences in this impl.
					break;
				case 0x73: // Zifencei+Zicsr  (0b1110011)
				{
					uint32_t csrno = ir >> 20;
					uint32_t microop = ( ir >> 12 ) & 0x7;
					if( (microop & 3) ) // It's a Zicsr function.
					{
						int rs1imm = (ir >> 15) & 0x1f;
						uint32_t rs1 = REG(rs1imm);
						uint32_t writeval = rs1;

						// https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
						// Generally, support for Zicsr
						switch( csrno )
						{
						case 0x340: rval = CSR( mscratch ); break;
						case 0x305: rval = CSR( mtvec ); break;
						case 0x304: rval = CSR( mie ); break;
						case 0xC00: rval = cycle; break;
						case 0xC80: rval = CSR( cycleh ); break;
						// time/timeh MUST read the same clock sbi_set_timer()
						// arms (timerl/timerh, 1MHz wall time): the kernel
						// programs its next tick as rdtime+delta, so if rdtime
						// read anything else (or fell through to 0), every
						// programmed expiry would already be in the past and
						// the guest would live-lock in a timer-interrupt storm.
						case 0xC01: rval = CSR( timerl ); break;
						case 0xC81: rval = CSR( timerh ); break;
						case 0x344: rval = CSR( mip ); break;
						case 0x341: rval = CSR( mepc ); break;
						case 0x300: rval = CSR( mstatus ); break; //mstatus
						case 0x342: rval = CSR( mcause ); break;
						case 0x343: rval = CSR( mtval ); break;
						case 0xf11: rval = 0xff0ff0ff; break; //mvendorid
						case 0x301: rval = 0x40441101; break; //misa (XLEN=32, IMASU+X)
						case 0x100: rval = CSR( mstatus ) & MINIRV32_SSTATUS_MASK; break; // sstatus
						case 0x104: rval = CSR( mie ) & MINIRV32_SIE_MASK; break; // sie
						case 0x105: rval = CSR( stvec ); break;
						case 0x140: rval = CSR( sscratch ); break;
						case 0x141: rval = CSR( sepc ); break;
						case 0x142: rval = CSR( scause ); break;
						case 0x143: rval = CSR( stval ); break;
						case 0x144: rval = CSR( mip ) & MINIRV32_SIE_MASK; break; // sip (same bit positions as sie)
						case 0x180: rval = CSR( satp ); break;
						case 0x302: rval = CSR( medeleg ); break;
						case 0x303: rval = CSR( mideleg ); break;
						//case 0x3B0: rval = 0; break; //pmpaddr0
						//case 0x3a0: rval = 0; break; //pmpcfg0
						//case 0xf12: rval = 0x00000000; break; //marchid
						//case 0xf13: rval = 0x00000000; break; //mimpid
						//case 0xf14: rval = 0x00000000; break; //mhartid
						default:
							MINIRV32_OTHERCSR_READ( csrno, rval );
							break;
						}

						switch( microop )
						{
							case 1: writeval = rs1; break;  			//CSRRW
							case 2: writeval = rval | rs1; break;		//CSRRS
							case 3: writeval = rval & ~rs1; break;		//CSRRC
							case 5: writeval = rs1imm; break;			//CSRRWI
							case 6: writeval = rval | rs1imm; break;	//CSRRSI
							case 7: writeval = rval & ~rs1imm; break;	//CSRRCI
						}

						switch( csrno )
						{
						case 0x340: SETCSR( mscratch, writeval ); break;
						case 0x305: SETCSR( mtvec, writeval ); break;
						case 0x304: SETCSR( mie, writeval ); break;
						case 0x344: SETCSR( mip, writeval ); break;
						case 0x341: SETCSR( mepc, writeval ); break;
						case 0x300: SETCSR( mstatus, writeval ); break; //mstatus
						case 0x342: SETCSR( mcause, writeval ); break;
						case 0x343: SETCSR( mtval, writeval ); break;
						case 0x100: SETCSR( mstatus, ( CSR( mstatus ) & ~MINIRV32_SSTATUS_MASK ) | ( writeval & MINIRV32_SSTATUS_MASK ) ); break;
						case 0x104: SETCSR( mie, ( CSR( mie ) & ~MINIRV32_SIE_MASK ) | ( writeval & MINIRV32_SIE_MASK ) ); break;
						case 0x105: SETCSR( stvec, writeval ); break;
						case 0x140: SETCSR( sscratch, writeval ); break;
						case 0x141: SETCSR( sepc, writeval ); break;
						case 0x142: SETCSR( scause, writeval ); break;
						case 0x143: SETCSR( stval, writeval ); break;
						case 0x144: SETCSR( mip, ( CSR( mip ) & ~MINIRV32_SIP_WMASK ) | ( writeval & MINIRV32_SIP_WMASK ) ); break;
						case 0x180: SETCSR( satp, writeval ); MiniRV32IMATLBFlush( state ); break; // Address space (possibly) changed: stale translations must go.
						case 0x302: SETCSR( medeleg, writeval ); break;
						case 0x303: SETCSR( mideleg, writeval ); break;
						//case 0x3a0: break; //pmpcfg0
						//case 0x3B0: break; //pmpaddr0
						//case 0xf11: break; //mvendorid
						//case 0xf12: break; //marchid
						//case 0xf13: break; //mimpid
						//case 0xf14: break; //mhartid
						//case 0x301: break; //misa
						default:
							MINIRV32_OTHERCSR_WRITE( csrno, writeval );
							break;
						}
					}
					else if( microop == 0x0 ) // "SYSTEM" 0b000
					{
						rdid = 0;
						if( csrno == 0x302 )  // MRET
						{
							//https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
							//Table 7.6. MRET then in mstatus/mstatush sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. La
							// Should also update mstatus to reflect correct mode.
							uint32_t startmstatus = CSR( mstatus );
							uint32_t startextraflags = CSR( extraflags );
							SETCSR( mstatus , (( startmstatus & 0x80) >> 4) | ((startextraflags&3) << 11) | 0x80 );
							SETCSR( extraflags, (startextraflags & ~3) | ((startmstatus >> 11) & 3) );
							pc = CSR( mepc ) -4;
						}
						else if( csrno == 0x102 ) // SRET
						{
							uint32_t startmstatus = CSR( mstatus );
							uint32_t spp = ( startmstatus >> 8 ) & 1;
							uint32_t spie = ( startmstatus >> 5 ) & 1;
							uint32_t newmstatus = ( startmstatus & ~( (1u<<1) | (1u<<5) | (1u<<8) ) ) | ( spie << 1 ) | ( 1u << 5 );
							SETCSR( mstatus, newmstatus );
							SETCSR( extraflags, ( CSR( extraflags ) & ~3u ) | spp );
							pc = CSR( sepc ) - 4;
						}
						else if( ( csrno & 0xfe0 ) == 0x120 ) // SFENCE.VMA (funct7=0001001, rs1/rs2 in the low bits)
						{
							// rs1/rs2 can request a narrower flush (single
							// address / single ASID); we don't track ASIDs
							// and the TLB is small, so always flushing
							// everything is simplest and always correct,
							// just occasionally broader than strictly
							// required.
							MiniRV32IMATLBFlush( state );
						} else {
							switch (csrno) {
							case 0:
							{
								uint32_t curpriv = CSR( extraflags ) & 3;
								trap = ( curpriv == 3 ) ? (11+1) : ( curpriv == 1 ? (9+1) : (8+1) ); // ECALL from M/S/U mode respectively.
								break;
							}
							case 1:
								trap = (3+1); break; // EBREAK 3 = "Breakpoint"
							case 0x105: //WFI (Wait for interrupts)
								CSR( mstatus ) |= 8;    //Enable interrupts
								CSR( extraflags ) |= 4; //Infor environment we want to go to sleep.

								if( CSR( cyclel ) > cycle ) CSR( cycleh )++;
								SETCSR( cyclel, cycle );

								MINIRV32_POSTEXEC( pc, ir, trap );

								SETCSR( pc, pc + 4 );
								return 1;
							default:
								trap = (2+1); break; // Illegal opcode.
							}
						}
					}
					else
						trap = (2+1); 				// Note micrrop 0b100 == undefined.
					break;
				}
				case 0x2f: // RV32A (0b00101111)
				{
					uint32_t rs1v = REG((ir >> 15) & 0x1f); // Virtual address.
					uint32_t rs2 = REG((ir >> 20) & 0x1f);
					uint32_t irmid = ( ir>>27 ) & 0x1f;

					// We don't implement load/store from UART or CLNT with RV32A here.

					uint32_t rs1;
					uint32_t amo_xlate_trap = MiniRV32IMATranslate( state, image, rs1v, MINIRV32_ACCESS_STORE, ram_size, &rs1 );
					if( MINIRV32_UNLIKELY( amo_xlate_trap ) )
					{
						trap = amo_xlate_trap;
						rval = rs1v;
						break;
					}

					if( MINIRV32_UNLIKELY( rs1 >= ram_size-3 ) )
					{
						trap = (7+1); //Store/AMO access fault
						rval = rs1 + MINIRV32_RAM_IMAGE_OFFSET;
					}
					else
					{
						rval = MINIRV32_LOAD4( rs1 );

						// Referenced a little bit of https://github.com/franzflasch/riscv_em/blob/master/src/core/core.c
						uint32_t dowrite = 1;
						switch( irmid )
						{
							case 2: //LR.W (0b00010)
								dowrite = 0;
								CSR( extraflags ) = (CSR( extraflags ) & 0x07) | (rs1<<3);
								break;
							case 3:  //SC.W (0b00011) (Make sure we have a slot, and, it's valid)
								rval = ( CSR( extraflags ) >> 3 != ( rs1 & 0x1fffffff ) );  // Validate that our reservation slot is OK.
								dowrite = !rval; // Only write if slot is valid.
								break;
							case 1: break; //AMOSWAP.W (0b00001)
							case 0: rs2 += rval; break; //AMOADD.W (0b00000)
							case 4: rs2 ^= rval; break; //AMOXOR.W (0b00100)
							case 12: rs2 &= rval; break; //AMOAND.W (0b01100)
							case 8: rs2 |= rval; break; //AMOOR.W (0b01000)
							case 16: rs2 = ((int32_t)rs2<(int32_t)rval)?rs2:rval; break; //AMOMIN.W (0b10000)
							case 20: rs2 = ((int32_t)rs2>(int32_t)rval)?rs2:rval; break; //AMOMAX.W (0b10100)
							case 24: rs2 = (rs2<rval)?rs2:rval; break; //AMOMINU.W (0b11000)
							case 28: rs2 = (rs2>rval)?rs2:rval; break; //AMOMAXU.W (0b11100)
							default: trap = (2+1); dowrite = 0; break; //Not supported.
						}
						if( dowrite ) MINIRV32_STORE4( rs1, rs2 );
					}
					break;
				}
				default: trap = (2+1); // Fault: Invalid opcode.
			}

			// If there was a trap, do NOT allow register writeback.
			if( MINIRV32_UNLIKELY( trap ) ) {
				SETCSR( pc, pc );
				MINIRV32_POSTEXEC( pc, ir, trap );
				if( trap ) break; // Not absorbed by the hook: fall through to real trap delivery below.
				// The hook fully serviced this itself (e.g. an SBI ecall) -
				// treat it exactly like a normally-completed instruction
				// (no register writeback either way: every trapping case
				// above already leaves rdid at 0 or unused) and move on.
				pc += 4;
				continue;
			}

			if( rdid )
			{
				REGSET( rdid, rval ); // Write back register.
			}
		}

		MINIRV32_POSTEXEC( pc, ir, trap );

		pc += 4;

		// Interrupts are only evaluated at the top of MiniRV32IMAStep, but a
		// SYSTEM instruction (CSR write, SRET/MRET) can unmask a pending one
		// mid-batch - e.g. the kernel's idle loop re-enabling sstatus.SIE
		// right after a wfi. Since wfi ends every batch, without this the
		// batch boundary would always land where interrupts are still masked
		// and a pending STIP/SEIP could never be delivered (a total deadlock
		// at large `count` values). End the batch after any SYSTEM
		// instruction while an enabled interrupt is pending so the top-of-
		// step logic gets an immediate chance to deliver it.
		if( ( ir & 0x7f ) == 0x73 && ( CSR( mip ) & CSR( mie ) ) )
			break;
	}

	// Handle traps and interrupts.
	if( trap )
	{
		uint32_t is_interrupt = trap & 0x80000000;
		uint32_t cause = is_interrupt ? ( trap & 0x7fffffff ) : ( trap - 1 );
		uint32_t curpriv = CSR( extraflags ) & 3;
		uint32_t delegated = ( curpriv != 3 ) &&
			( is_interrupt ? ( ( CSR( mideleg ) >> cause ) & 1 ) : ( ( CSR( medeleg ) >> cause ) & 1 ) );

		if( is_interrupt ) pc += 4; // PC needs to point to where the PC will return to.

		// tval should carry the faulting address for access-fault/misaligned
		// traps (6-8, upstream's set) and for our added page-fault causes
		// (13=instr, 14=load, 16=store - see MiniRV32IMATranslate) - all of
		// which stash the offending virtual address in rval. Every other
		// trap (e.g. illegal instruction, ecall) carries the faulting pc.
		uint32_t tval = is_interrupt ? 0 : ( ( ( trap > 5 && trap <= 8 ) || trap == 13 || trap == 14 || trap == 16 ) ? rval : pc );
		uint32_t causeval = is_interrupt ? trap : cause;

		if( delegated )
		{
			SETCSR( scause, causeval );
			SETCSR( stval, tval );
			SETCSR( sepc, pc );

			uint32_t ms = CSR( mstatus );
			uint32_t sie = ( ms >> 1 ) & 1;
			// SPIE = SIE, SIE = 0, SPP = privilege we trapped from.
			ms = ( ms & ~( (1u<<1) | (1u<<5) | (1u<<8) ) ) | ( sie << 5 ) | ( curpriv << 8 );
			SETCSR( mstatus, ms );

			pc = CSR( stvec ) - 4;
			SETCSR( extraflags, ( CSR( extraflags ) & ~3u ) | 1 ); // -> S-mode
		}
		else
		{
			SETCSR( mcause, causeval );
			SETCSR( mtval, tval );
			SETCSR( mepc, pc );
			//CSR( mstatus ) & 8 = MIE, & 0x80 = MPIE
			// On an interrupt, the system moves current MIE into MPIE
			SETCSR( mstatus, (( CSR( mstatus ) & 0x08) << 4) | ( curpriv << 11 ) );
			pc = CSR( mtvec ) - 4;
			SETCSR( extraflags, ( CSR( extraflags ) & ~3u ) | 3 ); // -> M-mode
		}

		trap = 0;
		pc += 4;
	}

	if( CSR( cyclel ) > cycle ) CSR( cycleh )++;
	SETCSR( cyclel, cycle );
	SETCSR( pc, pc );
	return 0;
}

#endif

#endif

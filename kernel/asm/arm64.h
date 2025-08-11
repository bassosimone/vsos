// File: kernel/asm/arm64.h
// Purpose: ARM64 assembly routines
// SPDX-License-Identifier: MIT
#ifndef KERNEL_ASM_ARM64_H
#define KERNEL_ASM_ARM64_H

#include <sys/types.h>

// DSB: data synchronization barrier using sy.
//
// Similar to Zig's `.SeqCst`.
//
// Use this for:
//
// 1. MMIO device register writes
//
// 2. Page table updates
//
// 3. System state transitions
static inline void dsb_sy(void) {
	__asm__ volatile("dsb sy" ::: "memory");
}

// WFI: wait for interrupts
static inline void wfi(void) {
	__asm__ volatile("wfi");
}

// ISB: instruction synchronization barrier.
//
// Required after system register writes (e.g., TTBR, CPACR).
static inline void isb(void) {
	__asm__ volatile("isb" ::: "memory");
}

// Enable or disable access to FP/SIMD registers at EL0 and EL1.
//
// The CPACR_EL1 register controls this access via bits 20:21 (FPEN):
//
// - 0b00: Trap FP/SIMD at both EL0 and EL1
// - 0b01: Trap at EL0 only
// - 0b11: Allow FP/SIMD at EL0 and EL1
//
// Note: despite the _EL1 suffix, this register controls *both* EL0 and EL1
// behavior, from the perspective of EL1.
static inline void __enable_disable_fp_simd(bool enable) {
	// Read the register
	uint64_t cpacr;
	__asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));

	// Either set FPEN to 0b11 or clear it
	if (enable) {
		cpacr |= (3 << 20);
	} else {
		cpacr &= ~(3 << 20);
	}

	// Write the bits back
	__asm__ volatile("msr cpacr_el1, %0" ::"r"(cpacr));

	// Ensure the change takes effect immediately
	isb();
}

// Enable FP/SIMD for both kernel (EL1) and user space (EL0).
//
// Required if Clang generates NEON or floating-point instructions (e.g. for
// `printk`, `memcpy`, etc.). Without this, such instructions trap at 0x200 with
// exception class 0x7 (Undefined Instruction).
static inline void enable_fp_simd(void) {
	__enable_disable_fp_simd(true);
}

// Disallow FP/SIMD usage at EL0 and EL1 and restores traps on use.
static inline void disable_fp_simd(void) {
	__enable_disable_fp_simd(false);
}

// DMB: data memory barrier using `sy.`
static inline void dmb_sy(void) {
	__asm__ volatile("dmb sy" ::: "memory");
}

// DMB: data memory barrier using `ish`.
static inline void dmb_ish(void) {
	__asm__ volatile("dmb ish" ::: "memory");
}

// DMB: data memory barrier using `ishst`.
static inline void dmb_ishst(void) {
	__asm__ volatile("dmb ishst" ::: "memory");
}

// Disable IRQ interrupts by setting the I-bit in PSTATE.DAIF.
//
// DAIF = Debug mask (bit 9), SError mask (bit 8), IRQ mask (bit 7), FIQ mask (bit 6).
//
// The `msr daifset, #imm` instruction sets (masks) whichever bits are 1 in `imm`:
//
//   #8 → D-bit, #4 → A-bit, #2 → I-bit, #1 → F-bit.
//
// Therefore, `#2` masks IRQ only, leaving FIQ, SError, Debug unchanged.
static inline void msr_daifset_2(void) {
	__asm__ volatile("msr daifset, #2" ::: "memory");
}

// Enable IRQ interrupts by clearing the I-bit in PSTATE.DAIF.
//
// DAIF bit mapping: D=8, A=4, I=2, F=1. (#2 → IRQ mask bit).
//
// This is the inverse of msr_daifset_2(): it unmasks IRQ only,
// leaving FIQ, SError, and Debug mask bits unchanged.
static inline void msr_daifclr_2(void) {
	__asm__ volatile("msr daifclr, #2" ::: "memory");
}

// DSB: data synchronization barrier using ishst.
static inline void dsb_ishst(void) {
	__asm__ volatile("dsb ishst" ::: "memory");
}

// Write MAIR_EL1
static inline void msr_mair_el1(uint64_t val) {
	__asm__ volatile("msr mair_el1, %0" ::"r"(val) : "memory");
}

// Write TCR_EL1
static inline void msr_tcr_el1(uint64_t val) {
	__asm__ volatile("msr tcr_el1, %0" ::"r"(val) : "memory");
}

// Write TTBR1_EL1
static inline void msr_ttbr1_el1(uint64_t val) {
	__asm__ volatile("msr ttbr1_el1, %0" ::"r"(val) : "memory");
}

// Write TTBR0_EL1
static inline void msr_ttbr0_el1(uint64_t val) {
	__asm__ volatile("msr ttbr0_el1, %0" ::"r"(val) : "memory");
}

// Read SCTLR_EL1
static inline uint64_t mrs_sctlr_el1(void) {
	uint64_t v;
	__asm__ volatile("mrs %0, sctlr_el1" : "=r"(v));
	return v;
}

// Write SCTLR_EL1
static inline void msr_sctlr_el1(uint64_t val) {
	__asm__ volatile("msr sctlr_el1, %0" ::"r"(val) : "memory");
}

// Write VBAR_EL1
static inline void msr_vbar_el1(uint64_t v) {
	__asm__ volatile("msr vbar_el1, %0" ::"r"(v) : "memory");
}

// Returns the number of ticks per second used by the hardware.
static inline uint64_t mrs_cntfrq_el0(void) {
	uint64_t v;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
	return v;
}

// Program the clock to fire after the given amount of time has elapsed.
static inline void msr_cntp_tval_el0(uint64_t v) {
	__asm__ volatile("msr cntp_tval_el0, %0" ::"r"(v));
}

// Enable the clock and unmask its interrupt when v is equal to 1.
static inline void msr_cntp_ctl_el0(uint64_t v) {
	__asm__ volatile("msr cntp_ctl_el0, %0" ::"r"(v));
}

// Perform a MMIO uint32_t read at the given address.
//
// We place a `dmb_ish` barrier after the load.
static inline uint32_t mmio_read_uint32(volatile uint32_t *address) {
	uint32_t value = *address;
	dmb_ish();
	return value;
}

// Like mmio_read_uint32 but for an uint8_t.
static inline uint8_t mmio_read_uint8(volatile uint8_t *address) {
	uint8_t value = *address;
	dmb_ish();
	return value;
}

// Perform a MMIO uint32_t write at the given address.
//
// We place a `dmb_ishst` barrier before the store.
static inline void mmio_write_uint32(volatile uint32_t *address, uint32_t value) {
	dmb_ishst();
	*address = value;
}

// Like mmio_write_uint32 but for an uint8_t.
static inline void mmio_write_uint8(volatile uint8_t *address, uint8_t value) {
	dmb_ishst();
	*address = value;
}

// Puts the CPU in low-power state until an interrupt occurs.
static inline void cpu_sleep_until_interrupt(void) {
	wfi();
}

// Disables interrupts unconditionally.
static inline void local_irq_disable(void) {
	msr_daifset_2();
}

// Enables interrupts unconditionally.
static inline void local_irq_enable(void) {
	msr_daifclr_2();
}

#endif // KERNEL_ASM_ARM64

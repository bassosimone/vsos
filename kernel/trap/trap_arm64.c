// File: kernel/trap/trap_arm64.c
// Purpose: ARM64 code to handle traps.
// SPDX-License-Identifier: MIT

#include <kernel/asm/arm64.h>		// for msr_vbar_el1
#include <kernel/boot/boot.h>		// for __vectors_el1
#include <kernel/drivers/gicv2_arm64.h> // for struct gicv2_device
#include <kernel/sched/sched.h>		// for sched_clock_init_irqs
#include <kernel/trap/trap.h>		// for trap_init_mm
#include <kernel/trap/trap_arm64.h>	// for struct trap_frame
#include <kernel/tty/uart.h>		// for uart_init_irqs

#include <sys/types.h> // for uint64_t

#include <unistd.h> // for syscall

// Base and limit memory addresses for the GICC.
#define GICC_BASE 0x08010000UL

// Base and limit memory addresses for the GICD.
#define GICD_BASE 0x08000000UL

// The ARM Generic Timer (physical EL1) PPI is INTID 30 on GIC (per-cpu)
#define IRQ_PPI_CNTP 30u

// The UART0 (PL011) on QEMU virt
#define UART0_INTID 33u

// The global irq0 device driver attached to the GICCv2.
struct gicv2_device irq0;

void trap_init_mm(void) {
	gicv2_init_struct(&irq0, GICC_BASE, GICD_BASE, "irq0");
	gicv2_init_mm(&irq0);
}

static inline void __enable_timer_irq(void) {
	gicv2_enable_ppi(&irq0, IRQ_PPI_CNTP, 0x80);
}

static inline void __enable_uart_irq(void) {
	gicv2_enable_spi_level_cpu0(&irq0, UART0_INTID, 0x80);
}

void trap_init_irqs(void) {
	// Set the vector interrupt table
	msr_vbar_el1((uint64_t)__vectors_el1);
	isb();

	// Reset interrupt controller
	gicv2_reset(&irq0);

	// Program devices (group/prio/route/trigger and set-enable)
	__enable_timer_irq();
	__enable_uart_irq();

	// Re-enable interrupt controller
	gicv2_enable(&irq0);

	// Start IRQ for other subsystems
	sched_clock_init_irqs();
	uart_init_irqs();
}

void __trap_isr(struct trap_frame *frame) {
	(void)frame;

	// Acknowledge the IRQ and get the context
	uint32_t iar = 0;
	uint32_t irqid = 0;
	if (!gicv2_acknowledge_irq(&irq0, &iar, &irqid)) {
		return;
	}

	// Handle each IRQ type
	switch (irqid) {
	case IRQ_PPI_CNTP:
		sched_clock_isr();
		break;

	case UART0_INTID:
		uart_isr();
		break;
	}

	// We're done handling this interrupt
	gicv2_end_of_interrupt(&irq0, iar);
}

void __trap_ssr(struct trap_frame *frame, uint64_t esr, uint64_t far) {
	// TODO(bassosimone): we should use these
	(void)esr;
	(void)far;

	// We pass the syscall number as x8.
	//
	// Keep in sync with `./libc/unistd/syscall_arm64.c`.
	frame->x[0] = syscall(
	    frame->x[8], frame->x[0], frame->x[1], frame->x[2], frame->x[3], frame->x[4], frame->x[5]);
}

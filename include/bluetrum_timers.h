#ifndef BLUETRUM_TIMERS_H
#define BLUETRUM_TIMERS_H

#include <stdint.h>
#include "sfr.h"

#define IRQ_TMR0_VECTOR   -1 // can't find interrupt?
#define IRQ_TMR1_VECTOR   4

typedef struct {
	volatile uint32_t *CON;
	volatile uint32_t *CPND;
	volatile uint32_t *CNT;
	volatile uint32_t *PR;
	uint8_t irq_vector;
} timer_regs_t;

// Lookup table for timer registers and vectors
static const timer_regs_t timer_regs[2] = {
	{
		.CON = &TMR0CON,
		.CPND = &TMR0CPND,
		.CNT = &TMR0CNT,
		.PR = &TMR0PR,
		.irq_vector = IRQ_TMR0_VECTOR
	},
	{
		.CON = &TMR1CON,
		.CPND = &TMR1CPND,
		.CNT = &TMR1CNT,
		.PR = &TMR1PR,
		.irq_vector = IRQ_TMR1_VECTOR
	}
};

volatile uint32_t systick;

__attribute__((section(".isr"), interrupt))
void TMR1_isr(void) {
	// Acknowledge the timer interrupt
	TMR1CPND = BIT(9);
	systick++;
}

void timer_init(int timer_idx, uint32_t period_ticks) {
	if (timer_idx >= 2) return;

	const timer_regs_t *regs = &timer_regs[timer_idx];

	// Reset counter
	*regs->CNT = 0;

	// Set reload period
	*regs->PR = (period_ticks - 1);

	if(regs->irq_vector > 0) {
		// Disable global interrupts temporarily
		uint32_t cpu_ie = PICCON & BIT(0);
		PICCON &= ~BIT(0);

		// Configure timer control register
		// Bit 7: TIE = Timer Interrupt Enable
		*regs->CON = BIT(7);

		// Configure PIC (Programmable Interrupt Controller)
		// Set priority (optional)
		PICPR &= ~BIT(regs->irq_vector);

		// Enable this specific interrupt in PICEN
		PICEN |= BIT(regs->irq_vector);

		// Restore global interrupt enable state
		PICCON |= cpu_ie;
	}

	// Start the timer
	// Bit 0: EN = Timer Enable
	*regs->CON |= BIT(0);
}

void timer_stop(int timer_idx) {
	if (timer_idx >= 2) return;

	const timer_regs_t *regs = &timer_regs[timer_idx];

	// Disable global interrupts temporarily
	uint32_t cpu_ie = PICCON & BIT(0);
	PICCON &= ~BIT(0);

	// Clear timer enable bit
	*regs->CON &= ~BIT(0);

	// Disable in PICEN
	PICEN &= ~BIT(regs->irq_vector);

	// Restore global interrupt enable state
	PICCON |= cpu_ie;
}

uint32_t timer_get_count(int timer_idx) {
	if (timer_idx < 2) {
		return *timer_regs[timer_idx].CNT;
	}
	return 0;
}

#endif // BLUETRUM_TIMERS_H

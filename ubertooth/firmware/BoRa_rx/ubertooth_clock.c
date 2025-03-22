/*
 * Copyright 2015 Hannes Ellinger
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ubertooth_clock.h"
#include "ubertooth.h"

volatile uint32_t last_hop;

volatile uint32_t clkn_offset;
volatile uint16_t clk100ns_offset;

// linear clock drift
volatile int16_t clk_drift_ppm;
volatile uint16_t clk_drift_correction;

volatile uint32_t clkn_last_drift_fix;
volatile uint32_t clkn_next_drift_fix;

volatile uint8_t t0_int;

volatile uint32_t st, et;

void TIMER0_IRQHandler()
{
	if (T0IR & TIR_MR0_Interrupt) {
		/* Ack interrupt */
		T0IR = TIR_MR0_Interrupt;
		t0_int = 1;
	}
}

void clkn_stop()
{
	/* stop and reset the timer to zero */
	T0TCR = TCR_Counter_Reset;
	T1TCR = TCR_Counter_Reset;
}

void clkn_start()
{
	/* start timer */
	T0TCR = TCR_Counter_Enable;
}

void clkn_init()
{
	/*
	 * Because these are reset defaults, we're assuming TIMER0 is powered on
	 * and in timer mode.  The TIMER0 peripheral clock should have been set by
	 * clock_start().
	 */

	clkn_stop();

#ifdef TC13BADGE
	/*
	 * The peripheral clock has a period of 33.3ns.  3 pclk periods makes one
	 * CLK100NS period (100 ns).
	 */
	T0PR = 2;
#else
	/*
	 * The peripheral clock has a period of 20ns.  5 pclk periods
	 * makes one CLK100NS period (100 ns).
	 */
	T0PR = 0;
	T1PR = 0;
#endif
	T0MCR = TMCR_MR0I;
	// T1MCR = TMCR_MR0I;
	T1MCR = TMCR_MR0I | TMCR_MR0R;
	// ISER0 = ISER0_ISE_TIMER0;
}

// totally disable clkn and timer0
void clkn_disable(void) {
	clkn_stop();
	ICER0 = ICER0_ICE_TIMER0;
	ICER0 = ICER0_ICE_TIMER1;
}

void usleep(uint32_t us)
{
	st = T0TC;
	T0MR0 = st + us * 100;
	t0_int = 0;
	ISER0 = ISER0_ISE_TIMER0;
	while (!t0_int) {}
	ICER0 = ICER0_ICE_TIMER0;
}

void msleep(uint32_t ms)
{
	usleep(ms * 1000);
}

void usleep_noint(uint32_t us)
{
	uint32_t start_time = T0TC;
	uint32_t end_time = start_time + us * 100;
	while (T0TC < end_time) {}
}
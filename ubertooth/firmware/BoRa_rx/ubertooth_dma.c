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

#include "ubertooth_dma.h"

volatile uint8_t rxbuf1[DMA_SIZE];
volatile uint8_t rxbuf2[DMA_SIZE];
// 0x8E89BED6
volatile uint8_t txbuf[7] = {0x55, 107, 125, 145, 113, 0xff, 0xff};

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
volatile uint8_t* volatile active_rxbuf;
volatile uint8_t* volatile idle_rxbuf;

/* rx terminal count and error interrupt counters */
volatile uint32_t rx_tc;
volatile uint32_t rx_err;

dma_lli rx_dma_lli1;
dma_lli rx_dma_lli2;

dma_lli le_dma_lli[11]; // 11 x 4 bytes
dma_lli continue_dma_lli[7];


void dma_poweron(void) {
	// refer to UM10360 LPC17xx User Manual Ch 31 Sec 31.6.1, PDF page 616
	PCONP |= PCONP_PCGPDMA;

	// enable DMA interrupts at lowest priority
	IPR6 |= IPR6_IP_DMA; // hack, sets it to 31 (lowest)
	ISER0 = ISER0_ISE_DMA;

	// disable active channels
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));
	
}

void dma_poweroff(void) {
	// Disable the DMA controller by writing 0 to the DMA Enable bit in the DMACConfig
	// register.
	DMACConfig &= ~DMACConfig_E;
	while (DMACConfig & DMACConfig_E);

	ICER0 = ICER0_ICE_DMA;

	PCONP &= ~PCONP_PCGPDMA;
}

void dma_clear_interrupts(unsigned channel) {
	DMACIntTCClear = 1 << channel;
	DMACIntErrClr  = 1 << channel;
}

void dma_init_rx_symbols(void) {
	dma_clear_interrupts(0);

	/* DMA linked lists */
	rx_dma_lli1.src = (uint32_t)&(DIO_SSP_DR);
	rx_dma_lli1.dest = (uint32_t)&rxbuf1[0];
	rx_dma_lli1.next_lli = (uint32_t)&rx_dma_lli2;
	rx_dma_lli1.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	rx_dma_lli2.src = (uint32_t)&(DIO_SSP_DR);
	rx_dma_lli2.dest = (uint32_t)&rxbuf2[0];
	rx_dma_lli2.next_lli = (uint32_t)&rx_dma_lli1;
	rx_dma_lli2.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	/* configure DMA channel 0 
	   In Rx, we use DMA0,1 to collect the ADC input
	*/
	DMACC0SrcAddr = rx_dma_lli1.src;
	DMACC0DestAddr = rx_dma_lli1.dest;
	DMACC0LLI = rx_dma_lli1.next_lli;
	DMACC0Control = rx_dma_lli1.control;
	DMACC0Config = DIO_SSP_SRC
	               | (0x2 << 11)       /* peripheral to memory */
	               | DMACCxConfig_IE   /* allow error interrupts */
	               | DMACCxConfig_ITC; /* allow terminal count interrupts */

	rx_tc = 0;
	rx_err = 0;

	active_rxbuf = &rxbuf1[0];
	idle_rxbuf = &rxbuf2[0];

	// enable channel
	DMACC0Config |= 1;
}

// BoRa: to generate a continues wave
// uint32_t dur: the duration of the continous wave (us)
void bora_tx_dma_init()
{
	int i;
	dma_clear_interrupts(0);
	
	for (i = 0; i < 7; i++) {
		continue_dma_lli[i].src = (uint32_t)&(txbuf[i]);
		continue_dma_lli[i].dest = (uint32_t)&(DIO_SSP_DR);
		/* Since we only transmit the same byte, a cycle lli is enough */
		if (i < 6) {
			continue_dma_lli[i].next_lli = (uint32_t)&(continue_dma_lli[i+1]);
		}
		else {
			continue_dma_lli[i].next_lli = (uint32_t)&(continue_dma_lli[5]);
		}
		continue_dma_lli[i].control = 1 |
				(0 << 12) |        /* source burst size = 1 (RAM) */
				(0 << 15) |        /* destination burst size = 1 (DIO) */
				(0 << 18) |        /* source width 8 bits */
				(0 << 21) |        /* destination width 8 bits */
				DMACCxControl_I;   /* terminal count interrupt enable */
	}

	/* configure DMA channel 0 */
	DMACC0SrcAddr = continue_dma_lli[0].src;
	DMACC0DestAddr = continue_dma_lli[0].dest;
	DMACC0LLI = continue_dma_lli[0].next_lli;
	DMACC0Control = continue_dma_lli[0].control;
	DMACC0Config = 
		(2 << 6) |        /* SSP1 Tx */
		(0x1 << 11) |     /* memory to peripheral */
		DMACCxConfig_IE | /* allow error interrupts */
		DMACCxConfig_ITC; /* allow terminal count interrupts */
}

/* BoRa: start writting data to the un-buffered CC2400 via DMA */
void dio_ssp_write_start()
{
	DIO_SSEL_SET;
	DIO_SSP_DMACR = SSPDMACR_TXDMAE;
	DIO_SSP_CR1 = SSPCR1_SSE | SSPCR1_MS;
	
	// enable channel
	DMACC0Config |= 1;
	/* activate slave select pin */
	DIO_SSEL_CLR;
}

/* BoRa: Stop the DMA transmission */
void dio_ssp_write_stop()
{
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable DMA on SSP; disable SSP ; disable DMA channel
	DIO_SSP_DMACR &= ~SSPDMACR_TXDMAE;
	DIO_SSP_CR1 &= ~SSPCR1_SSE;
	DMACC0Config = 0;
	dma_clear_interrupts(0);
}

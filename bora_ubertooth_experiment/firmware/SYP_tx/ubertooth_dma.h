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

#ifndef __UBERTOOTH_DMA_H
#define __UBERTOOTH_DMA_H value

#include "inttypes.h"
#include "ubertooth.h"

/* DMA linked list items */
typedef struct {
	uint32_t src;
	uint32_t dest;
	uint32_t next_lli;
	uint32_t control;
} dma_lli;

extern volatile uint8_t rxbuf1[DMA_SIZE];
extern volatile uint8_t rxbuf2[DMA_SIZE];

extern volatile uint8_t txbuf1[DMA_SIZE];
extern volatile uint8_t txbuf2[DMA_SIZE];

extern volatile uint8_t test_buf[16];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
extern volatile uint8_t* volatile active_rxbuf;
extern volatile uint8_t* volatile idle_rxbuf;

extern volatile uint8_t* volatile active_txbuf;
extern volatile uint8_t* volatile idle_txbuf;

/* rx terminal count and error interrupt counters */
extern volatile uint32_t rx_tc;
extern volatile uint32_t rx_err;

extern volatile uint32_t gfsk_bytes_ct;
extern volatile uint32_t target_gfsk_bytes_ct;
extern volatile uint8_t txbuf[7];


void dma_poweron();
void dma_poweroff();
void dma_init_rx_symbols();
void dio_ssp_start();
void dio_ssp_stop();

void syp_dma_init();
void syp_tx_dma_init();
void dio_ssp_write_stop();
void dio_ssp_write_start();

#endif

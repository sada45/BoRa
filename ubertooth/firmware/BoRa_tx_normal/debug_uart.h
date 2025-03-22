/*
 * Copyright 2017 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#ifndef __DEBUG_UART_H__
#define __DEBUG_UART_H__

typedef struct dma_tx_cl{
    char *data;
    size_t len;
    struct dma_tx_cl *next;
} dma_tx_cl_t;

extern volatile dma_tx_cl_t *dma_uart_start;
extern volatile dma_tx_cl_t *dma_uart_end;

extern int debug_dma_active;

// initialize debug UART
void debug_uart_init(int flow_control);

// write a string to UART synchronously without DMA
// you probably don't want this
void debug_write(const char *str);

// printf to debug UART -- this is the one you want!
void debug_printf(char *fmt, ...);

void debug_send_dma(char *databuf, size_t size);

void pii();

#endif

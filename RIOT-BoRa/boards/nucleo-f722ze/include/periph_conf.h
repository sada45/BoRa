/*
 * Copyright (C) 2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_nucleo-f722ze
 * @{
 *
 * @file
 * @brief       Peripheral MCU configuration for the nucleo-f722ze board
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

/* This board provides an LSE */
#ifndef CONFIG_BOARD_HAS_LSE
#define CONFIG_BOARD_HAS_LSE    1
#endif

/* This board provides an HSE */
#ifndef CONFIG_BOARD_HAS_HSE
#define CONFIG_BOARD_HAS_HSE    1
#endif

#include "periph_cpu.h"
#include "clk_conf.h"
#include "cfg_i2c1_pb8_pb9.h"
#include "cfg_rtt_default.h"
#include "cfg_timer_tim2.h"
#include "cfg_usb_otg_fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        .dev        = USART3,
        .rcc_mask   = RCC_APB1ENR_USART3EN,
        .rx_pin     = GPIO_PIN(PORT_D, 9),
        .tx_pin     = GPIO_PIN(PORT_D, 8),
        .rx_af      = GPIO_AF7,
        .tx_af      = GPIO_AF7,
        .bus        = APB1,
        .irqn       = USART3_IRQn,
#ifdef MODULE_PERIPH_DMA
        .dma = 0,
        .dma_chan   = 4
#endif
    },
    {
        .dev        = USART6,
        .rcc_mask   = RCC_APB2ENR_USART6EN,
        .rx_pin     = GPIO_PIN(PORT_G, 9),
        .tx_pin     = GPIO_PIN(PORT_G, 14),
        .rx_af      = GPIO_AF8,
        .tx_af      = GPIO_AF8,
        .bus        = APB2,
        .irqn       = USART6_IRQn,
#ifdef MODULE_PERIPH_DMA
        .dma = 1,
        .dma_chan   = 4
#endif
    },
    {
        .dev        = USART2,
        .rcc_mask   = RCC_APB1ENR_USART2EN,
        .rx_pin     = GPIO_PIN(PORT_D, 6),
        .tx_pin     = GPIO_PIN(PORT_D, 5),
        .rx_af      = GPIO_AF7,
        .tx_af      = GPIO_AF7,
        .bus        = APB1,
        .irqn       = USART2_IRQn,
#ifdef MODULE_PERIPH_DMA
        .dma = 2,
        .dma_chan   = 4
#endif
    }
};

static const sdmmc_conf_t sdmmc_config[] = {
    {
        .dev = SDMMC1,
        .bus = APB2,
        .rcc_mask = RCC_APB2ENR_SDMMC1EN,
        .cd = GPIO_UNDEF,           /* CD is connected to MFX GPIO8 */
        .clk = { GPIO_PIN(PORT_C, 12), GPIO_AF12 },
        .cmd = { GPIO_PIN(PORT_D,  2), GPIO_AF12 },
        .dat0 = { GPIO_PIN(PORT_C,  8), GPIO_AF12 },
        .dat1 = { GPIO_PIN(PORT_C,  9), GPIO_AF12 },
        .dat2 = { GPIO_PIN(PORT_C, 10), GPIO_AF12 },
        .dat3 = { GPIO_PIN(PORT_C, 11), GPIO_AF12 },
#if MODULE_PERIPH_DMA
        .dma = 5,
        .dma_chan = 4,
#endif
        .irqn = SDMMC1_IRQn
    },
};
#define SDMMC_CONFIG_NUMOF  1


#define UART_0_ISR          (isr_usart3)
#define UART_0_DMA_ISR      (isr_dma1_stream6)
#define UART_1_ISR          (isr_usart6)
#define UART_1_DMA_ISR      (isr_dma1_stream5)
#define UART_2_ISR          (isr_usart2)
#define UART_2_DMA_ISR      (isr_dma1_stream4)

#define UART_NUMOF          ARRAY_SIZE(uart_config)
/** @} */


/* BoRa: Add support for the ADCs in f772ze 
 * We only need two ADC channel for sample I/Q
*/
static const adc_conf_t adc_config[] = {
    {GPIO_PIN(PORT_A, 3), 0, 3},  // PA3 chip, A0 in board, ADC123_IN3
    {GPIO_PIN(PORT_C, 0), 1, 10},  // PC0 chip, A1 in board, ADC123_IN10
};
#define ADC_NUMOF           ARRAY_SIZE(adc_config)

/* BoRa: Add support for the advanced-control timers (TIM1)
 * ps. the system uses TIM2 for clock
*/
static const timer_conf_t adv_timer_config = {
        .dev = TIM1,
        .max = 0x0000ffff,
        .rcc_mask = RCC_APB2ENR_TIM1EN,
        .bus = APB2,
        .irqn = TIM1_UP_TIM10_IRQn
};

/* BoRa: Add support for DMA to transfer ADC data to RAM 
 * the ADC1 connnect to DMA2 channel0 accroding to datasheet Page 221 Table 27
 * The UART in RIOT seems not support DMA well, we disable DMA for UART in uart.c
 */
static const dma_conf_t dma_config[6] = {
    {.stream = 6}, 
    {.stream = 5}, 
    {.stream = 4}, 
    {.stream = 8},
    {.stream = 12}, 
    {.stream = 11}
};

#define DMA_0_ISR   isr_dma1_stream6
#define DMA_1_ISR   isr_dma1_stream5
#define DMA_2_ISR   isr_dma1_stream4
#define DMA_3_ISR   isr_dma2_stream0
#define DMA_4_ISR   isr_dma2_stream4
#define DMA_5_ISR   isr_dma2_stream3

static const int adc_dma_chan1 = 0;
static const int adc_dma_chan2 = 0;
static const dma_t adc_dma1 = 3;
#define DMA_NUMOF   6

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */

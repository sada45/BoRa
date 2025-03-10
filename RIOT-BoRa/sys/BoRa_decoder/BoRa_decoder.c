/*
 * Copyright (C) 2024 Zhejiang University
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_BoRa_decoder
 * @{
 *
 * @file
 * @brief       BoRa_decoder implementation
 *
 * @author      Yeming Li <liymemnets@zju.edu.cn>
 *
 * @}
 */
#include <string.h>
#include <stdint.h>

#include "periph/adc.h"
#include "periph/cpu_dma.h"
#include "periph_conf.h"
#include "mutex.h"
#include "thread.h"
#include "ztimer.h"
#include "sema.h"
#include "board.h"
#include "BoRa_decoder.h"
#include "sdmmc/sdmmc.h"

/*
For BLE receiving CSS signals, we only implemenet the bandwidth of 125, 250, 500, and 1000 KHz
These bandwidth is actually used in sub-1GHz LoRa, the 2.4G LoRa uses 203.125, 406.25, and 812.5 KHz etc.
But this will cause the symbol duration is not an integer.
On USRP, this problem can be solved by setting the ADC sampling rate to N/BW, but the STM32 can not set arbitrary ADC sampling rate.
*/

/*
LoRa symbol length less than 512 us:
250KHz: SF 7
500KHz: SF 8
1M:     SF 9
*/

// The resolution of the ADC collecting the I/Q signal
#define ADC_RES ADC_RES_12BIT
#define ADC_SAMPLE_RATE 1 /* Unite Msps */
#define MAX_ADC_SPEED MHZ(36)
#define MAX_ADC_SMP (7u)
/* 000: 3 cycles, 001 15, 010 28, 011 56, 100 84, 101 112, 110 144, 111 480*/
#define ADC_SMP (0u)
#define SD_BUF_LEN  8

/* Add one extra to deal with the memory bound */
volatile uint32_t iq_sample_buffer[ADC_DMA_BUFFER_LEN] = {0};
volatile uint32_t *iq_sample_buffer1 = iq_sample_buffer;
volatile uint32_t *iq_sample_buffer2 = &(iq_sample_buffer[ADC_DMA_TRANSFRE_NUM]);
volatile uint32_t sd_write_buffer[ADC_DMA_TRANSFRE_NUM * SD_BUF_LEN] = { 0 };
uint32_t sd_write_idx = 0;
uint32_t adc_write_idx = 0;
sema_t buf_sema1 = SEMA_CREATE(0);
sema_t buf_sema2 = SEMA_CREATE(0);
sema_t sd_sema = SEMA_CREATE(0);
volatile uint8_t recv_flag = 0;
uint8_t cur_buffer = 0;
uint32_t write_addr = 512U;
uint32_t end_part_buffer[SDMMC_SDHC_BLOCK_SIZE / 4];
volatile uint32_t sem1_time_stamp, sem2_time_stamp;
uint32_t time_stamps[100] = { 0 };
uint32_t sd_stamps[100] = { 0 };
uint32_t write_buf_cnt = 0;
uint32_t sd_cnt = 0;

// For debug 
// uint32_t wait_time_i = 0;
// uint32_t of_time_i = 0;
int sem1_cnt = 0;
int sem2_cnt = 0;

/* The thread of the main receiving work */
kernel_pid_t main_work_pid;
thread_t *main_work_thread;
kernel_pid_t sd_write_pid;
thread_t *sd_write_thread;
char main_work_stack[1024];
char sd_write_stack[1024];

sdmmc_dev_t *sd_dev = NULL;

static mutex_t locks[] = {
#if ADC_DEVS > 1
    MUTEX_INIT,
#endif
#if ADC_DEVS > 2
    MUTEX_INIT,
#endif
    MUTEX_INIT};

static inline ADC_TypeDef *adc_dev(adc_t line)
{
    return (ADC_TypeDef *)(ADC1_BASE + (adc_config[line].dev << 8));
}

static inline void prep(adc_t line)
{
    mutex_lock(&locks[adc_config[line].dev]);
    periph_clk_en(APB2, (RCC_APB2ENR_ADC1EN << adc_config[line].dev));
}

int iq_adc_init(adc_t line)
{
#if LORA_BW == 812 || LORA_BW == 1000
    uint32_t clk_div = 2;
#else
    uint32_t clk_div = 4;
#endif

    /* configure the pin */
    if (adc_config[line].pin != GPIO_UNDEF)
    {
        gpio_init_analog(adc_config[line].pin);
    }
    /* set sequence length to 1 conversion and enable the ADC device */
    adc_dev(line)->SQR1 = 0;
    adc_dev(line)->SQR3 = adc_config[line].chan;
    adc_dev(line)->CR2 = ADC_CR2_ADON;

    ADC->CCR = (((clk_div / 2) - 1) << 16); // Clock divider
    ADC->CCR |= 6;                          // Multi ADC Regular simultaneous mode
    ADC->CCR |= (2 << 14);                  // DMA mode 2
    ADC->CCR |= (1 << 13);                  // DDS DMA disable selection
    adc_dev(line)->CR1 = ADC_RES;
    if (adc_config[line].chan >= 10)
    {
        adc_dev(line)->SMPR1 &= ~(MAX_ADC_SMP << (3 * (adc_config[line].chan - 10)));
        adc_dev(line)->SMPR1 |= ADC_SMP << (3 * (adc_config[line].chan - 10));
    }
    else
    {
        adc_dev(line)->SMPR2 &= ~(MAX_ADC_SMP << (3 * adc_config[line].chan));
        adc_dev(line)->SMPR2 |= ADC_SMP << (3 * adc_config[line].chan);
    }
    if (line == 0)
    {
        /* Set the master ADC to capture TIM1 trigger */
        adc_dev(line)->CR2 |= (10 << ADC_CR2_EXTSEL_Pos); // Set EXTSEL Timer1 TRGO2
        adc_dev(line)->CR2 |= 1 << 28;                    // Set EXTEN to trigger detection on rising edge
    }

    return 0;
}

int bora_adc_timer_init(void)
{
    TIM_TypeDef *timer_dev = adv_timer_config.dev;
    // periph_clk_en(adv_timer_config.bus, adv_timer_config.rcc_mask);  // Set the TIM1EN bit
    timer_dev->CR1 = 0;
    timer_dev->CR2 = 2 << 20; // MMS2 set to Update
#if LORA_BW == 250 || LORA_BW == 500
    timer_dev->ARR = 107;  // 1Msps
#elif LORA_BW == 501
    timer_dev->ARR = 71;  // 1.5Msps
// #elif LORA_BW == 1000
//     timer_dev->ARR = 47;  // 2.25Msps
#elif LORA_BW == 203 || LORA_BW == 406
    timer_dev->ARR = 63;  // 1.625Msps
// #elif LORA_BW == 812
//     timer_dev->ARR = 47;  // 2.03125Msps
#else 
#error "Invalid LoRa bandwidth"
#endif 
    timer_dev->RCR = 0;
    timer_dev->PSC = 0;          // Let it run at the max frequency (108MHz, 104MHz, 97.5MHz)
    timer_dev->EGR = TIM_EGR_UG; // UG = 1
    printf("timer init, ARR=%ld, BW=%d\n", timer_dev->ARR, LORA_BW);
    return 0;
}

int bora_sdmmc_init(void)
{
    /*
     * It is worth to notice that, currently, the RIOT OS not supports the SDMMC well,
     * we find some TF cards (i.e., SanDisk Ultra 32G and 64G) can not be correctly initialized
     * We use KIOXIA EXCERIA G2 32GB (V30, U3). Please power-off and on the SD card if the program restart.
    */
    int i, res;

    sd_dev = sdmmc_get_dev(0);
    assert(sd_dev!=NULL);
    assert(sd_dev->present);
    for (i = 0; i < 10; i++) 
    {
        /* Some times we need to retry init the card */
        res = sdmmc_card_init(sd_dev);
        if (res == 0)
        {
            break;
        }
        else
        {
            ztimer_sleep(ZTIMER_USEC, 10000);
        }
    }
    printf("SD card detected, with %d times retry\n", i);
    assert(res==0);
    /* The SD card writing start at 512 */
    write_addr = 512U;
    /* TODO: we can check the SD capacity with sdmmc_get_capacity(dev), 
     * and make sure not exceed the card capacity 
    */
    return 0;

}

inline int bora_sdmmc_write(uint32_t *data, int len) 
{
    int blkcnt;
    uint16_t done;
    int res;
    // uint32_t st;

    blkcnt = len / SDMMC_SDHC_BLOCK_SIZE;
    // st = ztimer_now(ZTIMER_USEC);
    res = sdmmc_write_blocks(sd_dev, write_addr, SDMMC_SDHC_BLOCK_SIZE, blkcnt, (void *)data, &done);
    // if (sd_cnt < 100) {
    //     sd_stamps[sd_cnt++] = ztimer_now(ZTIMER_USEC) - st;
    // }
    if (res != 0) printf("Write error, %d\n", res);
    assert(res==0);
    write_addr += blkcnt;

    return len;
}

int bora_recv_read(void)
{
    uint32_t data = (uint32_t)ADC->CDR;
    int16_t adc2_data = (int16_t)((data >> 16) & 0x0000ffff);
    int16_t adc1_data = (int16_t)(data & 0x0000ffff);
    printf("data = %u, %u, ||| %lu, %lu, adc1_sr=%lu, adc2_sr=%lu, csr=%lu\n", adc1_data, adc2_data, adc_dev(0)->DR, adc_dev(1)->DR, adc_dev(0)->SR, adc_dev(1)->SR, ADC->CSR);
    return 0;
}

static void print_wait(void)
{
    // for (uint32_t i = 0; i < (sem1_cnt > sem2_cnt? sem1_cnt: sem2_cnt); i++)
    // {
    //     printf("%ld, -> %ld, %ld\n", i, s1_time[i], s2_time[i]);
    // }
    // for (uint32_t i = 0; i < wait_time_i; i++) 
    // {
    //     printf("%ld, %lu\n", i, wait_time[i]);
    // }
    // for (uint32_t i = 0; i < of_time_i; i++) 
    // {
    //     printf("%ld, %lu\n", i, of_time[i]);
    // }
    // for (uint32_t i = 1; i < write_buf_cnt; i++) {
    //     printf("sema_time = %ld ,%ld\n", i, time_stamps[i]-time_stamps[i-1]);
    // }
    for (uint32_t i = 0; i < sd_cnt; i++) {
        printf("sd_time = %ld ,%ld\n", i, sd_stamps[i]);
    }
    // printf("Wait_time_i = %ld, sem1_cnt = %d, sem2_cnt = %d\n", wait_time_i, sem1_cnt, sem2_cnt);
    return;
}

static void* bora_sd_write_work(void *arg)
{
    (void) arg;

    while (recv_flag) 
    {
        sema_wait(&sd_sema);
        if (!recv_flag) {
            return NULL;
        }
        bora_sdmmc_write(sd_write_buffer + (sd_write_idx % SD_BUF_LEN) * ADC_DMA_TRANSFRE_NUM, ADC_DMA_TRANSFRE_NUM * 4);
        sd_write_idx++;
    }
    return NULL;
}

static void* bora_recv_mainwork(void *arg)
{
    (void) arg;
    volatile uint32_t *ptr;

    while (buf_sema1.value > 0) 
    {
        sema_try_wait(&buf_sema1);
    }
    while (buf_sema2.value > 0) 
    {
        sema_try_wait(&buf_sema2);
    }
    // for (int i = 0; i < 1000; i++) {
    //     st = ztimer_now(ZTIMER_USEC);
    //     // memcpy(sd_buffer, (uint32_t *)iq_sample_buffer1, ADC_DMA_TRANSFRE_NUM * 4);
    //     bora_sdmmc_write(sd_buffer, ADC_DMA_TRANSFRE_NUM * 4);
    //     wait_time[wait_time_i++] = ztimer_now(ZTIMER_USEC) - st;
    // }
    while (recv_flag) 
    {
        if (cur_buffer == 0) 
        {
            if (recv_flag && buf_sema1.value > 1)
            {
                printf("buf 1 over\n");
                cur_buffer = 0;
                return NULL;
            }
            
            // assert(recv_flag & (buf_sema1.value <= 1));
            sema_wait(&buf_sema1);
            ptr = iq_sample_buffer1;
        }
        else
        {
            if (recv_flag && buf_sema2.value > 1)
            {
                printf("buf 2 over\n");
                cur_buffer = 0;
                return NULL;
            }
            // assert(recv_flag && (buf_sema2.value <= 1));
            sema_wait(&buf_sema2);
            ptr = iq_sample_buffer2;
        }
        if (!recv_flag) 
        {
            break;
        }
        if (write_buf_cnt < 100) {
            time_stamps[write_buf_cnt++] = cur_buffer? sem2_time_stamp: sem1_time_stamp;
        }
        if (adc_write_idx - sd_write_idx >= SD_BUF_LEN) 
        {
            // overflowed
            printf("SD write overflowed, adc_idx = %ld, sd_idx = %ld\n", adc_write_idx, sd_write_idx);
            LED2_ON;
            break;
        }
        memcpy(sd_write_buffer + (adc_write_idx % SD_BUF_LEN) * ADC_DMA_TRANSFRE_NUM, (uint32_t *)ptr, ADC_DMA_TRANSFRE_NUM * 4);
        adc_write_idx++;
        // if (adc_write_idx % 20 == 0) {
        //     printf("cur adc_idx = %ld, sd_idx = %ld\n", adc_write_idx, sd_write_idx);
        // }
        cur_buffer = (cur_buffer + 1) % 2;
        sema_post(&sd_sema);
        // wait_time[wait_time_i++] = ztimer_now(ZTIMER_USEC) - st; 
        // printf("SD time = %lu\n", ztimer_now(ZTIMER_USEC) - st);
    }
    printf("main workd done\n");
    return NULL;
}

int bora_recv_start(void)
{
    recv_flag = 1;
    sd_write_pid = thread_create(sd_write_stack, 1024, 2, THREAD_CREATE_STACKTEST, bora_sd_write_work, NULL, "SD write thread");
    sd_write_thread = thread_get(sd_write_pid);
    main_work_pid = thread_create(main_work_stack, 1024, 2, THREAD_CREATE_STACKTEST, bora_recv_mainwork, NULL, "Main receiver thread");
    main_work_thread = thread_get(main_work_pid);

    ztimer_sleep(ZTIMER_USEC, 100);
    TIM_TypeDef *timer_dev = adv_timer_config.dev;
    timer_dev->CR1 |= TIM_CR1_CEN;
    dma_start(adc_dma1);

    return 0;
}

int bora_recv_stop(void)
{
    TIM_TypeDef *timer_dev = adv_timer_config.dev;
    recv_flag = 0;
    printf("STOP +++++++++++++++, %ld, %ld\n", timer_dev->CR1, adc_dev(0)->SR);

    dma_stop(adc_dma1);
    timer_dev->CR1 &= ~(TIM_CR1_CEN);
    /* POST the semaphore to end the main thread */
    sema_post(&buf_sema1);
    sema_post(&buf_sema2);
    sema_post(&sd_sema);
    puts("Waiting the SD card writing...");
    while (thread_is_active(main_work_thread) || thread_is_active(sd_write_thread)) 
    {
        puts("wait");
        ztimer_sleep(ZTIMER_USEC, 1000000);
    }
    /* Write the end part buffer */
    bora_sdmmc_write(end_part_buffer, SDMMC_SDHC_BLOCK_SIZE);
    bora_sdmmc_write(end_part_buffer, SDMMC_SDHC_BLOCK_SIZE);
    bora_sdmmc_write(end_part_buffer, SDMMC_SDHC_BLOCK_SIZE);
    bora_sdmmc_write(end_part_buffer, SDMMC_SDHC_BLOCK_SIZE);
    puts("Done, you can remove the SD card");
    printf("sem1_cnt = %d, sem2_cnt = %d\n", sem1_cnt, sem2_cnt);
    print_wait();
    // for (int i =0; i < 400; i++) {
    //     printf("%d, %d, %d\n", i, (sd_write_buffer[i] >> 16) & 0xffff, sd_write_buffer[i] & 0xffff);
    // }
    LED1_ON;
    return 0;
}

int bora_decoder_init(void)
{
    periph_clk_en(APB2, (RCC_APB2ENR_ADC1EN << 0));
    periph_clk_en(APB2, (RCC_APB2ENR_ADC1EN << 1));
    periph_clk_en(adv_timer_config.bus, adv_timer_config.rcc_mask);
    // Init the Timer 
    bora_adc_timer_init();
    // Init the ADC 
    iq_adc_init(0);
    iq_adc_init(1);
    printf("ADC ccr %lu, init CSR = %lu\n", ADC->CCR, ADC->CSR);
    /* We start two DMA transfer that copy CDR to RAM */
    dma_configure(adc_dma1, adc_dma_chan1, &(ADC->CDR), iq_sample_buffer1, ADC_DMA_TRANSFRE_NUM, DMA_PERIPH_TO_MEM, (DMA_INC_DST_ADDR | DMA_DATA_WIDTH_WORD));
    // Init SDMMC
    memset(end_part_buffer, 0, SDMMC_SDHC_BLOCK_SIZE);
    /* Since the ADC sample is not longer than 12-bit and store in a 16-bit data
     * the end_val will never occured in the normal ADC sample
     */
    uint32_t end_val = (15 << 12) | (15 << 28);
    for (int i = 0; i < SDMMC_SDHC_BLOCK_SIZE / 4; i++) 
    {
        end_part_buffer[i] = end_val;
    }
    bora_sdmmc_init();
    write_addr = 512U;
    // uint32_t st;
    // for (int i = 0; i < 1000; i++) {
    //     st = ztimer_now(ZTIMER_USEC);
    //     // memcpy(sd_buffer, (uint32_t *)iq_sample_buffer1, ADC_DMA_TRANSFRE_NUM * 4);
    //     bora_sdmmc_write(sd_buffer, ADC_DMA_TRANSFRE_NUM * 4);
    //     wait_time[wait_time_i++] = ztimer_now(ZTIMER_USEC) - st;
    // }
    // print_wait();
    return 0;
}

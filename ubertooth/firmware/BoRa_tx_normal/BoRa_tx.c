#include <stdlib.h>
#include <string.h>

#include "ubertooth.h"
#include "ubertooth_dma.h"
#include "ubertooth_clock.h"
#include "debug_uart.h"
#include "xmas.h"
#include "BoRa_config.h"
#define TIMING_LOG 0

/* build info */
const char compile_info[] =
	"ubertooth " GIT_REVISION " (" COMPILE_BY "@" COMPILE_HOST ") " TIMESTAMP;

uint16_t channel = 2390;

volatile int t_int = 0;

#if TIMING_LOG
uint32_t times[512] = { 0 };
uint32_t spi_times[512] = { 0};
uint32_t end_times[512] = { 0 };
uint32_t *ttt = times;
int times_counter = 0;
#endif

/* The delay of the operation */
uint32_t interrupt_delay = 1;

/* The configuration of chirps */
uint16_t base_val = 0;
uint16_t write_val = 0;
uint16_t *write_seq = 0;
extern uint16_t msbs[16];
uint8_t start_freq[128] = { 0 };
uint32_t chirp_num = PREAMBLE_LEN + 2 + 3 + LORA_SYMBOLS;
uint32_t cur_chirp = 0;
uint32_t cur_freq_bin = 0;
uint32_t freq_chg_int = FREQ_CHG_INT;
uint32_t freq_chg_dec = FREQ_CHG_DEC;
uint32_t max_freq_chg_num = MAX_FREQ_CHG_NUM;

// Used for timing control
volatile uint32_t next_int_counter = 0;
volatile uint32_t reminder = 0;

static void cc2400_idle() {
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK)); // need to wait for unlock?

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
	HGM_CLR;
#endif

	RXLED_CLR;
	TXLED_CLR;
	USRLED_CLR;

	clkn_stop();
	dio_ssp_stop();
}

void DMA_IRQHandler(void) {
	dma_tx_cl_t *temp;

	// DMA channel 7: debug UART
	if (DMACIntStat & (1 << 7)) {
		// TC -- DMA completed, unset flag so another printf can occur
		if (DMACIntTCStat & (1 << 7)) {
			DMACIntTCClear = (1 << 7);
			if (dma_uart_start->next) {
				temp = dma_uart_start->next;
				debug_send_dma(temp->data, temp->len);
				free(dma_uart_start->data);
				free((void*)dma_uart_start);
				dma_uart_start = temp;
			}
			else {
				free(dma_uart_start->data);
				free((void*)dma_uart_start);
				dma_uart_start = 0;
				dma_uart_end = 0;
			}
		}
		// error -- blow up
		if (DMACIntErrStat & (1 << 7)) {
			DMACIntErrClr = (1 << 7);
			// FIXME do something better here
			while (1) { }
		}
	}
	/* BoRa: deal with the DMA interrupt for carrier Tx */
	if (DMACIntStat & (1 << 0)) {
		/* Transfered one byte */
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear = (1 << 0);	
		}
		if (DMACIntErrStat & (1 << 0)) {
			DMACIntErrClr = (1 << 0);
		}
	}
}

/* We do not deal with the timer overflow, since the overflow takes 42.95s, 
 * and one LoRa packet can not reach such a long period.
*/
void TIMER1_IRQHandler() {
#if TIMING_LOG
	*ttt = T1TC;
#endif
	if (T1IR & TIR_MR0_Interrupt) {
		next_int_counter += freq_chg_int;
		reminder += freq_chg_dec;
		if (reminder >= CHIRP_TIME_ACC) {
			reminder -= CHIRP_TIME_ACC;
			next_int_counter++;
		}
		T1MR0 = next_int_counter;
		T1IR = TIR_MR0_Interrupt;
		
		/* It would be more clear if we use a array or use bit shift
		   such as the function cc2400_spi_continue_write()
		   but this way is much faster and can save stack memory! 
		   The time consumption is about 1.84us, while the array-based takes 3.2us 
		 */
		// MDMCTRL 15:13 reserved as 0
		MOSI_CLR;
		SCLK_SET;  // 15
		SCLK_CLR;
		SCLK_SET;  // 14
		SCLK_CLR;
		SCLK_SET;  // 13
		SCLK_CLR;
		// MDMCTRL 12:7 MOD_OFFSET: change various time
		if (write_val & 4096) MOSI_SET;  // 12
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 2048) MOSI_SET;  // 11
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 1024) MOSI_SET;  // 10
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 512) MOSI_SET;  // 9
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 256) MOSI_SET;  // 8
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 128) MOSI_SET;  // 7
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		// MDMCTRL 6:0 MOD_DEV, always 0x40 for BLE
		MOSI_SET;  // 6
		SCLK_SET;
		SCLK_CLR;
		MOSI_CLR;  // 5
		SCLK_SET;
		SCLK_CLR;
		SCLK_SET;  // 4
		SCLK_CLR;
		SCLK_SET;  // 3
		SCLK_CLR;
		SCLK_SET;  // 2
		SCLK_CLR;
		SCLK_SET;  // 1
		SCLK_CLR;
		SCLK_SET;  // 0
		SCLK_CLR;
		cur_freq_bin++;
		if (cur_freq_bin == max_freq_chg_num) {
			/* Start the next chirp */
			cur_chirp++;
			cur_freq_bin = 0;
			if (cur_chirp == QUA_START_LEN) {
				freq_chg_int = QUA_FREQ_CHG_INT;
				freq_chg_dec = QUA_FREQ_CHG_DEC;
				max_freq_chg_num = QUA_MAX_FREQ_CHG_NUM;
			}
			else if (cur_chirp == DATA_START_LEN) {
				// Recover the frequency change interval
				freq_chg_int = FREQ_CHG_INT;
				freq_chg_dec = FREQ_CHG_DEC;
				max_freq_chg_num = MAX_FREQ_CHG_NUM;
			}
			else if (cur_chirp == chirp_num) {
				/* Transmission done, disable timer interrupt,
				   close the RF in the main program */
				T1TCR = TCR_Counter_Reset;
				ICER0 = ICER0_ICE_TIMER1;
				t_int = 1;
				return;
			}
			write_seq = mdmctrl_seq[start_freq[cur_chirp]];
		}

		write_val = write_seq[cur_freq_bin];
#if TIMING_LOG
		end_times[times_counter] = next_int_counter;
		spi_times[times_counter++] = T1TC;
		ttt++;
#endif
	}
}

/* BoRa: Start a LoRa Tx */
void bora_tx(u8 len) {
	int i, c;
	int bit;
	u8 tx_len;
	u8 byte;
	int16_t end_freq = 32;
	
	base_val = 0x0040;  /* the basic value of the MDMCTRL */
	

	bora_tx_dma_init();
	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	cc2400_set(MDMTST0, 0x134b);    // no PRNG

	cc2400_set(GRMDM,   0x04e1);
	// 0 00 00 1 001 11 0 00 0 1
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: all 32 bits of SYNC_WORD
	//      |  | +---------------> 1 preamble byte of 01010101
	//      |  +-----------------> packet mode 
	//      +--------------------> un-buffered mode

	cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1111);    // amplifier level (0 dBm, picked from hat)
	cc2400_set(MDMCTRL, base_val);

	// turn on the radio
	while (!(cc2400_status() & XOSC16M_STABLE));
	TX_SET;
	RX_SET;
	while (!(cc2400_status() & FS_LOCK));
	TXLED_SET;
	PAEN_SET;
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_PIN_FS_ON);
	// debug_printf("target=%d; ct=%d\n", target_gfsk_bytes_ct, gfsk_bytes_ct);
	TX_SET;
	RX_CLR;
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_PIN_TX);
	dio_ssp_write_start();
	usleep(80);
	cc2400_spi_continue_write_start(MDMCTRL, base_val);
	cur_freq_bin = 0;
	cur_chirp = 0;
	freq_chg_int = FREQ_CHG_INT;
    freq_chg_dec = FREQ_CHG_DEC;
	max_freq_chg_num = MAX_FREQ_CHG_NUM;
#if TIMING_LOG
	times_counter = 0;
	ttt = times;
#endif
	write_val = mdmctrl_seq[0][0];
	write_seq = mdmctrl_seq[0];
	T1TCR = TCR_Counter_Reset;
	ISER0 = ISER0_ISE_TIMER1;
	T1MR0 = 200;
	next_int_counter = 200;
	reminder = 0;
	t_int = 0;
	T1TCR = TCR_Counter_Enable;
	while(!t_int) {}
	usleep((uint32_t)(FREQ_CHG_INT / 100));
	
	dio_ssp_write_stop();
	cc2400_spi_continue_write_stop();
	TX_CLR;
	RX_CLR;
	PAEN_CLR;
	TXLED_CLR;
#if TIMING_LOG
	for (i = 0; i < 128; i++) {
		debug_printf("i=%d, %u, %u, %u\n", i, times[i], spi_times[i]-times[i], times[i]-end_times[i-1]);
	}
#endif
}

/* BoRa: periodically transmit LoRa chips */
void bora_periodic_tx() {
	int i;
	uint8_t adv_ind_len;
	uint8_t led_state = 0;
	uint32_t raa;

	// enable USB interrupts due to busy waits
	ISER0 = ISER0_ISE_USB;
	uint32_t last_sch = T0TC;
	while (1) {
		// msleep(1000);
		last_sch =  msleep_acc(100, last_sch);
		// msleep_acc(500, last_sch);
		// last_sch = T0TC;
		bora_tx(0);
		if (led_state) {
			USRLED_CLR;
			led_state = 0;
		}
		else {
			USRLED_SET;
			led_state = 1;
		}
		// break;
	}

	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;
}

int main() {
	uint8_t a = 0xaa;
	int i;
	// enable all fault handlers (see fault.c)
	SCB_SHCSR = (1 << 18) | (1 << 17) | (1 << 16);

	ubertooth_init();
	clkn_init();
	// ubertooth_usb_init(vendor_request_handler);
	cc2400_idle();
	dma_poweron();
	dio_ssp_init();

	debug_uart_init(0);
	debug_printf("\n\n****UBERTOOTH BOOT****\n%s\n", compile_info);
	for (i = 0; i < PREAMBLE_LEN; i++) {
		start_freq[i] = 0;
	}
	start_freq[PREAMBLE_LEN] = 1;
	start_freq[PREAMBLE_LEN+1] = 1;
	start_freq[PREAMBLE_LEN+2] = BLE_BBIN_NUM;
	start_freq[PREAMBLE_LEN+3] = BLE_BBIN_NUM;
	start_freq[PREAMBLE_LEN+4] = BLE_BBIN_NUM + 1;  // Reserved for the last one-quater SFD

	for (i = 0; i < LORA_SYMBOLS; ++i)
	{
		int value = rand() % MAX_BIN;
		start_freq[PREAMBLE_LEN + 5 + i] = i % MAX_BIN;
	}

	wait(1);
	clkn_start();
	bora_periodic_tx();
	while(1);
	return 0;
}

#include <stdlib.h>
#include <string.h>

#include "ubertooth.h"
#include "ubertooth_dma.h"
#include "ubertooth_clock.h"
#include "debug_uart.h"
#include "xmas.h"

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
#endif
uint32_t times[512] = { 0 };
uint32_t end_times[512] = { 0 };
int times_counter = 0;
uint8_t adc_i[128] = { 0 };
uint8_t adc_q[128] = { 0 };

/* The delay of the operation */
uint32_t interrupt_delay = 1;

/* The configuration of chirps */
uint16_t base_val = 0;
uint16_t write_val = 0;
uint16_t *write_seq = 0;
extern uint16_t msbs[16];
uint32_t chirp_num = 6 + 2 + 15;

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
}

void bora_start_rx_analog() {
	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	cc2400_set(MDMTST0, 0x134b);    // no PRNG
	cc2400_set(GRMDM,   0b0001010011100001);  // 04e1
	cc2400_set(FSDIV,   channel);
	cc2400_set(FREND,   0b1111);    // amplifier level (0 dBm, picked from hat)
	cc2400_set(MDMCTRL, base_val);
	// 2, 4
	cc2400_set(PAMTST, 0x0003 | (4 << 8));  

	while (!(cc2400_status() & XOSC16M_STABLE));
	TX_SET;
	RX_SET;
	while (!(cc2400_status() & FS_LOCK));
	debug_printf("fs lock\n");
	TXLED_SET;
	PAEN_SET;
	HGM_SET;
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_PIN_FS_ON);
	TX_CLR;
	RX_SET;
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_PIN_RX);
}

int bora_stop_rx_analog() {
	return 0;
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
	// dio_ssp_init();

	debug_uart_init(0);
	debug_printf("\n\n****UBERTOOTH BOOT****\n%s\n", compile_info);
	wait(1);
	clkn_start();
	// FIO1SET = PINSEL1_P0_26;
	

	// bora_periodic_rx();
	bora_start_rx_analog();
	i = 0;
	while(1) {
		if (i == 1) {
			// FIO0SET = PIN_ATEST1;
			USRLED_SET;
			i = 0;
		}
		else {
			// FIO0CLR = PIN_ATEST1;
			USRLED_CLR;
			i = 1;
		}
		msleep(1000);
	}
	return 0;
}




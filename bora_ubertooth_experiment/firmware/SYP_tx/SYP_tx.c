#include <stdlib.h>
#include <string.h>

#include "ubertooth.h"
#include "ubertooth_dma.h"
#include "ubertooth_clock.h"
#include "debug_uart.h"
#include "xmas.h"
#include "BLE_config.h"

#define TIMING_LOG 0

// The native BLE packet length = 1 + 4 + 2 + 251 + 3
#define NATIVE_BLE_PACKET_LEN 32
volatile int bit_counter = 0;

/* build info */
const char compile_info[] =
	"ubertooth " GIT_REVISION " (" COMPILE_BY "@" COMPILE_HOST ") " TIMESTAMP;

uint16_t channel = 2390;

volatile int t_int = 0;

/* The delay of the operation */
uint32_t interrupt_delay = 1;

/* The configuration of chirps */
uint16_t base_val = 0;
extern uint16_t msbs[16];

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
	/* BoRa: deal with the DMA interrupt for native BLE data transmission */
	if (DMACIntStat & (1 << 0)) {
		/* Transfered one byte */
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear = (1 << 0);	
			bit_counter++;
			/* BLE packet transmit complete */
			if (bit_counter == NATIVE_BLE_PACKET_LEN * 8) {
				dio_ssp_write_stop();
				t_int = 1;
			}
		}
		if (DMACIntErrStat & (1 << 0)) {
			DMACIntErrClr = (1 << 0);
		}
	}
}

void ble_tx(u8 len) {
	int i, c;
	int bit;
	u8 tx_len;
	u8 byte;
	
	base_val = 0x0040;  /* the basic value of the MDMCTRL */
	bit_counter = 0;
	syp_tx_dma_init();
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
	cc2400_set(PAMTST, 0x0403);  

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
	t_int = 0;
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_PIN_TX);
	dio_ssp_write_start();
	usleep(80);
	while(!t_int) {}
	TX_CLR;
	RX_CLR;
	PAEN_CLR;
	TXLED_CLR;
}

/* BoRa: periodically transmit LoRa chips */
void ble_periodic_tx() {
	int i;
	uint8_t adv_ind_len;
	uint8_t led_state = 0;
	uint32_t raa;
	syp_dma_init();
	// enable USB interrupts due to busy waits
	ISER0 = ISER0_ISE_USB;
	uint32_t last_sch = T0TC;

	while (1) {
		last_sch =  msleep_acc(100, last_sch);
//msleep(500);
		ble_tx(0);

		if (led_state) {
			USRLED_CLR;
			led_state = 0;
		}
		else {
			USRLED_SET;
			led_state = 1;
		}	
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
	wait(1);
	clkn_start();
	ble_periodic_tx();
	while(1);
	return 0;
}

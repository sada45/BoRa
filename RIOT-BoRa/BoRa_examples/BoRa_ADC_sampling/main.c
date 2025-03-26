#include <stdio.h>
#include <stdint.h>

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"
#include "BoRa_decoder.h"
#include "ztimer/config.h"
#include "sdmmc/sdmmc.h"
// uint32_t mydata[8192];
// uint32_t t[2000];
uint32_t t_i = 0;


int main(void)
{
    int counter = 0;
    printf("start\n");
    bora_decoder_init();
    bora_recv_start();
    ztimer_sleep(ZTIMER_MSEC, 5 * 1000);
    puts("end");
    bora_recv_stop();
    return 0;
}

#include <stdlib.h>

#define LORA_BD (500)
#define LORA_SYMBOLS (32)
#define MAX_FREQ_CHG_NUM (4)
#define SKIP_NUM (1)
#define FREQ_CHG_INT (6400)
#define FREQ_CHG_DEC (0)
#define CHIRP_TIME_ACC (100000000)
#define FREQ_BIN_NUM (4)
#define BLE_BBIN_NUM (4)
#define BASE_VAL (16)
#define QUA_FREQ_CHG_INT (6400)
#define QUA_FREQ_CHG_DEC (0)
#define QUA_MAX_FREQ_CHG_NUM (1)
#define PREAMBLE_LEN (8)
#define QUA_START_LEN (12)
#define DATA_START_LEN (13)
uint16_t mdmctrl_seq[7][4] = {{6160, 7568, 656, 2064},
							{7568, 656, 2064, 6160},
							{656, 2064, 6160, 7568},
							{2064, 6160, 7568, 656},
							{2064, 656, 7568, 6160},
							{6160, 6160, 6160, 6160},
							{2064, 0, 0, 0}};


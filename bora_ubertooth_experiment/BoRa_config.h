#include <stdlib.h>

#define MAX_BIN (8)
#define LORA_BD (125)
#define MAX_FREQ_CHG_NUM (8)
#define SKIP_NUM (1)
#define FREQ_CHG_INT (6400)
#define FREQ_CHG_DEC (0)
#define CHIRP_TIME_ACC (100000000)
#define FREQ_BIN_NUM (8)
#define BLE_BBIN_NUM (8)
#define BASE_VAL (16)
#define QUA_FREQ_CHG_INT (6400)
#define QUA_FREQ_CHG_DEC (0)
#define QUA_MAX_FREQ_CHG_NUM (2)
#define PREAMBLE_LEN (8)
#define QUA_START_LEN (12)
#define DATA_START_LEN (13)
uint16_t mdmctrl_seq[10][8] = {{1552, 1680, 1808, 1936, 16, 144, 272, 400},
							{1680, 1808, 1936, 16, 144, 272, 400, 1552},
							{1808, 1936, 16, 144, 272, 400, 1552, 1680},
							{1936, 16, 144, 272, 400, 1552, 1680, 1808},
							{16, 144, 272, 400, 1552, 1680, 1808, 1936},
							{144, 272, 400, 1552, 1680, 1808, 1936, 16},
							{272, 400, 1552, 1680, 1808, 1936, 16, 144},
							{400, 1552, 1680, 1808, 1936, 16, 144, 272},
							{528, 400, 272, 144, 16, 1936, 1808, 1680},
							{528, 400, 0, 0, 0, 0, 0, 0}};


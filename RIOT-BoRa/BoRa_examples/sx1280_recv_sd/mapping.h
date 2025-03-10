#include <stdint.h>

/**
 * Mapping lora frequency bin(0~255) to ble frequency bin(0~31)
*/
void mapping_init(void);
void freq_mapping(uint16_t *lora_symbols, int num_symbols, uint16_t *ble_symbols, uint8_t sf);

void freq_demapping(uint16_t *ble_symbols, int num_symbols, uint16_t *lora_symbols);

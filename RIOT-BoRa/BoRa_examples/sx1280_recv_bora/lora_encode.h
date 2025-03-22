#include <stdint.h>

// /**
//  * To print array.
//  * data: the start address of symbols array.
//  * nums: The num of symbols.
// */
// void print_symbols(uint16_t *data, int nums);

/**
 * encode: bytes ---> frequency symbols bin.
 * int length: num of bytes.
 * uint8_t *bytes: data, the start address of bytes array.
 * uint16_t *symbols: To get final frequency symbols bin, the start address of symbols array.
*/
int encode(uint8_t *bytes, int length, uint16_t *symbols);
/**
 * Update encode parameters.
 * sf: Spread factor: 6~12 [default: 7]
 * cr: Coding Rate: 0~4 [default: 0]
 * crc: having crc? 1(true) or 0(false) [default: 1]
 * payload_len: Num of bytes [default: 8]
*/
void update_parameters(uint8_t sf, int cr, int crc, int payload_len);

#include <stdint.h>
#include <stdio.h>

#include <math.h>
#include "utility.h"

uint32_t rotl(uint32_t bits, uint32_t count, const uint32_t size) {
    const uint32_t len_mask = (1u << size) - 1u;

    count %= size;      // Limit bit rotate count to size
    bits  &= len_mask;  // Limit given bits to size

    return ((bits << count) & len_mask) | (bits >> (size - count));
}
uint32_t pmod(int32_t x, int32_t n)
{
    return ((x % n) + n) % n;
}

/**
 *  \brief  Python-style float modulo, (x % n) >= 0
 *
 *  \param  x
 *          dividend
 *  \param  n
 *          divisor
 */
float fpmod(float x, float n)
{
    return fmod(fmod(x, n) + n, n);
}


unsigned char parity(unsigned char c, unsigned char bitmask)
{
    unsigned char _parity = 0;
    unsigned char shiftme = c & bitmask;

    for (int i = 0; i < 8; i++)
    {
        if (shiftme & 0x1) _parity++;
        shiftme = shiftme >> 1;
    }

    return _parity % 2;
}

void print_nums(uint8_t *data, int nums)
{
    for (int i = 0; i < nums; ++i)
    {
        printf("%d ", *(data + i));
    }
    printf("\n");
}


void print_symbols(uint16_t *data, int nums)
{
    printf("<Symbols>:");
    for (int i = 0; i < nums; ++i)
    {
        if (i > 0)
            printf(" ");
        printf("%d", *(data + i));
    }
    printf(":\n");
}



uint16_t data_checksum(const uint8_t *data, int length) {
    uint16_t crc = 0;
    for (int j = 0; j < length - 2; j++)
    {
        uint8_t new_byte = data[j];
        for (int i = 0; i < 8; i++) {
            if (((crc & 0x8000) >> 8) ^ (new_byte & 0x80)) {
            crc = (crc << 1) ^ 0x1021;
            } else {
            crc = (crc << 1);
            }
            new_byte <<= 1;
        }
    }

    // XOR the obtained CRC with the last 2 data bytes
    uint16_t x1 = (length >= 1) ? data[length-1]      : 0;
    uint16_t x2 = (length >= 2) ? data[length-2] << 8 : 0;
    crc = crc ^ x1 ^ x2;
    return crc;
}

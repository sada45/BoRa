#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "utility.h"
#include "lora_encode.h"

#define HAMMING_P1_BITMASK 0x0D  // 0b00001101
#define HAMMING_P2_BITMASK 0x0B  // 0b00001011
#define HAMMING_P3_BITMASK 0x07  // 0b00000111
#define HAMMING_P4_BITMASK 0x0F  // 0b00001111
#define HAMMING_P5_BITMASK 0x0E  // 0b00001110
#define LOG_LEVEL 0
static int PAYLOAD_LENGTH = 8;
static uint8_t d_sf = 7;
static int d_crc = 0;
static int d_cr = 0;
static int d_ldr = 0;
static int d_header = 0;
static uint8_t codewords[512] = {0};
static uint8_t nibbles[512] = {0};
static uint16_t symbols[512] = {0};



uint16_t calc_sym_num(uint8_t payload_len)
{
    double tmp = 2 * payload_len - d_sf + 7 + 4 * d_crc - 5 * (1 - d_header);
    int num = (4 + d_cr) * (uint16_t)ceil(tmp / (d_sf - 2 * d_ldr));
    if (num > 0)
    {
        return 8 + num;
    }
    else
    {
        return 8;
    }
}


static void whiten(uint8_t *bytes, uint8_t len)
{
    for (int i = 0; i < len && i < whitening_sequence_length; i++)
    {
        *(bytes + i) = ((unsigned char)((*(bytes + i)) & 0xFF) ^ whitening_sequence[i]) & 0xFF;
    }
}


void hamming_encode(int nums)
{
    unsigned char p1, p2, p3, p4, p5;
    unsigned char mask;

    for (int i = 0; i < nums; i++)
    {
        p1 = parity((unsigned char)nibbles[i], mask = (unsigned char)HAMMING_P1_BITMASK);
        p2 = parity((unsigned char)nibbles[i], mask = (unsigned char)HAMMING_P2_BITMASK);
        p3 = parity((unsigned char)nibbles[i], mask = (unsigned char)HAMMING_P3_BITMASK);
        p4 = parity((unsigned char)nibbles[i], mask = (unsigned char)HAMMING_P4_BITMASK);
        p5 = parity((unsigned char)nibbles[i], mask = (unsigned char)HAMMING_P5_BITMASK);

        uint8_t cr_now = (i < d_sf - 2) ? 4 : d_cr;

        switch (cr_now)
        {
            case 1:
                codewords[i] = (p4 << 4) | (nibbles[i] & 0xF);
                break;
            case 2:
                codewords[i] = (p5 << 5) | (p3 << 4) | (nibbles[i] & 0xF);
                break;
            case 3:
                codewords[i] = (p2 << 6) | (p5 << 5) | (p3 << 4) | (nibbles[i] & 0xF);
                break;
            case 4:
                codewords[i] = (p1 << 7) | (p2 << 6) | (p5 << 5) | (p3 << 4) | (nibbles[i] & 0xF);
                break;
            case 0:
                codewords[i] = (nibbles[i] & 0xF);
                break;
            default:
                // THIS CASE SHOULD NOT HAPPEN
                printf("Invalid Code Rate: %d  -- this state should never occur.\n", cr_now);
                break;
        }
    }
}

int interleave(int nums)
{
    uint32_t bits_per_word = 8;
    uint32_t ppm = d_sf - 2;
    int _idx = 0;
    for (uint32_t start_idx = 0; start_idx + ppm - 1 < (uint32_t)nums;) {
        bits_per_word = (start_idx == 0) ? 8 : (d_cr + 4);
        ppm = (start_idx == 0) ? (d_sf - 2) : (d_sf - 2 * d_ldr);
        uint16_t block[bits_per_word];
        memset(block, 0, sizeof(block));
        // std::vector<uint16_t> block(bits_per_word, 0u);

        for (uint32_t i = 0; i < ppm; i++) {
            const uint32_t word = codewords[start_idx + i];
            for (uint32_t j = (1u << (bits_per_word - 1)), x = bits_per_word - 1; j; j >>= 1u, x--) {
                block[x] |= !!(word & j) << i;
            }
        }
        if (LOG_LEVEL > 1)
        {
            printf("[Interleave]Pre_ROTL:\n");
            for(uint32_t i = 0; i < bits_per_word; ++i) printf(" %d", block[i]);
            printf("\n");
        }

        for (uint32_t i = 0; i < bits_per_word; ++i)
        {
            // rotate each element to the right by i bits
            block[i] = rotl(block[i], 2*ppm - i, ppm);
        }
        if (LOG_LEVEL > 1)
        {
            printf("[Interleave]ROTL:\n");
            for(uint32_t i = 0; i < bits_per_word; ++i) printf(" %d", block[i]);
            printf("\n");
        }
        
        for (uint32_t i = 0; i < bits_per_word; ++i)
        {
            symbols[_idx] = block[i];
            _idx++;
        }
        start_idx = start_idx + ppm;
    }

    // printf("symbols_num: %d\n", _idx);
    return _idx;
}


void from_gray(int nums)
{
    for (int i = 0; i < nums; i++)
    {
        symbols[i] = symbols[i] ^ (symbols[i] >> 16);
        symbols[i] = symbols[i] ^ (symbols[i] >>  8);
        symbols[i] = symbols[i] ^ (symbols[i] >>  4);
        symbols[i] = symbols[i] ^ (symbols[i] >>  2);
        symbols[i] = symbols[i] ^ (symbols[i] >>  1);
        symbols[i] = (i < 8 || d_ldr) ? (symbols[i] * 4 + 1) % (1 << d_sf) : (symbols[i] + 1) % (1 << d_sf);
    }
}



int encode(uint8_t *bytes, int length, uint16_t *freq_symbols)
{
    int idx = length;
    if (LOG_LEVEL > 0)
    {
        printf("Bytes:\n");
        print_nums(bytes, idx);
    }
    // ADD CRC
    if(d_crc)
    {
        uint16_t check_sum = data_checksum(bytes, length);
        *(bytes + idx) = check_sum & 0xFF;
        idx++;
        *(bytes + idx) = (check_sum >> 8) & 0xFF;
        idx++;
        if (LOG_LEVEL > 0)
        {
            printf("[ADD_CRC]Bytes:\n");
            print_nums(bytes, idx);
        }
    }

    uint16_t sym_num = calc_sym_num(length);
    uint16_t nibble_num = d_sf - 2 + (sym_num - 8) / (d_cr + 4) * (d_sf - 2 * d_ldr);
    uint16_t redundant_num = ceil((nibble_num - 2 * idx) / 2);
    
    for (int i = 0; i < redundant_num; ++i)
    {
        *(bytes + idx) = 0;
        idx++;
    }
    // Whiten 
    whiten(bytes, length);
    if (LOG_LEVEL > 0)
    {
        printf("sym_num:%d, nibble_num:%d, redundant_num:%d\n", sym_num, nibble_num, redundant_num);
        printf("[Whiten]Bytes:\n");
        print_nums(bytes, idx);
    }
    for (int i = 0; i < nibble_num; ++i)
    {
        if (i % 2 == 0)
        {
            nibbles[i] = *(bytes + (int)(i / 2)) & 0xF;
        }
        else
        {
            nibbles[i] = (*(bytes + (int)(i / 2)) >> 4) & 0xF;
        }
    }

    // Hamming Encode
    hamming_encode(nibble_num);
    // 
    int symbols_num = interleave(nibble_num);
    if (LOG_LEVEL > 0)
    {
        printf("[Split Bytes]Nibbles:\n");
        print_nums(&nibbles[0], nibble_num);
        printf("[Hamming Encode]Codewords:\n");
        print_nums(&codewords[0], nibble_num);
        printf("[Interleave]");
        print_symbols(&symbols[0], symbols_num);
    }
    from_gray(symbols_num);
    for(int i = 0; i < symbols_num; ++i) *(freq_symbols + i) = symbols[i];
    if (LOG_LEVEL > 0)
    {
        printf("[Gray Encode] Final Freq Symbols:");
        print_symbols(freq_symbols, symbols_num);
    }
    return symbols_num;
}

void update_parameters(uint8_t sf, int cr, int crc, int payload_len)
{
    d_sf = sf;
    d_cr = cr;
    d_crc = crc;
    PAYLOAD_LENGTH = payload_len;
    printf("[lora_encode]Update! d_sf:%d, d_cr:%d, d_crc: %d, payload_len: %d.\n", sf, cr, crc, PAYLOAD_LENGTH);

}

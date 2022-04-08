#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "crc_check.h"
#include "utility.h"

// local variable
uint8_t lookup_table[256];
int lookup_table_calculated = 0;

static uint8_t get_max_digit(uint8_t number)
{
    uint8_t i;
    uint8_t digit_arr[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    for (i = 7; i >= 0; --i)
    {
        if (number & digit_arr[i])
        {
            return i + 1;
        }
    }
    return 0;
}

// calculate CRC-8-ATM for each 8 bits pattern
static void calc_lookup_table(void)
{
    // polynomial = x^8 + x^2 + x+ 1
    uint8_t poly_ref = 0x07; // the first bit(1) is ignore, full poly is 100000111
    uint8_t i, shift, current_byte, next_byte;

    log_message("start calculating CRC look up table");
    for (i = 0; i < 256; ++i)
    {
        next_byte = 0x00;
        current_byte = i;
        while (current_byte != 0)
        {
            shift = get_max_digit(current_byte);
            current_byte = current_byte ^ (((poly_ref >> 1) | 0x80) >> (8 - shift)); // add first bit then shift
            next_byte = next_byte ^ (poly_ref << (shift - 1));
        }
        lookup_table[i] = next_byte;

        // prevent overflow
        if (i == 255)
        {
            break;
        }
    }
    log_message("finish calculating CRC look up table");
}

// return CRC-8-ATM
uint8_t calc_CRC_8_ATM(unsigned char *target, size_t target_size)
{
    uint8_t pattern, current_byte;
    size_t i;

    if (target_size == 0)
    {
        log_error("passing target_size = 0 to calc_CRC_8_ATM()");
    }
    if (!lookup_table_calculated)
    {
        calc_lookup_table();
        lookup_table_calculated = 1;
    }

    pattern = 0x00;
    for (i = 0; i < target_size; ++i)
    {
        current_byte = *((uint8_t *)(target + i));
        current_byte = current_byte ^ pattern;
        pattern = lookup_table[current_byte];
    }

    return pattern;
}

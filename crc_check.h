#ifndef CRC_CHECK_H
#define CRC_CHECK_H
#include <stdint.h>
#include <stdlib.h>

uint8_t calc_CRC_8_ATM_rev(unsigned char *target, size_t target_size);

#endif
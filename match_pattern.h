#ifndef BIT_SHIFT_H
#define BIT_SHIFT_H

#define SCIENCE_DATA_SIZE 1104
#define SD_header_SIZE 6

extern unsigned int bit_shift;

int is_sd_header(unsigned char* target);
int find_next_sd_header(unsigned char** buffer, size_t current_sd_header_location);

#endif
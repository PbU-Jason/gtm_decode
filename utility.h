#ifndef UTILITY_H
#define UTILITY_H
#include <stdlib.h>
#include <stdint.h>

#define MAX_BINARY_BUFFER_SIZE 1174405120   //in bytes

extern unsigned char* binary_buffer;

void log_message(char* description);
void log_error(char* description);
void check_endianness(void);
void big2little_endian(unsigned char* target, size_t target_size);
unsigned char** create_2D_arr(size_t row, size_t col);
void destroy_2D_arr(unsigned char** arr, size_t row);
void left_shift_mem(unsigned char* target_start, size_t target_size, uint8_t bits);
void create_all_buffer(void);
void destroy_all_buffer(void);
void print_buffer_around(unsigned char* target, int back, int forward);
#endif
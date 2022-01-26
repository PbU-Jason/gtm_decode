#ifndef UTILITY_H
#define UTILITY_H
#include <stdlib.h>

#define MAX_BUFFER_SIZE 1174405120   //in bytes

void log_message(char* description);
void log_error(char* description);
unsigned char** create_2D_arr(size_t row, size_t col);
void destroy_2D_arr(unsigned char** arr, size_t row);
void left_shift_mem(unsigned char* target_start, size_t target_size, unsigned int bits);
size_t load_buffer(FILE* file_stream, unsigned char** target, size_t target_size);

#endif
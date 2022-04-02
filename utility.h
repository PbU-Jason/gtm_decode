#ifndef UTILITY_H
#define UTILITY_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "match_pattern.h"

extern size_t max_binary_buffer_size; // in bytes
extern int decode_mode;
extern int export_mode;
extern int terminal_out;
extern int debug_output;
extern int exclude_nohit;
extern unsigned char *binary_buffer;
extern FILE *bin_file;
extern FILE *out_file_raw;
extern FILE *out_file_raw_sync;
extern FILE *out_file_pipeline;
extern FILE *out_file_pipeline_pos; // for decoding science data + pipeline output, use to record position data

void log_message(const char *format, ...);
void log_error(const char *format, ...);
void check_endianness(void);
void big2little_endian(void *target, size_t target_size);
void left_shift_mem(unsigned char *target_start, size_t target_size, uint8_t bits);
void create_all_buffer(void);
void destroy_all_buffer(void);
void print_buffer_around(unsigned char *target, int back, int forward);
void open_all_file(char *input_file_path, char *out_file_path);
void close_all_file(void);
double calc_sec(Time *time);
double find_time_delta(Time *start, Time *end);
void get_month_and_mday(void);
#endif
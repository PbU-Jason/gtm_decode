#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "utility.h"
#include "match_pattern.h"

// global variables
size_t max_binary_buffer_size = 1174405120; // 1GB
int decode_mode = 0;
int export_mode = 0;
int terminal_out = 0;
int debug_output = 1;
unsigned char *binary_buffer = NULL;
FILE *bin_file = NULL;
FILE *out_file_raw = NULL;
FILE *out_file_pipeline = NULL;
FILE *out_file_pipeline_pos = NULL;

// local variable
char tmtc_raw_header[] = "head;gtm module;Packet Counter;year;day;hour;minute;sec;Lastest PPS Counter;Lastest Fine Time Counter Value Between 2 PPS;Board Temperature#1;Board Temperature#2;CITIROC1 Temperature#1;CITIROC1 Temperature#2;CITIROC2 Temperature#1;CITIROC2 Temperature#2;CITIROC1 Live time;CITIROC2 Live time;CITIROC1 Hit Counter#0;CITIROC1 Hit Counter#1;CITIROC1 Hit Counter#2;CITIROC1 Hit Counter#3;CITIROC1 Hit Counter#4;CITIROC1 Hit Counter#5;CITIROC1 Hit Counter#6;CITIROC1 Hit Counter#7;CITIROC1 Hit Counter#8;CITIROC1 Hit Counter#9;CITIROC1 Hit Counter#10;CITIROC1 Hit Counter#11;CITIROC1 Hit Counter#12;CITIROC1 Hit Counter#13;CITIROC1 Hit Counter#14;CITIROC1 Hit Counter#15;CITIROC1 Hit Counter#16;CITIROC1 Hit Counter#17;CITIROC1 Hit Counter#18;CITIROC1 Hit Counter#19;CITIROC1 Hit Counter#20;CITIROC1 Hit Counter#21;CITIROC1 Hit Counter#22;CITIROC1 Hit Counter#23;CITIROC1 Hit Counter#24;CITIROC1 Hit Counter#25;CITIROC1 Hit Counter#26;CITIROC1 Hit Counter#27;CITIROC1 Hit Counter#28;CITIROC1 Hit Counter#29;CITIROC1 Hit Counter#30;CITIROC1 Hit Counter#31;CITIROC2 Hit Counter#0;CITIROC2 Hit Counter#1;CITIROC2 Hit Counter#2;CITIROC2 Hit Counter#3;CITIROC2 Hit Counter#4;CITIROC2 Hit Counter#5;CITIROC2 Hit Counter#6;CITIROC2 Hit Counter#7;CITIROC2 Hit Counter#8;CITIROC2 Hit Counter#9;CITIROC2 Hit Counter#10;CITIROC2 Hit Counter#11;CITIROC2 Hit Counter#12;CITIROC2 Hit Counter#13;CITIROC2 Hit Counter#14;CITIROC2 Hit Counter#15;CITIROC2 Hit Counter#16;CITIROC2 Hit Counter#17;CITIROC2 Hit Counter#18;CITIROC2 Hit Counter#19;CITIROC2 Hit Counter#20;CITIROC2 Hit Counter#21;CITIROC2 Hit Counter#22;CITIROC2 Hit Counter#23;CITIROC2 Hit Counter#24;CITIROC2 Hit Counter#25;CITIROC2 Hit Counter#26;CITIROC2 Hit Counter#27;CITIROC2 Hit Counter#28;CITIROC2 Hit Counter#29;CITIROC2 Hit Counter#30;CITIROC2 Hit Counter#31;CITIROC1 Trigger counter;CITIROC2 Trigger counter;Counter period Setting;HV DAC1;HV DAC2;SPW#A Error count;SPW#B Error count;SPW#A Last Recv Byte;SPW#B Last Recv Byte;SPW#A status;SPW#B status;Recv Checksum of Last CMD;Calc Checksum of Last CMD;Number of Recv CMDs;SEU-Measurement#1;SEU-Measurement#2;SEU-Measurement#3;checksum;tail\n";

void log_message(const char *format, ...)
{
    if (debug_output)
    {
        va_list args;
        va_start(args, format);
        printf("Message: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}

void log_error(const char *format, ...)
{
    if (debug_output)
    {
        va_list args;
        va_start(args, format);
        printf("ERROR: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
        exit(1);
    }
}

void check_endianness(void)
{
    unsigned char x[2] = {0x00, 0x01};
    uint16_t *y;
    y = (uint16_t *)x;
    if (*y == 1)
    {
        log_error("Your computer use big endian format, which is not supported by the program!!");
    }
}

void big2little_endian(void *target, size_t target_size)
{
    unsigned char *buffer = NULL;
    size_t i;

    buffer = (unsigned char *)malloc(target_size);
    for (i = 0; i < target_size; ++i)
    {
        buffer[i] = ((unsigned char *)target)[target_size - 1 - i];
    }

    memcpy(target, buffer, target_size);
    free(buffer);
}

// shift the array n bits left, you should make sure 0<=bits<=7
void left_shift_mem(unsigned char *target, size_t target_size, uint8_t bits)
{
    unsigned char current, next;
    size_t i;

    for (i = 0; i < target_size - 1; ++i)
    {
        current = target[i];
        next = target[i + 1];
        target[i] = (current << bits) | (next >> (8 - bits));
    }
    // shift the last element
    target[target_size - 1] = target[target_size - 1] << bits;
}

// allocate all global buffer
void create_all_buffer(void)
{
    binary_buffer = (unsigned char *)malloc(max_binary_buffer_size);
    if (!binary_buffer)
    {
        log_error("fail to create binary buffer");
    }

    sync_data_buffer = (unsigned char *)malloc(SYNC_DATA_SIZE);
    if (!sync_data_buffer)
    {
        log_error("fail to create sync data buffer");
    }

    time_buffer = (Time *)malloc(sizeof(Time));
    if (!time_buffer)
    {
        log_error("faile to create time buffer");
    }
    // initialize value
    time_buffer->pps_counter = 0;
    time_buffer->fine_counter = 0;

    time_start = (Time *)malloc(sizeof(Time));
    if (!time_start)
    {
        log_error("fail to create time start buffer");
    }

    position_buffer = (Position *)malloc(sizeof(Position));
    if (!position_buffer)
    {
        log_error("faile to create position buffer");
    }

    pre_position = (Position *)malloc(sizeof(Position));
    if (!pre_position)
    {
        log_error("fail to create pre position buffer");
    }

    event_buffer = (Event *)malloc(sizeof(Event));
    if (!event_buffer)
    {
        log_error("fail to create event buffer");
    }

    tmtc_buffer = (Tmtc *)malloc(sizeof(Tmtc));
    if (!tmtc_buffer)
    {
        log_error("fail to create tmtc buffer");
    }
}

void destroy_all_buffer(void)
{
    free(binary_buffer);
    free(sync_data_buffer);
    free(time_buffer);
    free(time_start);
    free(position_buffer);
    free(pre_position);
    free(event_buffer);
    free(tmtc_buffer);
}

// debug use, print from i=-back to i<=forward
void print_buffer_around(unsigned char *target, int back, int forward)
{
    int i;
    printf("debug print --------------------\n");
    for (i = -back; i <= forward; ++i)
    {
        printf("%02x ", *(target + i));
    }
    printf("\n--------------------------------\n");
}

char *easy_strcat(char *prefix, char *postfix)
{
    char *new;
    size_t size_prefix, size_postfix;

    size_prefix = strlen(prefix);
    size_postfix = strlen(postfix);

    new = (char *)malloc((size_prefix + size_postfix + 1) * sizeof(char));
    if (!new)
    {
        log_error("fail to create new str buffer in easy_strcat");
    }

    memcpy(new, prefix, size_prefix + 1);
    strcat(new, postfix);
    return new;
}

void open_all_file(char *input_file_path, char *out_file_path)
{
    char *raw_outpath = NULL, *pipeline_outpath = NULL, *pipeline_pos_outpath = NULL;
    // input file
    bin_file = fopen(input_file_path, "rb");
    if (!bin_file)
    {
        log_error("binary file not found");
    }
    log_message("finish loading bin file");
    // output file
    if (terminal_out)
    {
        out_file_raw = stdout;
        out_file_pipeline = stdout;
    }
    else
    {
        switch (decode_mode)
        {
        case 0:
            if (export_mode == 0 || export_mode == 2)
            {
                raw_outpath = easy_strcat(out_file_path, "_science_raw.txt");
                out_file_raw = fopen(raw_outpath, "w");
                if (!out_file_raw)
                {
                    log_error("can't open raw output file");
                }
                free(raw_outpath);
            }
            if (export_mode == 1 || export_mode == 2)
            {
                pipeline_outpath = easy_strcat(out_file_path, "_science_pipeline.txt");
                out_file_pipeline = fopen(pipeline_outpath, "w");
                if (!out_file_pipeline)
                {
                    log_error("can't open pipeline output file");
                }
                free(pipeline_outpath);

                pipeline_pos_outpath = easy_strcat(out_file_path, "_science_pipeline_pos.txt");
                out_file_pipeline_pos = fopen(pipeline_pos_outpath, "w");
                if (!out_file_pipeline_pos)
                {
                    log_error("can't open pipeline_pos output file");
                }
                free(pipeline_pos_outpath);
            }
            break;
        case 1:
            raw_outpath = easy_strcat(out_file_path, "_tmtc.csv");
            out_file_raw = fopen(raw_outpath, "w");
            if (!out_file_raw)
            {
                log_error("can't open raw output file");
            }
            fputs(tmtc_raw_header, out_file_raw);
            free(raw_outpath);
            break;
        case 2:
            raw_outpath = easy_strcat(out_file_path, "_extracted.bin");
            out_file_raw = fopen(raw_outpath, "w");
            if (!out_file_raw)
            {
                log_error("can't open raw output file");
            }
            free(raw_outpath);
            break;
        default:
            log_error("unknown decode mode");
            break;
        }
    }
}

void close_all_file(void)
{
    fclose(bin_file);
    switch (decode_mode)
    {
    case 0:
        if (export_mode == 0 || export_mode == 2)
        {
            fclose(out_file_raw);
        }
        if (export_mode == 1 || export_mode == 2)
        {
            fclose(out_file_pipeline);
            fclose(out_file_pipeline_pos);
        }
        break;
    case 1:
        fclose(out_file_raw);
        break;
    case 2:
        fclose(out_file_raw);
        break;
    default:
        log_error("unknown decode mode");
        break;
    }
    log_message("close all file");
}

double calc_sec(Time *time)
{
    double total_sec;
    total_sec = (double)time->sec + ((double)time->sub_sec) * 0.001 + (double)time->pps_counter + ((double)time->fine_counter) * 0.24 * 0.000001;
    return total_sec;
}

double find_time_delta(Time *time_start, Time *time_end)
{
    double del_sec = 0;
    del_sec += (time_end->year - time_start->year) * 31536000;
    del_sec += (time_end->day - time_start->day) * 86400;
    del_sec += (time_end->hour - time_start->hour) * 3600;
    del_sec += (time_end->minute - time_start->minute) * 60;
    del_sec += (calc_sec(time_end) - calc_sec(time_start));
    return del_sec;
}

void get_month_and_mday(void)
{
    struct tm time_old, *time_new = NULL;
    time_t loctime;

    time_old.tm_sec = 0;
    time_old.tm_min = 0;
    time_old.tm_hour = 0;
    time_old.tm_mday = (int)time_buffer->day;
    time_old.tm_mon = 1;
    time_old.tm_year = (int)time_buffer->year;
    time_old.tm_wday = 0;
    time_old.tm_yday = 0;
    time_old.tm_isdst = 0;

    log_message("time old year %i, days %i", time_old.tm_year, time_old.tm_yday);
    loctime = mktime(&time_old);
    time_new = localtime(&loctime);
    if (!time_new)
    {
        log_error("NULL time_new in get_month_and_mday");
    }
    log_message("time new year %i, month %i, mday %i", time_new->tm_year, time_new->tm_mon, time_new->tm_mday);
    time_buffer->month = (uint8_t)time_new->tm_mon;
    time_buffer->mday = (uint8_t)time_new->tm_mday;
}

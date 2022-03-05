#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "utility.h"
#include "match_pattern.h"

size_t max_binary_buffer_size = 1174405120; //1GB
int decode_mode = 0;
int export_mode = 0;
int terminal_out = 0;
int debug_output = 1;
unsigned char* binary_buffer = NULL;
FILE* bin_file = NULL;
FILE* out_file_raw = NULL;
FILE* out_file_pipeline = NULL;

//local variable
char tmtc_raw_header[] = "gtm module;Packet Counter;Lastest PPS Counter;Lastest Fine Time Counter Value Between 2 PPS;Board Temperature#1;Board Temperature#2;CITIROC1 Temperature#1;CITIROC1 Temperature#2;CITIROC2 Temperature#1;CITIROC2 Temperature#2;CITIROC1 Live time;CITIROC2 Live time;CITIROC1 Hit Counter#0;CITIROC1 Hit Counter#1;CITIROC1 Hit Counter#2;CITIROC1 Hit Counter#3;CITIROC1 Hit Counter#4;CITIROC1 Hit Counter#5;CITIROC1 Hit Counter#6;CITIROC1 Hit Counter#7;CITIROC1 Hit Counter#8;CITIROC1 Hit Counter#9;CITIROC1 Hit Counter#10;CITIROC1 Hit Counter#11;CITIROC1 Hit Counter#12;CITIROC1 Hit Counter#13;CITIROC1 Hit Counter#14;CITIROC1 Hit Counter#15;CITIROC1 Hit Counter#16;CITIROC1 Hit Counter#17;CITIROC1 Hit Counter#18;CITIROC1 Hit Counter#19;CITIROC1 Hit Counter#20;CITIROC1 Hit Counter#21;CITIROC1 Hit Counter#22;CITIROC1 Hit Counter#23;CITIROC1 Hit Counter#24;CITIROC1 Hit Counter#25;CITIROC1 Hit Counter#26;CITIROC1 Hit Counter#27;CITIROC1 Hit Counter#28;CITIROC1 Hit Counter#29;CITIROC1 Hit Counter#30;CITIROC1 Hit Counter#31;CITIROC2 Hit Counter#0;CITIROC2 Hit Counter#1;CITIROC2 Hit Counter#2;CITIROC2 Hit Counter#3;CITIROC2 Hit Counter#4;CITIROC2 Hit Counter#5;CITIROC2 Hit Counter#6;CITIROC2 Hit Counter#7;CITIROC2 Hit Counter#8;CITIROC2 Hit Counter#9;CITIROC2 Hit Counter#10;CITIROC2 Hit Counter#11;CITIROC2 Hit Counter#12;CITIROC2 Hit Counter#13;CITIROC2 Hit Counter#14;CITIROC2 Hit Counter#15;CITIROC2 Hit Counter#16;CITIROC2 Hit Counter#17;CITIROC2 Hit Counter#18;CITIROC2 Hit Counter#19;CITIROC2 Hit Counter#20;CITIROC2 Hit Counter#21;CITIROC2 Hit Counter#22;CITIROC2 Hit Counter#23;CITIROC2 Hit Counter#24;CITIROC2 Hit Counter#25;CITIROC2 Hit Counter#26;CITIROC2 Hit Counter#27;CITIROC2 Hit Counter#28;CITIROC2 Hit Counter#29;CITIROC2 Hit Counter#30;CITIROC2 Hit Counter#31;CITIROC1 Trigger counter;CITIROC2 Trigger counter;Counter period Setting;HV DAC1;HV DAC2;SPW#A Error count;SPW#B Error count;SPW#A Last Recv Byte;SPW#A Last Recv Byte;SPW#A status;SPW#B status;Recv Checksum of Last CMD;Calc Checksum of Last CMD;Number of Recv CMDs;SEU-Measurement#1;SEU-Measurement#2;SEU-Measurement#3;checksum\n";


void log_message(const char* format, ...){
    if (debug_output){
        va_list args;
        va_start(args, format);
        printf("Message: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
}

void log_error(const char* format, ...){
    if (debug_output){
        va_list args;
        va_start(args, format);
        printf("ERROR: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
        exit(1);
    }
}

void check_endianness(void){
    unsigned char x[2] = {0x00, 0x01};
    uint16_t* y;
    y = (uint16_t*) x;
    if (*y == 1){
        log_error("Your computer use big endian format, which is not supported by the program!!");
    }
}

void big2little_endian(void* target, size_t target_size){
    unsigned char* buffer = NULL;
    size_t i;

    buffer = (unsigned char*) malloc(target_size);
    for (i=0;i<target_size;++i){
        buffer[i] = ((unsigned char*) target)[target_size - 1 - i];
    }
    
    memcpy(target, buffer, target_size);
    free(buffer);
}

unsigned char** create_2D_arr(size_t row, size_t col){
	size_t i;
	unsigned char** arr;
	arr = (unsigned char**) malloc(row * sizeof(unsigned char*));
	if (! arr){log_error("fail to allocate 2D arr level 1\n");}
	for (i=0; i<row;i++){
		arr[i] = (unsigned char*) malloc(col);
		if (! arr[i]){log_error("fail to allocate 2D arr level 2\n");}
	}
	return arr;
}

void destroy_2D_arr(unsigned char** arr, size_t row){
	size_t i;
	for (i=0;i<row;i++){
		free(arr[i]);
	}
	free(arr);
}

//shift the array n bits left, you should make sure 0<=bits<=7
void left_shift_mem(unsigned char* target, size_t target_size, uint8_t bits){
    unsigned char current, next;
    size_t i;

    for (i=0;i<target_size-1;++i){
        current = target[i];
        next = target[i+1];
        target[i] = (current << bits) | (next >> (8-bits));
    }
    //shift the last element
    target[target_size-1] = target[target_size-1] << bits;
}

void create_all_buffer(void){
    binary_buffer = (unsigned char*) malloc(max_binary_buffer_size);
    if (! binary_buffer){log_error("fail to create binary buffer");}

    sync_data_buffer = (unsigned char*) malloc(SYNC_DATA_SIZE);
    if (! sync_data_buffer){
        log_error("fail to create sync data buffer");
    }

    tmtc_data_buffer = (unsigned char*) malloc(TMTC_DATA_SIZE);
    if (! tmtc_data_buffer){
        log_error("fail to create tmtc data buffer");
    }

    time_buffer = (Time*) malloc(sizeof(Time));
    if (! time_buffer){
        log_error("faile to create time buffer");
    }

    position_buffer = (Position*) malloc(sizeof(Position));
    if (! position_buffer){
        log_error("faile to create position buffer");
    }

    event_buffer = (Event*) malloc(sizeof(Event));
    if (! event_buffer){
        log_error("fail to create event buffer");
    }

    tmtc_buffer = (Tmtc*) malloc(sizeof(Tmtc));
    if (! tmtc_buffer){
        log_error("fail to create tmtc buffer");
    }
}

void destroy_all_buffer(void){
    free(binary_buffer);
    free(sync_data_buffer);
    free(tmtc_data_buffer);
    free(time_buffer);
    free(position_buffer);
    free(event_buffer);
    free(tmtc_buffer);
}

void print_buffer_around(unsigned char* target, int back, int forward){
    int i;
    printf("debug print --------------------\n");
    for (i=-back;i<=forward;++i){
        printf("%02x ", *(target + i));
    }
    printf("\n-------------------------------\n");
}

void open_all_file(char* input_file_path, char* out_file_path){
    size_t size_prefix, size_postfix_raw, size_postfix_pipeline;
    char raw_postfix[] = "_raw.txt";
    char pipeline_postfix[] = "_pipeline.txt";
    char *raw_outpath = NULL, *pipeline_outpath = NULL;
    
    size_prefix = sizeof(out_file_path);
    size_postfix_raw = sizeof(raw_postfix);
    size_postfix_pipeline = sizeof(pipeline_postfix);

    bin_file = fopen(input_file_path, "rb");
    if (! bin_file){log_error("binary file not found");}
    log_message("finish loading bin file");

    if (terminal_out){
        out_file_raw = stdout;
        out_file_pipeline = stdout;
    }
    else{
        //figure out full output file path
        raw_outpath = (char*) malloc(size_prefix + size_postfix_raw);
        if (! raw_outpath){log_error("fail to create raw_outpath in open_all_file()");}
        memcpy(raw_outpath, out_file_path, size_prefix);
        strcat(raw_outpath, raw_postfix);
        pipeline_outpath = (char*) malloc(size_prefix + size_postfix_pipeline);
        if (! raw_outpath){log_error("fail to create pipeline_outpath in open_all_file()");}
        memcpy(pipeline_outpath, out_file_path, size_prefix);
        strcat(pipeline_outpath, pipeline_postfix);

        if (export_mode != 0 && export_mode != 1 && export_mode != 2){
            log_error("unknown export mode");
        }

        if (export_mode == 0 || export_mode == 2){
            out_file_raw = fopen(raw_outpath, "w");
            if (! out_file_raw){log_error("can't open raw output file");}
            if (decode_mode == 1){
                fprintf(out_file_raw, tmtc_raw_header);
            }
        }
        if (export_mode == 1 || export_mode == 2){
            out_file_pipeline = fopen(pipeline_outpath, "w");
            if (! out_file_pipeline){log_error("can't open pipeline output file");}
        }
        log_message("finish opening output file");
    }
}

void close_all_file(void){
    fclose(bin_file);
    switch (export_mode)
    {
    case 0:
        fclose(out_file_raw);
        break;
    case 1:
        fclose(out_file_pipeline);
        break;
    case 2:
        fclose(out_file_raw);
        fclose(out_file_pipeline);
        break;
    default:
        log_error("unknown export mode");
        break;
    }
    log_message("close all file");
}


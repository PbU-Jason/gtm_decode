#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "utility.h"
#include "match_pattern.h"

size_t max_binary_buffer_size = 1174405120;
unsigned char* binary_buffer = NULL;
FILE* bin_file = NULL;
FILE* out_file = NULL;

void log_message(char* description){
    printf("Message: ");
    printf(description);
    printf("\n");
}

void log_error(char* description){
    printf("ERROR: ");
    printf(description);
    printf("\n");
    exit(1);
}

void check_endianness(void){
    unsigned char x[2] = {0x00, 0x01};
    uint16_t* y;
    y = (uint16_t*) x;
    if (*y == 1){
        log_error("Your computer use big endian format, which is not supported by the program!!");
    }
}

void big2little_endian(unsigned char* target, size_t target_size){
    unsigned char* buffer = NULL;
    size_t i;

    buffer = (unsigned char*) malloc(target_size);
    for (i=0;i<target_size;++i){
        buffer[i] = target[target_size - 1 - i];
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

    event_buffer = (Event*) malloc(sizeof(Event));
    if (! event_buffer){
        log_error("fail to create event buffer");
    }
}

void destroy_all_buffer(void){
    free(binary_buffer);
    free(sync_data_buffer);
    free(event_buffer);
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
     bin_file = fopen(input_file_path, "rb");
    if (! bin_file){log_error("binary file not found");}
    log_message("finish loading bin file");

    out_file = fopen(out_file_path, "w");
    if (! out_file){log_error("can't open output file");}
    log_message("finish opening output file");
}

void close_all_file(void){
    fclose(bin_file);
    fclose(out_file);
}


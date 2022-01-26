#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utility.h"


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
void left_shift_mem(unsigned char* target, size_t target_size, unsigned int bits){
    unsigned char current, next, new;
    size_t i;

    for (i=0;i<target_size-1;++i){
        current = target[i];
        next = target[i+1];
        new = (current << bits) | (next >> (8-bits));

        target[i] = new;
    }
    //shift the last element
    target[target_size-1] = target[target_size-1] << bits;
}


size_t load_buffer(FILE* file_stream, unsigned char** target, size_t target_size){
    int i;
    unsigned char* temp;
    size_t buffer_size = 0;
    temp = (unsigned char*) malloc(target_size);
    if (! temp){log_error("can't allocate temp arr in load_buffer()");}
    buffer_size = fread(temp, 1, target_size, file_stream);
    if (buffer_size){
        //shift the correct bit and copy temp into target
        for (i=0;i<8;++i){
            memcpy(target[i], temp, buffer_size);
            left_shift_mem(temp, target_size, 1);
        }
    }

    free(temp);
    return buffer_size;
}


/*
size_t load_buffer(FILE* file_stream, unsigned char** target, size_t target_size){
    int i;
    size_t buffer_size = 0;
    buffer_size = fread(target[0], 1, target_size, file_stream);
    if (buffer_size){
        //shift the correct bit and copy temp into target
        for (i=1;i<8;++i){
            memcpy(target[i], target[0], buffer_size);
            left_shift_mem(target[i], target_size, i);
        }
    }

    return buffer_size;
}
*/
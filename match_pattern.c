#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utility.h"
#include "match_pattern.h"

unsigned int bit_shift = 0;

int is_sd_header(unsigned char* target){
    static unsigned char target_copy[SD_header_SIZE];
    static unsigned char ref_master[SD_header_SIZE]={0x88, 0x55, 0xC0, 0x00, 0x04, 0x4f};
    static unsigned char ref_slave[SD_header_SIZE]={0x88, 0xAA, 0xC0, 0x00, 0x04, 0x4f};

    //mask the sequence count byte
    memcpy(target_copy, target, SD_header_SIZE);
    target_copy[3] = 0x00;

    if (! memcmp(target_copy, ref_master, SD_header_SIZE)){return 1;}
    if (! memcmp(target_copy, ref_slave, SD_header_SIZE)){return 1;}
    return 0;
}

int find_next_sd_header(unsigned char** buffer, size_t current_sd_header_location){
    unsigned int shift;
    size_t location, byte;
    location = current_sd_header_location + SCIENCE_DATA_SIZE + SD_header_SIZE; // the size of sd header is 6
    if (is_sd_header(buffer[bit_shift] + location)){
        return location;
    }
    else{
        //look backward
        byte = SCIENCE_DATA_SIZE;
        while (byte--)
        {
            location--;
            //inspect each bit shift
            for (shift=0;shift<8;shift++){
                if (is_sd_header(buffer[shift] + location)){
                    bit_shift = shift;
                    return location;
                }
            }
        }
        //if no next sd header is found
        log_error("no next sd header");
        return 0;
    }
}
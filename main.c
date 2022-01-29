#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "match_pattern.h"

// the code is designed for little endian computers (like x86_64) !!
int main(void){
    log_message("program start");

    FILE* bin_file = NULL;
    size_t actual_binary_buffer_size = 0;
    size_t sd_header_location = 0;
    size_t old_sd_header_location = 0;
    size_t full = 0;
    size_t broken = 0;

    check_endianness();

    //create all the global buffer and setup global variable
    create_all_buffer();
    sync_data_buffer_counter = SYNC_DATA_SIZE;
    event_buffer->pps_counter = INI_PPS_COUNTER;
    event_buffer->fine_counter = INI_FINE_COUNTER;
    missing_sync_data = 1;

    bin_file = fopen("/home/ian/Documents/code/gtm/decode/Data_151222.368.bin", "rb");
    if (! bin_file){log_error("file not found");}
    log_message("fin loading bin file");

    actual_binary_buffer_size = fread(binary_buffer, 1, MAX_BINARY_BUFFER_SIZE, bin_file);
    while(actual_binary_buffer_size > SCIENCE_DATA_SIZE + SD_HEADER_SIZE){
        log_message("load new chunk");
        sd_header_location = 0;
        if (! is_sd_header(binary_buffer)){
            //int i;
            //for (i=0;i<15;++i){
            //    printf("%02x", binary_buffer[0][i]);
            //}
            //printf("\n");
            log_error("bin file doesn't start with sd header");
        }
        while (sd_header_location + SCIENCE_DATA_SIZE + SD_HEADER_SIZE < actual_binary_buffer_size){
            //if the packet is not continueous, reset related parameter
            if (0){ //place holder, write the condition later
                sync_data_buffer_counter = 0;
            }

            // find next sd header
            old_sd_header_location = sd_header_location;
            sd_header_location = find_next_sd_header(binary_buffer, sd_header_location, actual_binary_buffer_size);
            if (sd_header_location - old_sd_header_location == SCIENCE_DATA_SIZE + SD_HEADER_SIZE){
                //process science data
                log_message("start parsing full science packet");
                parse_full_science_packet(binary_buffer, sd_header_location);
                full++;
                //printf("sd header location = %zu, bit shift = %u\n", sd_header_location, bit_shift);
                return 0;
            }
            else{
                broken++;
            }
        }

        //offset position indicator of input file stream based on previous sd header position
        fseek(bin_file, sd_header_location - actual_binary_buffer_size, SEEK_CUR);
        actual_binary_buffer_size = fread(binary_buffer, 1, MAX_BINARY_BUFFER_SIZE, bin_file);
    }
    printf("full = %zu, broken = %zu\n", full, broken);

    fclose(bin_file);

    destroy_all_buffer();
    return 0;
}
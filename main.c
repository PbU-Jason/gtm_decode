#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "match_pattern.h"
#include "shared.h"

// the code is designed for little endian computers (like x86_64) !!
int main(void){
    log_message("program start");

    size_t actual_binary_buffer_size = 0;
    size_t sd_header_location = 0;
    size_t old_sd_header_location = 0;
    size_t full = 0;
    size_t broken = 0;
    int packet_to_ignore = 0;
    int packet_counter = 0;

    check_endianness();

    //create all the global buffer and setup global variable
    create_all_buffer();
    sync_data_buffer_counter = SYNC_DATA_SIZE;
    event_buffer->pps_counter = INI_PPS_COUNTER;
    event_buffer->fine_counter = INI_FINE_COUNTER;
    got_first_sync_data = 0;

    bin_file = fopen("/home/ian/Documents/code/gtm/decode/Data_151222.368.bin", "rb");
    if (! bin_file){log_error("binary file not found");}
    log_message("fin loading bin file");

    out_file = fopen("/home/ian/Documents/code/gtm/decode/test_out.txt", "w");
    if (! out_file){log_error("can't open output file");}
    log_message("fin opening output file");

    actual_binary_buffer_size = fread(binary_buffer, 1, MAX_BINARY_BUFFER_SIZE, bin_file);
    while(actual_binary_buffer_size > SCIENCE_DATA_SIZE + SD_HEADER_SIZE){
        log_message("load new chunk");
        sd_header_location = 0;
        if (! is_sd_header(binary_buffer)){
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
                //log_message("start parsing full science packet");
                if (packet_counter > packet_to_ignore){
                    //printf("starting location = %i\n", sd_header_location + SD_HEADER_SIZE);
                    parse_full_science_packet(binary_buffer + sd_header_location + SD_HEADER_SIZE);
                }
                full++;
                packet_counter++;
                got_first_sync_data = 0;
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
    fclose(out_file);
    destroy_all_buffer();
    return 0;
}
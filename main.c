#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "match_pattern.h"
#include "argument_parser.h"

// the code is designed for little endian computers (like x86_64) !!

int main(int argc, char **argv){
    log_message("program start");

    set_argument(argc, argv);

    size_t actual_binary_buffer_size = 0;
    size_t sd_header_location = 0;
    size_t old_sd_header_location = 0;
    size_t full = 0;
    size_t broken = 0;
    int packet_to_ignore = 0;
    int packet_counter = 0;

    char temp_input[] = "/home/ian/Documents/code/gtm/decode/Data_151222.368.bin";
    char temp_output[] = "/home/ian/Documents/code/gtm/decode/test_out.txt";

    check_endianness();
    //create all the global buffer
    create_all_buffer();
    event_buffer->pps_counter = INI_PPS_COUNTER;
    event_buffer->fine_counter = INI_FINE_COUNTER;

    actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    //loop through buffer
    while(1){
        log_message("load new chunk");
        sd_header_location = 0;

        if (actual_binary_buffer_size <= SD_HEADER_SIZE){
            break;
        }
        if (! is_sd_header(binary_buffer)){
            log_error("bin file doesn't start with sd header");
        }
        //loop through packet in buffer
        while (sd_header_location < actual_binary_buffer_size){
            //if the packet is not continueous, reset related parameter
            if (! continuous_packet){
                got_first_sync_data = 0;
            }

            // find next sd header
            old_sd_header_location = sd_header_location;
            sd_header_location = find_next_sd_header(binary_buffer, sd_header_location, actual_binary_buffer_size);

            if (sd_header_location == -1){
                //no next sd header and this is not the last buffer, load next buffer, don't parse the packet
                if (actual_binary_buffer_size == max_binary_buffer_size){
                    break;
                }
                else{
                    //it's the last buffer but can't find next packet
                    if (old_sd_header_location + SD_HEADER_SIZE + SCIENCE_DATA_SIZE < actual_binary_buffer_size){
                        log_message("can't find next sd header while this isn't the last packet, discard data after");
                        break;
                    }
                    //it's the last buffer and this is the last packet
                    else{
                        sd_header_location = actual_binary_buffer_size;
                    }
                }
            }

            parse_sd_header(binary_buffer + sd_header_location);
            if (sd_header_location - old_sd_header_location == SCIENCE_DATA_SIZE + SD_HEADER_SIZE){ // full packet
                if (packet_counter > packet_to_ignore){
                    //printf("starting location = %i\n", sd_header_location + SD_HEADER_SIZE);
                    parse_science_packet(binary_buffer + old_sd_header_location + SD_HEADER_SIZE, SCIENCE_DATA_SIZE);
                }
                full++;
                packet_counter++;
            }
            else{
                //packet smaller than expected
                if (sd_header_location - old_sd_header_location < SCIENCE_DATA_SIZE + SD_HEADER_SIZE){
                    log_message("packet size smaller than expected");
                    parse_science_packet(binary_buffer + old_sd_header_location + SD_HEADER_SIZE, sd_header_location);
                }
                //if packet larger than expected, don't parse the packet
                else{
                    log_message("packet size larger than expected");
                }
                continuous_packet = 0;
                broken++;
            }
        }

        if (actual_binary_buffer_size < max_binary_buffer_size){
            break;
        }
        //offset position indicator of input file stream based on previous sd header position
        fseek(bin_file, sd_header_location - actual_binary_buffer_size, SEEK_CUR);
        actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    }
    printf("full = %zu, broken = %zu\n", full, broken);

    destroy_all_buffer();
    close_all_file();
    
    return 0;
}
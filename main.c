#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "match_pattern.h"

int main(void){
    log_message("program start");

    FILE* bin_file = NULL;
    size_t ideal_buffer_size = MAX_BUFFER_SIZE / 8;
    size_t actual_buffer_size = 0;
    size_t sd_header_location = 0;
    size_t old_sd_header_location = 0;
    unsigned char** buffer = NULL;
    size_t full = 0;
    size_t broken = 0;
    
    buffer = create_2D_arr(8, ideal_buffer_size);

    bin_file = fopen("Data_151222.368.bin", "rb");
    if (! bin_file){log_error("file not found");}
    log_message("fin loading bin file");

    actual_buffer_size = load_buffer(bin_file, buffer, ideal_buffer_size);
    while(actual_buffer_size > SCIENCE_DATA_SIZE + SD_header_SIZE){
        log_message("load new chunk");
        sd_header_location = 0;
        if (! is_sd_header(buffer[bit_shift])){
            //int i;
            //for (i=0;i<15;++i){
            //    printf("%02x", buffer[0][i]);
            //}
            //printf("\n");
            log_error("bin file doesn't start with sd header");
        }
        while (sd_header_location + SCIENCE_DATA_SIZE + SD_header_SIZE < actual_buffer_size){
            old_sd_header_location = sd_header_location;
            sd_header_location = find_next_sd_header(buffer, sd_header_location);
            if (sd_header_location - old_sd_header_location == SCIENCE_DATA_SIZE + SD_header_SIZE){
                full++;
                //printf("sd header location = %zu, bit shift = %u\n", sd_header_location, bit_shift);
            }
            else{
                broken++;
            }
        }

        //offset position indicator of input file stream based on previous sd header position
        fseek(bin_file, sd_header_location - actual_buffer_size, SEEK_CUR);
        actual_buffer_size = load_buffer(bin_file, buffer, ideal_buffer_size);
    }
    printf("full = %zu, broken = %zu\n", full, broken);

    fclose(bin_file);
    destroy_2D_arr(buffer, 8);
    return 0;
}
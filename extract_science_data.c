#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "extract_science_data.h"

void extract_science_data(void)
{
    unsigned char* nspo_data_buffer = NULL;
    unsigned char* pre_binary_buffer = NULL;
    int nspo_data_buffer_counter = 0;
    size_t actual_binary_buffer_size = 0;
    long long int location;

    nspo_data_buffer = (unsigned char*) malloc(NSPO_DATA_SIZE);
    if (! nspo_data_buffer){
        log_error("fail to create NSPO data buffer");
    }
    pre_binary_buffer = (unsigned char*) malloc(NSPO_EPOCH_HEADER_SIZE + NSPO_SYNC_MARKER_SIZE);
    if (! pre_binary_buffer){
        log_error("fail to create pre binary buffer");
    }

    actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    // loop through buffer
    while (1)
    {
        log_message("load new chunk");
        location = 0;
        while (location < actual_binary_buffer_size)
        {
            if (location >=0 ){
                memcpy(nspo_data_buffer + nspo_data_buffer_counter, binary_buffer + location, 1);
            }
            else if (location >= -(NSPO_EPOCH_HEADER_SIZE + NSPO_SYNC_MARKER_SIZE)){
                memcmp(nspo_data_buffer + nspo_data_buffer_counter, pre_binary_buffer + location + NSPO_EPOCH_HEADER_SIZE+NSPO_SYNC_MARKER_SIZE);
            }
            else{
                log_error("try to access outside pre_binary_buffer boundary");
            }
            nspo_data_buffer_counter++;

            if (nspo_data_buffer_counter == NSPO_EPOCH_HEADER_SIZE + NSPO_SYNC_MARKER_SIZE)
            {
                if (!is_nspo_header(nspo_data_buffer))
                { // is not nspo header
                    nspo_data_buffer_counter = 0;
                    location-=NSPO_EPOCH_HEADER_SIZE + NSPO_SYNC_MARKER_SIZE - 1;
                }
            }
            if (nspo_data_buffer_counter == NSPO_DATA_SIZE)
            { // all nspo data is loaded into buffer

                nspo_data_buffer_counter = 0;
            }

            location++;
        }

        if (actual_binary_buffer_size < max_binary_buffer_size)
        {
            break;
        }
        actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    }

    free(nspo_data_buffer);
    free(pre_binary_buffer);
}
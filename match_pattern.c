#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utility.h"
#include "match_pattern.h"

unsigned char* sync_data_buffer = NULL;
Event* event_buffer = NULL;
int sync_data_buffer_counter = 0;
int missing_sync_data = 0;
int got_first_sync_data = 0;

int is_sd_header(unsigned char* target){
    static unsigned char target_copy[SD_HEADER_SIZE];
    static unsigned char ref_master[SD_HEADER_SIZE]={0x88, 0x55, 0xC0, 0x00, 0x04, 0x4f};
    static unsigned char ref_slave[SD_HEADER_SIZE]={0x88, 0xAA, 0xC0, 0x00, 0x04, 0x4f};

    //mask the sequence count byte
    memcpy(target_copy, target, SD_HEADER_SIZE);
    target_copy[3] = 0x00;

    if (! memcmp(target_copy, ref_master, SD_HEADER_SIZE)){return 1;}
    if (! memcmp(target_copy, ref_slave, SD_HEADER_SIZE)){return 1;}
    return 0;
}

static int is_sync_header(unsigned char* target){
    static unsigned char ref = 0xCA;
    return (*target == ref);
}

static int is_sync_tail(unsigned char* target){
    static unsigned char ref[3] = {0xF2, 0xF5, 0xFA};
    //printf("%02x\n", *target);
    return (! memcmp(target, ref, 3));
}

static void parse_sync_data(unsigned char* target){
    unsigned char* buffer;

    buffer = (unsigned char*) malloc(2);
    if (! buffer){log_error("can't allocate buffer in parse_sync_data()");}

    //for now, only update pps count
    memcpy(buffer, target + 1, 2);
    buffer[0] = buffer[0] & 0x3F; //mask header and gtm module
    big2little_endian(buffer, 2);
    memcpy(&(event_buffer->pps_counter), buffer, 2);

    free(buffer);
}

static void print_event_buffer(void){
    printf("-----current event-----\n");
    printf("gtm module = %i\n", event_buffer->gtm_module);
    printf("pps counter = %u\n", event_buffer->pps_counter);
    printf("fine counter = %u\n", event_buffer->fine_counter);
    printf("citiroc id = %u\n", event_buffer->citiroc_id);
    printf("channel id = %u\n", event_buffer->channel_id);
    printf("energy filter = %u\n", event_buffer->energy_filter);
    printf("adc value = %u\n", event_buffer->adc_value);
    printf("------------------------\n");
}

static void write_event_time(void){
    fprintf(out_file, "%10u\n", event_buffer->fine_counter);
}

static void write_event_buffer(void){
    fprintf(out_file, "%5u;", event_buffer->pps_counter);
    fprintf(out_file, "%10u;", event_buffer->fine_counter);
    if (event_buffer->gtm_module){fprintf(out_file, "Slave;");}
    else{fprintf(out_file, "Master;");}
    if (event_buffer->citiroc_id){fprintf(out_file, "B;");}
    else{fprintf(out_file, "A;");}
    fprintf(out_file, "%3u;", event_buffer->channel_id);
    if (event_buffer->energy_filter){fprintf(out_file, "HG;");}
    else{fprintf(out_file, "LG;");}
    fprintf(out_file, "%5u\n", event_buffer->adc_value);
}

static void parse_event_data(unsigned char* target){
    static unsigned char ref_event_time = 0x80;
    static unsigned char ref_event_adc = 0x40;
    unsigned char* buffer = NULL;

    if ((*target & 0xC0) == ref_event_time){    //event time data
        buffer = (unsigned char*) malloc(4);
        if (! buffer){log_error("can't allocate buffer in parse_event_data()");}

        buffer[0] = 0x00;
        memcpy(&buffer[1], target, 3);
        buffer[1] = buffer[1] & 0x3F;   //mask the header
        big2little_endian(buffer, 4);
        memcpy(&(event_buffer->fine_counter), buffer, 4);
        free(buffer);
        write_event_time();
        //log_message("update event time");
        return;
    }

    if ((*target & 0xC0) == ref_event_adc){ //event adc data with 1 hit
        event_buffer->gtm_module = (*target & 0x20) ? SLAVE : MASTER;
        event_buffer->citiroc_id = (*target & 0x10) ? 1 : 0;
        event_buffer->energy_filter = (*(target + 1) & 0x20) ? 1 : 0;

        buffer = (unsigned char*) malloc(3);
        if (! buffer){log_error("can't allocate buffer in parse_event_data()");}

        //read channel id, it's spilt between bytes, maybe worth fixing that?
        memcpy(buffer, target, 3);
        left_shift_mem(buffer, 3, 4);
        buffer[0] = buffer[0] >> 3;
        memcpy(&(event_buffer->channel_id), buffer, 1);

        //read adc value
        memcpy(buffer, target + 1, 2);
        buffer[0] = buffer[0] & 0x3F; //mask channel id and energy filter
        big2little_endian(buffer, 2);
        memcpy(&(event_buffer->adc_value), buffer, 2);
        free(buffer);

        write_event_buffer();
        return;
    }
}

void unit_test(unsigned char* target){
    check_endianness();
    create_all_buffer();

    event_buffer->gtm_module = 0;
    event_buffer->pps_counter = 0;
    event_buffer->fine_counter = 0;
    event_buffer->citiroc_id = 0;
    event_buffer->channel_id = 0;
    event_buffer->energy_filter = 0;
    event_buffer->adc_value = 0;

    parse_sync_data(target);
    print_event_buffer();

    destroy_all_buffer();
}

int find_next_sd_header(unsigned char* buffer, size_t current_sd_header_location, size_t actual_buffer_size){
    size_t location, byte;
    location = current_sd_header_location + SCIENCE_DATA_SIZE + SD_HEADER_SIZE; // the size of sd header is 6

    if (location + SD_HEADER_SIZE > actual_buffer_size){
        location = actual_buffer_size - SD_HEADER_SIZE;
    }

    if (is_sd_header(buffer + location)){
        return location;
    }
    else{
        //look backward
        byte = location - current_sd_header_location ;
        while (byte--)
        {
            location--;
            if (is_sd_header(buffer + location)){
                return location;
            }
        }
        //if no next sd header is found
        log_error("no next sd header");
        return 0;
    }
}

void parse_full_science_packet(unsigned char* buffer){
    int i;
    unsigned char* current_location;

    //parse data based on word (3 bytes)
    for (i=0;i<SCIENCE_DATA_SIZE / 3;++i){
        current_location = buffer + 3 * i;

        // always look for sync data header
        if (is_sync_header(current_location)){
            missing_sync_data = 1;
            sync_data_buffer_counter = 0;
            memcpy(sync_data_buffer, current_location, 3);
            continue;
        }

        if (missing_sync_data){
            sync_data_buffer_counter += 3;
            memcpy(&sync_data_buffer[sync_data_buffer_counter], current_location, 3);
            //the tail of the sync data
            if (sync_data_buffer_counter == 42){
                if (is_sync_tail(sync_data_buffer + 42)){
                    missing_sync_data = 0;
                    got_first_sync_data = 1;
                    sync_data_buffer_counter += 3;

                    //log_message("update sync data");
                    parse_sync_data(sync_data_buffer);
                }
                //if the tail is missing, keep finding the next sync data header
                else{
                    missing_sync_data = 1;
                    sync_data_buffer_counter = 0;
                }
            }
        }
        else if (got_first_sync_data){
            parse_event_data(current_location);
        }
    }

}
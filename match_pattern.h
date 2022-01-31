#ifndef BIT_SHIFT_H
#define BIT_SHIFT_H

#define SCIENCE_DATA_SIZE 1104
#define SD_HEADER_SIZE 6
#define SYNC_DATA_SIZE 45
#define INI_PPS_COUNTER 0
#define INI_FINE_COUNTER 0

typedef enum{
    MASTER,
    SLAVE
} Gtm_module;

typedef struct Event
{
    Gtm_module gtm_module;
    uint16_t pps_counter;
    uint32_t fine_counter;
    uint8_t citiroc_id;
    uint8_t channel_id;
    uint8_t energy_filter;
    uint16_t adc_value;
} Event;

extern int sync_data_buffer_counter;
extern unsigned char* sync_data_buffer;
extern Event* event_buffer;
extern int missing_sync_data;
extern int got_first_sync_data;
extern int continuous_packet;

int is_sd_header(unsigned char* target);
void parse_sd_header(unsigned char* target);
int find_next_sd_header(unsigned char* buffer, size_t current_sd_header_location, size_t actual_buffer_size);
void parse_science_packet(unsigned char* buffer, size_t max_location);

void unit_test(unsigned char* target);
#endif
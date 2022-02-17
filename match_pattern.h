#ifndef BIT_SHIFT_H
#define BIT_SHIFT_H

#define SCIENCE_DATA_SIZE 1104
#define SD_HEADER_SIZE 6
#define SYNC_DATA_SIZE 45
#define TMTC_DATA_SIZE 128
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
    uint8_t cmd_seq_num;    //CMD-SAD sequence number
    uint32_t fine_counter;
    uint8_t citiroc_id;
    uint8_t channel_id;
    uint8_t energy_filter;
    uint16_t adc_value;
} Event;

typedef struct Tmtc
{
    Gtm_module gtm_module;
    uint16_t packet_counter;
    uint16_t pps_counter;
    uint16_t fine_counter;
    uint8_t board_temp1;
    uint8_t board_temp2;
    uint8_t citiroc1_temp1;
    uint8_t citiroc1_temp2;
    uint8_t citiroc2_temp1;
    uint8_t citiroc2_temp2;
    uint32_t citiroc1_livetime;
    uint32_t citiroc2_livetime;
    uint8_t citiroc1_hit[32];
    uint8_t citiroc2_hit[32];
} Tmtc;

extern int sync_data_buffer_counter;
extern unsigned char* sync_data_buffer;
extern int tmtc_data_buffer_counter;
extern unsigned char* tmtc_data_buffer;
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
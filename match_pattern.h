#ifndef BIT_SHIFT_H
#define BIT_SHIFT_H

#define NSPO_DATA_SIZE 1129
#define NSPO_EPOCH_HEADER_SIZE 10
#define NSPO_SYNC_MARKER_SIZE 4
#define NSPO_FRAME_HEADER_SIZE 6
#define NSPO_TRAILER_SIZE 4
#define SCIENCE_DATA_SIZE 1104
#define SD_HEADER_SIZE 6
#define SYNC_DATA_SIZE 45
#define TMTC_DATA_SIZE 128
#define INI_PPS_COUNTER 0
#define INI_FINE_COUNTER 0

#include <stdint.h>
#include <stdlib.h>

typedef enum GTM_module
{
    MASTER,
    SLAVE
} Gtm_module;

typedef struct Time
{
    // from UTC
    uint16_t year;
    uint8_t month;
    uint8_t mday; // this is the day in the month
    uint16_t day; // this is the day from 1/1 of the year
    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
    uint8_t sub_sec; // msec

    // from sync and event time
    uint32_t pps_counter; // it's our own pps counter
    uint32_t pps_counter_base;
    uint32_t fine_counter; // 0.24 usec
} Time;

typedef struct Position
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t x_velocity;
    uint32_t y_velocity;
    uint32_t z_velocity;
    uint16_t quaternion1;
    uint16_t quaternion2;
    uint16_t quaternion3;
    uint16_t quaternion4;
} Position;

typedef struct Event
{
    uint8_t if_hit;
    Gtm_module gtm_module;
    uint16_t pps_counter;
    uint8_t cmd_seq_num; // CMD-SAD sequence number
    uint32_t fine_counter;
    uint8_t citiroc_id;
    uint8_t channel_id;
    uint8_t energy_filter;
    uint16_t adc_value;
    double energy;
} Event;

typedef struct Tmtc
{
    unsigned char head[2];
    unsigned char tail[2];
    Gtm_module gtm_module;
    uint16_t packet_counter;
    uint16_t pps_counter;
    uint32_t fine_counter;
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
    uint16_t citiroc1_trigger;
    uint16_t citiroc2_trigger;
    uint8_t counter_period;
    uint8_t hv_dac1;
    uint8_t hv_dac2;
    uint8_t spw_a_error_count;
    uint8_t spw_b_error_count;
    uint8_t spw_a_last_receive;
    uint8_t spw_b_last_receive;
    uint16_t spw_a_status;
    uint16_t spw_b_status;
    uint8_t recv_checksum;
    uint8_t calc_checksum;
    uint8_t recv_num;
    uint16_t seu1;
    uint16_t seu2;
    uint16_t seu3;
    uint8_t checksum;
} Tmtc;

extern int sync_data_buffer_counter;
extern unsigned char *sync_data_buffer;
extern Time *time_buffer;
extern Time *time_start;
extern Position *position_buffer;
extern Position *pre_position;
extern Event *event_buffer;
extern Tmtc *tmtc_buffer;
extern int missing_sync_data;   // =1 after sync data with no tail
extern int got_first_sync_data; // =1 after parsing first sync data
extern int continuous_packet;

void parse_utc_time_tmtc(unsigned char *target);
void parse_utc_time_sync(unsigned char *target);
void parse_position(unsigned char *target);
int is_nspo_header(unsigned char *target);
int is_sd_header(unsigned char *target);
void parse_sd_header(unsigned char *target);
int find_next_sd_header(unsigned char *buffer, size_t current_sd_header_location, size_t actual_buffer_size);
void parse_science_packet(unsigned char *buffer, size_t max_location);
int is_tmtc_header(unsigned char *target);
int is_tmtc_tail(unsigned char *targrt);
void parse_tmtc_packet(unsigned char *target);

void unit_test(unsigned char *target);
#endif
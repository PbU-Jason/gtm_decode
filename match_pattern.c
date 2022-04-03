#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utility.h"
#include "match_pattern.h"
#include "energy_calibration.h"

// global variables
unsigned char *sync_data_buffer = NULL;
unsigned char *tmtc_data_buffer = NULL;
Time *time_buffer = NULL;
Time *time_start = NULL;
Position *position_buffer = NULL;
Position *pre_position = NULL;
Event *event_buffer = NULL;
Tmtc *tmtc_buffer;
int sync_data_buffer_counter = 0;
int missing_sync_data = 0;
int got_first_sync_data = 0;
int continuous_packet = 1;

// others
uint8_t sequence_count = 0;
int got_first_time_info = 0;

void parse_utc_time_tmtc(unsigned char *target)
{
    // year
    memcpy(&(time_buffer->year), target, 2);
    big2little_endian(&(time_buffer->year), 2);
    // day
    memcpy(&(time_buffer->day), target + 2, 2);
    big2little_endian(&(time_buffer->day), 2);
    // hour
    memcpy(&(time_buffer->hour), target + 4, 1);
    // minute
    memcpy(&(time_buffer->minute), target + 5, 1);
    // sec
    memcpy(&(time_buffer->sec), target + 6, 1);
    // sub sec (ms)
    memcpy(&(time_buffer->sub_sec), target + 7, 1);

    return;
}

// should be wrote later when the format is clear, it's a place holder now
void parse_utc_time_sync(unsigned char *target)
{
    // year
    time_buffer->year = 2022;
    // day
    memcpy(&(time_buffer->day), target, 2);
    big2little_endian(&(time_buffer->day), 2);
    // hour
    memcpy(&(time_buffer->hour), target + 2, 1);
    // minute
    memcpy(&(time_buffer->minute), target + 3, 1);
    // sec
    memcpy(&(time_buffer->sec), target + 4, 1);
    // sub sec (ms)
    memcpy(&(time_buffer->sub_sec), target + 5, 1);

    return;
}

void parse_position(unsigned char *target)
{
    memcpy(&(position_buffer->x), target, 4);
    memcpy(&(position_buffer->y), target + 4, 4);
    memcpy(&(position_buffer->z), target + 8, 4);
    memcpy(&(position_buffer->x_velocity), target + 12, 4);
    memcpy(&(position_buffer->y_velocity), target + 16, 4);
    memcpy(&(position_buffer->z_velocity), target + 20, 4);
    memcpy(&(position_buffer->quaternion1), target + 24, 2);
    memcpy(&(position_buffer->quaternion2), target + 26, 2);
    memcpy(&(position_buffer->quaternion3), target + 28, 2);
    memcpy(&(position_buffer->quaternion4), target + 30, 2);
}

// chech epoch + sync marker
int is_nspo_header(unsigned char *target)
{
    static unsigned char epoch_ref[6] = {0x44, 0x69, 0x00, 0x23, 0x62, 0x3B};
    static unsigned char sync_mark_ref[4] = {0x1A, 0xCF, 0xFC, 0x1D};

    return ((!memcmp(target, epoch_ref, 6)) && (!memcmp(target + 10, sync_mark_ref, 4)));
}

// since sd header's head has some prob in current test data, reduce some matching pattern
int is_sd_header(unsigned char *target)
{
    static unsigned char target_copy[SD_HEADER_SIZE];
    static unsigned char ref[2] = {0x88, 0x55};

    // mask the sequence count byte
    memcpy(target_copy, target, SD_HEADER_SIZE);
    target_copy[3] = 0x00;

    if (!memcmp(target_copy, ref, 2))
    {
        return 1;
    }
    return 0;
}

static void write_sd_header(uint8_t sequence_count)
{
    if (export_mode == 0 || export_mode == 2)
    {
        fprintf(out_file_raw, "sd header: %3u\n", sequence_count);
    }
}

void parse_sd_header(unsigned char *target)
{
    static int got_first_sd_header = 0;
    uint8_t new_sequence_count;
    // parse sequence count and check packet continuity
    memcpy(&new_sequence_count, target + 3, 1);

    write_sd_header(new_sequence_count);

    if (!got_first_sd_header)
    {
        sequence_count = new_sequence_count;
        got_first_sd_header = 1;
        return;
    }
    // make sure the sequence count is continuous
    if (sequence_count == 255)
    {
        continuous_packet = (new_sequence_count == 0);
    }
    else
    {
        continuous_packet = (new_sequence_count == sequence_count + 1);
    }

    if (!continuous_packet)
    {
        log_message("sequence count not continuous, old sequence count = %i, new seqence count= %i", sequence_count, new_sequence_count);
    }

    sequence_count = new_sequence_count;
}

static int is_sync_header(unsigned char *target)
{
    static unsigned char ref = 0xCA;
    return (*target == ref);
}

static int is_sync_tail(unsigned char *target)
{
    static unsigned char ref[3] = {0xF2, 0xF5, 0xFA};
    // printf("%02x\n", *target);
    return (!memcmp(target, ref, 3));
}

static void write_sync_data(void)
{
    if (export_mode == 0 || export_mode == 2)
    {
        fprintf(out_file_raw, "sync: %5u, %3u\n", event_buffer->pps_counter, event_buffer->cmd_seq_num);
        fprintf(out_file_raw_sync, "%1u;%5u;%3u;%5u;%3u;%3u;%3u;%3u;%10u;%10u;%10u;%10u;%10u;%10u;%5u;%5u;%5u;%5u\n", event_buffer->gtm_module, event_buffer->pps_counter, event_buffer->cmd_seq_num, time_buffer->day, time_buffer->hour, time_buffer->minute, time_buffer->sec, time_buffer->sub_sec, position_buffer->x, position_buffer->y, position_buffer->z, position_buffer->x_velocity, position_buffer->y_velocity, position_buffer->z_velocity, position_buffer->quaternion1, position_buffer->quaternion2, position_buffer->quaternion3, position_buffer->quaternion4);
    }

    if (export_mode == 1 || export_mode == 2)
    {
        if (!got_first_time_info)
        {
            get_month_and_mday();
            fprintf(out_file_pipeline, "start time UTC,%02i_%02i_%04i_%02i_%02i_%0.6f\n", time_buffer->mday, time_buffer->month, time_buffer->year, time_buffer->hour, time_buffer->minute, calc_sec(time_buffer));
            fprintf(out_file_pipeline_pos, "start time UTC,%2i_%2i_%4i_%2i_%2i_%.6f\n", time_buffer->mday, time_buffer->month, time_buffer->year, time_buffer->hour, time_buffer->minute, calc_sec(time_buffer));
            fprintf(out_file_pipeline, "time;detector;pixel;energy\n");          // header
            fprintf(out_file_pipeline_pos, "time;qw;qx;qy;qz;ECIx;ECIy;ECIz\n"); // header
            memcpy(time_start, time_buffer, sizeof(Time));
            got_first_time_info = 1;
        }
        // if there is new position info, write and update pre_position
        if (memcmp(pre_position, position_buffer, sizeof(Position)) != 0)
        {
            fprintf(out_file_pipeline_pos, "%f;%5i;%5i;%5i;%5i;%10i;%10i;%10i\n", find_time_delta(time_start, time_buffer), position_buffer->quaternion1, position_buffer->quaternion2, position_buffer->quaternion3, position_buffer->quaternion4, position_buffer->x, position_buffer->y, position_buffer->z);
            memcpy(pre_position, position_buffer, sizeof(Position));
        }
    }
}

static void parse_sync_data(unsigned char *target)
{
    unsigned char buffer[2];
    Time time_before;

    // pps count
    memcpy(buffer, target + 1, 2);
    buffer[0] = buffer[0] & 0x3F; // mask header and gtm module
    big2little_endian(buffer, 2);
    memcpy(&(event_buffer->pps_counter), buffer, 2);
    // CMD-SAD sequence number
    memcpy(&(event_buffer->cmd_seq_num), target + 3, 1);
    // UTC
    memcpy(&time_before, time_buffer, sizeof(Time));
    parse_utc_time_sync(target + 4);
    // log_message("day %u, hour %u, min %u, sec %f", time_buffer->day, time_buffer->hour, time_buffer->minute, time_buffer->sec);
    //  ECI position stuff
    parse_position(target + 10);

    time_buffer->pps_counter++;
    time_buffer->fine_counter = 0;
    // if UTC update, reset out own pps
    if (!memcmp(&time_before, time_buffer, 80)) // only compare the UTC part
    {
        time_buffer->pps_counter = 0;
    }

    write_sync_data();
}

static void print_event_buffer(void)
{
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

static void write_event_time(void)
{
    if (export_mode == 0 || export_mode == 2)
    {
        fprintf(out_file_raw, "event time: %10u\n", event_buffer->fine_counter);
    }
}

static void write_event_buffer(void)
{
    int detector, pixel;
    if (export_mode == 0 || export_mode == 2)
    {
        fprintf(out_file_raw, "event adc: ");
        fprintf(out_file_raw, "%1u;%5u;%10u;%1u;%1u;%3u;%1u;%5u\n", event_buffer->if_hit, event_buffer->pps_counter, event_buffer->fine_counter, event_buffer->gtm_module, event_buffer->citiroc_id, event_buffer->channel_id, event_buffer->energy_filter, event_buffer->adc_value);
    }
    if (export_mode == 1 || export_mode == 2)
    {
        detector = ((int)event_buffer->gtm_module + 1) * ((int)event_buffer->citiroc_id + 1) + (int)(event_buffer->channel_id / 16);
        pixel = (int)event_buffer->channel_id % 16 + 1;
        fprintf(out_file_pipeline, "%0.6f;%i;%i;%f\n", find_time_delta(time_start, time_buffer), detector, pixel, event_buffer->energy);
    }
}

static void parse_event_adc(unsigned char *target)
{
    unsigned char *buffer = NULL;

    event_buffer->if_hit = ((*target & 0x40) == 0x40);
    event_buffer->gtm_module = (*target & 0x20) ? SLAVE : MASTER;
    event_buffer->citiroc_id = (*target & 0x10) ? 1 : 0;
    event_buffer->energy_filter = (*(target + 1) & 0x20) ? 1 : 0;

    buffer = (unsigned char *)malloc(3);
    if (!buffer)
    {
        log_error("can't allocate buffer in parse_event_data()");
    }

    // read channel id, it's spilt between bytes, maybe worth fixing that?
    memcpy(buffer, target, 3);
    left_shift_mem(buffer, 3, 4);
    buffer[0] = buffer[0] >> 3;
    memcpy(&(event_buffer->channel_id), buffer, 1);

    // read adc value
    memcpy(buffer, target + 1, 2);
    buffer[0] = buffer[0] & 0x3F; // mask channel id and energy filter
    big2little_endian(buffer, 2);
    memcpy(&(event_buffer->adc_value), buffer, 2);
    update_energy_from_adc();
    free(buffer);

    write_event_buffer();
    return;
}
static void parse_event_data(unsigned char *target)
{
    unsigned char *buffer = NULL;
    uint32_t old_fine_counter;

    if ((*target & 0xC0) == 0x80)
    { // event time data
        buffer = (unsigned char *)malloc(4);
        if (!buffer)
        {
            log_error("can't allocate buffer in parse_event_data()");
        }

        buffer[0] = 0x00;
        memcpy(&buffer[1], target, 3);
        buffer[1] = buffer[1] & 0x3F; // mask the header
        big2little_endian(buffer, 4);
        old_fine_counter = event_buffer->fine_counter;
        memcpy(&(event_buffer->fine_counter), buffer, 4);
        memcpy(&(time_buffer->fine_counter), buffer, 2);
        free(buffer);

        write_event_time();
        // log_message("update event time");
        return;
    }

    if (exclude_nohit)
    {
        if ((*target & 0xC0) == 0x40)
        {
            parse_event_adc(target);
        }
    }
    else
    {
        if ((*target & 0x80) == 0x00)
        {
            parse_event_adc(target);
        }
    }
}

int find_next_sd_header(unsigned char *buffer, size_t current_sd_header_location, size_t actual_buffer_size)
{
    size_t location;

    // quick check
    location = current_sd_header_location + SCIENCE_DATA_SIZE + SD_HEADER_SIZE;
    if (location <= actual_buffer_size - SD_HEADER_SIZE)
    {
        if (is_sd_header(buffer + location))
        {
            return location;
        }
    }

    for (location = current_sd_header_location + SD_HEADER_SIZE; location <= actual_buffer_size - SD_HEADER_SIZE; location++)
    {
        if (is_sd_header(buffer + location))
        {
            return location;
        }
    }

    // no next sd header is found
    return -1;
}

void parse_science_packet(unsigned char *buffer, size_t max_location)
{
    int i;
    unsigned char *current_location;

    // parse data based on word (3 bytes)
    for (i = 0; i < max_location / 3; ++i)
    {
        current_location = buffer + 3 * i;

        // always look for sync data header
        if (is_sync_header(current_location))
        {
            missing_sync_data = 1;
            sync_data_buffer_counter = 0;
            memcpy(sync_data_buffer, current_location, 3);
            continue;
        }

        if (missing_sync_data)
        {
            sync_data_buffer_counter += 3;
            memcpy(&sync_data_buffer[sync_data_buffer_counter], current_location, 3);
            // the tail of the sync data
            if (sync_data_buffer_counter == 42)
            {
                if (is_sync_tail(sync_data_buffer + 42))
                {
                    missing_sync_data = 0;
                    got_first_sync_data = 1;
                    sync_data_buffer_counter += 3;

                    // log_message("update sync data");
                    parse_sync_data(sync_data_buffer);
                }
                // if the tail is missing, keep finding the next sync data header
                else
                {
                    missing_sync_data = 1;
                    sync_data_buffer_counter = 0;
                }
            }
        }
        else if (got_first_sync_data)
        {
            parse_event_data(current_location);
        }
    }
}

int is_tmtc_header(unsigned char *target)
{
    static unsigned char ref[2] = {0x55, 0xAA};

    if (!memcmp(ref, target, 2))
    {
        return 1;
    }
    return 0;
}

int is_tmtc_tail(unsigned char *targrt)
{
    static unsigned char ref[2] = {0xFB, 0xF2};

    if (!memcmp(ref, targrt, 2))
    {
        return 1;
    }
    return 0;
}

void write_tmtc_buffer(void)
{
    int i;

    fprintf(out_file_raw, "%0X%0X", tmtc_buffer->head[0], tmtc_buffer->head[1]); // head
    fprintf(out_file_raw, ";%3u;%5u;%5u;%5u;%3u;%3u;%.3f;%5u;%10u;%3u;%3u;%3u;%3u;%3u;%3u;%10u;%10u", tmtc_buffer->gtm_module, tmtc_buffer->packet_counter, time_buffer->year, time_buffer->day, time_buffer->hour, time_buffer->minute, time_buffer->sec + time_buffer->sub_sec * 0.001, tmtc_buffer->pps_counter, tmtc_buffer->fine_counter, tmtc_buffer->board_temp1, tmtc_buffer->board_temp2, tmtc_buffer->citiroc1_temp1, tmtc_buffer->citiroc1_temp2, tmtc_buffer->citiroc2_temp1, tmtc_buffer->citiroc2_temp2, tmtc_buffer->citiroc1_livetime, tmtc_buffer->citiroc2_livetime);
    for (i = 0; i < 32; ++i)
    {
        fprintf(out_file_raw, ";%3u", tmtc_buffer->citiroc1_hit[i]);
    }
    for (i = 0; i < 32; ++i)
    {
        fprintf(out_file_raw, ";%3u", tmtc_buffer->citiroc2_hit[i]);
    }
    fprintf(out_file_raw, ";%5u;%5u;%3u;%3u;%3u;%3u;%3u;%3u;%3u;%5u;%5u;%3u;%3u;%3u;%5u;%5u;%5u;%3u", tmtc_buffer->citiroc1_trigger, tmtc_buffer->citiroc2_trigger, tmtc_buffer->counter_period, tmtc_buffer->hv_dac1, tmtc_buffer->hv_dac2, tmtc_buffer->spw_a_error_count, tmtc_buffer->spw_b_error_count, tmtc_buffer->spw_a_last_receive, tmtc_buffer->spw_b_last_receive, tmtc_buffer->spw_a_status, tmtc_buffer->spw_b_status, tmtc_buffer->recv_checksum, tmtc_buffer->calc_checksum, tmtc_buffer->recv_num, tmtc_buffer->seu1, tmtc_buffer->seu2, tmtc_buffer->seu3, tmtc_buffer->checksum);
    fprintf(out_file_raw, ";%0X%0X\n", tmtc_buffer->tail[0], tmtc_buffer->tail[1]); // tail
}

void parse_tmtc_packet(unsigned char *target)
{
    int i;
    unsigned char temp2[2], temp3[3];

    // header
    memcpy(tmtc_buffer->head, target, 2);
    // GTM module
    tmtc_buffer->gtm_module = (*(target + 2) == 0x02) ? 0 : 1;
    // packet counter
    memcpy(&(tmtc_buffer->packet_counter), target + 3, 2);
    big2little_endian(&(tmtc_buffer->packet_counter), 2);
    // UTC
    parse_utc_time_tmtc(target + 7);
    // pps_counter
    *(target + 15) = *(target + 15) & 0x7F; // mask GTM module bit
    memcpy(&(tmtc_buffer->pps_counter), target + 15, 2);
    big2little_endian(&(tmtc_buffer->pps_counter), 2);
    // fine counter
    memcpy(&(tmtc_buffer->fine_counter), target + 17, 3);
    big2little_endian(&(tmtc_buffer->fine_counter), 3);
    // board temp
    memcpy(&(tmtc_buffer->board_temp1), target + 20, 1);
    memcpy(&(tmtc_buffer->board_temp2), target + 21, 1);
    // citiroc temp
    memcpy(&(tmtc_buffer->citiroc1_temp1), target + 22, 1);
    memcpy(&(tmtc_buffer->citiroc1_temp2), target + 23, 1);
    memcpy(&(tmtc_buffer->citiroc2_temp1), target + 24, 1);
    memcpy(&(tmtc_buffer->citiroc2_temp2), target + 25, 1);
    // citiroc livetime
    memcpy(&(tmtc_buffer->citiroc1_livetime), target + 26, 3);
    big2little_endian(&(tmtc_buffer->citiroc1_livetime), 3);
    memcpy(&(tmtc_buffer->citiroc2_livetime), target + 29, 3);
    big2little_endian(&(tmtc_buffer->citiroc2_livetime), 3);
    // citiroc hit
    for (i = 0; i < 32; ++i)
    {
        memcpy(&(tmtc_buffer->citiroc1_hit[i]), target + 32 + i, 1);
        memcpy(&(tmtc_buffer->citiroc2_hit[i]), target + 64 + i, 1);
    }
    // citiroc trigger
    memcpy(&(tmtc_buffer->citiroc1_trigger), target + 96, 2);
    big2little_endian(&(tmtc_buffer->citiroc1_trigger), 2);
    memcpy(&(tmtc_buffer->citiroc2_trigger), target + 98, 2);
    big2little_endian(&(tmtc_buffer->citiroc2_trigger), 2);
    // counter period
    memcpy(&(tmtc_buffer->counter_period), target + 100, 1);
    // hv dac
    memcpy(&(tmtc_buffer->hv_dac1), target + 101, 1);
    memcpy(&(tmtc_buffer->hv_dac2), target + 102, 1);
    // spw stuff
    memcpy(&(tmtc_buffer->spw_a_error_count), target + 103, 1);
    memcpy(&(tmtc_buffer->spw_a_last_receive), target + 104, 1);
    memcpy(&(tmtc_buffer->spw_b_error_count), target + 105, 1);
    memcpy(&(tmtc_buffer->spw_b_last_receive), target + 106, 1);
    memcpy(&(tmtc_buffer->spw_a_status), target + 107, 2);
    big2little_endian(&(tmtc_buffer->spw_a_status), 2);
    memcpy(&(tmtc_buffer->spw_b_status), target + 109, 2);
    big2little_endian(&(tmtc_buffer->spw_b_status), 2);
    // checksum
    memcpy(&(tmtc_buffer->recv_checksum), target + 111, 1);
    memcpy(&(tmtc_buffer->calc_checksum), target + 112, 1);
    // recv num
    memcpy(&(tmtc_buffer->recv_num), target + 113, 1);
    // seu measurement
    memcpy(&(tmtc_buffer->seu1), target + 119, 2);
    big2little_endian(&(tmtc_buffer->seu1), 2);
    memcpy(&(tmtc_buffer->seu2), target + 121, 2);
    big2little_endian(&(tmtc_buffer->seu2), 2);
    memcpy(&(tmtc_buffer->seu3), target + 123, 2);
    big2little_endian(&(tmtc_buffer->seu3), 2);
    // checksum
    memcpy(&(tmtc_buffer->checksum), target + 125, 1);
    // tail
    memcpy(tmtc_buffer->tail, target + 126, 2);

    write_tmtc_buffer();
}
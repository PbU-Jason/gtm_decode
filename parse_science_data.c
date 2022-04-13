#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "match_pattern.h"
#include "argument_parser.h"
#include "crc_check.h"

void parse_science_data(void)
{
    size_t actual_binary_buffer_size = 0;
    size_t sd_header_location = 0;
    size_t old_sd_header_location = 0;
    size_t full = 0;
    size_t broken = 0;
    uint8_t CRC_next_packet, CRC_calculate;

    event_buffer->pps_counter = INI_PPS_COUNTER;
    event_buffer->fine_counter = INI_FINE_COUNTER;

    actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    // find first sd header
    sd_header_location = find_next_sd_header(binary_buffer, -SD_HEADER_SIZE, actual_binary_buffer_size);
    if (sd_header_location != 0)
    {
        log_message("Binary file doesn't start with science data header, first science data header is at byte %zu", (size_t)ftell(bin_file) - actual_binary_buffer_size + sd_header_location);
        fseek(bin_file, sd_header_location - actual_binary_buffer_size, SEEK_CUR);
        actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    }

    // loop through buffer
    while (1)
    {
        log_message("load new chunk");
        sd_header_location = 0;

        if (actual_binary_buffer_size <= SD_HEADER_SIZE)
        {
            break;
        }
        if (!is_sd_header(binary_buffer))
        {
            log_error("Bin file doesn't start with sd header");
        }
        // loop through packet in buffer
        while (sd_header_location < actual_binary_buffer_size)
        {
            // if the packet is not continueous, reset related parameter
            if (!continuous_packet)
            {
                log_message("Non contiuous occurs around bytes %zu", (size_t)ftell(bin_file) - actual_binary_buffer_size + sd_header_location);
                got_first_sync_data = 0;
            }

            // find next sd header
            old_sd_header_location = sd_header_location;
            sd_header_location = find_next_sd_header(binary_buffer, sd_header_location, actual_binary_buffer_size);

            if (sd_header_location == -1) // no next sd header is found
            {
                // no next sd header and this is not the last buffer, load next buffer, don't parse the packet
                if (actual_binary_buffer_size == max_binary_buffer_size)
                {
                    break;
                }
                else
                {
                    // it's the last buffer but can't find next packet
                    if (old_sd_header_location + SD_HEADER_SIZE + SCIENCE_DATA_SIZE < actual_binary_buffer_size)
                    {
                        log_message("Can't find next sd header while this isn't the last packet, discard data after");
                        break;
                    }
                    // it's the last buffer and this is the last packet
                    else
                    {
                        parse_science_packet(binary_buffer + old_sd_header_location + SD_HEADER_SIZE, actual_binary_buffer_size - old_sd_header_location - SD_HEADER_SIZE);
                        full++;
                        break;
                    }
                }
            }
            // parse and check the sequence count
            parse_sd_header(binary_buffer + sd_header_location);
            if (sd_header_location - old_sd_header_location == SCIENCE_DATA_SIZE + SD_HEADER_SIZE)
            { // full packet

                // check CRC byte
                memcpy(&CRC_next_packet, binary_buffer + sd_header_location + 2, 1);
                CRC_calculate = calc_CRC_8_ATM(binary_buffer + old_sd_header_location, SCIENCE_DATA_SIZE + SD_HEADER_SIZE);
                if (CRC_next_packet != CRC_calculate)
                {
                    log_message("Wrong CRC, calculate value: %02X, value from next packet: %02X", CRC_calculate, CRC_next_packet);
                    continuous_packet = 0;
                }

                parse_science_packet(binary_buffer + old_sd_header_location + SD_HEADER_SIZE, SCIENCE_DATA_SIZE);
                full++;
            }
            else
            {
                // packet smaller than expected
                if (sd_header_location - old_sd_header_location < SCIENCE_DATA_SIZE + SD_HEADER_SIZE)
                {
                    log_message("Packet size %zu bytes smaller than expected", sd_header_location - old_sd_header_location);
                    parse_science_packet(binary_buffer + old_sd_header_location + SD_HEADER_SIZE, sd_header_location - old_sd_header_location - SD_HEADER_SIZE);
                }
                // if packet larger than expected, don't parse the packet
                else
                {
                    log_message("Packet size %zu bytes larger than expected", sd_header_location - old_sd_header_location);
                }
                continuous_packet = 0;
                broken++;
            }
        }

        if (actual_binary_buffer_size < max_binary_buffer_size)
        {
            break;
        }
        // offset position indicator of input file stream based on previous sd header position
        fseek(bin_file, old_sd_header_location - actual_binary_buffer_size, SEEK_CUR);
        actual_binary_buffer_size = fread(binary_buffer, 1, max_binary_buffer_size, bin_file);
    }
    log_message("packet summary: full = %zu, broken = %zu", full, broken);
}
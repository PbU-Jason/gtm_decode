#include "utility.h"
#include "argument_parser.h"
#include "parse_science_data.h"
#include "parse_tmtc_data.h"
#include "extract_science_data.h"

char welcome_str[]="GTM decoder -- decode GTM binary file to human readable data\ntips:use --help flag to see how to use\n";

// the code is designed for little endian computers (like x86_64) !!
int main(int argc, char **argv)
{
    puts(welcome_str);
    set_argument(argc, argv);
    log_message("program start");

    check_endianness();
    // create all the global buffer
    create_all_buffer();

    switch (decode_mode)
    {
    case 0:
        log_message("start decoding science data");
        parse_science_data();
        break;
    case 1:
        log_message("start decoding telemetry data");
        parse_tmtc_data();
        break;
    case 2:
        log_message("start extracting science data");
        extract_science_data();
        break;
    default:
        log_error("unknown decode mode");
        break;
    }

    close_all_file();
    destroy_all_buffer();

    log_message("program finished");
    return 0;
}
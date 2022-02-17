#include "utility.h"
#include "argument_parser.h"
#include "parse_science_data.h"

// the code is designed for little endian computers (like x86_64) !!

int main(int argc, char **argv){
    log_message("program start");

    set_argument(argc, argv);

    check_endianness();
    //create all the global buffer
    create_all_buffer();

    switch (decode_mode)
    {
    case 0:
        log_message("start decoding science data");
        parse_science_data();
        break;
    
    default:
        log_error("unknown decode mode");
        break;
    }

    destroy_all_buffer();
    close_all_file();
    
    log_message("progarm finished");
    return 0;
}
#include <argp.h>
#include "utility.h"
#include "argument_parser.h"

char* input_file_path = NULL;
char* output_file_path = NULL;
char doc[] = 
    "GTM decoder -- decode GTM binary file to human readable data\nnote: this is a testing version!!";
char args_doc[] = "use --help flag to see more detail";
char version_str[] = "testing 1.0\n";

static struct argp_option options[] = {
  {"input", 'i', "FILE", 0, "The input binary file" },
  {"output", 'o', "FILE", 0, "The output file" },
  {"decode-mode", 'm', "Num", 0, "the decode mode, 0 = decode science data, 1 = decode telemetry data"},
  {"buffer-size", 'b', "Bytes", 0, "The max buffer size while loading the binary file. The defalt size is 1 GB" },
  {"terminal-out", 't', NULL, OPTION_ARG_OPTIONAL , "deocder will ignore output file and dump all the results into terminal" },
  {"silent",'s', NULL, OPTION_ARG_OPTIONAL ,"no log and error message"},
  {"version",'v', NULL, OPTION_ARG_OPTIONAL ,"show program version"},
  {0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state){
  switch (key)
    {
    case 'i':
      input_file_path = arg;
      break;
    case 'o':
      output_file_path = arg;
      break;
    case 'm':
      if (! sscanf(arg, "%u", &decode_mode)){
        log_error("can't parse decode mode");
      }
      break;
    case 't':
      log_message("toggle terminal output");
      terminal_out = 1;
      break;
    case 'b':
      if (! sscanf(arg, "%zu", &max_binary_buffer_size)){
          log_error("can't parse buffer-size");
      }
      break;
    case 's':
      debug_output = 0;
      break;
    case 'v':
      fputs(version_str, stdout);
      exit(0);
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void set_argument(int argc, char **argv){
    argp_parse(&argp, argc, argv, 0, 0, NULL);
    open_all_file(input_file_path, output_file_path);
}
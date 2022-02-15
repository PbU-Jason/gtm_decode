#include <argp.h>
#include "utility.h"
#include "argument_parser.h"

char* input_file_path = NULL;
char* output_file_path = NULL;
char doc[] = 
    "GTM decoder -- decode GTM binary file to human readable data\nnote: this is a testing version!!";
char args_doc[] = "use --help flag to see more detail";

static struct argp_option options[] = {
  {"input", 'i', "FILE", 0, "The input binary file" },
  {"output", 'o', "FILE", 0, "The output file" },
  {"buffer-size", 'b', "Bytes", OPTION_ARG_OPTIONAL , "The max buffer size while loading the binary file. The defalt size is xxx bytes" },
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
    case 'b':
      if (! sscanf(arg, "%zu", &max_binary_buffer_size)){
          log_error("can't parse buffer-size");
      }
      break;
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
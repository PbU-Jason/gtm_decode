#include <argp.h>
#include "utility.h"
#include "argument_parser.h"

char *input_file_path = NULL;
char *output_file_path = NULL;
char doc[] =
    "GTM decoder -- decode GTM binary file to human readable data";
char args_doc[] = "use --help flag to see more detail";
char version_str[] = "20220403\n\0";

static struct argp_option options[] = {
    {"input", 'i', "FILE", 0, "Required!!, The input binary file"},
    {"output", 'o', "FILE", 0, "Required!!, The output filename prefix"},
    {"decode-mode", 'm', "Num", 0, "Required!!, the decode mode, 0 = decode science data, 1 = decode telemetry data, 2 = extract science data"},
    {"export-mode", 'e', "Num", 0, "the export mode, for deocde mode 0 only. 0 = output raw format, 1 = output pipeline format, 2 = output both, default 0"},
    {"buffer-size", 'b', "Bytes", 0, "The max buffer size while loading the binary file. The defalt size is 1 GB"},
    {"terminal-out", 't', NULL, OPTION_ARG_OPTIONAL, "deocder will ignore output file and dump all the results into terminal"},
    {"silent", 's', NULL, OPTION_ARG_OPTIONAL, "no log and error message"},
    {"get-nohit-event", 128, NULL, OPTION_ARG_OPTIONAL, "output zero hit event adc, only affect science data(decode mode 0) raw output"},
    {"version", 'v', NULL, OPTION_ARG_OPTIONAL, "show program version"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  switch (key)
  {
  case 'i':
    input_file_path = arg;
    break;
  case 'o':
    output_file_path = arg;
    break;
  case 'm':
    if (!sscanf(arg, "%u", &decode_mode))
    {
      log_error("can't parse decode mode");
    }
    break;
  case 'e':
    if (!sscanf(arg, "%u", &export_mode))
    {
      log_error("can't parse export mode");
    }
    break;
  case 't':
    log_message("toggle terminal output");
    terminal_out = 1;
    break;
  case 'b':
    if (!sscanf(arg, "%zu", &max_binary_buffer_size))
    {
      log_error("can't parse buffer-size");
    }
    break;
  case 's':
    debug_output = 0;
    break;
  case 128:
    exclude_nohit = 0;
    break;
  case 'v':
    fputs(version_str, stdout);
    exit(0);
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void set_argument(int argc, char **argv)
{
  argp_parse(&argp, argc, argv, 0, 0, NULL);

  // make a small summary about the execution
  log_message("execution summary------------------");
  printf("  GTM decodeder version: %s", version_str);
  switch (decode_mode)
  {
  case 0:
    puts("  decode mode set to 0, decoding science data");
    switch (export_mode)
    {
    case 0:
      puts("  export mode set to 0, export raw format only");
      break;
    case 1:
      puts("  export mode set to 1, export pipeline format only");
      break;
    case 2:
      puts("  export mode set to 2, export both raw and pipeline format");
      break;
    default:
      log_error("unknown export mode");
    }
    break;
  case 1:
    puts("  decode mode set to 1, decoding telemetry data");
    break;
  case 2:
    puts("  decode mode set to 2, extracting science data");
    break;
  default:
    log_error("unknown decode mode");
    break;
  }
  printf("  input binary file: %s\n", input_file_path);
  puts("--------------------------------------------");

  // modified exort mode based on decode mode
  if (decode_mode == 1 || decode_mode == 2)
  { // no pipeline output for tmtc and nspo data
    export_mode = 0;
  }
  open_all_file(input_file_path, output_file_path);
}
# gtm_decode
a WIP gtm decoder
README version: 20220329
## Compile from source
### Linux
Install `git` and `gcc` from system's package manager. This program also use `argp`, which morden linux installed by default.

```
git clone git@github.com:iiiian/gtm_decode.git
cd gtm_decode
gcc *.c -o gtm_decoder -O3 
```
### Windows
first install cygwin 64, tick the devel package during the install (Remember to add C:\cygwin64\bin to your path). Then clone the respository. if you have already install git, open the command prompt.
```
git clone git@github.com:iiiian/gtm_decode.git
```
Or you can go to the respository and download the zip file.
In the command prompt, goes to the folder gtm_docode, then link `argp` library and compile
```
gcc *.c -o gtm_decoder -IC:\cygwin64\usr\include -LC:\cygwin64\usr\lib -largp -O3
```
## Use
```
GTM decoder -- decode GTM binary file to human readable data
tips:use --help flag to see how to use

Usage: gtm_decoder [OPTION...] use --help flag to see more detail
GTM decoder -- decode GTM binary file to human readable data

  -b, --buffer-size=Bytes    The max buffer size while loading the binary file.
                             The defalt size is 1 GB
  -e, --export-mode=Num      the export mode, for deocde mode 0 only. 0 =
                             output raw format, 1 = output pipeline format, 2 =
                             output both, default 0
  -i, --input=FILE           Required!!, The input binary file
  -m, --decode-mode=Num      Required!!, the decode mode, 0 = decode science
                             data, 1 = decode telemetry data, 2 = extract
                             science data
  -o, --output=FILE          Required!!, The output filename prefix
  -s, --silent               no log and error message
  -t, --terminal-out         deocder will ignore output file and dump all the
                             results into terminal
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -v, --version              show program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

```

Notice that input file, output file and decode mode is required.

## Output file
### Decode mode 0
depends on export mode, there might be pipeline_scienceraw.txt, pipeline_sciencepipeline.txt and pipeline_sciencepipeline_pos.txt
### Docde mode 1
prefix_tmtc.csv
### Docde mode 2
prefix_extracted.bin
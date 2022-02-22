# gtm_decode
a WIP gtm decoder
## Compile from source
### Linux
make sure your system have `gcc` installed. This program also use `argp`, which morden linux installed by default.

```
git clone git@github.com:iiiian/gtm_decode.git
cd gtm_decode
gcc *.c -o gtm_decoder -O3 
```
## Use
```
Usage: gtm_decoder [OPTION...] use --help flag to see more detail
GTM decoder -- decode GTM binary file to human readable data
note: this is a testing version!!

  -b, --buffer-size[=Bytes]  The max buffer size while loading the binary file.
                             The defalt size is 1 GB
  -i, --input=FILE           The input binary file
  -m, --decode-mode=Num      the decode mode, 0 = decode science data, 1 =
                             decode telemetry data
  -o, --output=FILE          The output file
  -s, --silent               no log and error message
  -t, --terminal-out         deocder will ignore output file and dump all the
                             results into terminal
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

```
## Output file format
### Science data
```
sd header: [sequence number]
sync: [pps count] [cmd sequence number]
event time: [fine count]
event adc: [pps count] [fine count] [gtm module] [citiroc id] [channel id] [gain] [adc value]
```
gtm module: 0=master, 1=slave
citiroc id: 0=a, 1=b
gain: 0=LG, 1=HG
### Telemetry data
```
[gtm module] [Packet Counter] [Lastest PPS Counter] [Lastest Fine Time Counter Value Between 2 PPS] [Board Temperature#1] [Board Temperature#2] [CITIROC1 Temperature#1] [CITIROC1 Temperature#2] [CITIROC2 Temperature#1] [CITIROC2 Temperature#2] [CITIROC1 Live time] [CITIROC2 Live time] [CITIROC1 Hit Counter#0 ] ~ [CITIROC1 Hit Counter#31 ] [CITIROC2 Hit Counter#0 ] ~ [CITIROC2 Hit Counter#31 ] [CITIROC1 Trigger counter] [CITIROC2 Trigger counter] [Counter period Setting] [HV DAC1] [HV DAC2] [SPW#A Error count] [SPW#B Error count] [SPW#A Last Recv Byte] [SPW#A Last Recv Byte] [SPW#A status] [SPW#B status] [Recv Checksum of Last CMD] [Calc Checksum of Last CMD] [Number of Recv CMDs] [SEU-Measurement#1] [SEU-Measurement#2] [SEU-Measurement#3] [checksum]
```







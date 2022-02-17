# gtm_decode
a WIP gtm decoder
## Compile from source
```
git clone git@github.com:iiiian/gtm_decode.git
cd gtm_decode
gcc *.c -o gtm_decoder -O3 
```
## Use
```
gtm_decoder --help
```
## Output file format
```
sd header: [sequence number]
sync: [pps count] [cmd sequence number]
event time: [fine count]
event adc: [pps count] [fine count] [gtm module] [citiroc id] [channel id] [gain] [adc value]
```
gtm module: 0=master, 1=slave
citiroc id: 0=a, 1=b
gain: 0=LG, 1=HG







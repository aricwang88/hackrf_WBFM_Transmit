# hackrf_WBFM_Transmit
This is simple WBFM Transmit code that used to verify your environment correctly installed.

# Describtion
This is a mini code, in a single C file and work only one thread. It can parse wav file and generate IQ data for hackrf one, then send data to hackrf one.
The default frequency is 100.4MHz.
#Prepare media source
---Option 1: Download or clone the test wave file from github.
```
git clone https://github.com/aricwang88/hackrf_example_res.git
```
---Option 2: Install ffmpeg or libav-tools to convert to wave file.
Search in repository:
```
apt-cache search libav-tools
libav-tools - Multimedia player, server, encoder and transcoder

apt-get install libav-tools

avconv -i mm_didu.mp3  -acodec pcm_s16le  -ac 2 -ar 44100 -vol 200  out.wav

M_didu.wav

```

# Compliation
```
gcc -o HACKRF_WBFM_Transmit HackRF_WBFM_Transmit.c -lm -lhackrf
```
Now we got HACKRF_WBFM_Transmit.
```
chmod +x HACKRF_WBFM_Transmit

./HACKRF_WBFM_Transmit

Usage:./HACKRF_WBFM_Transmit <WAV File Abs Path>
```

# Verification
I also upload the test wave file.
```
./HACKRF_WBFM_Transmit MM_didu.wav

HACKRF WBFM Transmit demo ...

Begin to init hackrf ...

hackrf_init sucessfully!

hackrf_open() success.

Board ID Number: 2 (HackRF One)

Firmware Version: 2015.07.2

HACKRF init done.

Load wav file:MM_didu.wav

(1-4): RIFF 

(5-8) Overall size: bytes:32360616, Kb:31602 

(9-12) Wave marker: WAVE

(13-16) Fmt marker: fmt 

16 0 0 0

(17-20) Length of Fmt header: 16 

1 0 

(21-22) Format type: 1 PCM 

2 0 

(23-24) Channels: 2 

(25-28) Sample rate: 44100

(29-32) Byte Rate: 176400 , Bit Rate:1411200

4 0 

(33-34) Block Alignment: 4 

16 0 

(35-36) Bits per sample: 16 

(37-40) Data Marker: data 

(41-44) Size of data chunk: 32360580 

Number of samples:8090145 

Size of each sample:4 bytes

Approx.Duration in seconds=183.450211

Approx.Duration in h:m:s=0:3:3.450

.Valid range for data values : -32768 to 32767 

Alloc memory resample done.

Alloc memory IQ data done.

Alloc memory TX data done.

Begin to process data ...(Please wait about 20 seconds.)

Processing: 0.00%

Processing: 5.45%

Processing: 10.90%

Processing: 16.35%

Processing: 21.80%

Processing: 27.26%

Processing: 32.71%

Processing: 38.16%

Processing: 43.61%

Processing: 49.06%

Processing: 54.51%

Processing: 59.96%

Processing: 65.41%

Processing: 70.86%

Processing: 76.32%

Processing: 81.77%

Processing: 87.22%

Processing: 92.67%

Processing: 98.12%

IQ data transcode done.

Release intermedia memory.

Data process done, transmiting ...
```
Ctrl+C to stop transmiting.

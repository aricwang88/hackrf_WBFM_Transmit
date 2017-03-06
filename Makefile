
all:
	gcc -o HACKRF_WBFM_Transmit HackRF_WBFM_Transmit.c -lm -lhackrf -Wno-strict-aliasing
wav:
	@echo "Generate wave file..."
	avconv -i mm_didu.mp3  -acodec pcm_s16le  -ac 2 -ar 44100 -vol 200  MM_didu.wav
clean:
	rm HACKRF_WBFM_Transmit


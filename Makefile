
all:
	gcc -o HACKRF_WBFM_Transmit HackRF_WBFM_Transmit.c -lm -lhackrf

clean:
	rm HACKRF_WBFM_Transmit


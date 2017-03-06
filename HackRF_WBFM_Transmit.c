#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libhackrf/hackrf.h>

hackrf_device* device = NULL;
char version[256]={0};
long num_samples;

char* seconds_to_time(float raw_seconds);
void interpolation(float * in_buf, unsigned int in_samples, float * out_buf, unsigned int out_samples, float last_in_samples[4]);
void modulation(float * input, unsigned int input_len, float * output, unsigned int mode) ;

#define TRUE 1
#define FALSE 0
#define M_PI 3.14159265358979323846
#define SAMPLERATE 2000000
#define CHUNK_LEN 262144
#define FREQ  100400000

#define SWAP16(x) (((x & 0xff) << 8) | ((x & 0xff00) >> 8))

#define SWAP32(x) (((x & 0xff) << 24) | ((x & 0xff00) << 8) \
| ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24))

double integerfactor=0;
float last_in_samples[4] = { 0.0, 0.0, 0.0, 0.0 };
double fm_deviation=0;
float dsp_gain=0.9;
double fm_phase=0;

// WAVE file header format
struct HEADER {
	unsigned char riff[4];						// RIFF string
	unsigned int overall_size	;				// overall size of file in bytes
	unsigned char wave[4];						// WAVE string
	unsigned char fmt_chunk_marker[4];			// fmt string with trailing null char
	unsigned int length_of_fmt;					// length of the format data
	unsigned int format_type;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned int channels;						// no.of channels
	unsigned long sample_rate;					// sampling rate (blocks per second)
	unsigned int byterate;						// SampleRate * NumChannels * BitsPerSample/8
	unsigned int block_align;					// NumChannels * BitsPerSample/8
	unsigned int bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char data_chunk_header [4];		// DATA string or FLLR string
	unsigned int data_size;						// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};

FILE *ptr=NULL;
struct HEADER header;
float *pPCM_data=NULL;
float *pResampleData=NULL;
float *pIQ_buf=NULL;
unsigned char* pTX_buf=NULL;

unsigned int idx=0;
unsigned int gChunkNum = 0;

int _hackrf_tx_callback(hackrf_transfer *transfer) {

	memcpy(transfer->buffer, pTX_buf+idx, transfer->valid_length);
	idx += CHUNK_LEN;
	if(idx > num_samples*integerfactor*2 - CHUNK_LEN)
	{ 
		idx = 0;
	}
	return 0;
}


void tx_hackrf(char* wav_file)
{
	int ret = 0;
	char buffer4[4]={0};
	char buffer2[2]={0};
	printf("Load wav file:%s\n", wav_file);
	if(ptr==NULL)
	{
		ptr=fopen(wav_file,"rb");
		if(ptr==NULL)
		{ printf("Open file failed.\n"); }
	}
	
	ret = fread(header.riff, sizeof(header.riff),1,ptr);
	printf("(1-4): %s \n", header.riff);
	unsigned int ui_size=0;
	ret = fread(&ui_size, sizeof(ui_size), 1, ptr);
	
	SWAP32(ui_size);
	header.overall_size = ui_size;
	printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);

        ret = fread(header.wave, sizeof(header.wave), 1, ptr);
 	printf("(9-12) Wave marker: %s\n", header.wave);

 	ret = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
 	printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

 	ret = fread(buffer4, sizeof(buffer4), 1, ptr);
 	printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

 	// convert little endian to big endian 4 byte integer
	 header.length_of_fmt = buffer4[0] |(buffer4[1] << 8) |(buffer4[2] << 16) |(buffer4[3] << 24);
	printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);

	ret = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.format_type = buffer2[0] | (buffer2[1] << 8);
	char format_name[10] = "";
	if (header.format_type == 1)
		strcpy(format_name,"PCM"); 
	else if (header.format_type == 6)
		strcpy(format_name, "A-law");
	else if (header.format_type == 7)
		strcpy(format_name, "Mu-law");
	printf("(21-22) Format type: %u %s \n", header.format_type, format_name);

	ret = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.channels = buffer2[0] | (buffer2[1] << 8);
	printf("(23-24) Channels: %u \n", header.channels);
	memset(buffer4,0,4);
	ret = fread(&ui_size, sizeof(buffer4), 1, ptr);

	SWAP32(ui_size);
	header.sample_rate = ui_size;

	printf("(25-28) Sample rate: %ld\n", header.sample_rate);

	ret = fread(&ui_size, sizeof(buffer4), 1, ptr);
	
	SWAP32(ui_size);
	header.byterate = ui_size;
	
	printf("(29-32) Byte Rate: %u , Bit Rate:%u\n", header.byterate, header.byterate*8);

	ret = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.block_align = buffer2[0] |
					(buffer2[1] << 8);
	printf("(33-34) Block Alignment: %u \n", header.block_align);

	ret = fread(buffer2, sizeof(buffer2), 1, ptr);
	printf("%u %u \n", buffer2[0], buffer2[1]);

	header.bits_per_sample = buffer2[0] |
					(buffer2[1] << 8);
	printf("(35-36) Bits per sample: %u \n", header.bits_per_sample);

	ret = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
	printf("(37-40) Data Marker: %s \n", header.data_chunk_header);

	ret = fread(&ui_size, sizeof(buffer4), 1, ptr);
	
	SWAP32(ui_size);
	header.data_size=ui_size;
	
	printf("(41-44) Size of data chunk: %u \n", header.data_size);


	// calculate no.of samples
	num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
	printf("Number of samples:%lu \n", num_samples);

	long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
	printf("Size of each sample:%ld bytes\n", size_of_each_sample);

	// calculate duration of file
	float duration_in_seconds = (float) header.overall_size / header.byterate;
	printf("Approx.Duration in seconds=%f\n", duration_in_seconds);
	printf("Approx.Duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));
	//if(num_samples < header.sample_rate)
	{
		num_samples = (duration_in_seconds+1)*header.sample_rate;
		header.data_size = num_samples * header.bits_per_sample/8;
        }

	// read each sample from data chunk if PCM
	if (header.format_type == 1) { // PCM
	   // printf("Dump sample data? Y/N?");
		char c = 'y';
	//	scanf("%c", &c);
		if (c == 'Y' || c == 'y') { 
			long i =0;
			char data_buffer[size_of_each_sample];
			int  size_is_correct = TRUE;

		// make sure that the bytes-per-sample is completely divisible by num.of channels
		long bytes_in_each_channel = (size_of_each_sample / header.channels);
		if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
			printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
			size_is_correct = FALSE;
		}
 
		if (size_is_correct) { 
		  // the valid amplitude range for values based on the bits per sample
			long low_limit = 0l;
			long high_limit = 0l;

			switch (header.bits_per_sample) {
				case 8:
					low_limit = -128;
					high_limit = 127;
					break;
				case 16:
					low_limit = -32768;
					high_limit = 32767;
					break;
				case 32:
					low_limit = -2147483648;
					high_limit = 2147483647;
					break;
			}					
			pPCM_data = (float*)malloc(num_samples*4);
			if(pPCM_data == NULL)
			{ printf("Alloc memory failed.\n"); return ; }
			
			printf("\n\n.Valid range for data values : %ld to %ld \n", low_limit, high_limit);
			for (i =1; i <= num_samples; i++) {
				//printf("==========Sample %ld / %ld=============\n", i, num_samples);
				ret  = fread(data_buffer, sizeof(data_buffer), 1, ptr);
				if (ret == 1) {
					
					
					// dump the data read
					unsigned int  xchannels = 0;
					short data_in_channel = 0;
					float data_f = 0;
					
					memcpy(&data_in_channel, data_buffer,2);

					data_f = data_in_channel/65530.0;
					
					memcpy((float*)pPCM_data+i-1,&data_f,4);
				}
				else {
					printf("Error reading file. %d bytes\n", ret);
					break;
				}

			} // 	for (i =1; i <= num_samples; i++) {

	        	} // 	if (size_is_correct) { 

        	 } // if (c == 'Y' || c == 'y') { 
     } //  if (header.format_type == 1) { 
    
    int i;
    
    //Only 10s samples.
    unsigned int num_samp = 44100*10;
    integerfactor = SAMPLERATE*1.0/44100;
   unsigned int ui_len = integerfactor*num_samp; 
    //Alloc memeory used to store resample data.
    pResampleData = (float*)malloc(ui_len*4+1);
    if(pResampleData == NULL)
	printf("Failed to alloc memory for resample function.\n");
    else
	printf("Alloc memory resample done.\n");
    //Alloc memory used to store data after moulation.
    pIQ_buf = (float*)malloc(ui_len*4*2);
    if(pIQ_buf == NULL)
	printf("Failed to alloc memory for IQ data.\n");
    else
	printf("Alloc memory IQ data done.\n");

    //Alloc memory used to store data ready for HACKRF.
    pTX_buf = (unsigned char*)malloc(num_samples*integerfactor*2);
    if(pTX_buf == NULL)
	printf("Failed to alloc memory for TX data.\n");
    else
	printf("Alloc memory TX data done.\n");
    memset(pTX_buf,0x7f,num_samples*integerfactor*2);
    unsigned int t_offset=0;
    int j;
    gChunkNum = num_samples*integerfactor*2/CHUNK_LEN-1;
    printf("Begin to process data ...(Please wait about 20 seconds.)\n");
    for(i=0;i<num_samples;i+=10*44100)
    {
	  printf("Processing: %0.2f%\n", (100*(i+1.0)/num_samples));
	  if(i+441000 > num_samples)
	  {
		break;
           }
	    interpolation(pPCM_data+i, num_samp, pResampleData, ui_len, last_in_samples);    
	    modulation(pResampleData, num_samp*integerfactor, pIQ_buf,0);
            for(j=0;j<2*num_samp*integerfactor;j++)
	    {
		pTX_buf[t_offset+j] =(unsigned char)(pIQ_buf[j]*127.0); 
            }
	    t_offset += 2*num_samp*integerfactor;

    }
    printf("\nIQ data transcode done.\n");
    //Release memory here, save memory.
    printf("Release intermedia memory.\n");

	if(pPCM_data)
	    free(pPCM_data);
	if(pResampleData)
	    free(pResampleData);
	if(pIQ_buf)
	    free(pIQ_buf);
    
    printf("Data process done, transmiting ...\n");
    hackrf_start_tx(device, _hackrf_tx_callback, NULL);
    
    while(1){
	sleep(2);
	}
										

	if(ptr)
	    fclose(ptr);

        if(pTX_buf)
	    free(pTX_buf);
}


/**
 * Convert seconds into hh:mm:ss format
 * Params:
 *	seconds - seconds value
 * Returns: hms - formatted string
 **/
char* seconds_to_time(float raw_seconds) {
	char *hms;
	int hours, hours_residue, minutes, seconds, milliseconds;
	hms = (char*) malloc(100);

	sprintf(hms, "%f", raw_seconds);

	hours = (int) raw_seconds/3600;
	hours_residue = (int) raw_seconds % 3600;
	minutes = hours_residue/60;
	seconds = hours_residue % 60;
	milliseconds = 0;

	// get the decimal part of raw_seconds to get milliseconds
	char *pos;
	pos = strchr(hms, '.');
	int ipos = (int) (pos - hms);
	char decimalpart[15];
	memset(decimalpart, ' ', sizeof(decimalpart));
	strncpy(decimalpart, &hms[ipos+1], 3);
	milliseconds = atoi(decimalpart);	


	sprintf(hms, "%d:%d:%d.%d", hours, minutes, seconds, milliseconds);
  return hms;
}

void interpolation(float * in_buf, unsigned int in_samples, float * out_buf, unsigned int out_samples, float last_in_samples[4]) 
{
	unsigned int i;		/* Input buffer index + 1. */
	unsigned int j = 0;	/* Output buffer index. */
	float pos;		/* Position relative to the input buffer
					* + 1.0. */

					/* We always "stay one sample behind", so what would be our first sample
					* should be the last one wrote by the previous call. */
	pos = (float)in_samples / (float)out_samples;
	while (pos < 1.0)
	{
		out_buf[j] = last_in_samples[3] + (in_buf[0] - last_in_samples[3]) * pos;
		j++;
		pos = (float)(j + 1)* (float)in_samples / (float)out_samples;
	}

	/* Interpolation cycle. */
	i = (unsigned int)pos;
	while (j < (out_samples - 1))
	{

		out_buf[j] = in_buf[i - 1] + (in_buf[i] - in_buf[i - 1]) * (pos - (float)i);
		j++;
		pos = (float)(j + 1)* (float)in_samples / (float)out_samples;
		i = (unsigned int)pos;
	}

	/* The last sample is always the same in input and output buffers. */
	out_buf[j] = in_buf[in_samples - 1];

	/* Copy last samples to last_in_samples (reusing i and j). */
	for (i = in_samples - 4, j = 0; j < 4; i++, j++)
		last_in_samples[j] = in_buf[i];
}


void modulation(float * input, unsigned int input_len, float * output, unsigned int mode) 
{
	unsigned int i;
	if (mode == 0) {
		fm_deviation = 2.0 * M_PI * 75.0e3 / SAMPLERATE; // 75 kHz max deviation WBFM
	}
	else if (mode == 1)
	{
		fm_deviation = 2.0 * M_PI * 5.0e3 / SAMPLERATE; // 5 kHz max deviation NBFM
	}

	//AM mode
	if (mode == 2) {
		for (i = 0; i < input_len; i++) {
			double	audio_amp = input[i] * dsp_gain;

			if (fabs(audio_amp) > 1.0) {
				audio_amp = (audio_amp > 0.0) ? 1.0 : -1.0;
			}

			output[i * 2] = 0;
			output[i * 2 + 1] = (float)audio_amp;
		}
	}
	//FM mode
	else {

		for (i = 0; i < input_len; i++) {

			double	audio_amp = input[i] * dsp_gain;

			if (fabs(audio_amp) > 1.0) {
				audio_amp = (audio_amp > 0.0) ? 1.0 : -1.0;
			}
			fm_phase += fm_deviation * audio_amp;
			while (fm_phase > (float)(M_PI))
				fm_phase -= (float)(2.0 * M_PI);
			while (fm_phase < (float)(-M_PI))
				fm_phase += (float)(2.0 * M_PI);

			output[i * 2] = (float)sin(fm_phase);
			output[i * 2 + 1] = (float)cos(fm_phase);
		}
	}


}

int init_hackrf()
{
	int result;
	printf("Begin to init hackrf ...\n");
	result = hackrf_init();

	if(result != HACKRF_SUCCESS) {
		printf("hackrf_init() failed.\n");
		return EXIT_FAILURE;
	}
	else
		printf("hackrf_init sucessfully!\n");
	
	result = hackrf_open(&device);
	if(result != HACKRF_SUCCESS) {
		printf("hackrf_open() failed.\n");
		return EXIT_FAILURE;
	}
	else
		printf("hackrf_open() success.\n");
	uint8_t board_id;
	result = hackrf_board_id_read(device, &board_id);
	if(result == HACKRF_SUCCESS)
	{
		printf("Board ID Number: %d (%s)\n", board_id,
			hackrf_board_id_name(board_id));
	}
	result = hackrf_version_string_read(device, &version[0], 255);
	if(result == HACKRF_SUCCESS) {
		printf("Firmware Version: %s\n", version);
	}

	/////////////////Settings for the devices. ////////////////////
	hackrf_set_sample_rate(device, 2000000);
	hackrf_set_baseband_filter_bandwidth(device, 2000000*0.75);
	hackrf_set_freq(device, FREQ);
	hackrf_set_txvga_gain(device, 20);
	hackrf_set_lna_gain(device, 28);
	hackrf_set_amp_enable(device, 0);

	printf("HACKRF init done.\n");
}
int main(int argc, char *argv[])
{
	if(argc == 2)
	{
		printf("HACKRF WBFM Transmit demo ...\n");
		init_hackrf();
		tx_hackrf(argv[1]);
	}else
	{
		printf("Usage:%s <WAV File Abs Path>\n", argv[0]);
	}
	return 0;
}


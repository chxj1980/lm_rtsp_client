#include <stdio.h>
#include "avdec_proc.h"

avdec::avdec()
{

}

avdec::~avdec()
{

}

int avdec::video_h264_dec(int chn,  long long pts, char *data, unsigned int size)
{

	return 0;
}

int avdec::audio_aac_dec(int chn, long long pts, char *data, unsigned int size)
{
	printf("####[avdec::audio_aac_dec]\n");
	return 0;
}



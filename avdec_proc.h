#ifndef __AVDEC_H__
#define __AVDEC_H__

#include <iostream>
#include <stdio.h>

using namespace std;

class avdec
{
	public:
		avdec();
		~avdec();

		int video_h264_dec(int chn,        long long pts, char *data, unsigned int size);
		int audio_aac_dec(int chn,        long long pts, char *data, unsigned int size);
		
	private:
		
};

#endif
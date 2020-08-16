#ifndef __FRAME_MAN_H__
#define __FRAME_MAN_H__

#include <iostream>

#include "record_manage.h"
#include "avdec_proc.h"

using namespace std;

class frame_man
{
	public:
		frame_man();
		~frame_man();

		int recv_video_frame_from_rtp(int chn, int stream_type, long long pts, char *data, unsigned int size);
		int recv_audio_frame_from_rtp(int chn,int stream_type,           long long pts, char *data, unsigned int size);
		
	private:
		avdec       env;
		record_man	rec;
};

#endif


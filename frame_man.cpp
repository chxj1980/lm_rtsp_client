#include <stdio.h>

#include "frame_man.h"


frame_man::frame_man()
{

}

frame_man::~frame_man()
{

}

int frame_man::recv_video_frame_from_rtp(int chn,			int stream_type, long long pts, char *data, unsigned int size)
{
		rec.add_frame2list(chn, stream_type, pts, data, size);
		env.video_h264_dec(chn, pts, data, size);
		
	return 0;
}

int frame_man::recv_audio_frame_from_rtp(int chn,int stream_type,			long long pts, char *data, unsigned int size)
{
	
	return 0;
}

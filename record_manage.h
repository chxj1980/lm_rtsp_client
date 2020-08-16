#ifndef __RECORD_MAN_H__
#define __RECORD_MAN_H__

#include <iostream>

using namespace std;

#define DEF_RECORD_PATH    "/home/aaron/record"

class record_man
{
	public:
		record_man();
		~record_man();

		int init_record( char *path);
		int add_frame2list(int chn, int stream_type, long long pts, char *data, unsigned int size);
		
	private:
		
};

#endif


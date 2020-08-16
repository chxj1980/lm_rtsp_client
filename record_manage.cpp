#include <stdio.h>
#include <string.h>
#include "record_manage.h"

record_man::record_man()
{

}

record_man::~record_man()
{

}

int record_man::init_record( char *path)
{
	char rec_dir[36]= {'\0'};
	
	if(path == NULL){
		memcpy(rec_dir,  DEF_RECORD_PATH, strlen(DEF_RECORD_PATH));
	}
	else{
		memcpy(rec_dir, path , strlen(path));
	}
	
	printf("[init_record]:path[%s]\n", rec_dir);
	
	return 0;
}

int record_man::add_frame2list(int chn, int stream_type, long long pts, char *data, unsigned int size)
{

}
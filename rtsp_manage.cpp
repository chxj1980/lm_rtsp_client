//#include<stdio.h>
#include"rtsp_client.h"

int main()
{
	rtsp_client rtsp_m;
    const char *rtsp = "rtsp://:8554/192.168.1.100";
	//const char *rtsp = "rtsp://192.168.1.215:5544/live0.264";
	rtsp_m.start_rtsp((char *)rtsp, 1);//1:tcp, 0:udp

	while(1)
	{
		sleep(2);
	}

}

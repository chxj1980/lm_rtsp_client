#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__


#include "network.h"
#include "netevent.h"
#include "m_buff.h"

#include "frame_man.h"

using namespace std;


#define PACK_SIZE  (1024*1024)

#define RTSP_SER_PORT  (62620)

typedef enum
{
	OPTIONS = 0, 
	DESCRIBE,
	SETUP, 
	PLAY, 
	PAUSE, 
	TEARDOWN, 
	GET_PARAMETER, 
	SET_PARAMETERs,
	MAX
}Rtsp_cmd;


typedef enum
{
	FRAME_TYPE_H264_I,
	FRAME_TYPE_H264_P,
	FRAME_TYPE_H264_B,
}Frame_type_t;

typedef enum
{
	VIDOE_TYPE_H264,
	VIDOE_TYPE_H265,
	VIDOE_TYPE_MJPG,
	
	VIDOE_TYPE_MAX,
	
	AUDIO_TYPE_G711A,
	AUDIO_TYPE_G711U,
	AUDIO_TYPE_G726A,
	AUDIO_TYPE_AAC,
	AUDIO_TYPE_PCMU,
	
	AUDIO_TYPE_MAX
}Media_type_t;

typedef enum
{
	RTCP_SR = 200,
	RTCP_RR ,
	RTCP_SDES ,
	RTCP_BYE,
	RTCP_APP 
}Rtcp_type_t;

typedef struct 
{	
	int client_rtp_sock;
	int client_rtp_port;
	
	int client_rtcp_sock;
	int client_rtcp_port;
	
	int serevr_rtp_port;
	int serevr_rtcp_port;
	
}Net_Info_t;

typedef struct 
{
	int type;
	int port;
	int payload_type;
	int sample_rate;
	int		sps_len;
	unsigned char sps[128];
	int		pps_len;
	unsigned char pps[128];
	char track_name[24];
	char session[16];
}Media_t;

typedef struct 
{
	char url[256];
	char ip[24];
	char port[8];

	int  local_tcp_sock;
	char local_port[8];
	int	 rtp_socket;
	char rtp_port[2][8]; //rtp_port[0]:vidoe rtp_port[0]:audio 
	int	 rtcp_socket;
	char rtcp_port[2][8];
	int  track_nums;
	Media_t  video_m;
	Media_t  audio_m;
}Rtsp_info_t;

typedef struct 
{
	unsigned char	dollar;
	unsigned char	channel;
	unsigned short	len;
}RtpPack_t;

typedef struct 
{
#ifdef BIGENDIAN
	unsigned char  ver:2;   //V
	unsigned char  padb:1;  //P
	unsigned char  extb:1;  //X
	unsigned char  csrc:4;  //CC
	unsigned char  markb:1; //M
	unsigned char  paytype:7; //M
#else
	unsigned char  csrc:4;	//CC
	unsigned char  extb:1;	//X
	unsigned char  padb:1;	//P
	unsigned char  ver:2;  //V
	unsigned char  paytype:7; //M
	unsigned char  markb:1; //M
	
#endif
	unsigned short seq_num;
	unsigned int   timestamp;
	unsigned int   ssrc;
}RtpHead_t;

typedef struct
{
   #if 1
	unsigned char F:1;
	unsigned char Nri:2;
	unsigned char Type:5;
   #else
    unsigned char Type:5;
    unsigned char Nri:2;
    unsigned char F:1;
   #endif
}Fu_indicator_t;

typedef struct
{
	unsigned char S:1;
	unsigned char E:1;
	unsigned char R:1;
	unsigned char Type:5;
}Fu_head_t;

typedef struct
{
	#ifdef BIGENDIAN
	unsigned char  ver:2;   //V
	unsigned char  padb:1;  //P
	unsigned char  rc:5;    //RC
	unsigned char  pt:8;   //pack_type
#else
	unsigned char  rc:5;    //RC
	unsigned char  padb:1;  //P
	unsigned char  ver:2;   //V
	unsigned char  pt:8;   //pack_type	
#endif
	unsigned short lenth:16;
}Rtcp_Head_t;

typedef struct
{
	unsigned int  ssrc;     //SSRC_1(SSRC0 first source)
	unsigned int  fl_cnpl;  //Fractionlost + cumulative number of packet lost
	unsigned int  ex_hsnr; 
	unsigned int  int_jitter;
	unsigned int  lsr; 
	unsigned int  delay_snc_lsr; 
}Report_t;

typedef struct
{
	Rtcp_Head_t  comm_head;
	unsigned int ssrc; //SR pack sysnc flag info.
	Report_t rb;
}Rtcp_RR_t; //rtcp receiver report.

typedef struct 
{
	unsigned int   pack_count;
	unsigned int   all_pack_size;
	unsigned short first_seq_number;
	unsigned short last_seq_number;
	unsigned int   ssrc;
}rtp_stat_t;

class rtsp_client : public netevent
{
	public:
		rtsp_client();
		~rtsp_client();

	 	int start_rtsp(char *url, unsigned char istcp);

	private:
			int rtsp_cmd_options(int sock, network &net);
			int rtsp_cmd_describre(int sock, network &net);
			int rtsp_cmd_setup(int sock, network &net, unsigned char istcp, Rtsp_info_t &rtspinf);
			int rtsp_cmd_play(int sock, network &net);

			int rtp_rtcp_udp_create(Rtsp_info_t &netifo, network &net);
			int rtp_rtcp_proc(Rtsp_info_t &rtspinf, char istcp, network &net);
			int rtp_rtcp_net_tcp_proc(int socket, network &net);
			int rtp_rtcp_net_udp_proc(Rtsp_info_t &rtspinf, network &net);
			
			static void * init_rtsp(void *args);
			static void * create_rtsp(void * args);
			static void * function(void * args);
			int parse_url(char *ip,  char *port, char *url);
			int rtsp_parse_describre_sdp (char *data, int size,  Rtsp_info_t &rtspinfo);
			int rtsp_parse_recv_setup (char *data, int size, Rtsp_info_t &rtspinfo);
			int parse_media_type( char *data,  char isvideo);
			int media_type2(int payload_type);
			int parse_recv_tcp_data(char *data, int size);
			int parse_recv_udp_rtp_data(char *data, int size);
			int parse_recv_udp_rtcp_data(char *data, int size); 

			int parse_rtp(char *data, int size);
			int h264pack_proc(char* data,  int size);
			virtual int on_net_event(int type, char *buff, int lenth);
			int update_rtcp_stat( RtpHead_t *rtphead, int size, rtp_stat_t *rtp_stat);
			int rtcp_info_fill(rtp_stat_t *rtp_stat, Rtcp_RR_t * rtcp_head);

	private:
			Rtsp_info_t     rtspInf_m;
			Net_Info_t		net_ifo_m;
			unsigned char   IsTcp;
			unsigned char   supportcmd[MAX];
			M_buff_t		m_buff;

			frame_man		m_frame_man;
			rtp_stat_t		rtp_stat_m[2];
};

#endif

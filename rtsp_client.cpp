#include "rtsp_client.h"
#include "Mstring.h"
#include "frame_man.h"


const char *CMD[12] = {"OPTIONS", "DESCRIBE", "SETUP", "PLAY", "PAUSE", "TEARDOWN", "GET_PARAMETER", "SET_PARAMETER"};
const char  media_type[][8] = {"h264", "h265", "MJPG", "VMAX", "g711a", "g711u", "g726a", "aac", "pcmu",  "AMAX"};

int H264_HEAD_FLAE = 0x01000000;

void print_t(const char *data, int size, const char *flasg)
{
	 int i = 0;
	 printf("%s:\n", flasg);
	 for(; i < size; i++)
	 {
		 printf("%02x ", data[i]);
	 }

 	printf("\n");
}

rtsp_client::rtsp_client()
{
	memset(supportcmd, 0, sizeof(supportcmd));
	memset(&rtspInf_m, 0, sizeof(Rtsp_info_t));
	memset(&m_buff, 0, sizeof(M_buff_t));	
	memset(&net_ifo_m, 0, sizeof(Net_Info_t));
	memset(&netevent_info_m, 0, sizeof(netevent_info_t));
	
	memset(&rtp_stat_m, 0, sizeof(rtp_stat_m));
}

rtsp_client::~rtsp_client()
{
	destroy_buff(&m_buff);
}

int rtsp_client::start_rtsp(char *url, unsigned char istcp)
{
	PRT_COND(url == NULL, "[start_rtsp]=>Input [url] is NULL, exit !");

	memcpy(rtspInf_m.url, url,  strlen(url));
	IsTcp = istcp;
		
	pthread_t pid;
	pthread_create(&pid, 0, init_rtsp, this);

	return 0;
}

void*  rtsp_client::init_rtsp(void *args)
{
	pthread_t pid;
	rtsp_client * This = (rtsp_client *)args;
	
	pthread_create(&pid, 0, create_rtsp, This);
	while(1)
	{
		sleep(10);
	}
	
	pthread_join(pid, NULL);
}

void * rtsp_client::create_rtsp(void * args)
{
	rtsp_client * This = (rtsp_client *)args;
	
	This->parse_url(This->rtspInf_m.ip, This->rtspInf_m.port, This->rtspInf_m.url);
	
	network net;
	int sockfd;
	
	sockfd = net.create_tcp(This->rtspInf_m.ip, This->rtspInf_m.port);
	PRT_COND1(sockfd< 0, "Connect failed,  exit !");
	
	if( This->IsTcp == 0)//rtcp/ rtp will use udp translet.
		This->rtp_rtcp_udp_create(This->rtspInf_m, net);

	int ret = init_buff( &(This->m_buff), PACK_SIZE );
	PRT_COND1(ret < 0, "[create_rtsp]: malloc pack buff failed,  exit !");
	
	ret = This->rtsp_cmd_options(sockfd, net); //cmd options
	if(ret < 0) return NULL;
	
	ret = This->rtsp_cmd_describre(sockfd, net);
	if(ret < 0) return NULL;
	
	This->rtsp_cmd_setup(sockfd, net, This->IsTcp, This->rtspInf_m);
	if(ret < 0) return NULL;

	This->rtsp_cmd_play(sockfd, net);
	if(ret < 0) return NULL;

	This->rtspInf_m.local_tcp_sock = sockfd;
	This->rtp_rtcp_proc(This->rtspInf_m, This->IsTcp, net);
	
	return NULL;
}

int rtsp_client::rtp_rtcp_udp_create(Rtsp_info_t &netifo, network &net)
{
	char port[8] = {'\0'};
	netifo.rtp_socket = net.create_udp(NULL, port);
	if(netifo.rtp_socket > 0)
	{
			net.get_sock_info(netifo.rtp_socket, netifo.local_port);
			if(netifo.local_port > 0)
			{
				sprintf(port, "%d",  atoi(netifo.local_port) + 1);
				netifo.rtcp_socket = net.create_udp(NULL, port);
				PRT_COND(netifo.rtcp_socket < 0, "[rtp_rtcp_udp_create]: create_udp rtcp failed, exit !");

				return 0;
			}
	}
	
	printf("[rtp_rtcp_udp_create]: create_udp rtp failed, exit !\n");
	return -1;
}

int rtsp_client::rtp_rtcp_proc(Rtsp_info_t &rtspinf, char istcp, network &net)
{
	if(istcp)
		rtp_rtcp_net_tcp_proc(rtspinf.local_tcp_sock, net);
	else
		rtp_rtcp_net_udp_proc(rtspinf, net);
	
	return 0;
}

int rtsp_client::rtp_rtcp_net_tcp_proc(int socket, network &net)
{
	Event_Handler_t hand;
	Rtcp_RR_t rtcp_rr;
	memset(&hand, 0, sizeof(Event_Handler_t));
	memset(&rtcp_rr, 0, sizeof(rtp_stat_t));
	int ret =0;
	int video = 0;
	int len = 0;
	int date = 0;
	char send_buf[512] ={'\0'};
	hand.istcp = true;
	hand.e_type = TYPE_TCP_RTP_RTCP_RECV;
	hand.sock = socket;
	hand.id = 0;
	
	net.start_net_proc(hand);
	send_buf[0] = 12;
	while(1)
	{
		//net.recv_data_tcp(socket, buff, 1024 * 400);
		rtcp_info_fill(&(rtp_stat_m[0]),  &rtcp_rr);
		len = sprintf(send_buf, "$");
		if(video)
			len = sprintf(send_buf + len, "1");
		else	
			len = sprintf(send_buf + len, "3");

		SET_16(send_buf + len, sizeof(rtp_stat_t));  
		len += sizeof(unsigned short);
		
		memcpy(send_buf + len,  &rtcp_rr,  sizeof(Rtcp_RR_t));
		len += sizeof(Rtcp_RR_t);
		
		ret = net.send_data_tcp(socket, send_buf, len);
		if(ret < 0){
			printf("!!!!!!!!!!!!----->\n");
		}

		//printf("=======[%d]=[%d]=======\n", len, ret);
		usleep(1000 *1100);
	}

	return 0;
}

int rtsp_client::rtp_rtcp_net_udp_proc(Rtsp_info_t &rtspinf, network &net)
{
	Event_Handler_t rtp_hand;
	Event_Handler_t rtcp_hand;
	memset(&rtp_hand, 0, sizeof(Event_Handler_t));

	rtp_hand.event_hand = this;
	rtp_hand.istcp = false;
	rtp_hand.e_type = TYPE_UDP_RTP_RECV;
	rtp_hand.sock = rtspinf.rtp_socket;
	rtp_hand.id = 0;
	net.start_net_proc(rtp_hand);

	memset(&rtcp_hand, 0, sizeof(Event_Handler_t));
	rtcp_hand.event_hand = this;
	rtcp_hand.istcp = false;
	rtcp_hand.e_type = TYPE_UDP_RTCP_RECV;
	rtcp_hand.sock = rtspinf.rtcp_socket;
	rtcp_hand.id = 0;
	net.start_net_proc(rtcp_hand);
	
	while(1)
	{
		sleep(3);
	}

	return 0;
}

int rtsp_client::on_net_event(int type, char *buff, int lenth)
{	
	PRT_COND((type > TYPE_MAX ) || (type > TYPE_MAX ), "[on_net_event] Input [type] error, exit !");

	switch(type)
	{
		case TYPE_TCP_RTP_RTCP_RECV:
					printf("tcp---RECV---->\n");
					parse_recv_tcp_data(buff, lenth);
				break;
				
		case TYPE_UDP_RTP_RECV:
					printf("-------222---->\n");
					parse_recv_udp_rtp_data(buff, lenth);
				break;
				
		case TYPE_UDP_RTCP_RECV:
					printf("++++++333+++++>\n");
					parse_recv_udp_rtcp_data(buff, lenth);
				break;
		
		default:
				break;
	}

	return 0;
}

int rtsp_client::parse_recv_udp_rtp_data(char *data, int size)
{
	if( (size <= 0) || (data == NULL) ){
			printf("[parse_recv_udp_rtp_data]:Input [data] is NULL,  or [size] <= 0, exit ! \n");
			return -1;
	}

	return 0;
}

int rtsp_client::parse_recv_udp_rtcp_data(char *data, int size)
{
	if( (size <= 0) || (data == NULL) ){
			printf("[parse_recv_udp_rtcp_data]:Input [data] is NULL,  or [size] <= 0, exit ! \n");
			return -1;
	}
	
	return 0;
}

int rtsp_client::parse_recv_tcp_data(char *data, int size)
{
	PRT_COND((size <= 0) || (data == NULL), "[parse_recv_tcp_data]:Input [data] is NULL,  or [size] <= 0, exit !");
	
	if(data[0] == '$')
	{
		RtpPack_t *pack = (RtpPack_t *)data;
		printf("[%d===%d]\n", pack->channel, pack->len);
		parse_rtp(data + 4, pack->len);
	}
	else{
		printf("[]:exit !\n");
	}

	return 0;
}

int rtsp_client::parse_rtp(char *data, int size)
{
	PRT_COND((size <= 0) || (data == NULL), "[parse_rtp]:Input [data] is NULL,  or [size] <= 0, exit !");

	RtpHead_t rtph;
	int len = sizeof(RtpHead_t);
	memset(&rtph,  0, sizeof(rtph));
	memcpy(&rtph, data, len);
	data += sizeof(RtpHead_t);

	if(rtph.paytype == rtspInf_m.video_m.payload_type){
		update_rtcp_stat(&rtph, len, &(rtp_stat_m[0]));
	}
	else if(rtph.paytype == rtspInf_m.audio_m.payload_type){
		update_rtcp_stat(&rtph, len, &(rtp_stat_m[1]));
	}
	
	printf("paytype:[%d], version:[%d]\n", rtph.paytype, rtph.ver);
	printf("[%d], [%d],[%d], [%d]\n", rtph.extb, rtph.padb, rtph.csrc, rtph.seq_num);
	
	if(rtph.paytype == 96){ //h264
		print_t(data - 2, 16, "recv rtp:");
		h264pack_proc(data, size - len );
	}
	else{ //mabey is audio.

	}
	
	return 0;
}

int rtsp_client::h264pack_proc(char* data,  int size)
{
	static unsigned char full_frame = 0;
	int frame_type = 0;
	int nal_type = 0;
	static int long  pts = 0;
	
	Fu_indicator_t *fu_indicator = (Fu_indicator_t *)data;
	data++;
	size--; 

	//printf("======[%d]======\n", fu_indicator->Type);
	Fu_head_t  *fu_head = NULL;

	if(fu_indicator->Type == 24)
	{
		fu_head = (Fu_head_t *)(data + 1);
		if(fu_head->S){
			
		}
		else if(fu_head->E){
			
		}
	}
	else //单包
	{
		memcpy(m_buff.buff, &H264_HEAD_FLAE,  4); //little mode
		memcpy(m_buff.buff, data+4,  size);
		
		m_buff.off = 4 + size;
		nal_type = fu_indicator->Type;
		full_frame = 1;
	}

	PRT_COND(!full_frame, "[h264pack_proc]: Recv data, is not full frame, Buffing first, exit !");
	PRT_COND(m_buff.off > 2 *1024 *1024, "[h264pack_proc]: Recv data size more 2M, exit !");
	
	full_frame = 0;
	frame_type = (nal_type == 5)?FRAME_TYPE_H264_I:FRAME_TYPE_H264_P;
	
	m_frame_man.recv_video_frame_from_rtp(0, frame_type, pts, m_buff.buff, m_buff.off);
	return 0;	
}

int rtsp_client::rtsp_cmd_options(int sock, network &net)
{
	char send_buf[256] = {'\0'};
	char recv_buf[256] = {'\0'};
	
	int len = sprintf(send_buf, "OPTIONS %s RTSP/1.0\r\n", rtspInf_m.url);
	len += sprintf(send_buf + len, "CSeq: 2\r\n");
	len += sprintf(send_buf + len, "User-Agent: LibVLC/2.2.8 (LIVE555 Streaming Media v2016.02.22)\r\n\r\n");

	int ret = net.send_data_tcp(sock, send_buf, len);
	PRT_COND(ret < 0, "send cmd options failed, exit !");

	usleep(1000 *20);
	ret = net.recv_data_tcp(sock, recv_buf, sizeof(recv_buf));
	PRT_COND(ret < 0, "recv cmd respond failed, exit !");

	//正常情况下 都支持这些cmd的, 这里其实不用解析的.
	for(int i = OPTIONS; i < GET_PARAMETER; i++) 
	{
		if(find_str(recv_buf, ret, CMD[i]) > 0)
		{
			supportcmd[i] = 1;
			printf("[CMD]:[%s]\n", CMD[i]);
		}
	}
	printf("\n");

	PRT_COND(!supportcmd[DESCRIBE], "Cant not support DESCRIBE cmd, exit !");
	return 0;
}

int rtsp_client::rtsp_cmd_describre(int sock, network &net)
{
	char send_buf[256] = {'\0'};
	char recv_buf[2000] = {'\0'};
	char *parse_sdp = NULL;
	int off_size = 0;
	
	int len = sprintf(send_buf, "DESCRIBE %s RTSP/1.0\r\n", rtspInf_m.url);
	len += sprintf(send_buf + len, "CSeq: 3\r\n");
	len += sprintf(send_buf + len, "User-Agent: LibVLC/2.2.8 (LIVE555 Streaming Media v2016.02.22)\r\n");
	len += sprintf(send_buf + len, "Accept: application/sdp\r\n\r\n"); //X

	int ret = net.send_data_tcp(sock, send_buf, len);
	PRT_COND(ret < 0, "send cmd options failed, exit !");

	usleep(1000 *20);
	ret = net.recv_data_tcp(sock, recv_buf, sizeof(recv_buf));
	PRT_COND(ret < 0, "recv cmd respond failed, exit !");

	off_size = find_str(recv_buf, ret, "Content-Type");
	PRT_COND(off_size < 0, "[TNT]:Cant get sdp data, exit !");

	parse_sdp = &recv_buf [off_size + 1];
    len = ret - off_size;
	rtsp_parse_describre_sdp(parse_sdp, len, rtspInf_m);
	//printf("++++++1312+++++++++\n\n");
	return 0;
}

int rtsp_client::rtsp_parse_describre_sdp (char *data, int size,  Rtsp_info_t &rtspinfo)
{
	PRT_COND((data == NULL) || (size <= 0), "[rtsp_parse_describre_sdp]:[data] is NULL or [size] <= 0, exit !");

	int track_nums = 2;
	char *media = NULL;
	char *tmp = NULL;
	int sur_len = 0;
	char *parse_data = data;

	for(int i = 0; i < track_nums; i++)
	{
		if(	(media = strstr(parse_data, "m=video")) )
		{ //m=video 0 RTP/AVP 96
			if(media = find_char(media, size, ' ', 1))  rtspinfo.video_m.port =  atoi(media + 1);
			if(media = find_char(media, size, ' ', 2))  rtspinfo.video_m.payload_type = atoi(media + 1);					
		
			if(	(media = strstr(data, "a=rtpmap")) )
			{ //a=rtpmap:96 H264/90000
				if(media = find_char(media, size, ' ', 1))  
				{
					sur_len = size - (media - data);
					tmp = media + 1;
					if(media = find_char(media + 1,sur_len,  '/', 1))  
					{
						char type[8];
						int int_type = 0;
						memset(type, '\0', sizeof(type));
						memcpy(type, tmp, media - tmp);
												
						if( (int_type = parse_media_type(type, 1)) < 0 ){
							return -1;
						}
						
						rtspinfo.video_m.type = int_type;
						rtspinfo.video_m.sample_rate = atoi(media + 1); //"/90000"

						// get pps,sps.
						if(	(media = strstr(media, "a=fmtp")) )
						{// a=fmtp:96 packetization-mode=1;profile-level-id=64001E;sprop-parameter-sets=Z2QAHqwsaoKA9puAgICB,aO48sA==

						}

						if(	(media = strstr(media, "a=control:")) )  //a=control:track1
						{	
							tmp = media+10;
							if((media = strstr(tmp, "\r\n")))
							{
								memcpy(rtspinfo.video_m.track_name, tmp, media  - tmp);
								rtspinfo.track_nums = 1;
							}
						}
						parse_data = media;
						printf("type:%s, payload_type:[%d], port:[%d],sample_rate:[%d], track_name[%s]\n", type, rtspinfo.video_m.payload_type, \
							                        rtspinfo.video_m.port, rtspinfo.video_m.sample_rate, rtspinfo.video_m.track_name);
					}
				}
			}
		}
		else if((media = strstr(parse_data, "m=audio")) )
		{
			if(	(media = strstr(media, "a=rtpmap")) )
			{ //a=rtpmap:0 PCMU/8000
				if(media = find_char(media, size, ' ', 1))  
				{
					sur_len = size - (media - data);
					tmp = media + 1;
					if(media = find_char(tmp,sur_len,  '/', 1))  
					{
						char type[8];
						int int_type = 0;
						memset(type, '\0', sizeof(type));
						memcpy(type, tmp, media - tmp);						
						if( (int_type = parse_media_type(type, 0)) < 0 ){
							return -1;
						}
						
						rtspinfo.audio_m.type = int_type;
						rtspinfo.audio_m.sample_rate = atoi(media + 1); //"/8000"

						if(	(media = strstr(media, "a=control:")) )  //a=control:track2
						{	
							tmp = media+10;
							if((media = strstr(tmp, "\r\n")))
							{
								memcpy(rtspinfo.audio_m.track_name, tmp, media  - tmp);
								rtspinfo.track_nums = 2;
							}
						}
			
						printf("type:%s, payload_type:[%d], port:[%d],sample_rate:[%d], track_name[%s]\n\n", type, rtspinfo.audio_m.payload_type, \
							                        rtspinfo.audio_m.port, rtspinfo.audio_m.sample_rate, rtspinfo.audio_m.track_name);
					}
				}	
			}
		}
	}
	
}

int rtsp_client::parse_media_type( char *data,  char isvideo)
{
	PRT_COND(data == NULL, "[parse_media_type]: [data] is NULL, exit !");
	int i = 0;
	int tmptype =0;
	
	if(isvideo)
	{
		i = VIDOE_TYPE_H264;
		tmptype = VIDOE_TYPE_MAX;
	}
	else
	{
		i = AUDIO_TYPE_G711A;
		tmptype = AUDIO_TYPE_MAX;
	}
	
	for(; i < tmptype; i++)
	{
		if(!strncasecmp(data, media_type[i], strlen(media_type[i])))
			return i;
	}

	printf("[parse_media_type]: Cant not get media type, exit !\n");
	return -1;
}


int rtsp_client::media_type2(int payload_type)
{
	int type = 0;

	switch(payload_type)
	{
		case 96:
			type = VIDOE_TYPE_H264;
			break;

		case 97:
			type = VIDOE_TYPE_H265;
			break;

		case 98:
			type = VIDOE_TYPE_MJPG;
			break;
		
		case 0:
			type = AUDIO_TYPE_PCMU;
			break;

		case 100:
			type = AUDIO_TYPE_G726A;
			break;

		default:
				break;
	}

	return type;
}
	
int rtsp_client::rtsp_cmd_setup(int sock, network &net, unsigned char istcp, Rtsp_info_t &rtspinf)
{
	char send_buf[256] = {'\0'};
	char recv_buf[256] = {'\0'};
	int len = 0;
	int ret = 0;
	
	for(int i = 0; i < 2; i++)
	{
		memset(send_buf, 0, sizeof(send_buf));
		memset(recv_buf, 0, sizeof(recv_buf));
		len  = 0;

		len = sprintf(send_buf, "SETUP %s/%s RTSP/1.0\r\n", rtspInf_m.url, (i==0)?(rtspInf_m.video_m.track_name):(rtspInf_m.audio_m.track_name) );
		len += sprintf(send_buf + len, "CSeq: %d\r\n", 4 +i);
		len += sprintf(send_buf + len, "User-Agent: LibVLC/2.2.8 (LIVE555 Streaming Media v2016.02.22)\r\n");
		if(istcp){
			  len += sprintf(send_buf + len, "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n\r\n", 0, 1);  //interleaved=0-1
		}
		else{
			  int port = atoi(rtspInf_m.local_port);
			  len += sprintf(send_buf + len, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n", port+2*i, port+1+2*i);
			  if(rtspInf_m.video_m.session[0] != 0){
			     len += sprintf(send_buf + len, "Session: %s\r\n\r\n", rtspInf_m.video_m.session);
			  }
			  else{
				len += sprintf(send_buf + len, "\r\n");
			}
		}
		
		ret = net.send_data_tcp(sock, send_buf, len);
		PRT_COND(ret < 0, "send cmd options failed, exit !");

		usleep(1000 *20);
		ret = net.recv_data_tcp(sock, recv_buf, sizeof(recv_buf));
		PRT_COND(ret < 0, "recv cmd respond failed, exit !");

		rtsp_parse_recv_setup(recv_buf, ret, rtspinf);
		usleep(1000 *20);
	}

	return ret;
}

int rtsp_client::rtsp_parse_recv_setup (char *data, int size, Rtsp_info_t &rtspinfo)
{
	static int trick_nums = 0;
	PRT_COND((data == NULL) || (size <= 0), "[rtsp_parse_recv_setup]:[data] is NULL or [size] <= 0, exit !");

	int offsize = find_str(data, size, "Session:"); //Session: 416DAB4F;timeout=60
	if(offsize > 0)
	{
		char *tmpdata = data + offsize + 1;
		char *end = NULL;
		if((end = strchr(tmpdata, ';')))
		{
				memcpy(rtspInf_m.video_m.session, tmpdata + 1, end - tmpdata -1);
				return 0;
		}
	}

	offsize = find_str(data, size, "server_port:"); //server_port=30004-30005;
	if(offsize > 0)
	{
		char *tmpdata = data + offsize + 1;
		char *end = NULL;
		if((end = strchr(tmpdata, '-')))
		{		
				memcpy(rtspinfo.rtp_port[trick_nums] , tmpdata, end - tmpdata );
				tmpdata = end+1;
			    if((end = strchr(tmpdata, ';'))){
					memcpy(rtspinfo.rtcp_port[trick_nums] , tmpdata, end - tmpdata );
					trick_nums++;
					if(trick_nums >= 2) trick_nums = 0;
					
					return 0;
				}	
		}
	}
	return -1;
}

int rtsp_client::rtsp_cmd_play(int sock, network &net)
{
	char send_buf[256] = {'\0'};
	char recv_buf[512] = {'\0'};
	
	int len = sprintf(send_buf, "PLAY %s RTSP/1.0\r\n", rtspInf_m.url);
	len += sprintf(send_buf + len, "CSeq: 6\r\n");
	len += sprintf(send_buf + len, "User-Agent: LibVLC/2.2.8 (LIVE555 Streaming Media v2016.02.22)\r\n");
	len += sprintf(send_buf + len, "Session: %s\r\n\r\n", rtspInf_m.video_m.session);  //X
	len += sprintf(send_buf + len, "Range: npt=\r\n\r\n");  //X

	int ret = net.send_data_tcp(sock, send_buf, len);
	PRT_COND(ret < 0, "send cmd options failed, exit !");

	usleep(1000 *20);
	ret = net.recv_data_tcp(sock, recv_buf, sizeof(recv_buf));
	PRT_COND(ret < 0, "recv cmd respond failed, exit !");

	int offsize = find_str(recv_buf, ret, "200 OK");
	PRT_COND(offsize < 0, "play  failed, exit !");
	
	return 0;
}

void * rtsp_client::function(void * args)
{

}

//rtsp://192.168.1.114:5544/live1.264
int rtsp_client::parse_url(char *ip,  char *port, char *url)
{
	PRT_COND(ip == NULL || port == NULL || url == NULL, "[parse_url]==>Input [ip or port or url]  point is NULL, exit !");
	
	char *p = NULL;    
	#if 0
	if( (p = strstr(url, "://")) ) //  192.168.1.114:5544/live1.264
	{
		p +=3;
		char *second =  NULL;
		if( (second = strstr(p, ":")) ) //  :5544/live1.264
		{
		 	memcpy(ip, p, second - p ); //get ip
		 	second +=1;
			
		 	char *third = NULL;
		 	if( (third = strstr(second, "/")) )//  /live1.264
			{
		 		memcpy(port, second, third - second ); //get port
		 		if(strspn(port, "0123456789"))
		 		{
		 			printf("[ip]:[%s],[port]:[%s]\n", ip, port);
					return 0;
				}
				printf("Cant not get port, retun !\n");
		 		return  1;
			}
		}
		
		printf("Cant not get IP, retun !\n");
		return  1;
	}
	else
	{
		printf("Not have IP, retun !\n");
		return  1;
	}
	#else
	if( (p = strstr(url, "://")) )  //rtsp://:8554/192.168.1.100
	{
		p +=3;
		if(*p == ':')
		{
			memcpy(port, p+1, 4);
			printf("port:[%s]\n", port);
			
			if(p[5] == '/')
			{
				int sz = strlen(url) - (p - url + 6);
				memcpy(ip, &p[6], sz); //get ip
				printf("ip:[%s]\n", ip);
			}
		}
	}
	#endif
}

int rtsp_client::update_rtcp_stat( RtpHead_t *rtphead, int size, rtp_stat_t *rtp_stat)
{
	PRT_COND((rtp_stat == NULL) || (rtphead == NULL) || (size <= 0), "[[update_rtcp_stat]: Input [rtp_stat] or [rtphead] is NULL, or [size] < 0,  exit !");

	unsigned short seq_num = ntohl(rtphead->seq_num);
	
	rtp_stat->pack_count++;
	rtp_stat->all_pack_size +=size;

	if(!rtp_stat->first_seq_number)
			rtp_stat->first_seq_number = seq_num;

	rtp_stat->last_seq_number = seq_num;
	rtp_stat->ssrc = ntohl(rtphead->ssrc);
	
	return 0;
}

int rtsp_client::rtcp_info_fill(rtp_stat_t *rtp_stat, Rtcp_RR_t * rtcp_head)
{
	PRT_COND((rtp_stat == NULL) || (rtcp_head == NULL), "[rtcp_info_fill]: Input [rtp_stat] or [rtcp_head] is NULL, exit !");

	rtcp_head->comm_head.ver  = 2;
	rtcp_head->comm_head.padb = 1;
	rtcp_head->comm_head.rc = 1;
	rtcp_head->comm_head.pt = RTCP_RR;

	rtcp_head->ssrc = htonl(rtp_stat->ssrc+1);

	rtcp_head->rb.ssrc = htonl(rtp_stat->ssrc);
	rtcp_head->rb.fl_cnpl = 0;
	rtcp_head->rb.ex_hsnr = 20;
	rtcp_head->rb.int_jitter = 0;
	rtcp_head->rb.lsr = htonl(0);
	rtcp_head->rb.delay_snc_lsr = 0;

	return 0;
}

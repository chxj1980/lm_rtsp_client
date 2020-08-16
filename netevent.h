#ifndef __NET_EVENT_H__
#define __NET_EVENT_H__


typedef enum 
{
	TYPE_TCP_RTP_RTCP_RECV = 0,
	TYPE_UDP_RTP_RECV,
	TYPE_UDP_RTCP_RECV,

	TYPE_MAX
}Event_type;

typedef struct
{
	Event_type   curr_event_type;
	unsigned int curr_event_id;
}netevent_info_t;

typedef struct
{ 
	void *	   event_hand;
	bool 	   istcp;
	Event_type e_type;
	int        sock;	
	int        id;
}Event_Handler_t;

class netevent
{
	public:
		netevent();
		~netevent();

		virtual int on_net_event(int type, char *buff, int lenth) = 0;
	public:
		netevent_info_t  netevent_info_m;
		
	private:
				
	
};

#endif

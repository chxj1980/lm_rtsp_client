#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

#include "netevent.h"
#include "Mstring.h"

using namespace std;


typedef struct 
{
	pthread_t       pid;
	void *          handler;
	Event_Handler_t hand;
}Net_hand_t;

class network
{
	public:
		network();
		~network();

		int create_tcp(char *ip,    char *port);
		int create_udp(char *ip,   char *port);
		int send_data_tcp(int sock, char *data, int size);
		int recv_data_tcp(int sock, char *data, int size);

		int start_net_proc(Event_Handler_t &hand);
		int get_sock_info(int sock, char *port);
		
		void set_tcp_state(int     state){ tcp_state = state; }
		int  get_tcp_state(){ return tcp_state; }

		void set_udp_state(int state){ udp_state = state; }
		int  get_udp_state(){ return udp_state; }
		
	private:
		int net_make_addr(char *host, int port, struct sockaddr_in *addr);
		int start_tcp_pth(Event_Handler_t hand);
		int start_udp_pth(Event_Handler_t hand);
		static void *tcp_proc(void *args);
		static void *udp_proc(void *args);
		
	private:
		int  tcp_state;
		int  udp_state;
		netevent  *event;
		Net_hand_t *nethand[2];
};

#endif

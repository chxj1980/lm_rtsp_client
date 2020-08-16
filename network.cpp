
#include "network.h"

network::network()
{

}

network::~network()
{

}

int network::net_make_addr(char *host, int port, struct sockaddr_in *addr)
{
	struct hostent *phe;
	struct sockaddr_in addrv4; 
	addrv4.sin_port = htons((unsigned short)port);
	addrv4.sin_family = AF_INET;
	if ( (addrv4.sin_addr.s_addr = inet_addr(host)) == -1 ) 
	{
		if ( (phe = gethostbyname(host)) == NULL ){
			return -1;
		}
		memcpy(&addrv4.sin_addr, phe->h_addr, phe->h_length);
	}

	printf("[port]:[%d]\n", port);
	
	if ( addr )
		memcpy(addr, &addrv4, sizeof(struct sockaddr_in));
	return 0;
}

int network::create_tcp(char *ip, char *port)
{
	PRT_COND(ip == NULL || port == NULL, "[create_sock]==>Input [ip or port ]  point is NULL, exit !");

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	PRT_COND(sock < 0, "Create socket faile,  exit !");

	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK); //设置sock阻塞.
	
	int  bReuseaddr= 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(int));

	struct sockaddr_in  addr;
	net_make_addr(ip, atoi(port), &addr);

	if( connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in )) < 0)
	{
		perror("connect tcp server failed, error:");
		return -1;	
	}

	tcp_state = 1;

	return sock;
}

int network::create_udp(char *ip, char *port)
{
	PRT_COND(port == NULL, "[create_sock]==>Input [ip or port ]  point is NULL, exit !");
	
	int sock = socket(AF_INET, SOCK_DGRAM, 0);	
	PRT_COND(sock < 0, "[create_udp]:Create socket faile,  exit !");

	int  bReuseaddr= 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr, sizeof(int));

	struct sockaddr_in  addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port)); 
	if(ip == NULL)
		addr.sin_addr.s_addr  =  htonl(INADDR_ANY); 
	else
		addr.sin_addr.s_addr  =  inet_addr(ip); 
   
	if( bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
	{
		printf("bind failed, exit !\n");
		return -1;
	}
	
	udp_state = 1;
	return sock;
}


int network::send_data_tcp(int sock, char *data, int size)
{
	PRT_COND(!data,    "[send_data]:Input [data] point is NULL, exit !");
	PRT_COND(size < 0, "[send_data]:Input [size] is < 0 , exit !");

	int ret = send(sock, data, size, 0);
	PRT_COND(ret < 0, "[send_data]: send data failed, ERROR:");

	printf("\033[0;32m""[send TNT]:[%d]:[%d]:[%s]\n",  ret, size, data);
	return ret;
}

int network::recv_data_tcp(int sock, char *data, int size)
{
	PRT_COND(!data,    "[recv_data_tcp]:Input [data] point is NULL, exit !");
	PRT_COND(size < 0, "[recv_data_tcp]:Input [size] is < 0 , exit !");
	
	int ret = recv(sock, data, size, 0); //阻塞
	PRT_COND(ret < 0, "[recv_data_tcp]: recv data failed, exit !");

	printf( "\033[0;33m""[recv TNT]:[%d]:[%d]:%s\n", ret, size, data);

	printf( "\033[1;37m\n");
	return ret;
}

int network::start_net_proc(Event_Handler_t &hand)
{
	event = (netevent  *)hand.event_hand;

	if(hand.istcp)
		start_tcp_pth(hand);
	else
		start_udp_pth(hand);

	return 0;
}

int network::start_tcp_pth(Event_Handler_t hand)
{	
	pthread_t pid;
	
	Net_hand_t *tmphand = NULL;
	if(nethand[0] == NULL)
	{
		nethand[0] = (Net_hand_t *)malloc(sizeof(Net_hand_t));
		memset(nethand[0], 0, sizeof(Net_hand_t));
		nethand[0]->pid = pid;
		nethand[0]->hand = hand;
		nethand[0]->handler = this;
		tmphand = nethand[0];
	}
	else 
	{
		nethand[1] = (Net_hand_t *)malloc(sizeof(Net_hand_t));
		memset(nethand[1], 0, sizeof(Net_hand_t));
		nethand[1]->pid = pid;
		nethand[1]->hand = hand;
		nethand[1]->handler = this;
		tmphand = nethand[1];
	}

	pthread_create(&pid, NULL, tcp_proc, tmphand);

	return 0;
}

int network::start_udp_pth(Event_Handler_t hand)
{
	pthread_t pid;
	
	Net_hand_t *tmphand = NULL;
	if(nethand[0] == NULL)
	{
		nethand[0] = (Net_hand_t *)malloc(sizeof(Net_hand_t));
		memset(nethand[0], 0, sizeof(Net_hand_t));
		nethand[0]->pid = pid;
		nethand[0]->hand = hand;
		nethand[0]->handler = this;
		tmphand = nethand[0];
	}
	else 
	{
		nethand[1] = (Net_hand_t *)malloc(sizeof(Net_hand_t));
		memset(nethand[1], 0, sizeof(Net_hand_t));
		nethand[1]->pid = pid;
		nethand[1]->hand = hand;
		nethand[1]->handler = this;
		tmphand = nethand[1];
	}

	pthread_create(&pid, NULL, udp_proc,  tmphand);
	return 0;
}

void * network::tcp_proc(void *args)
{
	Net_hand_t *sock_tcp =  (Net_hand_t *)args;
	if(sock_tcp == NULL )
	{
		printf("[tcp_proc]: Input [agrs] is NULL, exit !\n");
		return NULL;
	}

	network   *net = (network *)sock_tcp->handler;
	Event_type e_type = sock_tcp->hand.e_type;
	int sock = sock_tcp->hand.sock;

	char *buff = (char *)malloc(1024 * 400); 
	int lenth = 1024 * 400;
	int ret = 0;
	
	struct timeval tm;
	fd_set recv_fd;
	
	while(net->get_tcp_state())
	{
		FD_ZERO(&recv_fd);
		FD_SET(sock, &recv_fd);

		tm.tv_sec = 0;
		tm.tv_usec = 100 * 1000;
		
		ret = select(sock + 1, &recv_fd,  NULL, NULL, NULL);
		if(ret <= 0){
			printf("-----sock:[%d]--[%d]---\n", sock, lenth);
			continue;
		}
		
		memset(buff, 0, sizeof(buff));
		if(FD_ISSET(sock, &recv_fd))
		{
			if ( (ret = net->recv_data_tcp(sock, buff, lenth)) > 0){
				printf("@@@@@@@@@@@@@------->\n");
				net->event->on_net_event(e_type, buff, ret);
				printf("1111111111111111---->\n");
			}
		}

		usleep(1000 * 200);
	}
}

void * network::udp_proc(void *args)
{
	Net_hand_t *sock_udp =  (Net_hand_t *)args;
	if(sock_udp == NULL )
	{
		printf("[tcp_proc]: Input [agrs] is NULL, exit !\n");
		return NULL;
	}

	network   *net = (network *)sock_udp->handler;
	Event_type e_type = sock_udp->hand.e_type;
	int sock = sock_udp->hand.sock;

	printf("[%p]:type:[%d]:sock:[%d]\n", net, e_type, sock);
	char *buff = (char *)malloc(1024 * 400); 
	int lenth = 1024 * 400;
	int ret = 0;
	
	struct timeval tm;
	fd_set recv_fd;
	
	while(net->get_tcp_state())
	{
		FD_ZERO(&recv_fd);
		FD_SET(sock, &recv_fd);

		tm.tv_sec = 0;
		tm.tv_usec = 100 * 1000;
		
		ret = select(sock + 1, &recv_fd,  NULL, NULL, NULL);
		if(ret <= 0){
			printf("-----sock:[%d]--[%d]---\n", sock, lenth);
			continue;
		}
		
		memset(buff, 0, sizeof(buff));
		if(FD_ISSET(sock, &recv_fd))
		{
			if ( (ret = net->recv_data_tcp(sock, buff, lenth)) > 0){
				net->event->on_net_event(e_type, buff, ret);
			}
		}

		usleep(1000 * 200);
	}
}

int network::get_sock_info(int sock, char *port)
{
	PRT_COND(port == NULL, "[get_sock_info]:[port] is NULL, exit !");
	
	unsigned short tmpport;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	
	if (getsockname(sock, (struct sockaddr *)&addr, (socklen_t *)&len) <0 )
	{
		printf("[getsockname] function failed, ERROR:%m\n");
		return -1;
	}

	tmpport = ntohs(addr.sin_port);
	PRT_COND(tmpport <= 0, "[get_sock_info]:[port] <= 0 error, exit !");

	sprintf(port, "%d", tmpport);
	return 0;
}

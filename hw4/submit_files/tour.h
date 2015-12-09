#define MAX_ROUTE 20
#define RT_PROTO 195
#define IDENTIFICATION "deva_007"
#define MAX_VM_NAME_LEN 10

#define MCAST_IP "224.220.108.121"
#define MCAST_PORT "1987"

struct route{
	char ip_addr[MAX_ROUTE][INET_ADDRSTRLEN];
	int index;
	int total_size;
};

typedef struct route route;

void display_route(route* r);
void send_rt();
int recv_rt(char*);
void send_multicast(SA*,socklen_t);
void populate_route(int , char**);
void send_ping();

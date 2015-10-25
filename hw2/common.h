typedef enum {ack,ping,data} data_type;

//UDP Packet sent across along with data
struct udp_data{
	uint32_t seq;
	uint32_t ts;
	uint32_t rwnd;
	data_type type;
};

struct iAddr{
	char ip_addr[MAXLINE];
	char mask[MAXLINE];
};
typedef struct iAddr iAddr;

struct query_obj{
	struct msghdr msgdata;
	struct udp_data config;
	char buf[MAXLINE];
	struct iovec iovec_send[2];
	struct sockaddr_in sock_addr; //This will be used to know the address of query received
};

typedef struct query_obj query_obj;

void init_query_obj(void* ,socklen_t ,struct query_obj* );

ssize_t recv_udp_data(int sockfd,void*,socklen_t,struct query_obj*);
void send_udp_data(int sockfd,void*,socklen_t,struct query_obj*);
int get_addr_count();
void fill_addr_contents(iAddr *addr);


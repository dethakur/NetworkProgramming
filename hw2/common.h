typedef enum {ack,ping,data} data_type;

//UDP Packet sent across along with data
struct udp_data{
	uint32_t seq;
	uint32_t ts;
	uint32_t rwnd;
	data_type type;
};

struct query_obj{
	struct msghdr msgdata;
	struct udp_data config;
	char buf[MAXLINE];
	struct iovec iovec_send[2];
	struct sockaddr_in sock_addr;
};

void init_query_obj(void* ,socklen_t ,struct query_obj* );

void get_udp_data(int sockfd,void*,socklen_t,struct query_obj*);
void send_udp_data(int sockfd,void*,socklen_t,struct query_obj*);

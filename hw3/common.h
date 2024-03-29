#include "unp.h"
#include "hw_addrs.h"

#define ETH_FRAME_LEN 1518
#define PF_PACK_PROTO "_test0589"
#define RAW_SERVER_PROTO "_test0910"

#define ROUTING_TABLE_SIZE 100
#define IP_LEN 20

#define DOMAIN_SOCK_PATH "/tmp/deva_kau"
#define MAX_MSG_LEN 50
#define MAX_VM_NAME_LEN 10

static char currhostname[50] = "";

struct peer_info {
	char dest_port[100];
	char dest_ip[INET_ADDRSTRLEN];
	char src_port[100];
	char src_ip[INET_ADDRSTRLEN];
	char msg[MAX_MSG_LEN];
	char source_vm[MAX_VM_NAME_LEN];
	int flag;
}__attribute((packed));

typedef enum {
	rrep, rreq, payload_req,payload_resp
} data_type;

struct server_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
};

struct server_buf{
	struct peer_info peer_info;
	int count;
};
typedef struct server_buf server_buf;

struct duplicate_packet{
	char src_mac_addr[6];
	char dest_mac_addr[6];
	data_type type;
	int count;
	int packet_number;
};
typedef struct duplicate_packet duplicate_packet;

struct frame_header {
	char src_ip[IP_LEN];
	char dest_ip[IP_LEN];
	int hop_count;
	int bc_id;
	char msg[100];
	int force_flag;
	data_type type;
}__attribute((packed));
typedef struct frame_header frame_head;

struct routing_table_row {
	char dest_ip[IP_LEN]; //destination interface index
	char next_hop_mac[6];
	int outgoing_index;
	int hop_count;
	long ts;
	int broadcast_id;
	int rrep_sent;
	int is_filled;
}__attribute((packed));
typedef struct routing_table_row routing_table_row;

struct routing_table {
	routing_table_row row[ROUTING_TABLE_SIZE];
}__attribute((packed));

typedef struct routing_table routing_table;

//void get_ip_address(char*,int*);
void init_buffer(server_buf*,int);
void push_data_to_buf(server_buf*,struct peer_info);
void display_header(frame_head*);
void display_mac_addr(char*);
void init_routing_table(routing_table*);
int get_empty_row(routing_table*);
void update_routing_table(routing_table*, frame_head*, char*);
void delete_table_entry(routing_table*, int);
int get_row_entry(routing_table*, char*);
int source_ip_cmp(char*,struct server_details*,int);
int source_mac_cmp(char*,struct server_details*,int);
int should_update_rt(routing_table*, frame_head*);
int populate_server_details(struct server_details*);
void populate_frame_header(char*,char*,int,int,data_type,int,frame_head*);

int msg_send(int socket, char *dest_ip, char * dest_port, char * src_ip,
		char *src_port, char *msg, int flag, struct sockaddr_un * odr_addr_ptr);
int msg_recv(int socket, char *msg, char *src_ip, char *src_port,
		struct sockaddr_un * odr_addr_ptr);
void set_ip(char *host, char *ip);
void set_this_ip(char *this_ip);
void get_data_from_server(frame_head *header_ptr, struct sockaddr_un *servaddr_ptr, int dgramfd, char *src_vm);
int check_duplicate_pac(duplicate_packet*,char*,char*,data_type,int);

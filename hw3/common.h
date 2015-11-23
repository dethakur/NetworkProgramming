#include "hw_addrs.h"

#define ETH_FRAME_LEN 1518
#define PF_PACK_PROTO "_test0589"
#define RAW_SERVER_PROTO "_test0910"

#define ROUTING_TABLE_SIZE 100
#define IP_LEN 20

#define DOMAIN_SOCK_PATH "/tmp/deva_kau"
#define MAX_MSG_LEN 50
#define MAX_VM_NAME_LEN 10

struct peer_info {
	char dest_port[100];
	char dest_ip[INET_ADDRSTRLEN];
	char src_port[100];
	char src_ip[INET_ADDRSTRLEN];
	char msg[MAX_MSG_LEN];
	char source_vm[MAX_VM_NAME_LEN];
}__attribute((packed));

int msg_send(int socket, char *dest_ip, char * dest_port, char * src_ip,
		char *src_port, char *msg, int flag, struct sockaddr_un * odr_addr_ptr);
int msg_recv(int socket, char *msg, char *src_ip, char *src_port,
		struct sockaddr_un * odr_addr_ptr);
void set_ip(char *host, char *ip);
void set_this_ip(char *this_ip);

typedef enum {
	rrep, rreq, payload_req,payload_resp
} data_type;

struct server_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
};

struct server_buf{
	char ip[IP_LEN];
	int count;
};
typedef struct server_buf server_buf;

struct frame_header {
	char src_ip[IP_LEN];
	char dest_ip[IP_LEN];
	int hop_count;
	int bc_id;
	long timeVal;
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
void push_data_to_buf(server_buf*,char*);
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
void populate_frame_header(char*,char*,int,int,data_type,frame_head*);




void set_ip(char *host, char *ip) {
	struct hostent * hptr = gethostbyname(host);
	char **pptr = hptr->h_addr_list;
	Inet_ntop(hptr->h_addrtype, *pptr, ip, INET_ADDRSTRLEN);
}

void set_this_ip(char *this_ip) {
	char hostname[MAX_VM_NAME_LEN] = "";
	gethostname(hostname, MAX_VM_NAME_LEN);
	set_ip(hostname, this_ip);
}

int msg_send(int socket, char *dest_ip, char * dest_port, char * src_ip,
		char *src_port, char *msg, int flag, struct sockaddr_un * odr_addr_ptr) {
	char hostname[MAX_VM_NAME_LEN] = "";
	gethostname(hostname, MAX_VM_NAME_LEN);

	struct peer_info pinfo;
	bzero(&pinfo, sizeof(pinfo));
	strcpy(pinfo.dest_port, dest_port);
	strncpy(pinfo.dest_ip, dest_ip, INET_ADDRSTRLEN);
	strcpy(pinfo.src_port, src_port);
	strcpy(pinfo.src_ip, src_ip);
	strncpy(pinfo.source_vm, hostname, MAX_VM_NAME_LEN);
	strcpy(pinfo.msg, msg);

	int size = sizeof(struct peer_info) + 1;
	char odr_info[size];
	// copy all info into a char sequence to send the same to ODR
	memcpy(odr_info, &pinfo, sizeof(pinfo));
	odr_info[size] = '\0';

	struct peer_info pinfo2;
	bzero(&pinfo2, sizeof(pinfo2));
	memcpy(&pinfo2, odr_info, sizeof(pinfo2));
	printf("time to check\n");
	printf("%s\n", pinfo.dest_port);
	printf("%s\n", pinfo.dest_ip);
	printf("%s\n", pinfo.src_port);
	printf("%s\n", pinfo.src_ip);
	printf("%s\n", pinfo.source_vm);
	printf("%s\n", pinfo.msg);
	printf("%s\n", pinfo2.dest_port);
	printf("%s\n", pinfo2.dest_ip);
	printf("%s\n", pinfo2.src_port);
	printf("%s\n", pinfo2.src_ip);
	printf("%s\n", pinfo2.source_vm);
	printf("%s\n", pinfo2.msg);

	// send all info to ODR
	sendto(socket, odr_info, strlen(odr_info), 0, odr_addr_ptr,
			sizeof(*odr_addr_ptr));
}

int msg_recv(int socket, char *msg, char *src_ip, char *src_port,
		struct sockaddr_un * odr_addr_ptr) {
	int size = sizeof(struct peer_info) + 1;
	char recvline[size];
	struct sockaddr_un odraddr;
	socklen_t odraddrlen;

	Recvfrom(socket, recvline, size, 0, &odraddr, &odraddrlen);

	struct peer_info pinfo;
	bzero(&pinfo, sizeof(pinfo));
	memcpy(&pinfo, recvline, sizeof(pinfo));

	strcpy(msg, pinfo.msg);
	strcpy(src_ip, pinfo.src_ip);
	strcpy(src_port, pinfo.src_port);
}

#include "hw_addrs.h"

#define ETH_FRAME_LEN 1518
#define PF_PACK_PROTO "_test0589"
#define RAW_SERVER_PROTO "_test0910"

#define ROUTING_TABLE_SIZE 100
#define IP_LEN 20

typedef enum {
	rrep, rreq, payload
} data_type;

struct server_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
};

struct frame_header {
	char src_ip[IP_LEN];
	char dest_ip[IP_LEN];
	int hop_count;
	int bc_id;

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


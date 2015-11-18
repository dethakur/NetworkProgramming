#include "common.h"

void display_header(frame_head* head) {
	printf("Src Ip = %s \t", head->src_ip);
	printf("Dest Ip = %s \t", head->dest_ip);
	printf("Hop count = %d \t", head->hop_count);
	printf("BroadCast Id = %d \t", head->bc_id);
	if (head->type == rreq) {
		printf("RREQ received\n");
	}
	if (head->type == rrep) {
		printf("RREP received\n");
	}
	if (head->type == payload) {
		printf("Payload received\n");
	}
}

void display_mac_addr(char* mac_addr) {
	int i = 0;
	char* ptr = mac_addr;
	i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

}
void init_routing_table(routing_table *table) {
	int i = 0;
	for (i = 0; i < ROUTING_TABLE_SIZE; i++) {
		table->row[i].is_filled = 0;
	}
}
int get_empty_row(routing_table *table) {
	int i = 0;
	for (i = 0; i < ROUTING_TABLE_SIZE; i++) {
		if (table->row[i].is_filled == 0) {
			break;
		}
	}
	return i;

}

void display_routing_table(routing_table* table) {
	int i = 0;
	printf("Displaying Routing Table----\n");
	printf("Dest Ip\t\t\tHops\tTS\t\t\tNext Hop\n");
	for (i = 0; i < ROUTING_TABLE_SIZE; i++) {
		if (table->row[i].is_filled != 0) {
			printf("%s\t\t%d\t%d\t\t",
					table->row[i].dest_ip,
					table->row[i].hop_count, table->row[i].ts);
			display_mac_addr(table->row[i].next_hop_mac);
			printf("\n");
		}
	}
	printf("------------xxx------------\n");
}

void add_data_to_table(routing_table* table, frame_head* head,
		char* next_hop_mac) {
	int index = get_empty_row(table);
	table->row[index].is_filled = 1;
	memcpy(table->row[index].dest_ip, head->src_ip, IP_LEN);
	memcpy(table->row[index].next_hop_mac, next_hop_mac, sizeof(char)*6);
	table->row[index].ts = time(NULL);
	table->row[index].rrep_sent = 0;
	table->row[index].hop_count = head->hop_count;

}


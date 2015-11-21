#include "common.h"

void display_header(frame_head* head) {
	printf("Src Ip = %s \t", head->src_ip);
	printf("Dest Ip = %s \t", head->dest_ip);
	printf("Hop count = %d \t", head->hop_count);
//	printf("BroadCast Id = %d \t", head->bc_id);
	if (head->type == rreq) {
		printf("RREQ\n");
	}
	if (head->type == rrep) {
		printf("RREP\n");
	}
	if (head->type == payload_req) {
		printf("Payload Request\n");
	}
	if (head->type == payload_resp) {
		printf("Payload Response\n");
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
		bzero(&table->row[i], sizeof(routing_table_row));
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

int get_row_entry(routing_table *table, char* ip) {
	int i = 0;
	for (i = 0; i < ROUTING_TABLE_SIZE; i++) {
		if (strcmp(&table->row[i].dest_ip, ip) == 0) {
			return i;
		}
	}
	return -1;

}
void delete_table_entry(routing_table *table, int index) {
	bzero(&table->row[index], sizeof(routing_table_row));
	table->row[index].is_filled = 0;
}

void init_buffer(server_buf* buf, int count) {
	int i = 0;
	for (i = 0; i < count; i++) {
		bzero(&buf[i], sizeof(buf[i]));
		buf->count = 0;
	}
}

void push_data_to_buf(server_buf* buf, char* ip) {
	int i = 0;
	for (i = 0; i < 100; i++) {
		if (buf[i].count == 0) {
			break;
		}
		if (strcmp(buf[i].ip, ip) == 0) {
			break;
		}
	}
	if (buf[i].count == 0) {
		buf[i].count += 1;
		strcpy(buf[i].ip, ip);
	} else {
		buf[i].count += 1;
	}
}

void display_routing_table(routing_table* table) {
	int i = 0;
	printf("Displaying Routing Table----\n");
	printf("Dest Ip\t\t\tHops\tTS\t\t\tNext Hop\t\t\tB ID\n");
	for (i = 0; i < ROUTING_TABLE_SIZE; i++) {
		if (table->row[i].is_filled != 0) {
			printf("%s\t\t%d\t%ld\t\t", table->row[i].dest_ip,
					table->row[i].hop_count, table->row[i].ts);
			display_mac_addr(table->row[i].next_hop_mac);

			printf("\t\t%d", table->row[i].broadcast_id);
			printf("\n");
		}
	}
	printf("------------xxx------------\n\n");
}
int should_update_rt(routing_table* table, frame_head* head) {
	int output = 1;
	int row_index = get_row_entry(table, head->src_ip);
	if (row_index == -1) {
		return 1;
	}
	if (head->bc_id > table->row[row_index].broadcast_id) {
		return 1;
	}
	if (table->row[row_index].hop_count > head->hop_count) {
		return 1;
	}
	return -1;
}
void update_routing_table(routing_table* table, frame_head* head,
		char* next_hop_mac) {

	int row_index = get_row_entry(table, head->src_ip);

	if (row_index == -1) {
		printf("No entry for IP = %s. Making entry in Routing Table\n",
				head->src_ip);
		int index = get_empty_row(table);
		table->row[index].is_filled = 1;
		memcpy(table->row[index].dest_ip, head->src_ip, IP_LEN);
		memcpy(table->row[index].next_hop_mac, next_hop_mac, sizeof(char) * 6);
//		memcpy(table->row[index].self_mac, self_mac, sizeof(char) * 6);
		table->row[index].ts = time(NULL);
		table->row[index].broadcast_id = head->bc_id;
		table->row[index].rrep_sent = 0;
		table->row[index].hop_count = head->hop_count;

	} else {
		printf("Source IP = %s EXISTS in Routing Table\n", head->src_ip);
		int b_id = table->row[row_index].broadcast_id;
		int delete = -1;
		if (head->bc_id > b_id) {
			printf("B ID is lesser = %d than %d\n", b_id, head->bc_id);
			table->row[row_index].broadcast_id = head->bc_id;
			delete = 1;
		} else {
			if (table->row[row_index].hop_count > head->hop_count) {
				printf("Hop count is lesser = %d than %d\n",
						table->row[row_index].hop_count, head->hop_count);
				table->row[row_index].hop_count = head->hop_count;
				delete = 1;
			}
			if (delete == 1) {
//				printf("DEST IP = %s deleted! RREQ is sent!\n", head->dest_ip);
			}
		}
	}
}

int source_ip_cmp(char* ip, struct server_details *serv,
		int number_of_interfaces) {
	int found = -1;
	int i = 0;
	for (i = 0; i < number_of_interfaces; i++) {
		if (strcmp(serv[i].ip, ip) == 0) {
			found = i;
		}
	}
	return found;
}

int source_mac_cmp(char* mac, struct server_details *serv,
		int number_of_interfaces) {
	int found = -1;
	int i = 0;
	for (i = 0; i < number_of_interfaces; i++) {
		if (strcmp(serv[i].mac, mac) == 0) {
			found = i;
		}
	}
	return found;
}

int populate_server_details(struct server_details* serv) {
	struct hwa_info *hwa, *hwahead;
	struct sockaddr *sa;
	char *ptr;
	int i, prflag;
	char ip_addr[IP_LEN];
	bzero(ip_addr, sizeof(ip_addr));

	int count = 0;
//	bzero(serv, sizeof(struct server_details));
//	serv[0] = 0;
//	serv[1] = 0;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if ((strcmp(hwa->if_name, "lo") == 0)
				|| (strcmp(hwa->if_name, "eth0") == 0))
			continue;

		prflag = 0;
		i = 0;
		bzero(ip_addr, sizeof(ip_addr));
		if ((sa = hwa->ip_addr) != NULL)
			sprintf(ip_addr, "%s", Sock_ntop_host(sa, sizeof(*sa)));

		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {

			bzero(&serv[count], sizeof(struct server_details));
			strcpy(serv[count].ip, ip_addr);
			memcpy(serv[count].mac, hwa->if_haddr, IF_HADDR);
			serv[count].index = hwa->if_index;

			count++;
			//			break;
		}

	}
	free_hwa_info(hwahead);
	return count;
}

void populate_frame_header(char* src_ip, char* dest_ip, int hops, int b_id,
		data_type type, frame_head* header) {
	bzero(header, sizeof(frame_head));
	strcpy(header->src_ip, src_ip);
	strcpy(header->dest_ip, dest_ip);
	header->hop_count = hops;
	header->bc_id = b_id;
	header->type = type;
}


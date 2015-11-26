#include "odr.h"

//void send_packet(char* dest_ip,char* data);

int main(int argc, char* argv[]) {
	gethostname(currhostname, 50);

	init_routing_table(&table);
	init_buffer(buffer, 100);
	number_of_interfaces = populate_server_details(serv);
//	printf("Number of Interfaces = %d \n", number_of_interfaces);

	unlink(PF_PACK_PROTO);
	unlink(RAW_SERVER_PROTO);

	rawfd = Socket(PF_PACKET, SOCK_RAW, htons(PF_PACK_PROTO));
	dgramfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	fd_set rset;
	bzero(&rset, sizeof(fd_set));

	bzero(&serveraddr, sizeof(struct sockaddr_un));
	serveraddr.sun_family = AF_LOCAL;
	strcpy(serveraddr.sun_path, DOMAIN_SOCK_PATH);

	struct sockaddr_un odraddr;
	bzero(&odraddr, sizeof(struct sockaddr_un));
	odraddr.sun_family = AF_LOCAL;
	strcpy(odraddr.sun_path, RAW_SERVER_PROTO);

	Bind(dgramfd, (SA*) &odraddr, sizeof(odraddr));
	int max_val = max(rawfd, dgramfd) + 1;

	void* output = malloc(ETH_FRAME_LEN);
	bzero(output, ETH_FRAME_LEN);

	int count = 1;
	int time_val = atoi(argv[1]);
	while (1) {
		FD_ZERO(&rset);
		FD_SET(rawfd, &rset);
		FD_SET(dgramfd, &rset);
//		printf("Waiting now!\n");
		struct timeval timeout;
		bzero(&timeout, sizeof(struct timeval));
		timeout.tv_sec = time_val;

		Select(max_val, &rset, NULL, NULL, &timeout);
		bzero(output, ETH_FRAME_LEN);
		if (FD_ISSET(rawfd, &rset)) {
//			printf("Ethernet frame received!!  = %d\n", count);
			Recvfrom(rawfd, output, ETH_FRAME_LEN, 0, NULL, NULL);
			process_frame(output);
			count++;
		} else if (FD_ISSET(dgramfd, &rset)) {
			char output_client[MAXLINE];
//			printf("Request from client received!!  = %d\n", count);
			broadcast_id++;
			packet_counter = packet_counter + 1;
			Recvfrom(dgramfd, output_client, MAXLINE, 0, NULL, NULL);
			struct peer_info peer_info;
//			bzero(&peer_info,sizeof(struct peer_info));

			memcpy(&peer_info, output_client, sizeof(struct peer_info));
			printf("[%s] Dest IP = %s and force flag = %d\n", currhostname,
					peer_info.dest_ip,peer_info.flag);
			int i = 0;
			push_data_to_buf(&buffer, peer_info);
			for (i = 0; i < number_of_interfaces; i++) {
				strcpy(peer_info.dest_ip, peer_info.dest_ip);
				if (peer_info.flag == 1) {
					printf("[Client][%s] Force flag received. Sending RREQ!\n",
							currhostname);
					init_routing_table(&table);
				}
				send_payload(serv[i].ip, peer_info.dest_ip, payload_req, " ",
						peer_info.flag);
//				break;
			}

			count++;
		} else {
			printf("[%s] Stale value reached. Clearing routing table! \n",
					currhostname);
			init_routing_table(&table);
//			init_buffer(buffer, 100);
		}

	}

	return 1;
}

void send_to_client(int dgramfd, char* msg, struct peer_info * pinfo_ptr) {
	struct sockaddr_un cliaddr;
	bzero(&cliaddr, sizeof(struct sockaddr_un));
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, pinfo_ptr->src_port);

	msg_send(dgramfd, "dst_ip", "dst_port", "srcip", "srcport", msg, 0,
			&cliaddr);
}

void process_frame(char* output) {
	char dest_mac[6];
	char src_mac[6];
	frame_head header;
	memcpy(dest_mac, output, 6);
	output = output + 6;
	memcpy(src_mac, output, 6);
	output = output + 6;
	output = output + 2; //Skip the packet size
	memcpy(&header, output, sizeof(frame_head));
	if (header.force_flag == 1) {
		printf("[%s] Force Route bit is set. Forcing Reroute \n", currhostname);
	}

	display_header(&header);

	int update = 0;
	int duplicate = 0;

	int src_index = source_ip_cmp(header.src_ip, serv, number_of_interfaces);
	if (header.type != payload_req && header.type != payload_resp) {
		if (should_update_rt(&table, &header) == 1 && src_index == -1) {
			update_routing_table(&table, &header, src_mac);
			display_routing_table(&table);
			update = 1;
		}
	}
	if (header.type == rreq) {
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		int row_entry = get_row_entry(&table, header.dest_ip);

		if (index == -1 && row_entry == -1 && update == 1) {
			if (duplicate == 1) {
				printf("[%s] Sending RREQ \n", currhostname);
			}
			send_rreq(rawfd, header.bc_id, header.hop_count + 1, header.src_ip,
					header.dest_ip, header.force_flag);
		}
		if (index != -1) {
			printf("[%s] Dest IP is for this VM!\n", currhostname);
//			printf("Now SENDING RREP\n");
			send_rrep(rawfd, header.bc_id, 1, header.src_ip, header.dest_ip);
		} else if (row_entry != -1) {
			send_rrep(rawfd, header.bc_id, 1, header.src_ip, header.dest_ip);
			printf("[%s] Destination Exists in the table!\n", currhostname);

		}

	} else if (header.type == rrep) {

//		printf("Processing RREP!!\n");
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		if (index != -1) {
			printf("[%s] RREP received by Destination!! \n", currhostname);
			send_payload(header.dest_ip, header.src_ip, payload_req, " ", 0);
		} else {

			printf(
					"[%s] RREP not received by Destination!! Forwarding RREP = \n",
					currhostname);
			send_rrep(rawfd, header.bc_id, header.hop_count + 1, header.dest_ip,
					header.src_ip);
		}

	} else if (header.type == payload_req || header.type == payload_resp) {
		int t_i = get_row_entry(&table, header.dest_ip);
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		if (index != -1) {

			if (header.type == payload_req) {
				printf("[%s] Pay load Request received by destination!\n",
						currhostname);
				char source_vm[10];
				ip_to_vm(header.src_ip, source_vm);
				get_data_from_server(&header, &serveraddr, dgramfd, source_vm);
				send_payload(header.dest_ip, header.src_ip, payload_resp,
						&header.msg, 0);
			} else {
				int i = 0;
				for (i = 0; i < 100; i++) {
					//	printf("Comparing %s to %s = %d\n",header.src_ip,buffer[i].peer_info.dest_ip,strcmp(header.src_ip,buffer[i].peer_info.dest_ip));
					if ((strcmp(header.src_ip, buffer[i].peer_info.dest_ip) == 0)
							&& buffer[i].count != 0) {
						printf(
								"[%s] Pay load response received by destination!\n",
								currhostname);
						printf(
								"[%s] Found client to send reply with msg = %s\n",
								currhostname, header.msg);
						send_to_client(dgramfd, header.msg,
								&(buffer[i].peer_info));
						buffer[i].count = 0;
						break;
					}
				}
			}

		} else {
			if (t_i != -1) {
				printf(
						"[%s] Destination is there , forwarding payload with data = %s!\n",
						currhostname, header.msg);
				send_payload(header.src_ip, header.dest_ip, header.type,
						&header.msg, 0);
			} else {
//				printf("[]Destination is not there! Ignoring Payload\n");
			}
		}
	}
}

void send_payload(char* src_ip, char* dest_ip, data_type payload, char* msg,
		int force_flag) {
//	printf("Force flag == %d\N", force_flag);
//	char *dest_ip = &peer_info.dest_ip;
//	printf("Sending Payload to %s with data = %s\n", dest_ip,msg);

	int t_i = get_row_entry(&table, dest_ip);
	if (t_i == -1) {
		printf("[%s] Dest IP %s not in RT. Making RREQ!\n", currhostname,
				dest_ip);
		int i = 0;
		for (i = 0; i < number_of_interfaces; i++) {
			send_rreq(rawfd, broadcast_id, 1, serv[i].ip, dest_ip, force_flag);
		}
	} else {
		int i = 0;
		for (i = 0; i < number_of_interfaces; i++) {
			frame_head header;
			populate_frame_header(src_ip, dest_ip, table.row[t_i].hop_count,
					broadcast_id, payload, force_flag, &header);
			strcpy(&header.msg, msg);

			send_packet(table.row[t_i].next_hop_mac, serv[i].mac, serv[i].index,
					rawfd, &header);

		}

	}
}

void send_rreq(int sockfd, int b_id, int h_count, char* src_ip, char* dest_ip,
		int force_flag) {
	printf("[%s] RREQ Sent\n", currhostname);
	unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	int i = 0;
	for (i = 0; i < number_of_interfaces; i++) {
		frame_head header;

		populate_frame_header(src_ip, dest_ip, h_count, b_id, rreq, force_flag,
				&header);
		send_packet(b_mac, serv[i].mac, serv[i].index, sockfd, &header);
	}

}

void send_rrep(int sockfd, int b_id, int h_count, char* dest_ip, char* src_ip) {
	printf("[%s] RREP src Ip = %s and Dest Ip = %s\n", currhostname, dest_ip,
			src_ip);
	int i = 0;
	int t_i = get_row_entry(&table, dest_ip);
	for (i = 0; i < number_of_interfaces; i++) {
		frame_head header;
		populate_frame_header(src_ip, dest_ip, h_count, b_id, rrep, 0, &header);

		send_packet(table.row[t_i].next_hop_mac, serv[i].mac, serv[i].index,
				sockfd, &header);

	}
}

void send_packet(char* dest_mac, char* src_mac, int if_index, int sockfd,
		frame_head* header) {

//	printf("DATA SENT ");
	int j;
	struct sockaddr_ll socket_address;
	void* buffer = (void*) malloc(ETH_FRAME_LEN);
	unsigned char* etherhead = buffer;
	struct ethhdr *eh = (struct ethhdr *) etherhead;
	unsigned char* data = buffer + 14;

	socket_address.sll_family = PF_PACKET;
	socket_address.sll_protocol = htons(0);
	socket_address.sll_family = PF_PACKET;
	socket_address.sll_ifindex = if_index;
	socket_address.sll_hatype = ARPHRD_ETHER;
	socket_address.sll_pkttype = PACKET_OTHERHOST;
	socket_address.sll_halen = ETH_ALEN;

	socket_address.sll_addr[0] = dest_mac[0];
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];

	/*MAC - end*/
	socket_address.sll_addr[6] = 0x00;/*not used*/
	socket_address.sll_addr[7] = 0x00;/*not used*/

	memcpy((void*) buffer, (void*) dest_mac, ETH_ALEN);
	memcpy((void*) (buffer + ETH_ALEN), (void*) src_mac, ETH_ALEN);
	eh->h_proto = htons(PF_PACK_PROTO);

	memcpy(data, header, sizeof(frame_head));

	int send_result = sendto(sockfd, buffer, ETH_FRAME_LEN, 0,
			(struct sockaddr*) &socket_address, sizeof(socket_address));

	if (send_result == -1) {
		printf("Failed!!!\n");
	} else {
//		printf("Data sent!!!!\n");
	}

}


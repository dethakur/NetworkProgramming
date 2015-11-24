#include "odr.h"

//void send_packet(char* dest_ip,char* data);

int main(int argc, char* argv[]) {
	init_routing_table(&table);
	init_buffer(buffer, 100);
	number_of_interfaces = populate_server_details(serv);
	printf("Number of Interfaces = %d \n", number_of_interfaces);

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
//	if (argc > 1) {
//		int i = 0;
//		for (i = 0; i < number_of_interfaces; i++) {
//			send_payload(serv[i].ip, "192.168.12.4", payload_req);
//		}
//	}
	int count = 1;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(rawfd, &rset);
		FD_SET(dgramfd, &rset);
		printf("Waiting now!\n");

		Select(max_val, &rset, NULL, NULL, NULL);
		bzero(output, ETH_FRAME_LEN);
		if (FD_ISSET(rawfd, &rset)) {
			printf("Ethernet frame received!!  = %d\n", count);
			Recvfrom(rawfd, output, ETH_FRAME_LEN, 0, NULL, NULL);
			process_frame(output);
			count++;
		}
		if (FD_ISSET(dgramfd, &rset)) {
			char output_client[MAXLINE];
			printf("Request from client received!!  = %d\n", count);
			Recvfrom(dgramfd, output_client, MAXLINE, 0, NULL, NULL);
			struct peer_info peer_info;
			memcpy(&peer_info, output_client, sizeof(struct peer_info));
			printf("Dest IP = %s\n", peer_info.dest_ip);
			int i = 0;
			for (i = 0; i < number_of_interfaces; i++) {
				push_data_to_buf(&buffer, peer_info);
				strcpy(peer_info.dest_ip, peer_info.dest_ip);
				send_payload(serv[i].ip, peer_info.dest_ip, payload_req," ");
//				break;
			}

			count++;
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
	printf("DATA RECEIVED ");
	display_header(&header);

	int update = 0;

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
			printf("Sending RREQ again\n");
			send_rreq(rawfd, header.bc_id, header.hop_count + 1, header.src_ip,
					header.dest_ip);
		}
		if (index != -1) {
			printf("Dest IP is for this VM!\n");
			printf("Now SENDING RREP\n");
			send_rrep(rawfd, broadcast_id, 1, header.src_ip, header.dest_ip);
		} else if (row_entry != -1) {
			send_rrep(rawfd, broadcast_id, 1, header.src_ip, header.dest_ip);
			printf("Destination Exists in the table!\n");

		}

	} else if (header.type == rrep) {

		printf("Processing RREP!!\n");
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		if (index != -1) {
			printf("RREP received by Destination!! \n");
			send_payload(header.dest_ip, header.src_ip, payload_req," ");
		} else {

			printf("RREP not received by Destination!! Forwarding RREP = \n");
			send_rrep(rawfd, header.bc_id, header.hop_count + 1, header.dest_ip,
					header.src_ip);
		}

	} else if (header.type == payload_req || header.type == payload_resp) {
		int t_i = get_row_entry(&table, header.dest_ip);
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		if (index != -1) {
			printf("Pay load received by destination!\n");
			if (header.type == payload_req) {
				get_data_from_server(&header, &serveraddr, dgramfd);
				send_payload(header.dest_ip, header.src_ip, payload_resp,
						&header.msg);
			} else {
				printf("Data Received = %s = \n", header.msg);
				int i = 0;
				for (i = 0; i < 100; i++) {
					//	printf("Comparing %s to %s = %d\n",header.src_ip,buffer[i].peer_info.dest_ip,strcmp(header.src_ip,buffer[i].peer_info.dest_ip));
					if ((strcmp(header.src_ip, buffer[i].peer_info.dest_ip) == 0)
							&& buffer[i].count != 0) {
						printf("Found client to send reply with msg = %s\n",
								header.msg);
						send_to_client(dgramfd, header.msg,
								&(buffer[i].peer_info));
						buffer[i].count = 0;
						break;
					}
				}
			}

		} else {
			if (t_i != -1) {
				printf("Destination is there , forwarding payload with data = %s!\n",header.msg);
				send_payload(header.src_ip, header.dest_ip, header.type,&header.msg);
			} else {
				printf("Destination is not there! Ignoring Payload\n");
			}
		}
	}
}

void send_payload(char* src_ip, char* dest_ip, data_type payload, char* msg) {
//	char *dest_ip = &peer_info.dest_ip;
//	printf("Sending Payload to %s with data = %s\n", dest_ip,msg);
	int t_i = get_row_entry(&table, dest_ip);
	if (t_i == -1) {
		printf("Dest IP %s not in RT. Making RREQ!\n", dest_ip);
		int i = 0;
		for (i = 0; i < number_of_interfaces; i++) {
			send_rreq(rawfd, broadcast_id, 1, serv[i].ip, dest_ip);
		}
	} else {
		int i = 0;
		for (i = 0; i < number_of_interfaces; i++) {
			frame_head header;

//			if (payload == payload_resp && (strcmp(msg, "") != 0)){
//
//				printf("String copied to message = %s\n",header.msg);
//			}

			populate_frame_header(src_ip, dest_ip, table.row[t_i].hop_count,
					table.row[t_i].broadcast_id, payload, &header);
			strcpy(&header.msg, msg);
//						printf("Sending Payload to %s with data = %s\n", dest_ip,header.msg);
			send_packet(table.row[t_i].next_hop_mac, serv[i].mac, serv[i].index,
					rawfd, &header);
		}

	}
}

void send_rreq(int sockfd, int b_id, int h_count, char* src_ip, char* dest_ip) {
	printf("RREQ Sent\n");
	unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	int i = 0;
	for (i = 0; i < number_of_interfaces; i++) {
		frame_head header;
		populate_frame_header(src_ip, dest_ip, h_count, b_id, rreq, &header);
		send_packet(b_mac, serv[i].mac, serv[i].index, sockfd, &header);
	}

}

void send_rrep(int sockfd, int b_id, int h_count, char* dest_ip, char* src_ip) {
	printf("RREP src Ip = %s and Dest Ip = %s\n", dest_ip, src_ip);
	int i = 0;
	int t_i = get_row_entry(&table, dest_ip);
	for (i = 0; i < number_of_interfaces; i++) {
		frame_head header;
		populate_frame_header(src_ip, dest_ip, h_count, b_id, rrep, &header);
		send_packet(table.row[t_i].next_hop_mac, serv[i].mac, serv[i].index,
				sockfd, &header);
	}
}

void send_packet(char* dest_mac, char* src_mac, int if_index, int sockfd,
		frame_head* header) {

	printf("DATA SENT ");
	display_header(header);

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


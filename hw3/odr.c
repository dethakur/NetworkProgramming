#include <stdio.h>
#include <sys/socket.h>
#include "common.h"
#include "unp.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

static struct server_details serv[2];
static int number_of_interfaces;
static routing_table table;
static int broadcast_id = 1;

void send_rreq(int, int, int, char*, char*);
void send_packet(char*, char*, int, int, char*, char*, int, int, data_type);
void process_frame(char*);
//void send_packet(char* dest_ip,char* data);

int rawfd, dgramfd;

int main(int argc, char* argv[]) {
	init_routing_table(&table);
	number_of_interfaces = populate_server_details(serv);
	printf("Number of Interfaces = %d \n", number_of_interfaces);

	struct sockaddr_un servaddr;
	unlink(PF_PACK_PROTO);
	unlink(RAW_SERVER_PROTO);

	rawfd = Socket(PF_PACKET, SOCK_RAW, htons(PF_PACK_PROTO));
	dgramfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	fd_set rset;

	bzero(&rset, sizeof(fd_set));
	bzero(&servaddr, sizeof(struct sockaddr_un));

	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, RAW_SERVER_PROTO);

	Bind(dgramfd, (SA*) &servaddr, sizeof(servaddr));
	int max_val = max(rawfd, dgramfd) + 1;

	void* output = malloc(ETH_FRAME_LEN);
	bzero(output, ETH_FRAME_LEN);
	if (argc > 1) {
		send_rreq(rawfd, broadcast_id, 1, serv[0].ip, "192.168.12.4");
	}
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
			printf("Request from client received!!  = %d\n", count);
			Recvfrom(dgramfd, output, ETH_FRAME_LEN, 0, NULL, NULL);
			count++;
		}

	}

	return 1;
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

	display_header(&header);
	printf("Src and destnation macs ");
	display_mac_addr(src_mac);
	display_mac_addr(dest_mac);
	printf("\n");

	int update = 0;

	int src_index = source_ip_cmp(header.src_ip, serv, number_of_interfaces);

	if (should_update_rt(&table, &header) == 1 && src_index == -1) {
		update_routing_table(&table, &header, src_mac);
		display_routing_table(&table);
		update = 1;
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
			int server_index = source_ip_cmp(header.dest_ip, serv,
					number_of_interfaces);
			send_rrep(rawfd, broadcast_id, 1, header.src_ip, header.dest_ip);
		} else if (row_entry != -1) {
			printf("Destination Exists in the table!\n");
//			send_rrep(rawfd, broadcast_id, 1, header->src_ip, serv[index].ip,
//								header->src_mac)
		}

	} else if (header.type == rrep) {

		printf("Processing RREP!!\n");
		int index = source_ip_cmp(header.dest_ip, serv, number_of_interfaces);
		if (index != -1) {
			printf("RREP received by Destination!! \n");
		} else {

//			int i = source_ip_cmp(header.dest_ip, serv,
//					number_of_interfaces);
			printf("RREP not received by Destination!! Forwarding RREP = \n");
			send_rrep(rawfd, header.bc_id, header.hop_count + 1, header.dest_ip,
					header.src_ip);
		}

	}
}

void send_rreq(int sockfd, int b_id, int h_count, char* src_ip, char* dest_ip) {
	printf("RREQ Sent\n");
	unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	int i = 0;
	for (i = 0; i < number_of_interfaces; i++) {
		send_packet(b_mac, serv[i].mac, serv[i].index, sockfd, src_ip, dest_ip,
				b_id, h_count, rreq);
	}

}

void send_rrep(int sockfd, int b_id, int h_count, char* dest_ip, char* src_ip) {
	printf("RREP src Ip = %s and Dest Ip = %s\n", dest_ip, src_ip);

////	printf("Insdide send RREP");
//	int table_index = get_row_entry(&table, dest_ip);
////	int index = source_ip_cmp(src_ip, serv, number_of_interfaces);
//	printf("Src IP = %s", src_ip);
//	printf("RREP sent to Dest IP = %s ", dest_ip);
//	printf(" , Source MAC Addr ");
//	display_mac_addr(serv[server_index].mac);
//	printf(" , Dest MAC Addr ");
//	display_mac_addr(table.row[table_index].next_hop_mac);
//	printf("\n");
	int i = 0;
	int t_i = get_row_entry(&table, dest_ip);
	for (i = 0; i < number_of_interfaces; i++) {
		send_packet(table.row[t_i].next_hop_mac, serv[i].mac, serv[i].index,
				sockfd, src_ip, dest_ip, b_id, h_count, rrep);
	}
//	send_packet(table.row[table_index].next_hop_mac, serv[server_index].mac,
//			serv[server_index].index, sockfd, src_ip, dest_ip, b_id, h_count,
//			rrep);
}

void send_packet(char* dest_mac, char* src_mac, int if_index, int sockfd,
		char* src_ip, char* dest_ip, int bc_id, int h_count, data_type type) {

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

	frame_head header;
	bzero(&header, sizeof(header));
	strcpy(header.src_ip, src_ip);
	strcpy(header.dest_ip, dest_ip);
	header.hop_count = h_count;
	header.bc_id = bc_id;
	header.type = type;

	memcpy(data, &header, sizeof(header));

	int send_result = sendto(sockfd, buffer, ETH_FRAME_LEN, 0,
			(struct sockaddr*) &socket_address, sizeof(socket_address));

	if (send_result == -1) {
		printf("Failed!!!\n");
	} else {
//		printf("Data sent!!!!\n");
	}

}
//void send_packet(char* dest_ip,char* data){
//	int index = get_row_entrt(&table,char*);
//	if(index == -1){
//		send_rreq(rawfd, broadcast_id, 1,dest_ip );
//	}else{
//		print("The data entry is present in the table!!!\n");
//	}
//}

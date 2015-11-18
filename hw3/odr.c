#include <stdio.h>
#include <sys/socket.h>
#include "common.h"
#include "unp.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
void broadcast_data(int);
void send_data(char*, char*,int,int,char*);
void process_frame(char*);
static routing_table table;

int main() {
	init_routing_table(&table);
	int rawfd, dgramfd;
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
	bzero(output,ETH_FRAME_LEN);

	broadcast_data(rawfd);
	int count = 1;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(rawfd, &rset);
		FD_SET(dgramfd, &rset);
		printf("Waiting now!\n");

		Select(max_val, &rset, NULL, NULL, NULL);
		bzero(output,ETH_FRAME_LEN);
		if(FD_ISSET(rawfd,&rset)){
			printf("Ethernet frame received!!  = %d\n",count);
			Recvfrom(rawfd,output,ETH_FRAME_LEN,0,NULL,NULL);
			process_frame(output);
			count++;
		}
		if(FD_ISSET(dgramfd,&rset)){
			printf("Request from client received!!  = %d\n",count);
			Recvfrom(dgramfd,output,ETH_FRAME_LEN,0,NULL,NULL);
			count++;
		}

	}

	return 1;
}
void process_frame(char* output){
	char dest_mac[6];
	char src_mac[6];
	frame_head header;
	memcpy(dest_mac,output,6);
	output = output + 6;
	memcpy(src_mac,output,6);
	output = output + 6;
	output = output + 2; //Skip the packet size
	memcpy(&header,output,sizeof(frame_head));

//	display_header(&header);
//	display_mac_addr(src_mac);
	add_data_to_table(&table,&header,src_mac);
	display_routing_table(&table);
}
void broadcast_data(int sockfd) {
	struct hwa_info *hwa, *hwahead;
	struct sockaddr *sa;
	char *ptr;
	int i, prflag;
	char ip_addr[20];

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if ((strcmp(hwa->if_name, "lo") == 0)
				|| (strcmp(hwa->if_name, "eth0") == 0))
			continue;

		prflag = 0;
		i = 0;
		bzero(ip_addr,sizeof(ip_addr));
		if ((sa = hwa->ip_addr) != NULL)
			sprintf(ip_addr,"%s", Sock_ntop_host(sa, sizeof(*sa)));

		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
		if (prflag) {
			send_data(b_mac, hwa->if_haddr, hwa->if_index, sockfd,ip_addr);
		}

	}
	free_hwa_info(hwahead);

}

void send_data(char* dest_mac, char* src_mac, int if_index, int sockfd,char* src_ip) {


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
	bzero(&header,sizeof(header));
	strcpy(header.src_ip,src_ip);
	strcpy(header.dest_ip,"dest1");
	header.hop_count = 1;
	header.bc_id = 1;
	header.type = rreq;

	memcpy(data,&header,sizeof(header));

	int send_result = sendto(sockfd, buffer, ETH_FRAME_LEN, 0,
			(struct sockaddr*) &socket_address, sizeof(socket_address));

	if (send_result == -1) {
		printf("Failed!!!\n");
	}else{
		printf("Data sent!!!!\n");
	}

}


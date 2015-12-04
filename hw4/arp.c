#include "arp.h"
#define NUM_CACHE_ENTRIES 40

char src_mac[IF_HADDR];
char src_ip[20];
int src_ifindex = -1;
char identification[3] = "dk";
arp_cache_details arp_cache[NUM_CACHE_ENTRIES];

void update_cache_entry(server_details *svd_ptr, int i, int fd) {
	strcpy(arp_cache[i].ip, svd_ptr->ip);
	memcpy(arp_cache[i].mac, svd_ptr->mac, IF_HADDR);
	arp_cache[i].index = svd_ptr->index;
	arp_cache[i].filled = 1;
	if (arp_cache[i].fd != NO_FD) {
		printf(
				"Something wrong, there is another fd already waiting for cache data of ip: %s\n",
				arp_cache[i].ip);
	}
	arp_cache[i].fd = fd;
}

void insert_into_cache(server_details *svd_ptr, int fd) {
	int i = 0;
	for (i = 0; i < NUM_CACHE_ENTRIES && arp_cache[i].filled == 1; i++) {
		if (strcmp(arp_cache[i].ip, svd_ptr->ip) == 0) {
			// update
			update_cache_entry(svd_ptr, i, fd);
			return;
		}
	}
	update_cache_entry(svd_ptr, i, fd);
}

void boot_cache(server_details svd[], arp_cache_details acache[], int n) {
	int i = 0;
	for (i = 0; i < n && svd[i].ip[0] != '\0'; i++) {
		if (src_ifindex == -1) {
			memcpy(src_mac, svd[i].mac, IF_HADDR);
			src_ifindex = svd[i].index;
			strcpy(src_ip, svd[i].ip);
		}
		insert_into_cache(&svd[i], NO_FD);
	}
}

void display_cache() {
	//println();
	printf("Cache contents\n");
	int i;
	for (i = 0; i < NUM_CACHE_ENTRIES && arp_cache[i].filled == 1; i++) {
		//printf("\n**** found sv %s\n", sv[i].ip);
		//display_mac_addr(sv[i].mac);
		printf("(%d)\n", i + 1);
		printf("IP         : %s\n", arp_cache[i].ip);
		printf("HW         : ");
		display_mac_addr(arp_cache[i].mac);
		printf("\nIfindex    : %d\n", arp_cache[i].index);
		printf("Waiting Fd : %d\n", arp_cache[i].fd);

		printf("\n");
	}
	//println();
}

void send_packet(char* dest_mac, char* src_mac, int if_index, int sockfd,
		arp_req_reply* arp_req_reply_ptr) {

	//      printf("DATA SENT ");
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
	eh->h_proto = htons(OUR_PF_PROTOCOL);

	//memcpy(data, header, sizeof(frame_head));
	memcpy(data, arp_req_reply_ptr, sizeof(arp_req_reply));

	Sendto(sockfd, buffer, ETH_FRAME_LEN, 0,
			(struct sockaddr*) &socket_address, sizeof(socket_address));
}

void println() {
	printf("********************************************\n");
}

void print_arp(arp_req_reply *arp_ptr) {
	println();
	if (arp_ptr->op == 1) {
		printf("\t Ethernet frame header and ARP request\t\n");
	} else {
		printf("\t Ethernet frame header and ARP reply\t\n");
	}
	println();
	printf("eth_dest       : ");
	display_mac_addr(arp_ptr->eth_dest);
	printf("\neth_src        : ");
	display_mac_addr(arp_ptr->eth_src);
	printf("\nframe_type     : %d\n", arp_ptr->frame_type);

	printf("hard_type      : %d\n", arp_ptr->hard_type);
	printf("prot_type      : %d\n", arp_ptr->prot_type);
	printf("hard_size      : %d\n", arp_ptr->hard_size);
	printf("prot_size      : %d\n", arp_ptr->prot_size);
	printf("op             : %d\n", arp_ptr->op);
	printf("sender_eth     : ");
	display_mac_addr(arp_ptr->sender_eth);
	printf("\nsender_ip      : %s\n", arp_ptr->sender_ip);
	printf("target_eth     : ");
	display_mac_addr(arp_ptr->target_eth);
	printf("\ntarget_ip      : %s\n", arp_ptr->target_ip);
	printf("identification : %s\n", arp_ptr->identification);
	println();
}

void prepare_arp_req_reply(arp_req_reply *arp_req_ptr, unsigned short op) {
	bzero(arp_req_ptr, sizeof(arp_req_reply));

	memcpy(arp_req_ptr->eth_src, src_mac, IF_HADDR);
	arp_req_ptr->frame_type = 0x0806;
	arp_req_ptr->hard_type = 1;
	arp_req_ptr->prot_type = htons(OUR_PF_PROTOCOL);
	arp_req_ptr->hard_size = IF_HADDR;
	arp_req_ptr->prot_size = sizeof(arp_req_ptr->prot_type);
	arp_req_ptr->op = op;
	memcpy(arp_req_ptr->sender_eth, src_mac, IF_HADDR);

	strcpy(arp_req_ptr->sender_ip, src_ip);
	strcpy(arp_req_ptr->identification, identification);
}

void send_req(int sockfd, char *target_ip, int waiting_fd) {
	unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	arp_req_reply arp_req;
	prepare_arp_req_reply(&arp_req, 1);
	strcpy(arp_req.target_ip, target_ip);

	server_details sv;
	bzero(&sv, sizeof(server_details));
	strcpy(sv.ip, target_ip);
	insert_into_cache(&sv, waiting_fd);

	printf("Sending ARP request\n");
	print_arp(&arp_req);
	send_packet(b_mac, src_mac, src_ifindex, sockfd, &arp_req);
}

void swap(char *a, char *b, int len) {
	char temp[100] = "";
	memcpy(temp, a, len);
	memcpy(a, b, len);
	memcpy(b, temp, len);
}

int send_reply(char *ip, int fd) {
	int i = 0;
	for (i = 0; i < NUM_CACHE_ENTRIES && arp_cache[i].filled == 1; i++) {
		if (strcmp(arp_cache[i].ip, ip) == 0) {
			// reply
			struct hwaddr hwa;
			bzero(&hwa, sizeof(struct hwaddr));
			memcpy(hwa.sll_addr, arp_cache[i].mac, IF_HADDR);
			hwa.sll_ifindex = arp_cache[i].index;
			hwa.sll_hatype = 1;
			hwa.sll_halen = strlen(arp_cache[i].mac);

			char sendline[sizeof(struct hwaddr)] = "";
			memcpy(sendline, &hwa, sizeof(struct hwaddr));

			int fd_to_use = (fd != -1) ? fd : arp_cache[i].fd;
			Write(fd_to_use, sendline, sizeof(struct hwaddr));
			arp_cache[i].fd = NO_FD;
			display_cache();
			return 1;
		}
	}
	return 0;
}

int update_response_in_cache(arp_req_reply *arp_ptr) {
	int i = 0;
	for (i = 0; i < NUM_CACHE_ENTRIES; i++) {
		if (strcmp(arp_cache[i].ip, arp_ptr->sender_ip) == 0) {
			printf("Updating response in cache.\n");
			memcpy(arp_cache[i].mac, arp_ptr->sender_eth, IF_HADDR);
			arp_cache[i].filled = 1;
			return 1;
		}
	}
	return 0;
}

void handle_response(arp_req_reply *arp_ptr) {
	update_response_in_cache(arp_ptr);
	send_reply(arp_ptr->sender_ip, -1);
}

void handle_request(int rawfd, arp_req_reply* arp_ptr) {
	if (strcmp(src_ip, arp_ptr->target_ip) == 0) {
		arp_ptr->op = 2;

		server_details sv;
		bzero(&sv, sizeof(server_details));
		strcpy(sv.ip, arp_ptr->sender_ip);
		memcpy(sv.mac, arp_ptr->sender_eth, IF_HADDR);
		insert_into_cache(&sv, NO_FD);
		display_cache();

		swap(arp_ptr->eth_src, arp_ptr->eth_dest, IF_HADDR);
		swap(arp_ptr->sender_eth, arp_ptr->target_eth, IF_HADDR);
		swap(arp_ptr->sender_ip, arp_ptr->target_ip, IP_LEN);

		memcpy(arp_ptr->eth_src, src_mac, IF_HADDR);
		memcpy(arp_ptr->sender_eth, src_mac, IF_HADDR);

		printf("Sending response\n");
		print_arp(arp_ptr);
		send_packet(arp_ptr->target_eth, src_mac, src_ifindex, rawfd, arp_ptr);
	} else {
		if (!update_response_in_cache(arp_ptr)) {
			printf(
					"Not a request for this host. Sender not in cache. Not performing cache update.\n");
		}
	}
}

void process_frame(int rawfd, char* output, arp_req_reply* arp_req_reply_ptr) {
	char dest_mac[6];
	char src_mac[6];
	char header[100];
	bzero(arp_req_reply_ptr, sizeof(arp_req_reply));

	memcpy(dest_mac, output, 6);
	output = output + 6;
	memcpy(src_mac, output, 6);
	output = output + 6;
	output = output + 2; //Skip the packet size
	memcpy(arp_req_reply_ptr, output, sizeof(arp_req_reply));

	if (strcmp(arp_req_reply_ptr->identification, identification) != 0) {
		printf("Seems like someone else's packet is received\n");
		return;
	}

	print_arp(arp_req_reply_ptr);

	switch (arp_req_reply_ptr->op) {
	case 1:
		handle_request(rawfd, arp_req_reply_ptr);
		break;
	case 2:
		handle_response(arp_req_reply_ptr);
		break;
	default:
		printf("Request not well formed");
	}
}

void boot() {
	server_details sv[20];
	bzero(sv, sizeof(server_details) * 20);
	populate_server_details(sv);

	boot_cache(sv, arp_cache, 20);
	display_cache();
}

int main(int argc, char **argv) {
	boot();

	int listenfd, connfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_un cliaddr, servaddr;

	listenfd = Socket(AF_LOCAL, SOCK_STREAM, 0);

	unlink(DOMAIN_SOCK_PORT);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, DOMAIN_SOCK_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	Listen(listenfd, LISTENQ);

	int rawfd = Socket(PF_PACKET, SOCK_RAW, htons(OUR_PF_PROTOCOL));

	void* recvline = malloc(ETH_FRAME_LEN);
	bzero(recvline, ETH_FRAME_LEN);

	fd_set rset;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(rawfd, &rset);
		FD_SET(listenfd, &rset);
		Select(max(rawfd, listenfd) + 1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(rawfd, &rset)) {
			printf("Received packet on raw socket\n");
			Recvfrom(rawfd, recvline, ETH_FRAME_LEN, 0, NULL, NULL);
			arp_req_reply recvd_arp;
			process_frame(rawfd, recvline, &recvd_arp);
		}
		if (FD_ISSET(listenfd, &rset)) {
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
			char buf[100] = "";
			Read(connfd, buf, 100);
			printf("Received request to get hw addr for ip:%s\n", buf);
			if (send_reply(buf, connfd) == 1) {
				printf("Served request from cache\n");
			} else {
				send_req(rawfd, buf, connfd);
				display_cache();
			}
		}
	}

	free(recvline);
	return 0;
}


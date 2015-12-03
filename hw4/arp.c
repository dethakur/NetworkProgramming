#include "arp.h"
#define NUM_CACHE_ENTRIES 40

char identification[3] = "dk";

arp_cache_details arp_cache[NUM_CACHE_ENTRIES];
char src_mac[IF_HADDR];
char src_ip[20]; // testing only
int src_ifindex = -1;

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
	int i;
	for (i = 0; i < NUM_CACHE_ENTRIES && arp_cache[i].filled == 1; i++) {
		//printf("\n**** found sv %s\n", sv[i].ip);
		//display_mac_addr(sv[i].mac);
		printf("Entry #%d)\n", i + 1);
		printf("IP: %s\n", arp_cache[i].ip);
		printf("HW: ");
		display_mac_addr(arp_cache[i].mac);
		printf("\n");
	}
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

void prepare_arp_req_reply(arp_req_reply *arp_req_ptr, unsigned short op) {
	bzero(arp_req_ptr, sizeof(arp_req_reply));
	strcpy(arp_req_ptr->eth_src, src_mac);
	arp_req_ptr->frame_type = 0x0806;
	arp_req_ptr->hard_type = 1;
	arp_req_ptr->prot_type = htons(OUR_PF_PROTOCOL);
	arp_req_ptr->hard_size = IF_HADDR;
	arp_req_ptr->prot_size = sizeof(arp_req_ptr->prot_type);
	arp_req_ptr->op = op;
	strcpy(arp_req_ptr->sender_eth, src_mac);
	strcpy(arp_req_ptr->sender_ip, src_ip);
}

void send_req(int sockfd, char *target_ip) {
	unsigned char b_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	arp_req_reply arp_req;
	prepare_arp_req_reply(&arp_req, 1);
	strcpy(arp_req.identification, identification);
	strcpy(arp_req.target_ip, target_ip);
	send_packet(b_mac, src_mac, src_ifindex, sockfd, &arp_req);
}

void process_frame(char* output, arp_req_reply* arp_req_reply_ptr) {
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
	switch (arp_req_reply_ptr->op) {
	case 1:
		printf("received arp request for ip: %s\n",
				arp_req_reply_ptr->target_ip);
		break;
	case 2:
		printf("received arp reply from ip: %s\n", arp_req_reply_ptr->sender_ip);
		break;
	default:
		printf("Request not well formed");
	}
}

int main() {
	server_details sv[20];
	bzero(sv, sizeof(server_details) * 20);
	populate_server_details(sv);

	boot_cache(sv, arp_cache, 20);
	display_cache();

	int rawfd = Socket(PF_PACKET, SOCK_RAW, htons(OUR_PF_PROTOCOL));

	// temporary for testing
	if (strcmp(src_ip, "130.245.156.23") == 0) {
		send_req(rawfd, "130.245.156.21");
		printf("sent req..\n");
	}

	void* recvline = malloc(ETH_FRAME_LEN);
	bzero(recvline, ETH_FRAME_LEN);

	fd_set rset;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(rawfd, &rset);
		Select(rawfd + 1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(rawfd, &rset)) {
			printf("Got an ARP request/reply \n");
			Recvfrom(rawfd, recvline, ETH_FRAME_LEN, 0, NULL, NULL);
			arp_req_reply recvd_arp;
			process_frame(recvline, &recvd_arp);
		}
	}

	free(recvline);
	return 0;
}

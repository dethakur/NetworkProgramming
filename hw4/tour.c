#include "common.h"
#include "tour.h"

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define ETH_HDRLEN sizeof(struct ethhdr)
#define IP4_HDRLEN sizeof(struct ip)
#define ICMP_HDRLEN sizeof(struct icmp)

int rt_sock, pg_sock, pf_sock, udp_send_sock, udp_recv_sock;
char src_vm[MAX_VM_NAME_LEN];
char src_ip[INET_ADDRSTRLEN];
route route_el;
int rawfd;
int pgfd;

char dest_ping_ip[INET_ADDRSTRLEN];

static int visited = 0;


void set_icmp(struct icmp *icmp_ptr) {
	//	struct icmp_ptr *icmp_ptr;
	icmp_ptr->icmp_type = ICMP_ECHO;
	icmp_ptr->icmp_code = 0;
	icmp_ptr->icmp_id = getpid();
	icmp_ptr->icmp_seq = 0;
	int datalen = sizeof(struct timeval);
	memset(icmp_ptr->icmp_data, 0xa5, datalen); /* fill with pattern */
	Gettimeofday((struct timeval *) icmp_ptr->icmp_data, NULL);
	//	len = 8 + datalen; /* checksum ICMP header and data */
	int len = sizeof(struct icmp) + datalen;
	icmp_ptr->icmp_cksum = 0;
	icmp_ptr->icmp_cksum = in_cksum((u_short *) icmp_ptr, len);
}

void send_ping_request(char* dst_mac, char* src_mac, char * src_ip,
		char *dest_ip, int if_index, int rawfd) {
	printf("Index = %d src_ip %s, src_mac ",if_index, src_ip);
	display_mac_addr(src_mac);
	printf("\ndst_ip %s, dst_mac ", dest_ip);
	display_mac_addr(dst_mac);
	printf("\n hello..\n ");

	struct icmp *icmp;
	set_icmp(&icmp);

	struct ip header;
	bzero(&header, sizeof(struct ip));
	header.ip_hl = 5;
	header.ip_v = IPVERSION;
	header.ip_tos = 0;
	header.ip_id = htons(IDENTIFICATION);
	header.ip_ttl = MAXTTL;
	header.ip_p = htons(IPPROTO_ICMP);
	header.ip_src.s_addr = inet_addr(src_ip);
	header.ip_dst.s_addr = inet_addr(dest_ip);

	struct sockaddr_ll socket_address;
	//	void* buffer = (void*) malloc(ETH_FRAME_LEN);
	void* buffer = (void*) malloc(IP_MAXPACKET);
//	printf("buffer len %d\n", IP_MAXPACKET);

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

	socket_address.sll_addr[0] = dst_mac[0];
	socket_address.sll_addr[1] = dst_mac[1];
	socket_address.sll_addr[2] = dst_mac[2];
	socket_address.sll_addr[3] = dst_mac[3];
	socket_address.sll_addr[4] = dst_mac[4];
	socket_address.sll_addr[5] = dst_mac[5];

	/*MAC - end*/
	socket_address.sll_addr[6] = 0x00;/*not used*/
	socket_address.sll_addr[7] = 0x00;/*not used*/

//	printf("ETH_ALEN %d\n", ETH_ALEN);
	memcpy((void*) buffer, (void*) dst_mac, ETH_ALEN);
	memcpy((void*) (buffer + ETH_ALEN), (void*) src_mac, ETH_ALEN);
	eh->h_proto = htons(ETH_P_IP);

	// IPv4 header
	memcpy(data, &header, IP4_HDRLEN);

	// ICMP header
	memcpy(data + IP4_HDRLEN, &icmp, ICMP_HDRLEN);

	//	  // ICMP data
	//	  memcpy (data + ETH_HDRLEN + IP4_HDRLEN + ICMP_HDRLEN, data, datalen);

	int frame_length = 6 + 6 + 2 + IP4_HDRLEN + ICMP_HDRLEN + 0;
//	printf("final framelen %d\n", frame_length);
	Sendto(rawfd, buffer, frame_length, 0, (struct sockaddr*) &socket_address,
			sizeof(socket_address));
}

//void alarm_handler() {
//	printf("Trying to ping from vm9 to vm10\n");
//	char src_mac[6];
//	src_mac[0] = 0x00;
//	src_mac[1] = 0x0c;
//	src_mac[2] = 0x29;
//	src_mac[3] = 0xbb;
//	src_mac[4] = 0x12;
//	src_mac[5] = 0xaa;
//
//	char dst_mac[6];
//	dst_mac[0] = 0x00;
//	dst_mac[1] = 0x0c;
//	dst_mac[2] = 0x29;
//	dst_mac[3] = 0xde;
//	dst_mac[4] = 0x6a;
//	dst_mac[5] = 0x62;
//
//	char src_ip[20] = "130.245.156.29";
//	char dest_ip[20] = "130.245.156.20";
//	int val = 1;
//	int on = 1;
//
//	setsockopt(pgfd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
//
//	char host[20];
//	gethostname(host, 20);
//	if (strcmp(host, "vm9") == 0) {
//		printf("Sending ping request\n");
//		send_ping_request(dst_mac, src_mac, src_ip, dest_ip, 2, rawfd);
//	}
//}

//int main(int argc, char** argv) {
//	signal(SIGALRM, alarm_handler);
//	char host[20];
//	gethostname(host, 20);
//	rawfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
//	pgfd = Socket(AF_INET, SOCK_RAW, htons(IPPROTO_ICMP));
//
//	if (strcmp(host, "vm9") == 0) {
//		fd_set rset;
//		while (1) {
//			alarm(1);
//			FD_ZERO(&rset);
//			FD_SET(pgfd, &rset);
//			printf("Waiting to hear a ping response\n");
//			select(max(pgfd, rawfd) + 1, &rset, NULL, NULL, NULL);
//
//			if (FD_ISSET(pgfd, &rset)) {
//				printf("received something on ping socket\n");
//			}
//		}
//	}
//
//}

int main(int argc, char** argv) {

	gethostname(src_vm, MAX_VM_NAME_LEN);
	get_ip_from_host(src_vm, src_ip);

	int val = 1;
	int on = 1;

	rt_sock = Socket(AF_INET, SOCK_RAW, RT_PROTO);
	setsockopt(rt_sock, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
	rawfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	pgfd = Socket(AF_INET, SOCK_RAW, htons(IPPROTO_ICMP));

	struct sockaddr *sasend, sarecv;
	sasend = malloc(sizeof(struct sockaddr));
	socklen_t salen;

	if (visited == 0) {

		bzero(&sarecv, sizeof(sarecv));
		udp_send_sock = Udp_client(MCAST_IP, MCAST_PORT, (void**) &sasend,
				&salen);

		udp_recv_sock = Socket(sasend->sa_family, SOCK_DGRAM, 0);
		Setsockopt(udp_recv_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		memcpy(&sarecv, sasend, salen);
		Bind(udp_recv_sock, &sarecv, salen);
		Mcast_join(udp_recv_sock, sasend, salen, NULL, 0);

		mcast_set_ttl(udp_send_sock, 1);

		visited = 1;

	}

	if (argc > 1) {
		populate_route(argc, argv);
	}

	fd_set rset;

	pthread_t tid;

	while (1) {
		char output[MAXLINE];
		bzero(output, sizeof(output));
		FD_ZERO(&rset);
		FD_SET(rt_sock, &rset);
		FD_SET(udp_recv_sock, &rset);
//		FD_SET(pf_sock, &rset);
//		FD_SET(rawfd, &rset);
		FD_SET(pgfd, &rset);


		int max_val = max(rt_sock, udp_recv_sock);
		max_val = max(max_val,pgfd);
		max_val = max_val + 1;
		printf("Waiting for select!\n");
		Select(max_val, &rset, NULL, NULL, NULL);


		if (FD_ISSET(rt_sock, &rset)) {
			//Data received from Tour!
			struct sockaddr_in recvaddr;
			socklen_t len = sizeof(recvaddr);
			Recvfrom(rt_sock, output, MAXLINE, 0, (SA*) &recvaddr, &len);

			if (recv_rt(output) > 0) {
				struct hwaddr temp_addr;

				strcpy(dest_ping_ip, sock_ntop(&recvaddr, len));
				printf("Data received from IP = %s\n", dest_ping_ip);


				pthread_create(&tid, NULL, &send_ping, NULL);
				pthread_detach(tid);
//				send_ping(dest_ping_ip);

				if (route_el.index >= route_el.total_size) {
//					send_multicast(sasend, salen);
				} else {
					send_rt();
				}
			}
		}

		if (FD_ISSET(pgfd, &rset)) {
			printf("[PING]received something on ping socket\n");
		}

		if (FD_ISSET(udp_recv_sock, &rset)) {

			//Data received from multicast!
			Recvfrom(udp_recv_sock, output, MAXLINE, 0, NULL, NULL);
			printf("Node %s. Received : %s\n", src_vm, output);
			break;
		}
	}
	free(sasend);

	return 0;
}

void send_ping() {
//	signal(SIGALRM, send_ping);

	struct hwaddr src_hwaddr, dest_hwaddr;

	if(strcmp(dest_ping_ip,src_ip) == 0){
		return;
	}
	areq((char*) dest_ping_ip, &dest_hwaddr);
	areq(src_ip, &src_hwaddr);
	while(1){

	send_ping_request(dest_hwaddr.sll_addr, src_hwaddr.sll_addr, src_ip,
			dest_ping_ip, src_hwaddr.sll_ifindex, rawfd);
//	alarm(1);
		sleep(3);

	}
}

void populate_route(int argc, char** argv) {
	get_ip_from_host(src_vm, route_el.ip_addr[0]);
	route_el.total_size = argc;
	int i = 0;
	for (i = 1; i < argc; i++) {
		get_ip_from_host(argv[i], route_el.ip_addr[i]);
	}
	route_el.index += 1;
	send_rt(&route_el);
}

void send_multicast(SA* sadest, socklen_t salen) {
	char output[MAXLINE];
	sprintf(output,
			"<<<< This is %s. Tour has ended. Group members please identify yourselves >>>>",
			src_vm);
	printf("Node %s. Sending : %s\n", src_vm, output);
	Sendto(udp_send_sock, output, strlen(output), 0, sadest, salen);
}

int recv_rt(char* output) {
	struct ip header;

	bzero(&header, sizeof(struct ip));
	memcpy(&header, output, sizeof(header));

	if (header.ip_id != htons(IDENTIFICATION)) {
		//Identification doesnt match. Not our packet!
		return -1;
	}
	output = output + sizeof(header);
	memcpy(&route_el, output, sizeof(route));
	route_el.index += 1;
	return 1;

}
void send_rt() {
	int index = route_el.index;
	char* dest_ip = route_el.ip_addr[route_el.index];

	printf("Sending Request to Dest_ip %s\n", dest_ip);

	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(dest_ip);
	socklen_t s_len = sizeof(addr);

	struct ip header;
	bzero(&header, sizeof(struct ip));
	header.ip_hl = 5;
	header.ip_v = IPVERSION;
	header.ip_tos = 0;
	header.ip_id = htons(IDENTIFICATION);
	header.ip_ttl = MAXTTL;
	header.ip_p = RT_PROTO;
	header.ip_src.s_addr = inet_addr(src_ip);
	header.ip_dst.s_addr = inet_addr(dest_ip);

	int size_packet = sizeof(header) + sizeof(route);
	char *send_data = malloc(size_packet);
	bzero(send_data, sizeof(send_data));
	memcpy(send_data, &header, sizeof(header));
	memcpy(send_data + sizeof(header), &route_el, sizeof(route));

	Sendto(rt_sock, send_data, size_packet, 0, (SA*) &addr, s_len);

}
void display_route(route* r) {
	//	printf("---Dislaying Route Details----\n");
	//	int size = r->total_size;
	//	printf("Total size = %d\n", size);
	//	printf("Current index = %d\n", r->index);
	//	int i = 0;
	//	for (i = 0; i < size; i++) {
	//		printf("Ip address = %s\n", r->ip_addr[i]);
	//	}
}

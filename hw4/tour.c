#include "common.h"
#include "tour.h"

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#define NUM_IPS_PING 20

#define ETH_HDRLEN sizeof(struct ethhdr)
#define IP4_HDRLEN sizeof(struct ip)
#define ICMP_HDRLEN sizeof(struct icmp)

int rt_sock, pg_sock, pf_sock, udp_send_sock, udp_recv_sock;
char src_vm[MAX_VM_NAME_LEN];
char src_ip[INET_ADDRSTRLEN];
route route_el;
int rawfd;
int pgfd;
int seq = 0;
int pid;

volatile int keep_pinging = 1;
pthread_mutex_t mutex;
char* ips_to_ping[];
int ip_to_ping_index = 0;

char dest_ping_ip[INET_ADDRSTRLEN];

static int visited = 0;

void set_icmp(struct icmp *icmp_ptr) {
	//	struct icmp_ptr *icmp_ptr;
	icmp_ptr->icmp_type = ICMP_ECHO;
	icmp_ptr->icmp_code = 0;
	icmp_ptr->icmp_id = pid;
	icmp_ptr->icmp_seq = seq + 1;
	int datalen = sizeof(struct timeval);
	memset(icmp_ptr->icmp_data, 0xa5, datalen); /* fill with pattern */
	Gettimeofday((struct timeval *) icmp_ptr->icmp_data, NULL);
	int len = 8 + datalen; /* checksum ICMP header and data */

	icmp_ptr->icmp_cksum = 0;
	icmp_ptr->icmp_cksum = in_cksum((u_short *) icmp_ptr, len);
	seq++;
}

void send_ping_request(char* dst_mac, char* src_mac, char * src_ip,
		char *dest_ip, int if_index, int rawfd) {

	struct icmp icmp;
	bzero(&icmp, sizeof(icmp));
	set_icmp(&icmp);

	struct ip header;
	bzero(&header, sizeof(struct ip));
	header.ip_hl = 5;
	header.ip_len = htons(sizeof(icmp) + ICMP_HDRLEN);
	header.ip_v = IPVERSION;
	header.ip_tos = 0;
	header.ip_id = htons(0);
	header.ip_ttl = MAXTTL;
	header.ip_p = IPPROTO_ICMP;
	header.ip_src.s_addr = inet_addr(src_ip);
	header.ip_dst.s_addr = inet_addr(dest_ip);
	header.ip_sum = 0;
	header.ip_sum = in_cksum((u_short *) &header, IP4_HDRLEN);

	struct sockaddr_ll socket_address;
	//	void* buffer = (void*) malloc(ETH_FRAME_LEN);
	void* buffer = (void*) malloc(ETH_FRAME_LEN);
	//	printf("buffer len %d\n", IP_MAXPACKET);

	unsigned char* etherhead = buffer;
	struct ethhdr *eh = (struct ethhdr *) etherhead;
	unsigned char* data = buffer + 14;

	socket_address.sll_family = PF_PACKET;
	socket_address.sll_protocol = htons(0);
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
	memcpy(data, &header, sizeof(header));

	// ICMP header
	memcpy(data + sizeof(header), &icmp, sizeof(icmp));

	//	  // ICMP data
	//	  memcpy (data + ETH_HDRLEN + IP4_HDRLEN + ICMP_HDRLEN, data, datalen);

	//	int frame_length = 6 + 6 + 2 + IP4_HDRLEN + ICMP_HDRLEN + 0;
	//	printf("final framelen %d\n", frame_length);
	Sendto(rawfd, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*) &socket_address,
			sizeof(socket_address));
}

void add_ip_to_ping(char *dest_ping_ip) {
	int k = 0;
	for (k = 0; k < ip_to_ping_index; k++) {
		if (strcmp(dest_ping_ip, ips_to_ping[k]) == 0) {
			printf("Already added ip: %s to ping list\n", dest_ping_ip);
			return;
		}
		if (strcmp(src_ip, dest_ping_ip) == 0) {
			printf("Not adding self to ping list\n");
			return;
		}
	}
	strcpy(ips_to_ping[ip_to_ping_index++], dest_ping_ip);

	// although technically ping request is not sent here,
	// we are printing that ping req is sent because this printing
	// has to be done only once per ping target
	struct in_addr ipv4addr;
	inet_pton(AF_INET, dest_ping_ip, &ipv4addr);
	struct hostent *he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	struct addrinfo *ai = Host_serv(he->h_name, NULL, 0, 0);
	char *h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
	printf("PING %s (%s): %d data bytes\n", ai->ai_canonname ? ai->ai_canonname
			: h, h, 56);
}

void proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv, char * src_ip) {
	int hlen1, icmplen;
	double rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (struct ip *) ptr; /* start of IP header */
	hlen1 = ip->ip_hl << 2; /* length of IP header */

	icmp = (struct icmp *) (ptr + hlen1); /* start of ICMP header */
	if ((icmplen = len - hlen1) < 8)
		err_quit("icmplen (%d) < 8", icmplen);

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)
			return; /* not a response to our ECHO_REQUEST */
		if (icmplen < 16)
			err_quit("icmplen (%d) < 16", icmplen);

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		struct in_addr ipv4addr;
		inet_pton(AF_INET, src_ip, &ipv4addr);
		struct hostent *he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
		struct addrinfo *ai = Host_serv(he->h_name, NULL, 0, 0);
		char *h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);

		printf("%d bytes from %s (%s): seq=%u, ttl=%d, rtt=%.3f ms\n", icmplen,
				ai->ai_canonname, h, icmp->icmp_seq, ip->ip_ttl, rtt);

	}
}

int main(int argc, char** argv) {
	pid = (getpid() & 0xffff);
	int k = 0;
	for (k = 0; k < NUM_IPS_PING; k++) {
		ips_to_ping[k] = (char *) malloc(20);
	}

	gethostname(src_vm, MAX_VM_NAME_LEN);
	get_ip_from_host(src_vm, src_ip);

	int val = 1;
	int on = 1;
	int val1 = 1;
	int val2 = 1;
	int val3 = 1;

	rt_sock = Socket(AF_INET, SOCK_RAW, RT_PROTO);
	rawfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	pgfd = Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	setsockopt(pgfd, IPPROTO_IP, IP_HDRINCL, &val1, sizeof(val1));
	setsockopt(rt_sock, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
	setsockopt(rawfd, IPPROTO_IP, IP_HDRINCL, &val2, sizeof(val2));
	setsockopt(rawfd, SOL_SOCKET, SO_REUSEADDR, &val3, sizeof(val3));

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
	pthread_create(&tid, NULL, &send_ping, NULL);
	pthread_detach(tid);
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
		max_val = max(max_val, pgfd);
		max_val = max_val + 1;
		//		printf("Waiting for select!\n");
		Select(max_val, &rset, NULL, NULL, NULL);

		if (FD_ISSET(rt_sock, &rset)) {
			//Data received from Tour!
			struct sockaddr_in recvaddr;
			socklen_t len = sizeof(recvaddr);
			Recvfrom(rt_sock, output, MAXLINE, 0, (SA*) &recvaddr, &len);

			if (recv_rt(output) > 0) {
				struct hwaddr temp_addr;

				strcpy(dest_ping_ip, sock_ntop(&recvaddr, len));
				add_ip_to_ping(dest_ping_ip);
				if (route_el.index >= route_el.total_size) {
					//					send_multicast(sasend, salen);
				} else {
					send_rt();
				}
			}
		}

		if (FD_ISSET(pgfd, &rset)) {

			//			printf("[PING]received something on ping socket\n");
			int len = Recvfrom(pgfd, output, MAXLINE, 0, NULL, NULL);
			char *p = output;

			struct ip ip_hdr;
			memcpy(&ip_hdr, p, sizeof(ip_hdr));

			p = p + sizeof(ip_hdr);

			struct icmp icmp_pkt; //= p + 20;
			memcpy(&icmp_pkt, p, sizeof(icmp_pkt));

			if (icmp_pkt.icmp_id == pid) {
				char src_ip[INET_ADDRSTRLEN];
				char host[20];
				strcpy(src_ip, inet_ntoa(ip_hdr.ip_src));
				get_host_from_ip(src_ip, host);

				struct timeval tval;
				Gettimeofday(&tval, NULL);
//				printf("Ping Received by %s - %s \n", src_ip, host);

				proc_v4(output, len, &tval, src_ip);
			}

		}

		if (FD_ISSET(udp_recv_sock, &rset)) {

			//Data received from multicast!
			Recvfrom(udp_recv_sock, output, MAXLINE, 0, NULL, NULL);
			printf("Node %s. Received : %s\n", src_vm, output);
			pthread_mutex_lock(&mutex);
			keep_pinging = 0;
			pthread_mutex_unlock(&mutex);
			break;
		}
	}
	pthread_exit(tid);
	free(sasend);

	return 0;
}

void send_ping() {
	//	signal(SIGALRM, send_ping);
	while (keep_pinging == 1) {
		pthread_mutex_lock(&mutex);
		int idx = ip_to_ping_index;
		pthread_mutex_unlock(&mutex);
		int k = 0;
		for (k = 0; k < idx; k++) {
			struct hwaddr src_hwaddr, dest_hwaddr;
			areq((char*) ips_to_ping[k], &dest_hwaddr);
			areq(src_ip, &src_hwaddr);

			//			printf("Sending ping to %s!!\n", ips_to_ping[k]);
			send_ping_request(dest_hwaddr.sll_addr, src_hwaddr.sll_addr,
					src_ip, ips_to_ping[k], src_hwaddr.sll_ifindex, rawfd);
			//	alarm(1);
			sleep(1);
		}
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
	sprintf(
			output,
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

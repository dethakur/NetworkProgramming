#include "common.h"
#include "tour.h"

int rt_sock, pg_sock, pf_sock, udp_send_sock, udp_recv_sock;
char src_vm[MAX_VM_NAME_LEN];
char src_ip[INET_ADDRSTRLEN];
route route_el;

static int visited = 0;

int main(int argc, char** argv) {

	gethostname(src_vm, MAX_VM_NAME_LEN);
	get_ip_from_host(src_vm, src_ip);

	int val = 1;
	int on = 1;

	rt_sock = Socket(AF_INET, SOCK_RAW, RT_PROTO);
	setsockopt(rt_sock, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));

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

	while (1) {
		char output[MAXLINE];
		bzero(output, sizeof(output));
		FD_ZERO(&rset);
		FD_SET(rt_sock, &rset);
		FD_SET(udp_recv_sock, &rset);

		int max_val = max(rt_sock, udp_recv_sock) + 1;
		Select(max_val, &rset, NULL, NULL, NULL);

		if (FD_ISSET(rt_sock, &rset)) {
			//Data received from Tour!
			Recvfrom(rt_sock, output, MAXLINE, 0, NULL, NULL);
			if (recv_rt(output) > 0) {
				if (route_el.index >= route_el.total_size) {
					send_multicast(sasend, salen);
				} else {
					send_rt();
				}
			}
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

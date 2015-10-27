#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include	"unpifiplus.h"

void init_query_obj(void* servaddr, socklen_t servlen, struct query_obj* obj) {
	if (servaddr == NULL) {
		//If socket address is not given. Save the local socket address;
		obj->msgdata.msg_name = (SA*) &obj->sock_addr;
	} else {
		obj->msgdata.msg_name = servaddr;
	}
	if (servlen == 0) {
		obj->msgdata.msg_namelen = sizeof(struct sockaddr_in);
	} else {
		obj->msgdata.msg_namelen = servlen;
	}

	obj->iovec_send[1].iov_base = (void*) &obj->config;
	obj->iovec_send[1].iov_len = sizeof(struct udp_data);
	obj->iovec_send[0].iov_base = (void*) &obj->buf;
	obj->iovec_send[0].iov_len = MAXLINE;
	obj->msgdata.msg_iovlen = 2;
	obj->msgdata.msg_iov = (void*) &obj->iovec_send;

}
void send_udp_data(int sockfd, void* servaddr, socklen_t servlen,
		struct query_obj* obj) {
	init_query_obj(servaddr, servlen, obj);
	Sendmsg(sockfd, &obj->msgdata, 0);
}

void recv_udp_data(int sockfd, void* servaddr, socklen_t servlen,
		struct query_obj* obj) {
	init_query_obj(servaddr, servlen, obj);
	Recvmsg(sockfd, &obj->msgdata, 0);
}
int get_addr_count() {
	struct ifi_info *ifi, *ifihead;
	struct sockaddr *sa;
	u_char *ptr;
	int i, family, doaliases;

	doaliases = 0;
	family = AF_INET;

	int count = 0;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 0); ifi != NULL;
			ifi = ifi->ifi_next) {
//		printf("%s: \n", ifi->ifi_name);
		count++;
	}
	free_ifi_info_plus(ifihead);
	return count;
}

void disp_addr_contents(iAddr *addr, int count) {
	int i = 0;
	for (i = 0; i < count; i++) {
		printf("IP address = %s and Mask = %s\n", addr[i].ip_addr,
				addr[i].mask);
	}
}
//Has to be corrected to longest prefix match
int check_addr_local(char* address, iAddr* addr, int ip_addr_count) {
	int i = 0;
	struct sockaddr_in sock1;
	bzero(&sock1, sizeof(struct sockaddr_in));
	Inet_pton(AF_INET, address, &sock1.sin_addr);
	int local = 0;
	in_addr_t maxMask = 0;
	for (i = 0; i < ip_addr_count; i++) {
		struct sockaddr_in sock2, sock3;
		bzero(&sock2, sizeof(struct sockaddr_in));
		bzero(&sock3, sizeof(struct sockaddr_in));
		Inet_pton(AF_INET, addr[i].ip_addr, &sock2.sin_addr);
		Inet_pton(AF_INET, addr[i].mask, &sock3.sin_addr);
		if ((sock1.sin_addr.s_addr & sock3.sin_addr.s_addr)
				== (sock2.sin_addr.s_addr & sock3.sin_addr.s_addr)) {
			if (local == 0) {
				local = i;
			}
			if (sock3.sin_addr.s_addr > maxMask) {
				maxMask = sock3.sin_addr.s_addr;
				local = i;
			}
		}
	}
	if (local == 0) {
		printf("Address NOT Local! \n");
	}
	if (local == 1) {
		printf("Address is LOCAL\n");
	}
	return local;
}
void fill_addr_contents(iAddr *addr) {
	struct ifi_info *ifi, *ifihead;
	struct sockaddr *sa;
	u_char *ptr;
	int i;
	int count;
	count = 0;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 0); ifi != NULL;
			ifi = ifi->ifi_next) {
//		printf("%s: ", ifi->ifi_name);
		if ((sa = ifi->ifi_addr) != NULL) {
			strcpy(addr[count].ip_addr, Sock_ntop_host(sa, sizeof(*sa)));
//			printf("IP Addr = %s\n",Sock_ntop_host(sa, sizeof(*sa)));
		}
		if ((sa = ifi->ifi_ntmaddr) != NULL) {
			strcpy(addr[count].mask, Sock_ntop_host(sa, sizeof(*sa)));
		}
		count++;

	}
	free_ifi_info_plus(ifihead);
}

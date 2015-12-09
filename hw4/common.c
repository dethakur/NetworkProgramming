#include "common.h"

char client_port[20] = "/tmp/tmpcliport_24123";

void display_mac_addr(char* mac_addr) {
	int i = 0;
	char* ptr = mac_addr;
	i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

}

int populate_server_details(struct server_details* serv) {
	struct hwa_info *hwa, *hwahead;
	struct sockaddr *sa;
	char *ptr;
	int i, prflag;
	char ip_addr[IP_LEN];
	bzero(ip_addr, sizeof(ip_addr));

	int count = 0;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if (strcmp(hwa->if_name, "eth0") != 0)
			continue;

		prflag = 0;
		i = 0;
		bzero(ip_addr, sizeof(ip_addr));
		if ((sa = hwa->ip_addr) != NULL)
			sprintf(ip_addr, "%s", Sock_ntop_host(sa, sizeof(*sa)));

		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {

			bzero(&serv[count], sizeof(struct server_details));
			strcpy(serv[count].ip, ip_addr);
			memcpy(serv[count].mac, hwa->if_haddr, IF_HADDR);
			serv[count].index = hwa->if_index;

			count++;
		}

	}
	free_hwa_info(hwahead);
	return count;
}

int areq(char *ip, struct hwaddr *hwa) {
	bzero(hwa, sizeof(struct hwaddr));

	struct sockaddr_un cliaddr, servaddr;
	int fd = Socket(AF_LOCAL, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, DOMAIN_SOCK_PORT);

	Connect(fd, (SA *) &servaddr, sizeof(servaddr));

	Write(fd, ip, strlen(ip) + 1);
	char recvline[sizeof(struct hwaddr)] = "";

	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	struct timeval timeout;
	bzero(&timeout, sizeof(struct timeval));
	timeout.tv_sec = 1;

	Select(fd + 1, &rset, NULL, NULL, &timeout);

	if (FD_ISSET(fd, &rset)) {
		Read(fd, recvline, sizeof(struct hwaddr));

		bzero(hwa, sizeof(struct hwaddr));
		memcpy(hwa, recvline, sizeof(struct hwaddr));

//		printf("Received hw addr ");
		display_mac_addr(hwa->sll_addr);
		printf("\n");
		return 0;
	} else {
		// time-out
		printf("Timeout waiting for reply from arp server.\n");
		return 1;
	}
}

void get_host_from_ip(char* ip,char* host){
	struct in_addr ipv4addr;
	struct hostent* host_val;

	inet_pton(AF_INET, ip, &ipv4addr);
	host_val = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	if(host_val == NULL){
		printf("IP is not found \n");
	}else{
		strcpy(host,host_val->h_name);
//		char **pptr = host_val-	>h_addr_list;
//		for ( ; *pptr != NULL; pptr++){
//			Inet_ntop(host_val->h_addrtype, *pptr, str, INET_ADDRSTRLEN);
//		}
	}
}

//void get_host_from_ip(char* ip,char* host){
//	struct in_addr ipv4addr;
//	struct hostent* host_val;
//
//	inet_pton(AF_INET, ip, &ipv4addr);
//	host_val = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
//	if(host_val == NULL){
//		printf("IP is not found \n");
//	}else{
//		strcpy(host,host_val->h_name);
////		char **pptr = host_val->h_addr_list;
////		for ( ; *pptr != NULL; pptr++){
////			Inet_ntop(host_val->h_addrtype, *pptr, str, INET_ADDRSTRLEN);
////		}
//	}
//}

void get_ip_from_host(char* host,char* str){

	struct hostent* host_val;

	host_val = gethostbyname(host);
	if(host_val == NULL){
		printf("Host name is not found \n");
	}else{
		char **pptr = host_val->h_addr_list;
		for ( ; *pptr != NULL; pptr++){
			Inet_ntop(host_val->h_addrtype, *pptr, str, INET_ADDRSTRLEN);
		}
	}
}

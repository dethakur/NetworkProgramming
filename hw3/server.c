#include "unp.h"
#include "common.h"

void respond(int sockfd, char *hostname) {
	char buff[100] = "", recvline[1000] = "";
	time_t ticks;
	static int bc_id = 0;

	char this_ip[INET_ADDRSTRLEN] = "";
	set_this_ip(this_ip);

	struct sockaddr_un odraddr;
	socklen_t odraddrlen;
	bzero(&odraddr, sizeof(odraddr));
	odraddr.sun_family = AF_LOCAL;
	strcpy(odraddr.sun_path, RAW_SERVER_PROTO);

	fd_set rset;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		Select(sockfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {
			//			Recvfrom(sockfd, recvline, 100, 0, &odraddr, &odraddrlen);
			char client_ip[100];
			char client_port[100];
			msg_recv(sockfd, recvline, client_ip, client_port, &odraddr);
			int k;
			for(k = 0; k < strlen(recvline) && recvline[k] != '#'; k++) {}
			recvline[k] = '\0';
			int bid = atoi(recvline);
			char *src_vm = recvline + k + 1;
			ticks = time(NULL);
			snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
			if (bc_id < bid) {
				printf(
						"Received request: getTime of BID = %s from client ip: %s, port: %s\n",
						recvline, client_ip, client_port);
				bc_id = bid;
				printf("Server at node: '%s' responding to request from VM: %s\n",
						hostname, src_vm);
			}

			//			sendto(sockfd, buff, strlen(buff), 0, &odraddr, sizeof(odraddr));
			msg_send(sockfd, client_ip, client_port, this_ip, DOMAIN_SOCK_PATH,
					buff, 0, &odraddr);

			sleep(1);
		}
	}
}

int main(int argc, char **argv) {
	int sockfd;
	socklen_t len;
	struct sockaddr_un addr1, addr2;

	char hostname[MAX_VM_NAME_LEN] = "";
	gethostname(hostname, MAX_VM_NAME_LEN);

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(DOMAIN_SOCK_PATH); /* OK if this fails */

	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, DOMAIN_SOCK_PATH, sizeof(addr1.sun_path) - 1);

	Bind(sockfd, (SA *) &addr1, SUN_LEN(&addr1));
	printf("Bound\n");

	respond(sockfd, hostname);

	unlink(DOMAIN_SOCK_PATH);
}


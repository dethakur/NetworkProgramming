#include "unp.h"
#include "common.h"

void respond(int sockfd) {
	char buff[100] = "", recvline[1000] = "";
	time_t ticks;

	char this_ip[INET_ADDRSTRLEN] = "";
	set_this_ip(this_ip);

	struct sockaddr_un odraddr;
	socklen_t odraddrlen;
	//todo:uncomment below
	//	bzero(&odraddr, sizeof(odraddr));
	//	odraddr.sun_family = AF_LOCAL;
	//	strcpy(odraddr.sun_path, "ODR_PORT");

	fd_set rset;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		Select(sockfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {
			//			Recvfrom(sockfd, recvline, 100, 0, &odraddr, &odraddrlen);
			char client_ip[100];
			char client_port[100];
			bzero(&odraddr, sizeof(odraddr));
			msg_recv(sockfd, recvline, client_ip, client_port, &odraddr);

			// to directly communicate to client, uncomment below - only for testing
			//			bzero(&odraddr, sizeof(odraddr));
			//			odraddr.sun_family = AF_LOCAL;
			//			strcpy(odraddr.sun_path, client_port);

			printf("Received request: %s from client ip: %s, port: %s\n",
					recvline, client_ip, client_port);

			ticks = time(NULL);
			snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
			printf("Sending response: '%s' to client ip: %s, port: %s\n", buff,
					client_ip, client_port);

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

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(DOMAIN_SOCK_PATH); /* OK if this fails */

	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, DOMAIN_SOCK_PATH, sizeof(addr1.sun_path) - 1);

	Bind(sockfd, (SA *) &addr1, SUN_LEN(&addr1));
	printf("Bound\n");

	respond(sockfd);

	unlink(DOMAIN_SOCK_PATH);
}


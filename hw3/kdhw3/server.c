#include "unp.h"
#include "common.h"

void respond(int sockfd) {
	char buff[100] = "", recvline[1000] = "";
	time_t ticks;

	char this_ip[INET_ADDRSTRLEN] = "";
	set_this_ip(this_ip);

	struct sockaddr_un cliaddr;
	socklen_t cliaddrlen;

	fd_set rset;

	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		Select(sockfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {
			//			Recvfrom(sockfd, recvline, 100, 0, &cliaddr, &cliaddrlen);
			char client_ip[100];
			char client_port[100];
			msg_recv(sockfd, recvline, client_ip, client_port, &cliaddr);

			printf("Received request: %s from client: %s\n", recvline,
					cliaddr.sun_path);

			ticks = time(NULL);
			snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
			printf("Sending response: '%s' to client: %s\n", buff,
					cliaddr.sun_path);

			//			sendto(sockfd, buff, strlen(buff), 0, &cliaddr, sizeof(cliaddr));
			msg_send(sockfd, client_ip, client_port, this_ip, DOMAIN_SOCK_PATH,
					buff, 0, &cliaddr);

			sleep(1);
		}
	}
}

int main(int argc, char **argv) {
	int listenfd;
	socklen_t len;
	struct sockaddr_un addr1, addr2;

	listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(DOMAIN_SOCK_PATH); /* OK if this fails */

	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, DOMAIN_SOCK_PATH, sizeof(addr1.sun_path) - 1);

	Bind(listenfd, (SA *) &addr1, SUN_LEN(&addr1));
	printf("Bound\n");

	respond(listenfd);

	unlink(DOMAIN_SOCK_PATH);
}


#include "unp.h"
#include "common.h"

char client_port[20] = "/tmp/dk_XXXXXX";
void sigint_handle() {
	unlink(client_port);
	exit(1);
}

int main(int argc, char **argv) {
	int sockfd;
	socklen_t odraddrlen;

	atexit(sigint_handle);
	Signal(SIGINT, sigint_handle);

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	mkstemp(client_port);
	unlink(client_port);
	printf("client_port is %s\n", client_port);

	struct sockaddr_un cliaddr, odraddr;

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strncpy(cliaddr.sun_path, client_port, sizeof(cliaddr.sun_path) - 1);
	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

	bzero(&odraddr, sizeof(odraddr));
	odraddr.sun_family = AF_LOCAL;
	strcpy(odraddr.sun_path, RAW_SERVER_PROTO);

	char this_ip[INET_ADDRSTRLEN] = "";
	set_this_ip(this_ip);

	char sendline[20] = "GetMeTime";
	char recvline[30] = "";

	//todo: 1 extra sendto added here. Not sure why the server does not get the client
	// socket details properly on the first call to recvfrom.
	//	Sendto(sockfd, sendline, strlen(sendline), 0, &odraddr, sizeof(odraddr));
//	msg_send(sockfd, "ip", RAW_SERVER_PROTO, this_ip, client_port, sendline, 0,
//			&odraddr);

	char arg[20] = "";
	while (1) {
		printf("\n*** Choose a vm (vm1-vm10) ***\n");
		scanf("%s", arg);
		arg[4] = '\0';

		char ip[INET_ADDRSTRLEN];
		set_ip(arg, ip);
		printf("IP address for vm:%s is %s\n", arg, ip);

		printf("Sending request to server vm:%s\n", arg);
		//		Sendto(sockfd, sendline, strlen(sendline), 0, &odraddr,
		//				sizeof(odraddr));
		msg_send(sockfd, ip, DOMAIN_SOCK_PATH, this_ip, client_port, sendline,
				0, &odraddr);
		printf("Waiting for reply\n");
		//		Recvfrom(sockfd, recvline, 40, 0, &odraddr, &odraddrlen);
		char src_ip[100];
		char src_port[100];
		msg_recv(sockfd, recvline, src_ip, src_port, &odraddr);
		printf("Received response from server:%s\n", recvline);
	}

	printf("unlinking %s\n", client_port);
	unlink(client_port);
}


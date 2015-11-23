#include "unp.h"
#include "common.h"

char tempfile[20] = "/tmp/dk_XXXXXX";
void sigint_handle() {
	unlink(tempfile);
	exit(1);
}

int main(int argc, char **argv) {
	int sockfd;
	socklen_t servaddrlen;

	atexit(sigint_handle);
	Signal(SIGINT, sigint_handle);

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	mkstemp(tempfile);
	unlink(tempfile);
	printf("Tempfile is %s\n", tempfile);

	struct sockaddr_un cliaddr, servaddr;

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strncpy(cliaddr.sun_path, tempfile, sizeof(cliaddr.sun_path) - 1);
	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, DOMAIN_SOCK_PATH);

	char this_ip[INET_ADDRSTRLEN] = "";
	set_this_ip(this_ip);

	char sendline[20] = "GetMeTime";
	char recvline[30] = "";

	//todo: 1 extra sendto added here. Not sure why the server does not get the client
	// socket details properly on the first call to recvfrom.
	//	Sendto(sockfd, sendline, strlen(sendline), 0, &servaddr, sizeof(servaddr));
	msg_send(sockfd, "ip", DOMAIN_SOCK_PATH, this_ip, tempfile, sendline, 0,
			&servaddr);

	char arg[20] = "";
	while (1) {
		printf("\n*** Choose a vm (vm1-vm10) ***\n");
		scanf("%s", arg);
		arg[4] = '\0';

		char ip[INET_ADDRSTRLEN];
		set_ip(arg, ip);
		printf("IP address for vm:%s is %s\n", arg, ip);

		printf("Sending request to server vm:%s\n", arg);
		//		Sendto(sockfd, sendline, strlen(sendline), 0, &servaddr,
		//				sizeof(servaddr));
		msg_send(sockfd, ip, DOMAIN_SOCK_PATH, this_ip, tempfile, sendline, 0,
				&servaddr);
		printf("Waiting for reply\n");
		//		Recvfrom(sockfd, recvline, 40, 0, &servaddr, &servaddrlen);
		char src_ip[100];
		char src_port[100];
		msg_recv(sockfd, recvline, src_ip, src_port, &servaddr);
		printf("Received response from server:%s\n", recvline);
	}

	printf("unlinking %s\n", tempfile);
	unlink(tempfile);
}


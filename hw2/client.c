#include "unp.h"
void dg_cli_new(int,SA*,socklen_t);
int main(int argc,char** argv){
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5589);

	inet_pton(AF_INET,"127.0.0.1",&servaddr.sin_addr);

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	dg_cli_new(sockfd,(SA*)&servaddr,sizeof(servaddr));

	return 0;
}
void dg_cli_new(int sockfd, SA* servaddr,socklen_t servlen){
	int n;
	char sendline[MAXLINE],recvline[MAXLINE];

	strcpy(sendline,"chutiya");
	printf("Comes here !?!\n");




	Connect(sockfd,(SA*)servaddr,servlen);

	while(1){
		printf("Sending = %s\n",sendline);
		Write(sockfd,sendline,strlen(sendline));
		printf("Packet send , now waiting for it!\n");
		n = Read(sockfd,recvline,MAXLINE);
		printf("Received = %s and read li\n",recvline);
		bzero(recvline,sizeof(recvline));
		sleep(1);

	}

}

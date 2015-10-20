#include "unp.h"

void dg_server(int,SA*,socklen_t);
int main(int argc,char* argv[]){
	int sockfd;
	struct sockaddr_in servaddr,cliaddr;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(5589);

	bind(sockfd,(SA*)&servaddr,sizeof(servaddr));

	dg_server(sockfd,(SA*)&cliaddr,sizeof(cliaddr));
	return 0;

}
void dg_server(int sockfd,SA* cliaddr,socklen_t clilen){
	int n;
	socklen_t len;
	char msg[MAXLINE];
	while(1){
		len = clilen;
		n = recvfrom(sockfd,msg,MAXLINE,0,cliaddr,&len);
		printf("Received = %s \n",msg);
		sendto(sockfd,msg,n,0,cliaddr,len);
		bzero(msg,sizeof(msg));
	}
}

#include "unp.h"

//void dg_server(int,SA*,socklen_t);
void receive_msg_test(int,SA*,socklen_t);

static struct ack_data{
	int val;
}receive_hdr;

int main(int argc,char* argv[]){
	int sockfd;
	struct sockaddr_in servaddr,cliaddr;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(5589);

	bind(sockfd,(SA*)&servaddr,sizeof(servaddr));
	receive_msg_test( sockfd,(SA*)&servaddr,sizeof(servaddr));

	return 0;

}

void receive_msg_test(int sockfd,SA* servaddr,socklen_t servlen){
	struct iovec iovec_send[1];
	struct msghdr msgreceive;
	bzero(&msgreceive,sizeof(msgreceive));
	msgreceive.msg_name = servaddr;
	msgreceive.msg_namelen = servlen;
	msgreceive.msg_iov = iovec_send;
	msgreceive.msg_iovlen = 1;

//	receive_hdr.val = 999;

	iovec_send[0].iov_base = (void*)&receive_hdr;
	iovec_send[0].iov_len = sizeof(struct ack_data);
	while(1){
		Recvmsg(sockfd,&msgreceive,0);
		printf("Message received %d !!\n",receive_hdr.val);
//		sleep(1);

	}
}

//void dg_server(int sockfd,SA* cliaddr,socklen_t clilen){
//	int n;
//	socklen_t len;
//	char msg[MAXLINE];
//	while(1){
//		len = clilen;
//		n = recvfrom(sockfd,msg,MAXLINE,0,cliaddr,&len);
//		printf("Received = %s \n",msg);
//		sendto(sockfd,msg,n,0,cliaddr,len);
//		bzero(msg,sizeof(msg));
//	}
//}

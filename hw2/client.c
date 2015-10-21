#include "unp.h"
//void dg_cli_new(int,SA*,socklen_t);
void send_msg_test(int,SA*,socklen_t);

static struct ack_data{
	int val;
}send_hdr;
int main(int argc,char** argv){
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5589);

	inet_pton(AF_INET,"127.0.0.1",&servaddr.sin_addr);

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
//	dg_cli_new(sockfd,(SA*)&servaddr,sizeof(servaddr));
	send_msg_test(sockfd,(SA*)&servaddr,sizeof(servaddr));

	return 0;
}

void send_msg_test(int sockfd,SA* servaddr,socklen_t servlen){
	struct iovec iovec_send[1];
	struct msghdr msgsend;
	bzero(&msgsend,sizeof(msgsend));
	msgsend.msg_name = servaddr;
	msgsend.msg_namelen = servlen;
	msgsend.msg_iov = (void*)&iovec_send;
	msgsend.msg_iovlen = 1;

	send_hdr.val = 999;

	iovec_send[0].iov_base = (void*)&send_hdr;
	iovec_send[0].iov_len = sizeof(struct ack_data);

	Sendmsg(sockfd,&msgsend,0);
}
//void dg_cli_new(int sockfd, SA* servaddr,socklen_t servlen){
//	int n;
//	char sendline[MAXLINE],recvline[MAXLINE];
//
//	strcpy(sendline,"chutiya");
//	printf("Comes here !?!\n");
//
//
//
//
//	Connect(sockfd,(SA*)servaddr,servlen);
//
//	while(1){
//		printf("Sending = %s\n",sendline);
//		Write(sockfd,sendline,strlen(sendline));
//		printf("Packet send , now waiting for it!\n");
//		n = Read(sockfd,recvline,MAXLINE);
//		printf("Received = %s and read li\n",recvline);
//		bzero(recvline,sizeof(recvline));
//		sleep(1);
//
//
//	}
//
//}

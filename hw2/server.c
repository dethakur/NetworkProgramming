#include "unpthread.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>

//void dg_server(int,SA*,socklen_t);
void receive_filename(int);
void send_ack(void*, socklen_t,int);
void send_data(int,char*);
void udp_reliable_transfer(int);

typedef enum {ack,ping,data} data_type;
static struct udp_data{
	uint32_t seq;
	uint32_t ts;
	uint32_t rwnd;
	data_type type;
}send_hdr,recv_hdr;

struct sockaddr_in cliaddr;

int main(int argc,char* argv[]){
	int sockfd;
	struct sockaddr_in servaddr,cliaddr;

	sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(5589);

	Bind(sockfd,(SA*)&servaddr,sizeof(servaddr));

	fd_set rset;
	char buf[MAXLINE];
	while(1){
		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		int maxfd1 = sockfd+1;
		printf("Waiting for select! \n");
		Select(maxfd1,&rset,NULL,NULL,NULL);
		if(FD_ISSET(sockfd,&rset)){
			printf("Select got broken! which means socket got interrupted \n");
			receive_filename(sockfd);
			pid_t pid = fork();
			if(pid == 0){
				udp_reliable_transfer(sockfd);
			}else{
				continue;
			}
		}
	}



//	receive_filename( sockfd,(SA*)&servaddr,sizeof(servaddr));

	return 0;

}

void udp_reliable_transfer(int sockfd){
	int new_sockfd;
	struct sockaddr_in servaddr,sock_addr;
	socklen_t len2 = sizeof(sock_addr);

	bzero(&sock_addr,sizeof(sock_addr));
	bzero(&servaddr,sizeof(servaddr));
	new_sockfd = Socket(AF_INET,SOCK_DGRAM,0);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(0);

	Bind(new_sockfd,(SA*)&servaddr,sizeof(servaddr));

	getsockname(new_sockfd,(SA*)&sock_addr,&len2);

//	Connect(new_sockfd,(SA*)&cliaddr,sizeof(cliaddr));

	int port_number = ntohs(sock_addr.sin_port);
	printf("Connected Port Number = %d\n",port_number);

	char buf[MAXLINE];
	sprintf(buf, "%d", port_number);
	printf("Data send = %s\n",buf);

	while(1){

//		strcpy(buf,"Test1111");
		send_data(sockfd,(char*)&buf);
		sleep(2);
	}
}

void receive_filename(int sockfd){
	socklen_t clilen = sizeof(cliaddr);

	struct iovec iovec_send[2],iovec_recv[1];
	struct msghdr msgrecv;
	char send_buf[MAXLINE],recv_buf[MAXLINE];

	bzero(&msgrecv,sizeof(msgrecv));
	msgrecv.msg_iov = (void*)&iovec_recv;

	msgrecv.msg_name = (SA*)&cliaddr;
	msgrecv.msg_namelen = clilen;

	iovec_recv[0].iov_base = (void*)&recv_buf;
	iovec_recv[0].iov_len = sizeof(recv_buf);
	msgrecv.msg_iov = (void*)&iovec_recv;
	msgrecv.msg_iovlen = 1;


	recvmsg(sockfd,&msgrecv,0);
	printf("[Server][receive_filename] Received = %s\n",recv_buf);
	printf("Sending acknowledgement \n");
	if(msgrecv.msg_name == NULL){
		printf("Null Input \n");
	}else{
		printf("Non Null Input \n");
//		Sendmsg(sockfd,&msgrecv,0);
		send_ack(msgrecv.msg_name,msgrecv.msg_namelen,sockfd);
	}



}
void send_ack(void* addr, socklen_t len,int sockfd){
	struct udp_data ack_data;
	struct iovec iovec_send[1];
	struct msghdr msgrecv;
	bzero(&msgrecv,sizeof(msgrecv));

	ack_data.type=ack;
	msgrecv.msg_name = addr;
	msgrecv.msg_namelen = len;

	iovec_send[0].iov_base = (void*)&ack_data;
	iovec_send[0].iov_len = sizeof(struct udp_data);
	msgrecv.msg_iov = (void*)&iovec_send;
	msgrecv.msg_iovlen = 1;

	Sendmsg(sockfd,&msgrecv,0);
}
//void wait_for_ack(void* addr,socklen_t,int sockfd){
//
//}
void send_data(int sockfd,char* buf){
	printf("[Server] Sending data to the client \n");
	socklen_t clilen = sizeof(cliaddr);
	struct udp_data ack_data;
	struct iovec iovec_send[2];
	struct msghdr msgrecv;
	bzero(&msgrecv,sizeof(msgrecv));

	ack_data.type=data;
	msgrecv.msg_name = (SA*)&cliaddr;
	msgrecv.msg_namelen = clilen;

	iovec_send[0].iov_base = (void*)&ack_data;
	iovec_send[0].iov_len = sizeof(struct udp_data);
	iovec_send[1].iov_base = (void*)buf;
	iovec_send[1].iov_len = sizeof(char)*strlen(buf);
	msgrecv.msg_iov = (void*)&iovec_send;
	msgrecv.msg_iovlen = 2;

	Sendmsg(sockfd,&msgrecv,0);
}

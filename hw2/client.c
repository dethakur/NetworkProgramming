#include "unp.h"
#include <unistd.h>
#include "unpthread.h"
#include <string.h>
#define RWND_SIZE 10
//void dg_cli_new(int,SA*,socklen_t);
int send_file_name(int,SA*,socklen_t);
void send_ack(void*, socklen_t,int);
void receive_file_data(int,void*,socklen_t,char*);
void buffer_reader_thread();
void push_data_to_buffer(char*);
int get_window_size();

typedef enum {ack,ping,data} data_type;

//UDP Packet sent across along with data
static struct udp_data{
	uint32_t seq;
	uint32_t ts;
	uint32_t rwnd;
	data_type type;
}send_hdr,recv_hdr;


//Link List of Buffers
struct buffer{
	char data[MAXLINE];
	uint32_t is_filled;
	struct buffer *next;
};

struct buffer head[RWND_SIZE];



int main(int argc,char** argv){

	int sockfd;
	struct sockaddr_in servaddr,cliaddr,sock_addr;

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5589);

	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(0);

	inet_pton(AF_INET,"127.0.0.1",&servaddr.sin_addr);

	//	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	sockfd = socket(AF_INET,SOCK_DGRAM,0);

	int i=0;
	for(i=0;i<RWND_SIZE;i++){
		head[i].is_filled = -1;
	}

	bind(sockfd,(SA*)&cliaddr,sizeof(cliaddr));

	bzero(&sock_addr,sizeof(sock_addr));
	socklen_t len = sizeof(struct sockaddr_in);
	getsockname(sockfd, (SA*)&sock_addr,&len);
	printf("CLient port = %d\n",ntohs(sock_addr.sin_port));

	Connect(sockfd,(SA*)&servaddr,sizeof(servaddr));

	struct sockaddr_in peer_sock;
	bzero(&peer_sock,sizeof(peer_sock));
	socklen_t len2 = sizeof(peer_sock);


	getpeername(sockfd,(SA*)&peer_sock,&len2);
	printf("Server port = %d\n",ntohs(peer_sock.sin_port));

	char sendline[MAXLINE],recvline[MAXLINE];

	strcpy(sendline,"chutiya");
	printf("Comes here !?!\n");
	//	send_file_name(sockfd,(SA*)&servaddr,sizeof(servaddr));
	Write(sockfd,sendline,strlen(sendline));
	char buf[MAXLINE];
	receive_file_data(sockfd,NULL,0,&buf);
	printf("Data Received 1 = %s",buf);
	receive_file_data(sockfd,NULL,0,&buf);
	char *ptr;
	int new_port_number = strtol(buf,ptr,10);
	printf("New port number = %d\n",new_port_number);

	while(1){
		bzero(&buf,sizeof(buf));
		receive_file_data(sockfd,NULL,0,&buf);
		printf("Data Received = %s\n",buf);
	}

	close(sockfd);
	printf("Writinf done!!\n");

	return 0;

}



void receive_file_data(int sockfd,void* servaddr, socklen_t servlen,char* buf){
	struct msghdr msgrecv;
//	char send_buf[MAXLINE];
	struct iovec iovec_send[2];

	msgrecv.msg_name = NULL;
	msgrecv.msg_namelen = 0;

	iovec_send[0].iov_base = (void*)&send_hdr;
	iovec_send[0].iov_len = sizeof(struct udp_data);
	iovec_send[1].iov_base = (void*)buf;
	iovec_send[1].iov_len = MAXLINE;
	msgrecv.msg_iovlen = 2;
	msgrecv.msg_iov = (void*)&iovec_send;


//	bzero(send_buf,sizeof(send_buf));
//	printf("Trying again!!\n");

	Recvmsg(sockfd,&msgrecv,0);
	if(send_hdr.type == ack){
		printf("Acknowledgement received\n");
	}

	//		push_data_to_buffer(send_buf);
	//		send_ack(servaddr,servlen,sockfd);
	//		printf("[Client] Ack sent to the server!!\n");

	//	}
}

void push_data_to_buffer(char* send_buf){
	int i=0;
	for(i=0;i<RWND_SIZE;i++){
		if(head[i].is_filled == -1){
			strcpy(head[i].data,send_buf);
			head[i].is_filled = 1;
			printf("[Data] Pushed to buffer");
			break;
		}
	}
}
int get_window_size(){
	int count = 0;
	int i=0;
	for(i=0;i<RWND_SIZE;i++){
		if(head[i].is_filled == -1){
			count++;
		}
	}
	return count;
}
void send_ack(void* addr, socklen_t len,int sockfd){
	struct udp_data ack_data;
	struct iovec iovec_send[1];
	struct msghdr msgrecv;
	bzero(&msgrecv,sizeof(msgrecv));

	ack_data.type=ack;
	ack_data.rwnd = get_window_size();
	msgrecv.msg_name = addr;
	msgrecv.msg_namelen = len;

	iovec_send[0].iov_base = (void*)&ack_data;
	iovec_send[0].iov_len = sizeof(struct udp_data);
	msgrecv.msg_iov = (void*)&iovec_send;
	msgrecv.msg_iovlen = 1;

	Sendmsg(sockfd,&msgrecv,0);
}
//
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

//int send_file_name(int sockfd,SA* servaddr,socklen_t servlen){
//	struct iovec iovec_send[2],iovec_recv[2];
//	struct msghdr msgsend,msgrecv;
//	char send_buf[MAXLINE],recv_buf[MAXLINE];
//
//	strcpy(send_buf,"file_test");
//	bzero(&msgsend,sizeof(msgsend));
//	send_hdr.type = data;
//
//	msgsend.msg_name = servaddr;
//	msgsend.msg_namelen = servlen;
//
//	iovec_send[0].iov_base = (void*)&send_hdr;
//	iovec_send[0].iov_len = sizeof(struct udp_data);
//	iovec_send[1].iov_base = (void*)&send_buf;
//	iovec_send[1].iov_len = sizeof(send_buf);
//	msgsend.msg_iovlen = 2;
//	msgsend.msg_iov = (void*)&iovec_send;
//
//	bzero(&msgrecv,sizeof(msgrecv));
//	msgrecv.msg_name = NULL;
//	msgrecv.msg_namelen = 0;
//	msgrecv.msg_iov = (void*)&iovec_recv;
//
//	msgrecv.msg_name = servaddr;
//	msgrecv.msg_namelen = servlen;
//
//	iovec_recv[0].iov_base = (void*)&send_hdr;
//	iovec_recv[0].iov_len = sizeof(struct udp_data);
//	iovec_recv[1].iov_base = (void*)&recv_buf;
//	iovec_recv[1].iov_len = sizeof(recv_buf);
//	msgrecv.msg_iov = (void*)&iovec_recv;
//	msgrecv.msg_iovlen = 2;
//
//
//	Sendmsg(sockfd,&msgsend,0);
//	Recvmsg(sockfd,&msgrecv,0);
//	if(recv_hdr.type == ack){
//		printf("[Client]Ack received From server. Lets now wait for data transfer. \n");
//		//		receive_file_data(sockfd,msgrecv.msg_name,msgrecv.msg_namelen);
//		return 1;
//	}
//	return -1;
//}

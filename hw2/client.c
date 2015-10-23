#include "unp.h"
#include <unistd.h>
#include "unpthread.h"
#include "common.h"
#include <string.h>

#define RWND_SIZE 10
//void dg_cli_new(int,SA*,socklen_t);
void buffer_reader_thread();
void push_data_to_buffer(char*);
int get_window_size();
void create_new_connection(int);




//Link List of Buffers
struct udp_data send_hdr,recv_hdr;
struct buffer{
	char data[MAXLINE];
	uint32_t is_filled;
	struct buffer *next;
};

struct buffer text_buffer[RWND_SIZE];



int main(int argc,char** argv){

	int sockfd;
	struct sockaddr_in servaddr,cliaddr,sock_addr;
	query_obj q_obj;

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
		text_buffer[i].is_filled = -1;
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

	strcpy(sendline,"file_name");
	printf("Comes here !?!\n");

	Write(sockfd,sendline,strlen(sendline));


	do{
		bzero(&q_obj,sizeof(query_obj));
		recv_udp_data(sockfd,NULL,0,&q_obj);
		printf("Type of data = %d = ",q_obj.config.type);
	}while(q_obj.config.type != ack);

	printf("Ack was received!!\n");

	do{
		bzero(&q_obj,sizeof(query_obj));
		recv_udp_data(sockfd,NULL,0,&q_obj);
		printf("Port Number = %s!!\n",q_obj.buf);
		printf("Data type = %d!!\n",q_obj.config.type);

	}while(q_obj.config.type != data);

	long *ptr;
	int new_port_number = strtol(q_obj.buf,ptr,10);


	create_new_connection(new_port_number);
	printf("[Client]Done creating connection with new server");

	close(sockfd);
	printf("Writinf done!!\n");

	return 0;

}
void create_new_connection(int port_num){
	printf("New port number = %d\n",port_num);
	int sockfd;
	struct sockaddr_in servaddr;

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	query_obj q_obj;
	bzero(&q_obj,sizeof(q_obj));
//	Connect(sockfd,(SA*)&servaddr,sizeof(servaddr));

	send_udp_data(sockfd,(SA*)&servaddr,sizeof(servaddr),&q_obj);
	char sendline[MAXLINE];

	strcpy(sendline,"chutiya");
	printf("Comes here !?!\n");
	//	send_file_name(sockfd,(SA*)&servaddr,sizeof(servaddr));
	//This code has to be changed to acknowledgement.
//	send
//	Write(sockfd,sendline,strlen(sendline));

	fd_set rset;

//	query_obj q_obj;

	while(1){
		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		int maxfd1 = sockfd  +1;
		printf("[Client] Waiting for Data from server\n");
		Select(maxfd1,&rset,NULL,NULL,NULL);
		if(FD_ISSET(sockfd,&rset)){
			bzero(&q_obj,sizeof(query_obj));
			recv_udp_data(sockfd,NULL,0,&q_obj);

			printf("Select interrupted in client!!!. Data received = %s\n",q_obj.buf);

//			bzero(&q_obj.buf,sizeof(q_obj.buf));
			q_obj.config.type == ack;
			q_obj.config.seq = 99;
			strcpy(q_obj.buf,"Hello!!");
//			init_query_obj(sockfd,(SA*)&q_obj.sock_addr,sizeof(q_obj.sock_addr),q_obj);
//			init_query_obj((SA*)&q_obj.sock_addr,sizeof(q_obj.sock_addr),&q_obj);
			char sendline[MAXLINE];
			strcpy(sendline,"chutiya");
//			Write(sockfd,&q_obj.msgdata,sizeof(q_obj.msgdata));
			send_udp_data(sockfd,(SA*)&q_obj.sock_addr,sizeof(q_obj.sock_addr),&q_obj);



		}
	}
}

void push_data_to_buffer(char* send_buf){
	int i=0;
	for(i=0;i<RWND_SIZE;i++){
		if(text_buffer[i].is_filled == -1){
			strcpy(text_buffer[i].data,send_buf);
			text_buffer[i].is_filled = 1;
			printf("[Data] Pushed to buffer");
			break;
		}
	}
}
int get_window_size(){
	int count = 0;
	int i=0;
	for(i=0;i<RWND_SIZE;i++){
		if(text_buffer[i].is_filled == -1){
			count++;
		}
	}
	return count;
}

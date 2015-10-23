#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>

void udp_reliable_transfer(int,struct sockaddr_in);
void* send_file_data(void*);
void* receive_ack(void*);

struct thread_config{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int sockfd;
};
typedef struct thread_config th_config;
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
	query_obj q_obj;
	while(1){
		FD_ZERO(&rset);
		FD_SET(sockfd,&rset);
		int maxfd1 = sockfd+1;
		printf("[Parent][Server]Waiting for select! \n");
		Select(maxfd1,&rset,NULL,NULL,NULL);
		if(FD_ISSET(sockfd,&rset)){
			printf("[Parent][Server]Select got broken! which means socket got interrupted \n");
			struct sockaddr_in cliaddr;
			bzero(&q_obj,sizeof(q_obj));
			recv_udp_data(sockfd,NULL,0,&q_obj);
			cliaddr = q_obj.sock_addr;

			bzero(&q_obj,sizeof(q_obj));
			q_obj.config.type = ack;
			send_udp_data(sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);

			pid_t pid = fork();
			if(pid == 0){
				udp_reliable_transfer(sockfd,cliaddr);
				exit(0);
			}else{
				continue;
			}
		}
	}
	return 0;

}

void udp_reliable_transfer(int sockfd,struct sockaddr_in cliaddr){
	int new_sockfd;
	query_obj q_obj;
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

	int port_number = ntohs(sock_addr.sin_port);
	printf("Connected Port Number = %d\n",port_number);

	bzero(&q_obj,sizeof(q_obj));
	sprintf(q_obj.buf, "%d", port_number);
	printf("Data send = %s\n",q_obj.buf);

	//Send port to the client
	q_obj.config.type = data;
	send_udp_data(sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);


	//Wait for acknowledgement. Use select.

	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(new_sockfd,&rset);
	int maxfd1 = new_sockfd+1;
	printf("[Child][Server]Waiting for select! \n");
	Select(maxfd1,&rset,NULL,NULL,NULL);
	if(FD_ISSET(new_sockfd,&rset)){
		bzero(&q_obj,sizeof(q_obj));
		recv_udp_data(new_sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);
		//Ack Received!
		printf("[Child][Server] Interrupted!!! Ack Received\n");
	}
	close(sockfd);

	pthread_t tid1;
	pthread_t tid2;

	th_config config;
	config.sockfd = new_sockfd;
	config.cliaddr = cliaddr;
	config.clilen = sizeof(cliaddr);

	Pthread_create(&tid1,NULL,&send_file_data,&config);
	Pthread_create(&tid2,NULL,&receive_ack,&config);

	pthread_join(tid1,NULL);
	printf("UDP connection done\n");

}
void* send_file_data(void* arg){

	th_config config;
	int seq_num = 0;
	config = *((th_config*)(arg));
	query_obj q_obj;
	while(1){
		bzero(&q_obj,sizeof(q_obj));
		strcpy(q_obj.buf,"success");
		q_obj.config.type = data;
		q_obj.config.seq = seq_num;
		printf("[Server] Data sent to server\n");
		send_udp_data(config.sockfd,(SA*)&config.cliaddr,config.clilen,&q_obj);
		seq_num+=1;
		sleep(1);
	}

}
void* receive_ack(void* arg){
	th_config config;
	config = *((th_config*)(arg));
	query_obj q_obj;
	while(1){
		bzero(&q_obj,sizeof(q_obj));
		strcpy(q_obj.buf,"success");
		q_obj.config.type = data;

		recv_udp_data(config.sockfd,(SA*)&config.cliaddr,config.clilen,&q_obj);
		printf("[Server][ACK]Window Size = %d and Seq Num = %d\n",q_obj.config.rwnd,q_obj.config.seq);

	}
}

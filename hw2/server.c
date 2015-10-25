#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>

void udp_reliable_transfer(int,struct sockaddr_in);
void* send_file_data(void*);
void* receive_ack(void*);
//void find_addresses(iAddr*);

struct thread_config{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int sockfd;
	uint32_t rwnd;
};
typedef struct thread_config th_config;
int main(int argc,char* argv[]){

	int count = get_addr_count();
	iAddr addr[count];
	fill_addr_contents((iAddr*)&addr);
	int i=0;

	fd_set rset_arr;
	int sockfd_arr[count];

	for(i=0;i<count;i++){

		struct sockaddr_in servaddr,cliaddr;

		sockfd_arr[i] = Socket(AF_INET,SOCK_DGRAM,0);

		bzero(&servaddr,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(5589);

		Bind(sockfd,(SA*)&servaddr,sizeof(servaddr));

		printf("IP addr = %s and mask = %s \n",addr[i].ip_addr,addr[i].mask);
	}

	while(1){
		int i=0;
		int maxVal = -1
		FD_ZERO(&rset_arr);
		for(i=0;i<count;i++){
			FD_SET(sockfd_arr[i],&rset_arr);
			maxVal = max(maxVal,sockfd_arr[i]);
		}
		maxVal = maxVal +1 ;
		Select(maxVal,&rset_arr,NULL,NULL,NULL);

	}


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
			}
//			else{
//				continue;
//			}
		}
	}
	return 0;

}
void udp_reliable_transfer(int sockfd,struct sockaddr_in cliaddr){
	pthread_t tid1;
	pthread_t tid2;

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


	th_config config;
	config.sockfd = new_sockfd;
	config.cliaddr = cliaddr;
	config.clilen = sizeof(cliaddr);
	config.rwnd = -1;

	Pthread_create(&tid1,NULL,&send_file_data,&config);
	Pthread_create(&tid2,NULL,&receive_ack,&config);

	pthread_join(tid1,NULL);
	printf("UDP connection done\n");

}
void* send_file_data(void* arg){

	th_config *config;
	int seq_num = -1;
	config = (th_config*)(arg);
	query_obj q_obj;
	int sleep_time = 1;
	while(1){
		int rwnd = config->rwnd;
		bzero(&q_obj,sizeof(q_obj));
		if(rwnd != 0){
			q_obj.config.type = data;
			strcpy(q_obj.buf,"success");
			q_obj.config.seq = seq_num;
			seq_num+=1;
			sleep_time = 1;
			printf("[Data] Data sent to server with rwnd size = %d\n",config->rwnd);
		}else{
			q_obj.config.type = ping;
			sleep_time = 1;
			printf("[Ping] Data sent to server with rwnd size = %d\n",config->rwnd);
		}

		q_obj.config.seq = seq_num;

		send_udp_data(config->sockfd,(SA*)&config->cliaddr,config->clilen,&q_obj);

		sleep(sleep_time);
	}

}
void* receive_ack(void* arg){
	th_config *config;
	config = (th_config*)(arg);
	query_obj q_obj;
	while(1){
		bzero(&q_obj,sizeof(q_obj));
		strcpy(q_obj.buf,"success");
		q_obj.config.type = data;

		recv_udp_data(config->sockfd,(SA*)&config->cliaddr,config->clilen,&q_obj);
		printf("[ACK]Window Size = %d and Seq Num = %d\n",q_obj.config.rwnd,q_obj.config.seq);
		config->rwnd = q_obj.config.rwnd;

	}
}

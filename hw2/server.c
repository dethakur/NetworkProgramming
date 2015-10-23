#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>



//struct udp_data send_hdr;
//struct udp_data recv_hdr;
//struct sockaddr_in cliaddr;

//void dg_server(int,SA*,socklen_t);
void receive_filename(int);
void udp_reliable_transfer(int,struct sockaddr_in);





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
	struct query_obj q_obj;
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
			get_udp_data(sockfd,NULL,0,&q_obj);
			cliaddr = q_obj.sock_addr;

			bzero(&q_obj,sizeof(q_obj));
			q_obj.config.type = ack;
			send_udp_data(sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);

			pid_t pid = fork();
			if(pid == 0){
				udp_reliable_transfer(sockfd,cliaddr);
			}else{
				continue;
			}
		}
	}
	return 0;

}

void udp_reliable_transfer(int sockfd,struct sockaddr_in cliaddr){
	int new_sockfd;
	struct query_obj q_obj;
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

	//Wait for acknowledgement

	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(new_sockfd,&rset);
	int maxfd1 = new_sockfd+1;
	printf("[Child][Server]Waiting for select! \n");
	Select(maxfd1,&rset,NULL,NULL,NULL);
	if(FD_ISSET(new_sockfd,&rset)){
		bzero(&q_obj,sizeof(q_obj));
		get_udp_data(new_sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);
//		receive_data(new_sockfd,&data,&buf);

		printf("[Child][Server] Interrupted!!! Ack Received\n");
	}
	close(sockfd);

	while(1){
		bzero(&q_obj,sizeof(q_obj));
		strcpy(q_obj.buf,"success");
		q_obj.config.type = data;
		send_udp_data(new_sockfd,(SA*)&cliaddr,sizeof(cliaddr),&q_obj);
//		send_data(new_sockfd,buf);
		sleep(1);
	}


}

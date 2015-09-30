#include "unp.h"


void time_server(int,int,int);
int main(int argc,char *argv[]){
	int cli_parent_fd = strtol(argv[2],NULL,10);
	int sockfd,confd;
	int port_num = 5590;
	char feedback[MAXLINE];

	struct sockaddr_in servaddr;

	//Create a TCP socket that will connect to the port of time server.
	sockfd = Socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);

	Inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

	//Connect to the server using the socket just created.
	confd = connect(sockfd,(SA*)&servaddr,sizeof(servaddr));
	if(confd < 0){
		//If connection with the server fails.
		bzero(feedback,sizeof(feedback));
		strcpy(feedback,"[Client][Time]Server could not be reached\n");
		Write(cli_parent_fd,feedback,strlen(feedback));
		close(cli_parent_fd);
		exit(0);
	}

	time_server(cli_parent_fd,sockfd,port_num);
	close(sockfd);
	close(cli_parent_fd);
	close(confd);
	exit(0);

}

//This function does the main work. It reads response from time server and prints
//it to stdout.
void time_server(int cli_parent_fd,int sockfd,int port_num){

	fd_set rset;
	int maxfd1;
	char feedback[MAXLINE];
	char buf[MAXLINE];
	int nread;
	FD_ZERO(&rset);

	while(1){
		FD_SET(sockfd,&rset);
		maxfd1 = max(0,sockfd)+1;

		Select(maxfd1,&rset,NULL,NULL,NULL);

		//If a response is received from the server.
		if(FD_ISSET(sockfd,&rset)){
			bzero(buf,sizeof(buf));

			//Read the response frm the server.
			while(nread = Read(sockfd,buf,MAXLINE) > 0){
				printf("[Client][Time]: %s",buf);
				bzero(buf,sizeof(buf));

				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Client][Time][Feedback] Data received from server\n");
				//Write the feedback back to the parent.
				Write(cli_parent_fd,feedback,strlen(feedback));

			}
			if(nread == 0){
				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Client][Time] Server: Server Terminated Prematurely\n");
				Write(cli_parent_fd,feedback,strlen(feedback));
				return;
			}
			if(nread < 0){
				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Client][Time] Server: RST Packet Received\n");
				Write(cli_parent_fd,feedback,strlen(feedback));
				return;
			}
		}
	}

}

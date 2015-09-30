#include "unp.h"

//The function that takes input from user , sends it to server and waits for the response.
void echo_server(int,int,int);
int main(int argc,char *argv[]){
	int cli_parent_fd = strtol(argv[2],NULL,10);
	int sockfd,confd;
	int port_num = 5589;
	char feedback[MAXLINE];

	struct sockaddr_in servaddr;

	//TCP socket on port number for ECHO.
	sockfd = Socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);

	Inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

	//Connect using the socket.
	confd = connect(sockfd,(SA*)&servaddr,sizeof(servaddr));
	if(confd < 0){
		//If connection was not possible with the server.
		bzero(feedback,sizeof(feedback));
		strcpy(feedback,"Server could not be reached\n");
		Write(cli_parent_fd,feedback,strlen(feedback));
		close(cli_parent_fd);
		exit(0);
	}

	echo_server(cli_parent_fd,sockfd,port_num);
	//Return from echo server. Time to exit.
	close(sockfd);
	close(cli_parent_fd);
	close(confd);
	exit(0);

}

//This is the main function that does all the job.
void echo_server(int cli_parent_fd,int sockfd,int port_num){

	fd_set rset;
	int maxfd1;
	char feedback[MAXLINE];
	char buf[MAXLINE];
	int nread;
	FD_ZERO(&rset);


	while(1){

		FD_SET(0,&rset);
		FD_SET(sockfd,&rset);
		maxfd1 = max(0,sockfd)+1;

		Select(maxfd1,&rset,NULL,NULL,NULL);

		//Data received from the server.
		if(FD_ISSET(sockfd,&rset)){
			bzero(buf,sizeof(buf));

			if((nread = Readline(sockfd,buf,MAXLINE)) == 0){
				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Feedback][Echo][Client]: Server Terminated Prematurely\n");
				write(cli_parent_fd,feedback,strlen(feedback));
				return;
				//err_quit("Echo Server: Server Terminated Prematurely\n");
			}
			if(nread < 0){
				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Feedback][Echo][Client] Server: Reset Copy Received\n");
				write(cli_parent_fd,feedback,strlen(feedback));
				return;
			}

			bzero(feedback,sizeof(feedback));
			sprintf(feedback,"[Feedback][Echo][Client]: Data received from server of size %d bytes!\n",nread);
			write(cli_parent_fd,feedback,strlen(feedback));

			printf("[Echo][Client] Data received from server : %s",buf);


		}

		//User typed something. Interrupt received on stdin.
		if(FD_ISSET(0,&rset)){
			bzero(buf,sizeof(buf));
			if((nread = Readline(0,buf,MAXLINE)) == 0){
				bzero(feedback,sizeof(feedback));
				strcpy(feedback,"[Feedback][Echo][Client]: ^D found. Client is done executing\n");
				write(cli_parent_fd,feedback,strlen(feedback));
				return;
			}

			bzero(feedback,sizeof(feedback));
			strcpy(feedback,"[Feedback][Echo][Client]: Data sent to server!\n");
			write(cli_parent_fd,feedback,strlen(feedback));

			//Write the data the user typed onto the socket.
			write(sockfd,buf,nread);
		}

	}

}

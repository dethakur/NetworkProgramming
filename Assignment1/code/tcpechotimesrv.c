#include "unpthread.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>

int echo_listenfd,time_listenfd;

//Below are the port numbers used.
int echo_port_num = 5589; //Port Number for echo server
int time_port_num = 5590; //Port Number for time server

//Thread that handles the request for time.
void* time_server_thread(void*);

//Thread that handles the request for echo.
void* echo_server_thread(void*);

//Handles SIGPIPE interrupt.
void sig_pipe(int);

int main(int argc,char* argv[]){

	fd_set rset;
	int enable = 1;
	int fileflags;
	struct sockaddr_in echo_servaddr,time_serv_addr;

	//Below code registers echo socket on the server.
	echo_listenfd = Socket(AF_INET,SOCK_STREAM,0);
	//We make the RESUE ADDRESS for the port number.
	setsockopt(echo_listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	//Below code makes the echo listener non blocking.
	if (fileflags = fcntl(echo_listenfd, F_GETFL, 0) == -1)  {
		printf("Error in fileflags \n");
	   exit(1);
	}
	if (fcntl(echo_listenfd, F_SETFL, fileflags | FNDELAY) == -1)  {
	   perror("fcntl F_SETFL, FNDELAY");
	   exit(1);
	}


	bzero(&echo_servaddr,sizeof(echo_servaddr));
	echo_servaddr.sin_family = AF_INET;
	echo_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echo_servaddr.sin_port = htons(echo_port_num);

	//The server now listens to the socket created for echo server.
	Bind(echo_listenfd,(SA*)&echo_servaddr,sizeof(echo_servaddr));
	Listen(echo_listenfd,LISTENQ);

	//Below code registers time socket on the server.
	time_listenfd = Socket(AF_INET,SOCK_STREAM,0);
	//We make the PORT ADDRESS reusable.
	setsockopt(time_listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	//Below code makes the time listener non blocking.
	if (fileflags = fcntl(time_listenfd, F_GETFL, 0) == -1)  {
		printf("Error in fileflags \n");
	   exit(1);
	}
	if (fcntl(time_listenfd, F_SETFL, fileflags | FNDELAY) == -1)  {
	   perror("fcntl F_SETFL, FNDELAY");
	   exit(1);
	}


	bzero(&time_serv_addr,sizeof(time_serv_addr));
	time_serv_addr.sin_family = AF_INET;
	time_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	time_serv_addr.sin_port = htons(time_port_num);

	//The server now listens to the socket created for Time Server.
	Bind(time_listenfd,(SA*)&time_serv_addr,sizeof(time_serv_addr));
	Listen(time_listenfd,LISTENQ);

	//Register the SIGPIPE handling function.
	Signal(SIGPIPE,sig_pipe);

	while(1){
		FD_SET(time_listenfd,&rset);
		FD_SET(echo_listenfd,&rset);
		int maxfd1 = max(time_listenfd,echo_listenfd)+1;

		//Use Select to handle interrupts on echo and time listener.
		Select(maxfd1,&rset,NULL,NULL,NULL);
		printf("[Server] Select Interrupted\n");
		if(FD_ISSET(time_listenfd,&rset)){
			printf("[DayTime] Connection Request Received \n");
			pthread_t tid;
			struct sockaddr_in cliaddr;
			socklen_t clilen = sizeof(cliaddr);
			int connfd;

			connfd = accept(time_listenfd,(SA*)&cliaddr,&clilen);
			//Make Accept call blocking.
			if(connfd == -1){
				printf("[Server]Connection could not be established\n");
				continue;
			}

			if (fileflags = fcntl(connfd, F_GETFL, 0) == -1)  {
				printf("Error in fileflags \n");
			   exit(1);
			}
			if (fcntl(connfd, F_SETFL, fileflags & ~FNDELAY) == -1)  {
			   perror("fcntl F_SETFL, FNDELAY");
			   exit(1);
			}

			printf("[DayTime] Connection Request Successful \n");
			//Create the thread to handle the request on time port.
			Pthread_create(&tid,NULL,&time_server_thread,&connfd);

		}

		if(FD_ISSET(echo_listenfd,&rset)){
			printf("[Echo] Connection Request Received \n");
			int connfd;
			pthread_t tid;
			struct sockaddr_in cliaddr;

			socklen_t clilen = sizeof(cliaddr);
			connfd = accept(echo_listenfd,(SA*)&cliaddr,&clilen);
			//Make accept call blocking.
			if(connfd == -1){
				printf("[Server]Connection could not be established\n");
				continue;
			}

			if (fileflags = fcntl(connfd, F_GETFL, 0) == -1)  {
				printf("Error in fileflags \n");
			   exit(1);
			}
			if (fcntl(connfd, F_SETFL, fileflags & ~FNDELAY) == -1)  {
			   perror("fcntl F_SETFL, FNDELAY");
			   exit(1);
			}

			printf("[Echo] Connection Request Successful \n");
			//Create a thread to handle request on echo port.
			Pthread_create(&tid,NULL,&echo_server_thread,&connfd);
		}
	}

	return 0;
}

//This function contains details for thread to handle requests on port number for echo port.
void* echo_server_thread(void* arg){
	char buf[MAXLINE];

	int status,connfd,nread;

	connfd = *((int*)(arg));

	Pthread_detach(pthread_self());

	while((nread = Readline(connfd,buf,MAXLINE)) > 0){
		printf("[Echo][Server] Input received from Client = %s\n",buf);
		write(connfd,buf,nread);
		bzero(&buf,MAXLINE);
	}
	if(nread == 0){
		printf("[Echo][Server] Client closed the connection \n");
		close(connfd);
		pthread_exit(&status);

	}
	if(nread < 0){
		printf("[Echo][Server] Client terminated incorrectly! \n");
		close(connfd);
		pthread_exit(&status);

	}
}

//This function contains details for thread to handle requests on port number for time port.
void* time_server_thread(void* arg){
	int nwrite,connfd,status;
	time_t ticks;
	char buf[MAXLINE];

	connfd = *((int*)(arg));
	Pthread_detach(pthread_self());

	bzero(buf,sizeof(buf));

	while(1){
		ticks = time(NULL);
		snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
		if((nwrite = write(connfd,buf,MAXLINE)) == -1){
			break;
		}
		if(nwrite == 0){
			printf("[DayTime] Write Failed. Return");
			break;
		}
		sleep(5);
	}
	bzero(&buf,MAXLINE);
	close(connfd);
	pthread_exit(&status);
}

//The code to handle sigpipe.
void sig_pipe(int signo){
	int status;
	printf("[Server] SIGPIPE found. Terminating the thread \n");
	pthread_exit(&status);
}

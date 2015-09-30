#include "unp.h"
#include <netdb.h>
void sig_child(int);

int main(int argc,char *argv[]){
	int fd[2];
	char buf[MAXLINE];
	int nread;
	char *input = malloc(sizeof(char)*MAXLINE);
	//Register SIGCHLD
	Signal(SIGCHLD,sig_child);
	fd_set rset;
	struct hostent* host_val;

	//This variable contains the IP address in presentation format. (127.0.0.1)
	char str [INET_ADDRSTRLEN];


	struct in_addr in_addr_val;
	if(argc <2){
		//If invalid request received to execute the program.
		printf("Please enter the IP address or hostname as second argument!\n");
		exit(0);
	}

	if (inet_pton(AF_INET,argv[1],&in_addr_val.s_addr) > 0){
		//If IP address found , print the host name.
		host_val = gethostbyaddr(&in_addr_val.s_addr,4,AF_INET);
		if(host_val != NULL){
			printf("Hostname for the IP address entered is = %s\n",host_val->h_name);
			strcpy(str,argv[1]);
		}else{
			printf("IP address is not found\n");
			exit(0);
		}
	}else{
		//If hostname found , get the IP address and save it in str.
		char *ptr;
		ptr = *++argv;
		host_val = gethostbyname(ptr);
		if(host_val == NULL){
			printf("Host name is not found \n");
			exit(0);
		}else{
			char **pptr = host_val->h_addr_list;
			for ( ; *pptr != NULL; pptr++){
				inet_ntop (host_val->h_addrtype, *pptr, str, sizeof (str));
				printf("IP address for the host name = %s\n",str);
			}
		}

	}

	while(1){
		FD_SET(0,&rset);
		int maxfd1 = 1;
		printf("\nType \"echo\" to start a echo client. \n");
		printf("Type \"time\" to start a daytime client. \n");
		printf("Type \"quit\" to terminate. \n");
		Select(maxfd1,&rset,NULL,NULL,NULL);
		//If input from the user received on stdin.
		if(FD_ISSET(0,&rset)){
			Readline(0,input,MAXLINE);

			if(strncmp(input,"quit",strlen(input)-1) == 0){
				printf("Terminating!!!!\n");
				exit(0);
			}

			pipe(fd);
			pid_t pid;
			pid = fork();
			if(pid == 0){
				//Close the read end from the child.
				close(fd[0]);
				char s[10];
				sprintf(s,"%d",fd[1]);
				if(strncmp(input,"echo",strlen(input)-1) == 0){
					execlp("xterm","xterm","-e","./echo_cli",&str,s,NULL);
				}else if(strncmp(input,"time",strlen(input)-1) == 0){
					execlp("xterm","xterm","-e","./time_cli",&str,s,NULL);
				}else{
					exit(0);
				}

			}else{
				//Close the write end of the pipe.
				close(fd[1]);
				int nread;
				int status;
				bzero(buf,sizeof(buf));

				//Read the feedback from the child.
				while((nread = read(fd[0],&buf,MAXLINE)) > 0){
					Write(1,buf,MAXLINE);
					bzero(buf,sizeof(buf));
				}

				//If client has terminated.
				if(nread == 0){
					printf("[Echo][Client] Client Terminates\n");
				}
				if(nread < 0){

				}
				close(fd[0]);

			}
		}
	}
	return 0;
}
//If SIGCHLD received , handle it.
void sig_child(int signo){
	int status;
	pid_t pid;
	//Wait for all the terminated childs.
	while((pid = waitpid(-1,&status,WNOHANG)) > 0){
		printf("[Echo][Client]Child terminated %d\n",pid);
	}
	return;
}



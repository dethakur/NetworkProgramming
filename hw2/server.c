#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <setjmp.h>
#include "./unprtt.h"
#include "./rtt.c"
#include  <signal.h>

void udp_reliable_transfer(int, struct sockaddr_in);
void* send_file_data(void*);
void* receive_ack(void*);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//void find_addresses(iAddr*);

struct thread_config {
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int sockfd;
	uint32_t rwnd;
	int last_unacked_seq;
	struct rtt_info* rttinfo_ptr;
	int *rttinit_ptr;
};

typedef struct thread_config th_config;

static sigjmp_buf jmpbuf;
static void sig_alrm(int signo);

static void sig_alrm(int signo) {
	siglongjmp(jmpbuf, 1);
}

int main(int argc, char* argv[]) {

	int count = get_addr_count();
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(5589);

	Bind(sockfd, (SA*) &servaddr, sizeof(servaddr));

	fd_set rset;
	char buf[MAXLINE];
	query_obj q_obj;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		int maxfd1 = sockfd + 1;
		printf("[Parent][Server]Waiting for select! \n");
		Select(maxfd1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) {
			printf(
					"[Parent][Server]Select got broken! which means socket got interrupted \n");
			struct sockaddr_in cliaddr;
			bzero(&q_obj, sizeof(q_obj));
			recv_udp_data(sockfd, NULL, 0, &q_obj);
			cliaddr = q_obj.sock_addr;

			bzero(&q_obj, sizeof(q_obj));
			q_obj.config.type = ack;
			send_udp_data(sockfd, (SA*) &cliaddr, sizeof(cliaddr), &q_obj);

			pid_t pid = fork();
			if (pid == 0) {
				udp_reliable_transfer(sockfd, cliaddr);
				exit(0);
			}
			//			else{
			//				continue;
			//			}
		}
	}
	return 0;

}
void udp_reliable_transfer(int sockfd, struct sockaddr_in cliaddr) {
	pthread_t tid1;
	pthread_t tid2;

	int new_sockfd;
	query_obj q_obj;
	struct sockaddr_in servaddr, sock_addr;
	socklen_t len2 = sizeof(sock_addr);

	bzero(&sock_addr, sizeof(sock_addr));
	bzero(&servaddr, sizeof(servaddr));
	new_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(0);

	Bind(new_sockfd, (SA*) &servaddr, sizeof(servaddr));

	getsockname(new_sockfd, (SA*) &sock_addr, &len2);

	int port_number = ntohs(sock_addr.sin_port);
	printf("Connected Port Number = %d\n", port_number);

	bzero(&q_obj, sizeof(q_obj));
	sprintf(q_obj.buf, "%d", port_number);
	printf("Data send = %s\n", q_obj.buf);

	//Send port to the client
	q_obj.config.type = data;
	send_udp_data(sockfd, (SA*) &cliaddr, sizeof(cliaddr), &q_obj);

	//Wait for acknowledgement. Use select.

	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(new_sockfd, &rset);
	int maxfd1 = new_sockfd + 1;
	printf("[Child][Server]Waiting for select! \n");
	Select(maxfd1, &rset, NULL, NULL, NULL);
	if (FD_ISSET(new_sockfd, &rset)) {
		bzero(&q_obj, sizeof(q_obj));
		recv_udp_data(new_sockfd, (SA*) &cliaddr, sizeof(cliaddr), &q_obj);
		//Ack Received!
		printf("[Child][Server] Interrupted!!! Ack Received\n");
	}
	close(sockfd);

	struct rtt_info rttinfo_data;
	int rttinit = 0;
	th_config config;
	config.sockfd = new_sockfd;
	config.cliaddr = cliaddr;
	config.clilen = sizeof(cliaddr);
	config.rwnd = -1;
	config.rttinfo_ptr = &rttinfo_data;
	config.rttinit_ptr = &rttinit;

	Pthread_create(&tid2, NULL, &receive_ack, &config);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	Pthread_create(&tid1, NULL, &send_file_data, &config);

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	printf("UDP connection done\n");

}
void* send_file_data(void* arg) {

	th_config *config;
	int seq_num = -1;
	config = (th_config*) (arg);
	query_obj q_obj;
	int sleep_time = 1;

	Signal(SIGALRM, SIG_IGN);
	// just starting to send, so there is no un-acked seq num
	config->last_unacked_seq = -1;

	while (1) {
		pthread_mutex_lock(&mutex);

		int rwnd = config->rwnd;
		bzero(&q_obj, sizeof(q_obj));
		if (rwnd != 0) {

			if (*(config->rttinit_ptr) == 0) {
				rtt_init(config->rttinfo_ptr);
				*(config->rttinit_ptr) = 1;
			}

			if (config->last_unacked_seq != -1) {
				// seq num config->last_unacked_seq has not been ack'ed, so re-transmit.
				printf("Sender retransmitting packet with seq_num %d\n",
						config->last_unacked_seq);
				q_obj.config.seq = config->last_unacked_seq;
			} else {
				q_obj.config.ts = rtt_ts(config->rttinfo_ptr);
				seq_num += 1;
				q_obj.config.seq = seq_num;
			}

			q_obj.config.type = data;
			strcpy(q_obj.buf, "success");
			sleep_time = 1;
			printf(
					"[Data] Data sent to server with rwnd size = %d and Seq num %d\n",
					config->rwnd, q_obj.config.seq);
		} else {
			q_obj.config.type = ping;
			sleep_time = 1;
			printf("[Ping] Data sent to server with rwnd size = %d\n",
					config->rwnd);
			q_obj.config.seq = seq_num;
		}

		rtt_newpack(config->rttinfo_ptr);

		pthread_mutex_unlock(&mutex);
		send_udp_data(config->sockfd, (SA*) &config->cliaddr, config->clilen,
				&q_obj);

		sleep(sleep_time);
	}

}
//void* receive_ack(void* arg){
//	th_config *config;
//	config = (th_config*)(arg);
//	query_obj q_obj;
//	while(1){
//		bzero(&q_obj,sizeof(q_obj));
//		strcpy(q_obj.buf,"success");
//		q_obj.config.type = data;
//
//		recv_udp_data(config->sockfd,(SA*)&config->cliaddr,config->clilen,&q_obj);
//		printf("[ACK]Window Size = %d and Seq Num = %d\n",q_obj.config.rwnd,q_obj.config.seq);
//		config->rwnd = q_obj.config.rwnd;
//
//	}
//}

void* receive_ack(void *conf) {
	th_config *config = (th_config*) (conf);
	struct rtt_info *rttinfo_ptr = config->rttinfo_ptr;
	query_obj q_obj;
	int prev_seq_num = 0;
	Signal(SIGALRM, sig_alrm);
	while (1) {
		pthread_mutex_lock(&mutex);

		int alarm_secs = rtt_start(rttinfo_ptr);
		//		printf("Setting alarm value to: %d\n", alarm_secs);
		alarm(1); /* calc timeout value & start timer */
		//		alarm(10);
		if (sigsetjmp(jmpbuf, 1) != 0) {
			printf("Timeout happened\n");
			if (rtt_timeout(rttinfo_ptr) < 0) {
				printf("no response from peer, giving up");
				*(config->rttinit_ptr) = 0;
				errno = ETIMEDOUT;
				return;
			}
			// timeout has happened, resending of previous packet has to be done
			config->last_unacked_seq = prev_seq_num + 1;
			*(config->rttinit_ptr) = 0;
		}

		pthread_mutex_unlock(&mutex);

		ssize_t n;
		do {
			bzero(&q_obj, sizeof(q_obj));
			strcpy(q_obj.buf, "success");
			q_obj.config.type = data;

			printf(
					"Last acknowledged was %d. Trying to receive ack in process %d\n",
					prev_seq_num, getpid());
			n = recv_udp_data(config->sockfd, (SA*) &config->cliaddr,
					config->clilen, &q_obj);
			printf("[ACK]Window Size = %d and Seq Num = %d\n",
					q_obj.config.rwnd, q_obj.config.seq);

			pthread_mutex_lock(&mutex);
			config->rwnd = q_obj.config.rwnd;
			pthread_mutex_unlock(&mutex);

		} while (n < sizeof(struct udp_data) || q_obj.config.seq
				<= prev_seq_num);

		alarm(0); /* stop SIGALRM timer *//* calculate & store new RTT estimator values */

		pthread_mutex_lock(&mutex);
		//		printf("rtt items are %d %d\n", rtt_ts(rttinfo_ptr), q_obj.config.ts);
		rtt_stop(rttinfo_ptr, rtt_ts(rttinfo_ptr) - q_obj.config.ts);
		config->last_unacked_seq = -1;
		pthread_mutex_unlock(&mutex);

		prev_seq_num = q_obj.config.seq;
	}
}


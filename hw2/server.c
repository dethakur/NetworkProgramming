#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <setjmp.h>
#include "./unprtt.h"
#include "./rtt.c"
#include  <signal.h>

struct thread_config {
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int sockfd;
	uint32_t rwnd;
	int last_unacked_seq;
	struct rtt_info rttinfo_ptr;
	int rttinit_ptr;
};

typedef struct thread_config th_config;

void udp_reliable_transfer(int, struct sockaddr_in);
void send_file_data(th_config*);
void receive_ack(th_config*, int);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//void find_addresses(iAddr*);

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
//		printf("[Parent][Server]Waiting for Connection! \n");
		Select(maxfd1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) {
//			printf("[Parent][Server]Select got broken! which means socket got interrupted \n");
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
	printf("Server new Port Number = %d\n", port_number);

	bzero(&q_obj, sizeof(q_obj));
	sprintf(q_obj.buf, "%d", port_number);
	printf("Data sent to the client by the server = %s\n", q_obj.buf);

	//Send port to the client
	q_obj.config.type = data;
	send_udp_data(sockfd, (SA*) &cliaddr, sizeof(cliaddr), &q_obj);

	//Wait for acknowledgement. Use select.

	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(new_sockfd, &rset);
	int maxfd1 = new_sockfd + 1;
//	printf("[Child][Server]Waiting for select! \n");
	Select(maxfd1, &rset, NULL, NULL, NULL);
	if (FD_ISSET(new_sockfd, &rset)) {
		bzero(&q_obj, sizeof(q_obj));
		recv_udp_data(new_sockfd, (SA*) &cliaddr, sizeof(cliaddr), &q_obj);
	}
	close(sockfd);
	struct rtt_info rttinfo_data;
	int rttinit = 0;
	th_config config;
	config.sockfd = new_sockfd;
	config.cliaddr = cliaddr;
	config.clilen = sizeof(cliaddr);
	config.rwnd = -1;
	config.rttinfo_ptr = rttinfo_data;
	config.rttinit_ptr = rttinit;
	send_file_data(&config);

}
void send_file_data(th_config* config) {
	printf("Inside send file data\n");
	int seq_num = -1;
	query_obj q_obj;
	int sleep_time = 1;

	config->last_unacked_seq = -1;
	config->rwnd = 1; //Start by sending one packet.
	printf("Time to go in a while loop \n");

	while (1) {
		int rwnd = config->rwnd;
		int packet_count = 0;
		if ((config->rttinit_ptr) == 0) {
			rtt_init(&config->rttinfo_ptr);
			(config->rttinit_ptr) = 3;
		}
		//This loop is basically to send packets together.
		for (packet_count = 0; packet_count < rwnd; packet_count++) {
			bzero(&q_obj, sizeof(q_obj));

			if (config->last_unacked_seq != -1) {
				// seq num config->last_unacked_seq has not been ack'ed, so re-transmit.
				printf("[Send]Sender retransmitting packet with seq_num %d\n",
						config->last_unacked_seq);
				q_obj.config.seq = config->last_unacked_seq;
				seq_num = config->last_unacked_seq;
				config->last_unacked_seq = -1;
			} else {
				q_obj.config.ts = rtt_ts(&config->rttinfo_ptr);
				seq_num += 1;
				q_obj.config.seq = seq_num;
			}

			q_obj.config.type = data;
			strcpy(q_obj.buf, "success");
			sleep_time = 1;
			printf(
					"[Send][Data] Data sent to server with rwnd size = %d and Seq num %d\n",
					config->rwnd, q_obj.config.seq);

			rtt_newpack(&config->rttinfo_ptr);

			send_udp_data(config->sockfd, (SA*) &config->cliaddr,
					config->clilen, &q_obj);

		}
		if (config->rwnd == 0) {
			q_obj.config.type = ping;
			sleep_time = 1;
			printf("[Send][Ping] Data sent to server with rwnd size = %d\n",
					config->rwnd);
			q_obj.config.seq = seq_num;

			send_udp_data(config->sockfd, (SA*) &config->cliaddr,
					config->clilen, &q_obj);
		}
		receive_ack(config, seq_num);

		sleep(sleep_time);
	}

}

void receive_ack(th_config *config, int seq_no) {

	struct rtt_info *rttinfo_ptr = &config->rttinfo_ptr;
	query_obj q_obj;

	/*Now since we are doing in one process. This variable will save
	 the sequence number server expects from the client in ACK.I will wait in recvmsg to
	 get ACK , if a timeout happens , I will return asking the server to resend packet in variable prev_seq_num.
	 */
	int expected_seq_num = seq_no + 1;

	//This variable will save the seq number client wants.
	int prev_seq_num = 0;

	int fast_retransmit = 0;

	Signal(SIGALRM, sig_alrm);


	ssize_t n;
	//Kaushik : Check this variable. This seems to be too small. If i set timeout to this value , it keeps throwing timeouts.
	int alarm_secs = rtt_start(rttinfo_ptr);
	struct itimerval timer;
	timer.it_value.tv_sec = config->rttinit_ptr;
	timer.it_value.tv_usec = 0;
	setitimer (ITIMER_REAL, &timer, NULL);
	do {
		if (sigsetjmp(jmpbuf, 1) != 0 || fast_retransmit == 2) {
			printf("[Fast][Retransmit]Fast retransmit == %d\n",fast_retransmit);
			printf("Timeout happened\n");
			if (rtt_timeout(rttinfo_ptr) < 0) {
				printf("no response from peer, giving up\n");
				(config->rttinit_ptr) = 0;
				errno = ETIMEDOUT;
				return;
			}
			config->last_unacked_seq = prev_seq_num;
			(config->rttinit_ptr) = 0;
			return;
		}

		bzero(&q_obj, sizeof(q_obj));

		n = recv_udp_data(config->sockfd, (SA*) &config->cliaddr,
				config->clilen, &q_obj);
		printf("[ACK]Window Size = %d and Client wants seq no = %d , Expected Seq No = %d\n", q_obj.config.rwnd,
				q_obj.config.seq,expected_seq_num);

		config->rwnd = q_obj.config.rwnd;
		//Saving the last seq number received
		if(prev_seq_num == q_obj.config.seq){
			fast_retransmit++;
		}else{
			prev_seq_num = q_obj.config.seq;
			fast_retransmit = 0;
		}


	} while (n < sizeof(struct udp_data) || q_obj.config.seq != expected_seq_num);

	rtt_stop(rttinfo_ptr, rtt_ts(rttinfo_ptr) - q_obj.config.ts);
	config->last_unacked_seq = -1;
}


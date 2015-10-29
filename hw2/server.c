#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <setjmp.h>
#include "./unprtt.h"
#include "./rtt.c"
#include  <signal.h>

struct server_config {
	int port_num;
	int rwnd;
	char file_name[MAXLINE];
};

struct thread_config {
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int sockfd;
	uint32_t rwnd;
	int last_unacked_seq;
	struct rtt_info rttinfo_ptr;
	int rttinit_ptr;
	int cwnd;
	int isEOF;
	int last_pack_seq_no;
	int ssthresh;
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

struct server_config serv_config;

int main(int argc, char* argv[]) {

	//Reading config from server.in
	FILE* fp = fopen("server.in", "r");
	if (fp == NULL) {
		printf("Cannot open server.in file in READ mode! \n");
		exit(0);
	}
	fscanf(fp, "%d", &serv_config.port_num);
	fscanf(fp, "%d", &serv_config.rwnd);

	log_line_seperator();
	printf("Configuration read from Server.in file \n");
	printf("Port Number read from Server.in = %d\n", serv_config.port_num);
	printf("Sliding Window read from Server.in = %d\n", serv_config.rwnd);
	log_line_seperator();
	fclose(fp);

	//End of code to read configuration.

	int ip_addr_count = get_addr_count();
	iAddr addr[ip_addr_count];
	fill_addr_contents((iAddr*) &addr);
	disp_addr_contents((iAddr*) &addr, ip_addr_count);

	int i = 0;

	fd_set rset_arr;
	query_obj q_obj;
	int sockfd_arr[ip_addr_count];

	struct sockaddr_in servaddr[ip_addr_count], cliaddr[ip_addr_count];

	for (i = 0; i < ip_addr_count; i++) {
		bzero(&servaddr[i], sizeof(struct sockaddr_in));
		sockfd_arr[i] = Socket(AF_INET, SOCK_DGRAM, 0);
		servaddr[i].sin_family = AF_INET;
		servaddr[i].sin_port = htons(serv_config.port_num);
		Inet_pton(AF_INET, addr[i].ip_addr, &servaddr[i].sin_addr);
		Bind(sockfd_arr[i], (SA*) &servaddr[i], sizeof(struct sockaddr_in));
	}

	while (1) {
		int i = 0;
		int maxVal = -1;
		FD_ZERO(&rset_arr);
		for (i = 0; i < ip_addr_count; i++) {
			FD_SET(sockfd_arr[i], &rset_arr);
			maxVal = max(maxVal, sockfd_arr[i]);
		}
		maxVal = maxVal + 1;
		Select(maxVal, &rset_arr, NULL, NULL, NULL);
		for (i = 0; i < ip_addr_count; i++) {
			if (FD_ISSET(sockfd_arr[i], &rset_arr)) {
				struct sockaddr_in cliaddr;

				char clientIP[MAXLINE];

				bzero(clientIP, sizeof(clientIP));
				bzero(&q_obj, sizeof(q_obj));
				recv_udp_data(sockfd_arr[i], NULL, 0, &q_obj);
				cliaddr = q_obj.sock_addr;

				Inet_ntop(AF_INET, &cliaddr.sin_addr, clientIP,
						sizeof(clientIP));
				printf(
						"[Server] Request received from client IP = %s\n for file_name = %s\n",
						clientIP, q_obj.buf);
				strcpy(serv_config.file_name, q_obj.buf);
				int index = check_addr_local(clientIP, addr, ip_addr_count);
				if (index != 0) {
					printf("[Server] Client IP = %s is LOCAL to "
							"Server IP = %s with mask = %s\n", clientIP,
							addr[index].ip_addr, addr[index].mask);
				}

				bzero(&q_obj, sizeof(q_obj));
				q_obj.config.type = ack;
				send_udp_data(sockfd_arr[i], (SA*) &cliaddr, sizeof(cliaddr),
						&q_obj);

				pid_t pid = fork();
				if (pid == 0) {
					udp_reliable_transfer(sockfd_arr[i], cliaddr);
					printf(
							"[Server] File transfer successful. Now child ends!\n");
					exit(0);
				}
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
//	printf("Inside send file data\n");
	int seq_num = -1;
	query_obj q_obj;
	int sleep_time = 1;

	config->last_unacked_seq = -1;
	config->last_pack_seq_no = -1;
	config->ssthresh = -1;
	config->rwnd = serv_config.rwnd; //Start by sending one packet.
	config->cwnd = 1; //Slow Start
//	printf("Time to go in a while loop \n");
	printf("File data to be transferred = %s\n", serv_config.file_name);
	FILE* fp = fopen(serv_config.file_name, "r");
//	int is_eof = 0;
	while (1) {
		int rwnd = config->rwnd;
		int cwnd = config->cwnd;
		int packet_count = 0;

		//This loop is basically to send packets together.
		for (packet_count = 0; packet_count < min(cwnd, rwnd); packet_count++) {
			bzero(&q_obj, sizeof(q_obj));
//			char data[MAXLINE];

//			if ((seq_num > config->last_pack_seq_no || config->isEOF == 1)
//					&& config->last_pack_seq_no != -1) {
//				break;
//			}

			if (config->isEOF == 1) {
				break;
			}
			if (config->last_unacked_seq != -1) {
				// seq num config->last_unacked_seq has not been ack'ed, so re-transmit.
				q_obj.config.seq = config->last_unacked_seq;
				seq_num = config->last_unacked_seq;
				//reset the file pointer
				fclose(fp);
				fp = fopen(serv_config.file_name, "r");
				int count = 0;
				while (count != q_obj.config.seq) {
					fscanf(fp, "%s", &q_obj.buf);
					count++;
				}
				printf("[Send]Sender retransmitting data = %s with"
						" Seq No =  %d\n", q_obj.buf, config->last_unacked_seq);
				config->last_unacked_seq = -1;
			} else {
				q_obj.config.ts = rtt_ts(&config->rttinfo_ptr);
				seq_num += 1;
				q_obj.config.seq = seq_num;
				if (fscanf(fp, "%s", &q_obj.buf) == EOF && config->last_pack_seq_no == -1) {
					config->last_pack_seq_no = seq_num - 1;
					printf("LAST  Seq No = %d\n", config->last_pack_seq_no);
				}
			}

			if (seq_num > config->last_pack_seq_no
					&& config->last_pack_seq_no != -1) {
				printf("[EOF] Setting data type to EOF\n");
				q_obj.config.type = eof;
			} else {
				q_obj.config.type = data;
			}

			if ((config->rttinit_ptr) == 0) {
				rtt_init(&config->rttinfo_ptr);
				(config->rttinit_ptr) = 3;
			}

//			strcpy(q_obj.buf, data);
			sleep_time = 1;
			printf(
					"[Send][Data] Data = %s sent to server with rwnd size = %d and Seq num %d\n",
					q_obj.buf, config->rwnd, q_obj.config.seq);

			rtt_newpack(&config->rttinfo_ptr);

			send_udp_data(config->sockfd, (SA*) &config->cliaddr,
					config->clilen, &q_obj);

		}
		if (config->rwnd == 0 && config->isEOF != 1) {
			q_obj.config.type = ping;
			sleep_time = 1;
			printf("[Send][Ping] Sending Ping to the Client = %d\n",
					config->rwnd);
			q_obj.config.seq = seq_num;

			send_udp_data(config->sockfd, (SA*) &config->cliaddr,
					config->clilen, &q_obj);
		}
		if (config->isEOF == 1) {
			break;
		}
		receive_ack(config, seq_num);

//		sleep(sleep_time);
	}
	fclose(fp);

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
	//Kaushik : Check this variable. This seems to be too small.
	//If I set timeout to this value , it keeps throwing timeouts.
	int alarm_secs = rtt_start(rttinfo_ptr);

	struct itimerval timer;
	bzero(&timer,sizeof(timer));
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	do {
		if (sigsetjmp(jmpbuf, 1) != 0 || fast_retransmit == 3) {
			if (fast_retransmit == 3) {
				printf("[Fast][Retransmit]Fast retransmit == %d\n",
						fast_retransmit);
			} else {
				printf("Timeout happened. Expected Seq %d\n", expected_seq_num);
			}

			if(prev_seq_num > config->last_pack_seq_no && config->last_pack_seq_no != -1){
				config->isEOF = 1;
				return;
			}

			//Kaushik: Explain me what this do.
			//What does giving up mean. Is this a case of Timeout ot something else
			if (rtt_timeout(rttinfo_ptr) < 0) {
				printf("No response from peer, giving up.\n");
				(config->rttinit_ptr) = 0;
				errno = ETIMEDOUT;
				return;
			}

			config->ssthresh = (int) (config->cwnd / 2);
			config->cwnd = 1;
			printf("[Update] Cwnd set to %d and ssthresh set to %d \n",
					config->cwnd, config->ssthresh);
			config->last_unacked_seq = prev_seq_num;
			(config->rttinit_ptr) = 0;
			return;
		}

		bzero(&q_obj, sizeof(q_obj));

		n = recv_udp_data(config->sockfd, (SA*) &config->cliaddr,
				config->clilen, &q_obj);
//		printf("[ACK]Window Size = %d and Client wants seq no = %d , Expected Seq No = %d\n", q_obj.config.rwnd,
//				q_obj.config.seq,expected_seq_num);

		if (config->ssthresh != -1 && config->cwnd >= config->ssthresh) {
			config->cwnd += 1;
		} else {
			config->cwnd *= 2;
		}
		printf(
				"[Ack] Received from client with Seq No = %d. Cwnd updated to %d. Client Rwnd = %d\n",
				q_obj.config.seq, config->cwnd, q_obj.config.rwnd);

		if (q_obj.config.seq >= config->last_pack_seq_no && config->last_pack_seq_no != -1) {
			printf("[ACK] Last packet ACK received. Marking EOF\n");
			config->isEOF = 1;
			break;
		}

		config->rwnd = q_obj.config.rwnd;
		//Saving the last seq number received
		if (prev_seq_num == q_obj.config.seq) {
			fast_retransmit++;
		} else {
			prev_seq_num = q_obj.config.seq;
			fast_retransmit = 0;
		}

	} while (n < sizeof(struct udp_data) || q_obj.config.seq != expected_seq_num);

	rtt_stop(rttinfo_ptr, rtt_ts(rttinfo_ptr) - q_obj.config.ts);
	config->last_unacked_seq = -1;
}

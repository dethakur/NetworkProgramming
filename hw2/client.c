#include "unp.h"
#include <netdb.h>
#include "unpthread.h"
#include <unistd.h>
#include "unpthread.h"
#include "common.h"
#include <string.h>
#include <limits.h>

//#define cli_config.rwnd 100
//void dg_cli_new(int,SA*,socklen_t);
void* buffer_reader_thread(void*);
void push_data_to_buffer(char*, int, int);
int get_window_size();
void create_new_connection(int, char*);
int min_seq_num();
void send_ack_to_server(int, struct sockaddr_in, socklen_t, int, uint32_t,
		int expected_seq);
int same_host_name(char*, char*);

//Link List of Buffers
struct udp_data send_hdr, recv_hdr;
struct buffer {
	char data[MAXLINE];
	uint32_t is_filled;
	uint32_t seq;
	struct buffer *next;
};

struct client_config {
	char server_ip[MAXLINE];
	int server_port;
	char file_name[MAXLINE];
	int rwnd;
	int seed;
	float probability;
	int buffer_read_time;
};

struct buffer *text_buffer;
volatile int is_EOF = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_config cli_config;

int get_host(char *ip_addr, char *hostname) {
	char copy[100];
	strcpy(copy, ip_addr);
	struct in_addr myservaddr;
	inet_pton(AF_INET, copy, &myservaddr.s_addr);
	struct hostent * hostentry = gethostbyaddr(&myservaddr.s_addr, 4, AF_INET);
	strcpy(hostname, hostentry->h_name);
//	printf("Host name for %s\n", hostentry->h_name);
}
int main(int argc, char** argv) {

	FILE* fp = fopen("client.in", "r");
	if (fp == NULL) {
		printf("Cannot open client.in file in READ mode! \n");
		exit(0);
	}
	fscanf(fp, "%s", &cli_config.server_ip);
	fscanf(fp, "%d", &cli_config.server_port);
	fscanf(fp, "%s", &cli_config.file_name);
	fscanf(fp, "%d", &cli_config.rwnd);
	fscanf(fp, "%d", &cli_config.seed);
	fscanf(fp, "%f", &cli_config.probability);
	fscanf(fp, "%d", &cli_config.buffer_read_time);
	log_line_seperator();

	text_buffer = (struct buffer*) malloc(
			sizeof(struct buffer) * cli_config.rwnd);

	printf("Client Config Read from client.in file \n");
	printf("Server IP = %s\n", cli_config.server_ip);
	printf("Server PORT = %d\n", cli_config.server_port);
	printf("File Name = %s\n", cli_config.file_name);
	printf("Sliding Window Size = %d", cli_config.rwnd);
	printf("Random generator seed = %f\n", cli_config.seed);
	printf("Drop Probability = %f\n", cli_config.probability);
	printf("mean u in milliseconds = %d\n", cli_config.buffer_read_time);
	log_line_seperator();

	int sockfd;
	pthread_t tid;
	struct sockaddr_in servaddr, cliaddr, sock_addr;
	query_obj q_obj;

	char server_ip[MAXLINE];

	int ip_addr_count = get_addr_count();
	iAddr addr[ip_addr_count];
	fill_addr_contents((iAddr*) &addr);
	disp_addr_contents((iAddr*) &addr, ip_addr_count);

	//This is the Server IP that has to be connected.
	char clihost[40];
	char servhost[40];
	get_host(cli_config.server_ip, servhost);
	get_host(addr[1].ip_addr, clihost);
	if (strcmp(clihost, servhost) == 0) {
		printf("Both client and server are on the same host\n");
		strcpy(server_ip, "127.0.0.1");
	} else {
		strcpy(server_ip, cli_config.server_ip);
	}

	int index = check_addr_local(server_ip, addr, ip_addr_count);
	if (index != 0) {
		printf("[Client] Server IP = %s is local to "
				"CLient IP = %s with mask = %s\n", server_ip,
				addr[index].ip_addr, addr[index].mask);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(cli_config.server_port);

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(0);

	inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

	//	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	int i = 0;
	for (i = 0; i < cli_config.rwnd; i++) {
		text_buffer[i].is_filled = -1;
	}

	Pthread_create(&tid, NULL, &buffer_reader_thread, NULL);

	bind(sockfd, (SA*) &cliaddr, sizeof(cliaddr));

	bzero(&sock_addr, sizeof(sock_addr));
	socklen_t len = sizeof(struct sockaddr_in);
	getsockname(sockfd, (SA*) &sock_addr, &len);
	printf("Client port = %d\n", ntohs(sock_addr.sin_port));

	Connect(sockfd, (SA*) &servaddr, sizeof(servaddr));

	struct sockaddr_in peer_sock;
	bzero(&peer_sock, sizeof(peer_sock));
	socklen_t len2 = sizeof(peer_sock);

	getpeername(sockfd, (SA*) &peer_sock, &len2);
	printf("Server port = %d\n", ntohs(peer_sock.sin_port));

	char sendline[MAXLINE], recvline[MAXLINE];

	strcpy(sendline, cli_config.file_name);

	Write(sockfd, sendline, strlen(sendline));

	do {
		bzero(&q_obj, sizeof(query_obj));
		recv_udp_data(sockfd, NULL, 0, &q_obj);
		printf("Type of data = %d = ", q_obj.config.type);
	} while (q_obj.config.type != ack);

	//	printf("Ack was received!!\n");

	do {
		bzero(&q_obj, sizeof(query_obj));
		recv_udp_data(sockfd, NULL, 0, &q_obj);
		printf("Port Number = %s!!\n", q_obj.buf);
		//		printf("Data type = %d!!\n",q_obj.config.type);

	} while (q_obj.config.type != data);

	long *ptr;
	int new_port_number = strtol(q_obj.buf, ptr, 10);

	create_new_connection(new_port_number, server_ip);
	printf("[Client]Done creating connection with new server\n");
	is_EOF = 1;
//	usleep(cli_config.buffer_read_time*3);
//	Pthread_join(tid, NULL);

	i = 0;
//			pthread_mutex_lock(&mutex);
	for (i = min_seq_num(); i < cli_config.rwnd; i++) {
		if (text_buffer[i].is_filled == 1) {
			printf("[FileData] File Data = %s with seq = %d\n",
					text_buffer[i].data, text_buffer[i].seq);
			bzero(&text_buffer[i].data, sizeof(text_buffer[i].data));
			text_buffer[i].is_filled = -1;
		}
	}

	close(sockfd);
	fclose(fp);
	free(text_buffer);
	printf("Writing done!!\n");

	return 0;

}

int ispresent(int seq_num) {
	int i = 0;
	for (; i < cli_config.rwnd; i++) {
		if (text_buffer[i].is_filled != -1 && text_buffer[i].seq == seq_num) {
			return 1;
		}
	}
	return 0;
}
int min_seq_num() {
	int min_val = INT_MAX;
	int min_index = 0;
	int i = 0;
	for (; i < cli_config.rwnd; i++) {
		if (text_buffer[i].is_filled != -1) {
			int val = min(min_val, text_buffer[i].seq);
			if (min_val > val) {
				min_index = i;
				min_val = val;
			}
//			printf("Min seq _ num  = %d\n",min_val);
		}
	}
//	if(min_val != INT_MAX)
//		printf("Min seq _ num  = %d\n",min_val);

	return min_index;
}
void update_expected_seq_num(int * expected_seq_num_ptr) {
	int i = 0;
	while (ispresent(*expected_seq_num_ptr) == 1) {
		*expected_seq_num_ptr += 1;
	}
}

int drop_packet(double probability, int max_seed) {
	int randval = rand();
	double r = (double) (rand() % max_seed) / max_seed;
//	printf("probab = %f r = %f\n", probability, r);
	//This has to be opposite. A probability of 1 means all packets have to be dropped.
	return probability > r;
}

void create_new_connection(int port_num, char* server_ip) {
	printf("New port number = %d\n", port_num);
	log_line_seperator();
	int sockfd;
	struct sockaddr_in servaddr;
	int expected_seq_num = 0;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);
	Inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	query_obj q_obj;
	bzero(&q_obj, sizeof(q_obj));

	send_udp_data(sockfd, (SA*) &servaddr, sizeof(servaddr), &q_obj);

	fd_set rset;

	int max_seed = cli_config.seed;
	srand(max_seed);
//	int eof_seq_no = -1;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		int maxfd1 = sockfd + 1;
		//		printf("[Client] Waiting for Data from server\n");
		Select(maxfd1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) {
			bzero(&q_obj, sizeof(query_obj));
			recv_udp_data(sockfd, NULL, 0, &q_obj);

			//Kaushik: Confirm if this change is correct. Changing probability
			if (drop_packet(cli_config.probability, max_seed)) {
				printf("[DROP]Simulating loss of datagram with seq num %d\n",
						q_obj.config.seq);
				continue;
			}

			pthread_mutex_lock(&mutex);
//			printf("Lock1 \n");

			if (q_obj.config.type == data) {
				printf("[Data]Data = %s Received with seq no = %d\n", q_obj.buf,
						q_obj.config.seq);
				push_data_to_buffer((char*) &q_obj.buf, q_obj.config.seq,
						expected_seq_num);
			} else if (q_obj.config.type == ping) {
				printf("[Ping]Ping Query Received\n");
			} else if (q_obj.config.type == eof) {
//				eof_seq_no = q_obj.config.seq;
				char buf[MAXLINE];
				bzero(buf, sizeof(buf));
				strcpy(buf, "End of File!");
				push_data_to_buffer(&buf, q_obj.config.seq, expected_seq_num);
				printf("[EOF]EOFFFFF Received for seq no = %d\n",
						q_obj.config.seq);
			}
			//			printf("[Client]Select interrupted in client!!!. Data received = %s\n",q_obj.buf);
			update_expected_seq_num(&expected_seq_num);
			printf(
					"[Ack] Data sent to server with seq no = %d and ack no = %d\n",
					q_obj.config.seq, expected_seq_num);
			send_ack_to_server(sockfd, q_obj.sock_addr, sizeof(q_obj.sock_addr),
					q_obj.config.seq, q_obj.config.ts, expected_seq_num);

			if (q_obj.config.type == eof
					&& q_obj.config.seq + 1 == expected_seq_num) {
				printf("EOFFF actuall happened ! , exiting\n");
				break;
			} else {
//				printf("Expected = %d and got = %d\n", expected_seq_num,
//						q_obj.config.seq);
			}

			pthread_mutex_unlock(&mutex);

		}
	}
}

void push_data_to_buffer(char* send_buf, int seq_num, int expected_seq_num) {
	int i = seq_num % cli_config.rwnd;
	if (text_buffer[i].is_filled == -1) {
		if (seq_num != expected_seq_num) {
			printf(
					"[DROP][DATA]Cannot accept this datagram. Expecting one with seq number %d\n",
					expected_seq_num);
			return;
		}
		strcpy(text_buffer[i].data, send_buf);
		text_buffer[i].is_filled = 1;
		text_buffer[i].seq = seq_num;
		printf(
				"[Data] Pushed to buffer at index %d with data = %s and seq_num = %d\n",
				i, send_buf, seq_num);
	} else {
//		printf("Buffer is full, dropping datagram with seq num %d\n", seq_num);
	}
}

//This should read only continous packets
void* buffer_reader_thread(void* arg) {

	while (1) {
		int i = 0;
		pthread_mutex_lock(&mutex);
		for (i = min_seq_num(); i < cli_config.rwnd; i++) {
			if (text_buffer[i].is_filled == 1) {
				printf("[FileData] File Data = %s with seq = %d\n",
						text_buffer[i].data, text_buffer[i].seq);
				bzero(&text_buffer[i].data, sizeof(text_buffer[i].data));
				text_buffer[i].is_filled = -1;
			}
		}
		if (is_EOF == 1)
			break;
//		printf("Lock2 \n");
		pthread_mutex_unlock(&mutex);
		usleep(cli_config.buffer_read_time);

	}

}

void send_ack_to_server(int sockfd, struct sockaddr_in cliaddr,
		socklen_t clilen, int seq, uint32_t ts, int expected_seq_num) {
	query_obj q_obj;
	q_obj.config.type == ack;
	q_obj.config.seq = expected_seq_num;
	q_obj.config.rwnd = get_window_size();
	q_obj.config.ts = ts;
	printf("[ACK]Sending ack to server with new seq num %d\n",
			q_obj.config.seq);
	send_udp_data(sockfd, (SA*) &cliaddr, clilen, &q_obj);
}
int get_window_size() {
	int count = 0;
	int i = 0;
	for (i = 0; i < cli_config.rwnd; i++) {
		if (text_buffer[i].is_filled == -1) {
			count++;
		}
	}
	return count;
}
int same_host_name(char* server_ip, char* client_ip) {
	struct in_addr cli_addr_val;
	struct in_addr serv_addr_val;

	Inet_pton(AF_INET, client_ip, &cli_addr_val.s_addr);
	Inet_pton(AF_INET, server_ip, &serv_addr_val.s_addr);

	struct hostent* cli_val;
	struct hostent* serv_val;

	cli_val = gethostbyaddr(&cli_addr_val.s_addr, 4, AF_INET);
	serv_val = gethostbyaddr(&serv_addr_val.s_addr, 4, AF_INET);
	return 0;
}

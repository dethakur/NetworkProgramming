#include "unp.h"
#include "unpthread.h"
#include <unistd.h>
#include "unpthread.h"
#include "common.h"
#include <string.h>

#define RWND_SIZE 5
//void dg_cli_new(int,SA*,socklen_t);
void* buffer_reader_thread(void*);
void push_data_to_buffer(char*, int, int);
int get_window_size();
void create_new_connection(int);
void send_ack_to_server(int, struct sockaddr_in, socklen_t, int, uint32_t,
		int expected_seq);

//Link List of Buffers
struct udp_data send_hdr, recv_hdr;
struct buffer {
	char data[MAXLINE];
	uint32_t is_filled;
	uint32_t seq;
	struct buffer *next;
};

struct buffer text_buffer[RWND_SIZE];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char** argv) {

	int sockfd;
	pthread_t tid;
	struct sockaddr_in servaddr, cliaddr, sock_addr;
	query_obj q_obj;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5589);

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(0);

	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	//	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	int i = 0;
	for (i = 0; i < RWND_SIZE; i++) {
		text_buffer[i].is_filled = -1;
	}

	Pthread_create(&tid, NULL, &buffer_reader_thread, NULL);

	bind(sockfd, (SA*) &cliaddr, sizeof(cliaddr));

	bzero(&sock_addr, sizeof(sock_addr));
	socklen_t len = sizeof(struct sockaddr_in);
	getsockname(sockfd, (SA*) &sock_addr, &len);
	//	printf("CLient port = %d\n",ntohs(sock_addr.sin_port));

	Connect(sockfd, (SA*) &servaddr, sizeof(servaddr));

	struct sockaddr_in peer_sock;
	bzero(&peer_sock, sizeof(peer_sock));
	socklen_t len2 = sizeof(peer_sock);

	getpeername(sockfd, (SA*) &peer_sock, &len2);
	//	printf("Server port = %d\n",ntohs(peer_sock.sin_port));

	char sendline[MAXLINE], recvline[MAXLINE];

	strcpy(sendline, "file_name");
	//	printf("Comes here !?!\n");

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

	create_new_connection(new_port_number);
	printf("[Client]Done creating connection with new server");

	close(sockfd);
	printf("Writing done!!\n");

	return 0;

}

int ispresent(int seq_num) {
	int i = 0;
	for (; i < RWND_SIZE; i++) {
		if (text_buffer[i].is_filled != -1 && text_buffer[i].seq == seq_num) {
			return 1;
		}
	}
	return 0;
}
void update_expected_seq_num(int * expected_seq_num_ptr) {
	int i = 0;
	while (ispresent(*expected_seq_num_ptr) == 1) {
		*expected_seq_num_ptr += 1;
	}
}

void create_new_connection(int port_num) {
	printf("New port number = %d\n", port_num);
	int sockfd;
	struct sockaddr_in servaddr;
	int expected_seq_num = 0;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	query_obj q_obj;
	bzero(&q_obj, sizeof(q_obj));

	send_udp_data(sockfd, (SA*) &servaddr, sizeof(servaddr), &q_obj);

	fd_set rset;

	while (1) {
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		int maxfd1 = sockfd + 1;
		//		printf("[Client] Waiting for Data from server\n");
		Select(maxfd1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) {
			bzero(&q_obj, sizeof(query_obj));
			recv_udp_data(sockfd, NULL, 0, &q_obj);

			pthread_mutex_lock(&mutex);

			if (q_obj.config.type == data) {
				//				printf("[Data]Data Query Received\n");
				push_data_to_buffer((char*) &q_obj.buf, q_obj.config.seq,
						expected_seq_num);
			} else {
				printf("[Ping]Ping Query Received\n");
			}
			//			printf("[Client]Select interrupted in client!!!. Data received = %s\n",q_obj.buf);
			update_expected_seq_num(&expected_seq_num);
			send_ack_to_server(sockfd, q_obj.sock_addr,
					sizeof(q_obj.sock_addr), q_obj.config.seq, q_obj.config.ts,
					expected_seq_num);

			pthread_mutex_unlock(&mutex);
		}
	}
}

void push_data_to_buffer(char* send_buf, int seq_num, int expected_seq_num) {
	// TODO: need to make sure the buffer is cleared once full, else no datagram will be accepted
	int i = seq_num % RWND_SIZE;
	if (text_buffer[i].is_filled == -1) {
		if (seq_num != expected_seq_num) {
			printf(
					"Cannot accept this datagram. Expecting one with seq number\n",
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
		printf("Buffer is full, dropping datagram with seq num %d\n", seq_num);
	}
}

//This should read only continous packets
void* buffer_reader_thread(void* arg) {
	while (1) {
		int i = 0;
		pthread_mutex_lock(&mutex);
		for (i = 0; i < RWND_SIZE; i++) {
			if (text_buffer[i].is_filled == 1) {
				printf("[Buffer][Thread] File Data = %s with seq = %d\n",
						text_buffer[i].data, text_buffer[i].seq);
				bzero(&text_buffer[i].data, sizeof(text_buffer[i].data));
				text_buffer[i].is_filled = -1;
			}
		}
		pthread_mutex_unlock(&mutex);
		sleep(10);
	}

}
void send_ack_to_server(int sockfd, struct sockaddr_in cliaddr,
		socklen_t clilen, int seq, uint32_t ts, int expected_seq_num) {
	query_obj q_obj;
	q_obj.config.type == ack;
	q_obj.config.seq = expected_seq_num;
	q_obj.config.rwnd = get_window_size();
	q_obj.config.ts = ts;
	strcpy(q_obj.buf, "Hello!!");
	printf("Sending ack to server with new seq num %d\n", q_obj.config.seq);
	send_udp_data(sockfd, (SA*) &cliaddr, clilen, &q_obj);
}
int get_window_size() {
	int count = 0;
	int i = 0;
	for (i = 0; i < RWND_SIZE; i++) {
		if (text_buffer[i].is_filled == -1) {
			count++;
		}
	}
	return count;
}

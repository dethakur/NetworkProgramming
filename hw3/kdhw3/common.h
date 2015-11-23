#define DOMAIN_SOCK_PATH "/tmp/deva_kau"
#define MAX_MSG_LEN 50
#define MAX_VM_NAME_LEN 10

struct peer_info {
	char dest_port[100];
	char dest_ip[INET_ADDRSTRLEN];
	char src_port[100];
	char src_ip[INET_ADDRSTRLEN];
	char msg[MAX_MSG_LEN];
	char source_vm[MAX_VM_NAME_LEN];
}__attribute((packed));

int msg_send(int socket, char *dest_ip, char * dest_port, char * src_ip,
		char *src_port, char *msg, int flag, struct sockaddr_un * odr_addr_ptr);
int msg_recv(int socket, char *msg, char *src_ip, char *src_port,
		struct sockaddr_un * odr_addr_ptr);
void set_ip(char *host, char *ip);
void set_this_ip(char *this_ip);

void set_ip(char *host, char *ip) {
	struct hostent * hptr = gethostbyname(host);
	char **pptr = hptr->h_addr_list;
	Inet_ntop(hptr->h_addrtype, *pptr, ip, INET_ADDRSTRLEN);
}

void set_this_ip(char *this_ip) {
	char hostname[MAX_VM_NAME_LEN] = "";
	gethostname(hostname, MAX_VM_NAME_LEN);
	set_ip(hostname, this_ip);
}

int msg_send(int socket, char *dest_ip, char * dest_port, char * src_ip,
		char *src_port, char *msg, int flag, struct sockaddr_un * odr_addr_ptr) {
	char hostname[MAX_VM_NAME_LEN] = "";
	gethostname(hostname, MAX_VM_NAME_LEN);

	struct peer_info pinfo;
	bzero(&pinfo, sizeof(pinfo));
	strcpy(pinfo.dest_port, dest_port);
	strncpy(pinfo.dest_ip, dest_ip, INET_ADDRSTRLEN);
	strcpy(pinfo.src_port, src_port);
	strcpy(pinfo.src_ip, src_ip);
	strncpy(pinfo.source_vm, hostname, MAX_VM_NAME_LEN);
	strcpy(pinfo.msg, msg);

	int size = sizeof(struct peer_info);
	char odr_info[size];
	// copy all info into a char sequence to send the same to ODR
	memcpy(odr_info, &pinfo, size);

	struct peer_info pinfo2;
	bzero(&pinfo2, size);
	memcpy(&pinfo2, odr_info, size);

	// send all info to ODR
	sendto(socket, odr_info, strlen(odr_info), 0, odr_addr_ptr,
			sizeof(*odr_addr_ptr));
}

int msg_recv(int socket, char *msg, char *src_ip, char *src_port,
		struct sockaddr_un * odr_addr_ptr) {
	int size = sizeof(struct peer_info);
	char recvline[size];
	struct sockaddr_un odraddr;
	socklen_t odraddrlen;

	Recvfrom(socket, recvline, MAXLINE, 0, &odraddr, &odraddrlen);

	struct peer_info pinfo;
	bzero(&pinfo, size);
	memcpy(&pinfo, recvline, sizeof(pinfo));

	printf("time to check\n");
	printf("%s\n", pinfo.dest_port);
	printf("%s\n", pinfo.dest_ip);
	printf("%s\n", pinfo.src_port);
	printf("%s\n", pinfo.src_ip);
	printf("%s\n", pinfo.source_vm);
	printf("%s\n", pinfo.msg);

	strcpy(msg, pinfo.msg);
	strcpy(src_ip, pinfo.src_ip);
	strcpy(src_port, pinfo.src_port);
}

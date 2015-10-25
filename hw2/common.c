#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include	"unpifiplus.h"

void init_query_obj(void* servaddr,socklen_t servlen,struct query_obj* obj){
	if(servaddr == NULL){
		//If socket address is not given. Save the local socket address;
		obj->msgdata.msg_name = (SA*)&obj->sock_addr;
	}else{
		obj->msgdata.msg_name = servaddr;
	}
	if(servlen == 0){
		obj->msgdata.msg_namelen = sizeof(struct sockaddr_in);
	}else{
		obj->msgdata.msg_namelen = servlen;
	}


	obj->iovec_send[1].iov_base = (void*)&obj->config;
	obj->iovec_send[1].iov_len = sizeof(struct udp_data);
	obj->iovec_send[0].iov_base = (void*)&obj->buf;
	obj->iovec_send[0].iov_len = MAXLINE;
	obj->msgdata.msg_iovlen = 2;
	obj->msgdata.msg_iov = (void*)&obj->iovec_send;

}
void send_udp_data(int sockfd,void* servaddr,socklen_t servlen,struct query_obj* obj){
	init_query_obj(servaddr,servlen,obj);
	Sendmsg(sockfd,&obj->msgdata,0);
}

void recv_udp_data(int sockfd,void* servaddr,socklen_t servlen,struct query_obj* obj){
	init_query_obj(servaddr,servlen,obj);
	Recvmsg(sockfd,&obj->msgdata,0);
}
int get_addr_count(){
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	u_char		*ptr;
	int		i, family, doaliases;

	doaliases = 0;
	family = AF_INET;

	int count = 0;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 0);ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: \n", ifi->ifi_name);
		count ++;
	}
	free_ifi_info_plus(ifihead);
	return count;
}

void fill_addr_contents(iAddr *addr){
	struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
	u_char		*ptr;
	int		i;
	int count;
	count =0;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 0);
			ifi != NULL; ifi = ifi->ifi_next) {
		printf("%s: ", ifi->ifi_name);
		if ( (sa = ifi->ifi_addr) != NULL){
			strcpy(addr[count].ip_addr,Sock_ntop_host(sa, sizeof(*sa)));
//			printf("IP Addr = %s\n",Sock_ntop_host(sa, sizeof(*sa)));
		}
		if ( (sa = ifi->ifi_ntmaddr) != NULL){
			strcpy(addr[count].mask,Sock_ntop_host(sa, sizeof(*sa)));
		}
		count++;

	}
	free_ifi_info_plus(ifihead);
}

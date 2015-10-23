#include "unpthread.h"
#include "common.h"
#include	<time.h>
#include <fcntl.h>
#include <sys/file.h>

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

#include "common.h"

void get_ip_from_host(char* host,char* str){

	struct hostent* host_val;

	host_val = gethostbyname(host);
	if(host_val == NULL){
		printf("Host name is not found \n");
	}else{
		char **pptr = host_val->h_addr_list;
		for ( ; *pptr != NULL; pptr++){
			Inet_ntop(host_val->h_addrtype, *pptr, str, INET_ADDRSTRLEN);
		}
	}
}

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

#include "unp.h"
#include "hw_addrs.h"
#define IP_LEN 20
#define DOMAIN_SOCK_PORT "/tmp/_deva_kau_temp"

struct server_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
};
typedef struct server_details server_details;

struct hwaddr {
	int sll_ifindex; /* Interface number */
	unsigned short sll_hatype; /* Hardware type */
	unsigned char sll_halen; /* Length of address */
	char sll_addr[8]; /* Physical layer address */
}__attribute((packed));

int areq(char *ip, struct hwaddr *hwaddr);
void get_ip_from_host(char* host,char* str);
void get_host_from_ip(char*,char* );

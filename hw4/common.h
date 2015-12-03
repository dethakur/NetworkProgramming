#include "unp.h"
#include "hw_addrs.h"
#define IP_LEN 20

struct server_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
};
typedef struct server_details server_details;


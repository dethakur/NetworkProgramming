#include "common.h"
#include "unp.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>


#define NO_FD 0  // although 0 is for stdin it is ok since it is not a socket fd
#define OUR_PF_PROTOCOL "_dkd_1732"

//#define ETH_FRAME_LEN 1518

struct arp_cache_details {
	char ip[IP_LEN];
	char mac[IF_HADDR];
	int index;
	int fd;
	int filled;
};
typedef struct arp_cache_details arp_cache_details;

struct arp_req_reply {
	char eth_dest[IF_HADDR];
	char eth_src[IF_HADDR];
	unsigned short int frame_type;
	// *** header ends and req/reply starts **
	unsigned short int hard_type;
	unsigned short int prot_type;
	unsigned short int hard_size;
	unsigned short int prot_size;
	unsigned short int op;
	char sender_eth[IF_HADDR];
	char sender_ip[IP_LEN];
	char target_eth[IF_HADDR];
	char target_ip[IP_LEN];
	char identification[3];
}__attribute((packed));
typedef struct arp_req_reply arp_req_reply;

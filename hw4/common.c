#include "common.h"


void display_mac_addr(char* mac_addr) {
	int i = 0;
	char* ptr = mac_addr;
	i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);

}



int populate_server_details(struct server_details* serv) {
	struct hwa_info *hwa, *hwahead;
	struct sockaddr *sa;
	char *ptr;
	int i, prflag;
	char ip_addr[IP_LEN];
	bzero(ip_addr, sizeof(ip_addr));

	int count = 0;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if (strcmp(hwa->if_name, "eth0") != 0)
			continue;

		prflag = 0;
		i = 0;
		bzero(ip_addr, sizeof(ip_addr));
		if ((sa = hwa->ip_addr) != NULL)
			sprintf(ip_addr, "%s", Sock_ntop_host(sa, sizeof(*sa)));

		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {

			bzero(&serv[count], sizeof(struct server_details));
			strcpy(serv[count].ip, ip_addr);
			memcpy(serv[count].mac, hwa->if_haddr, IF_HADDR);
			serv[count].index = hwa->if_index;

			count++;
		}

	}
	free_hwa_info(hwahead);
	return count;
}


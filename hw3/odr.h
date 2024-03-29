#include <stdio.h>
#include <sys/socket.h>
#include "common.h"
#include "unp.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

static struct server_details serv[2];
static int number_of_interfaces;
static routing_table table;
static int broadcast_id = 1;
static int rawfd, dgramfd;
static struct sockaddr_un serveraddr;
volatile static int packet_counter = 0;



void send_rreq(int, int, int, char*, char*,int);
void send_packet(char*, char*, int, int, frame_head*);
void process_frame(char*);
void send_payload(char*, char*, data_type,char*,int);
static server_buf buffer[100];
static duplicate_packet dup_packet[10];

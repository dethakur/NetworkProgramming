
CC = gcc

LIBS =  -lsocket\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: server client clean

server: server.o get_ifi_info_plus.o
	${CC} ${FLAGS} -o server server.o common.o get_ifi_info_plus.o ${LIBS}

server.o: server.c common.h common.o
	${CC} ${CFLAGS} -c server.c 

client: client.o get_ifi_info_plus.o common.o
	${CC} ${FLAGS} -o client client.o common.o get_ifi_info_plus.o ${LIBS}
	
prifinfo_plus: prifinfo_plus.o get_ifi_info_plus.o
	${CC} ${FLAGS} -o prifinfo_plus prifinfo_plus.o get_ifi_info_plus.o ${LIBS}
	
client.o: client.c common.h
	${CC} ${CFLAGS} -c client.c

common.o: common.c common.h
	${CC} ${CFLAGS} -c common.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c

prifinfo_plus.o: prifinfo_plus.c
	${CC} ${CFLAGS} -c prifinfo_plus.c

clean:
	rm server.o client.o get_ifi_info_plus.o common.o


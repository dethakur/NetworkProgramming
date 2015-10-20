
CC = gcc

LIBS =  -lsocket\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: get_ifi_info_plus.o prifinfo_plus.o
	${CC} -o prifinfo_plus prifinfo_plus.o get_ifi_info_plus.o ${LIBS}

server: server.o get_ifi_info_plus.o
	${CC} ${FLAGS} -o server server.o get_ifi_info_plus.o ${LIBS}

server.o: server.c
	${CC} ${CFLAGS} -c server.c 

client: client.o get_ifi_info_plus.o
	${CC} ${FLAGS} -o client client.o get_ifi_info_plus.o ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c

prifinfo_plus.o: prifinfo_plus.c
	${CC} ${CFLAGS} -c prifinfo_plus.c

clean:
	rm prifinfo_plus prifinfo_plus.o get_ifi_info_plus.o server.o client.o


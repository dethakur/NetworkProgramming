
CC = gcc

LIBS =  -lnsl\
	/home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e/lib

all: odr_dev_kau server_dev_kau client_dev_kau clean

odr_dev_kau: get_hw_addrs.o odr.o common.o
	${CC} ${FLAGS} -o odr_dev_kau odr.o common.o get_hw_addrs.o ${LIBS}
	
pwhaddr: get_hw_addrs.o prhwaddrs.o
	${CC} ${CFLAGS} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}
get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c
	
odr.o: odr.c odr.h common.h hw_addrs.h
	${CC} ${CFLAGS} -c odr.c

common.o: common.c common.h
	${CC} ${CFLAGS} -c common.c	

server_dev_kau: server.o common.o
	${CC} ${FLAGS} -o server_dev_kau server.o get_hw_addrs.o common.o ${LIBS}

server.o: server.c  common.h
	${CC} ${CFLAGS} -c server.c 

client_dev_kau: client.o common.o
	${CC} ${FLAGS} -o client_dev_kau client.o get_hw_addrs.o common.o ${LIBS}	
	
client.o: client.c common.h
	${CC} ${CFLAGS} -c client.c

clean:
	rm *.o
	
remove:
	rm odr

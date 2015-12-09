
CC = gcc

LIBS =  -lnsl\
	/home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2 -pthread
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e/lib

all: tour arp clean

tour: get_hw_addrs.o tour.o common.o
	${CC} ${FLAGS} -o tour tour.o common.o get_hw_addrs.o ${LIBS}
	
pwhaddr: get_hw_addrs.o prhwaddrs.o
	${CC} ${CFLAGS} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}
get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c
	
tour.o: tour.c tour.h common.h hw_addrs.h
	${CC} ${CFLAGS} -c tour.c

common.o: common.c common.h
	${CC} ${CFLAGS} -c common.c	

arp: arp.o common.o
	${CC} ${FLAGS} -o arp arp.o get_hw_addrs.o common.o ${LIBS}

arp.o: arp.c  common.h
	${CC} ${CFLAGS} -c arp.c 

clean:
	rm *.o
	
remove:
	rm tour arp
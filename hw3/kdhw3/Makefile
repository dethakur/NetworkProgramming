
CC = gcc

LIBS =  -lnsl\
	/home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e/lib

all: server client clean

server: server.o
	${CC} ${FLAGS} -o server server.o  ${LIBS}

server.o: server.c 
	${CC} ${CFLAGS} -c server.c 

client: client.o 
	${CC} ${FLAGS} -o client client.o ${LIBS}
	
	
client.o: client.c
	${CC} ${CFLAGS} -c client.c

clean:
	rm server.o client.o 


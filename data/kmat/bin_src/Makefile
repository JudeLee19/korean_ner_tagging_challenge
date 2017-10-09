CC = gcc
CFLAGS = -O3 -Wall -g
#CFLAGS = -O2 -g -pg
RM= rm -f
CP= cp

BIN= ../bin
LIB= ../lib
MY_BIN= $(HOME)/bin

INCLUDE         = -I../include
LDFLAGS         = -L$(LIB)

.c.o: 
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

all: 	kma kmat

kma: kma_run.o 
	$(CC) -o $(BIN)/$@ kma_run.o $(LDFLAGS) -lkmat -lm

kmat: kmat_run.o 
	$(CC) -o $(BIN)/$@ kmat_run.o $(LDFLAGS) -lkmat -lm

clean: 	rmobj rmbin

rmobj:
	${RM} *.o 

rmbin:
	${RM} $(BIN)/kma
	${RM} $(BIN)/kmat

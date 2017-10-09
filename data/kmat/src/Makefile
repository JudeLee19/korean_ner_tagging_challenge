CC = gcc
CFLAGS = -O3 -Wall -g -pg
#CFLAGS = -O2 -g -pg
RM= rm -f
CP= cp

BIN= ../bin
LIB= ../lib
MY_BIN= $(HOME)/bin

INCLUDE         = -I../include \
		  -I../exploit

LDFLAGS         = -L$(LIB) -L../exploit

.c.o: 
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

KMAT_OBJS	= ma.o \
	ma_e.o \
	phonetic_recover.o \
	ma_m.o \
	cyk.o \
	triangular_matrix.o  \
	unit_conversion.o \
	get_morph_tag.o \
	ma_s.o \
	bfs.o \
	strvector.o \
	tagging.o \
	viterbi.o \
	get_sentence.o \
	kmat.o \
	split_word.o \
	user_dic.o

EXPLOIT		= ../exploit
EXPLOIT_OBJS	= $(EXPLOIT)/dafst.o \
		  $(EXPLOIT)/fst_get_probability.o $(EXPLOIT)/fst_probability_io.o $(EXPLOIT)/load_double.o  \
		  $(EXPLOIT)/t2b.o $(EXPLOIT)/b2t.o \
		  $(EXPLOIT)/hsplit.o \
		  $(EXPLOIT)/binio.o \
		  $(EXPLOIT)/fstdic.o

all: 	makeexploit libkmat.a makebin

makeexploit:
	cd ../exploit; make

libkmat.a: $(EXPLOIT_OBJS) $(KMAT_OBJS)
	$(AR) rcv $(LIB)/$@ $(EXPLOIT_OBJS) $(KMAT_OBJS)
	ranlib $(LIB)/$@

makebin:
	cd ../bin_src; make clean; make

clean: 	rmobj rmlib

rmobj:
	${RM} *.o 

rmlib:
	${RM} $(LIB)/libkmat.a

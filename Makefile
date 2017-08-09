CC=gcc
CFLAGS=-Wall
ODIR = .
SDIR = src
INC = -Iinc

_OBJS = main.o thread_pool.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -g $(INC) -o $@ $< $(CFLAGS)

shabf: $(OBJS)
	$(CC) -o shabf $^ -g -lm -pthread -lssl -lcrypto

all: shabf

.PHONY: clean

clean:
	rm -f $(ODIR)/shabf
	rm -f $(ODIR)/*.o
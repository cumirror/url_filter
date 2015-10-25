CC=gcc
CFLAGS=-O2 -Wall
PROG=url_filter
LIBS=md5/md5c.o HashFilter/hash.o CuckooFilter/cuckoo_filter.o BloomFilter/bloom.o
LIB_H=md5/md5.h HashFilter/hash.h CuckooFilter/cuckoo_filter.h BloomFilter/bloom.h
OBJS=main.o $(LIBS)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

main.o: $(LIB_H)

.PHONY: clean
clean:
	rm -f *.o $(LIBS) $(PROG)

backup: clean
	cd .. ; tar jcvf url_filter.tar.bz2 url_filter

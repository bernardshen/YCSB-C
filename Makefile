CC=g++
CFLAGS=-std=c++11 -g -Wall -pthread -I./ -I/home/sjc/learn/learn-kv/mykv/include/
# LDFLAGS= -lpthread -ltbb -lhiredis
LDFLAGS= /home/sjc/learn/learn-kv/mykv/build/src/libMyKV.so -lpthread -ltbb -libverbs -lhiredis	# added by sjc
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)


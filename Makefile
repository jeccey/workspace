#makefile
all: libevent_server libevent_client
CXX      = g++
CXXFLAGS = -Wall -levent

CC       = gcc
CCFLAGS  = -g -levent

OBJS_S     = libevent_server.o
OBJS_C     = libevent_client.o

libevent_server : $(OBJS_S)
	$(CXX) -o $@ $(OBJS_S) -levent

libevent_client : $(OBJS_C)
	$(CC) -o $@ $(OBJS_C) -levent

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $<

%.o : %.c
	$(CC) -c $(CCFLAGS) $<

clean:
	@echo "cleanning project"
	-rm libevent_server libevent_client *.o *~
	@echo "clean completed"
.PHONY: clean

OBJS = dataServer.o modules/MyQueue.o
OBJS2 = remoteClient.o
SOURCE = dataServer.c modules/MyQueue.c
SOURCE2 = remoteClient.c
HEADER = include/common_types.h include/MyQueue.h include/myDeclarations.h
OUT = dataServer
OUT2 = remoteClient
CC = gcc
FLAGS = -g -c -lpthread

all: $(OUT) $(OUT2)

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $@ -lpthread

$(OUT2): $(OBJS2)
	$(CC) -g $(OBJS2) -o $@

dataServer.o: dataServer.c
	$(CC) $(FLAGS) dataServer.c

remoteClient.o: remoteClient.c
	$(CC) $(FLAGS) remoteClient.c

modules/MyQueue.o: modules/MyQueue.c
	$(CC) $(FLAGS) modules/MyQueue.c -o $@

clean:
	rm -f $(OBJS) $(OUT)
	rm -f $(OBJS2) $(OUT2)
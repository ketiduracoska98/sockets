# Protocoale de comunicatii:
# Makefile
CC=gcc
CFLAGS = -Wall -g
EXE1=server
EXE2=subscriber
all: server subscriber

# Compileaza server.c
server: server.c
	$(CC) $^ $(CFLAGS) -o $(EXE1)

# Compileaza client.c
subscriber: subscriber.c 
	$(CC) $^ $(CFLAGS) -o $(EXE2) -lm

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber $(CLIENT_ID) ${IP_SERVER} ${PORT} 

clean:
	rm -f server subscriber

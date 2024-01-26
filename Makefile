CFLAGS = -pthread -std=c99

all: server client

server: server.c
	gcc server.c -o server $(CFLAGS)

client: client.c 
	gcc client.c -o client $(CFLAGS)

clean:
	rm -f server client

CFLAGS := -g
targets = client.out server.out
src  = $(wildcard *.c)
objs = $(wildcard *.o)

all: $(targets)
.PHONY:	all
	
client.out:	net protocol_client protocol_session protocol_utils
	gcc $(CFLAGS) -c -pthread client client/client.c
	gcc $(CFLAGS) -o client.out client.o net.o protocol_client.o protocol_session.o protocol_utils.o -pthread
	
server.out: net protocol_server protocol_session protocol_utils maze
	gcc $(CFLAGS) -c -pthread server server/server.c
	gcc $(CFLAGS) -o server.out server.o net.o protocol_server.o protocol_session.o protocol_utils.o maze.o -pthread
	
net:	
	gcc $(CFLAGS) -c lib/net.c

protocol_utils:
	gcc $(CFLAGS) -c lib/protocol_utils.c

protocol_client: protocol_utils protocol_session
	gcc $(CFLAGS) -c -pthread lib/protocol_client.c

protocol_server: protocol_utils protocol_session
	gcc $(CFLAGS) -c -pthread lib/protocol_server.c

protocol_session: protocol_utils net
	gcc $(CFLAGS) -c lib/protocol_session.c

maze:
	gcc $(CFLAGS) -c lib/maze.c
clean:
	rm $(objs) $(targets)

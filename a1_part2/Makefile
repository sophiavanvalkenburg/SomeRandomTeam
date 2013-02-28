CFLAGS := -g
targets = client.out server.out
src  = $(wildcard *.c)
objs = $(wildcard *.o)

all: $(targets)
.PHONY:	all
	
client.out:	net
	gcc $(CFLAGS) -c client client/client.c
	gcc $(CFLAGS) -o client.out client.o net.o -lpthread
	
server.out:	net
	gcc $(CFLAGS) -c server server/server.c
	gcc $(CFLAGS) -o server.out server.o net.o -lpthread
	
net:	
	gcc $(CFLAGS) -c lib/net.c
	
clean:
	rm $(objs) $(targets)

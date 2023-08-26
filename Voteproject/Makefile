executable = poller
executable2 = pollSwayer
all : poller pollSwayer

poller: poller.c
	gcc -o poller poller.c -lpthread -g

pollSwayer: pollSwayer.c
	gcc -o pollSwayer pollSwayer.c -lpthread -g


clean:
	rm -f $(executable) $(executable2)  
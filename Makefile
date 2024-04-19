all:
	mkdir -p bin
	gcc -Wall client.c -o bin/client
	gcc -Wall server.c -o bin/server -lm

clean:
	rm client server

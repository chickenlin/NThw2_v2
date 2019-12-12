all:
	gcc hw2_server.c -o server -lpthread
	gcc hw2_client.c -o client -lpthread
clean:
	rm server client

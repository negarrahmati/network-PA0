all:
	g++ server.cpp  -lpthread -o server
	g++ player.cpp -o player
clean:
	rm server client


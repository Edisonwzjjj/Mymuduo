all: server client

server: server.cpp
	g++ -g -std=c++20 $^ -o $@

client: Server
	g++ -g -std=c++20 $^ -o $@

.PHONY:clean
clean:
	rm -rf server client


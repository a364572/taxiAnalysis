server: server.cpp utils.cpp commom.cpp utils.h commom.h
	g++ server.cpp utils.cpp commom.cpp -o server --std=c++11 -lmx -leng -lpthread

client: client.cpp commom.cpp utils.h commom.h
	g++ client.cpp commom.cpp -o client --std=c++11

clean:
	rm server client

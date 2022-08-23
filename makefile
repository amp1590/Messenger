all: chat_server.x chat_client.x
	

chat_server.x: chat_server.cpp
	g++ -std=c++11 chat_server.cpp -o chat_server.x 

chat_client.x: chat_client.cpp
	g++ -std=c++11 chat_client.cpp -o chat_client.x -pthread

clean:
	rm -f *.x
	rm -f *~


all : socks5.o server.o proxy.o client.o echoserver.o sockserver.o
	g++ -o proxy proxy.o client.o
	g++ -o server server.o socks5.o
	g++ -o echoserver echoserver.o
	g++ -o sockserver sockserver.o socks5.o

sockserver.o: sockserver.cc sockserver.h
	g++ -c sockserver.cc
	
echoserver.o:echoserver.cc
	g++ -c echoserver.cc

server.o: server.cc server.h
	g++ -c server.cc

socks5.o:socks5.cc socks5.h common.h
	g++ -c socks5.cc

proxy.o : proxy.cc client.h
	g++ -c proxy.cc

client.o: client.cc client.h
	g++ -c client.cc

clean:
	rm -f *.o *.so proxy server


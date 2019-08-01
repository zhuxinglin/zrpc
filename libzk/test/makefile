all:
	g++ -o Con main.cpp -std=c++11 -I../../libnet/include ../libzk.a ../../libnet/libnet.a -lpthread -L/usr/local/lib -lssl -lcrypto -lrt -ldl

clean:
	rm -rf Con
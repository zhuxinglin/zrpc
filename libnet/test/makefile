

all:
	rm Svc -rf
	g++ -o Svc svc.cpp -I../include ../libnet.a -lpthread -g -lrt -ldl -L/usr/local/lib -lssl -lcrypto -std=c++11
	g++ -o Tsl test_spin_lock.cpp -I../include ../libnet.a -lpthread -lrt -g -std=c++11
	g++ -o Chan test_chan.cpp -I../include ../libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto -g -std=c++11

clean:
	rm Svc
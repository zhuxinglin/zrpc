

all:
	rm Svc -rf
	g++ -o Svc svc.cpp -I../include ../libnet.a -lpthread -g -lrt
	g++ -o Tsl test_spin_lock.cpp -I../include ../libnet.a -lpthread -lrt -g 

clean:
	rm Svc
CXX=g++
CC=gcc
INCLUDES= -I../libnet/include -I./ -I../zsvc/include -I/usr/local/include
VERSION=1_2_1
CFLAGS=-g -Wall -D_REENTRANT -Wno-invalid-offsetof -fvisibility=hidden -shared -fPIC -DVERSION -Wl,--as-needed
LIBS= ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto

OBJS+=test_ad.o

EXE=test_ad.so.V$(VERSION)

%.o:%.cpp
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(SRC)/*.o $(EXE) ./*.o
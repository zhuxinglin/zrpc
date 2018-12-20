CXX=g++
CC=gcc
INCLUDES= -I../libnet/include -I./ -I../zsvc/include
VERSION=1_2_1
CFLAGS=-g -Wall -D_REENTRANT -Wno-invalid-offsetof -fvisibility=hidden -fPIC -shared -DVERSION
LIBS= ../libnet/libnet.a

OBJS+=test_ad.o

EXE=test_ad.so.V$(VERSION)

%.o:%.cpp
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(SRC)/*.o $(EXE) ./*.o
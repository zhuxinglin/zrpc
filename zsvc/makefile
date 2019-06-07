CXX=g++
CC=gcc
LIBS= ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto
INCLUDES= -I../libnet/include -I./src -I./include -I/usr/local/include
CFLAGS=-O2 -Wall -D_REENTRANT -Wl,--as-needed -Wno-invalid- -std=c++11
SRC=src

OBJS+=main.o
-include $(SRC)/makefile.mk

EXE=zsvc

$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(SRC)/$*.o

%.o:%.cpp
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(SRC)/*.o $(EXE) ./*.o



CC := gcc
CXX := g++
AR := ar
SRC = src
INCLUDES=-I./include -I./src -I/usr/local/include -I../libnet/include

LIBS=
CFLAGS	= -O2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC -std=c++11

-include $(SRC)/makefile.mk

OBJS += 

LIB=libzk.a

$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(SRC)/$*.o

all:$(LIB)

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

clean:
	rm -rf $(SRC)/*.o $(LIB)

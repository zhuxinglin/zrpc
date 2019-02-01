
CC := gcc
CXX := g++
AR := ar
SRC = src
CORE = core
INCLUDES=-I./include -I./core -I./src -I/usr/local/include

LIBS=
CFLAGS	= -O2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC

-include $(SRC)/makefile.mk
-include $(CORE)/makefile.mk

OBJS += 

LIB=libnet.a

$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(SRC)/$*.o

$(CORE)/%.o: $(CORE)/%.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(CORE)/$*.cpp -o $(CORE)/$*.o

%.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $*.o

all:$(LIB)

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

clean:
	rm -rf $(SRC)/*.o $(LIB) $(CORE)/*.o

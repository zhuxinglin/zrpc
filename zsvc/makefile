CXX=g++
CC=gcc
LIBS= ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto
INCLUDES= -I../libnet/include -I./src -I./include -I/usr/local/include
CFLAGS=-g2 -Wall -D_REENTRANT -Wl,--as-needed -Wno-invalid- -std=c++11
SRC=src
BUILD_DIR=build

OBJS+=$(BUILD_DIR)/main.o
-include $(SRC)/makefile.mk

EXE=zsvc

$(BUILD_DIR)/%.o: $(SRC)/%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $(BUILD_DIR)/$*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)


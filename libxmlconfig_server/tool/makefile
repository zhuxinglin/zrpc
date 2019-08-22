CXX=g++
CC=gcc
LIBS= ../../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto
INCLUDES= -I../../libconet/include -I../ -I./include -I/usr/local/include -I../../../code/include -I../../libmysql_cli/include
CFLAGS=-g2 -Wall -D_REENTRANT -Wl,--as-needed -Wno-invalid- -std=c++11
SRC=
BUILD_DIR=build

OBJS+=$(BUILD_DIR)/xml_config_tool.o

EXE=xml_config_tool

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $(BUILD_DIR)/$*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)


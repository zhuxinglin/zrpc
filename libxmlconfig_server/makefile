CXX=g++
CC=gcc
INCLUDES= -I../libnet/include -I./ -I../corpc/include -I/usr/local/include -I../libmysql_cli/include -I../../code/include
VERSION=1.0.0
CFLAGS=-g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -fvisibility=hidden -shared -fPIC -DVERSION -Wl,--as-needed -std=c++11
LIBS= ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto ../libmysql_cli/libmysql_cli.a
BUILD_DIR= build

OBJS+=$(BUILD_DIR)/config_server.o $(BUILD_DIR)/dao_config.o

EXE=libxmlconfig_server.so.V$(VERSION)

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $(BUILD_DIR)/$*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)

CXX=g++
CC=gcc
INCLUDES= -I../libnet/include -I./ -I/usr/local/include -I../libmysql_cli/include -I../libshm_config/include
CFLAGS=-g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -shared -fPIC -DVERSION -Wl,--as-needed -std=c++11 
LIBS= ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto ../libshm_config/libshmconfig.a
BUILD_DIR= build
VERSION=1.0.0

OBJS+=$(BUILD_DIR)/xml_config.o 

EXE=libxml_config.so.V$(VERSION)

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $(BUILD_DIR)/$*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(BUILD_DIR) $(EXE)

CXX=g++
CC=gcc
INCLUDES= -I../libnet/include -I./ -I../zsvc/include -I/usr/local/include -I../libzk/include
VERSION=_1_2_0
CFLAGS=-g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -fvisibility=hidden -shared -fPIC -DVERSION -Wl,--as-needed -std=c++11
LIBS= ../libzk/libzk.a ../libnet/libnet.a -lpthread -lrt -ldl -L/usr/local/lib -lssl -lcrypto
BUILD_DIR= build

OBJS+=$(BUILD_DIR)/config_agent.o $(BUILD_DIR)/config_zk.o $(BUILD_DIR)/config_watcher.o $(BUILD_DIR)/config_shm.o

EXE=libconfig_agent.so.V$(VERSION)

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $(BUILD_DIR)/$*.o

$(EXE):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) 

all:$(EXE)

clean:
	rm -rf $(EXE) $(BUILD_DIR)

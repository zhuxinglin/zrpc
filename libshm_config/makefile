
CC := gcc
CXX := g++
AR := ar
BUILD_DIR = build
INCLUDES=-I../libshmconfig_agent/include -I/usr/local/include -I../libnet/include

LIBS=
CFLAGS	= -g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC -std=c++11
CFLAGS_C	= -g2 -Wall -D_REENTRANT -static -fPIC

OBJS += $(BUILD_DIR)/shm_config.o $(BUILD_DIR)/shm_config_go.o

LIB=libshmconfig.a

$(BUILD_DIR)/%.o:%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CXX) -c $(CFLAGS) $(INCLUDES) $*.cpp -o $(BUILD_DIR)/$*.o

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

all:$(LIB)

clean:
	rm -rf $(BUILD_DIR) $(LIB)

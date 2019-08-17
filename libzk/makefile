
CC := gcc
CXX := g++
AR := ar
SRC = src
HASH = src/hashtable
BUILD_DIR = build
INCLUDES=-I./include -I./src -I/usr/local/include -I../libnet/include

LIBS=
CFLAGS	= -O2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC -std=c++11
CFLAGS_C	= -g2 -Wall -D_REENTRANT -static -fPIC

-include $(SRC)/makefile.mk

OBJS += 

LIB=libzk.a

$(BUILD_DIR)/%.o: $(SRC)/%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.o: $(HASH)/%.c
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CC) -c $(CFLAGS_C) $(INCLUDES) $(HASH)/$*.c -o $(BUILD_DIR)/$*.o

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

all:$(LIB)

clean:
	rm -rf $(BUILD_DIR) $(LIB)

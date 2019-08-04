
CC := gcc
CXX := g++
AR := ar
SRC = src
CORE = core
BUILD_DIR=build
INCLUDES=-I./include -I./core -I./src -I/usr/local/include

LIBS=
CFLAGS	= -O2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC -std=c++11

-include $(SRC)/makefile.mk
-include $(CORE)/makefile.mk

OBJS += 

LIB=libnet.a

$(BUILD_DIR)/%.o: $(SRC)/%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.o: $(CORE)/%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(CORE)/$*.cpp -o $(BUILD_DIR)/$*.o

%.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CFLAGS) $*.cpp -o $*.o

all:$(LIB)

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

clean:
	rm -rf $(BUILD_DIR) $(LIB)

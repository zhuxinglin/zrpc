
CC := gcc
CXX := g++
AR := ar
SRC = src
BUILD_DIR = build
INCLUDES=-I../include -I/usr/local/include -I../libconet/include -I../../code/include/

LIBS=
CFLAGS	= -g2 -Wall -D_REENTRANT -Wno-invalid-offsetof -static -fPIC -std=c++11
CFLAGS_C	= -g2 -Wall -D_REENTRANT -static -fPIC

OBJS += $(BUILD_DIR)/mysql_cli.o $(BUILD_DIR)/mysql_helper.o

LIB=libmysql_cli.a

$(BUILD_DIR)/%.o:$(SRC)/%.cpp
	if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi; \
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(SRC)/$*.cpp -o $(BUILD_DIR)/$*.o

$(LIB): $(OBJS)
	$(AR) -rcs $@ $^

all:$(LIB)

clean:
	rm -rf $(BUILD_DIR) $(LIB)

CC = g++
CFLAGS = -std=c++11
LIBS = -lpthread -latomic -lsqlite3
DIR_OBJS = obj
DIR_BIN = bin
DIR_SRC = src

dirs := $(DIR_OBJS) $(DIR_BIN)
bin = ef
src = $(wildcard $(DIR_SRC)/*.cpp $(DIR_SRC)/os/linux/*.cpp)
obj = $(patsubst %.cpp,%.o,$(src))

obj := $(addprefix $(DIR_OBJS)/,$(obj))
bin := $(addprefix $(DIR_BIN)/,$(bin))

all: $(dirs) $(bin)
$(dirs):
	mkdir obj
	mkdir obj/src
	mkdir obj/src/os
	mkdir obj/src/os/linux
	mkdir bin

$(bin):$(obj)
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)

$(DIR_OBJS)/%.o:%.cpp
	$(CC) -c $^ -o $@ $(CFLAGS) $(LIBS)

clean:
	rm -rf $(dirs) $(bin)

.PHONY:all clean

CC = g++
CFLAGS = -std=c++11 
LIBS = -lpthread -latomic
DIR_OBJS = ../obj
DIR_BIN = ../bin

dirs := $(DIR_OBJS) $(DIR_BIN)
bin = test
src = $(wildcard *.cpp os/linux/*.cpp)
obj = $(patsubst %.cpp,%.o,$(notdir $(src)))

obj := $(addprefix $(DIR_OBJS)/,$(obj))
bin := $(addprefix $(DIR_BIN)/,$(bin))

# all: $(dirs) $(bin)
all:
	@echo $(obj)
	@echo $(bin)

$(dirs):
	mkdir $@

$(bin):

clean:
	rm -rf $(dirs) $(bin)

.PHONY:all clean
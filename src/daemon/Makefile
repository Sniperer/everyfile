CC = g++
CFLAGS = -std=c++11
LIBS = -lpthread -latomic -lsqlite3 

obj := ../../obj/src/ef_db.o ../../obj/src/ef_file.o ../../obj/src/os/linux/*.o

all:
	$(CC) $(CFLAGS) $(obj) efd.cpp -o efd $(LIBS)

clean:
	rm -rf efd

.PHONY:all clean

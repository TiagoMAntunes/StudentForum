CC=gcc
FLAGS=-Wall -g -lm
TARGETS=FS user
SOURCES=server.c user.c
OBJS = $(SOURCES:%.c=%.o)
DEPENDENCIES=lib/list.o lib/iterator.o lib/hash.o lib/topic.o lib/file_management.o
all: $(TARGETS)

FS: server.o $(DEPENDENCIES)
	$(CC) $(FLAGS) $^ -o FS

user: user.o $(DEPENDENCIES)
	$(CC) $(FLAGS) $^ -o user

server.o: server.c 
user.o: user.c
lib/hash.o: lib/hash.c lib/hash.h 
lib/list.o: lib/list.c lib/list.h 
lib/iterator.o: lib/iterator.c lib/iterator.h
lib/file_management.o: lib/file_management.c lib/file_management.h
lib/topic.o: lib/topic.c lib/topic.h
$(OBJS):
	$(CC) $(FLAGS) -c -o $@ $<

$(DEPENDENCIES):
	$(CC) $(FLAGS) -c -o $@ $<
clean:
	rm $(TARGETS) *.o lib/*.o

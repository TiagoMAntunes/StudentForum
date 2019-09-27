CC=gcc
FLAGS=-Wall -g -lm
TARGETS=FS user
SOURCES=server.c user.c
OBJS = $(SOURCES:%.c=%.o)
DEPENDENCIES=list.o iterator.o hash.o topic.o file_management.o
all: $(TARGETS)

FS: server.o $(DEPENDENCIES)
	$(CC) $(FLAGS) $^ -o FS

user: user.o $(DEPENDENCIES)
	$(CC) $(FLAGS) $^ -o user

server.o: server.c 
user.o: user.c
hash.o: hash.c hash.h list.o
list.o: list.c list.h iterator.o
iterator.o: iterator.c iterator.h
file_management.o: file_management.c file_management.h
topic.o: topic.c topic.h
$(OBJS):
	$(CC) $(FLAGS) -c -o $@ $<

$(DEPENDENCIES):
	$(CC) $(FLAGS) -c -o $@ $<
clean:
	rm $(TARGETS) *.o

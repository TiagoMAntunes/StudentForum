CC=gcc
FLAGS=-Wall -g -lm
TARGETS=FS user
SOURCES=server.c user.c
OBJS = $(SOURCES:%.c=%.o)
DEPENDENCIES=list.o iterator.o hash.o topic.o
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


$(OBJS):
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm $(TARGETS) $(OBJS)
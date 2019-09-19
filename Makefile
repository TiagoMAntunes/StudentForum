CC=gcc
FLAGS=-Wall -g -lm
TARGETS=FS user
SOURCES=server.c user.c
OBJS = $(SOURCES:%.c=%.o)

all: $(TARGETS)

FS: server.o
	$(CC) $(FLAGS) $^ -o FS

user: user.o
	$(CC) $(FLAGS) $^ -o user

server.o: server.c
user.o: user.c

$(OBJS):
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm $(TARGETS) $(OBJS)
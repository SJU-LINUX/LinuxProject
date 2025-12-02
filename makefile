CC = gcc
CFLAGS = -W -Wall
TARGET = App
OBJS = main.o client.o server.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c common.h client.h server.h
	$(CC) $(CFLAGS) -c main.c

client.o: client.c client.h common.h
	$(CC) $(CFLAGS) -c client.c
server.o: server.c server.h common.h
	$(CC) $(CFLAGS) -c server.c
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f data
	rm -f sm*_8x8 sm*_4x4
	rm -f client_gathered_*
	rm -f client_sorted_*
	rm -f server_*.bin

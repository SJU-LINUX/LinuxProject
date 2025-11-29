CC = gcc
CFLAGS = -W -Wall
TARGET = App
OBJS = main.o client.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c common.h client.h
	$(CC) $(CFLAGS) -c main.c

client.o: client.c client.h common.h
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f data
	rm -f sm*_8x8 sm*_4x4
	rm -f client_gathered_*
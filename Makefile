TARGET=btrfs_parser

OBJS=main.o btrfs.o

CC=clang
CFLAGS=-std=c11

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)
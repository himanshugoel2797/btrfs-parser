TARGET=btrfs_parser

OBJS=main.o btrfs.o crc32c.o superblock.o root.o checksum_tree.o chunk_tree.o

CC=clang
CFLAGS=-std=c11

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)
TARGET=btrfs_parser

OBJS=main.o btrfs/btrfs.o btrfs/crc32c.o btrfs/extent.o btrfs/log.o btrfs/fs_tree.o btrfs/superblock.o btrfs/root.o btrfs/checksum_tree.o btrfs/chunk_tree.o

CFLAGS:=-std=c11 -Wall -g

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)
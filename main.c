#include <stdio.h>
#include <stdlib.h>

#include "btrfs/btrfs.h"

static FILE *fd = NULL;

uint64_t disk_read(void* buf, uint64_t devID, uint64_t off, uint64_t len) {
	fseek(fd, off, SEEK_SET);
	return fread(buf, 1, len, fd);
}

uint64_t disk_write(void* buf, uint64_t devID, uint64_t off, uint64_t len) {
	return -1;
}


int main(int argc, char *argv[]){

	uint8_t *buf = malloc(0x1000);

	fd = fopen(argv[1], "rb");
	fseek(fd, 0x1000, SEEK_SET);
	fread(buf, 1, 0x1000, fd);

	BTRFS_InitializeStructures(32 * 1024);
	BTRFS_SetDiskReadHandler(disk_read);
	BTRFS_SetDiskWriteHandler(disk_write);

	BTRFS_StartParser(buf);

	free(buf);

	//Build an actual mapping table to translate logical addresses
	//Use it to walk the chunk tree
	//Use the chunk tree to be able to translate any logical address
	//Dev tree only needed when shrinking/removing devices

	//Parse root tree to get the fs tree and checksum tree
	//The fs tree is traversed for file reads

	//The checksum tree is used to verify the filesystem
}
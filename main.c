#include <stdio.h>
#include <stdlib.h>

#include "btrfs.h"

int main(int argc, char *argv[]){

	FILE * fd = fopen(argv[1], "rb");

	uint8_t *buf = malloc(1024 * 1024);
	fread(buf, 1, 1024 * 1024, fd);

	BTRFS_Superblock *sblock = NULL;
	BTRFS_ParseSuperblock(buf + BTRFS_SuperblockOffset0, &sblock);

	if(sblock == NULL) {
		printf("Bad superblock\n");
		return 0;
	}

	printf("Label:%s\n", sblock->label);

	uint64_t table_bytes = sblock->key_chunkItem_table_len;
	BTRFS_Key_ChunkItem_Pair *mapping = (BTRFS_Key_ChunkItem_Pair*)sblock->key_chunkItem_table;

	while(table_bytes > 0) {

		printf("Key Type:%hhx Chunk Size: %lx\n", mapping->key.type, mapping->value.chunk_size_bytes);

		int stripe_cnt = mapping->value.stripe_count;
		uint64_t logical_addr = mapping->key.offset;

		for(int i = 0; i < stripe_cnt; i++)
		{
			printf("Stripe #%d | DevID: %lx | Physical Address: %lx | Logical Address: %lx\n", i, mapping->value.stripes[i].object_id, mapping->value.stripes[i].offset, logical_addr);
			logical_addr += mapping->value.stripe_size;
		}

		int sz = sizeof(BTRFS_Key_ChunkItem_Pair) + stripe_cnt * sizeof(BTRFS_Stripe);
		table_bytes -= sz;
		mapping = (BTRFS_Key_ChunkItem_Pair*)((uint8_t*)mapping + sz);
	}

	printf("Chunk Tree Address: %lx\n", sblock->chunk_tree_root_addr);

	fseek(fd, sblock->chunk_tree_root_addr, SEEK_SET);
	fread(buf, 1, 1024 * 1024, fd);
	
	BTRFS_Header *hdr = buf;

	printf("Logical Address: %lx Item Count: %x\n", hdr->logical_address, hdr->item_count);

	for(int i = 0; i < hdr->item_count; i++) {
		printf("#%x Data Offset: %x Data Size: %x\n", i, hdr->key_ptrs[i].data_offset, hdr->key_ptrs[i].data_size);
	}

	//Build an actual mapping table to translate logical addresses
	//Use it to walk the chunk tree
	//Use the chunk tree to be able to translate any logical address
	//Dev tree only needed when shrinking/removing devices

	//Parse root tree to get the fs tree and checksum tree
	//The fs tree is traversed for file reads

	//The checksum tree is used to verify the filesystem
}
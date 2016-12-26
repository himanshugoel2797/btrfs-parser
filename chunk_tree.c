#include "btrfs.h"

#include <stdlib.h>

void
BTRFS_FillChunkTreeCache(BTRFS_Header *parent)
{
	uint32_t node_size = BTRFS_GetNodeSize();

	if(parent->level == 0)
	{
		//Fill the chunk cache
		BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer*)(parent + 1);

		for(int i = 0; i < parent->item_count; i++) {

			if(chunk_entry->key.type == KeyType_DeviceItem){

			}else if(chunk_entry->key.type == KeyType_ChunkItem) {

				BTRFS_ChunkItem *chunk_item = (BTRFS_ChunkItem*)((uint8_t*)parent + sizeof(BTRFS_Header) + chunk_entry->data_offset);

				uint64_t logical_addr = chunk_entry->key.offset;
				for(int j = 0; j < chunk_item->stripe_count; j++){
					BTRFS_AddMappingToCache(logical_addr, chunk_item->stripes[j].device_id, chunk_item->stripes[j].offset, chunk_item->chunk_size_bytes);
					logical_addr += chunk_item->chunk_size_bytes;
				}
			}

			chunk_entry++;
		}

	}else
	{
		//Visit all of this node's children
		
		BTRFS_Header *children = malloc(node_size);

		BTRFS_KeyPointer *key_ptr = (BTRFS_KeyPointer*)(parent + 1);

		for(uint64_t i = 0; i < parent->item_count; i++){

			if(BTRFS_GetNode(children, key_ptr->block_number) != 0) {
				printf("Chunk Tree Checksum does not match!\n");
				return;
			}

			BTRFS_FillChunkTreeCache(children);
			key_ptr++;
		}

		free(children);
	}
}

int
BTRFS_ParseChunkTree(void){

	uint32_t node_size = BTRFS_GetNodeSize();

	BTRFS_Header *chunk_tree = malloc(node_size);
	if(BTRFS_GetNode(chunk_tree, BTRFS_GetChunkTreeRootAddress()) != 0)
		return -1;

	BTRFS_FillChunkTreeCache(chunk_tree);
	free(chunk_tree);

	return 0;
}
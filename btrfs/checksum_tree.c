#include "btrfs.h"
#include "crc32c.h"

#include <stdlib.h>

uint64_t
BTRFS_VerifyChecksums(BTRFS_Header *parent)
{
	uint32_t node_size = BTRFS_GetNodeSize();
	uint32_t sector_size = BTRFS_GetSectorSize();

	if(parent->level == 0)
	{
		uint64_t retVal = 0;

		//Fill the chunk cache
		BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer*)(parent + 1);

		void *data_block = malloc(node_size);

		for(int i = 0; i < parent->item_count; i++) {

			if(chunk_entry->key.type == KeyType_ExtentChecksum){

				uint32_t *chunk_item = (uint32_t*)((uint8_t*)parent + sizeof(BTRFS_Header) + chunk_entry->data_offset);
				
				uint64_t sz = 0;
				uint64_t logicalAddr = chunk_entry->key.offset;
				while(sz < chunk_entry->data_size){

					BTRFS_Read(data_block, logicalAddr, sector_size);
					uint32_t crc = crc32c(-1, data_block, sector_size);

					if(crc != *chunk_item){
						retVal++;
					}

					logicalAddr += sector_size;
					chunk_item ++;
					sz += sizeof(uint32_t);
				}

			}

			chunk_entry++;
		}

		free(data_block);
		return retVal;
	}else
	{
		uint64_t retVal = 0;

		//Visit all of this node's children
		BTRFS_Header *children = malloc(node_size);
		BTRFS_KeyPointer *key_ptr = (BTRFS_KeyPointer*)(parent + 1);

		for(uint64_t i = 0; i < parent->item_count; i++){

			if(BTRFS_GetNode(children, key_ptr->block_number) != 0) {
				return 1;
			}

			retVal += BTRFS_VerifyChecksums(children);

			key_ptr++;
		}
		free(children);

		return retVal;
	}
}

uint64_t
BTRFS_Scrub(void)
{
	BTRFS_Header *children = malloc(BTRFS_GetNodeSize());
	if(BTRFS_GetNode(children, BTRFS_GetChecksumTreeLocation()) != 0) {
		return 1;
	}	

	return BTRFS_VerifyChecksums(children);
}
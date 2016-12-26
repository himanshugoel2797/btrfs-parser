#include "btrfs.h"
#include "crc32c.h"

#include <stdlib.h>

int
BTRFS_VerifyChecksums(BTRFS_Header *parent)
{
	uint32_t node_size = BTRFS_GetNodeSize();
	uint32_t sector_size = BTRFS_GetSectorSize();

	if(parent->level == 0)
	{
		int retVal = 0;

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
						printf("Expected Csum: %x\t Address: %lx Calculated Csum: %x\n", *chunk_item, logicalAddr, crc);
						retVal = -1;
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
		int retVal = 0;

		//Visit all of this node's children
		BTRFS_Header *children = malloc(node_size);
		BTRFS_KeyPointer *key_ptr = (BTRFS_KeyPointer*)(parent + 1);

		for(uint64_t i = 0; i < parent->item_count; i++){

			if(BTRFS_GetNode(children, key_ptr->block_number) != 0) {
				printf("Checksum Tree Checksum does not match!\n");
				return -1;
			}

			int val = BTRFS_VerifyChecksums(children);
			
			if(retVal == 0)
				retVal = val;

			key_ptr++;
		}
		free(children);

		return retVal;
	}
}

void
BTRFS_Scrub(void)
{
	BTRFS_Header *children = malloc(BTRFS_GetNodeSize());
	if(BTRFS_GetNode(children, BTRFS_GetChecksumTreeLocation()) != 0) {
		printf("Checksum Tree Checksum does not match!\n");
		return;
	}	

	if(BTRFS_VerifyChecksums(children) == 0)
		printf("Verification complete, no errors found.\n");
	else
		printf("Errors found.\n");
}
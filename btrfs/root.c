#include "btrfs.h"
#include <stdlib.h>

static uint64_t extent_tree_loc;
static uint64_t dev_tree_loc;
static uint64_t fs_tree_loc;
static uint64_t checksum_tree_loc;

uint64_t
BTRFS_GetExtentTreeLocation(void){
	return extent_tree_loc;
}

uint64_t
BTRFS_GetDevTreeLocation(void){
	return dev_tree_loc;
}

uint64_t
BTRFS_GetFSTreeLocation(void){
	return fs_tree_loc;
}

uint64_t
BTRFS_GetChecksumTreeLocation(void){
	return checksum_tree_loc;
}

int
BTRFS_ParseRootTree(void)
{
	BTRFS_Header *children = malloc(BTRFS_GetNodeSize());
	if(BTRFS_GetNode(children, BTRFS_GetRootTreeBlockAddress()) != 0) {
		return -1;
	}
	
	BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer*)(children + 1);

	for(int i = 0; i < children->item_count; i++) {
	
		if(chunk_entry->key.type == KeyType_RootItem) {
			BTRFS_RootItem *root_item = (BTRFS_RootItem*)((uint8_t*)children + sizeof(BTRFS_Header) + chunk_entry->data_offset);

			//Root items refer to tree types.
			switch(chunk_entry->key.object_id){
				case ReservedObjectID_ExtentTree:
					extent_tree_loc = root_item->root_block_num;
				break;
				case ReservedObjectID_DevTree:
					dev_tree_loc = root_item->root_block_num;
				break;
				case ReservedObjectID_FSTree:
					fs_tree_loc = root_item->root_block_num;
				break;
				case ReservedObjectID_ChecksumTree:
					checksum_tree_loc = root_item->root_block_num;
				break;
			}
		}
		chunk_entry++;
	}

	free(children);
	return 0;
}
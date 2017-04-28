#include "btrfs.h"

static uint64_t current_inode = 0;

int
BTRFS_TraverseFullFSTree(BTRFS_Header *parent, uint64_t inode_index, char *file_path, uint64_t *desired_inode)
{
	uint32_t node_size = BTRFS_GetNodeSize();

	if(parent->level == 0)
	{
		//Calculate the hash of the next piece of the path
		char *path_end = strchr(file_path, '/');
		if(path_end == NULL)path_end = strchr(file_path, '\0');
		uint32_t name_hash = ~crc32c_sw(~1, file_path, path_end - file_path);

		//Fill the chunk cache
		BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer*)(parent + 1);

		for(int i = 0; i < parent->item_count; i++) {

			switch(chunk_entry->key.type) {
				case KeyType_InodeItem:
				{
					//All the entries under the current inode have been checked and no match was found
					if(current_inode == inode_index)
						return -1;


					current_inode = chunk_entry->key.object_id;
					inode_node_translation_table_key[current_inode % INODE_NODE_TRANSLATION_CACHE_SIZE] = current_inode;
					inode_node_translation_table[current_inode % INODE_NODE_TRANSLATION_CACHE_SIZE] = parent->logical_address;
				}
				break;
				case KeyType_DirItem:
				{
					if(current_inode != inode_index)
						break;

					BTRFS_DirectoryItem *dir_item = (BTRFS_DirectoryItem*)((uint8_t*)parent + sizeof(BTRFS_Header) + chunk_entry->data_offset);
					if(chunk_entry->key.offset == name_hash) {	
						*desired_inode = dir_item->key.object_id;
						return 1;
					}
				}
				break;
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
				return -1;
			}

			int retVal = BTRFS_TraverseFullFSTree(children, inode_index, file_path, desired_inode);

			if(retVal != 0)
				return retVal;

			key_ptr++;
		}

		free(children);
	}

	return 0;
}

int
BTRFS_ParseFullFSTree(char *path, uint64_t *resolved_inode)
{
	//Parse the tree from the root
	uint32_t node_size = BTRFS_GetNodeSize();

	uint64_t path_len = strlen(path);
	uint64_t inode = 256;

	if(path[0] == '/')
		path++;

	BTRFS_Header *children = malloc(node_size);
	for(uint64_t i = 0; i < path_len; i++){

		if(inode_node_translation_table_key[inode % INODE_NODE_TRANSLATION_CACHE_SIZE] == inode){
			if(BTRFS_GetNode(children, inode_node_translation_table[inode % INODE_NODE_TRANSLATION_CACHE_SIZE]) != 0) {
				return -1;
			}
		}else{
			if(BTRFS_GetNode(children, BTRFS_GetFSTreeLocation()) != 0) {
				return -1;
			}
		}

		if(BTRFS_TraverseFullFSTree(children, inode, &path[i], &inode) != 1)
		{
			return -2;
		}

		i = strchr(&path[i], '/') - path;
	}
	free(children);
	
	//Now we have found the inode of the target, this can be used to retrieve any desired information
	*resolved_inode = inode;
	return 0;
}


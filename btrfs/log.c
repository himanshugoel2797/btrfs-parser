/**
 * Copyright (c) 2017 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
#include <stdlib.h>
#include "btrfs.h"

int BTRFS_TraverseLogTree(BTRFS_Header *parent) {
  uint32_t node_size = BTRFS_GetNodeSize();

  if (parent->level == 0) {
    BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer *)(parent + 1);
    for (int i = 0; i < parent->item_count; i++) {
      // Fill the chunk cache

      printf("(%llx, %x, %llx)\n", chunk_entry->key.object_id,
             (uint32_t)chunk_entry->key.type, chunk_entry->key.offset);

      chunk_entry++;
    }
  } else {
    // Visit all of this node's children

    BTRFS_Header *children = malloc(node_size);
    BTRFS_KeyPointer *key_ptr = (BTRFS_KeyPointer *)(parent + 1);

    for (uint64_t i = 0; i < parent->item_count; i++) {
      if (BTRFS_GetNode(children, key_ptr->block_number) != 0) {
        free(children);
        return -1;
      }

      int retVal = BTRFS_TraverseLogTree(children);

      if (retVal != 0) {
        free(children);
        return retVal;
      }

      key_ptr++;
    }

    free(children);
  }
  return 0;
}
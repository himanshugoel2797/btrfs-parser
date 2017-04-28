/**
 * Copyright (c) 2017 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "btrfs.h"

static uint64_t current_inode = 0;

int BTRFS_GetFSTreeExtent(BTRFS_Header *parent, uint64_t inode, uint64_t offset,
                          BTRFS_ExtentDataInline *node, uint64_t *node_off) {
  uint32_t node_size = BTRFS_GetNodeSize();

  if (parent->level == 0) {
    BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer *)(parent + 1);
    for (int i = 0; i < parent->item_count; i++) {
      // Fill the chunk cache

      if (chunk_entry->key.type == KeyType_ExtentData &&
          chunk_entry->key.object_id == inode) {
        // Verify that this node contains the desired offset

        BTRFS_ExtentDataInline *extent =
            (BTRFS_ExtentDataInline *)((uint8_t *)parent +
                                       sizeof(BTRFS_Header) +
                                       chunk_entry->data_offset);

        if (chunk_entry->key.offset <= offset &&
            extent->decoded_size >= (offset - chunk_entry->key.offset)) {
          memcpy(node, extent, chunk_entry->data_size);
          *node_off = chunk_entry->key.offset;
          return 1;  // Fit found
        }
      }

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

      int retVal =
          BTRFS_GetFSTreeExtent(children, inode, offset, node, node_off);

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

int BTRFS_TraverseFullFSTree(BTRFS_Header *parent, uint64_t inode_index,
                             char *file_path, uint64_t *desired_inode) {
  uint32_t node_size = BTRFS_GetNodeSize();

  if (parent->level == 0) {
    // Calculate the hash of the next piece of the path
    char *path_end = strchr(file_path, '/');
    if (path_end == NULL) path_end = strchr(file_path, '\0');
    uint32_t name_hash = ~crc32c(~1, file_path, path_end - file_path);

    // Fill the chunk cache
    BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer *)(parent + 1);

    for (int i = 0; i < parent->item_count; i++) {
      switch (chunk_entry->key.type) {
        case KeyType_InodeItem: {
          // All the entries under the current inode have been checked and no
          // match was found
          if (current_inode == inode_index) return -1;

          current_inode = chunk_entry->key.object_id;
          BTRFS_AddInodeToCache(current_inode, parent->logical_address);

        } break;
        case KeyType_DirItem: {
          if (current_inode != inode_index) break;

          BTRFS_DirectoryItem *dir_item =
              (BTRFS_DirectoryItem *)((uint8_t *)parent + sizeof(BTRFS_Header) +
                                      chunk_entry->data_offset);
          if (chunk_entry->key.offset == name_hash) {
            *desired_inode = dir_item->key.object_id;
            return 1;
          }
        } break;
      }

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

      int retVal = BTRFS_TraverseFullFSTree(children, inode_index, file_path,
                                            desired_inode);

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

uint64_t BTRFS_ReadFile(uint64_t inode, uint64_t offset, uint64_t len,
                        void *dest_buf) {
  uint32_t node_size = BTRFS_GetNodeSize();

  BTRFS_Header *children = malloc(node_size);
  if (BTRFS_GetNode(children, BTRFS_GetFSTreeLocation()) != 0) {
    free(children);
    return -1;
  }

  BTRFS_ExtentDataInline *extent = malloc(node_size);
  uint64_t extent_off = 0;
  uint64_t size_rem = len;
  uint64_t size_read = 0;
  uint64_t buf_off = 0;
  uint8_t *dst = (uint8_t *)dest_buf;
  bool exit_read = false;

  while (!exit_read) {
    if (BTRFS_GetFSTreeExtent(children, inode, offset, extent, &extent_off) !=
        1) {
      free(extent);
      free(children);
      return size_read;
    }

    // Parse the extent to get the next part of the requested file.
    if (extent->decoded_size >= size_rem) exit_read = true;

    uint64_t off_in_ext = (offset - extent_off);
    uint64_t rd_size =
        (extent->decoded_size > size_rem ? extent->decoded_size : size_rem);

    if (extent->type & ExtentDataType_Inline) {
      memcpy(dst + buf_off, (uint8_t *)(extent + 1) + off_in_ext, rd_size);
    } else if (extent->type & ExtentDataType_Regular) {
      BTRFS_ExtentDataFull *extent_full = (BTRFS_ExtentDataFull *)extent;
      BTRFS_Read(dst + buf_off, extent_full->extent_logical_addr + off_in_ext,
                 rd_size);
    }

    offset += rd_size;
    size_rem -= rd_size;
    size_read += rd_size;
    buf_off += rd_size;
  }

  return size_read;
}

int BTRFS_ParseFullFSTree(char *path, uint64_t *resolved_inode) {
  // Parse the tree from the root
  uint32_t node_size = BTRFS_GetNodeSize();

  uint64_t path_len = strlen(path);
  uint64_t inode = 256;

  if (path[0] == '/') {
    path++;
    path_len--;
  }

  BTRFS_Header *children = malloc(node_size);
  for (uint64_t i = 0; i < path_len; i++) {
    uint64_t read_inode = inode;
    uint64_t read_inode_addr = 0;
    BTRFS_GetInodeFromCache(&read_inode, &read_inode_addr);

    if (read_inode == inode) {
      if (BTRFS_GetNode(children, read_inode_addr) != 0) {
        return -1;
      }
    } else {
      if (BTRFS_GetNode(children, BTRFS_GetFSTreeLocation()) != 0) {
        return -1;
      }
    }

    if (BTRFS_TraverseFullFSTree(children, inode, &path[i], &inode) != 1) {
      return -2;
    }

    i = strchr(&path[i], '/') - path;
  }
  free(children);

  // Now we have found the inode of the target, this can be used to retrieve any
  // desired information
  *resolved_inode = inode;
  return 0;
}

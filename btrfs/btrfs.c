/**
 * Copyright (c) 2017 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "btrfs.h"
#include "crc32c.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t ***chunk_tree_root[512];

static uint64_t (*write_handler)(void *buf, uint64_t devID, uint64_t off,
                                 uint64_t len);
static uint64_t (*read_handler)(void *buf, uint64_t devID, uint64_t off,
                                uint64_t len);

#define L4_LEVEL_SIZE (512 * 1024 * 1024 * 1024ull)
#define L3_LEVEL_SIZE (1 * 1024 * 1024 * 1024ull)
#define L2_LEVEL_SIZE (2 * 1024 * 1024ull)
#define L1_LEVEL_SIZE (4 * 1024ull)

#define INODE_NODE_TRANSLATION_CACHE_SIZE (64 * 1024)
uint64_t inode_node_translation_table[INODE_NODE_TRANSLATION_CACHE_SIZE];
uint64_t inode_node_translation_table_key[INODE_NODE_TRANSLATION_CACHE_SIZE];

void BTRFS_InitializeStructures(int cache_size) {
  crc32c_init();
  memset(chunk_tree_root, 0, 512 * sizeof(uint64_t));
  memset(inode_node_translation_table, 0,
         INODE_NODE_TRANSLATION_CACHE_SIZE * sizeof(uint64_t));
  memset(inode_node_translation_table_key, 0,
         INODE_NODE_TRANSLATION_CACHE_SIZE * sizeof(uint64_t));
}

void BTRFS_AddInodeToCache(uint64_t inode, uint64_t addr) {
  inode_node_translation_table_key[inode % INODE_NODE_TRANSLATION_CACHE_SIZE] =
      inode;

  inode_node_translation_table_key[inode % INODE_NODE_TRANSLATION_CACHE_SIZE] =
      addr;
}

void BTRFS_GetInodeFromCache(uint64_t *inode, uint64_t *addr) {
  *inode = inode_node_translation_table_key[*inode %
                                            INODE_NODE_TRANSLATION_CACHE_SIZE];

  *addr =
      inode_node_translation_table[*inode % INODE_NODE_TRANSLATION_CACHE_SIZE];
}

void BTRFS_AddMappingToCache(uint64_t vAddr, uint64_t deviceID, uint64_t pAddr,
                             uint64_t len) {
  if (vAddr % 4096) return;

  if (pAddr % 4096) return;

  if (len % 4096) return;

  if (len != L4_LEVEL_SIZE && len != L3_LEVEL_SIZE && len != L2_LEVEL_SIZE &&
      len != L1_LEVEL_SIZE) {
    while (len > 0) {
      if (len >= L4_LEVEL_SIZE && vAddr % L4_LEVEL_SIZE == 0) {
        BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L4_LEVEL_SIZE);

        len -= L4_LEVEL_SIZE;
        vAddr += L4_LEVEL_SIZE;
        pAddr += L4_LEVEL_SIZE;
      } else if (len >= L3_LEVEL_SIZE && vAddr % L3_LEVEL_SIZE == 0) {
        BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L3_LEVEL_SIZE);

        len -= L3_LEVEL_SIZE;
        vAddr += L3_LEVEL_SIZE;
        pAddr += L3_LEVEL_SIZE;
      } else if (len >= L2_LEVEL_SIZE && vAddr % L2_LEVEL_SIZE == 0) {
        BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L2_LEVEL_SIZE);

        len -= L2_LEVEL_SIZE;
        vAddr += L2_LEVEL_SIZE;
        pAddr += L2_LEVEL_SIZE;
      } else if (len >= L1_LEVEL_SIZE && vAddr % L1_LEVEL_SIZE == 0) {
        BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L1_LEVEL_SIZE);

        len -= L1_LEVEL_SIZE;
        vAddr += L1_LEVEL_SIZE;
        pAddr += L1_LEVEL_SIZE;
      }
    }

    return;
  }
  printf("Vaddr: %llx \tPaddr: %llx \tLen: %llx \n", vAddr, pAddr, len);

  uint32_t l4_i = (vAddr >> 39) & 0x1FF;
  uint32_t l3_i = (vAddr >> 30) & 0x1FF;
  uint32_t l2_i = (vAddr >> 21) & 0x1FF;
  uint32_t l1_i = (vAddr >> 12) & 0x1FF;

  if ((uint64_t)chunk_tree_root[l4_i] & 1) {
    chunk_tree_root[l4_i] = 0;
  }

  if (chunk_tree_root[l4_i] == 0) {
    if (len != L4_LEVEL_SIZE) {
      chunk_tree_root[l4_i] = malloc(sizeof(uint64_t) * 512);
      memset(chunk_tree_root[l4_i], 0, 512 * sizeof(uint64_t));
    } else {
      chunk_tree_root[l4_i] = (uint64_t ***)(pAddr | 1);
      return;
    }
  }

  if ((uint64_t)chunk_tree_root[l4_i][l3_i] & 1) {
    chunk_tree_root[l4_i][l3_i] = 0;
  }

  if (chunk_tree_root[l4_i][l3_i] == 0) {
    if (len != L3_LEVEL_SIZE) {
      chunk_tree_root[l4_i][l3_i] = malloc(sizeof(uint64_t) * 512);
      memset(chunk_tree_root[l4_i][l3_i], 0, 512 * sizeof(uint64_t));
    } else {
      chunk_tree_root[l4_i][l3_i] = (uint64_t **)(pAddr | 1);
      return;
    }
  }

  if ((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] & 1) {
    chunk_tree_root[l4_i][l3_i][l2_i] = 0;
  }

  if (chunk_tree_root[l4_i][l3_i][l2_i] == 0) {
    if (len != L2_LEVEL_SIZE) {
      chunk_tree_root[l4_i][l3_i][l2_i] = malloc(sizeof(uint64_t) * 512);
      memset(chunk_tree_root[l4_i][l3_i][l2_i], 0, 512 * sizeof(uint64_t));
    } else {
      chunk_tree_root[l4_i][l3_i][l2_i] = (uint64_t *)(pAddr | 1);
      return;
    }
  }

  chunk_tree_root[l4_i][l3_i][l2_i][l1_i] = pAddr | 1;
}

void BTRFS_SetDiskReadHandler(uint64_t (*handler)(void *buf, uint64_t devID,
                                                  uint64_t off, uint64_t len)) {
  read_handler = handler;
}

void BTRFS_SetDiskWriteHandler(uint64_t (*handler)(void *buf, uint64_t devID,
                                                   uint64_t off,
                                                   uint64_t len)) {
  write_handler = handler;
}

uint64_t BTRFS_Read(void *buf, uint64_t logicalAddr, uint64_t len) {
  BTRFS_PhysicalAddress p_addr;
  int err = 0;
  if ((err = BTRFS_TranslateLogicalAddress(logicalAddr, &p_addr)) != 0) {
    return err;
  }

  return read_handler(buf, p_addr.device_id, p_addr.physical_addr, len);
}

uint64_t BTRFS_ReadRaw(void *buf, uint64_t devId, uint64_t addr, uint64_t len) {
  return read_handler(buf, devId, addr, len);
}

uint64_t BTRFS_Write(void *buf, uint64_t logicalAddr, uint64_t len) {
  BTRFS_PhysicalAddress p_addr;
  int err = 0;
  if ((err = BTRFS_TranslateLogicalAddress(logicalAddr, &p_addr)) != 0) {
    return err;
  }

  return write_handler(buf, p_addr.device_id, p_addr.physical_addr, len);
}

int BTRFS_GetNode(void *buf, uint64_t logicalAddr) {
  BTRFS_Header *chunk_tree = buf;
  int err = 0;
  if((err = BTRFS_Read(chunk_tree, logicalAddr, BTRFS_GetNodeSize())) < 0)
    return err;

  uint32_t crc = crc32c(-1, chunk_tree->uuid, BTRFS_GetNodeSize() - 0x20);
  uint32_t expected_csum = *(uint32_t *)(chunk_tree->csum);

  if (crc != expected_csum) return -2;

  return 0;
}

void *BTRFS_GetNodePointer(BTRFS_Header *parent, BTRFS_KeyType type,
                           int base_index, int index) {
  if (parent->level != 0) return NULL;

  BTRFS_ItemPointer *chunk_entry = (BTRFS_ItemPointer *)(parent + 1);

  int match_cnt = 0;
  for (int i = base_index; i < parent->item_count; i++) {
    if (chunk_entry->key.type == type && match_cnt == index)
      return ((uint8_t *)parent + sizeof(BTRFS_Header) +
              chunk_entry->data_offset);

    if (chunk_entry->key.type == type) match_cnt++;

    chunk_entry++;
  }

  return NULL;
}

int BTRFS_TranslateLogicalAddress(uint64_t logicalAddress,
                                  BTRFS_PhysicalAddress *physicalAddress) {
  // Walk the chunk tree to translate the provided address

  uint32_t l4_i = (logicalAddress >> 39) & 0x1FF;
  uint32_t l3_i = (logicalAddress >> 30) & 0x1FF;
  uint32_t l2_i = (logicalAddress >> 21) & 0x1FF;
  uint32_t l1_i = (logicalAddress >> 12) & 0x1FF;

  if ((uint64_t)chunk_tree_root[l4_i] == 0) return -1;

  if ((uint64_t)chunk_tree_root[l4_i] & 1) {
    physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i] & ~1;
    physicalAddress->physical_addr += (logicalAddress % L4_LEVEL_SIZE);
    return 0;
  }

  if ((uint64_t)chunk_tree_root[l4_i][l3_i] == 0) return -2;

  if ((uint64_t)chunk_tree_root[l4_i][l3_i] & 1) {
    physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i][l3_i] & ~1;
    physicalAddress->physical_addr += (logicalAddress % L3_LEVEL_SIZE);
    return 0;
  }

  if ((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] == 0) return -3;

  if ((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] & 1) {
    physicalAddress->physical_addr =
        (uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] & ~1;
    physicalAddress->physical_addr += (logicalAddress % L2_LEVEL_SIZE);
    return 0;
  }

  if ((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] == 0) return -4;

  if ((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] & 1) {
    physicalAddress->physical_addr =
        (uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] & ~1;
    physicalAddress->physical_addr += (logicalAddress % L1_LEVEL_SIZE);
    return 0;
  }

  return -5;
}

int BTRFS_StartParser(void) {
  BTRFS_Superblock *sblock = malloc(0x1000);
  if (sblock == NULL) return -1;

  // TODO: Find the highest generation superblock.

  if (BTRFS_ParseSuperblock(sblock) != 0) return -2;

  int err = 0;
  if((err = BTRFS_ParseChunkTree()) != 0) return err;
  if(BTRFS_ParseRootTree() != 0) return -4;
  return 0;
}
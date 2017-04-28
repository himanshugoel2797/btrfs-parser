#include "btrfs.h"
#include "crc32c.h"

#include <stdbool.h>
#include <string.h>

static BTRFS_Superblock superblock;

uint32_t BTRFS_GetSectorSize(void) { return superblock.sector_size; }

uint32_t BTRFS_GetNodeSize(void) { return superblock.node_size; }

uint32_t BTRFS_GetLeafSize(void) { return superblock.leaf_size; }

uint64_t BTRFS_GetRootTreeBlockAddress(void) {
  return superblock.root_tree_root_addr;
}

uint64_t BTRFS_GetChunkTreeRootAddress(void) {
  return superblock.chunk_tree_root_addr;
}

void BTRFS_GetLabel(char *buffer) { strcpy(buffer, superblock.label); }

int BTRFS_ParseSuperblock(BTRFS_Superblock *block) {

  crc32c_init();

  BTRFS_Superblock *sblock = block;
  int highest_gen_idx = -1;
  uint64_t highest_gen = 0;

  for (int i = 0; BTRFS_superblock_offsets[i] != 0; i++) {

    if (BTRFS_ReadRaw(sblock, 0, BTRFS_superblock_offsets[i], 0x1000) != 0x1000)
      continue;

    // Start by verifying the checksum
    uint32_t crc = 0;
    crc = crc32c(-1, sblock->uuid, 0x1000 - 0x20);

    uint32_t expected_csum = *(uint32_t *)(&sblock->csum);

    char btrfs_magic[] = BTRFS_MagicString;
    bool magic_check_failed = false;

    for (int i = 0; i < BTRFS_MagicStringLen; i++) {
      if (btrfs_magic[i] != sblock->magic[i]) {
        magic_check_failed = true;
        break;
      }
    }

    if (magic_check_failed)
      continue;

    if (expected_csum != crc)
      continue;

    // Determine if this block has the highest generation
    if (sblock->generation >= highest_gen) {
      highest_gen = sblock->generation;
      highest_gen_idx = i;
    }
  }

  if (highest_gen_idx == -1)
    return -1; // Failed to find a valid superblock.

  // Read in the highest generation block
  BTRFS_ReadRaw(sblock, 0, BTRFS_superblock_offsets[highest_gen_idx], 0x1000);

  // Copy the superblock into a backup table
  memcpy(&superblock, sblock, sizeof(BTRFS_Superblock));

  // Fill the btrfs translation cache
  uint64_t table_bytes = sblock->key_chunkItem_table_len;
  BTRFS_Key_ChunkItem_Pair *mapping =
      (BTRFS_Key_ChunkItem_Pair *)sblock->key_chunkItem_table;

  while (table_bytes > 0) {

    int stripe_cnt = mapping->value.stripe_count;
    uint64_t logical_addr = mapping->key.offset;

    for (int i = 0; i < stripe_cnt; i++) {
      BTRFS_AddMappingToCache(logical_addr, mapping->value.stripes[i].device_id,
                              mapping->value.stripes[i].offset,
                              mapping->value.chunk_size_bytes);
      logical_addr += mapping->value.chunk_size_bytes;
    }

    int sz =
        sizeof(BTRFS_Key_ChunkItem_Pair) + stripe_cnt * sizeof(BTRFS_Stripe);
    table_bytes -= sz;
    mapping = (BTRFS_Key_ChunkItem_Pair *)((uint8_t *)mapping + sz);
  }

  return 0;
}

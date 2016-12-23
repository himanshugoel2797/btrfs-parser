#include "btrfs.h"

#include <stdlib.h>
#include <stdio.h>

/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78

/* Table for a quadword-at-a-time software crc. */
static uint32_t crc32c_table[8][256];

/* Construct table for software CRC-32C calculation. */
static void crc32c_init_sw(void)
{
    uint32_t n, crc, k;

    for (n = 0; n < 256; n++) {
        crc = n;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        crc32c_table[0][n] = crc;
    }
    for (n = 0; n < 256; n++) {
        crc = crc32c_table[0][n];
        for (k = 1; k < 8; k++) {
            crc = crc32c_table[0][crc & 0xff] ^ (crc >> 8);
            crc32c_table[k][n] = crc;
        }
    }
}

/* Table-driven software version as a fall-back.  This is about 15 times slower
   than using the hardware instructions.  This assumes little-endian integers,
   as is the case on Intel processors that the assembler code here is for. */
static uint32_t crc32c_sw(uint32_t crci, const void *buf, size_t len)
{
    const unsigned char *next = buf;
    uint64_t crc;

    crc = crci ^ 0xffffffff;
    while (len && ((uintptr_t)next & 7) != 0) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    while (len >= 8) {
        crc ^= *(uint64_t *)next;
        crc = crc32c_table[7][crc & 0xff] ^
              crc32c_table[6][(crc >> 8) & 0xff] ^
              crc32c_table[5][(crc >> 16) & 0xff] ^
              crc32c_table[4][(crc >> 24) & 0xff] ^
              crc32c_table[3][(crc >> 32) & 0xff] ^
              crc32c_table[2][(crc >> 40) & 0xff] ^
              crc32c_table[1][(crc >> 48) & 0xff] ^
              crc32c_table[0][crc >> 56];
        next += 8;
        len -= 8;
    }
    while (len) {
        crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        len--;
    }
    return (uint32_t)crc ^ 0xffffffff;
}

typedef struct {
	uint64_t VirtualAddress;
	uint64_t PhysicalAddress;
	uint64_t Length;
	int64_t LastUsed;
} BTRFS_Mapping;

#define TRANSLATION_CACHE_SIZE (32 * 1024)
#define MAX_AGE 16
static int64_t CurrentAge = 0;
static int CacheSearchStartIndex = 0;
static BTRFS_Mapping *mapping_cache = NULL;

static BTRFS_Superblock superblock;

static uint64_t (*write_handler)(void* buf, uint64_t off, uint64_t len);
static uint64_t (*read_handler)(void* buf, uint64_t off, uint64_t len);

void
BTRFS_InitializeStructures(void) {
	crc32c_init_sw();

	mapping_cache = malloc(sizeof(BTRFS_Mapping) * TRANSLATION_CACHE_SIZE);
	memset(mapping_cache, 0, sizeof(BTRFS_Mapping) * TRANSLATION_CACHE_SIZE);
}

void
BTRFS_AddMappingToCache(uint64_t vAddr, uint64_t pAddr, uint64_t len) {

	for(uint64_t c = 0; c < TRANSLATION_CACHE_SIZE; c++) {
		uint64_t i = (CacheSearchStartIndex + c) % TRANSLATION_CACHE_SIZE;

		if(mapping_cache[i].LastUsed < CurrentAge - MAX_AGE) {
			mapping_cache[i].VirtualAddress = vAddr;
			mapping_cache[i].PhysicalAddress = pAddr;
			mapping_cache[i].Length = len;
			mapping_cache[i].LastUsed = CurrentAge;
			CacheSearchStartIndex++;
			break;
		}
	}
}

void
BTRFS_SetDiskReadHandler(uint64_t (*handler)(void* buf, uint64_t off, uint64_t len)) {
	read_handler = handler;
}

void
BTRFS_SetDiskWriteHandler(uint64_t (*handler)(void* buf, uint64_t off, uint64_t len)) {
	write_handler = handler;
}

void
BTRFS_ParseSuperblock(void *buf, BTRFS_Superblock **block) {

	crc32c_init_sw();

	BTRFS_Superblock sblock = (BTRFS_Superblock*)buf;

	//Start by verifying the checksum
	uint32_t crc = 0;
	crc = crc32c_sw(0, sblock->uuid, 0x1000 - 0x20);

	uint32_t expected_csum = *(uint32_t*)(&sblock->csum);

	char btrfs_magic[] = BTRFS_MagicString;

	for(int i = 0; i < BTRFS_MagicStringLen; i++) {
		if(btrfs_magic[i] != sblock->magic[i]) {
			*block = NULL;
			return;
		}
	}

	if(expected_csum != crc){
		*block = NULL;
		return;
	}

	*block = sblock;

	//Copy the superblock into a backup table
	memcpy(&superblock, sblock, sizeof(BTRFS_Superblock));

	//Fill the btrfs translation cache
	uint64_t table_bytes = sblock->key_chunkItem_table_len;
	BTRFS_Key_ChunkItem_Pair *mapping = (BTRFS_Key_ChunkItem_Pair*)sblock->key_chunkItem_table;

	while(table_bytes > 0) {

		int stripe_cnt = mapping->value.stripe_count;
		uint64_t logical_addr = mapping->key.offset;

		for(int i = 0; i < stripe_cnt; i++)
		{
			BTRFS_AddMappingToCache(logical_addr, mapping->value.stripes[i].offset, mapping->value.stripe_size);
			logical_addr += mapping->value.stripe_size;
		}

		int sz = sizeof(BTRFS_Key_ChunkItem_Pair) + stripe_cnt * sizeof(BTRFS_Stripe);
		table_bytes -= sz;
		mapping = (BTRFS_Key_ChunkItem_Pair*)((uint8_t*)mapping + sz);
	}

	//Use the current allocation cache to parse and save as much of the chunk tree as possible.
}

int
BTRFS_TranslateLogicalAddress(uint64_t logicalAddress, 
							  BTRFS_PhysicalAddress *physicalAddress) {

	//Walk the chunk tree to translate the provided address

}
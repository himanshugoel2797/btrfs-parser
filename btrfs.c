#include "btrfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

static uint64_t ***chunk_tree_root[512];

static BTRFS_Superblock superblock;

static uint64_t (*write_handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len);
static uint64_t (*read_handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len);

#define L4_LEVEL_SIZE (512 * 1024 * 1024 * 1024ull)
#define L3_LEVEL_SIZE (1 * 1024 * 1024 * 1024ull)
#define L2_LEVEL_SIZE (2 * 1024 * 1024ull)
#define L1_LEVEL_SIZE (4 * 1024ull)

void
BTRFS_InitializeStructures(int cache_size) {
	crc32c_init_sw();

	memset(chunk_tree_root, 0, 512 * sizeof(uint64_t));
}

void
BTRFS_AddMappingToCache(uint64_t vAddr, uint64_t deviceID, uint64_t pAddr, uint64_t len) {


	if(vAddr % 4096)
		return;

	if(pAddr % 4096)
		return;

	if(len % 4096)
		return;


	if(len != L4_LEVEL_SIZE && len != L3_LEVEL_SIZE && len != L2_LEVEL_SIZE && len != L1_LEVEL_SIZE) {

		while(len > 0) {

			if(len >= L4_LEVEL_SIZE){

				BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L4_LEVEL_SIZE);

				len -= L4_LEVEL_SIZE;
				vAddr += L4_LEVEL_SIZE;
				pAddr += L4_LEVEL_SIZE;
			}else if(len >= L3_LEVEL_SIZE){

				BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L3_LEVEL_SIZE);

				len -= L3_LEVEL_SIZE;
				vAddr += L3_LEVEL_SIZE;
				pAddr += L3_LEVEL_SIZE;
			}else if(len >= L2_LEVEL_SIZE){

				BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L2_LEVEL_SIZE);

				len -= L2_LEVEL_SIZE;
				vAddr += L2_LEVEL_SIZE;
				pAddr += L2_LEVEL_SIZE;
			}else if(len >= L1_LEVEL_SIZE){

				BTRFS_AddMappingToCache(vAddr, deviceID, pAddr, L1_LEVEL_SIZE);

				len -= L1_LEVEL_SIZE;
				vAddr += L1_LEVEL_SIZE;
				pAddr += L1_LEVEL_SIZE;
			}

		}

		return;
	}

	uint32_t l4_i = (vAddr >> 39);
	uint32_t l3_i = (vAddr >> 30) & 0x1FF;
	uint32_t l2_i = (vAddr >> 21) & 0x1FF;
	uint32_t l1_i = (vAddr >> 12) & 0x1FF;

	if(chunk_tree_root[l4_i] == 0){
		if(len != L4_LEVEL_SIZE){
			chunk_tree_root[l4_i] = malloc(sizeof(uint64_t) * 512);
			memset(chunk_tree_root[l4_i], 0, 512 * sizeof(uint64_t));
		}else
		{
			chunk_tree_root[l4_i] = (uint64_t***)(pAddr | 1);
			return;
		}
	}

	if(chunk_tree_root[l4_i][l3_i] == 0){
		if(len != L3_LEVEL_SIZE){
			chunk_tree_root[l4_i][l3_i] = malloc(sizeof(uint64_t) * 512);
			memset(chunk_tree_root[l4_i][l3_i], 0, 512 * sizeof(uint64_t));
		}else
		{
			chunk_tree_root[l4_i][l3_i] = (uint64_t**)(pAddr | 1);
			return;
		}
	}

	if(chunk_tree_root[l4_i][l3_i][l2_i] == 0){
		if(len != L2_LEVEL_SIZE){
			chunk_tree_root[l4_i][l3_i][l2_i] = malloc(sizeof(uint64_t) * 512);
			memset(chunk_tree_root[l4_i][l3_i][l2_i], 0, 512 * sizeof(uint64_t));
		}else
		{
			chunk_tree_root[l4_i][l3_i][l2_i] = (uint64_t*)(pAddr | 1);
			return;
		}
	}

	chunk_tree_root[l4_i][l3_i][l2_i][l1_i] = pAddr | 1;
}

void
BTRFS_SetDiskReadHandler(uint64_t (*handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len)) {
	read_handler = handler;
}

void
BTRFS_SetDiskWriteHandler(uint64_t (*handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len)) {
	write_handler = handler;
}

uint64_t
BTRFS_Read(void *buf, uint64_t logicalAddr, uint64_t len) {
	BTRFS_PhysicalAddress p_addr;
	if(BTRFS_TranslateLogicalAddress(logicalAddr, &p_addr) != 0)
		return -1;

	//Read into an array of 4MiB pages.
	printf("V:%lx:P:%lx\n", logicalAddr, p_addr.physical_addr);

	return read_handler(buf, p_addr.device_id, p_addr.physical_addr, len);
}

uint64_t
BTRFS_Write(void *buf, uint64_t logicalAddr, uint64_t len) {
	BTRFS_PhysicalAddress p_addr;
	if(BTRFS_TranslateLogicalAddress(logicalAddr, &p_addr) != 0)
		return -1;

	//Update the cache if needed, then push the changes

	return write_handler(buf, p_addr.device_id, p_addr.physical_addr, len);	
}

void
BTRFS_ParseSuperblock(void *buf, BTRFS_Superblock **block) {

	crc32c_init_sw();

	BTRFS_Superblock *sblock = (BTRFS_Superblock*)buf;

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
			BTRFS_AddMappingToCache(logical_addr, mapping->value.stripes[i].device_id, mapping->value.stripes[i].offset, mapping->value.stripe_size);
			logical_addr += mapping->value.stripe_size;
		}

		int sz = sizeof(BTRFS_Key_ChunkItem_Pair) + stripe_cnt * sizeof(BTRFS_Stripe);
		table_bytes -= sz;
		mapping = (BTRFS_Key_ChunkItem_Pair*)((uint8_t*)mapping + sz);
	}

	BTRFS_Header chunk_tree;
	BTRFS_Read(&chunk_tree, sblock->chunk_tree_root_addr, sizeof(BTRFS_Header));

	//Fill the chunk cache
	uint64_t header_end_addr = sblock->chunk_tree_root_addr + sizeof(BTRFS_Header);
	uint64_t leaf_node_addr = header_end_addr;
	for(int i = 0; i < chunk_tree.item_count; i++) {

		BTRFS_ItemPointer chunk_entry;
		BTRFS_Read(&chunk_entry, leaf_node_addr, sizeof(BTRFS_ItemPointer));

		if(chunk_entry.key.type == KeyType_DeviceItem){

		}else if(chunk_entry.key.type == KeyType_ChunkItem) {

			BTRFS_ChunkItem *chunk_item = malloc(chunk_entry.data_size);
			BTRFS_Read(chunk_item, header_end_addr + chunk_entry.data_offset, chunk_entry.data_size);

			uint64_t logical_addr = chunk_entry.key.offset;
			for(int j = 0; j < chunk_item->stripe_count; j++){

				BTRFS_AddMappingToCache(logical_addr, chunk_item->stripes[j].device_id, chunk_item->stripes[j].offset, chunk_item->stripe_size);
				logical_addr += chunk_item->stripe_size;

			}

			free(chunk_item);
		}

		leaf_node_addr += sizeof(BTRFS_ItemPointer);
	}

}

int
BTRFS_TranslateLogicalAddress(uint64_t logicalAddress, 
							  BTRFS_PhysicalAddress *physicalAddress) {

	//Walk the chunk tree to translate the provided address

	uint32_t l4_i = (logicalAddress >> 39);
	uint32_t l3_i = (logicalAddress >> 30) & 0x1FF;
	uint32_t l2_i = (logicalAddress >> 21) & 0x1FF;
	uint32_t l1_i = (logicalAddress >> 12) & 0x1FF;

	if((uint64_t)chunk_tree_root[l4_i] == 0)
		return -1;

	if((uint64_t)chunk_tree_root[l4_i] & 1){
		physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i] & ~1;
		physicalAddress->physical_addr += (logicalAddress % L4_LEVEL_SIZE);
		return 0;
	}

	if((uint64_t)chunk_tree_root[l4_i][l3_i] == 0)
		return -1;

	if((uint64_t)chunk_tree_root[l4_i][l3_i] & 1){
		physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i][l3_i] & ~1;
		physicalAddress->physical_addr += (logicalAddress % L3_LEVEL_SIZE);
		return 0;
	}

	if((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] == 0)
		return -1;

	if((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] & 1){
		physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i][l3_i][l2_i] & ~1;
		physicalAddress->physical_addr += (logicalAddress % L2_LEVEL_SIZE);
		return 0;
	}

	if((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] == 0)
		return -1;

	if((uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] & 1){
		physicalAddress->physical_addr = (uint64_t)chunk_tree_root[l4_i][l3_i][l2_i][l1_i] & ~1;
		physicalAddress->physical_addr += (logicalAddress % L1_LEVEL_SIZE);
		return 0;
	}

	return -1;
}
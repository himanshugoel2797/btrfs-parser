#ifndef _BTRFS_PARSER_H_
#define _BTRFS_PARSER_H_

#include <stdint.h>
#include <stddef.h>

#include "btrfs_types.h"


///
/// @brief      Initialize the BTRFS driver
///
void
BTRFS_InitializeStructures(int cache_size);


void
BTRFS_AddMappingToCache(uint64_t vAddr, uint64_t deviceID, uint64_t pAddr, uint64_t len);

///
/// @brief      Set the disk read handler.
///
/// @param[in]  handler  The handler
///
void
BTRFS_SetDiskReadHandler(uint64_t (*handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len));

///
/// @brief      Set the disk write handler.
///
/// @param[in]  handler  The handler
///
void
BTRFS_SetDiskWriteHandler(uint64_t (*handler)(void* buf, uint64_t devID, uint64_t off, uint64_t len));


uint64_t
BTRFS_Read(void *buf, uint64_t logicalAddr, uint64_t len);

uint64_t
BTRFS_Write(void *buf, uint64_t logicalAddr, uint64_t len);

int
BTRFS_GetNode(void *buf, uint64_t logicalAddr);

void*
BTRFS_GetNodePointer(BTRFS_Header *parent, BTRFS_KeyType type, int base_index, int index);

///
/// @brief      Start the BTRFS driver.
///
/// @return     Error code on error, 0 on success.
///
int
BTRFS_StartParser(void *buf);

///
/// @brief      Retrive the superblock from the buffer after verifying it.
///
/// @param      buf    The buffer, from BTRFS_SuperBlockOffset0
/// @param      block  The block
///
void
BTRFS_ParseSuperblock(void *buf, BTRFS_Superblock **block);

///
/// @brief      Translate a logical address to physical address.
///
/// @param[in]  logicalAddress   The logical address
/// @param      physicalAddress  The physical address
///
/// @return     -1 on translation failure, 0 on success.
///
int
BTRFS_TranslateLogicalAddress(uint64_t logicalAddress, 
							  BTRFS_PhysicalAddress *physicalAddress);

///
/// @brief      Get the sector size.
///
/// @return     The sector size in bytes.
///
uint32_t
BTRFS_GetSectorSize(void);

///
/// @brief      Get the node size.
///
/// @return     The node size in bytes.
///
uint32_t
BTRFS_GetNodeSize(void);

///
/// @brief      Get the leaf size.
///
/// @return     The leaf size in bytes.
///
uint32_t
BTRFS_GetLeafSize(void);

///
/// @brief      Get the logical address of the root of the root tree.
///
/// @return     The logical address of the root of the root tree.
///
uint64_t
BTRFS_GetRootTreeBlockAddress(void);

uint64_t
BTRFS_GetChunkTreeRootAddress(void);

uint64_t
BTRFS_GetChecksumTreeLocation(void);

uint64_t
BTRFS_GetFSTreeLocation(void);

uint64_t
BTRFS_GetDevTreeLocation(void);

uint64_t
BTRFS_GetExtentTreeLocation(void);


void
BTRFS_Scrub(void);

void
BTRFS_ParseRootTree(void);

int
BTRFS_ParseChunkTree(void);

#endif
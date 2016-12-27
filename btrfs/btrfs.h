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
BTRFS_AddMappingToCache(uint64_t vAddr, 
						uint64_t deviceID, 
						uint64_t pAddr, 
						uint64_t len);

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

///
/// @brief      Read from the disk logical address.
///
/// @param      buf          The buffer
/// @param[in]  logicalAddr  The logical address
/// @param[in]  len          The length
///
/// @return     Number of bytes read.
///
uint64_t
BTRFS_Read(void *buf, 
		   uint64_t logicalAddr, 
		   uint64_t len);

///
/// @brief      Write to the disk logical address.
///
/// @param      buf          The buffer
/// @param[in]  logicalAddr  The logical address
/// @param[in]  len          The length
///
/// @return     Number of bytes written.
///
uint64_t
BTRFS_Write(void *buf, 
			uint64_t logicalAddr, 
			uint64_t len);

///
/// @brief      Get a node.
///
/// @param      buf          The buffer
/// @param[in]  logicalAddr  The logical address
///
/// @return     Error code on failure, 0 on success.
///
int
BTRFS_GetNode(void *buf, 
			  uint64_t logicalAddr);

///
/// @brief      Get a node pointer.
///
/// @param      parent      The parent
/// @param[in]  type        The type
/// @param[in]  base_index  The base index
/// @param[in]  index       The index
///
/// @return     A pointer to the node.
///
void*
BTRFS_GetNodePointer(BTRFS_Header *parent, 
					 BTRFS_KeyType type, 
					 int base_index, 
					 int index);

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
BTRFS_ParseSuperblock(void *buf, 
					  BTRFS_Superblock **block);

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

///
/// @brief      Get the chunk tree root address.
///
/// @return     The logical address of the chunk tree.
///
uint64_t
BTRFS_GetChunkTreeRootAddress(void);

///
/// @brief      Get the volume label.
///
/// @param      buffer  The buffer of 0x100 bytes in which to put the label name.
///
void
BTRFS_GetLabel(char *buffer);

///
/// @brief      Get the checksum tree location.
///
/// @return     The logical address of the checksum tree.
///
uint64_t
BTRFS_GetChecksumTreeLocation(void);

///
/// @brief      Get the FS tree location.
///
/// @return     The logical address of the fs tree.
///
uint64_t
BTRFS_GetFSTreeLocation(void);

///
/// @brief      Get the dev tree location.
///
/// @return     The logical address of the dev tree.
///
uint64_t
BTRFS_GetDevTreeLocation(void);

///
/// @brief      Get the extent tree location.
///
/// @return     The logical address of the extent tree.
///
uint64_t
BTRFS_GetExtentTreeLocation(void);

///
/// @brief      Verify the file system's checksums.
///
/// @return     The number of checksum mismatches detected.
///
uint64_t
BTRFS_Scrub(void);

///
/// @brief      Parse the root tree.
///
/// @return     Error code on failure, 0 on success.
///
int
BTRFS_ParseRootTree(void);

///
/// @brief      Parse the chunk tree.
///
/// @return     Error code on failure, 0 on success.
///
int
BTRFS_ParseChunkTree(void);

///
/// @brief      Get the inode for the specified file or directory.
///
/// @param      path            The path
/// @param      resolved_inode  The resolved inode
///
/// @return     -1 on checksum failure, -2 on file not found, 0 on success.
///
int
BTRFS_ParseFullFSTree(char *path, 
					  uint64_t *resolved_inode);


#endif
#ifndef _BTRFS_TYPES_H_
#define _BTRFS_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#define BTRFS_SuperblockOffset0 (64 * 1024)

#define BTRFS_MagicString "_BHRfS_M"
#define BTRFS_MagicStringLen 8

#define UUID_LEN 0x10
#define CHECKSUM_LEN 0x20

typedef enum {
	KeyType_InodeItem = 0x01,
	KeyType_InodeRef = 0x0c,
	KeyType_InodeExtRef = 0x0d,
	KeyType_XAttrItem = 0x18,
	KeyType_OrphanItem = 0x30,
	KeyType_DirLogItem = 0x3c,
	KeyType_DirLogIndex = 0x48,
	KeyType_DirItem = 0x54,
	KeyType_DirIndex = 0x60,
	KeyType_ExtentData = 0x6c,
	KeyType_ExtentChecksum = 0x80,
	KeyType_RootItem = 0x84,
	KeyType_RootBackRef = 0x90,
	KeyType_RoofRef = 0x9c,
	KeyType_ExtentItem = 0xa8,
	KeyType_TreeBlockRef = 0xb0,
	KeyType_ExtentDataRef = 0xb2,
	KeyType_ExtentRefV0 = 0xb4,
	KeyType_SharedBlockRef = 0xb6,
	KeyType_SharedDataRef = 0xb8,
	KeyType_BlockGroupItem = 0xc0,
	KeyType_DeviceExtent = 0xcc,
	KeyType_DeviceItem = 0xd8, 
	KeyType_ChunkItem = 0xe4,
	KeyType_StringItem = 0xfd,
} BTRFS_KeyType;

typedef enum {
	ReservedObjectID_ExtentTree = 0x02,
	ReservedObjectID_DevTree = 0x04,
	ReservedObjectID_FSTree = 0x05,
	ReservedObjectID_ChecksumTree = 0x07,
} BTRFS_ReservedObjectID;

typedef enum {
	DirectoryItemType_Unknown = 0,
	DirectoryItemType_File = 1,
	DirectoryItemType_Directory = 2,
} BTRFS_DirectoryItemType;

typedef enum {
	ExtentDataType_Inline = 0,
	ExtentDataType_Regular = 1,
	ExtentDataType_Prealloc = 2,
} BTRFS_ExtentDataType;

typedef struct {
	uint64_t object_id;
	uint8_t type;
	uint64_t offset;
} __attribute__((packed)) BTRFS_Key;

typedef struct {
	uint64_t seconds;
	uint32_t nanoseconds;
} __attribute__((packed)) BTRFS_Time;

typedef struct {
	uint64_t device_id;
	uint64_t offset;
	uint8_t uuid[UUID_LEN];
} __attribute__((packed)) BTRFS_Stripe;

typedef struct {
	uint64_t chunk_size_bytes;
	uint64_t object_id;
	uint64_t stripe_size;
	uint64_t type;
	uint32_t preferred_io_alignment;
	uint32_t preferred_io_width;
	uint32_t minimum_io_size;
	uint16_t stripe_count;
	uint16_t sub_stripes;
	BTRFS_Stripe stripes[0];
} __attribute__((packed)) BTRFS_ChunkItem;

typedef struct {
	BTRFS_Key key;
	BTRFS_ChunkItem value;
} BTRFS_Key_ChunkItem_Pair;

typedef struct {
	uint64_t device_id;
	uint64_t byte_count;
	uint64_t bytes_used;
	uint32_t preferred_io_alignment;
	uint32_t preferred_io_width;
	uint32_t minimum_io_size;
	uint64_t type;
	uint64_t generation;
	uint64_t start_offset;
	uint32_t device_group;
	uint8_t seek_speed;
	uint8_t bandwidth;
	uint8_t device_uuid[UUID_LEN];
	uint8_t fs_uuid[UUID_LEN];
} __attribute__((packed)) BTRFS_DeviceItem;

typedef struct {
	/*Checksum of the superblock*/
	uint8_t csum[CHECKSUM_LEN];

	uint8_t uuid[UUID_LEN];
	
	/*Physical address of this block*/
	uint64_t cur_block_phys_addr;
	
	uint64_t flags;
	
	/*"_BHRIS_M"*/
	char magic[8];
	
	uint64_t generation;
	
	/*Logical address of the root tree root*/
	uint64_t root_tree_root_addr;
	
	/*Logical address of the chunk tree root*/
	uint64_t chunk_tree_root_addr;
	
	/*Logical address of the log tree root*/
	uint64_t log_tree_root_addr;
	
	uint64_t log_root_transid;
	
	uint64_t total_bytes;
	
	uint64_t bytes_used;
	
	uint64_t root_dir_objectid;
	
	uint64_t num_devices;
	
	uint32_t sector_size;
	
	uint32_t node_size;
	
	uint32_t leaf_size;
	
	uint32_t stripe_size;
	
	/*Length in bytes of the (KEY, CHUNK_ITEM) table at the end of the superblock*/
	uint32_t key_chunkItem_table_len;
	
	uint64_t chunk_root_generation;
	
	uint64_t compat_flags;
	
	/*Only implementations that support the flags can write*/
	uint64_t compat_ro_flags;
	
	/*Only implementations that support the flags can use*/
	uint64_t inompat_flags;
	
	uint16_t checksum_type;
	
	uint8_t root_level;
	
	uint8_t chunk_root_level;
	
	uint8_t log_root_level;
	
	BTRFS_DeviceItem dev_item;
	
	char label[0x100];
	
	uint8_t rsv0[0x100];

	/*Start of the (KEY, CHUNK_ITEM) table*/
	uint8_t key_chunkItem_table[0];
} __attribute__((packed)) BTRFS_Superblock;

typedef struct {
	uint64_t device_id;
	uint64_t physical_addr;
} BTRFS_PhysicalAddress;

typedef struct {
	BTRFS_Key key;
	uint64_t block_number;
	uint64_t generation;
} __attribute__((packed)) BTRFS_KeyPointer;

typedef struct {
	BTRFS_Key key;
	uint32_t data_offset;
	uint32_t data_size;
} __attribute__((packed)) BTRFS_ItemPointer;

typedef struct {
	uint8_t csum[CHECKSUM_LEN];
	uint8_t uuid[UUID_LEN];
	uint64_t logical_address;
	uint8_t flags[7];
	uint8_t backref_revision;
	uint8_t chunk_tree_uuid[UUID_LEN];
	uint64_t generation;
	uint64_t parent_tree_id;
	uint32_t item_count;
	uint8_t level;
} __attribute__((packed)) BTRFS_Header;

typedef struct {
	BTRFS_Header hdr;
	BTRFS_KeyPointer key_ptrs[0];
} BTRFS_InternalNode;

//If the level of the node is 0, the entry is a leaf node
typedef struct {
	BTRFS_Header hdr;
	BTRFS_ItemPointer item_ptrs[0];
} BTRFS_LeafNode;

typedef struct {
	uint64_t generation;
	uint64_t last_transid;
	uint64_t st_size;
	uint64_t st_blocks;
	uint64_t block_group;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	uint32_t st_mode;
	uint64_t st_rdev;
	uint64_t flags;
	uint64_t sequence;
	uint8_t rsv0[0x20];
	BTRFS_Time st_atime;
	BTRFS_Time st_ctime;
	BTRFS_Time st_mtime;
	BTRFS_Time otime;
} __attribute__((packed)) BTRFS_InodeItem;

typedef struct {
	uint64_t index;
	uint16_t name_len;
	char name[0];
} BTRFS_InodeReference;

typedef struct {
	uint64_t dir_objectid;
	uint64_t index;
	uint16_t name_len;
	char name[0];
} BTRFS_InodeExtReference;

typedef struct {
	BTRFS_Key key;
	uint64_t transid;
	uint16_t data_size;
	uint16_t name_len;
	uint8_t type;
	char name_data[0];
} __attribute__((packed)) BTRFS_DirectoryItem;

typedef BTRFS_DirectoryItem BTRFS_ExtendedAttributeItem;

typedef struct {
	uint64_t end_offset;
} BTRFS_LogItem;

typedef BTRFS_LogItem BTRFS_DirectoryLogIndex;
typedef BTRFS_DirectoryItem BTRFS_DirectoryIndex;

typedef struct {
	uint64_t generation;
	uint64_t decoded_size;
	uint8_t compression_type;
	uint8_t encryption_present;
	uint16_t other_encoding;
	uint8_t type;
} __attribute__((packed)) BTRFS_ExtentDataInline;

typedef struct {
	BTRFS_ExtentDataInline inlineData;
	uint64_t extent_logical_addr;
	uint64_t extent_size;
	uint64_t extent_offset;
	uint64_t logical_byte_count;
} __attribute__((packed)) BTRFS_ExtentDataFull;

typedef struct {
	BTRFS_InodeItem inode;
	uint64_t expected_generation;
	uint64_t tree_root_object_id;
	uint64_t root_block_num;
	uint64_t byte_limit;
	uint64_t bytes_used;
	uint64_t last_snapshot_generation;
	uint64_t flags;
	uint32_t reference_count;
	BTRFS_Key drop_progress;
	uint8_t drop_level;
	uint8_t level;
} __attribute__((packed)) BTRFS_RootItem;

typedef struct {
	uint64_t dir_objectid;
	uint64_t index;
	uint16_t name_len;
	char name[0];
} BTRFS_RootReference;

typedef BTRFS_RootReference BTRFS_RootBackReference;

#endif
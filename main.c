/**
 * Copyright (c) 2017 Himanshu Goel
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <stdlib.h>

#include "btrfs/btrfs.h"

static FILE *fd = NULL;

uint64_t disk_read(void *buf, uint64_t devID, uint64_t off, uint64_t len) {
  fseek(fd, off, SEEK_SET);
  return fread(buf, 1, len, fd);
}

uint64_t disk_write(void *buf, uint64_t devID, uint64_t off, uint64_t len) {
  return -1;
}

int main(int argc, char *argv[]) {
  fd = fopen(argv[1], "rb");
  if (fd == NULL) {
    printf("Failed to load image.");
    return 0;
  }

  int retVal = 0;

  BTRFS_InitializeStructures(32 * 1024);
  BTRFS_SetDiskReadHandler(disk_read);
  BTRFS_SetDiskWriteHandler(disk_write);

  retVal = BTRFS_StartParser();

  printf("RetVal = %d\n", retVal);


  uint64_t inode = 0;
  retVal = BTRFS_ParseFullFSTree("/test/wallpaper.png", &inode);

  void *file_buf = malloc(10 * 1024 * 1024);
  uint64_t len = BTRFS_ReadFile(inode, 0, 10 * 1024 * 1024, file_buf);

   FILE *oF = fopen("test.png", "wb");
   fwrite(file_buf, 1, len, oF);
   fclose(oF);

  printf("Result: %lld RetVal = %d Inode: %lld\n", len, retVal, inode);

  BTRFS_Header *children = malloc(BTRFS_GetNodeSize());
  if (BTRFS_GetNode(children, BTRFS_GetFSTreeLocation()) != 0) {
    return -1;
  }

  BTRFS_TraverseLogTree(children);
  // Build an actual mapping table to translate logical addresses
  // Use it to walk the chunk tree
  // Use the chunk tree to be able to translate any logical address
  // Dev tree only needed when shrinking/removing devices

  // Parse root tree to get the fs tree and checksum tree
  // The fs tree is traversed for file reads

  // The checksum tree is used to verify the filesystem
}
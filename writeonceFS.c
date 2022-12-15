#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct diskinfo{
  char diskName[16];
  unsigned int numFiles;
  unsigned int numBlocksUsed;
  time_t createTime;
  time_t modifiedTime;
  struct fileNode *firstFile;
};

struct filenode{
  char name[16];
  unsigned int size;
  time_t createTime;
  time_t modifyTime;
  struct filenode *nextfile;
};

void printDisk(void *memaddr){
  struct diskinfo super;
  memcpy(&super, memaddr, sizeof(struct diskinfo));
  printf("Disk %s: %u Files, %u Blocks Used\n", super.diskName, super.numFiles, super.numBlocksUsed);
  printf("Created: %s Modified: %s\n", ctime(&super.createTime), ctime(&super.modifiedTime));
}

void createEmptyDisk(char *diskName, void *memaddr){
  struct diskinfo super;
  memcpy(super.diskName, diskName, strlen(diskName)+1);
  super.numFiles = 0;
  super.numBlocksUsed = 0;
  super.firstFile = NULL;
  super.createTime = time(NULL);
  memcpy(memaddr, &super, sizeof(struct diskinfo));
}

int wo_mount(char *filename, void *memaddr){
  FILE *fp = fopen(filename, "r");
  fread(memaddr, 4096, 1, fp);
  fclose(fp);
  return 0;
}

int wo_unmount(void *memaddr){
  struct diskinfo dInfo;
  memcpy(&dInfo, memaddr, sizeof(struct diskinfo));
  dInfo.modifiedTime = time(NULL);
  memcpy(memaddr, &dInfo, sizeof(struct diskinfo));
  FILE *fp = fopen(dInfo.diskName, "w+");
  fwrite(memaddr, 4096, 1, fp);
  fclose(fp);
  return 0;
}



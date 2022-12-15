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
  struct filenode *firstFile;
  unsigned int blockTable[128]; // What blocks are used overall
};

struct filenode{
  char name[16];
  unsigned int size;
  unsigned int permissions;
  time_t createTime;
  time_t modifiedTime;
  struct filenode *nextFile;
  unsigned int usedTable[128]; // What blocks this file occupies
};

struct fileDescriptor{ // Used only on runtime (not stored on disk)
  int fd;
  int currentPos;
  void *fileAddr;
  struct fileDescriptor *next;
};

void *baseAddress; // Compute relative addresses on disk instead
struct fileDescriptor *fdList;
int diskMounted = 0; // 1 if disk is mounted

// Get and set bits from blocks
int getBlock(int *table, int position){
  return table[position/32]<<(position%32)>>31;
}
void setBlock(int *table, int position, int value){
  table[position/32] = (value==0) ? table[position/32]|(1<<(position%32)) : table[position/32] & ~(1<<(position%32));
}

void printDisk(void *memaddr){
  struct diskinfo super;
  memcpy(&super, memaddr, sizeof(struct diskinfo));
  printf("Disk %s: %u Files, %u Blocks Used\n", super.diskName, super.numFiles, super.numBlocksUsed);
  printf("Created: %s",ctime(&super.createTime));
  printf("Modified: %s\n",ctime(&super.modifiedTime));
}

void createEmptyDisk(char *diskName, void *memaddr){
  struct diskinfo super;
  memcpy(super.diskName, diskName, strlen(diskName)+1);
  super.numFiles = 0;
  super.numBlocksUsed = 0;
  super.firstFile = NULL;
  super.createTime = time(NULL);
  super.modifiedTime = time(NULL);
  setBlock(super.blockTable, 0, 1);
  memcpy(memaddr, &super, sizeof(struct diskinfo));
}

int wo_mount(char *filename, void *memaddr){
  FILE *fp = fopen(filename, "r");
  fread(memaddr, 4096, 1, fp);
  baseAddress = memaddr;
  fclose(fp);
  diskMounted = 1;
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
  diskMounted = 0;
  return 0;
}

// RDONLY is 0, WRONLY is 1, RDWR is 2, CR is 100
int wo_open(char *fileName, int flags, int mode){
  if (mode==100){

  }
  return 0;
}

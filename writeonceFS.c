#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct diskinfo{
  char diskName[16];
  unsigned int diskSize;
  unsigned int usedSpace;
  unsigned int usedBlocks;
  unsigned int numBlocks;
  time_t createTime;
  time_t modifiedTime;
  unsigned int firstFile;
  unsigned int blockTable[128]; // What blocks are used overall
};

struct filenode{
  char name[16];
  unsigned int size;
  unsigned int permissions;
  time_t createTime;
  time_t modifiedTime;
  unsigned int nextFile;
  unsigned int usedTable[128]; // What blocks this file occupies
};

struct fileDescriptor{ // Used only on runtime (not stored on disk)
  int fd;
  int currentPos;
  unsigned int locationBlock;
  struct fileDescriptor *next;
};

void *baseAddress; // Compute relative addresses on disk instead
struct fileDescriptor *fdList;
int currentFD = 0; // FD generation increments
int diskMounted = 0; // 1 if disk is mounted
struct diskinfo super;

// Get and set bits from blocks
int getBlock(unsigned int *table, int position){
  return table[position/32]<<(position%32)>>31;
}
void setBlock(unsigned int *table, int position, int value){
  table[position/32] = (value==0) ? table[position/32]|(1<<(position%32)) : table[position/32] & ~(1<<(position%32));
}

void printDisk(void *memaddr){
  printf("Disk %s: %u/%u Bytes Used, %u/%u Blocks Used\n", super.diskName, super.usedSpace, super.diskSize, super.usedBlocks, super.numBlocks);
  printf("Created: %s",ctime(&super.createTime));
  printf("Modified: %s",ctime(&super.modifiedTime));
}

void createEmptyDisk(char *diskName, unsigned int diskSize, void *memaddr){
  memcpy(super.diskName, diskName, strlen(diskName)+1);
  super.firstFile = 0;
  super.diskSize = diskSize;
  super.usedSpace = 0;
  super.usedBlocks = 1;
  super.numBlocks = diskSize/1024;
  super.createTime = time(NULL);
  super.modifiedTime = time(NULL);
  setBlock(super.blockTable, 0, 1);
  memcpy(memaddr, &super, sizeof(struct diskinfo));
  diskMounted = 1;
}

int wo_mount(char *filename, void *memaddr){
  FILE *fp = fopen(filename, "r");
  fread(&super, 1024, 1024, fp);
  fclose(fp);
  baseAddress = memaddr;
  fp = fopen(filename, "r");
  fread(memaddr, super.diskSize, 1024, fp);
  fclose(fp);
  diskMounted = 1;
  return 0;
}

int wo_unmount(void *memaddr){
  super.modifiedTime = time(NULL);
  FILE *fp = fopen(super.diskName, "w+");
  fwrite(memaddr, 1024, super.numBlocks, fp);
  fclose(fp);
  diskMounted = 0;
  return 0;
}

// Returns block number of file or 0 if not found
unsigned int findFile(char *fileName){
  unsigned int currentFile = super.firstFile;
  if (currentFile==0){
    return 0;
  }
  struct filenode current;
  while (currentFile!=0 && currentFile<super.numBlocks){
    memcpy(&current, baseAddress+(1024*currentFile),1024);
    if (strcmp(current.name, fileName)==0){
      return currentFile;
    } else {
      currentFile = current.nextFile;
    }
  }
  return 0;
}

unsigned int getFirstFreeBlock(){
  for (unsigned int i=1;i<super.numBlocks;i++){
    if (getBlock(super.blockTable, 1)==0){
      return i;
    }
  }
  return 0;
}

int createFD(unsigned int fileBlock){
  struct fileDescriptor *newFD = malloc(sizeof(struct fileDescriptor));
  newFD->locationBlock = fileBlock;
  newFD->currentPos = 0;
  newFD->next = fdList;
  newFD->fd = currentFD;
  currentFD++;
  fdList = newFD;
  return newFD->fd;
}

// RDONLY is 0, WRONLY is 1, RDWR is 2, CR is 100
int wo_open(char *fileName, int flags, int mode){
  if (mode==100){
    if (findFile(fileName)==0){
      unsigned int freeBlock = getFirstFreeBlock();
      if (freeBlock!=0){
        struct filenode newFile;
        memcpy(&newFile.name, fileName, 16);
        newFile.size = 0;
        newFile.permissions = 777;
        newFile.createTime = time(NULL);
        newFile.modifiedTime = time(NULL);
        newFile.nextFile = 0;
        memset(&newFile.usedTable, 0, 512);
        memcpy(baseAddress+(1024*freeBlock), &newFile, 1024);
        super.usedBlocks++;
        return createFD(freeBlock);
      } else {
        return -1; // Not enough space, can't be created
      }
    } else {
      return -1; // File already exists, can't be created
    }
  } else {
    return -1;
  }
}



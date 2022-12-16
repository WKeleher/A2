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
  int permissions;
  time_t createTime;
  time_t modifiedTime;
  unsigned int nextFile;
  unsigned int usedTable[128]; // What blocks this file occupies
};

struct fileDescriptor{ // Used only on runtime (not stored on disk)
  int fd;
  unsigned int locationBlock;
  unsigned int currentPos;
  int permissions;
  struct fileDescriptor *next;
};

void *baseAddress; // Compute relative addresses on disk instead
struct fileDescriptor *fdList = NULL;
int currentFD = 1; // FD generation increments
int diskMounted = 0; // 1 if disk is mounted
struct diskinfo super;

// Get and set bits from blocks
int getBlock(unsigned int *table, unsigned int position){
  return (table[position/32]<<(position%32))>>31;
}
void setBlock(unsigned int *table, unsigned int position){
  table[position/32] |= (1<<(31-(position%32)));
}

void printDisk(void *memaddr){
  printf("Disk %s: %u/%u Bytes Used, %u/%u Blocks Used\n", super.diskName, super.usedSpace, super.diskSize, super.usedBlocks, super.numBlocks);
  printf("Created: %s",ctime(&super.createTime));
  printf("Modified: %s",ctime(&super.modifiedTime));
  unsigned int current = super.firstFile;
  while (current!=0 && current<super.numBlocks){
    struct filenode *currentFile = baseAddress+(1024*current);
    printf("---%s: %u bytes - Permissions: %u\n", currentFile->name, currentFile->size, currentFile->permissions);
    current = currentFile->nextFile;
  }
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
  memset(super.blockTable, 0, 512);
  setBlock(super.blockTable, 0);
  memcpy(memaddr, &super, sizeof(struct diskinfo));
  baseAddress = memaddr;
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
  memcpy(memaddr, &super, 1024);
  fwrite(memaddr, 1024, super.numBlocks, fp);
  fclose(fp);
  diskMounted = 0;
  return 0;
}

// Returns block number of file or 0 if not found
unsigned int findFile(char *fileName){
  unsigned int currentFile = super.firstFile;
  while (currentFile!=0 && currentFile<super.numBlocks){
    struct filenode *current = baseAddress+(1024*currentFile);
    if (strcmp(current->name, fileName)==0){
      return currentFile;
    } else {
      currentFile = current->nextFile;
    }
  }
  return 0;
}

unsigned int getFirstFreeBlock(){
  for (unsigned int i=1;i<super.numBlocks;i++){
    if (getBlock(super.blockTable, i)==0){
      return i;
    }
  }
  return 0;
}

unsigned int getLastFile(){
  if (super.firstFile==0){
    return 0;
  }
  unsigned int lastFile = 0;
  unsigned int currentFile = 1;
  while (currentFile!=0 && currentFile<super.numBlocks){
    struct filenode *current = baseAddress+(1024*currentFile);
    lastFile = currentFile;
    currentFile = current->nextFile;
  }
  return lastFile;
}

int createFD(int fileBlock){
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
        struct filenode *newFile = baseAddress+(1024*freeBlock);
        memcpy(newFile->name, fileName, 16);
        newFile->size = 0;
        newFile->permissions = 777;
        newFile->createTime = time(NULL);
        newFile->modifiedTime = time(NULL);
        newFile->nextFile = 0;
        memset(newFile->usedTable, 0, 512);
        super.usedBlocks++;
        setBlock(super.blockTable, freeBlock);
        struct filenode *prevFile = baseAddress+(1024*getLastFile());
        prevFile->nextFile = freeBlock;
        memcpy(newFile->name, fileName, 16);
        if (super.firstFile==0){
          super.firstFile = freeBlock;
        }
        return createFD(freeBlock);
      } else {
        return -25; // Not enough space, can't be created
      }
    } else {
      return -15; // File already exists, can't be created
    }
  } else {
    unsigned int foundFile = findFile(fileName);
    if (foundFile==0){
      return -1; // File doesn't exist
    } else {
      int fd = createFD(foundFile);
      fdList->permissions = flags;
      return fd;
    }
  }
}

// Check and return permissions of valid FD, or -1
struct fileDescriptor* checkFD(int fd){
  struct fileDescriptor *current = fdList;
  while (current!=NULL){
    if (current->fd==fd){
      return current;
    }
    current = current->next;
  }
  return NULL;
}

int wo_read(int fd, void *buffer, int bytes){
  struct fileDescriptor *validFD = checkFD(fd);
  if (validFD==NULL){
    return -10; // Invalid FD
  } else if (validFD->permissions==1){
    return -5; // Cannot read a write only file
  } else {
    struct filenode *readFile = baseAddress+(1024*validFD->locationBlock);
    if (bytes>(readFile->size - validFD->currentPos)){
      return -20; // Not enough bytes to be read in file
    }
    unsigned int bytesRemaining = bytes;
    unsigned int positionRemaining = validFD->currentPos;
    unsigned int bufferPos = 0;
    for (unsigned int i=0;i<super.numBlocks;i++){
      if (getBlock(readFile->usedTable, i)==1){
        if (positionRemaining>(1024-1)){
          positionRemaining -= 1024;
        } else if (positionRemaining>0){
          if (bytesRemaining<(1024-positionRemaining)){
            memcpy(buffer+bufferPos, baseAddress+(1024*i)+positionRemaining, bytesRemaining);
            positionRemaining = 0;
            bytesRemaining = 0;
          } else {
            memcpy(buffer+bufferPos, baseAddress+(1024*i)+positionRemaining, 1024-positionRemaining);
            positionRemaining = 0;
            bytesRemaining -= (1024-positionRemaining);
            bufferPos += (1024-positionRemaining);
          }
        } else if (bytesRemaining>(1024-1)){
          memcpy(buffer+bufferPos, baseAddress+(1024*i), 1024);
          bytesRemaining -= 1024;
          bufferPos += 1024;
        } else if (bytesRemaining>0){
          memcpy(buffer+bufferPos, baseAddress+(1024*i), bytesRemaining);
          bytesRemaining = 0;
          bufferPos += bytesRemaining;
        }
      }
    }
    validFD->currentPos += bytes;
    return 0;
  }
}

int wo_write(int fd, void *buffer, int bytes){
  struct fileDescriptor *validFD = checkFD(fd);
  if (validFD==NULL){
    return -3; // Invalid FD
  } else if (validFD->permissions==0){
    return -7; // Cannot write a read only file
  } else {
    struct filenode *readFile = baseAddress+(1024*validFD->locationBlock);
    if ((super.numBlocks - super.usedBlocks - 1)<bytes/1024){
      return -12; // Not enough space on disk.
    }
    unsigned int bytesRemaining = bytes;
    unsigned int positionRemaining = validFD->currentPos;
    unsigned int bufferPos = 0;
    for (unsigned int i=0;i<super.numBlocks;i++){
      if (getBlock(readFile->usedTable, i)==1){
        if (positionRemaining>(1024-1)){
          positionRemaining -= 1024;
        } else if (positionRemaining>0){
          memcpy(baseAddress+(1024*i)+positionRemaining, buffer+bufferPos, 1024-positionRemaining);
          bytesRemaining -= (1024-positionRemaining);
          bufferPos += (1024-positionRemaining);
          positionRemaining = 0;
        } else if (bytesRemaining>0){
          break;
        }
      }
    }
    while (bytesRemaining>0){
      printf("Hello\n");
      unsigned int firstFree = getFirstFreeBlock();
      setBlock(readFile->usedTable, firstFree);
      if (bytesRemaining>(1024-1)){
        memcpy(baseAddress+(1024*firstFree),buffer+bufferPos, 1024);
        bufferPos += 1024;
        bytesRemaining -= 1024;
      } else {
        memcpy(baseAddress+(1024*firstFree),buffer+bufferPos, bytesRemaining);
        bufferPos += bytesRemaining;
        bytesRemaining = 0;
      }
    }
    readFile->size += bytes;
    validFD->currentPos += bytes;
    return 0;
  }
}


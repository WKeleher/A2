#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *disk; 

struct diskinfo{
  char diskName[16];
  int numFiles;
  int numBlocksUsed;
  struct fileNode *firstFile;
};

struct filenode{
  char name[16];
  int size;
  int numblocks;
  struct filenode *nextfile;
};

int wo_mount(char *filename, void *memaddr){
  FILE *fp = fopen(filename, "r");
  fread(memaddr, 4096, 1, fp);
  disk = memaddr;
  fclose(fp);
  return 0;
}

int wo_unmount(void *memaddr){
  char fName[16];
  memcpy(&fName, disk, 16);
  fName[15] = '\0';
  FILE *fp = fopen(fName, "w+");
  fwrite(memaddr, 4096, 1, fp);
  fclose(fp);
  return 0;
}

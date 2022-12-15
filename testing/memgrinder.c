#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../writeonceFS.c"

int main(int argc, char *argv[]){
  char *disk = malloc(32768);
  //createEmptyDisk("testdisk", 32768, disk);
  wo_mount("testdisk", disk);
  printDisk(disk);
  int fd = wo_open("file1.txt", 777, 100);
  printf("fd: %d\n", fd);
  printDisk(disk);
  wo_unmount(disk);
  free(disk);
}

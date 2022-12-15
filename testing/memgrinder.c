#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../writeonceFS.c"

int main(int argc, char *argv[]){
  char *disk = malloc(4096);
  createEmptyDisk("testdisk", 4096, disk);
  //wo_mount("testdisk", disk);
  printDisk(disk);
  wo_unmount(disk);
  free(disk);
}

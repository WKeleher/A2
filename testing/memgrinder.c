#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../writeonceFS.c"

int main(int argc, char *argv[]){
  char *disk = malloc(4096);
  wo_mount("testdisk123.txt", disk);
  for (int i=65;i<91;i++){
    disk[i] = i;
  }
  wo_unmount(disk);
  free(disk);
}

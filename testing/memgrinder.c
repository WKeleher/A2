#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../writeonceFS.c"

int main(int argc, char *argv[]){
  char *disk = malloc(32768);
  wo_mount("testdisk2", disk);
  int d;
  char input[20];
  while (1==1){
    printDisk(disk);
    printf("Create, read, write file, or exit? 0 1 2 3\n");
    scanf("%d", &d);
    if (d==3){
      break;
    }
    printf("File Name?\n");
    scanf("%s", &input);
    int fd;
    if (d==0){	
      fd = wo_open(&input, 2, 100);
    } else if (d==1){
      fd = wo_open(&input, 2, 0);
      char buffer[500];
      wo_read(fd, &buffer, 500);
      printf("%s\n", &buffer);
    } else {
      int size = 0;
      char buffer[5000];
      int alive = 1;
      char linebuffer[100];
      char exit[] = "exit";
      while (size<900){
        printf("%s\n", linebuffer);
        sprintf(linebuffer, "%s\n", buffer);
        scanf("%s\n", linebuffer);
        size += 100;
      }
      wo_write(fd, &buffer, size);
    }
  }
  wo_unmount(disk);
  free(disk);
}

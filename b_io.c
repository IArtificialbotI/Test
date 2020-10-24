#include "b_io.h"

int b_open (char * filename, int flags){
  return 1;
}

int b_read (int fd, char * buffer, int count){
  return 1;
}

int b_write (int fd, char * buffer, int count){
  return 1;
}
int b_seek (int fd, off_t offset, int whence){
  return 1;
}
void b_close (int fd){
}
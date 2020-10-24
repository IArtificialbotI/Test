#include "b_io.h"
#include "fsLow.h"
#include "mfs.h"

int fs_mkdir(const char *pathname, mode_t mode);
{
    return 1;
}
int fs_rmdir(const char *pathname)
{
    return 1;
}
fdDir * fs_opendir(const char *name)
{
    return name;
}
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    return dirp;
}
int fs_closedir(fdDir *dirp)
{
    return 1;
}

char * fs_getcwd(char *buf, size_t size){
  return buf;
}
int fs_setcwd(char *buf){
    return 1;
}  

int fs_isFile(char * path){
    return 1;
}	

int fs_isDir(char * path){
    return 1;
}		

int fs_delete(char* filename){
    return 1;
}	

int fs_stat(const char *path, struct fs_stat *buf){
  return 1;
}

/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "HandleVolumeUtility.h"
#include "b_io.h"

#define FILENAME_LENGTH 32
#define FILEPATH_LENGTH 248
#define DATABLOCK_SIZE 64
#define INVALID_DATABLOCK_VALUE -1
#define DIRECTORY_DEPTHS 10
#define NUMBER_CHILDREN 64

/*Type of node in filesystem*/
typedef enum
{
	E_FILE,
	E_DIR,
	E_UNUSED
} E_NodeType;

/* In memory structure defined by linux. */
struct fs_diriteminfo
{
	ino_t d_id;
	unsigned short d_reclen;
	unsigned char fileType;
	char d_name[256]; /* filename max filename is 255 characters */
};

typedef struct
{
	uint64_t id; // index of the inode
	int inUse;	 //  0: inode is free and 1: node in use
	E_NodeType type;
	char parent[FILEPATH_LENGTH];
	char children[NUMBER_CHILDREN][FILENAME_LENGTH]; // array  for names of the children
	int numChildren;								 //  number of children
	char name[FILENAME_LENGTH];						 // the file name
	char path[FILEPATH_LENGTH];						 // the path of the file/folder
	int blockData[DATABLOCK_SIZE];					 // data blocks
	int numBlockElements;						 // number of elements data blocks

} fd_DIR;

// Changed fs_setcwd to return an int instead of char*
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fd_DIR *fs_opendir(const char *fileName);
struct fs_diriteminfo *fs_readdir(fd_DIR *dirp);
int fs_closedir(fd_DIR *dirp);

char *fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);

int fs_isFile(char *path);	   //return 1 if file, 0 otherwise
int fs_isDir(char *path);	   //return 1 if directory, 0 otherwise
int fs_delete(char *filename); //removes a file

void fs_init();
void fs_writeInodes();
void fs_close();

void parseFilePath(const char *pathname);

fd_DIR *getInode(const char *pathname);
fd_DIR *getFreeInode();
void printCurrentDirectoryPath();

fd_DIR *createInodeForPath(E_NodeType type, const char *path);
int setParentPath(fd_DIR *parent, fd_DIR *child);
char *getParentPath(char *buf, const char *path);

/* Writes data to Node */
int writeDataToInode(fd_DIR *inode, char *buffer, size_t bufSizeBytes, uint64_t blockNumber);

void freeInode(fd_DIR *node);

struct fs_stat
{
	off_t st_size;		  /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t st_blocks;	  /* number of 512B blocks allocated */
	time_t st_accesstime; /* time of last access */
	time_t st_modtime;	  /* time of last modification */
	time_t st_createtime; /* time of last status change */
};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

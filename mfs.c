/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: mfs.c
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#include "mfs.h"

fd_DIR *inodes;

void fs_init()
{
  printf("==========================fs_init==========================\n");

  uint64_t totalBytes = getVolumePointer()->totalInodeBlocks * getVolumePointer()->blockSize;
  printf("fs_init: totalInodeBlocks %ld, blockSize %ld\n", getVolumePointer()->totalInodeBlocks, getVolumePointer()->blockSize);

  inodes = calloc(getVolumePointer()->totalInodeBlocks, getVolumePointer()->blockSize);

  uint64_t blocksRead = LBAread(inodes, getVolumePointer()->totalInodeBlocks, getVolumePointer()->inodeStartBlock);

  if (blocksRead != getVolumePointer()->totalInodeBlocks)
  {
    printf("Error: Not all inodes loaded into cache.\n");
    fs_close();
    exit(0);
  }

  fs_setcwd("/root");

  printf("=========================================================\n");
}

void fs_writeInodes()
{
  LBAwrite(inodes, getVolumePointer()->totalInodeBlocks, getVolumePointer()->inodeStartBlock);
}

char currentDirectoryPath[FILEPATH_LENGTH];
char currentDirectoryPathArray[DIRECTORY_DEPTHS][FILENAME_LENGTH];
int currentDirectoryPathArraySize = 0;

char requestedFilePath[FILEPATH_LENGTH];
char requestedFilePathArray[DIRECTORY_DEPTHS][FILENAME_LENGTH];
int requestedFilePathArraySize = 0;

void parseFilePath(const char *pathname)
{
  printf("==========================Parsing File Path==========================\n");

  printf("Input: %s\n", pathname);

  /* Clear previous value and count. */
  requestedFilePath[0] = '\0';
  requestedFilePathArraySize = 0;

  /* Make mutable copy of pathname. */
  char _pathname[FILEPATH_LENGTH];
  strcpy(_pathname, pathname);

  /* Setup tokenizer. */
  char *savePointer;
  char *token = strtok_r(_pathname, "/", &savePointer);

  /* Categorize the pathname. */
  int isAbsolute = pathname[0] == '/';
  int isSelfRelative = !strcmp(token, ".");
  int isParentRelative = !strcmp(token, "..");
  // Fourth case: relative to cwd

  if (token && !isAbsolute)
  {
    int maxLevel = isParentRelative ? currentDirectoryPathArraySize - 1 : currentDirectoryPathArraySize;
    for (int i = 0; i < maxLevel; i++)
    {
      strcpy(requestedFilePathArray[i], currentDirectoryPathArray[i]);
      sprintf(requestedFilePath, "%s/%s", requestedFilePath, currentDirectoryPathArray[i]);
      requestedFilePathArraySize++;
    }
  }

  if (isSelfRelative || isParentRelative)
  {
    token = strtok_r(0, "/", &savePointer);
  }

  while (token && requestedFilePathArraySize < DIRECTORY_DEPTHS)
  {

    strcpy(requestedFilePathArray[requestedFilePathArraySize], token);
    sprintf(requestedFilePath, "%s/%s", requestedFilePath, token);

    requestedFilePathArraySize++;
    token = strtok_r(0, "/", &savePointer);
  }

  printf("Output: %s\n", requestedFilePath);

  printf("=========================================================\n");
}

fd_DIR *getInode(const char *pathname)
{
  printf("==========================getInode==========================\n");
  // Loop over inodes, find requested node, return that node, if does not exist return NULL

  printf("Searching for path: '%s'\n", pathname);
  for (size_t i = 0; i < getVolumePointer()->totalInodes; i++)
  {
    //printf("\tInode path: '%s'\n", inodes[i].path);
    if (strcmp(inodes[i].path, pathname) == 0)
    {
      printf("=========================================================\n");
      return &inodes[i];
    }
  }

  printf("Inode with path '%s' does not exist.\n", pathname);

  printf("=========================================================\n");

  return NULL;
}

fd_DIR *getFreeInode()
{

  fd_DIR *returnediNode;

  for (size_t i = 0; i < getVolumePointer()->totalInodes; i++)
  {
    if (inodes[i].inUse == 0)
    {                      // if the inode inUse equals 0, that means it is free so we return it
      inodes[i].inUse = 1; // update the node to be in use before returning it
      returnediNode = &inodes[i];

      return returnediNode;
    }
  }

  return NULL;
}

fd_DIR *createInodeForPath(E_NodeType type, const char *path)
{

  // returns an inode if it succeeds and NULL if it could not create the inode
  fd_DIR *inode;
  char parentPath[FILEPATH_LENGTH];
  fd_DIR *parentNode;

  /* Obtain current time. */
  time_t currentTime;
  currentTime = time(NULL);

  // call getFreeInode() to recieve the next available inode
  inode = getFreeInode();

  //find and assign the parent to the new inode
  getParentPath(parentPath, path);
  parentNode = getInode(parentPath);

  /* Set properties on inode. */
  inode->type = type;
  strcpy(inode->name, requestedFilePathArray[requestedFilePathArraySize - 1]);
  sprintf(inode->path, "%s/%s", parentPath, inode->name);

  /* Try to set the parent. If it fails, revert. */
  if (!setParentPath(parentNode, inode))
  {
    freeInode(inode);
    printf("Error setting parent. Reverting changes.\n");
    printf("==============================--------\n");
    return NULL;
  }

  printf("Sucessfully created inode for path '%s'.\n", path);
  return inode;
}

int parentHasChild(fd_DIR *parent, fd_DIR *child)
{

  /* Loop through children. Return 1 on the first name match. */
  for (int i = 0; i < parent->numChildren; i++)
  {
    if (!strcmp(parent->children[i], child->name))
    {
      return 1;
    }
  }

  return 0;
}

//set the given parent and child inodes to each other if they exist
//returns 0 if it can't, and 1 on success
int setParentPath(fd_DIR *parent, fd_DIR *child)
{

  //checking if the parent can take anymore children in our file system
  if (parent->numChildren == NUMBER_CHILDREN)
  {
    printf("Folder '%s' has maximum children.\n", parent->path);
    return 0;
  }

  //checking if a child like the one passed, already exists
  if (parentHasChild(parent, child))
  {
    printf("Folder '%s' already exists.\n", child->path);
    return 0;
  }

  //set the rest of the properties of the parent and child to correspond with each other
  strcpy(parent->children[parent->numChildren], child->name);
  parent->numChildren++;

  strcpy(child->parent, parent->path);
  sprintf(child->path, "%s/%s", parent->path, child->name);

  printf("Set parent of '%s' to '%s'.\n", child->path, child->parent);

  return 1;
}

int removeFromParent(fd_DIR *parent, fd_DIR *child)
{

  /* Loop through parent's list of children until name match. */
  for (int i = 0; i < parent->numChildren; i++)
  {

    if (!strcmp(parent->children[i], child->name))
    {

      /* Clear entry in parent's list of children. Decrement child count. */
      strcpy(parent->children[i], "");
      parent->numChildren--;
      return 1;
    }
  }

  printf("Could not find child '%s' in parent '%s'.\n", child->name, parent->path);
  return 0;
}

//method will return NULL if the parent's path cannot be found, otherwise sets the passed buffer to the path and returns it
char *getParentPath(char *buf, const char *path)
{

  //Parses the requestedFilePathArray into a string to find the parent string "path"
  //Copy parent path to buf, return buf

  /* Parse the path. */
  parseFilePath(path);

  char parentPath[FILEPATH_LENGTH] = "";

  /* Loop until the second to last element. */
  for (int i = 0; i < requestedFilePathArraySize - 1; i++)
  {

    /* Append a separator and the next level of the path. */
    strcat(parentPath, "/");
    strcat(parentPath, requestedFilePathArray[i]);
  }

  strcpy(buf, parentPath);

  printf("Input: %s, Parent Path: %s\n", path, buf);

  return buf;
}

int writeDataToInode(fd_DIR *inode, char *buffer, size_t bufSizeBytes, uint64_t blockNumber)
{
  printf("===============================writeDataToInode===============================\n");

  /* Check if dataBlockPointers is full. */
  int freeIndex = -1;
  for (int i = 0; i < DATABLOCK_SIZE; i++)
  {
    if (inode->blockData[i] == INVALID_DATABLOCK_VALUE)
    {

      /* Record free dataBlockPointer index. */
      freeIndex = i;
      break;
    }
  }

  /* If there is no place to put the new dataBlock pointer. Return 0 blocks/bytes written. */
  if (freeIndex == -1)
  {
    return 0;
  }

  /* Write buffered data to disk, update inode, write inodes to disk. */
  LBAwrite(buffer, 1, blockNumber);

  /* Record the block number in the inode, reserve the block in the freeMap and write the VCB. */
  inode->blockData[freeIndex] = blockNumber;
  setBit(getVolumePointer()->freeMap, blockNumber);
  writeVolume();

  inode->numBlockElements++;

  fs_writeInodes();

  printf("==============================================================\n");

  return 1;
}

void freeInode(fd_DIR *node)
{

  printf("Freeing inode: '%s'\n", node->path);

  node->inUse = 0;
  node->type = E_UNUSED;
  node->name[0] = NULL;
  node->path[0] = NULL;
  node->parent[0] = NULL;

  /* free the data blockes asociated with the file, if we are deleting a file */
  if (node->type == E_FILE)
  {
    for (size_t i = 0; i < node->numBlockElements; i++)
    {
      int blockPointer = node->blockData[i];
      clearBit(getVolumePointer()->freeMap, blockPointer);
    }
  }

  fs_writeInodes();
}

void fs_close()
{
  free(inodes);
}

int fs_mkdir(const char *pathname, mode_t mode)
{ // return 0 for sucsess and -1 if not
  printf("===============fs_mkdir===============\n");
  // Parses file name, adds info for inode fd_DIR
  // Adds info to parent if necessary and checks if the folder already exists
  char parentPath[256] = "";
  parseFilePath(pathname);

  for (size_t i = 0; i < requestedFilePathArraySize - 1; i++)
  {
    strcat(parentPath, "/");
    strcat(parentPath, requestedFilePathArray[i]);
  }

  fd_DIR *parent = getInode(parentPath);
  if (parent)
  {
    for (size_t i = 0; i < parent->numChildren; i++)
    {
      if (strcmp(parent->children[i], requestedFilePathArray[requestedFilePathArraySize - 1]))
      {
        printf("Folder already exists!\n");
        printf("=====================================\n");
        return -1;
      }
    }
  }
  else
  {
    printf("Parent '%s' does not exist!\n", parentPath);
    printf("=====================================\n");
    return -1;
  }

  if (createInodeForPath(E_DIR, pathname))
  {
    fs_writeInodes();
    printf("=====================================\n");
    return 0;
  }
  printf("Failed to make directory '%s'.\n", pathname);
  printf("=====================================\n");
  return -1;
}

//Upon success, returns a 0 otherwise returns a -1
int fs_rmdir(const char *pathname)
{
  printf("===============fs_rmdir===============\n");
  fd_DIR *node = getInode(pathname);
  if (!node)
  {
    printf("%s does not exist.\n", pathname);
    return -1;
  }
  fd_DIR *parent = getInode(node->parent);
  if (node->type == E_DIR && node->numChildren == 0)
  {
    removeFromParent(parent, node);
    freeInode(node);
    printf("==============================\n");
    return 0;
  }
  printf("==============================\n");
  return -1;
}

fd_DIR *fs_opendir(const char *fileName)
{
  printf("==============================fs_opendir==============================\n");
  int ret = b_open(fileName, 0);
  if (ret < 0)
  {
    printf("==============================\n");
    return NULL;
  }
  printf("==============================\n");
  return getInode(fileName);
}

int readdirCounter = 0;
struct fs_diriteminfo directoryEntry;

struct fs_diriteminfo *fs_readdir(fd_DIR *dirp)
{
  printf("==============================fs_readdir==============================\n");

  if (readdirCounter == dirp->numChildren)
  {
    readdirCounter = 0;
    return NULL;
  }

  /* Get child inode. */
  char childPath[FILEPATH_LENGTH];
  sprintf(childPath, "%s/%s", dirp->path, dirp->children[readdirCounter]);
  fd_DIR *child = getInode(childPath);

  /* Set properties on dirent. */
  directoryEntry.d_id = child->id;
  strcpy(directoryEntry.d_name, child->name);

  /* Increment the counter to the next child. */
  readdirCounter++;

  printf("==============================\n");
  return &directoryEntry;
}

int fs_closedir(fd_DIR *dirp)
{
  printf("==============================fs_closedir==============================\n");
  printf("==============================--------\n");
  //
  return 0;
}

char *fs_getcwd(char *buf, size_t size)
{
  printf("==============================fs_getcwd==============================\n");
  if (strlen(currentDirectoryPath) > size)
  {
    errno = ERANGE;
    printf("============================================\n");
    return NULL;
  }
  strcpy(buf, currentDirectoryPath);
  printf("=================================================\n");
  return buf;
}

//Note: This does not currently check validity of
//      the path. This may need to first check whether the provided path is within limit of FILENAME_LENGTH
//      and set errno similar to fs_getcwd.

int fs_setcwd(char *buf)
{
  printf("==============================fs_setcwd==============================\n");

  parseFilePath(buf);

  /* Check if inode exists. */
  fd_DIR *inode = getInode(requestedFilePath);
  if (!inode)
  {
    printf("Directory '%s' does not exist.\n", requestedFilePath);
    printf("=================================================\n");
    return 1;
  }

  /* Clear previous cwd. */
  currentDirectoryPath[0] = '\0';
  currentDirectoryPathArraySize = 0;

  /* Copy parsed pathname to currentDirectoryPathArray. */
  for (int i = 0; i < requestedFilePathArraySize; i++)
  {
    strcpy(currentDirectoryPathArray[i], requestedFilePathArray[i]);
    sprintf(currentDirectoryPath, "%s/%s", currentDirectoryPath, requestedFilePathArray[i]);
    currentDirectoryPathArraySize++;
  }

  printf("Set cwd to '%s'.\n", currentDirectoryPath);

  printf("===========================================\n");
  return 0;
}

//Added to test fs_setcwd
void printCurrentDirectoryPath()
{

  for (int i = 0; i < currentDirectoryPathArraySize; i++)
  {

    if (i < currentDirectoryPathArraySize - 1)
    {
      printf("Directory %d: %s\n", i, currentDirectoryPathArray[i]);
    }
    else
    {
      printf("Filename: %s\n", currentDirectoryPathArray[i]);
    }
  }
}

//Initial implementation of fs_isFile and fs_isDir
int fs_isFile(char *path)
{
  printf("===============--isFile===============\n");
  fd_DIR *inode = getInode(path);
  printf("================================================-\n");
  return inode ? inode->type == E_FILE : 0;
}

int fs_isDir(char *path)
{
  printf("===============--isDir===============-\n");
  fd_DIR *inode = getInode(path);
  printf("===============================================\n");
  return inode ? inode->type == E_DIR : 0;
}

int fs_delete(char *filePath)
{
  printf("==============================fs_delete==============================\n");
  //Get inode
  fd_DIR *fileNode = getInode(filePath);
  //Get parent
  fd_DIR *parentNode = getInode(fileNode->parent);
  //Remove child from parent
  removeFromParent(parentNode, fileNode);
  //Clear properties on child inode so it is not found in search & Set inuse to false.
  freeInode(fileNode);

  printf("================================================\n");
  return 0;
}

// fd_DIR needs rework to contain these fields.
int fs_stat(const char *path, struct fs_stat *buf)
{
  printf("===============fs_stat===============\n");
  fd_DIR *inode = getInode(path);
  if (inode)
  {
    buf->st_size = 999;
    printf("============================================\n");
    return 1;
  }
  printf("============================================\n");
  return 0;
}

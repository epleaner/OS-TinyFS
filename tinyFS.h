#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256
/* Your program should use a 10240 Byte disk size giving you 40 blocks total. This is a
 * default size. You must be able to support different possible values 
 */
#define DEFAULT_DISK_SIZE 10240 
/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk” 	
typedef int fileDescriptor;


/*	For libDisk.c	*/

typedef struct disk {
	FILE *file;
	int diskNum;
	int space;
	int open;
} Disk;

typedef struct diskNode {
	Disk *disk;
	struct diskNode *next;
} DiskNode;

/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. nBytes should be an integral number of the block size. If nBytes > 0 and there is already a file by the given filename, that file’s contents may be overwritten. If nBytes is 0, an existing disk is opened, and should not be overwritten. There is no requirement to maintain integrity of any file content beyond nBytes. The return value is -1 on failure or a disk number on success. */
int openDisk(char *filename, int nBytes);

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open disk (identified by ‘disk’) and copies the result into a local buffer (must be at least of BLOCKSIZE bytes). The bNum is a logical block number, which must be translated into a byte offset within the disk. The translation from logical to physical block is straightforward: bNum=0 is the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns 0. -1 or smaller is returned if disk is not available (hasn’t been opened) or any other failures. You must define your own error code system. */
int readBlock(int disk, int bNum, void *block);

/* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’ and writes the content of the buffer ‘block’ to that location. ‘block’ must be integral with BLOCKSIZE. The disk must be open. Just as in readBlock(), writeBlock() must translate the logical block bNum to the correct byte position in the file. On success, it returns 0. -1 or smaller is returned if disk is not available (i.e. hasn’t been opened) or any other failures. You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block);

/* closeDisk() takes a disk number ‘disk’ and makes the disk closed to further I/O; i.e. any subsequent reads or writes to a closed disk should return an error. Closing a disk should also close the underlying file, committing any buffered writes. */
void closeDisk(int disk);


/*	For libTinyFS.c	*/

#define MAGIC_NUMBER 0x45

enum blockCodes {
	SUPERBLOCK = 1,
	INODE = 2,
	FILE_EXTENT = 3,
	FREE = 4
};

/* The superblock contains three different pieces of information. 
 * 1) It specifies the “magic number,” used for detecting when the disk is not of 
 * 		the correct format.  For TinyFS, that number is 0x45, and it is to be found 
 *		exactly on the second byte of every block.  
 * 2) It contains the block number of 
 *		the root inode (for directory-based file systems). 
 * 3) It contains a pointer to the list of free blocks, or some other way to manage 
 		free blocks.
 */
typedef struct superBlock {
	int magicNumber;
	struct blockNode *freeBlocks;
} SuperBlock;

typedef struct inode {
	char name[9];
	int size;
	int seekOffset;					//	current file pointer
	int open;						//	whether file is open or not
	struct blockNode *dataBlocks;	//	data block linked list
} Inode;

typedef struct blockNode {
	int blockNum;
	struct blockNode *next;
} BlockNode;

typedef struct fileSystem {
	int size;
	int diskNum;
	char *filename;
	int mounted;
	SuperBlock superblock;
} FileSystem;

typedef struct fileSystemNode {
	FileSystem *fileSystem;
	struct fileSystemNode *next;
} FileSystemNode;

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error code. */
int tfs_mount(char *filename);
int tfs_unmount(void);

/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);
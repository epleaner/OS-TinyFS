#include "tinyFS.h"
#include "tinyFS_errno.h"

int setMagicNumbers(fileDescriptor diskNum, int blocks);
int writeSuperBlock(fileDescriptor diskNum, SuperBlock superblock);
int writeRootInode(fileDescriptor diskNum, Inode rootInode);
BlockNode *setupFreeBlockList();
void addFileSystem(FileSystem fileSystem);

FileSystemNode *head;

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’.
 * This function should use the emulated disk library to open the specified file, and
 * upon success, format the file to be mountable. This includes initializing all data
 * to 0x00, setting magic numbers, initializing and writing the superblock and inodes,
 * etc. Must return a specified success/error code.
 */
int tfs_mkfs(char *filename, int nBytes) {
	fileDescriptor diskNum;
	int blockCount;
	SuperBlock superblock;
	Inode rootInode;
	FileSystem fileSystem;
	void *freeBlockPtr;

	if((diskNum = openDisk(filename, nBytes)) < 0) {
		return MAKE_FS_ERROR;
	}

	blockCount = nBytes / BLOCKSIZE;

	//	this will zero out data and set 2nd byte to magic number for each block
	if(setMagicNumbers(diskNum, blockCount) < 0) {
		return MAKE_FS_ERROR;
	}

	//	set up free block linked list (total blocks - 1 (for superblock) - 1 (for root inode))
	freeBlockPtr = setupFreeBlockList(blockCount - 2);

	//	superblock contains magic number, root block num, and pointer to free blocks
	superblock = (SuperBlock) {
		MAGIC_NUMBER,
		freeBlockPtr
	};

	writeSuperBlock(diskNum, superblock);

	rootInode = (Inode) {
		"/",	//	root's name is slash
		0,		//	root has file size zero (it's a special inode)
		0,		//	seek offset is at beginning of file
		0,		//	not open yet
		NULL	//	root inode doesn't have any data blocks
	};

	writeRootInode(diskNum, rootInode);

	fileSystem = (FileSystem) {
		nBytes,		//	nBytes size
	 	diskNum,
		0,			//	not mounted
		superblock
	};

	addFileSystem(fileSystem);

	return MAKE_FS_SUCCESS;
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’.
 * tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount
 * operation, tfs_mount should verify the file system is the correct type. Only one file
 * system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently
 * mounted file system. Must return a specified success/error code. 
 */
int tfs_mount(char *filename) {
	return 0;
}

int tfs_unmount(void) {
	return 0;
}

/* Opens a file for reading and writing on the currently mounted file system. Creates a
 * dynamic resource table entry for the file, and returns a file descriptor (integer)
 * that can be used to reference this file while the filesystem is mounted. 
 */
fileDescriptor tfs_openFile(char *name) {
	return 0;
}

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
	return 0;
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to 
 *the file system. Sets the file pointer to 0 (the start of file) when done. Returns 
 * success/error codes. 
 */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size) {
	return 0;
}

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD) {
	return 0;
}

/* reads one byte from the file and copies it to buffer, using the current file pointer 
 * location and incrementing it by one upon success. If the file pointer is already at 
 * the end of the file then tfs_readByte() should return an error and not increment the 
 * file pointer. 
 */
int tfs_readByte(fileDescriptor FD, char *buffer) {
	return 0;
}

/* change the file pointer location to offset (absolute). Returns success/error codes. */
int tfs_seek(fileDescriptor FD, int offset) {
	return 0;
}

int setMagicNumbers(fileDescriptor diskNum, int blocks) {
	int block, result = 1;
	char *data = calloc(1, BLOCKSIZE);

	//	set first byte of data to free block code
	memset(&data[0], FREE, 1);

	//	set second byte of data to magic number
	memset(&data[1], MAGIC_NUMBER, 1);

	for(block = 0; block < blocks; block++) {
		if((result = writeBlock(diskNum, block, data)) < 0) {
			return result;
		}
	}

	return result;
}

int writeSuperBlock(fileDescriptor diskNum, SuperBlock superblock) {
	char *data = calloc(1, BLOCKSIZE);
	
	//	set first byte of data to superblock block code
	memset(&data[0], SUPERBLOCK, 1);

	//	set second byte of data to magic number
	memset(&data[1], MAGIC_NUMBER, 1);

	//	copy over superblock data
	memcpy(&data[2], &(superblock), sizeof(superblock));

	return writeBlock(diskNum, 0, data);
}

int writeRootInode(fileDescriptor diskNum, Inode rootInode) {
	char *data = calloc(1, BLOCKSIZE);
	int result;
	
	//	set first byte of data to inode block code
	memset(&data[0], INODE, 1);

	//	set second byte of data to magic number
	memset(&data[1], MAGIC_NUMBER, 1);

	//	copy over inode data
	memcpy(&data[2], &(rootInode), sizeof(rootInode));

	//	write root inode and return status
	return writeBlock(diskNum, 1, data);
}

BlockNode *setupFreeBlockList(int freeBlockCount) {
	int block;

	BlockNode *curr, *head = malloc(sizeof(BlockNode));

	//	first two blocks are used for superblock and root inode, so start at 2
	head->blockNum = 2;
	head->next = NULL;

	curr = head;

	for(block = 3; block < freeBlockCount; block++) {
		printf("added block %d as free block\n", block);
		curr->next = malloc(sizeof(BlockNode));
		curr->next->blockNum = block;
		curr->next->next = NULL;

		curr = curr->next;
	}

	return head;
}

void addFileSystem(FileSystem fileSystem) {
	FileSystemNode *curr;

	FileSystem *fileSystemPtr = malloc(sizeof(FileSystem));
	memcpy(fileSystemPtr, &fileSystem, sizeof(FileSystem));
	printf("testing file system ptr: size is %d\n", fileSystemPtr->size);
	
	//	create head if it is null
	if(head == NULL) {
		head = malloc(sizeof(FileSystemNode));
		*head = (FileSystemNode) {
			fileSystemPtr,
			NULL
		};
	}
	else {
		curr = head;
		
		while(curr->next != NULL) curr = curr->next;
				
		curr->next = malloc(sizeof(FileSystemNode));
		*(curr->next) = (FileSystemNode) {
			fileSystemPtr,
			NULL
		};
	}
}
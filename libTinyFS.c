#include "tinyFS.h"
#include "tinyFS_errno.h"

int setMagicNumbers(fileDescriptor diskNum, int blocks);
int writeSuperBlock(fileDescriptor diskNum, SuperBlock superblock);
int writeRootInode(fileDescriptor diskNum, Inode rootInode);
BlockNode *setupFreeBlockList();
void addFileSystem(FileSystem fileSystem);
FileSystem *findFileSystem(char *filename);
int verifyFileSystem(FileSystem fileSystem);
int findFile(FileSystem fileSystem, char *filename);
int getFreeBlock(FileSystem *fileSystemPtr);
int addInode(fileDescriptor diskNum, Inode inode, int blockNum);
int addDynamicResource(FileSystem *fileSystemPtr, DynamicResource dynamicResource);
int removeDynamicResource(FileSystem *fileSystem, fileDescriptor FD);
int tfs_rename(char *oldName, char *newName);
int tfs_readdir();
int renameInode(FileSystem *fileSystemPtr, int blockNum, char *newName);
int renameDynamicResource(FileSystem *fileSystemPtr, int inodeBlockNum, char *newName);
DynamicResource *findResource(DynamicResourceNode *rsrcTable, int fd);

FileSystemNode *fsHead = NULL;

char *mountedFsName = NULL;

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

	//	superblock contains magic number and pointer to free blocks
	superblock = (SuperBlock) {
		MAGIC_NUMBER,
		freeBlockPtr
	};

	writeSuperBlock(diskNum, superblock);

	rootInode = (Inode) {
		"/",	//	root's name is slash
		0,		//	root has file size zero (it's a special inode)
		NULL	//	root inode doesn't have any data blocks
	};

	writeRootInode(diskNum, rootInode);

	fileSystem = (FileSystem) {
		nBytes,		//	nBytes size
	 	diskNum,
	 	0,			//	zero files open
		filename,
		0,			//	not mounted
		superblock,
		NULL 		//	has empty dynamic resource table
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
	FileSystem *fileSystemPtr;

	//	unmount currently mounted file system
	tfs_unmount();

	//	find file system by name
	fileSystemPtr = findFileSystem(filename);

	//	verify
	if(verifyFileSystem(*fileSystemPtr) < 0) {
		return FS_VERIFY_FAILURE;
	}

	//	set mounted to true, and store mounted file name for unmounting
	fileSystemPtr->mounted = 1;

	mountedFsName = filename;

	return MOUNT_FS_SUCCESS;
}

int tfs_unmount() {
	FileSystem *fileSystemPtr;

	if(mountedFsName == NULL) {
		return UNMOUNT_FS_SUCCESS;
	}
	
	//	find file system by name
	fileSystemPtr = findFileSystem(mountedFsName);

	//	set mounted to false, and clear mounted FS name
	fileSystemPtr->mounted = 0;
	mountedFsName = NULL;

	return UNMOUNT_FS_SUCCESS;
}

/* Opens a file for reading and writing on the currently mounted file system. Creates a
 * dynamic resource table entry for the file, and returns a file descriptor (integer)
 * that can be used to reference this file while the filesystem is mounted. 
 */
fileDescriptor tfs_openFile(char *name) {
	FileSystem *fileSystemPtr;
	DynamicResource dynamicResource;
	Inode inode;
	int inodeBlockNum, FD;

	if(strlen(name) > 8) {
		printf("Filename must be 8 or less characters.\n");
		return OPEN_FILE_FAILURE;
	}

	fileSystemPtr = findFileSystem(mountedFsName);

	if((inodeBlockNum = findFile(*fileSystemPtr, name)) < 0) {

		//	file doesn't exist so create inode
		inodeBlockNum = getFreeBlock(fileSystemPtr);

		//	error getting free block
		if(inodeBlockNum < 0) {
			return OPEN_FILE_FAILURE;
		}

		//	create new inode and insert it
		inode = (Inode) {
			name,
			0,
			NULL
		};

		addInode(fileSystemPtr->diskNum, inode, inodeBlockNum);
	}

	FD = fileSystemPtr->openCount++;
	
	dynamicResource = (DynamicResource) {
		name,
		0,
		FD,
		inodeBlockNum
	};

	if(addDynamicResource(fileSystemPtr, dynamicResource) < 0) {
		return OPEN_FILE_FAILURE;
	}

	return FD;
}

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
	FileSystem *fileSystemPtr;
	DynamicResource *dynamicResourcePtr;

	fileSystemPtr = findFileSystem(mountedFsName);

	return removeDynamicResource(fileSystemPtr, FD);
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to 
 * the file system. Sets the file pointer to 0 (the start of file) when done. Returns 
 * success/error codes. 
 */
 int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
 	FileSystem *fileSystemPtr;
 	DynamicResource *dynamicResourcePtr;
 	Inode *inodePtr;
 	BlockNode *currBlock;
 	char inodeData[BLOCKSIZE], data[BLOCKSIZE];
 	int blockNum, written = 0, writeSize, blockOffset;

 	fileSystemPtr = findFileSystem(mountedFsName);
 	dynamicResourcePtr = findResource(fileSystemPtr->dynamicResourceTable, FD);

 	//	dynamic resource doesnt exist, which means file isn't open
 	if(dynamicResourcePtr == NULL) {
 		printf("File %d is not open to write.\n", FD);
 		return WRITE_FILE_FAILURE;
 	}

 	//	read in inode block
 	if(readBlock(fileSystemPtr->diskNum, dynamicResourcePtr->inodeBlockNum, inodeData) < 0) {
 		return WRITE_FILE_FAILURE;
 	}

 	inodePtr = (Inode *) &inodeData[2];

 	if(inodePtr->size == 0) {
 		printf("file is empty\n");
 		blockNum = getFreeBlock(fileSystemPtr);

 		inodePtr->dataBlocks = malloc(sizeof(BlockNode));
 		*inodePtr->dataBlocks = (BlockNode) {
 			blockNum,
 			NULL
 		};

 		currBlock = inodePtr->dataBlocks;
 	}
 	else {
 		blockNum = dynamicResourcePtr->seekOffset / BLOCKSIZE;
 		printf("file has stuff in it, start writing at block %d\n", blockNum);

 		currBlock = inodePtr->dataBlocks;

 		int blockCount;
 		for(blockCount = 0; blockCount < blockNum; blockCount++) {
 			currBlock = currBlock->next;
 		}

 		printf("made it to block %d\n", currBlock->blockNum);
 	}

 	printf("artifically seeking 100 ahead\n");
 	dynamicResourcePtr->seekOffset = 100;

 	while(written < size) {
 		//	read in data block
	 	if(readBlock(fileSystemPtr->diskNum, blockNum, data) < 0) {
	 		return WRITE_FILE_FAILURE;
	 	}

	 	//	set block to file extent
		memset(&data[0], FILE_EXTENT, 1);

		//	get offset into block (minus two to account for reserved first two bytes)
		blockOffset = dynamicResourcePtr->seekOffset % (BLOCKSIZE - 2);

		//	how much to write this time, minus two for first two bytes
		writeSize = BLOCKSIZE - blockOffset - 2;

		//	adjust for when there is not much data left to write
		if(size - written < writeSize) writeSize = size - written;

		printf("will write %d from buffer at block offset %d into block num %d\n", writeSize, blockOffset + 2, currBlock->blockNum);

		//	write buffer into data that will go into block
		memcpy(&data[2 + blockOffset], buffer, writeSize);

		//	write data into block
		if(writeBlock(fileSystemPtr->diskNum, currBlock->blockNum, data) < 0) {
	 		return WRITE_FILE_FAILURE;
	 	}

		written += writeSize;
		buffer += writeSize;
		dynamicResourcePtr->seekOffset += writeSize;

		if(size - written > 0) {
			printf("theres more data, so get another block\n");

			if(currBlock->next == NULL) {
				printf("need to get another block from free\n");
				blockNum = getFreeBlock(fileSystemPtr);
				printf("next free block gotten is %d\n", blockNum);

 				currBlock->next = malloc(sizeof(BlockNode));
 				*currBlock->next = (BlockNode) {
 					blockNum,
 					NULL
 				};

 				currBlock = currBlock->next;
			}
			else {
				currBlock = currBlock->next;
				printf("moved to block %d\n", currBlock->blockNum);
			}
		}
		printf("have written in total %d bytes\n", written);
 	}

 	dynamicResourcePtr->seekOffset = 0;

 	return WRITE_FILE_SUCCESS;
 }

DynamicResource *findResource(DynamicResourceNode *rsrcTable, int fd) {
	DynamicResourceNode *tmpHead = rsrcTable;

	while (tmpHead != NULL) {
		if (tmpHead->dynamicResource->FD == fd) {
			return (tmpHead->dynamicResource);
		}
		tmpHead = tmpHead->next;
	}

	return NULL;
}

/* deletes a file and marks its blocks as free on disk.
 * You can think of deleteFile as equivalent to ftruncate(fd, 0). 
 * So it should be able to be written again.
 * So then free its blocks but don't delete its inode.
 */
int tfs_deleteFile(fileDescriptor FD) {
	FileSystem *fileSystemPtr = findFileSystem(mountedFsName);
	DynamicResource *dynamicResourcePtr;
        dynamicResourcePtr = findResource(fileSystemPtr->dynamicResourceTable, FD);
	Inode *inodePtr;
	BlockNode *tmpPtr;
	char buf[BLOCKSIZE];
	char *clearBuf = calloc(1, BLOCKSIZE);
	int offset;

	if (dynamicResourcePtr == NULL) {
		return DELETE_FILE_FAILURE;
	}

	if (readBlock(fileSystemPtr->diskNum, dynamicResourcePtr->inodeBlockNum, buf) < 0) {
		return DELETE_FILE_FAILURE;
	}

	inodePtr = (Inode *)&buf[2];

	offset = dynamicResourcePtr->seekOffset / BLOCKSIZE;
	tmpPtr = inodePtr->dataBlocks;

	memset(&clearBuf[0], FREE, 1);
	memset(&clearBuf[1], MAGIC_NUMBER, 1);

	writeBlock(fileSystemPtr->diskNum, tmpPtr->blockNum, clearBuf);

	while (offset > 0) {
		tmpPtr = tmpPtr->next;
		writeBlock(fileSystemPtr->diskNum, tmpPtr->blockNum, clearBuf);
		offset--;
	}

	inodePtr->size = 0;
	return DELETE_FILE_SUCCESS;
}

/* reads one byte from the file and copies it to buffer, using the current file pointer 
 * location and incrementing it by one upon success. If the file pointer is already at 
 * the end of the file then tfs_readByte() should return an error and not increment the 
 * file pointer. 
 */
int tfs_readByte(fileDescriptor FD, char *buffer) {
	FileSystem *fileSystemPtr = findFileSystem(mountedFsName);
	DynamicResource *dynamicResourcePtr = findResource(fileSystemPtr->dynamicResourceTable, FD);
	Inode *inodePtr;
	BlockNode *tmpPtr;
	char buf[BLOCKSIZE];
	int offset;

	if (dynamicResourcePtr == NULL) {
		return READ_BYTE_FAILURE;
	}

	if (readBlock(fileSystemPtr->diskNum, dynamicResourcePtr->inodeBlockNum, buf) < 0) {
		return READ_BYTE_FAILURE;
	}

	inodePtr = (Inode *)&buf[2];

	if (dynamicResourcePtr->seekOffset == inodePtr->size) {
		return READ_BYTE_FAILURE;
	}

	offset = dynamicResourcePtr->seekOffset / BLOCKSIZE;
	tmpPtr = inodePtr->dataBlocks;

	while (offset > 0) {
		tmpPtr = tmpPtr->next;
		offset--;
	}

	if (readBlock(fileSystemPtr->diskNum, tmpPtr->blockNum, buf) < 0) {
		return READ_BYTE_FAILURE;
	}

	offset = dynamicResourcePtr->seekOffset % BLOCKSIZE;
	*buffer = buf[offset];
	dynamicResourcePtr->seekOffset++;

	return READ_BYTE_SUCCESS;
}

/* change the file pointer location to offset (absolute). Returns success/error codes. */
int tfs_seek(fileDescriptor FD, int offset) {
	FileSystem *fileSystemPtr = findFileSystem(mountedFsName);
	DynamicResource *dynamicResourcePtr = findResource(fileSystemPtr->dynamicResourceTable, FD);
	Inode *inodePtr;
	BlockNode *tmpPtr;
        char buf[BLOCKSIZE];

	if (dynamicResourcePtr == NULL) {
		return SEEK_FILE_FAILURE;
	}
	if (readBlock(fileSystemPtr->diskNum, dynamicResourcePtr->inodeBlockNum, buf) < 0) {
		return SEEK_FILE_FAILURE;
	}

	inodePtr = (Inode *)&buf[2];
	if (offset > inodePtr->size) {
		return SEEK_FILE_FAILURE;
	}

	dynamicResourcePtr->seekOffset = offset;

	return SEEK_FILE_SUCCESS;
}

/* File listing and renaming */

/* renames a file.  New name should be passed in. */
int tfs_rename(char *oldName, char *newName) {
	int result;
	FileSystem *fileSystemPtr;
	int inodeBlockNum;

	if(strlen(newName) > 8) {
		printf("Filename length must be 8 characters or less\n");
		return RENAME_FILE_FAILURE;
	}

	fileSystemPtr = findFileSystem(mountedFsName);

	inodeBlockNum = findFile(*fileSystemPtr, oldName);

	if(inodeBlockNum < 0) {
		printf("File %s not found in current mounted file system\n", oldName);
		return RENAME_FILE_FAILURE;
	}

	if((result = renameInode(fileSystemPtr, inodeBlockNum, newName)) < 0) {
		return RENAME_FILE_FAILURE;
	}

	return renameDynamicResource(fileSystemPtr, inodeBlockNum, newName);
}

/* lists all the files and directories on the disk */
int tfs_readdir() {
	char data[BLOCKSIZE];
	FileSystem *fileSystemPtr;
	Inode *inodePtr;
	int block, blocks, result;

	fileSystemPtr = findFileSystem(mountedFsName);

	blocks = fileSystemPtr->size / BLOCKSIZE;

	printf("Listing of all files in file system %s\n", fileSystemPtr->filename);

	for(block = 0; block < blocks; block++) {
		if((result = readBlock(fileSystemPtr->diskNum, block, data)) < 0) {
			return result;		//	means error reading block
		}

		if(data[0] == INODE) {
			inodePtr = (Inode *)&data[2];

			printf("File: %s\n", inodePtr->name);
		}
	}

	return 1;
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
	
	//	create head if it is null
	if(fsHead == NULL) {
		fsHead = malloc(sizeof(FileSystemNode));
		*fsHead = (FileSystemNode) {
			fileSystemPtr,
			NULL
		};
	}
	else {
		curr = fsHead;
		
		while(curr->next != NULL) curr = curr->next;
				
		curr->next = malloc(sizeof(FileSystemNode));
		*(curr->next) = (FileSystemNode) {
			fileSystemPtr,
			NULL
		};
	}
}

FileSystem *findFileSystem(char *filename) {
	FileSystemNode *curr = fsHead;
	if(curr == NULL) {
		printf("No filesystems made yet\n");
		exit(-1);
	}
	
	else {
		if(strcmp(curr->fileSystem->filename, filename) == 0) return curr->fileSystem;
		
		while(curr->next != NULL) {
			curr = curr->next;

			if(strcmp(curr->fileSystem->filename, filename) == 0) return curr->fileSystem;
		}
		
		printf("No file systems found with filename %s\n", filename);
		exit(-1);
	}
}

int verifyFileSystem(FileSystem fileSystem) {
	int block, blocks, result;
	char *data = malloc(BLOCKSIZE);

	blocks = fileSystem.size / BLOCKSIZE;

	for(block = 0; block < blocks; block++) {
		if((result = readBlock(fileSystem.diskNum, block, data)) < 0) {
			return result;		//	means error reading block
		}

		if(data[1] != MAGIC_NUMBER) {
			return FS_VERIFY_FAILURE;
		}
	}

	return 1;
}

int findFile(FileSystem fileSystem, char *filename) {
	int block, blocks, result;
	char *data = malloc(BLOCKSIZE);
	Inode *inodePtr;

	blocks = fileSystem.size / BLOCKSIZE;

	for(block = 0; block < blocks; block++) {
		if((result = readBlock(fileSystem.diskNum, block, data)) < 0) {
			return result;		//	means error reading block
		}

		if(data[0] == INODE) {
			inodePtr = (Inode *)&data[2];

			if(strcmp(inodePtr->name, filename) == 0) {
				return block;
			}
		}
	}

	return -1;
}

int getFreeBlock(FileSystem *fileSystemPtr) {
	int freeBlockNum;

	if(fileSystemPtr->superblock.freeBlocks == NULL) {
		return -1;
	}

	freeBlockNum = fileSystemPtr->superblock.freeBlocks->blockNum;
	fileSystemPtr->superblock.freeBlocks = fileSystemPtr->superblock.freeBlocks->next;

	return freeBlockNum;
}

int addInode(fileDescriptor diskNum, Inode inode, int blockNum) {
	char *data = calloc(1, BLOCKSIZE);
	int result;
	
	//	set first byte of data to inode block code
	memset(&data[0], INODE, 1);

	//	set second byte of data to magic number
	memset(&data[1], MAGIC_NUMBER, 1);

	//	copy over inode data
	memcpy(&data[2], &(inode), sizeof(inode));

	//	write inode and return status
	return writeBlock(diskNum, blockNum, data);
}

int addDynamicResource(FileSystem *fileSystemPtr, DynamicResource dynamicResource) {
	DynamicResourceNode *curr = fileSystemPtr->dynamicResourceTable;

	DynamicResource *dynamicResourcePtr = malloc(sizeof(DynamicResource));
	memcpy(dynamicResourcePtr, &dynamicResource, sizeof(DynamicResource));

	//	create head if it is null
	if(curr == NULL) {
		curr = malloc(sizeof(DynamicResourceNode));
		*curr = (DynamicResourceNode) {
			dynamicResourcePtr,
			NULL
		};

		fileSystemPtr->dynamicResourceTable = curr;
	}
	else {
		while(curr->next != NULL) curr = curr->next;
				
		curr->next = malloc(sizeof(DynamicResourceNode));
		*(curr->next) = (DynamicResourceNode) {
			dynamicResourcePtr,
			NULL
		};
	}
}

int removeDynamicResource(FileSystem *fileSystem, fileDescriptor FD) {
	DynamicResourceNode *temp, *curr;

	curr = fileSystem->dynamicResourceTable;

	if(curr == NULL) {
		printf("No dynamic resources for this filesystem made yet\n");
		return -1;
	}
	
	else {
		if(curr->dynamicResource->FD == FD) {
			temp = curr;
			curr = curr->next;
			free(temp);

			return 1;
		}

		while(curr->next != NULL) {
			curr = curr->next;

			if(curr->dynamicResource->FD == FD) {
				temp = curr;
				curr = curr->next;
				free(temp);

				return 1;
			}
		}
		
		printf("No dynamic resources found with FD %d\n", FD);
		return -1;
	}
}

int renameInode(FileSystem *fileSystemPtr, int blockNum, char *newName) {
	int result;
	char data[BLOCKSIZE];
	Inode *inodePtr;

	if((result = readBlock(fileSystemPtr->diskNum, blockNum, data)) < 0) {
		return result;		//	means error reading block
	}

	inodePtr = (Inode *)&data[2];

	strcpy(inodePtr->name, newName);

	memcpy(&data[2], inodePtr, sizeof(Inode));

	return writeBlock(fileSystemPtr->diskNum, blockNum, data);
}

int renameDynamicResource(FileSystem *fileSystemPtr, int inodeBlockNum, char *newName) {
	DynamicResourceNode *curr = fileSystemPtr->dynamicResourceTable;

	if(fileSystemPtr->openCount == 0) {
		printf("No open files to rename!\n");
		return 1;
	}

	while (curr != NULL) {
		if(curr->dynamicResource->inodeBlockNum == inodeBlockNum) {
			printf("Renaming dynamic resource for inode %d to %s\n", inodeBlockNum, newName);
			strcpy(curr->dynamicResource->name, newName);

			return 1;
		}

		curr = curr->next;
	}

	printf("No files with inode %d open\n", inodeBlockNum);
	return -1;
}

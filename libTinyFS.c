#include "tinyFS.h"
#include "tinyFS_errno.h"

int setMagicNumbers(fileDescriptor diskNum, int blocks);
int writeSuperBlock(fileDescriptor diskNum, SuperBlock superblock);
int writeRootInode(fileDescriptor diskNum, Inode rootInode);
BlockNode *setupFreeBlockList();
void addFileSystem(FileSystem fileSystem);
FileSystem *findFileSystem(char *filename);
int verifyFileSystem(FileSystem fileSystem);

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
		filename,
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
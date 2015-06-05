#include "tinyFS.h"
#include "tinyFS_errno.h"

void libDiskTest();
void libTinyFSTest();

void libTinyFSCoreDemo();
void fileRenameDemo();
// void timeStampDemo();
// void permissionsDemo();

int main(int argc, char *argv[]) {
	libTinyFSCoreDemo();
	fileRenameDemo();
	// timeStampDemo();
	// permissionsDemo();

	// libDiskTest();
	// libTinyFSTest();
	
	return 0;
}

void libTinyFSCoreDemo() {
	int file1, file2;
	int i, largeWriteSize = BLOCKSIZE * 3 + 20;
	char largeWrite[largeWriteSize];
	char readByteBuffer;

	for(i = 0; i < largeWriteSize; i++) {
		if(i % 2) largeWrite[i] = 'E';
		else largeWrite[i] = 'D';
	}

	printf("Tiny FS Core Functionaly Demonstration\n\n");

	printf("Making a filesystem from a file that exists... %d\n",
		tfs_mkfs("testing/test1.bin", BLOCKSIZE * 20));

	printf("Making a filesystem from a file does not exist... %d\n",
		tfs_mkfs("testing/does_not_exist.bin", BLOCKSIZE * 10));

	printf("Making a read-only filesystem from a file that exists... %d\n",
		tfs_mkfs("testing/test1.bin", 0));

	printf("Throws an error if file does not exist and size is zero... %d\n", 
		tfs_mkfs("testing/also_does_not_exist.bin", 0));

	printf("Throws an error if filesystem size is not integral of BLOCKSIZE... %d\n", 
		tfs_mkfs("testing/test1.bin", BLOCKSIZE + 1));

	printf("Mounting a filesystem that has been made... %d\n",
		tfs_mount("testing/test1.bin"));

	printf("Mounting another filesystem that has been made... %d\n",
		tfs_mount("testing/does_not_exist.bin"));

	printf("Throws an error if trying to mount a filesystem that hasn't been made... %d\n",
		tfs_mount("testing/also_does_not_exist.bin"));

	tfs_mount("testing/test1.bin");

	printf("Unmounting the filesystem... %d\n",
		tfs_unmount());

	printf("Error unmounting when nothing is mounted... %d\n",
		tfs_unmount());

	tfs_mount("testing/test1.bin");

	printf("Opening a new file on current filesystem... FD: %d\n",
		(file1 = tfs_openFile("new file")));

	printf("Opening another new file on current filesystem... FD: %d\n",
		(file2 = tfs_openFile("another")));

	printf("Throws an error if trying to open a filename that is too long.. %d\n",
		tfs_openFile("this name is too long"));

	printf("Closing a file... %d\n",
		tfs_closeFile(file2));

	printf("Throws an error if trying to close a file twice... %d\n",
		tfs_closeFile(file2));

	printf("Writing small amount (< BLOCKSIZE) to an open file... %d\n",
		tfs_writeFile(file1, "writing less than BLOCKSIZE", sizeof("writing less than BLOCKSIZE")));

	printf("Writing large amount (> BLOCKSIZE) to an open file... %d\n",
		tfs_writeFile(file1, largeWrite, largeWriteSize));

	printf("Throws an error if trying to write to a closed file... %d\n",
		tfs_writeFile(file2, "write to closed", sizeof("write to closed")));

	printf("Seeking in a file... %d\n",
		tfs_seek(file1, 50));

	printf("Reading a byte from a file... %d\n",
		tfs_readByte(file1, &readByteBuffer));

	printf("Byte read (as char): %c\n", readByteBuffer);

	printf("Reading another byte from a file... %d\n",
		tfs_readByte(file1, &readByteBuffer));

	printf("Byte read (as char): %c\n", readByteBuffer);

	printf("Seeking to end of file... %d\n",
		tfs_seek(file1, largeWriteSize));

	printf("Reading last byte of file... %d\n",
		tfs_readByte(file1, &readByteBuffer));

	printf("Throws an error if reading past end of file... %d\n",
		tfs_readByte(file1, &readByteBuffer));

	printf("Throws an error if seeking past end of file... %d\n",
		tfs_seek(file1, largeWriteSize + 2));
}

void fileRenameDemo() {
	
}

void libTinyFSTest() {
	char readByte;
	int index;
	char *big = calloc(1, (BLOCKSIZE * 2) + 12);
	big[(BLOCKSIZE * 2) + 11] = 'X';
	int file1;
	tfs_mkfs("testing/test1.bin", 4096);
	tfs_mount("testing/test1.bin");
	file1 = tfs_openFile("MEOWOWOW");
	tfs_readFileInfo(file1);
	tfs_writeFile(file1, big, (BLOCKSIZE * 2) + 12);
	tfs_makeRO("MEOWOWOW");
	tfs_writeFile(file1, big, (BLOCKSIZE * 2) + 12);
	tfs_readFileInfo(file1);
	tfs_seek(file1, 0);
	tfs_writeByte(file1, 123);
	tfs_writeByte(file1, 9);
	tfs_seek(file1, 0);
	tfs_readByte(file1, &readByte);
	tfs_seek(file1, 123);
	tfs_readFileInfo(file1);
	tfs_seek(file1, (BLOCKSIZE * 2) + 13);
	tfs_seek(file1, (BLOCKSIZE * 2) + 11);
	tfs_makeRW("MEOWOWOW");
	tfs_writeByte(file1, 123);
	//tfs_makeRO("MEOWOWOW");
	tfs_writeFile(file1, big, (BLOCKSIZE * 2) + 12);
	//tfs_deleteFile(file1);
	tfs_readFileInfo(file1);

	tfs_closeFile(file1);
}

void libDiskTest() {
	int disk1 = openDisk("testing/disk1.dat", 4096);
	int disk2 = openDisk("testing/disk2.dat", 256);
	
	char *buffer[256];
	
	readBlock(disk1, 0, buffer);
	
	writeBlock(disk2, 0, buffer);
	
	closeDisk(disk1);
	closeDisk(disk2);
}
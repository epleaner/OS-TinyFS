#include "tinyFS.h"
#include "tinyFS_errno.h"

void libDiskTest();
void libTinyFSTest();

int main(int argc, char *argv[]) {
	// libDiskTest();
	libTinyFSTest();
	
	return 0;
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
	tfs_writeFile(file1, big, (BLOCKSIZE * 2) + 12);
	tfs_seek(file1, 123);
	tfs_seek(file1, (BLOCKSIZE * 2) + 13);
	tfs_seek(file1, (BLOCKSIZE * 2) + 11);
	tfs_readByte(file1, &readByte);
	printf("read in %c\n", readByte);
	//tfs_deleteFile(file1);

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
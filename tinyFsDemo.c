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
	char big[(BLOCKSIZE * 2) + 12];
	int file1;
	tfs_mkfs("testing/test1.bin", 4096);
	tfs_mount("testing/test1.bin");
	file1 = tfs_openFile("MEOWOWOW");
	tfs_writeFile(file1, big, (BLOCKSIZE * 2) + 12);
	tfs_seek(file1, 123);
	tfs_seek(file1, (BLOCKSIZE * 2) + 13);
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
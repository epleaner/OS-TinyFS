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
	int file1;
	int file2;
	tfs_mkfs("testing/test1.bin", 4096);
	tfs_mount("testing/test1.bin");
	file1 = tfs_openFile("MEOWOWOW");
	file2 = tfs_openFile("hey");
	// tfs_closeFile(file1);
	tfs_closeFile(file2);
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
#include "tinyFS.h"
#include "tinyFS_errno.h"

void libDiskTest();

int main(int argc, char *argv[]) {
	libDiskTest();
	
	return 0;
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
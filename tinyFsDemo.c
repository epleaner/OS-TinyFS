#include "tinyFS.h"
#include "tinyFS_errno.h"

int main(int argc, char *argv[]) {
	int disk1 = openDisk("testing/disk1.dat", 4096);
	int disk2 = openDisk("testing/disk2.dat", 256);
	
	char *buffer[256];
	
	readBlock(disk1, 0, buffer);
	
	writeBlock(disk2, 0, buffer);
	
	closeDisk(disk1);
	closeDisk(disk2);
	
	return 0;
}
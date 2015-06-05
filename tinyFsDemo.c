#include "tinyFS.h"
#include "tinyFS_errno.h"

void libDiskTest();
void libTinyFSTest();

void libTinyFSCoreDemo();
void fileRenameDemo();
void timeStampDemo();
void permissionsDemo();

int main(int argc, char *argv[]) {
	libTinyFSCoreDemo();
	fileRenameDemo();
	permissionsDemo();
	timeStampDemo();
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
	printf("\nFile Renaming and List Demonstration\n\n");

	tfs_mkfs("testing/fileRename.bin", BLOCKSIZE * 10);

	tfs_mount("testing/fileRename.bin");
	
	tfs_openFile("File 1");
	tfs_openFile("File 2");
	tfs_openFile("File 3");

	printf("Listing files in filesystem:\n");
	tfs_readdir();

	printf("\nRenaming file 2... %d\n",
		tfs_rename("File 2", "Renamed"));

	printf("Listing files in filesystem:\n");
	tfs_readdir();

	printf("\nThrows an error if using too long of a name... %d\n",
		tfs_rename("File 1", "this name is too long"));

	printf("Throws an error if file does not exist... %d\n",
		tfs_rename("DNE", "error"));

	printf("Throws an error if trying to rename root... %d\n",
		tfs_rename("/", "error"));
}

void permissionsDemo() {
	int file1;

	printf("\nFile RO/RW Permissions Demonstration\n\n");

	tfs_mkfs("testing/filePermissions.bin", BLOCKSIZE * 10);

	tfs_mount("testing/filePermissions.bin");

	file1 = tfs_openFile("File 1");

	printf("Making file read-only... %d\n",
		tfs_makeRO("File 1"));

	printf("Throws an error when writing to RO file... %d\n",
		tfs_writeFile(file1, "should not be written", sizeof("should not be written")));

	printf("Throws an error when deleting an RO file... %d\n",
		tfs_deleteFile(file1));

	printf("Throws an error when writing a byte an RO file... %d\n",
		tfs_writeByte(file1, 88));

	printf("Making file read-write... %d\n",
		tfs_makeRW("File 1"));

	printf("Writing to RW file... %d\n",
		tfs_writeFile(file1, "should be written", sizeof("should be written")));

	printf("Writing a byte to file... %d\n",
		tfs_writeByte(file1, 88));

	printf("Deleting a file... %d\n",
		tfs_deleteFile(file1));
}

void timeStampDemo() {
	int file1, file2;
	char readByteBuffer;

	printf("\nFile TimeStamp Demonstration\n\n");

	tfs_mkfs("testing/filePermissions.bin", BLOCKSIZE * 10);

	tfs_mount("testing/filePermissions.bin");

	file1 = tfs_openFile("File 1");

	tfs_readFileInfo(file1);

	printf("\nSleeping for 2 seconds...\n\n");

	sleep(2);

	file2 = tfs_openFile("File 2");

	tfs_readFileInfo(file2);

	printf("\nSleeping for 2 seconds...\n\n");

	sleep(2);

	printf("Updating file 1 modify time while changing permissions...\n\n");
	tfs_makeRW("File 1");

	tfs_readFileInfo(file1);
	

	printf("\nSleeping for 2 seconds...\n\n");
	sleep(2);

	tfs_writeFile(file2, "write to closed", sizeof("write to closed"));
	tfs_seek(file2, 0);

	printf("Updating file 2 modify time when writing to file...\n\n");
	tfs_readFileInfo(file2);

	printf("\nSleeping for 2 seconds...\n\n");
	sleep(2);
	tfs_readByte(file2, &readByteBuffer);

	printf("Updating file 2 access time when reading a byte...\n\n");
	tfs_readFileInfo(file2);

	printf("\nSleeping for 2 seconds...\n\n");
	sleep(2);
	tfs_rename("File 1", "File 1A");

	printf("Updating file 1 modify time when changing file name...\n\n");
	tfs_readFileInfo(file1);

	printf("\nSleeping for 2 seconds...\n\n");
	sleep(2);

	tfs_deleteFile(file2);

	printf("Updating file 2 modify time when deleting file...\n\n");
	tfs_readFileInfo(file2);
}
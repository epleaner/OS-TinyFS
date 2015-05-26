all: libDisk libTinyFS tinyFsDemo

libDisk: libDisk.c tinyFS.h tinyFS_errno.h
	gcc -o libDisk libDisk.c tinyFS.h tinyFS_errno.h
libTinyFS: libTinyFS.c tinyFS.h tinyFS_errno.h
	gcc -o libTinyFS libTinyFS.c tinyFS.h tinyFS_errno.h
tinyFsDemo: tinyFsDemo.c libDisk.c libTinyFS.c tinyFS.h tinyFS_errno.h
	gcc -o tinyFsDemo tinyFsDemo.c tinyFS.h tinyFS_errno.h
clean:
	rm *.o libDisk libTinyFS tinyFsDemo


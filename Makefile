tinyFsDemo: tinyFsDemo.c libDisk.c libTinyFS.c tinyFS.h tinyFS_errno.h
	gcc -o tinyFsDemo tinyFsDemo.c libDisk.c libTinyFS.c tinyFS.h tinyFS_errno.h
clean:
	rm *.o libDisk libTinyFS tinyFsDemo


default: sfs.o disk_emu.o sfs_ftest.o
	gcc -o SFS sfs.o disk_emu.o sfs_ftest.o -DHAVE_PTHREAD_RWLOCK=1 -lslack

sfs.o: sfs.c
	gcc -c sfs.c -DHAVE_PTHREAD_RWLOCK=1 -lslack

disk_emu.o: disk_emu.c
	gcc -c disk_emu.c -DHAVE_PTHREAD_RWLOCK=1 -lslack

sfs_ftest.o: sfs_ftest.c
	gcc -c sfs_ftest.c -DHAVE_PTHREAD_RWLOCK=1 -lslack

clean: 
	rm -rf *.o SFS

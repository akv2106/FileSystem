INC_DIR = ../include

default: sfs_api.c disk_emu.c
	gcc -o SFS sfs_api.c disk_emu.c -I$(INC_DIR) -DHAVE_PTHREAD_RWLOCK=1 -lslack
sfs_api: sfs_api.h
	gcc -c sfs_api.c -I$(INC_DIR) -DHAVE_PTHREAD_RWLOCK=1 -lslack 
disk_emu: disk_emu.h
	gcc -c disk_emu.c -I$(INC_DIR) -DHAVE_PTHREAD_RWLOCK=1 -lslack 

/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Shivan Kaul Sahib
 Version     : 1.0
 Description : FileSystem, assignment 4, COMP 310/ECSE 427
 ============================================================================
 */

#include <stdio.h>
#include "sfs_api.h"
#include <stdlib.h>
#include <unistd.h>
#include <slack/std.h>
#include <slack/map.h>

//#define MAX_FILES 20 // this will be defined by the N (total blocks in CCDisk) - (# of blocks allocated for
// root directory and FAT) - 2 (the blocks for superblock and free block list)

/*
 * In memory data structures
 */

// Directory Table, implemented as a map between filenames and FAT indexes
Map *directory_table;

int mksfs(int fresh) {
	if (!fresh) { // re-open from disk
		// the first file written to the disk should be
		init_disk()
	}
	if (!(directory_table = map_create(NULL))) {
		return EXIT_FAILURE;
	}

}

int main() {
	printf("Hello world!\n");
}





/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Shivan Kaul Sahib
 Version     : 1.0
 Description : FileSystem, assignment 4, COMP 310/ECSE 427
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <slack/std.h>
#include <slack/map.h>
#include <slack/list.h>

//#define MAX_FILES 20 // this will be defined by the N (total blocks in CCDisk) - (# of blocks allocated for
					// root directory and FAT) - 2 (the blocks for superblock and free block list)

/*
 * In memory data structures
 */

// Directory Table, implemented as a map between filenames and FAT indexes
Map *directory_table;

if (!(directory_table = map_create(NULL)))
            return EXIT_FAILURE;




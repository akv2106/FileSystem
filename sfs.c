/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Shivan Kaul Sahib
 Version     : 1.0
 Description : FileSystem, assignment 4, COMP 310/ECSE 427
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <slack/std.h>
#include <slack/map.h>
#include "disk_emu.h"
#include <stdlib.h>
#include <unistd.h>

/*
 * Function signatures
 */
void mksfs(int fresh);          // creates the file system
void sfs_ls();                  // lists files in the root directory
int sfs_fopen(char *name);      // opens the given file
void sfs_fclose(int fileID);    // closes the given file
void sfs_fwrite(int fileID, char *buf, int length); // writes buffer chars into disk
void sfs_fread(int fileID, char *buf, int length); // read chars from disk into buffer
void sfs_fseek(int fileID, int loc); // seek to location from beginning
int sfs_remove(char *file);     // removes a file from the filesystem


#define SIZE_OF_DISK 1000 // in bytes
#define MAX_FILES 100
#define EMPTY (-2)
#define MAX_FILENAME_LENGTH 25

/*
 * In-memory data structures
 */

/*
 * All maps are of type: string, int
 */
Map *directory_table;
Map *directory_table_sizes;
// Counter for next item
int directory_table_counter = 0;

// Initializes the directory table
int make_directory_table () {
	if (!(directory_table = map_create(NULL))) {
		return EXIT_FAILURE;
	}
	if (!(directory_table_sizes = map_create(NULL))) {
		return EXIT_FAILURE;
	}
	else return EXIT_SUCCESS;
}

// Free directory table and related maps
void destroy_directory_table () {
	map_destroy(&directory_table);
	map_destroy(&directory_table_sizes);
}

// The FAT element
typedef struct
{
	int data_block_number;
	int next;
} FAT_element;

// The FAT struct
typedef struct
{
	FAT_element fat_elements[SIZE_OF_DISK];
	int next;
} FAT_table;

// The File Descriptor Table element
typedef struct
{
	char filename[MAX_FILENAME_LENGTH];
	int opened; // boolean
	int write_ptr;
	int read_ptr;
} FDT_element;

typedef struct
{
	// 0 indicates empty
	// 1 indicates used
	int freeblocks[SIZE_OF_DISK];
} freeblocklist;


/*
 * Initialized structures
 */
FAT_table FAT;
freeblocklist FREE_BLOCKS_LIST;
FDT_element FDT[MAX_FILES];

int files_open = 0;
int READBLOCK_size;
int FAT_size;
int BLOCKSIZE;


/*
 * Functions
 */
void mksfs(int fresh)
{
	READBLOCK_size = sizeof(directory_table);
	FAT_size = sizeof(FAT_table);
	BLOCKSIZE = ( READBLOCK_size > FAT_size ? READBLOCK_size : FAT_size );

	if (fresh) {
		init_fresh_disk("my_disk", BLOCKSIZE, SIZE_OF_DISK);
		make_directory_table();

		int i;
		// Initialize all blocks to empty
		for(i = 0; i < SIZE_OF_DISK; i++) {
			FREE_BLOCKS_LIST.freeblocks[i] = 0;
		}
		for(i = 0; i < SIZE_OF_DISK; i++) {
			FAT.fat_elements[i].data_block_number = EMPTY;
			FAT.fat_elements[i].next = EMPTY;
		}

		// Setting the FAT next to be a certain value
		FAT.next = 1;

		write_blocks( 0, 1, (void *)&directory_table );
		write_blocks( 1, 1, (void *)&FAT );
		write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);
	} else { // If not a fresh disk
		init_disk("my_disk", BLOCKSIZE, SIZE_OF_DISK);
		read_blocks( 0, 1, (void *)&directory_table );
		read_blocks( 1, 1, (void *)&FAT );
		read_blocks( SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST );
	}
}

void sfs_ls()
{
	printf("Number of files in root directory: %d\n", map_size(directory_table));
	if (map_size(directory_table) == 0) printf("No files created as of yet! \n");

	while (map_has_next(directory_table) == 1) {
		Mapping *mapping;
		mapping = map_next_mapping(directory_table);
		char *filename = mapping_key(mapping);
		int fat_index = mapping_value(mapping);
		int size = map_get(directory_table_sizes, filename);

		printf("%s  %dBytes  \n", filename, size);
	}
}

int sfs_fopen(char * name)
{
	// Returns fileID in FDT
	int nfileID = is_opened_in_FDT(name);
	if ( nfileID != -1) {
		// if already open
		return nfileID;
	}
	if ((nfileID = exists_in_FDT(name)) != -1) {
		FDT[nfileID].opened = 1;
		return nfileID;
	}

	// File doesn't exist at all in FDT

	// Add file to file descriptor table (fdt)
	strcpy( FDT[files_open].filename, name );
	FDT[files_open].opened = 1;
	FDT[files_open].read_ptr = 0;
	nfileID = files_open;
	++files_open;

	// if file doesn't exist in directory_table
	if ( map_get(directory_table, name) == NULL ) {
		// Set write pointer to zero since new file
		FDT[files_open].write_ptr = 0;

		// Add new file to root_dir and FAT
		FAT.fat_elements[FAT.next].data_block_number = get_next_freeblock();
		FAT.fat_elements[FAT.next].next = -1;
		// Copy into directory_table and associated maps
		map_add(directory_table, name, FAT.next);
		map_add(directory_table_sizes, name, 0);

		if (get_next_fat_cursor() == -1) exit(1);
		if (map_size(directory_table) == MAX_FILES) exit(1);
		write_blocks( 0, 1, (void *)&directory_table );
		write_blocks( 1, 1, (void *)&FAT );
		write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);
	} else {
		// Set write pointer at end of file
		FDT[nfileID].write_ptr = map_get(directory_table_sizes, name);
	}
	return nfileID;

}

void sfs_fclose(int fileID)
{
	if (files_open <= fileID) {
		fprintf(stderr, "File %d doesn't exist", fileID);
	} else {
		FDT[ fileID ].opened = 0;
		FDT[ fileID ].read_ptr = 0;
	}
	return;
}

void sfs_fwrite(int fileID, char * buf, int length)
{
	if (files_open <= fileID) {
		fprintf(stderr, "No such file %d is opened\n", fileID);
		return;
	}

	int nlength = length;

	// get the associated FAT node from the directory table, which uses the FDT to get the key
	char *filename = FDT[fileID].filename;
	int fat_index = map_get(directory_table, filename);
	int db_index = FAT.fat_elements[ fat_index ].data_block_number;

	while( FAT.fat_elements[ fat_index ].next != -1 ) {
		db_index = FAT.fat_elements[ fat_index ].data_block_number;
		fat_index = FAT.fat_elements[ fat_index ].next;
	}

	// fill the current block first
	char temp_buffer[BLOCKSIZE];
	read_blocks(db_index, 1, (void *)temp_buffer);

	int write_pointer = size_in_block( FDT[ fileID ].write_ptr );
	int moreThanOneBlock = length < (BLOCKSIZE - write_pointer) ? 0 : 1;

	if ( write_pointer != -1 ) {
		if (moreThanOneBlock) memcpy( (temp_buffer + write_pointer), buf, (BLOCKSIZE - write_pointer) );
		else memcpy( (temp_buffer + write_pointer), buf, length);
		write_blocks( db_index, 1, (void *)temp_buffer );
		if (moreThanOneBlock) length = length - (BLOCKSIZE - write_pointer);
		else length = 0;
		buf = buf + (BLOCKSIZE - write_pointer);
	}


	// write the rest of the blocks
	if (moreThanOneBlock) {
		while (length > 0) {
			memcpy( temp_buffer, buf, BLOCKSIZE );

			db_index = get_next_freeblock();

			FAT.fat_elements[ fat_index ].next = FAT.next;
			FAT.fat_elements[ FAT.next ].data_block_number = db_index;
			fat_index = FAT.next;
			FAT.fat_elements[ fat_index ].next = -1;
			get_next_fat_cursor();

			length = length - BLOCKSIZE;
			buf = buf + BLOCKSIZE;

			write_blocks( db_index, 1, (void *)temp_buffer );
		}
	}

	int current_size = map_get(directory_table_sizes, filename);
	int new_size = current_size + nlength;
	map_put(directory_table_sizes, filename, new_size);
	FDT[ fileID ].write_ptr = new_size;

	write_blocks( 0, 1, (void *)&directory_table );
	write_blocks( 1, 1, (void *)&FAT );
	write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);

	return;
}

void sfs_fread(int fileID, char * buf, int length)
{
	if (files_open <= fileID && FDT[ fileID ].opened == 0 ) {
		fprintf(stderr, "No such file %d is opened\n", fileID);
		return;
	}

	char * buf_ptr = buf;

	char temp_buffer[BLOCKSIZE];
	char *filename = FDT[fileID].filename;
	int fat_index = map_get(directory_table, filename);
	int db_index;
	db_index = FAT.fat_elements[ fat_index ].data_block_number;

	// Get to the block with the current read_ptr
	int read_pointer = size_in_block( FDT[ fileID ].read_ptr );
	int rd_block_pointer = get_read_block( FDT[ fileID ].read_ptr );

	while (rd_block_pointer > 0) {
		if ( FAT.fat_elements[ fat_index ].next != -1 ) {
			fat_index = FAT.fat_elements[ fat_index ].next;
			--rd_block_pointer;
		}
	}

	db_index = FAT.fat_elements[ fat_index ].data_block_number;

	int originalLength = length;

	int moreOneBlock = length > (BLOCKSIZE - read_pointer) ? 1 : 0;

	// Read the first block
	read_blocks( db_index, 1, temp_buffer );
	if (moreOneBlock) memcpy(buf_ptr, (temp_buffer + read_pointer), (BLOCKSIZE - read_pointer));
	else memcpy(buf_ptr, (temp_buffer + read_pointer), length);
	length = length - (BLOCKSIZE - read_pointer);
	buf_ptr = buf_ptr + (BLOCKSIZE - read_pointer);
	fat_index = FAT.fat_elements[ fat_index ].next;

	// Read other blocks
	while( FAT.fat_elements[ fat_index ].next != -1 && length > 0 && length > BLOCKSIZE ) {
		db_index = FAT.fat_elements[ fat_index ].data_block_number;

		read_blocks(db_index, 1, temp_buffer);

		memcpy(buf_ptr, temp_buffer, BLOCKSIZE);

		length -= BLOCKSIZE;

		buf_ptr = buf_ptr + BLOCKSIZE;

		fat_index = FAT.fat_elements[ fat_index ].next;
	}

	if (moreOneBlock) {
		// Read last block
		db_index = FAT.fat_elements[ fat_index ].data_block_number;
		read_blocks(db_index, 1, temp_buffer);
		memcpy(buf_ptr, temp_buffer, length);
	}

	FDT[ fileID ].read_ptr += originalLength;
	return;
}

void sfs_fseek(int fileID, int loc)
{
	if (files_open <= fileID) {
		fprintf(stderr, "No such file %d is opened\n", fileID);
		return;
	}
	FDT[ fileID ].write_ptr = loc;
	FDT[ fileID ].read_ptr = loc;
	return;
}

int sfs_remove(char * file)
{
	int temp_fat_index;
	int fat_index;

	// get the FAT index
	fat_index = map_get(directory_table, file);
	if (fat_index == NULL) {
		printf("File doesn't exist in directory table.\n");
		return -1; // if the file doesn't exist in directory_table
	}

	// Remove from the FAT
	while( FAT.fat_elements[ fat_index ].next != EMPTY ) {
		FAT.fat_elements[ fat_index ].data_block_number = EMPTY;
		temp_fat_index = FAT.fat_elements[ fat_index ].next;
		FAT.fat_elements[ fat_index ].next = EMPTY;
		fat_index = temp_fat_index;
	}

	// Remove from FDT
	int fdt_index = exists_in_FDT(file);
	if ( fdt_index != -1 ) {
		FDT[ fdt_index ].opened = 0;
	}

	// Remove from directory_table
	if ((map_remove(directory_table, file)) != 0) {
		printf("Error removing file from directory table. \n");
		return EXIT_FAILURE;
	}

	// Remove from sizes
	if ((map_remove(directory_table_sizes, file)) != 0) {
		printf("Error removing file from sizes table. \n");
		return EXIT_FAILURE;
	}

	return 0;
}

/*
 * Returns -1 if not opened or
 * returns FileID if it exists in FDT
 * */

int exists_in_FDT(char * name)
{
	int i;
	int result;
	for(i = 0; i < MAX_FILES; i++) {
		result = strcmp( FDT[i].filename, name );
		if ( result == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * Returns -1 if not opened or
 * returns FileID if it exists in FDT and is opened
 * */
int is_opened_in_FDT(char * name)
{
	int i;
	int result;
	for(i = 0; i < MAX_FILES; i++) {
		result = strcmp( FDT[i].filename, name );
		if ( result == 0 && FDT[i].opened == 1) {
			return i;
		}
	}
	return -1;
}


int get_next_freeblock()
{
	int i;
	for(i = 2; i < SIZE_OF_DISK - 1; i++) {
		if (FREE_BLOCKS_LIST.freeblocks[i] == 0) {
			FREE_BLOCKS_LIST.freeblocks[i] = 1;
			return i;
			//printf("next free block: %d\n", i);
		}
	}
	printf("WARNING DISK IS FULL; OPERATION CANNOT COMPLETE\n");
	printf("next free block: %d\n", i);
	return -1;
}

int get_next_fat_cursor()
{
	FAT.next += 1;
	if (FAT.next > SIZE_OF_DISK - 1) {
		int z;
		for (z = 0; z < SIZE_OF_DISK; z++) {
			if (FAT.fat_elements[z].data_block_number == EMPTY) {
				break;
			}
		}
		FAT.next = z;
		if ( z == SIZE_OF_DISK ) {
			printf("WARNING DISK IS FULL; OPERATION CANNOT COMPLETE\n");
			return -1;
		}
	}
	return 0;
}

/*
 * Returns the pointer in the last block
 * if the pointer is on the last byte of the block,
 * -1 is returned
 * */

int size_in_block(size)
{
	if (size == 0)return 0;
	if (size % BLOCKSIZE == 0){
		return -1;
	}
	while(size > BLOCKSIZE) {
		size -= BLOCKSIZE;
	}
	return size;
}

int get_read_block(size)
{
	int block = 0;
	while (size > BLOCKSIZE) {
		size -= BLOCKSIZE;
		++block;
	}
	return block;
}

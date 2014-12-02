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
#include "disk_emu.h"
#include <stdlib.h>
#include <unistd.h>
#include <slack/std.h>
#include <slack/map.h>

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

// TO DELETE

//void printfat(void);
//void printfdt(void);
//void printroot(void);

#define SIZE_OF_DISK 1000 // in bytes
#define MAXFILES 100
#define EMPTY (-2)
#define MAX_FILENAME_LENGTH 25

/*
 * In memory data structures
 */
//typedef struct
//{
//	char filename[MAX_FILENAME_LENGTH];
//	int fat_index;
//	int size;           // In bytes
//	int empty;			// boolean
//} file;

// Map between filenames and FAT indexes
//typedef struct
//{
//	file dir_table[MAXFILES];
//	int next;
//} directory_table;


/*
 * All maps are of type: string, int
 */
Map *directory_table;
Map *directory_table_sizes, *directory_table_empty;
// Counter for next item
int directory_table_counter = 0;

int make_directory_table () {
	if (!(directory_table = map_create(NULL))) {
		return EXIT_FAILURE;
	}
	if (!(directory_table_sizes = map_create(NULL))) {
		return EXIT_FAILURE;
	}
	if (!(directory_table_empty = map_create(NULL))) {
		return EXIT_FAILURE;
	}
	else return EXIT_SUCCESS;
}

// Free directory table and related maps
void destroy_directory_table () {
	map_destroy(&directory_table);
	map_destroy(&directory_table_sizes);
	map_destroy(&directory_table_empty);
}

typedef struct
{
	int db_index;
	int next;
} FAT_node;

typedef struct
{
	FAT_node fat_nodes[SIZE_OF_DISK];
	int next;
} FAT_table;

typedef struct
{
	char filename[MAX_FILENAME_LENGTH];
	//	int root_index; // index into the root directory
	int opened; // boolean
	int write_ptr;
	int read_ptr;
} file_descriptor_table_node;

typedef struct
{
	// 0 indicates empty
	// 1 indicates used
	int freeblocks[SIZE_OF_DISK];
} freeblocklist;


/*
 * Initialized structures
 */
//directory_table ROOT;
FAT_table FAT;
freeblocklist FREE_BLOCKS_LIST;
file_descriptor_table_node FDT[MAXFILES];

int files_open = 0;
int READBLOCK_size;
int FAT_size;
int BLOCKSIZE;

//int main(void)
//{
//	mksfs(1);
//	make_directory_table();
//	sfs_ls();
//	int hell = sfs_fopen("hello");
//	sfs_fopen("world");
//	printroot();
//	printfat();
//	printfdt();
//
//	char * a_w_buffer = "This is sparta!";
//	char a_buffer[BLOCKSIZE];
//	char read_buffer[BLOCKSIZE];
//
//	printf("Before writing: %s \n", a_w_buffer);
//
//	//	char test1[] = "Hello This is the answer";
//	//	int lengthoftest1 = sizeof(test1);
//
//	sfs_fwrite(hell, "Hello This is the answer", 24);
//	//	sfs_fwrite(hell, test1, sizeof(test1));
//	sfs_fwrite(hell, "Baby, I love", 13);
//
//	printf("After Writing\n");
//
//	close_disk();
//
//	mksfs(0);
//
//	sfs_fread(hell, read_buffer, 37);
//
//	printf("\nread : \n%s\n\n", read_buffer);
//
//	printroot();
//	sfs_remove("hello");
//	printroot();
//	sfs_remove("world");
//
//	printroot();
//	printfat();
//	printfdt();
//
//	//write_blocks(2, 1, (void *)a_w_buffer);
//	//read_blocks(2, 1, a_buffer);
//	//printf("%s\nFinished\n", a_buffer);
//
//	printf("HELLO THIS IS STARFLEET COMMAND\n");
//
//	close_disk();
//	return 0;
//}


/* Here Starts the functions */
void mksfs(int fresh)
{
	READBLOCK_size     = sizeof(directory_table);
	FAT_size     = sizeof(FAT_table);
	BLOCKSIZE   = ( READBLOCK_size > FAT_size ? READBLOCK_size : FAT_size );

	if (fresh) {
		init_fresh_disk("ROOT.sfs", BLOCKSIZE, SIZE_OF_DISK);
		make_directory_table();

		// Initialize files to empty in ROOT dir
		int i;
		//		for(i = 1; i < MAXFILES; i++) {
		//			ROOT.dir_table[i].empty = 1;
		//		}
		//		ROOT.next = 0;
		// Initialize all blocks to empty
		for(i = 0; i < SIZE_OF_DISK; i++) {
			FREE_BLOCKS_LIST.freeblocks[i] = 0;
		}
		for(i = 0; i < SIZE_OF_DISK; i++) {
			FAT.fat_nodes[i].db_index = EMPTY;
			FAT.fat_nodes[i].next = EMPTY;
		}

		// Setting the FAT next to be a certain value
		FAT.next = 1;

		write_blocks( 0, 1, (void *)&directory_table );
		write_blocks( 1, 1, (void *)&FAT );
		write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);
	} else {
		init_disk("ROOT.sfs", BLOCKSIZE, SIZE_OF_DISK);
		read_blocks( 0, 1, (void *)&directory_table );
		read_blocks( 1, 1, (void *)&FAT );
		read_blocks( SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST );
	}
}

void sfs_ls()
{
	//root_dir root, *root_ptr;
	//read_blocks(0, 1, (void *)root_ptr);

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
	//	int i;
	//	for (i = 0; ROOT.dir_table[i].empty == 0; i++) {
	//		int filesize = ROOT.dir_table[i].size;
	//		printf("%s  %dBytes  \n", ROOT.dir_table[i].filename,
	//				filesize);
	//	}
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
		//printf("FAT cursor: %d\n", FAT.next_cursor);

		// Add new file to root_dir and FAT
		FAT.fat_nodes[FAT.next].db_index = get_next_freeblock();
		FAT.fat_nodes[FAT.next].next = -1;
		//		ROOT.dir_table[ ROOT.next ].empty = 0;
		//		strcpy( ROOT.dir_table[ ROOT.next ].filename, name );
		// Copy into directory_table and associated maps
		map_add(directory_table, name, FAT.next);
		//		ROOT.dir_table[ ROOT.next ].fat_index = FAT.next;
		//		ROOT.dir_table[ ROOT.next ].size = 0;
		map_add(directory_table_sizes, name, 0);

		//printroot();
		//printfat();

		//		FDT[nfileID].root_index = ROOT.next;

		if (get_next_fat_cursor() == -1) exit(1);
		//		if (get_next_root_cursor() == -1) exit(1);
		if (map_size(directory_table) == MAXFILES) exit(1);
		//		write_blocks( 0, 1, (void *)&ROOT );
		write_blocks( 0, 1, (void *)&directory_table );
		write_blocks( 1, 1, (void *)&FAT );
		write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);
	} else {
		// Set write pointer at end of file
		//		FDT[nfileID].write_ptr = ROOT.dir_table[ root_dir_index ].size;
		FDT[nfileID].write_ptr = map_get(directory_table_sizes, name);
	}
	return nfileID;

}

void sfs_fclose(int fileID)
{
	if (files_open <= fileID) {
		fprintf(stderr, "No such file %d", fileID);
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

	//	int root_index = FDT[fileID].root_index;
	// get the associated FAT node from the directory table, which uses the FDT to get the key
	char *filename = FDT[fileID].filename;
	int fat_index = map_get(directory_table, filename);
	int db_index = FAT.fat_nodes[ fat_index ].db_index;

	while( FAT.fat_nodes[ fat_index ].next != -1 ) {
		db_index = FAT.fat_nodes[ fat_index ].db_index;
		fat_index = FAT.fat_nodes[ fat_index ].next;
	}

	// fill the current block first
	char temp_buffer[BLOCKSIZE];
	/*DEBUG*/
	//printf("\ndb_index: %d\n", db_index);
	//printf("\nfat_index : %d\n", fat_index);

	read_blocks(db_index, 1, (void *)temp_buffer);

	int write_pointer = size_in_block( FDT[ fileID ].write_ptr ); //root.dir_table[ root_index ].size );
	int moreThanOneBlock = length < (BLOCKSIZE - write_pointer) ? 0 : 1;

	int lengthofbuf = strlen(buf);
	if ( write_pointer != -1 ) {
		//		memcpy( (temp_buffer + write_pointer), buf, (BLOCKSIZE - write_pointer) );
		if (moreThanOneBlock) memcpy( (temp_buffer + write_pointer), buf, (BLOCKSIZE - write_pointer) );
		else memcpy( (temp_buffer + write_pointer), buf, length);
		//		memcpy( (temp_buffer + write_pointer), buf, sizeof(buf) );
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

			FAT.fat_nodes[ fat_index ].next = FAT.next;
			FAT.fat_nodes[ FAT.next ].db_index = db_index;
			fat_index = FAT.next;
			FAT.fat_nodes[ fat_index ].next = -1;
			get_next_fat_cursor();

			/*DEBUG*/
			//printf("\ndb_index: %d\n", db_index);
			//printf("\nfat_index : %d\n", fat_index);


			length = length - BLOCKSIZE;
			//printf("length : %d\n", length);
			buf = buf + BLOCKSIZE;

			write_blocks( db_index, 1, (void *)temp_buffer );
		}
	}

	int current_size = map_get(directory_table_sizes, filename);
	int new_size = current_size + nlength;
	map_put(directory_table_sizes, filename, new_size);
	//	ROOT.dir_table[ root_index ].size += nlength;
	FDT[ fileID ].write_ptr = new_size;

	write_blocks( 0, 1, (void *)&directory_table );
	write_blocks( 1, 1, (void *)&FAT );
	write_blocks(SIZE_OF_DISK-1, 1, (void *)&FREE_BLOCKS_LIST);

	//printf("returning from sfs_fwrite\n");
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

	//	int root_index = FDT[fileID].root_index;
	//	int fat_index = ROOT.dir_table[ root_index ].fat_index;
	// get the associated FAT node from the directory table, which uses the FDT to get the key
	char *filename = FDT[fileID].filename;
	int fat_index = map_get(directory_table, filename);
	int db_index;
	db_index = FAT.fat_nodes[ fat_index ].db_index;

	// Get to the block with the current read_ptr
	int read_pointer = size_in_block( FDT[ fileID ].read_ptr );
	int rd_block_pointer = get_read_block( FDT[ fileID ].read_ptr );
	//printf("rd_block_pointer: %d\n", rd_block_pointer);

	while (rd_block_pointer > 0) {
		if ( FAT.fat_nodes[ fat_index ].next != -1 ) {
			fat_index = FAT.fat_nodes[ fat_index ].next;
			--rd_block_pointer;
		}
	}

	db_index = FAT.fat_nodes[ fat_index ].db_index;

	int originalLength = length;

	int moreOneBlock = length > (BLOCKSIZE - read_pointer) ? 1 : 0;

	// Read the first block
	read_blocks( db_index, 1, temp_buffer );
	if (moreOneBlock) memcpy(buf_ptr, (temp_buffer + read_pointer), (BLOCKSIZE - read_pointer));
	else memcpy(buf_ptr, (temp_buffer + read_pointer), length);
	//	memcpy(buf_ptr, (temp_buffer + read_pointer), sizeof(temp_buffer+read_pointer));
	length = length - (BLOCKSIZE - read_pointer);
	buf_ptr = buf_ptr + (BLOCKSIZE - read_pointer);
	fat_index = FAT.fat_nodes[ fat_index ].next;

	// Read other blocks
	while( FAT.fat_nodes[ fat_index ].next != -1 && length > 0 && length > BLOCKSIZE ) {
		db_index = FAT.fat_nodes[ fat_index ].db_index;

		read_blocks(db_index, 1, temp_buffer);

		memcpy(buf_ptr, temp_buffer, BLOCKSIZE);

		length -= BLOCKSIZE;

		buf_ptr = buf_ptr + BLOCKSIZE;

		fat_index = FAT.fat_nodes[ fat_index ].next;
	}

	if (moreOneBlock) {
		// Read last block
		db_index = FAT.fat_nodes[ fat_index ].db_index;
		read_blocks(db_index, 1, temp_buffer);
		memcpy(buf_ptr, temp_buffer, length);
		//	memcpy(buf_ptr, temp_buffer, sizeof(temp_buffer));
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
	//	int root_index = searchfile(file);
	//	if ( root_index == -1 ) return -1;

	int fat_index;

	// Remove from ROOT dir
	//	ROOT.dir_table[ root_index ].empty = 1;

	// get the FAT index
	fat_index = map_get(directory_table, file);
	if (fat_index == NULL) {
		printf("File doesn't exist in directory table.\n");
		return -1; // if the file doesn't exist in directory_table
	}

	// Remove from the FAT
	while( FAT.fat_nodes[ fat_index ].next != EMPTY ) {
		FAT.fat_nodes[ fat_index ].db_index = EMPTY;
		temp_fat_index = FAT.fat_nodes[ fat_index ].next;
		FAT.fat_nodes[ fat_index ].next = EMPTY;
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
 * returns FileID if it exists in FDT and is opened
 * */

int exists_in_FDT(char * name)
{
	int i;
	int result;
	for(i = 0; i < MAXFILES; i++) {
		//printf("filename: \"%s\", \"%s\"\n", FDT[i].filename, name);
		result = strcmp( FDT[i].filename, name );
		//printf("result:%d\n\n", result);
		//		if ( result == 0 ) {
		//			return i;
		//		}
		if ( result == 0) {
			return i;
		}
	}
	return -1;
}

int is_opened_in_FDT(char * name)
{
	int i;
	int result;
	for(i = 0; i < MAXFILES; i++) {
		//printf("filename: \"%s\", \"%s\"\n", FDT[i].filename, name);
		result = strcmp( FDT[i].filename, name );
		//printf("result:%d\n\n", result);
		//		if ( result == 0 ) {
		//			return i;
		//		}
		if ( result == 0 && FDT[i].opened == 1) {
			return i;
		}
	}
	return -1;
}

/*
 * Returns the index in the root dir if the file exists
 * else
 * Returns -1
 * */

//int searchfile(char * name)
//{
//	int i;
//	for(i = 0; i < MAXFILES; i++) {
//		if ( strcmp(ROOT.dir_table[i].filename, name) == 0 ) {
//			return i;
//		}
//	}
//	return -1;
//}

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
			if (FAT.fat_nodes[z].db_index == EMPTY) {
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

//int get_next_root_cursor()
//{
//	ROOT.next += 1;
//	if (ROOT.next > MAXFILES - 1) {
//		int z;
//		for (z = 0; z < MAXFILES; z++) {
//			if (ROOT.dir_table[z].empty == 1) {
//				break;
//			}
//		}
//		ROOT.next = z;
//		if (z == MAXFILES) {
//			printf("WARNING MAX FILES CAPACITY REACHED\n");
//			return -1;
//		}
//	}
//	return 0;
//}

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

void printroot(void)
{
	printf("Number of files in root directory: %d\n", map_size(directory_table));

	printf("\nRoot Directory\n");
	int i;
	printf("filename\tfat_index\tsize\n");

	while (map_has_next(directory_table) == 1) {
		//		char *filename = (char*)map_next(directory_table);
		//		int fat_index = (int)map_next(directory_table);

		//		char *filename = (char *)map_get(directory_table, index);
		// get the filename
		//		FDT[fat_index].filename
		Mapping *mapping;
		mapping = map_next_mapping(directory_table);
		char *filename = mapping_key(mapping);
		int fat_index = mapping_value(mapping);
		int size = map_get(directory_table_sizes, filename);
		printf("%s\t\t%d\t\t%d\n", filename, fat_index, size);
	}


	//	for (i = 0; i < MAXFILES; i++) {
	//		if ( ROOT.dir_table[i].empty == 0 ) {
	//			printf("%s\t\t%d\t\t%d\t\t%d\n", ROOT.dir_table[i].filename, ROOT.dir_table[i].fat_index,ROOT.dir_table[i].size, ROOT.dir_table[i].empty);
	//		}
	//	}
}

void printfat(void)
{
	printf("\nFAT\n");
	int i;
	printf("db_index\tnext\n");
	for (i = 0; i < SIZE_OF_DISK; i++) {
		if ( FAT.fat_nodes[i].db_index != EMPTY ) {
			printf("%d\t\t%d\n", FAT.fat_nodes[i].db_index,
					FAT.fat_nodes[i].next);
		}
	}
}

void print_FDT_all(void)
{
	printf("\nFile Descriptor Table\n");
	printf("filename\troot_index\topened\twr_ptr\trd_ptr\n");
	int i;
	for (i = 0; i < MAXFILES; i++) {
		//		if(FDT[i].opened == 1) {
		if (map_get(directory_table, FDT[i].filename) != NULL) {
			int root_index = map_get(directory_table, FDT[i].filename);
			printf("%s\t\t%d\t\t%d\t%d\t%d\n", FDT[i].filename, root_index,
					FDT[i].opened, FDT[i].write_ptr, FDT[i].read_ptr);
		}
	}
}

void print_FDT(void) {
	printf("\nFile Descriptor Table\n");
	printf("filename\troot_index\topened\twr_ptr\trd_ptr\n");
	int i;
	for (i = 0; i < MAXFILES; i++) {
		if(FDT[i].opened == 1) {
			int root_index = map_get(directory_table, FDT[i].filename);
			printf("%s\t\t%d\t\t%d\t%d\t%d\n", FDT[i].filename, root_index,
					FDT[i].opened, FDT[i].write_ptr, FDT[i].read_ptr);
		}
	}
}


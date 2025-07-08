#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "../lib/filesystem.h"
#include "../lib/utils.h"
#include <errno.h>

file_system* fs_load(const char* fs_file_path){
	//open file
	FILE* fs_file = fopen(fs_file_path,"r");
	if(fs_file == NULL){
		exit(1);
	}
	file_system* new_fs = malloc(sizeof(file_system));

	new_fs->s_block = malloc(sizeof(superblock));

	//read size from superblock
	fread(new_fs->s_block, sizeof(superblock), 1, fs_file);

	//allocate memory for the free list and load the free list from file
	new_fs->free_list = malloc(new_fs->s_block->num_blocks);
	fread(new_fs->free_list,sizeof(uint8_t), new_fs->s_block->num_blocks, fs_file);

	//allocate memory for the inodes and read them from file
	new_fs->inodes = malloc(sizeof(inode) * new_fs->s_block->num_blocks);
	fread(new_fs->inodes,sizeof(inode), new_fs->s_block->num_blocks, fs_file);

	//allocate memory for the data blocks and read them from file
	new_fs->data_blocks = malloc(sizeof(data_block)* new_fs->s_block->num_blocks);
	fread(new_fs->data_blocks,sizeof(data_block), new_fs->s_block->num_blocks, fs_file);

	//find root node
	for (int i = 0; i<new_fs->s_block->num_blocks; i++) {
		if(new_fs->inodes[i].n_type==directory && strncmp(new_fs->inodes[i].name,"/",NAME_MAX_LENGTH)==0){
			new_fs->root_node = i;
			break;
		}
	}
	
	LOG("Loaded filesystem from file\n");

	fclose(fs_file);
	return new_fs;
}

file_system* fs_create(const char* fs_file_path, uint32_t size){
	file_system* new_fs = malloc(sizeof(file_system));
	if(new_fs == NULL){
		perror("Malloc error");
		exit(errno);
	}

	// Create and Initialize the superblock
	new_fs->s_block = malloc(sizeof(superblock));
	if(new_fs->s_block == NULL){
		perror("Malloc error");
		exit(errno);
	}
	new_fs->s_block->num_blocks = size;
	new_fs->s_block->free_blocks = size;
	
	// Create free list and set every entry to 1 (meaning that block is free);
	new_fs->free_list = malloc(size);
	if (new_fs->free_list == NULL) {
		perror("Malloc error");
		exit(errno);
	}
	for (int i=0; i<size; i++) {
		new_fs->free_list[i] = 1;
	}

	// Create Inodes and initialize them
	new_fs->inodes = calloc(size,sizeof(inode));
	if (new_fs->inodes == NULL) {
		perror("Calloc error");
		exit(errno);
	}
	

	//Initialize all the inodes
	for (int i=0; i<size; i++) {
		inode_init(&(new_fs->inodes[i]));
	}
	
	//set first inode as root directory.
	//Attention: the root doesn't have to be the first inode.
	//Any other node is sufficient
	new_fs->inodes[0].n_type = directory;
	strncpy(new_fs->inodes[0].name,"/",NAME_MAX_LENGTH);
	new_fs->root_node = 0;

	
	new_fs->data_blocks = calloc(size,sizeof(data_block));
	if (new_fs->data_blocks == NULL) {
		perror("Calloc error");
		exit(errno);
	}	
	

	//write the components to file
	fs_dump(new_fs, fs_file_path);
	LOG("Created new file system.\n");

	return new_fs;

}

void inode_init(inode *i){
	i->n_type=free_block;
	i->size=0;
	memset(i->name,0,NAME_MAX_LENGTH);
	for (int j=0; j<DIRECT_BLOCKS_COUNT; j++) {
		i->direct_blocks[j] = -1;
	}
	i->parent = -1; //meaning it has no parent
}


int fs_dump(file_system *fs, const char *file_path){
	uint32_t size = fs->s_block->num_blocks;

	FILE* fs_file = fopen(file_path,"w+b");
	if (fs_file == NULL){
		exit(1);
	}

	fwrite(fs->s_block, sizeof(superblock), 1, fs_file);
	fwrite(fs->free_list, sizeof(uint8_t),size,fs_file);
	fwrite(fs->inodes, sizeof(inode),size,fs_file);
	fwrite(fs->data_blocks, sizeof(data_block),size,fs_file);
	fclose(fs_file);

	return 0;

}


int find_free_inode(file_system* fs){
	for (int i=0; i<fs->s_block->num_blocks; i++) {
		if(fs->inodes[i].n_type==free_block){
			return i;
		}
	}
	return -1;
}


void cleanup(file_system *fs){
	
	free(fs->s_block);
	free(fs->inodes);
	free(fs->free_list);
	free(fs->data_blocks);
	free(fs);

}

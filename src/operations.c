#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX_LENGTH 1024

int 
path2inode(file_system *fs, char *path, int *inodeId)
{ 
	char path_copy[PATH_MAX_LENGTH];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[sizeof(path_copy) - 1] = '\0';

	//start from superblock
	char* token = strtok(path_copy, "/");
	int token_id = 0;
	int notFound = 0;
	 
	while(token != NULL){
		if(fs->inodes[token_id].n_type != directory){
			//middle of path meets file, path is invalid
			return -1;
		}
		//if inode is directory, direct_blocks means sub inode list
		notFound = 1;
		for(int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++ ){
			int sub_inode_id = fs->inodes[token_id].direct_blocks[i];
			char* sub_dir = fs->inodes[sub_inode_id].name;
			//check directory name is matched
			if(strcmp(sub_dir, token) == 0){
				token_id = sub_inode_id;
				notFound = 0;
				break;
			}
		}
		token = strtok(NULL, "/");
		if(notFound){
			if(token == NULL){
				*inodeId = token_id;
				return -2; // end of path, not found
			}
			return -1; // middle of path, not found
		}
	}

	*inodeId = token_id;
	return 0;
}

int 
split_path(const char* full_path, char* dir_out, char* file_out) {
	// Check if full_path is null or ends with '/' or not start with'/'
	size_t len = strlen(full_path);
    if (len == 0 || full_path[len - 1] == '/' || full_path[0] != '/') {
        return -1;  // Invalid: path
    }

    // Copy the input because strtok modifies the string
    char path_copy[PATH_MAX_LENGTH];
    strncpy(path_copy, full_path, sizeof(path_copy));
    path_copy[sizeof(path_copy)-1] = '\0'; // null-terminate

    // Find the last slash '/'
    char* last_slash = strrchr(path_copy, '/');

    if (last_slash != NULL) {
        // Separate directory and file
        *last_slash = '\0'; // Cut the string at the slash
        strcpy(dir_out, path_copy);      // directory path
        strcpy(file_out, last_slash + 1); // file name
    } else {
        // No slash found — only a file name
        strcpy(dir_out, "/");
        strcpy(file_out, path_copy);
    }
	return 0;
}

int
fs_mkdir(file_system *fs, char *path)
{ 
	//the path is valid
	char parentPath[PATH_MAX_LENGTH];
	char dirName[NAME_MAX_LENGTH];
	if(split_path(path, parentPath, dirName) < 0){
		return -1; // path input error
	}

	//Get parent inode
	int parent_inodeId  = 0;
	int res = path2inode(fs, parentPath, &parent_inodeId);
	if(res != 0){
		return -1; //the parent path not exists
	}
	
	// Check if directory already exists
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int child_id = fs->inodes[parent_inodeId].direct_blocks[i];
        if (child_id == -1) continue;
        if (strcmp(fs->inodes[child_id].name, dirName) == 0) {
            return -2; // Directory already exists
        }
    }

	//find free Inode
	int freeInodeId = -1;
	for(int i = 0 ; i < fs->s_block->num_blocks ; i ++){
		if(fs->inodes[i].n_type == free_block){
			freeInodeId = i;
			break;
		}
	}
	if(freeInodeId < 0){
		return -3; //memory is full
	}
	
	// attach to parent
	int added = 0;
	for(int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++){
		if(fs->inodes[parent_inodeId].direct_blocks[i] == -1){
			fs->inodes[parent_inodeId].direct_blocks[i] = freeInodeId;
			added = 1;
			break;
		}
	}
	if(!added){
		return -3; // No space in parent inode's direct blocks
	}

	// initialize new directory inode
	inode *new_dir = &fs->inodes[freeInodeId];
    inode_init(new_dir);
	strncpy(new_dir->name, dirName, NAME_MAX_LENGTH);
	fs->inodes[freeInodeId].name[NAME_MAX_LENGTH - 1] = '\0';
	fs->inodes[freeInodeId].n_type = directory;
	fs->inodes[freeInodeId].parent = parent_inodeId;

	return 0; 
}

int
fs_mkfile(file_system *fs, char *path_and_name)
{
	return -1;
}

int
fs_cp(file_system *fs, char *src_path, char *dst_path_and_name)
{
	return -1;
}

char *
fs_list(file_system *fs, char *path)
{
	return NULL;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
	return -1;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{
	return NULL;
}


int
fs_rm(file_system *fs, char *path)
{
	return -1;
}

int
fs_import(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}

int
fs_export(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}

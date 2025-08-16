#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX_LENGTH 1024

// If a function fails, it returns a negative error code, as follows:
typedef enum {
    ERR_NOT_FOUND  = -1,
    ERR_IO         = ERR_NOT_FOUND,
	ERR_EXIST	  = -2,
    ERR_MEM_OVER   = ERR_EXIST
} err_status_t;

/*  Get child inode from path */
int inode_from_path(file_system *fs, char *path, int *inode_id)
{   
	if(!path || !inode_id) return ERR_IO;

	// Pointer protect
	char path_copy[PATH_MAX_LENGTH];
	strncpy(path_copy, path, sizeof(path_copy));
	path_copy[sizeof(path_copy) - 1] = '\0';

	// Root is '/'
	char* token = strtok(path_copy, "/");
	int token_id = 0;
	 
	while(token != NULL){
		if(fs->inodes[token_id].n_type != directory){
			//If middle of path meets file, path is invalid
			return ERR_NOT_FOUND;
		}
		
		//If inode is directory, direct_blocks means sub inode list
		int notFound = 1;
		for(int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++ ){
			int sub_inode_id = fs->inodes[token_id].direct_blocks[i];
			if(sub_inode_id != -1){
				char* sub_dir = fs->inodes[sub_inode_id].name;
				//check directory name is matched
				if(strcmp(sub_dir, token) == 0){
					token_id = sub_inode_id;
					notFound = 0;
					break;
				}
			}
		}
		if(notFound) return ERR_NOT_FOUND;  

		token = strtok(NULL, "/");
	}

	*inode_id = token_id;
	return 0;
}

//Convert path_and_name to inode_parent_dir,name
int inode_from_splitpath(file_system *fs, const char* full_path, int *parent_dir_id, char* file_out) 
{
	if(!full_path || !parent_dir_id) return ERR_IO;

	// Check if full_path ends with '/' or not start with'/'
    if (full_path[strlen(full_path) - 1] == '/' || full_path[0] != '/') {
		return ERR_IO;   
    }
	
    // Copy the input because strtok modifies the string
    char path_copy[PATH_MAX_LENGTH];
    strncpy(path_copy, full_path, sizeof(path_copy));
    path_copy[sizeof(path_copy)-1] = '\0'; // null-terminate
	
    // Find the last slash '/'
    char* last_slash = strrchr(path_copy, '/');
	
	char parent_path[PATH_MAX_LENGTH];
    if (last_slash != NULL) {
        // separate directory and file
        *last_slash = '\0'; // Cut the string at the slash
		if (strlen(path_copy) == 0)
			strcpy(parent_path, "/"); //this is used when last slash is root
		else
			strcpy(parent_path, path_copy);
		strcpy(file_out, last_slash + 1); // 		file name
    } else {
        // no slash found — only a file name
        strcpy(parent_path, "/");
        strcpy(file_out, path_copy);
    }

	//Get parent inode
	if(inode_from_path(fs, parent_path, parent_dir_id) != 0) return ERR_NOT_FOUND;
	if(fs->inodes[*parent_dir_id].n_type != directory)	return ERR_NOT_FOUND; 

	return 0;
}

// Returns created inode id
int inode_create(file_system *fs, int parent_inode_id, char *dst_name, int n_type)
{
	// Check for name collision
	inode *parent_inode = &fs->inodes[parent_inode_id];
	for (int i = 0; i < DIRECT_BLOCKS_COUNT; i ++) {
		int child_id = parent_inode->direct_blocks[i];
		if(child_id == -1) continue;
		if (strcmp(fs->inodes[child_id].name, dst_name) == 0) {
			return ERR_EXIST;  
		}
	}

	//Find free Inode
	int new_inode_id = -1;
	for(int i = 0 ; i < fs->s_block->num_blocks ; i ++){
		if(fs->inodes[i].n_type == free_block){
			new_inode_id = i;
			break;
		}
	}
	if(new_inode_id < 0) return ERR_MEM_OVER; 
	
	// Attach to parent
	int added = 0;
	for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
		if (parent_inode->direct_blocks[i] == -1) {
			parent_inode->direct_blocks[i] = new_inode_id;
			added = 1;
			break;
		}
	}
	if (!added) return ERR_MEM_OVER;  

	//Initialize inode
	inode *dst_inode = &fs->inodes[new_inode_id];
	inode_init(dst_inode);
	dst_inode->parent = parent_inode_id;
	strncpy(dst_inode->name, dst_name, NAME_MAX_LENGTH);
	dst_inode->name[NAME_MAX_LENGTH - 1] = '\0';
	dst_inode->n_type = n_type;

	return new_inode_id;
}

// One function for fs_mkdir and fs_mkfile
int inode_make(file_system *fs, char *path, int n_type)
{
	if(!fs || !path) return ERR_IO;
	// check the path is valid
	char inode_name[NAME_MAX_LENGTH];
	int parent_inode_id  = 0;
	if(inode_from_splitpath(fs, path, &parent_inode_id, inode_name) != 0) return ERR_NOT_FOUND; 

	// inode create
	int res = inode_create(fs, parent_inode_id, inode_name, n_type);
	return res < 0? res:0;
}

int
fs_mkdir(file_system *fs, char *path)
{
	return inode_make(fs, path, directory);
}

int
fs_mkfile(file_system *fs, char *path_and_name)
{
	return inode_make(fs, path_and_name, reg_file);
}

int 
inode_copy(file_system *fs, int src_inode_id, int dst_parent_inode_id, char *dst_name)
{
	//src inode
	inode *src_inode = &fs->inodes[src_inode_id];

	//dst inode
	int res = inode_create(fs, dst_parent_inode_id, dst_name, src_inode->n_type);
	if(res < 0) return res; 
	int new_inode_id = res;
 
	// Handle regular file copy
	if (src_inode->n_type == reg_file) {
		for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
			int src_block_id = src_inode->direct_blocks[i];
			if (src_block_id == -1) continue;

			// Find free data block
			int new_block_id = -1;
			for (int j = 0; j < fs->s_block->num_blocks; j++) {
				if (fs->free_list[j] == 1) {
					new_block_id = j;
					fs->free_list[j] = 0;
					break;
				}
			}
			if (new_block_id == -1) return ERR_MEM_OVER;

			fs->s_block->free_blocks--;

			// copy data
			fs->data_blocks[new_block_id].size = fs->data_blocks[src_block_id].size;
			memcpy(fs->data_blocks[new_block_id].block,
			       fs->data_blocks[src_block_id].block,
			       fs->data_blocks[src_block_id].size);

			fs->inodes[new_inode_id].direct_blocks[i] = new_block_id;
			fs->inodes[new_inode_id].size += fs->data_blocks[src_block_id].size;
		}
	}

	// Handle directory copy
	else if (src_inode->n_type == directory) {
		for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
			int child_inode_id = src_inode->direct_blocks[i];
			if (child_inode_id == -1) continue;
			
			//recursive copy
			char *child_name = fs->inodes[child_inode_id].name;
			int res = inode_copy(fs, child_inode_id, new_inode_id, child_name);
			if (res < 0) return res;
		}
	}
	return 0;
}

// Calculate the number of used inodes and blocks recursively.
void inode_property(file_system *fs, int inode_id, int *inodecnt, int *blockcnt)
{
	if(fs->inodes[inode_id].n_type == reg_file){
		(*inodecnt) ++;
		for(int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++){
			if(fs->inodes[inode_id].direct_blocks[i] != -1){
				(*blockcnt) ++;
			}
		}
	}
	else if(fs->inodes[inode_id].n_type == directory){
		(*inodecnt) ++;
		for(int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++){
			int subId = fs->inodes[inode_id].direct_blocks[i];
			if(subId != -1){
				inode_property(fs, subId, inodecnt, blockcnt);
			}
		}
	}
}

int
fs_cp(file_system *fs, char *src_path, char *dst_path_and_name)
{
	if(!fs || !src_path || !dst_path_and_name) return ERR_IO;

	//Find the inode of src_path	
	int src_inode_id  = 0;
	if(inode_from_path(fs, src_path, &src_inode_id) != 0) return ERR_NOT_FOUND; 

	//Get needed space
	int need_inode_size = 0;
	int need_datablock_size = 0;
	inode_property(fs, src_inode_id, &need_inode_size, &need_datablock_size);

	//Gget free space
	int free_inode_size = 0;
	int free_datablock_size = 0;
	for(int i = 0 ; i < fs->s_block->num_blocks ; i ++){
		if(fs->inodes[i].n_type == free_block){
			free_inode_size ++;
		}
		if(fs->free_list[i] == 1){
			free_datablock_size ++;
		}
	}

	//Check space
	if(free_inode_size < need_inode_size || free_datablock_size < need_datablock_size){
		return ERR_MEM_OVER;
	}

	// Destination inode
	char dst_name[NAME_MAX_LENGTH];
    int dst_parent_inode_id = 0;
	if (inode_from_splitpath(fs, dst_path_and_name, &dst_parent_inode_id, dst_name) != 0)  return ERR_NOT_FOUND;

	// Get parent directory inode
	return inode_copy(fs, src_inode_id, dst_parent_inode_id, dst_name);
}

char *
fs_list(file_system *fs, char *path)
{
	if(!fs || !path) return NULL;

	int inode_id = 0;
	if (inode_from_path(fs, path, &inode_id) != 0)return NULL;
	if (fs->inodes[inode_id].n_type != directory)return NULL;

	// Allocate a buffer for listing
	char *buffer = malloc(1024);
	if (!buffer) return NULL;
	buffer[0] = '\0';

	for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
		int child_id = fs->inodes[inode_id].direct_blocks[i];
		if (child_id != -1) {
			int child_type = fs->inodes[child_id].n_type;
			if(child_type == free_block) continue;
			if(child_type == reg_file){
				strcat(buffer, "FIL ");
			}else if(child_type == directory){
				strcat(buffer, "DIR ");
			}
			strcat(buffer, fs->inodes[child_id].name);
			strcat(buffer, "\n");
		}
	}

	return buffer;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
	if (!fs || !filename || !text) return ERR_IO;

	int inode_id;
	if (inode_from_path(fs, filename, &inode_id) != 0) return ERR_NOT_FOUND;  
	inode *node = &fs->inodes[inode_id];
	if (node->n_type != reg_file) return ERR_NOT_FOUND;

	// Skip exist data blocks
	int block_index = 0;
	while (block_index < DIRECT_BLOCKS_COUNT && node->direct_blocks[block_index] != -1){
		block_index ++;
	}

	// clean other data blocks
	for (int i = block_index ; i < DIRECT_BLOCKS_COUNT ; i ++){
		int block_id = node->direct_blocks[i];
		if (block_id != -1) {
			fs->free_list[block_id] = 1;
			fs->s_block->free_blocks ++;
			node->direct_blocks[i] = -1;
		}
	}

	size_t text_len = strlen(text);
	size_t bytes_written = 0;

	// Try appending into the last partially filled block, if it exists
	if (block_index > 0) {
        int last_block_id = node->direct_blocks[block_index - 1];
        data_block *blk = &fs->data_blocks[last_block_id];
        int space_left = BLOCK_SIZE - blk->size;

        if (space_left > 0) {
            int to_write = (text_len < space_left) ? text_len : space_left;
            memcpy(blk->block + blk->size, text, to_write);
            blk->size += to_write;
            bytes_written += to_write;
			node->size += to_write;
        }
    }

	while (bytes_written < text_len && block_index < DIRECT_BLOCKS_COUNT) {
		// find a free block
		int block_id = -1;
		for (int i = 0; i < fs->s_block->num_blocks; i++) {
			if (fs->free_list[i]) {
				block_id = i;
				fs->free_list[i] = 0;
				fs->s_block->free_blocks--;
				break;
			}
		}
		if (block_id == -1) return ERR_MEM_OVER;  

		// split unit size BLOCK_SIZE(1024 bytes)
		size_t chunk_size = text_len - bytes_written;
		if (chunk_size > BLOCK_SIZE)
			chunk_size = BLOCK_SIZE;

		memcpy(fs->data_blocks[block_id].block, text + bytes_written, chunk_size);
		fs->data_blocks[block_id].size = chunk_size;

		node->direct_blocks[block_index++] = block_id;
		bytes_written += chunk_size;
		node->size += chunk_size;
	} 

	// Not enough blocks available
    if (bytes_written < text_len) {
        return ERR_MEM_OVER;
    }

	return bytes_written;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{
	if (!fs || !filename || !file_size) return NULL;

	int inode_id;
    if (inode_from_path(fs, filename, &inode_id) != 0)  return NULL;
    inode *node = &fs->inodes[inode_id];
    if (node->n_type != reg_file) return NULL;

    // Calculate total file size
    int total_size = 0;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int block_id = node->direct_blocks[i];
        if (block_id != -1) {
            total_size += fs->data_blocks[block_id].size;
        }
    }

    if (total_size == 0) {
        *file_size = 0;
        return NULL; // Empty file
    }

    uint8_t *buffer = malloc(total_size);
    if (!buffer) return NULL;

    int offset = 0;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int block_id = node->direct_blocks[i];
        if (block_id != -1) {
            int size = fs->data_blocks[block_id].size;
            memcpy(buffer + offset, fs->data_blocks[block_id].block, size);
            offset += size;
        }
    }
	
    *file_size = total_size;
    return buffer;
}

int
fs_rm(file_system *fs, char *path)
{
	if (!fs || !path) return ERR_IO;

    int inode_id;
    if (inode_from_path(fs, path, &inode_id) != 0) return ERR_NOT_FOUND; 

    inode *target = &fs->inodes[inode_id];

    // Cannot remove root directory
    if (inode_id == 0) return ERR_NOT_FOUND;

    // Recursively remove contents if it's a directory
	int children[DIRECT_BLOCKS_COUNT];
	memcpy(children, target->direct_blocks, sizeof(children));
    if (target->n_type == directory) {
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            if (children[i] != -1) {
                char child_path[PATH_MAX_LENGTH];
                snprintf(child_path, sizeof(child_path), "%s/%s", path, fs->inodes[children[i]].name);
                fs_rm(fs, child_path);
            }
        }
    }

    // Free data blocks if it's a file
    if (target->n_type == reg_file) {
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            int block_id = target->direct_blocks[i];
            if (block_id != -1) {
                fs->free_list[block_id] = 1;
                fs->s_block->free_blocks++;
                target->direct_blocks[i] = -1;
            }
        }
    }
     
    // Remove inode from parent directory
    int parent_id = target->parent;
    if (parent_id >= 0 && parent_id < fs->s_block->num_blocks) {
        inode *parent = &fs->inodes[parent_id];
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            if (parent->direct_blocks[i] == inode_id) {
                parent->direct_blocks[i] = -1;
                break;
            }
        }
    }

    // Free inode
    inode_init(target);
    target->n_type = free_block;

    return 0;
}

int
fs_import(file_system *fs, char *int_path, char *ext_path)
{
	if (!fs || !int_path || !ext_path) return ERR_IO;

    // Open external file for reading
    FILE *src = fopen(ext_path, "rb");
    if (!src) return ERR_NOT_FOUND;  

    // Read external file content
    fseek(src, 0, SEEK_END);
    long data_len = ftell(src);
    fseek(src, 0, SEEK_SET);
    if (data_len <= 0) {
        fclose(src);
        return ERR_IO;
    }

    uint8_t *data = malloc(data_len);
    if (!data) {
		fclose(src);
        return ERR_MEM_OVER;
    }

    fread(data, 1, data_len, src);
    fclose(src);

    // Create internal file (if it does not exist)
    int create_result = fs_mkfile(fs, int_path);
    if (create_result != 0 && create_result != ERR_EXIST) {
        free(data);
        return create_result;
    }
	
	int inode_id;
	if (inode_from_path(fs, int_path, &inode_id) != 0){
        free(data);
		return ERR_NOT_FOUND; 
	}

	inode *node = &fs->inodes[inode_id]; 

	// Clear origin data blocks
	for (int i = 0 ; i < DIRECT_BLOCKS_COUNT ; i ++){
		int block_id = node->direct_blocks[i];
		if (block_id != -1) {
			fs->free_list[block_id] = 1;
			fs->s_block->free_blocks ++;
			node->direct_blocks[i] = -1;
		}
	}
	node->size = 0;

	// Write remaining data to new blocks
	int block_index = 0;
	size_t bytes_written = 0;
	while (bytes_written < data_len && block_index < DIRECT_BLOCKS_COUNT) {
		// find empty block
		int block_id = -1;
		for (int i = 0; i < fs->s_block->num_blocks; i++) {
			if (fs->free_list[i]) {
				block_id = i;
				fs->free_list[i] = 0;
				fs->s_block->free_blocks--;
				break;
			}
		}
		if (block_id == -1){
			free(data);
			return ERR_MEM_OVER; 
		}  

		//split unit is BLOCK_SIZE(1024 bytes)
		size_t chunk_size = data_len - bytes_written;
		if (chunk_size > BLOCK_SIZE)
			chunk_size = BLOCK_SIZE;

		memcpy(fs->data_blocks[block_id].block, data + bytes_written, chunk_size);
		fs->data_blocks[block_id].size = chunk_size;

		node->direct_blocks[block_index++] = block_id;
		bytes_written += chunk_size;
		node->size += chunk_size;
	}

	free(data);
	if (bytes_written < data_len){
		return ERR_MEM_OVER;
	}

	return 0;
}

int
fs_export(file_system *fs, char *int_path, char *ext_path)
{
	if (!fs || !int_path || !ext_path) return ERR_IO;

    // Locate the internal file inode
    int inode_id;
    if (inode_from_path(fs, int_path, &inode_id) != 0)  return ERR_NOT_FOUND;
    inode *file_inode = &fs->inodes[inode_id];
    if (file_inode->n_type != reg_file)  return ERR_NOT_FOUND;  

    // Open the external file for writing
    FILE *dst = fopen(ext_path, "wb");
    if (!dst)  return ERR_NOT_FOUND;

    // Write all data blocks in order
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; ++i) {
        int block_id = file_inode->direct_blocks[i];
        if (block_id == -1) continue;

        data_block *blk = &fs->data_blocks[block_id];
        if (fwrite(blk->block, 1, blk->size, dst) != blk->size) {
            fclose(dst);
            return ERR_MEM_OVER;
        }
    }

    fclose(dst);
    return 0;
}

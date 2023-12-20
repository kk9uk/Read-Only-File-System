#include "ROFS.h"
#include "superblock.h"
const char *DISK = "SOFTDISK";

#define min(a, b) ((a) < (b) ? (a) : (b))

void error(char* msg){
	fprintf(stderr, "%s\n", msg);
}

inode *get_inode(int inode_number){
	int hard_disk = open(DISK, O_RDWR), inode_offset = inode_number * sizeof(inode) + I_OFFSET;
	if (lseek(hard_disk, inode_offset, SEEK_SET) == -1){
		close(hard_disk);
		error("lseek error\n");
		return NULL;
	}
	
	inode* curr = (inode*) malloc(sizeof(inode));
	read(hard_disk, curr, sizeof(inode));
	close(hard_disk);
	return curr;
}

int get_data(int block_number, char* buffer, int offset, int count){
	int hard_disk = open(DISK, O_RDWR), block_offset = block_number * BLK_SIZE + D_OFFSET + offset;
	if (lseek(hard_disk, block_offset, SEEK_SET) == -1){
		close(hard_disk);
		error("lseek error\n");
		return -1;
	}
	
	int data = read(hard_disk, buffer, count);
	close(hard_disk);
	return data;
}

int get_indirect_block_number(int block_number, int i){
	int hard_disk = open(DISK, O_RDWR), block_offset = block_number * BLK_SIZE + D_OFFSET + i * sizeof(int);
	if (lseek(hard_disk, block_offset, SEEK_SET) == -1){
		close(hard_disk);
		error("lseek error\n");
		return -1;
	}
	
	int* indirect_block_number = (int*) malloc(sizeof(int));
	read(hard_disk, indirect_block_number, sizeof(int));
	close(hard_disk);
	return *indirect_block_number;
}

int find(inode* dir, char* filename){
	if (!dir->f_type){
		error("this is not a directory\n");
		return -1;
	}

	int hard_disk = open(DISK, O_RDWR);
	DIR_NODE* files = malloc(sizeof(DIR_NODE));
	char* buffer = malloc(dir->f_size);
	if (dir->blk_number >= 1){
		get_data(dir->direct_blk[0], buffer, 0, min(dir->f_size, BLK_SIZE));
	}
	if (dir->blk_number >= 2){
		get_data(dir->direct_blk[1], buffer + BLK_SIZE, 0, min(dir->f_size - BLK_SIZE, BLK_SIZE));
	}
	if (dir->blk_number >= 3){
		for (int i = 0; i + 3 <= dir->blk_number; ++i){
			int indirect_block_number = get_indirect_block_number(dir->indirect_blk, i);
			int buffer_offset = i * BLK_SIZE + 2 * BLK_SIZE;
			get_data(indirect_block_number, buffer + buffer_offset, 0, min(dir->f_size - buffer_offset, BLK_SIZE));
		}
	}

	int file = -1;
	for (int i = 0; i < dir->sub_f_num; ++i){
		int buffer_offset = i * sizeof(DIR_NODE);
		memcpy(files, buffer + buffer_offset, sizeof(DIR_NODE));
		
		if (strcmp(files->f_name, filename) == 0){
			close(hard_disk);
			file = files->i_number;
			break;
		}
	}
	close(hard_disk);
	return file;
}

int open_t(char *pathname){
    if (pathname[0] != '/'){
        error("pathname must start with '/'\n");
        return -1;
    }
	int inode_number;

	int l = 1, r = 1, pathname_len = strlen(pathname);
	inode* curr = get_inode(0);
	while (l < pathname_len) {
		while (pathname[r] != '/' && r < pathname_len) ++r;
		char* filename = (char*) malloc(r - l + 1);
		memcpy(filename, pathname + l, r - l);
		filename[r - l] = '\0';

		inode_number = find(curr, filename);
		if (inode_number == -1){
			error("file not found\n");
			return -1;
		}

		curr = get_inode(inode_number);
		l = r + 1;
		r = l;
	}

	return curr->i_number;
}

int read_t(int inode_number, int offset, void* buffer, int count){
	int read_bytes = 0;

	inode* curr = get_inode(inode_number);
	if (curr->f_type){
		error("this is not a file\n");
		return -1;
	}

	count = min(count, curr->f_size - offset);
	int start = offset / BLK_SIZE, start_offset = offset % BLK_SIZE;
	int end = (offset + count - 1) / BLK_SIZE;
	int block_number, block_offset = start_offset;
	for (int i = start; i <= end && read_bytes < count; ++i) {
		if (i < 2) {
			block_number = curr->direct_blk[i];
		}
		else {
			block_number = get_indirect_block_number(curr->indirect_blk, i - 2);
		}
		int read = min(count - read_bytes, BLK_SIZE - block_offset);
		read = get_data(block_number, buffer + read_bytes, block_offset, read);
		read_bytes += read;
		block_offset = 0;
	}
	return read_bytes; 
}

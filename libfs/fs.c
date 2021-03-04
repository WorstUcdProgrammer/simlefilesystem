#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

struct superblock {
	/* Signature "ECS150FS" */
	uint32_t signature[2];
	/* Total amount of blocks of virtual disk */
	uint16_t total_block_disk;
	/* Root directory block index */
	uint16_t root;
	/* Data block start index */
	uint16_t data;
	/* Amount of data blocks */
	uint16_t total_data_blocks;
	/* Number of blocks for FAT */
	uint8_t FAT_count;
	/* Unused / Padding */
	uint32_t Padding_4076[1019];
	uint8_t Padding_3[3];
};

struct file_entry {
	/* Filename (including NULL character) */
	uint32_t filename[4];
	/* Size of the file (in bytes) */
	uint32_t size;
	/* Index of the first data block */
	uint16_t index;
	/* Unused / Padding */
	uint16_t Padding_10[5];
};

struct ECS150fd {
	char filename[16];
	int fd;
	int offset;
};

/* Global variables */
struct superblock superblock;

uint16_t *FAT;

struct file_entry Root[FS_FILE_MAX_COUNT];

int mounted = 0;

int fd_count = 0;

struct ECS150fd fds[FS_OPEN_MAX_COUNT];

int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname)) {

		return -1;

	}

	block_read(0, &superblock);

	char sig[8] = {'E','C','S','1','5','0','F','S'};

	if (memcmp(&superblock.signature, &sig, 8)) {

		return -1;

	}

	FAT = (uint16_t *) malloc(sizeof(uint16_t) * 2048 * superblock.FAT_count);

	uint16_t *tmp_fat = FAT;

	for (int i = 0; i < superblock.FAT_count; i++) {

		block_read(i + 1, tmp_fat);

		tmp_fat = tmp_fat + 2048;

	}

	block_read(superblock.root, &Root[0]);

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {

		fds[i].fd = -1;

		fds[i].offset = 0;

	}

	mounted = 1;

	return 0;
}

int fs_umount(void)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	block_write(0, &superblock);

	uint16_t *tmp_fat = FAT;

	for (int i = 0; i < superblock.FAT_count; i++) {

		block_write(i + 1, tmp_fat);

		tmp_fat = tmp_fat + 2048;

	}

	block_write(superblock.root, &Root[0]);

	if (block_disk_close()) {

		return -1;

	}

	free(FAT);

	mounted = 0;

	return 0;
}

int fs_info(void)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", superblock.total_block_disk);
	printf("fat_blk_count=%d\n", superblock.FAT_count);
	printf("rdir_blk=%d\n", superblock.root);
	printf("data_blk=%d\n", superblock.data);
	printf("data_blk_count=%d\n", superblock.total_data_blocks);

	int free_fat_count = 0;

	uint16_t *tmp_fat = FAT;

	for (int i = 0; i < superblock.total_data_blocks; i++) {

		if (*tmp_fat == 0) {

			free_fat_count++;

		}

		tmp_fat++;

	}

	printf("fat_free_ratio=%d/%d\n", free_fat_count, superblock.total_data_blocks);

	int free_root_count = 0;

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

		uint8_t empty = '\0';

		if(!memcmp(&Root[i].filename, &empty, 1)) {

			free_root_count++;

		}
	}

	printf("rdir_free_ratio=%d/%d\n", free_root_count, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* invalid filename */
	if (strlen(filename) > FS_FILENAME_LEN - 1) {
		
		return -1;

	}

	char empty = '\0';

	/* not null terminated */
	if (memcmp(filename + strlen(filename), &empty, 1)) {

		return -1;

	}

	/* existing filename */
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

		if(!memcmp(&Root[i].filename, filename, strlen(filename))) {

			return -1;

		}
	}

	int first_root_empty_index = -1;

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

		uint8_t empty = '\0';

		if(!memcmp(&Root[i].filename, &empty, 1)) {

				first_root_empty_index = i;
				break;

		}
	}

	/* no more space */
	if (first_root_empty_index == -1) {

		return -1;

	}

	memcpy(&Root[first_root_empty_index].filename, filename, strlen(filename) + 1);

	Root[first_root_empty_index].size = 0;

	Root[first_root_empty_index].index = FAT_EOC;

	block_write(superblock.root, &Root[0]);

	return 0;
}

int fs_delete(const char *filename)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* invalid filename */
	if (strlen(filename) > FS_FILENAME_LEN - 1) {
		
		return -1;

	}

	char empty = '\0';

	/* not null terminated */
	if (memcmp(filename + strlen(filename), &empty, 1)) {

		return -1;

	}

	int root_file_index = -1;

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

		if(!memcmp(&Root[i].filename, filename, strlen(filename))) {

				root_file_index = i;
				break;

		}
	}

	/* no file found */
	if (root_file_index == -1) {

		return -1;

	}

	uint8_t empty_byte = '\0';

	memcpy(&Root[root_file_index].filename, &empty_byte, 1);

	return 0;
}

int fs_ls(void)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	printf("FS Ls:\n");

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {

		uint8_t empty = '\0';

		if(memcmp(&Root[i].filename, &empty, 1)) {
			
			printf("file: %s, size: %d, data_blk: %d\n", (char *) &Root[i].filename, Root[i].size, Root[i].index);

		}
	}

	return 0;
}

int fs_open(const char *filename)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* max files reached*/
	if (fd_count == FS_OPEN_MAX_COUNT) {
		
		return -1;
	}

	/* invalid filename */
	if (strlen(filename) > FS_FILENAME_LEN - 1) {
		
		return -1;

	}

	char empty = '\0';

	/* not null terminated */
	if (memcmp(filename + strlen(filename), &empty, 1)) {

		return -1;

	}

	int new_fd = open(filename, O_RDWR, 0644);

	if (new_fd == -1) {

		return -1;

	}

	int index = -1;

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {

		if (fds[i].fd == -1) {

			index = i;

		}
	}

	strcpy(&fds[index].filename[0], filename);

	fds[index].fd = new_fd;

	fd_count++;

	return new_fd;
}

int fs_close(int fd)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* out of bound */
	if (fd < 0) {

		return -1;
	}

	int index = -1;

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {

		if (fds[i].fd == fd) {

			index = i;
			break;

		}
	}

	/* fd not found */
	if (index == -1) {

		return -1;

	}

	close(fd);

	fds[index].fd = -1;

	fds[index].offset = 0;

	fd_count--;

	return 0;
}

int fs_stat(int fd)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* out of bound */
	if (fd < 0) {

		return -1;
	}

	int index = -1;

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {

		if (fds[i].fd == fd) {

			index = i;
			break;

		}
	}

	/* fd not found */
	if (index == -1) {

		return -1;

	}

	struct stat buf;

	fstat(fd, &buf);

	return buf.st_size;
}

int fs_lseek(int fd, size_t offset)
{
	/* no disk mounted */
	if (!mounted) {

		return -1;

	}

	/* out of bound */
	if (fd < 0) {

		return -1;
	}

	int index = -1;

	for (int i = 0; i < FS_OPEN_MAX_COUNT; i++) {

		if (fds[i].fd == fd) {

			index = i;
			break;

		}
	}

	/* fd not found */
	if (index == -1) {

		return -1;

	}

	/* larger than file size */
	if ((int) offset > fs_stat(fd)) {

		return -1;

	}

	fds[index].offset = offset;

	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	printf("%d\n", fd);
	block_write(count, buf);
	(void) count;
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	printf("%d\n", fd);
	block_read(count, buf);
	(void) count;
	return 0;
}


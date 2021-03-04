#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

/* Global variables */
struct superblock superblock;

uint16_t *FAT;

struct file_entry Root[FS_FILE_MAX_COUNT];

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

	FAT = (uint16_t *) malloc(sizeof(uint16_t) * superblock.total_data_blocks * (unsigned int) superblock.FAT_count);

	uint16_t *tmp_fat = FAT;

	for (int i = 0; i < superblock.FAT_count; i++) {

		block_read(i + 1, tmp_fat);

		tmp_fat = tmp_fat + superblock.total_data_blocks;

	}

	block_read(superblock.root, &Root[0]);

	return 0;
}

int fs_umount(void)
{
	block_write(0, &superblock);

	uint16_t *tmp_fat = FAT;

	for (int i = 0; i < superblock.FAT_count; i++) {

		block_write(i + 1, tmp_fat);

		tmp_fat = tmp_fat + superblock.total_data_blocks;

	}

	block_write(superblock.root, &Root);

	if (block_disk_close()) {

		return -1;

	}

	free(FAT);

	return 0;
}

int fs_info(void)
{
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
	printf("%s\n", filename);
	return 0;
}

int fs_delete(const char *filename)
{
	printf("%s\n", filename);
	return 0;
}

int fs_ls(void)
{
	return 0;
}

int fs_open(const char *filename)
{
	printf("%s\n", filename);
	return 0;
}

int fs_close(int fd)
{
	printf("%d\n", fd);
	return 0;
}

int fs_stat(int fd)
{
	printf("%d\n", fd);
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	printf("%d\n", fd);
	(void) offset;
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


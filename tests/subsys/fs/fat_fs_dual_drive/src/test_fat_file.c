/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test_filesystem
 * Demonstrates the ZEPHYR File System APIs
 */

#include "test_fat.h"
#include <string.h>

static int test_file_open(const char *path)
{
	int res;

	TC_PRINT("Open tests:\n");

	if (check_file_dir_exists(path)) {
		TC_PRINT("Opening existing file %s\n", path);
	} else {
		TC_PRINT("Creating new file %s\n", path);
	}

	/* Verify fs_open() */
	res = fs_open(&filep, path, FS_O_CREATE | FS_O_RDWR);
	if (res) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	TC_PRINT("Opened file %s\n", path);

	return res;
}

int test_file_write(void)
{
	ssize_t brw;
	int res;

	TC_PRINT("Write tests:\n");

	/* Verify fs_seek() */
	res = fs_seek(&filep, 0, FS_SEEK_SET);
	if (res) {
		TC_PRINT("fs_seek failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	TC_PRINT("Data written:\"%s\"\n\n", test_str);

	/* Verify fs_write() */
	brw = fs_write(&filep, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%zd]\n", brw);
		fs_close(&filep);
		return brw;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%zd]\n", brw);
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Data successfully written!\n");

	return res;
}

static int test_file_sync(void)
{
	int res;

	TC_PRINT("Sync tests:\n");

	/* Verify fs_sync() */
	res = fs_sync(&filep);
	if (res) {
		TC_PRINT("Error syncing file [%d]\n", res);
		return res;
	}

	return res;
}

static int test_file_read(void)
{
	ssize_t brw;
	int res;
	char read_buff[80];
	size_t sz = strlen(test_str);

	TC_PRINT("Read tests:\n");

	res = fs_seek(&filep, 0, FS_SEEK_SET);
	if (res) {
		TC_PRINT("fs_seek failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	/* Verify fs_read() */
	brw = fs_read(&filep, read_buff, sz);
	if (brw < 0) {
		TC_PRINT("Failed reading file [%zd]\n", brw);
		fs_close(&filep);
		return brw;
	}

	read_buff[brw] = 0;

	TC_PRINT("Data read:\"%s\"\n\n", read_buff);

	if (strcmp(test_str, read_buff)) {
		TC_PRINT("Error - Data read does not match data written\n");
		TC_PRINT("Data read:\"%s\"\n\n", read_buff);
		return TC_FAIL;
	}

	TC_PRINT("Data read matches data written\n");

	return res;
}

static int test_file_truncate(void)
{
	int res;
	off_t orig_pos;
	char read_buff[80];
	ssize_t brw;

	TC_PRINT("Truncate tests:\n");

	/* Verify fs_truncate() */
	/* Test truncating to 0 size */
	TC_PRINT("Testing shrink to 0 size\n");
	res = fs_truncate(&filep, 0);
	if (res) {
		TC_PRINT("fs_truncate failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	res = fs_seek(&filep, 0, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed after truncating file [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	/* Verify fs_tell() */
	if (fs_tell(&filep) > 0) {
		TC_PRINT("Failed truncating to size 0\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Testing write after truncating\n");
	res = test_file_write();
	if (res) {
		TC_PRINT("Write failed after truncating\n");
		return res;
	}

	res = fs_seek(&filep, 0, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed after writing to file [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	orig_pos = fs_tell(&filep);
	TC_PRINT("Original size of file = %ld\n", (long) orig_pos);

	/* Test shrinking file */
	TC_PRINT("\nTesting shrinking\n");
	res = fs_truncate(&filep, orig_pos - 5);
	if (res) {
		TC_PRINT("fs_truncate failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	res = fs_seek(&filep, 0, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed after shrinking file [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	TC_PRINT("File size after shrinking by 5 bytes = %ld\n",
						(long) fs_tell(&filep));
	if (fs_tell(&filep) != (orig_pos - 5)) {
		TC_PRINT("File size after fs_truncate not as expected\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	/* Test expanding file */
	TC_PRINT("Testing expanding\n");
	res = fs_seek(&filep, 0, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed before expanding file [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	orig_pos = fs_tell(&filep);
	res = fs_truncate(&filep, orig_pos + 10);
	if (res) {
		TC_PRINT("fs_truncate failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	res = fs_seek(&filep, 0, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed after expanding file [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	TC_PRINT("File size after expanding by 10 bytes = %ld\n",
						(long) fs_tell(&filep));
	if (fs_tell(&filep) != (orig_pos + 10)) {
		TC_PRINT("File size after fs_truncate not as expected\n");
		fs_close(&filep);
		return TC_FAIL;
	}

	/* Check if expanded regions are zeroed */
	TC_PRINT("Testing for zeroes in expanded region\n");
	res = fs_seek(&filep, -5, FS_SEEK_END);
	if (res) {
		TC_PRINT("fs_seek failed before testing for zeroes [%d]\n",
			 res);
		fs_close(&filep);
		return res;
	}

	brw = fs_read(&filep, read_buff, 5);

	if (brw < 5) {
		TC_PRINT("Read failed after truncating\n");
		fs_close(&filep);
		return -1;
	}

	for (int i = 0; i < 5; i++) {
		if (read_buff[i]) {
			TC_PRINT("Expanded regions are not zeroed\n");
			fs_close(&filep);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

int test_file_close(void)
{
	int res;

	TC_PRINT("Close tests:\n");

	res = fs_close(&filep);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	return res;
}

static int test_file_delete(const char *path)
{
	int res;

	TC_PRINT("Delete tests:\n");

	/* Verify fs_unlink() */
	res = fs_unlink(path);
	if (res) {
		TC_PRINT("Error deleting file [%d]\n", res);
		return res;
	}

	/* Check if file was deleted */
	if (check_file_dir_exists(path)) {
		TC_PRINT("Failed deleting %s\n", path);
		return TC_FAIL;
	}

	TC_PRINT("File (%s) deleted successfully!\n", path);

	return res;
}

ZTEST(fat_fs_dual_drive, test_fat_file)
{
	TC_PRINT("Testing file operations on %s\n", FATFS_MNTP);
	zassert_true(test_file_open(TEST_FILE) == TC_PASS, NULL);
	zassert_true(test_file_write() == TC_PASS, NULL);
	zassert_true(test_file_sync() == TC_PASS, NULL);
	zassert_true(test_file_read() == TC_PASS, NULL);
	zassert_true(test_file_truncate() == TC_PASS, NULL);
	zassert_true(test_file_close() == TC_PASS, NULL);
	zassert_true(test_file_delete(TEST_FILE) == TC_PASS, NULL);

	TC_PRINT("Testing file operations on %s\n", FATFS_MNTP1);
	zassert_true(test_file_open(TEST_FILE1) == TC_PASS, NULL);
	zassert_true(test_file_write() == TC_PASS, NULL);
	zassert_true(test_file_sync() == TC_PASS, NULL);
	zassert_true(test_file_read() == TC_PASS, NULL);
	zassert_true(test_file_truncate() == TC_PASS, NULL);
	zassert_true(test_file_close() == TC_PASS, NULL);
	zassert_true(test_file_delete(TEST_FILE1) == TC_PASS, NULL);
}

#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>

static const int INPUT_PARAMS = 2;
static const int SRC_FILE_ARGV_POSITION = 1, DEST_FILE_ARGV_POSITION = 2;

static const int GENERIC_ERROR_CODE = -1;
static const int FAILED = -1, SUCCESS = 0;

static const int FILE_EXISTS = 0, FILE_DOES_NOT_EXIST = -1;
static const bool EXIT_ON_FAILURE = true, DONT_EXIT_ON_FAILURE = false;

int
does_file_exist(char *filepath)
{
	int res = access(filepath, F_OK);
	if (res == FILE_DOES_NOT_EXIST) {
		return FILE_DOES_NOT_EXIST;
	}

	return FILE_EXISTS;
}

void
parse_arguments(char *argv[], char **src_filepath, char **dest_filepath)
{
	*src_filepath = argv[SRC_FILE_ARGV_POSITION];
	if (does_file_exist(*src_filepath) == FILE_DOES_NOT_EXIST) {
		fprintf(stderr,
		        "Error: source file '%s' does not exist",
		        *src_filepath);
		exit(EXIT_FAILURE);
	}

	*dest_filepath = argv[DEST_FILE_ARGV_POSITION];
	if (does_file_exist(*dest_filepath) == FILE_EXISTS) {
		fprintf(stderr, "Error: destination file '%s' already exists. Copy aborted", *dest_filepath);
		exit(EXIT_FAILURE);
	}
}

int
unlink_file(char *filepath)
{
	int res = unlink(filepath);
	if (res == GENERIC_ERROR_CODE) {
		perror("Error: could not remove the file from the filesystem");
		return FAILED;
	}
	return SUCCESS;
}

int
close_fd(int fd, bool exit_on_failure)
{
	int res = close(fd);
	if (res == GENERIC_ERROR_CODE) {
		perror("Error: failed to close FD");
		if (exit_on_failure == EXIT_FAILURE) {
			exit(EXIT_FAILURE);
		}
		return FAILED;
	}
	return SUCCESS;
}

void
close_all_fds(int src_fd, int dest_fd)
{
	int res_src = close_fd(src_fd, DONT_EXIT_ON_FAILURE);
	int res_dest = close_fd(dest_fd, DONT_EXIT_ON_FAILURE);

	if (res_src == FAILED || res_dest == FAILED) {
		exit(EXIT_FAILURE);
	}
}

void
open_files(int *src_fd,
           int *dest_fd,
           char *src_filepath,
           char *dest_filepath,
           long *src_filesize)
{
	*src_fd = open(src_filepath, O_RDONLY);
	if (*src_fd == GENERIC_ERROR_CODE) {
		perror("Error: failed to open source file from path");
		exit(EXIT_FAILURE);
	}

	*dest_fd = open(dest_filepath, O_CREAT | O_RDWR);
	if (*dest_fd == GENERIC_ERROR_CODE) {
		perror("Error: failed to create regular file from the "
		       "destination path");
		close_fd(*src_fd, EXIT_ON_FAILURE);
		exit(EXIT_FAILURE);
	}

	struct stat file_info;
	int res = fstat(*src_fd, &file_info);
	if (res == GENERIC_ERROR_CODE) {
		perror("Error: failed to accesing source file metadata");
		unlink_file(dest_filepath);
		close_all_fds(*src_fd, *dest_fd);
		exit(EXIT_FAILURE);
	}

	*src_filesize = file_info.st_size;
}

int
release_resources(void *src_bytemap, void *dest_bytemap, long src_filesize)
{
	int res_src = munmap(src_bytemap, src_filesize);
	if (res_src == GENERIC_ERROR_CODE) {
		perror("Failed to unmap memory from source");
	}

	int res_dest = munmap(dest_bytemap, src_filesize);
	if (res_dest == GENERIC_ERROR_CODE) {
		perror("Failed to unmap memory from destination");
	}

	if (res_src == GENERIC_ERROR_CODE || res_dest == GENERIC_ERROR_CODE) {
		return FAILED;
	}
	return SUCCESS;
}

void
copy_content(int src_fd, int dest_fd, long src_filesize, char *dest_filepath)
{
	void *src_map =
	        mmap(NULL, src_filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
	if (src_map == MAP_FAILED) {
		perror("Error: could not map memory for source file");
		unlink_file(dest_filepath);
		close_all_fds(src_fd, dest_fd);
		exit(EXIT_FAILURE);
	}

	int res = ftruncate(dest_fd, (off_t) src_filesize);
	if (res == GENERIC_ERROR_CODE) {
		perror("Error: could not grow file size for destination file");
		int res = munmap(src_map, src_filesize);
		if (res == GENERIC_ERROR_CODE) {
			perror("Failed to unmap memory from source");
		}
		unlink_file(dest_filepath);
		close_all_fds(src_fd, dest_fd);
		exit(EXIT_FAILURE);
	}

	void *dest_map = mmap(
	        NULL, src_filesize, PROT_READ | PROT_WRITE, MAP_SHARED, dest_fd, 0);
	if (dest_map == MAP_FAILED) {
		perror("Error: could not map memory for destination file");
		int res = munmap(src_map, src_filesize);
		if (res == GENERIC_ERROR_CODE) {
			perror("Failed to unmap memory from source");
		}
		unlink_file(dest_filepath);
		close_all_fds(src_fd, dest_fd);
		exit(EXIT_FAILURE);
	}

	memcpy(dest_map, src_map, src_filesize);

	res = release_resources(src_map, dest_map, src_filesize);
	if (res == FAILED) {
		unlink_file(dest_filepath);
		close_all_fds(src_fd, dest_fd);
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char *argv[])
{
	if (argc != INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s <source "
		        "file> <destination file>\n",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	char *src_filepath = NULL;
	char *dest_filepath = NULL;

	parse_arguments(argv, &src_filepath, &dest_filepath);

	int src_fd = 0;
	int dest_fd = 0;
	long src_filesize = 0;
	open_files(&src_fd, &dest_fd, src_filepath, dest_filepath, &src_filesize);

	// const long PAGE_SIZE = sysconf(_SC_PAGESIZE);
	copy_content(src_fd, dest_fd, src_filesize, dest_filepath);

	close_all_fds(src_fd, dest_fd);
	exit(EXIT_SUCCESS);
}

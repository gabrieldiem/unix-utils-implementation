#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>

#define MAX_USERNAME 30
#define MAX_PERMISSIONS_LEN 10
#define AUX_PERMISSIONS_VECTOR_LEN 3

#define COLOR_GREEN_BOLD "\e[1;32m"
#define COLOR_BLUE_BOLD "\e[1;34m"
#define COLOR_BG_BLUE_BOLD "\e[44m"
#define COLOR_RESET "\e[0m"

static const int INPUT_PARAMS = 0;

static const int GENERIC_ERROR_CODE = -1;
static const int SUCCESS = 0, FAILED = -1;

static const char STRING_NULL_TERMINATOR = '\0';
static const char WD_PATH_ALIAS[] = ".";

static const char FILETYPE_REGULAR_FILE = '-', FILETYPE_DIRECTORY = 'd',
                  FILETYPE_LINK = 'l';
static const char READ_PERMISSION = 'r', WRITE_PERMISSION = 'w',
                  EXECUTE_PERMISSION = 'x', NONE_PERMISSION = '-';

/*
 * Close the DIR `directory`.
 * If closing fails, the current process exits.
 */
void
close_directory(DIR *directory)
{
	int res = closedir(directory);

	if (res == GENERIC_ERROR_CODE) {
		perror("Error while closing directory");
		exit(EXIT_FAILURE);
	}
}

/*
 * Read an entity from the directory `wd` into the struct `entity`.
 * If the read fails, `entity` is nulled and the current process exits.
 */
void
read_entity_from_directory(DIR *wd, struct dirent **entity)
{
	errno = 0;
	*entity = readdir(wd);
	if (errno != 0 && *entity == NULL) {
		perror("Error while reading from directory");
		exit(EXIT_FAILURE);
	}
}

/*
 * Print the informational header of the meaning of the following values.
 */
void
print_header()
{
	printf("%s %-9s %-7s %-6s %s\n", "type", "perms", "ownerid", "owner", "filename");
}

/*
 * Print formatted and colored information about the entity read.
 * `link_destination` is only printed for enitities of filetype `FILETYPE_LINK`
 */
void
print_formatted(char *entity_name,
                char username[MAX_USERNAME],
                char user_id[MAX_USERNAME],
                char filetype,
                char permissions[MAX_PERMISSIONS_LEN],
                char link_destination[PATH_MAX])
{
	printf("%4c %s %7s %-6s ", filetype, permissions, user_id, username);

	if (filetype == FILETYPE_DIRECTORY) {
		printf(COLOR_BG_BLUE_BOLD "%s" COLOR_RESET, entity_name);
	} else if (filetype == FILETYPE_LINK) {
		printf(COLOR_BLUE_BOLD "%s" COLOR_RESET, entity_name);
	} else {
		printf(COLOR_GREEN_BOLD "%s" COLOR_RESET, entity_name);
	}

	if (filetype == FILETYPE_LINK) {
		printf(" -> " COLOR_BG_BLUE_BOLD "%s" COLOR_RESET,
		       link_destination);
	}

	printf("\n");
}

/*
 * Build the full filepath of `entity_name` into `filepath`.
 */
void
build_filepath(char filepath[PATH_MAX], char *entity_name)
{
	snprintf(filepath,
	         (PATH_MAX - 1) * sizeof(char),
	         "%s/%s",
	         WD_PATH_ALIAS,
	         entity_name);
}

/*
 * Load the status of the entity represented by `filepath` into `file_status`.
 * Returns `SUCCESS` if successful, `FAILED` otherwise.
 */
int
load_file_status(char *filepath, struct stat *file_status)
{
	int res = stat(filepath, file_status);
	if (res == GENERIC_ERROR_CODE) {
		perror("Error while getting status information from "
		       "file or directory");
		return FAILED;
	}
	return SUCCESS;
}


/*
 * Load the `username` and the `user_id` from `file_status`.
 */
void
load_user_info(char username[MAX_USERNAME],
               char user_id[MAX_USERNAME],
               struct stat *file_status)
{
	snprintf(user_id, MAX_USERNAME - 1, "%u", file_status->st_uid);

	struct passwd *user_info = NULL;
	user_info = getpwuid(file_status->st_uid);

	if (user_info == NULL) {
		snprintf(username, MAX_USERNAME - 1, "%u", file_status->st_uid);
		return;
	}

	snprintf(username, MAX_USERNAME - 1, "%s", user_info->pw_name);
}

/*
 * Load the `filetype` from `entity` which can be `FILETYPE_DIRECTORY`,
 * `FILETYPE_LINK` or `FILETYPE_REGULAR_FILE`.
 */
void
load_filetype(char *filetype, struct dirent *entity)
{
	switch (entity->d_type) {
	case DT_DIR:
		*filetype = FILETYPE_DIRECTORY;
		break;

	case DT_LNK:
		*filetype = FILETYPE_LINK;
		break;

	default:
		*filetype = FILETYPE_REGULAR_FILE;
		break;
	}
}

/*
 * Load `all_permissions` from `file_status` with the format: `<read perm user><write
 * perm user><execute perm user> <read perm group><write perm group><execute perm
 * group> <read perm others><write perm others><execute perm others>`.
 */
void
load_permissions_info(char all_permissions[MAX_PERMISSIONS_LEN],
                      struct stat *file_status)
{
	bool owner_read = file_status->st_mode & S_IRUSR;
	bool owner_write = file_status->st_mode & S_IWUSR;
	bool owner_execute = file_status->st_mode & S_IXUSR;

	bool group_read = file_status->st_mode & S_IRGRP;
	bool group_write = file_status->st_mode & S_IXGRP;
	bool group_execute = file_status->st_mode & S_IROTH;

	bool others_read = file_status->st_mode & S_IWGRP;
	bool others_write = file_status->st_mode & S_IWOTH;
	bool others_execute = file_status->st_mode & S_IXOTH;

	bool read_perms_vector[AUX_PERMISSIONS_VECTOR_LEN] = { owner_read,
		                                               group_read,
		                                               others_read };

	bool write_perms_vector[AUX_PERMISSIONS_VECTOR_LEN] = { owner_write,
		                                                group_write,
		                                                others_write };

	bool execute_perms_vector[AUX_PERMISSIONS_VECTOR_LEN] = {
		owner_execute, group_execute, others_execute
	};

	for (int i = 0; i < AUX_PERMISSIONS_VECTOR_LEN; i++) {
		char read_status = read_perms_vector[i] ? READ_PERMISSION
		                                        : NONE_PERMISSION;
		char write_status = write_perms_vector[i] ? WRITE_PERMISSION
		                                          : NONE_PERMISSION;
		char execute_status = execute_perms_vector[i] ? EXECUTE_PERMISSION
		                                              : NONE_PERMISSION;
		char perms_string[AUX_PERMISSIONS_VECTOR_LEN + 1] = {
			read_status, write_status, execute_status, STRING_NULL_TERMINATOR
		};
		strncat(all_permissions,
		        perms_string,
		        (MAX_PERMISSIONS_LEN - 1) * sizeof(char));
	}

	all_permissions[MAX_PERMISSIONS_LEN - 1] = STRING_NULL_TERMINATOR;
}

/*
 * Load the destination of the symbolic link specified by `filepath` into
 * `link_destination`. Returns `SUCCESS` if successful, `FAILED` otherwise.
 */
int
load_link_destination(char filepath[PATH_MAX], char link_destination[PATH_MAX])
{
	int res = readlink(filepath, link_destination, PATH_MAX);
	if (res == GENERIC_ERROR_CODE) {
		perror("Failed to read link destination");
		return FAILED;
	}

	return SUCCESS;
}

/*
 * Read all the entries of the directory `wd` and print their information with
 * the format: <filetype> <permissions> <owner id> <owner name>  <filename>
 * [link destination]. If the read of an enitity fails, the process exits.
 * Returns `SUCCESS` if successful, `FAILED` otherwise.
 */
int
read_directory_entries(DIR *wd)
{
	struct dirent *entity = NULL;
	read_entity_from_directory(wd, &entity);
	print_header();

	while (entity != NULL) {
		char filetype = FILETYPE_REGULAR_FILE;
		char filepath[PATH_MAX] = { STRING_NULL_TERMINATOR };
		char username[MAX_USERNAME] = { STRING_NULL_TERMINATOR };
		char user_id[MAX_USERNAME] = { STRING_NULL_TERMINATOR };
		char permissions[MAX_PERMISSIONS_LEN] = { STRING_NULL_TERMINATOR };
		char link_destination[PATH_MAX] = { STRING_NULL_TERMINATOR };
		build_filepath(filepath, entity->d_name);

		struct stat file_status;
		int res = load_file_status(filepath, &file_status);
		if (res == FAILED) {
			return FAILED;
		}

		load_user_info(username, user_id, &file_status);
		load_filetype(&filetype, entity);
		load_permissions_info(permissions, &file_status);

		if (filetype == FILETYPE_LINK) {
			int res = load_link_destination(filepath,
			                                link_destination);
			if (res == FAILED) {
				return FAILED;
			}
		}

		print_formatted(entity->d_name,
		                username,
		                user_id,
		                filetype,
		                permissions,
		                link_destination);

		read_entity_from_directory(wd, &entity);
		filepath[0] = STRING_NULL_TERMINATOR;
	}

	return SUCCESS;
}

int
main(int argc, char *argv[])
{
	if (argc != INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s with no "
		        "extra arguments\n",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	DIR *wd = opendir(WD_PATH_ALIAS);
	if (wd == NULL) {
		perror("Error while opening directory");
		exit(EXIT_FAILURE);
	}

	int wd_fd = dirfd(wd);
	if (wd_fd == GENERIC_ERROR_CODE) {
		perror("Error while getting FD from directory");
		close_directory(wd);
		exit(EXIT_FAILURE);
	}

	int res = read_directory_entries(wd);
	if (res == FAILED) {
		close_directory(wd);
		exit(EXIT_FAILURE);
	}

	close_directory(wd);
	exit(EXIT_SUCCESS);
}

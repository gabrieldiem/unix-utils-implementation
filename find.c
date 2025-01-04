#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

#define DIR_NAMES_BLACKLIST_SIZE 2

static const int MAX_INPUT_PARAMS = 2, MIN_INPUT_PARAMS = 1;
static const char CASE_SENSITIVITY_NONE_FLAG[] = "-i";
static const int NO_FLAG_FOUND = -1;
static const int CASE_SENSITIVITY_FULL_CODE = 1, CASE_SENSITIVITY_NONE_CODE = 0;

static const char STRING_NULL_TERMINATOR = '\0';

static const int GENERIC_ERROR_CODE = -1;

static const char WD_PATH_ALIAS[] = ".";
static const int WD_PATH_ALIAS_LEN = 1;

static const char *DIR_NAMES_BLACKLIST[DIR_NAMES_BLACKLIST_SIZE] = { ".", ".." };
static const int DIR_NAMES_BLACKLIST_MAX_LEN = 3;

/*
 * Parse the argv to extract the `phrase` to be used to find inside entity names
 * and establish the `case_sensitivity_code` ased on the presence of the case
 * insensitivity flag. If the arguments don't respect the order or are invalid
 * (e.g. zero size string) the program exits.
 */
void
parse_arguments(char phrase[PATH_MAX],
                int *case_sensitivity_code,
                int argc,
                char *argv[])
{
	size_t flag_str_len = strlen(CASE_SENSITIVITY_NONE_FLAG);
	int flag_position = NO_FLAG_FOUND;

	for (int i = 1; i < argc; i++) {
		bool are_args_same_len = strlen(argv[i]) == flag_str_len;
		bool have_args_equal_content =
		        strncmp(argv[i],
		                CASE_SENSITIVITY_NONE_FLAG,
		                flag_str_len * sizeof(char)) == 0;

		if (are_args_same_len && have_args_equal_content) {
			*case_sensitivity_code = CASE_SENSITIVITY_NONE_CODE;
			flag_position = i;
			break;
		}
	}

	for (int i = 1; i < argc; i++) {
		if (flag_position != i) {
			strncpy(phrase, argv[i], (PATH_MAX - 1) * sizeof(char));
			break;
		}
	}

	size_t phrase_len = strlen(phrase);
	if (phrase_len == 0) {
		fprintf(stderr,
		        "Error while calling program, no phrase found. "
		        "Expected %s [-i] <phrase>\n",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	if (flag_position == NO_FLAG_FOUND && argc != MIN_INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program, non recognized parameter "
		        "found. "
		        "Expected %s [-i] <phrase>\n",
		        argv[0]);
		exit(EXIT_FAILURE);
	}
}

/*
 * Close the `directory`. If the closing fails, the current process exits.
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
 * Check if `string` contains `substring` without taking into account the letter
 * case. Returns `true` if `substring` is found in `string`, `false` otherwise.
 */
bool
contains_substring_case_sensitive_none(char *string, char *substring)
{
	char *first_occurrence = strcasestr(string, substring);
	if (first_occurrence == NULL) {
		return false;
	}

	return true;
}

/*
 * Check if `string` contains `substring` taking into account the letter case.
 * Returns `true` if `substring` is found in `string`, `false` otherwise.
 */
bool
contains_substring_case_sensitive_full(char *string, char *substring)
{
	char *first_occurrence = strstr(string, substring);
	if (first_occurrence == NULL) {
		return false;
	}

	return true;
}

/*
 * Print the full path of the entity if it contains the `phrase` according to
 * the `contains_substring` parameter function.
 */
void
print_if_contains_substring(char *entity_name,
                            char *fullpath,
                            bool (*contains_substring)(char *, char *),
                            char *phrase)
{
	bool _contains_substring = (*contains_substring)(entity_name, phrase);

	if (_contains_substring) {
		printf("%s\n", fullpath);
	}
}

/*
 * Check if the `entity_name` is in the directory names blacklist `DIR_NAMES_BLACKLIST`.
 * Returns `true` if `entity_name` is blacklisted, `false` otherwise.
 */
bool
is_directory_blacklisted(char *entity_name)
{
	bool is_blacklisted = false;

	for (int i = 0; i < DIR_NAMES_BLACKLIST_SIZE; i++) {
		if (strncmp(entity_name,
		            DIR_NAMES_BLACKLIST[i],
		            DIR_NAMES_BLACKLIST_MAX_LEN * sizeof(char)) == 0) {
			is_blacklisted = true;
			break;
		}
	}

	return is_blacklisted;
}

/*
 * Build the full path of the entity given the `parent_path` and `entity_name`.
 * The result is stored in `fullpath`.
 */
void
build_fullpath(char fullpath[PATH_MAX], const char *parent_path, char *entity_name)
{
	if (strncmp(parent_path, WD_PATH_ALIAS, WD_PATH_ALIAS_LEN * sizeof(char)) ==
	    0) {
		snprintf(fullpath, (PATH_MAX - 1) * sizeof(char), "%s", entity_name);
		return;
	}

	snprintf(fullpath,
	         (PATH_MAX - 1) * sizeof(char),
	         "%s/%s",
	         parent_path,
	         entity_name);
}

/*
 * Recursively read the all the entities inside the DIR `directory` and print
 * the full path of each entity that contains the `phrase` in its name according
 * to the `contains_substring` parameter function. If the entity is a directory
 * and not blacklisted, it opens the directory and reads it recursively.
 * If the read of an entity fails, the process exits.
 */
void
read_directory(DIR *directory,
               const char *parent_path,
               bool (*contains_substring)(char *, char *),
               char *phrase)
{
	bool should_stop = false;
	struct dirent *entity = NULL;

	while (!should_stop) {
		read_entity_from_directory(directory, &entity);
		if (entity == NULL) {
			should_stop = true;
			break;
		}

		char fullpath[PATH_MAX] = { STRING_NULL_TERMINATOR };
		build_fullpath(fullpath, parent_path, entity->d_name);

		print_if_contains_substring(
		        entity->d_name, fullpath, contains_substring, phrase);

		if (entity->d_type == DT_DIR &&
		    !is_directory_blacklisted(entity->d_name)) {
			int inner_directory_fd = openat(dirfd(directory),
			                                entity->d_name,
			                                O_DIRECTORY);

			if (inner_directory_fd == GENERIC_ERROR_CODE) {
				perror("Failed to open directory");
				exit(EXIT_FAILURE);
			}

			DIR *inner_directory = fdopendir(inner_directory_fd);
			if (inner_directory == NULL) {
				perror("Failed to get DIR from directory FD");
				exit(EXIT_FAILURE);
			}

			read_directory(inner_directory,
			               fullpath,
			               contains_substring,
			               phrase);

			close_directory(inner_directory);
			close(inner_directory_fd);
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc > MAX_INPUT_PARAMS + 1 || argc < MIN_INPUT_PARAMS + 1) {
		fprintf(stderr, "Error while calling program. Expected %s [-i] <phrase>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char phrase[PATH_MAX] = { STRING_NULL_TERMINATOR };
	int case_sensitivity_code = CASE_SENSITIVITY_FULL_CODE;
	bool (*contains_substring)(char *, char *) =
	        &contains_substring_case_sensitive_full;

	parse_arguments(phrase, &case_sensitivity_code, argc, argv);

	if (case_sensitivity_code == CASE_SENSITIVITY_NONE_CODE) {
		contains_substring = &contains_substring_case_sensitive_none;
	}

	DIR *wd = opendir(WD_PATH_ALIAS);
	if (wd == NULL) {
		perror("Error while opening directory");
		exit(EXIT_FAILURE);
	}

	read_directory(wd, WD_PATH_ALIAS, contains_substring, phrase);
	close_directory(wd);

	exit(EXIT_SUCCESS);
}

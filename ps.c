#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

static const int MINIMUM_INPUT_PARAMS = 0;
static const char PROC_DIR_PATH[] = "/proc";
static const int CLOSE_DIR_ERROR_CODE = -1;

int
main(int argc, char *argv[])
{
	if (argc != MINIMUM_INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s call with no "
		        "extra arguments",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	DIR *proc_directory = opendir(PROC_DIR_PATH);
	if (proc_directory == NULL) {
		perror("Error while opening process directory");
		exit(EXIT_FAILURE);
	}

	struct dirent *entity = readdir(proc_directory);
	while (entity != NULL) {
		printf("%s\n", entity->d_name);
		entity = readdir(proc_directory);
	}

	int res = closedir(proc_directory);
	if (res == CLOSE_DIR_ERROR_CODE) {
		perror("Error while closing process directory");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

#define TEMP_BUFF_SIZE 50

static const int INPUT_PARAMS = 0;

static const char PROC_DIR_ABS_PATH[] = "/proc";
static const char COMM_FILEPATH_RELATIVE_TO_PID[] = "comm";
static const char LINE_BREAK = '\n', STRING_NULL_TERMINATOR = '\0';

static const int GENERIC_ERROR_CODE = -1;
static const int IS_DIGIT_TRUE = 0;
static const int SUCCESS = 0, FAILED = -1;

typedef struct process {
	size_t pid;
	char *cmd_name;
} process_t;

/*
 * Close `proc_directory`. If the closing fails the current process exits.
 */
void
close_process_directory(DIR *proc_directory)
{
	int res = closedir(proc_directory);

	if (res == GENERIC_ERROR_CODE) {
		perror("Error while closing process directory");
		exit(EXIT_FAILURE);
	}
}

/*
 * Read an entity to the `entity` parameter from proc_directory.
 * If the read finishes correctly `entity` is set to null.
 * If the read fails `entity` is set to null, `proc_directory` gets closed
 * and the current process exits.
 */
void
read_entity_from_directory(DIR *proc_directory, struct dirent **entity)
{
	errno = 0;
	*entity = readdir(proc_directory);
	if (errno != 0 && *entity == NULL) {
		perror("Error while reading process directory");
		close_process_directory(proc_directory);
		exit(EXIT_FAILURE);
	}
}

/*
 * Load `pid` with the PID contained in `entity`.
 */
void
load_pid(size_t *pid, struct dirent *entity)
{
	*pid = atoi(entity->d_name);
}

/*
 * Create a heap-allocated string with the absolute filepath of the comm file in
 * `comm_filepath` with the size saved in `comm_filepath_size`.
 * If the memory allocation fails, `FAILED` is returned, otherwise `SUCCESS` is returned.
 */
int
load_comm_filepath(char **comm_filepath, int *comm_filepath_size, size_t pid)
{
	int extra_buffer_size = 10;
	int pid_digits = snprintf(NULL, 0, "%lu", pid);
	*comm_filepath_size =
	        pid_digits + strlen(PROC_DIR_ABS_PATH) + extra_buffer_size;

	*comm_filepath = calloc(*comm_filepath_size, sizeof(char));
	if (*comm_filepath == NULL) {
		perror("Failed to allocate memory for comm filepath");
		return FAILED;
	}

	(*comm_filepath)[0] = '\0';
	snprintf(*comm_filepath,
	         *comm_filepath_size,
	         "%s/%lu/%s",
	         PROC_DIR_ABS_PATH,
	         pid,
	         COMM_FILEPATH_RELATIVE_TO_PID);
	return SUCCESS;
}

/*
 * Save the text content of the comm file pointed by the string `comm_filepath`
 * (with an summed null string terminator) into the newly heap-allocated
 * `cmd_name` string.
 * If the opening or the reading of `comm_filepath` fails, or the memory
 * allocation for `cmd_name` fails, `FAILED` is returned and `cmd_name` is
 * nulled, otherwise `SUCCESS` is returned.
 */
int
read_comm_file(char **cmd_name, char *comm_filepath)
{
	int position = 0;
	int comm_fd = open(comm_filepath, O_RDONLY);
	if (comm_fd == GENERIC_ERROR_CODE) {
		perror("Failed to open comm file form path");
		*cmd_name = NULL;
		return FAILED;
	}

	char temp_buff[TEMP_BUFF_SIZE];
	for (int i = 0; i < TEMP_BUFF_SIZE; i++) {
		temp_buff[i] = STRING_NULL_TERMINATOR;
	}

	*cmd_name = malloc(TEMP_BUFF_SIZE * sizeof(char));
	if (*cmd_name == NULL) {
		perror("Failed to allocate memory for cmd name");
		close(comm_fd);
		return FAILED;
	}
	(*cmd_name)[0] = '\0';

	int bytes_read = read(comm_fd, temp_buff, TEMP_BUFF_SIZE - 1);
	while (bytes_read == TEMP_BUFF_SIZE - 1 && *cmd_name != NULL) {
		strncat(*cmd_name, temp_buff, TEMP_BUFF_SIZE);
		position += bytes_read / sizeof(char);

		if (position >= TEMP_BUFF_SIZE - 1) {
			size_t new_size = ((position / TEMP_BUFF_SIZE) + 1) *
			                  TEMP_BUFF_SIZE * sizeof(char);
			char *cmd_name_aux = realloc(*cmd_name, new_size);

			if (cmd_name_aux == NULL) {
				free(*cmd_name);
				*cmd_name = NULL;
				break;
			}
			*cmd_name = cmd_name_aux;
		}

		bytes_read = read(comm_fd, temp_buff, TEMP_BUFF_SIZE - 1);
	}

	if (*cmd_name == NULL) {
		perror("Failed to allocate memory for cmd name");
		close(comm_fd);
		return FAILED;
	}

	if (bytes_read > 0) {
		strncat(*cmd_name, temp_buff, TEMP_BUFF_SIZE);
	} else if (bytes_read == GENERIC_ERROR_CODE) {
		perror("Failed to read comm file");
		close(comm_fd);
		free(*cmd_name);
		*cmd_name = NULL;
		return FAILED;
	}

	close(comm_fd);
	return SUCCESS;
}

/*
 * Load the text content of the comm file corresponding to the process with the
 * PID `pid` into the newly heap-allocated `cmd_name` string. If there is an
 * error processing the file of allocating memory, `FAILED` is returned and
 * `cmd_name` is nulled, otherwise, `SUCCESS` is returned.
 */
int
load_cmd_name(char **cmd_name, size_t pid)
{
	char *comm_filepath = NULL;
	int comm_filepath_size = 0;

	int res = load_comm_filepath(&comm_filepath, &comm_filepath_size, pid);
	if (res == FAILED) {
		return FAILED;
	}

	res = read_comm_file(cmd_name, comm_filepath);
	if (res == FAILED) {
		free(comm_filepath);
		return FAILED;
	}

	char *line_break_ptr = strchr(*cmd_name, LINE_BREAK);
	if (line_break_ptr != NULL) {
		*line_break_ptr = STRING_NULL_TERMINATOR;
	}
	free(comm_filepath);
	return SUCCESS;
}

/*
 * Determine if all the characters of `string` make up a number. A null string terminator is assumed
 * to be at the end of `string`.
 * Returns `true` if `string` is a number, returns `false` otherwise
 */
bool
is_number(char *string)
{
	int size = strlen(string);
	int i = 0;
	while (i < size) {
		char c = string[i];
		if (isdigit(c) == IS_DIGIT_TRUE) {
			return false;
		}
		i++;
	}

	return true;
}

/*
 * Free the heap-allocated memory for each entry of the vector `processes` and for the vector itself.
 * Afterwards, `processes` is nulled and `processes_size` is set to 0.
 */
void
free_processes_vector(process_t **processes, size_t *processes_size)
{
	for (size_t i = 0; i < *processes_size; i++) {
		free((*processes)[i].cmd_name);
	}

	free(*processes);
	*processes = NULL;
	*processes_size = 0;
}

/*
 * Add a process_t entry to the heap-allocated vector `processes` of size
 *`processes_size` from the information of `entity`. `processes`'s memory and
 *`processes_size` will grow accordingly. If extracting the command from the
 *comm file or allocating the memory for the expansion of `processes` fails,
 *`FAILED` is returned, otherwise `SUCCESS` is returned
 */
int
add_process(process_t **processes, size_t *processes_size, struct dirent *entity)
{
	size_t pid = 0;
	char *cmd_name = NULL;

	load_pid(&pid, entity);

	int res = load_cmd_name(&cmd_name, pid);
	if (res == FAILED) {
		return FAILED;
	}

	process_t new_process;
	new_process.pid = pid;
	new_process.cmd_name = cmd_name;

	process_t *processes_aux = NULL;
	if (*processes_size == 0) {
		processes_aux = malloc(sizeof(process_t));
	} else {
		size_t new_size = *processes_size + 1;
		processes_aux = realloc(*processes, new_size * sizeof(process_t));
	}

	if (processes_aux == NULL) {
		perror("Failed to allocate processes vector");
		free(cmd_name);
		return FAILED;
	}

	*processes = processes_aux;
	(*processes)[*processes_size] = new_process;
	(*processes_size)++;
	return SUCCESS;
}

/*
 * The return value is 0 if `process1` and `process2` have the same PID, 1 is
 * the PID of `process1` is greater than the PID of `process2`, or -1 otherwise.
 */
int
process_t_comparator(const void *process1, const void *process2)
{
	process_t _process1 = *((process_t *) process1);
	process_t _process2 = *((process_t *) process2);

	if (_process1.pid > _process2.pid) {
		return 1;
	} else if (_process1.pid < _process2.pid) {
		return -1;
	} else {
		return 0;
	}
}

/*
 * Sort the `processes` of size `processes_size` in ascending order by their PID.
 */
void
sort_vector_by_pid(process_t *processes, size_t processes_size)
{
	qsort(processes, processes_size, sizeof(process_t), process_t_comparator);
}

/*
 * Print the information of the process_t elements of `processes` of size
 * `processes_size` with spacing depending on the digit count of the PID number,
 * so the PID and the COMMAND are aligned with a center line.
 */
void
print_processes(process_t *processes, size_t processes_size)
{
	int max_pid = processes[processes_size - 1].pid;
	int max_spaces = snprintf(NULL, 0, "%d", max_pid);

	for (int i = 0; i < max_spaces - 2; i++) {
		printf(" ");
	}
	printf("PID COMMAND\n");

	for (size_t i = 0; i < processes_size; i++) {
		int spaces =
		        max_spaces - snprintf(NULL, 0, "%lu", processes[i].pid);

		for (int k = 0; k < spaces + 1; k++) {
			printf(" ");
		}

		printf("%lu %s\n", processes[i].pid, processes[i].cmd_name);
	}
}

int
main(int argc, char *argv[])
{
	if (argc != INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s call with no "
		        "extra arguments\n",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	DIR *proc_directory = opendir(PROC_DIR_ABS_PATH);
	if (proc_directory == NULL) {
		perror("Error while opening process directory");
		exit(EXIT_FAILURE);
	}

	struct dirent *entity = NULL;
	process_t *processes = NULL;
	size_t processes_size = 0;
	int res = SUCCESS;

	read_entity_from_directory(proc_directory, &entity);

	while (entity != NULL && res == SUCCESS) {
		if (is_number(entity->d_name)) {
			res = add_process(&processes, &processes_size, entity);

			if (res == FAILED) {
				break;
			}
		}

		read_entity_from_directory(proc_directory, &entity);
	}

	if (res == FAILED) {
		free_processes_vector(&processes, &processes_size);
		close_process_directory(proc_directory);
		exit(EXIT_FAILURE);
	}

	sort_vector_by_pid(processes, processes_size);
	print_processes(processes, processes_size);

	free_processes_vector(&processes, &processes_size);
	close_process_directory(proc_directory);
	exit(EXIT_SUCCESS);
}

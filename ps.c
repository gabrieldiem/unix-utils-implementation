#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>

#define TEMP_BUFF_SIZE 3

static const int MINIMUM_INPUT_PARAMS = 0;

static const char PROC_DIR_ABS_PATH[] = "/proc";
static const char COMM_FILEPATH_RELATIVE_TO_PID[] = "comm";
static const char LINE_BREAK = '\n', STRING_NULL_TERMINATOR = '\0';

static const int CLOSE_DIR_ERROR_CODE = -1;
static const int IS_DIGIT_TRUE = 0;
static const int SUCCESS = 0, FAILED = -1;

typedef struct process {
	size_t pid;
	char *cmd_name;
} process_t;


void
close_process_directory(DIR *proc_directory)
{
	int res = closedir(proc_directory);

	if (res == CLOSE_DIR_ERROR_CODE) {
		perror("Error while closing process directory");
		exit(EXIT_FAILURE);
	}
}

void
read_entity_from_directory(DIR *proc_directory, struct dirent **entity)
{
	errno = 0;
	*entity = readdir(proc_directory);
	if (*entity == NULL && errno != 0) {
		perror("Error while reading process directory");
		close_process_directory(proc_directory);
		exit(EXIT_FAILURE);
	}
}

void
load_pid(size_t *pid, struct dirent *entity)
{
	*pid = atoi(entity->d_name);
}

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

int
read_comm_file(char **cmd_name, char *comm_filepath)
{
	int comm_fd = open(comm_filepath, O_RDONLY);
	int position = 0;

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
		strcat(*cmd_name, temp_buff);
		position += bytes_read / sizeof(char);

		if (position >= TEMP_BUFF_SIZE - 1) {
			size_t new_size =
			        position * TEMP_BUFF_SIZE * sizeof(char);
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

	if (bytes_read != 0) {
		strcat(*cmd_name, temp_buff);
	}
	close(comm_fd);
	return SUCCESS;
}

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

bool
is_number(char *string)
{
	int size = strlen(string);
	int i = 0;
	while (i < size) {
		char c = string[i];
		if (isdigit(c) == IS_DIGIT_TRUE || c == STRING_NULL_TERMINATOR) {
			return false;
		}
		i++;
	}

	return true;
}

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

int
process_t_comparator(const void *elem1, const void *elem2)
{
	process_t process1 = *((process_t *) elem1);
	process_t process2 = *((process_t *) elem2);

	if (process1.pid > process2.pid) {
		return 1;
	} else if (process1.pid < process2.pid) {
		return -1;
	} else {
		return 0;
	}
}

void
sort_vector_by_pid(process_t *processes, size_t processes_size)
{
	qsort(processes, processes_size, sizeof(process_t), process_t_comparator);
}

void
print_processes(process_t *processes, size_t processes_size)
{
	int max_pid = processes[processes_size - 1].pid;
	int max_spaces = snprintf(NULL, 0, "%d", max_pid);

	for (int k = 0; k < max_spaces - 2; k++) {
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
	if (argc != MINIMUM_INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s call with no "
		        "extra arguments",
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

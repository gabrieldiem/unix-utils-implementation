#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define EXEC_ARGV_LEN 3

static const int MINIMUM_INPUT_PARAMS = 2, MAXIMUM_INPUT_PARAMS = 3;
static const int DURATION_INDEX_ARGV = 1, CMD_INDEX_ARGV = 2,
                 CMD_ARGS_INDEX_ARGV = 3;
static const int SIGNAL_NOT_SENT = -1;

void
handle_child_process(int child_pid, char *cmd, int cmd_duration)
{
	sleep(cmd_duration);

	int res = kill(child_pid, SIGTERM);
	if (res == SIGNAL_NOT_SENT) {
		fprintf(stderr, "Error while trying to terminate program: %s", cmd);
		perror("\n");
	}

	if (wait(NULL) < 0) {
		perror("Error on wait\n");
		exit(EXIT_FAILURE);
	}
}

void
run_command(char *cmd, char *cmd_args, int cmd_duration)
{
	char *exec_argv[EXEC_ARGV_LEN] = { NULL };
	exec_argv[0] = cmd;
	exec_argv[1] = cmd_args;

	pid_t child_id = fork();

	if (child_id < 0) {
		perror("Error while forking\n");
		exit(EXIT_FAILURE);

	} else if (child_id == 0) /* process is child */ {
		execvp(cmd, exec_argv);

		/* If this line is reached execvp returned, meaning that it failed */
		perror("Error from execvp\n");
		exit(EXIT_FAILURE);

	} else /* if process is parent */ {
		handle_child_process(child_id, cmd, cmd_duration);
	}
}

int
main(int argc, char *argv[])
{
	if (argc > MAXIMUM_INPUT_PARAMS + 1 || argc < MINIMUM_INPUT_PARAMS + 1) {
		fprintf(stderr,
		        "Error while calling program. Expected %s <max "
		        "duration in seconds> <command> <command argument>",
		        argv[0]);
		exit(EXIT_FAILURE);
	}

	int cmd_duration = atoi(argv[DURATION_INDEX_ARGV]);
	char *cmd = argv[CMD_INDEX_ARGV];
	char *cmd_args = NULL;

	if (argc == MAXIMUM_INPUT_PARAMS + 1) {
		cmd_args = argv[CMD_ARGS_INDEX_ARGV];
	}

	run_command(cmd, cmd_args, cmd_duration);

	exit(EXIT_SUCCESS);
}

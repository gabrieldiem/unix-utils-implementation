#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#define EXEC_ARGV_LEN 3
#define AUX_BUFF_SIZE 200
#define TIMER_SIGNAL (SIGRTMIN + 1)

static const int GENERIC_ERROR_CODE = -1;

static const int MINIMUM_INPUT_PARAMS = 2, MAXIMUM_INPUT_PARAMS = 3;
static const int DURATION_INDEX_ARGV = 1, CMD_INDEX_ARGV = 2,
                 CMD_ARGS_INDEX_ARGV = 3;
static const int SIGNAL_NOT_SENT = -1;

/*
 * Send a SIGTERM signal to the process with the PID `child_pid`.
 * If `child_pid` is NULL, the function returns.
 * If the kill operation fails, an error message is printed.
 * Waits for the child process to terminate.
 */
void
kill_process(int *child_pid)
{
	if (child_pid == NULL) {
		char err_msg[AUX_BUFF_SIZE] =
		        "Signal handler received a null pointer for PID\n";
		write(STDERR_FILENO, err_msg, AUX_BUFF_SIZE);
		fflush(stderr);
		return;
	}

	int res = kill(*child_pid, SIGTERM);
	if (res == SIGNAL_NOT_SENT) {
		char err_msg[AUX_BUFF_SIZE] =
		        "Error while trying to terminate program\n";
		write(STDERR_FILENO, err_msg, AUX_BUFF_SIZE);
		fflush(stderr);
	}

	char message[AUX_BUFF_SIZE] = "\nCommand timed out\n";
	write(STDOUT_FILENO, message, AUX_BUFF_SIZE);
	fflush(stdout);

	if (wait(NULL) < 0) {
		char err_msg[AUX_BUFF_SIZE] = "Error on wait\n";
		write(STDERR_FILENO, err_msg, AUX_BUFF_SIZE);
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
}


/*
 * Signal handler function that calls `kill_process` with the PID stored in `signal_info`.
 */
void
signal_handler_function(int, siginfo_t *signal_info, void *)
{
	kill_process((int *) (signal_info->si_value.sival_ptr));
}

/*
 * Create a timer that sends a SIGTERM signal to the process with PID `cmd_pid`.
 * If the timer creation fails the process exits.
 */
timer_t
create_timer(int *cmd_pid)
{
	struct sigaction signal_action;

	signal_action.sa_flags = SA_SIGINFO;
	signal_action.sa_sigaction = signal_handler_function;
	sigemptyset(&signal_action.sa_mask);

	int res = sigaction(TIMER_SIGNAL, &signal_action, NULL);
	if (res == GENERIC_ERROR_CODE) {
		perror("Failed to set up signal handler\n");
		exit(EXIT_FAILURE);
	}

	timer_t timerid;
	struct sigevent signal_event;

	signal_event.sigev_notify = SIGEV_SIGNAL;
	signal_event.sigev_signo = TIMER_SIGNAL;
	signal_event.sigev_value.sival_ptr = cmd_pid;

	res = timer_create(CLOCK_MONOTONIC, &signal_event, &timerid);
	if (res == GENERIC_ERROR_CODE) {
		perror("Failed to create timer\n");
		exit(EXIT_FAILURE);
	}

	return timerid;
}

/*
 * Arm the timer with id `timerid` for an expiration duration of `cmd_duration`.
 * If the timer setting fials the process exits.
 */
void
arm_timer(int *cmd_duration, timer_t *timerid)
{
	struct itimerspec timer_interval_specs;

	timer_interval_specs.it_value.tv_sec = *cmd_duration;
	timer_interval_specs.it_value.tv_nsec = 0;
	timer_interval_specs.it_interval.tv_sec =
	        timer_interval_specs.it_value.tv_sec;
	timer_interval_specs.it_interval.tv_nsec =
	        timer_interval_specs.it_value.tv_nsec;

	int res = timer_settime(*timerid, 0, &timer_interval_specs, NULL);
	if (res == GENERIC_ERROR_CODE) {
		perror("Failed to arm timer\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Wait for the child process to terminate.
 * If the wait is interrupted by a signal, the function returns.
 * If the wait fails for any other reason the process exits.
 */
void
wait_or_return_on_interrupt()
{
	if (wait(NULL) < 0) {
		int _errno = errno;
		if (_errno != EINTR) {
			perror("Error on wait\n");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * Arm the timer with an expiration time of `_cmd_duration` for the process
 * `_cmd_pid`, and wait for the child process to terminate. Deletes the timer
 * after the wait (useful if the command finishes before the timer expiration).
 */
void
arm_timer_and_wait(int _cmd_duration, int _cmd_pid)
{
	int cmd_pid = _cmd_pid;
	int *cmd_pid_ptr = &cmd_pid;
	int cmd_duration = _cmd_duration;
	int *cmd_duration_ptr = &cmd_duration;

	timer_t timerid = create_timer(cmd_pid_ptr);
	arm_timer(cmd_duration_ptr, &timerid);

	wait_or_return_on_interrupt();

	int res = timer_delete(timerid);
	if (res == GENERIC_ERROR_CODE) {
		perror("Failed to delete timer\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Run the command `cmd` with arguments `cmd_args` with a timeout of `cmd_duration` seconds.
 * Forks a child process to execute the command using exec.
 * The parent process arms the timer and waits for the child process to terminate.
 */
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
		arm_timer_and_wait(cmd_duration, (int) child_id);
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
	if (cmd_duration <= 0) {
		fprintf(stderr,
		        "Error while calling program. Expected <max duration "
		        "in seconds> argument to be greater than zero");
		exit(EXIT_FAILURE);
	}

	char *cmd = argv[CMD_INDEX_ARGV];
	char *cmd_args = NULL;

	if (argc == MAXIMUM_INPUT_PARAMS + 1) {
		cmd_args = argv[CMD_ARGS_INDEX_ARGV];
	}

	run_command(cmd, cmd_args, cmd_duration);

	exit(EXIT_SUCCESS);
}

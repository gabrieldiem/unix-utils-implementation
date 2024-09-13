#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define OPTIONAL_INPUT_PARAMS 1
#define MILISECONDS_TO_SLEEP 700L
#define AUX_BUFF_SIZE 100

int
main(int argc, char *argv[])
{
	if (argc != OPTIONAL_INPUT_PARAMS + 1 && argc != OPTIONAL_INPUT_PARAMS) {
		fprintf(stderr, "Error while calling program. Expected %s [optional message]", argv[0]);
		return -1;
	}

	char looping_message[AUX_BUFF_SIZE] = "looping\n";
	char custom_message[AUX_BUFF_SIZE] = { '\0' };

	char *message = NULL;
	if (argc == OPTIONAL_INPUT_PARAMS + 1) {
		message = argv[1];
		strncat(custom_message, argv[1], AUX_BUFF_SIZE - 1);
		strncat(custom_message, "\n", AUX_BUFF_SIZE - 1);
	}

	while (true) {
		int seconds = (int) (MILISECONDS_TO_SLEEP / 1000);
		long nanoseconds = MILISECONDS_TO_SLEEP * 1000000;

		struct timespec remaining;
		struct timespec duration = { seconds, nanoseconds };

		nanosleep(&duration, &remaining);

		if (message != NULL) {
			write(STDOUT_FILENO, custom_message, AUX_BUFF_SIZE);
			fflush(stdout);
		} else {
			write(STDOUT_FILENO, looping_message, AUX_BUFF_SIZE);
			fflush(stdout);
		}
	}

	return 0;
}

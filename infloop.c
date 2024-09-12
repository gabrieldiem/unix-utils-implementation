#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#define OPTIONAL_INPUT_PARAMS 1
#define MILISECONDS_TO_SLEEP 700L

int
main(int argc, char *argv[])
{
	if (argc != OPTIONAL_INPUT_PARAMS + 1 && argc != OPTIONAL_INPUT_PARAMS) {
		fprintf(stderr, "Error while calling program. Expected %s [optional message]", argv[0]);
		return -1;
	}

	char *message = NULL;
	if (argc == OPTIONAL_INPUT_PARAMS + 1) {
		message = argv[1];
	}

	while (true) {
		int seconds = (int) (MILISECONDS_TO_SLEEP / 1000);
		long nanoseconds = MILISECONDS_TO_SLEEP * 1000000;

		struct timespec remaining;
		struct timespec duration = { seconds, nanoseconds };

		nanosleep(&duration, &remaining);

		if (message != NULL)
			printf("%s\n", message);
		else
			printf("looping\n");
	}

	return 0;
}

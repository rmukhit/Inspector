# Project 1: System Inspector

See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-1.html 

My program has a main function that runs the whole program, it calls 4 functions: systemInformation(), hardwareInformation(),
taskSummary() and taskList() based on the flags that were passed as an an argument to the program.

I use functions readFile(char *filepath, char *buf) and *next_token(char **str_ptr, const char *delim) to read the file to
char array and tokenize it.

systemInformation() grabs info from different files to print hostname, linux version and uptime
Uptime is in seconds, then I get years, days, hours, minutes and seconds.

hardwareInformation() calls cpuModel(char *cpuModel), loadAver(char *loadAverage), cpuUsage(long int *result), memoryUsage(char *userPercentage);
These functions get information from different files and format it.

taskSummary() prints the number of tasks running, interrupts, context switches and processes.

taskList() prints the list of tasks and all the info about it (id, state, syscall name, username, num of tasks)


To compile and run:

	```bash
	make
	./inspector
	```

	## Testing

	To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

	```
	# Run all test cases:
	make test

	# Run a specific test case:
	make test run=4

	# Run a few specific test cases (4, 8, and 12 in this case):
	make test run='4 8 12'
	```
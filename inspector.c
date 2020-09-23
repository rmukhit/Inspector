#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

/* Preprocessor Directives */
#ifndef DEBUG
#define DEBUG 0
#endif
/**
 * Logging functionality. Set DEBUG to 1 to enable logging, 0 to disable.
 */
#define LOG(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
            __LINE__, __func__, __VA_ARGS__); } while (0)


/* Function prototypes */
void print_usage(char *argv[]);
char *next_token(char **str_ptr, const char *delim);
void systemInformation();
void hardwareInformation();
void cpuModel(char *cpuModel);
void loadAver(char *loadAverage);
void cpuUsage(long int *result);
void memoryUsage(char *userPercentage);
void taskList();
void taskSummary();
void readFile(char *filepath, char *buf);

/* This struct is a collection of booleans that controls whether or not the
 * various sections of the output are enabled. */
struct view_opts {
    bool hardware;
    bool system;
    bool task_list;
    bool task_summary;
};

void print_usage(char *argv[]) {
    printf("Usage: %s [-ahlrst] [-p procfs_dir]\n" , argv[0]);
    printf("\n");
    printf("Options:\n"
                   "    * -a              Display all (equivalent to -lrst, default)\n"
                   "    * -h              Help/usage information\n"
                   "    * -l              Task List\n"
                   "    * -p procfs_dir   Change the expected procfs mount point (default: /proc)\n"
                   "    * -r              Hardware Information\n"
                   "    * -s              System Information\n"
                   "    * -t              Task Information\n");
    printf("\n");
}

/* main func that runs the whole program
 *
 * */
int main(int argc, char *argv[]){

    /* Default location of the proc file system */
    char *procfs_loc = "/proc";

    /* Set to true if we are using a non-default proc location */
    bool alt_proc = false;

    struct view_opts all_on = { true, true, true, true };
    struct view_opts options = { false, false, false, false };

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "ahlp:rst")) != -1) {
        switch (c) {
            case 'a':
                options = all_on;
                break;
            case 'h':
                print_usage(argv);
                return 0;
            case 'l':
                options.task_list = true;
                break;
            case 'p':
                procfs_loc = optarg;
                alt_proc = true;
                break;
            case 'r':
                options.hardware = true;
                break;
            case 's':
                options.system = true;
                break;
            case 't':
                options.task_summary = true;
                break;
            case '?':
                if (optopt == 'p') {
                    fprintf(stderr,
                            "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n", optopt);
                }
                print_usage(argv);
                return 1;
            default:
                abort();
        }
    }

    if (alt_proc == true) {
        LOG("Using alternative proc directory: %s\n", procfs_loc);

        // change current working directory
        chdir(procfs_loc);

        /* Remove two arguments from the count: one for -p, one for the
         * directory passed in: */
        argc = argc - 2;

    } else {

        // if not using alternative directory, then just go to /proc
        chdir(procfs_loc);
    }

    if (argc <= 1) {
        /* No args (or -p only). Enable all options: */
        options = all_on;
    }

    if (options.system) {
        systemInformation();
    }

    if (options.hardware) {
        hardwareInformation();
    }

    if (options.task_summary) {
        taskSummary();
    }

    if (options.task_list) {
        taskList();
    }


    LOG("Options selected: %s%s%s%s\n",
        options.hardware ? "hardware " : "",
        options.system ? "system " : "",
        options.task_list ? "task_list " : "",
        options.task_summary ? "task_summary" : "");



    return 0;
}

/* readFile func reads file to char array
 * Parameters:
 * - path to file
 * - pointer to char array to which file will be written
 *
 * */
void readFile(char *filepath, char *buf) {

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    ssize_t read_sz;
    read_sz = read(fd, buf, 100000);

    close(fd);

    if (read_sz == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

};

/* systemInformation func that grabs files from /proc,
 * gets the needed info about system and prints this information
 *
 * */
void systemInformation() {

    // hostname
    char hostnameFile[100000];
    readFile("sys/kernel/hostname", hostnameFile);
    char *next_tok_hostname = hostnameFile;
    // take first word from file in /proc/sys/kernel/hostname
    char *hostname = next_token(&next_tok_hostname, "\n");

    //linux version
    char versionFile[100000];
    readFile("sys/kernel/osrelease", versionFile);
    char *next_tok_version = versionFile;
    // take first word from file in /proc/sys/kernel/osrelease
    char *version = next_token(&next_tok_version, "\n");

    //uptime
    char uptimeFile[100000];
    readFile("uptime", uptimeFile);
    char *next_tok_uptime = uptimeFile;
    // take first long int in /uptime file
    long int uptime = strtol(next_token(&next_tok_uptime, " "), NULL, 10);

    //now we need to convert seconds to years, days, mins and secs
    long int year = uptime / (24*3600*365);
    uptime = uptime % (24*3600*365);

    long int day = uptime / (24 * 3600);
    uptime = uptime % (24 * 3600);

    long int hour = uptime / 3600;
    uptime %= 3600;

    long int minutes = uptime / 60 ;
    uptime %= 60;

    long int seconds = uptime;

    //allocate memory for char array that will hold uptime info that is needed
    //to be printed
    char *resultUptime = malloc(200 * sizeof(char));
    strcpy(resultUptime, "Uptime: ");
    // variable for casting long int to char array
    char castToChar[20];

    // print years, days and hours ONLY if they are larger that 0
    if (year > 0) {
        sprintf(castToChar, "%ld", year);
        strcat(resultUptime, castToChar);
        strcat(resultUptime, " years, ");
    }
    if (day > 0) {
        sprintf(castToChar, "%ld", day);
        strcat(resultUptime, castToChar);
        strcat(resultUptime, " days, ");
    }
    if (hour > 0) {
        sprintf(castToChar, "%ld", hour);
        strcat(resultUptime, castToChar);
        strcat(resultUptime, " hours, ");
    }

    //print minutes and seconds even if they are equal to 0
    sprintf(castToChar, "%ld", minutes);
    strcat(resultUptime, castToChar);
    strcat(resultUptime, " minutes, ");
    sprintf(castToChar, "%ld", seconds);
    strcat(resultUptime, castToChar);
    strcat(resultUptime, " seconds");

    printf("System Information\n");
    printf("------------------\n");
    printf("Hostname: %s\n", hostname);
    printf("Kernel Version: %s\n", version);
    printf("%s\n\n", resultUptime);

    //free
    free(resultUptime);

}

/* hardwareInformation func that grabs files from /proc,
 * gets the needed info about hardware and prints this information
 *
 * */
void hardwareInformation() {

    //allocate memory for char array that will hold cpu model info and pass it to cpuModel func
    char *modelCpu = malloc(25 *sizeof(char));
    cpuModel(modelCpu);

    // get num of processing units
    char cpuInfoFile[100000];
    readFile("stat", cpuInfoFile);
    char *next_tok = cpuInfoFile;
    char *curr_tok;
    int numOfCPUs = 0;

    while ((curr_tok = next_token(&next_tok, " :\n\0,?!")) != NULL) {
        //increase numOfCpus with each "cpu" word in /stat file
        if (strncmp(curr_tok, "cpu", 3) == 0) {
            numOfCPUs++;
        }

    }
    // decrease numOfCpus by 1 because first "cpu" in stat is general cpu
    numOfCPUs--;

    //allocate memory for load avg char array and pass it to func loadAver
    char *loadAvg = malloc(100 *sizeof(char));
    loadAver(loadAvg);

    //allocate memory for memory usage char array and pass it to func memoryUsage
    char *usageMem = malloc(100 * sizeof(char));
    memoryUsage(usageMem);

    //get array containing total and idle then sleep for 1 sec and get total and idle time again
    long int *time1 = malloc(2* sizeof(long int));
    cpuUsage(time1);
    sleep(1);
    long int *time2 = malloc(2* sizeof(long int));
    cpuUsage(time2);
    float usageCpu;

    //if they are equal then 0
    if (time1[0] == time2[0] && time1[1] == time2[1]) {

        usageCpu = 0;

    } else {

        // calculate cpu usage by formula usage = 1 - ((idle2 - idle1) / (total2 - total1))
        long int total = time2[0] - time1[0];
        long int idle = time2[1] - time1[1];

        float idlePercent = idle/total;
        usageCpu = 1 - idlePercent;

    }

    //allocate memory for char array containing nicely formatted info about cpu usage
    char *printUsageCpu = malloc(50 * sizeof(char));
    char castToChar[20];

    strcpy(printUsageCpu, "CPU Usage: [");

    // for making right percantage of # and -
    int hashtags = (int)usageCpu/5;
    int dashes = 20 - hashtags;

    for (int i = 0; i < hashtags; i++) {
        strcat(printUsageCpu,"#");
    }
    for (int i = 0; i < dashes; i++) {
        strcat(printUsageCpu,"-");
    }

    strcat(printUsageCpu, "] ");
    sprintf(castToChar, "%0.1f", usageCpu);
    strcat(printUsageCpu, castToChar);
    strcat(printUsageCpu, "%");

    printf("Hardware Information\n");
    printf("--------------------\n");
    printf("CPU Model: %s\n", modelCpu);
    printf("Processing Units: %d\n", numOfCPUs);
    printf("Load Average (1/5/15 min): %s\n", loadAvg);
    printf("%s\n", printUsageCpu);
    printf("%s\n\n", usageMem);

    // free all allocated memory
    free(modelCpu);
    free(loadAvg);
    free(usageMem);
    free(printUsageCpu);
    free(time1);
    free(time2);

}

/* cpuModel func that grabs info from cpuinfo file in proc and writes it to char array
 * Parameters:
 * - pointer to char array to which cpu model will be written
 *
 * */
void cpuModel(char *cpuModel) {
    char cpuFile[100000];
    readFile("cpuinfo", cpuFile);
    char *next_tok = cpuFile;
    char *curr_tok;
    bool modelNameFound = false;
    int countOfWords = 0;

    while ((curr_tok = next_token(&next_tok, " ,?!:\t\n")) != NULL) {
        // cpu model name is after the word "name" and before word "stepping"
        if (modelNameFound) {
            // count the words to put whitespaces
            countOfWords++;
            // if "stepping" is reached, then break
            if(strcmp(curr_tok, "stepping") == 0) {
                break;
            }
            if (countOfWords > 1) {
                strcat(cpuModel, " ");
            }
            strcat(cpuModel, curr_tok);

        }
        if (strcmp(curr_tok, "name") == 0) {
            modelNameFound = true;
        }

    }
}

/* loadAver func that grabs info from loadavg file in proc and writes it to char array
 * Parameters:
 * - pointer to char array to which load average will be written
 *
 * */
void loadAver(char *loadAverage) {
    char loadAvgFile[100000];
    readFile("loadavg", loadAvgFile);
    char *next_tok = loadAvgFile;
    char *curr_tok;
    int count = 0;

    while ((curr_tok = next_token(&next_tok, " \0")) != NULL) {

        // only first three tokens in loadavg file are needed
        if (count >= 3) {
            break;
        } else {
            if (count >= 1) {
                strcat(loadAverage, " ");
            }
            strcat(loadAverage, curr_tok);
            count++;
        }

    }
}

/* cpuUsage func that grabs info from stat file in proc and writes it to long int array
 * Parameters:
 * - pointer to long int array to which total and idle will be written
 *
 * */
void cpuUsage(long int *result) {
    char cpuInfoFile[100000];
    readFile("stat", cpuInfoFile);
    char *next_tok = cpuInfoFile;
    char *curr_tok;
    int count = 0;
    // get all the fields
    long int user = 0;
    long int nice = 0;
    long int system = 0;
    long int idle = 0;
    long int iowait = 0;
    long int irq = 0;
    long int softirq = 0;
    long int steal = 0;
    long int guest = 0;
    long int guestnice = 0;

    while ((curr_tok = next_token(&next_tok, " :\n\0,?!")) != NULL) {
        if (count == 0) {
            count++;
        } else {
            if (count == 12) {
                break;
            }
            if (count == 1) {
                user = strtol(curr_tok, NULL, 10);
            } else if (count == 2) {
                nice = strtol(curr_tok, NULL, 10);
            } else if (count == 3) {
                system = strtol(curr_tok, NULL, 10);
            } else if (count == 4) {
                idle = strtol(curr_tok, NULL, 10);
            } else if (count == 5) {
                iowait = strtol(curr_tok, NULL, 10);
            } else if (count == 6) {
                irq = strtol(curr_tok, NULL, 10);
            } else if (count == 7) {
                softirq = strtol(curr_tok, NULL, 10);
            } else if (count == 8) {
                steal = strtol(curr_tok, NULL, 10);
            } else if (count == 9) {
                guest = strtol(curr_tok, NULL, 10);
            } else if (count == 10) {
                guestnice = strtol(curr_tok, NULL, 10);
            }
            count++;
        }
    }

    

    // calculate total
    long int total = user + nice + system + idle + iowait + irq + softirq + steal + guest + guestnice;

    //write total and idle as elements of array
    result[0] = total;
    result[1] = idle;

}

/* memoryUsage func that grabs info from meminfo file in proc and writes it to char array in a nice format
 * Parameters:
 * - pointer to char array to which memory usage will be written
 *
 * */
void memoryUsage(char *userPercentage) {
    char memInfoFile[100000];
    readFile("meminfo", memInfoFile);
    char *next_tok = memInfoFile;
    char *curr_tok;
    float memTotal = 0;
    float active = 0;

    while ((curr_tok = next_token(&next_tok, " :\n\0,?!")) != NULL) {

        // get memtotal and active
        if (strcmp(curr_tok, "MemTotal") == 0) {
            memTotal = strtof(next_token(&next_tok, " ,?!"), NULL);
        } else if (strcmp(curr_tok, "Active") == 0) {
            active = strtof(next_token(&next_tok, " ,?!"), NULL);
        }
    }

    //count how much it is in GB
    float totalGB = memTotal/1048576;
    float activeGB = active/1048576;

    //count percentage of mem usage
    float percentage = active/memTotal * 100;

    char castToChar[30];

    strcpy(userPercentage, "Memory Usage: [");

    int hashtags = (int)percentage/5;
    int dashes = 20 - hashtags;

    for (int i = 0; i < hashtags; i++) {
        strcat(userPercentage,"#");
    }
    for (int i = 0; i < dashes; i++) {
        strcat(userPercentage,"-");
    }
    strcat(userPercentage, "] ");
    sprintf(castToChar, "%0.1f", percentage);
    strcat(userPercentage, castToChar);
    strcat(userPercentage, "% (");
    sprintf(castToChar, "%0.1f", activeGB);
    strcat(userPercentage, castToChar);
    strcat(userPercentage, " GB / ");
    sprintf(castToChar, "%0.1f", totalGB);
    strcat(userPercentage, castToChar);
    strcat(userPercentage, " GB)");

}

/**
 * taskSummary counts the number of all digit folders in proc
 * gets info from stat file and prints all the info
 */
void taskSummary() {

    int numOfTasks = 0;

    //open the directory
    DIR *directory;

    if ((directory = opendir(".")) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        size_t len = strlen(entry->d_name);
        int i;
        // check if directory name is all digit
        for (i = 0; i < len; i++) {
            if (!isdigit(entry->d_name[i])) {
                break;
            }
        }

        bool allInt = i ? true : false;

        //if directory name is all digit and it is a folder then increment num of tasks
        if (allInt && entry->d_type == DT_DIR) {
            numOfTasks++;
        }
    }
    //close dir
    closedir(directory);

    //get num of interrupts, contSwitches and forks from stat file
    char buf[100000];
    readFile("stat", buf);
    char *next_tok = buf;
    char *curr_tok;
    long int interrupts;
    long int contSwitches;
    long int forks;

    while ((curr_tok = next_token(&next_tok, " \n\0,?!")) != NULL) {

        //tokens after intr, ctxt and processes are the info that we need
        if (strcmp(curr_tok, "intr") == 0) {
            interrupts = strtol(next_token(&next_tok, " ,?!"), NULL, 10);
        } else if (strcmp(curr_tok, "ctxt") == 0) {
            contSwitches = strtol(next_token(&next_tok, " ,?!"), NULL, 10);
        } else if (strcmp(curr_tok, "processes") == 0) {
            forks = strtol(next_token(&next_tok, " ,?!"), NULL, 10);
        }
    }

    //print everything
    printf("Task Information\n");
    printf("----------------\n");
    printf("Tasks running: %d\n", numOfTasks);
    printf("Since boot:\n" );
    printf("\tInterrupts: %ld\n", interrupts);
    printf("\tContext Switches: %ld\n", contSwitches);
    printf("\tForks: %ld\n\n", forks);

}

/**
 * taskList prints the task list: pid, state, task name, user and num of tasks
 *
 */
void taskList() {

    //open /proc directory
    DIR *directory;

    if ((directory = opendir(".")) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    printf("%5s | %12s | %25s | %15s | %s \n", "PID", "State", "Task Name", "User", "Tasks");
    printf("------+--------------+---------------------------+-----------------+-------\n");

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        size_t len = strlen(entry->d_name);
        int i;
        for (i = 0; i < len; i++) {
            if (!isdigit(entry->d_name[i])) {
                break;
            }
        }

        bool allInt = i ? true : false;

        //if file in /proc has all digit name and is a folder
        if (allInt && entry->d_type == DT_DIR) {
            // open /tasks directory and count all the all digit folder inside to count num of tasks
            int taskCount = 0;
            char *taskPath = malloc(strlen(entry->d_name) + strlen("/task") + 1);
            strcpy(taskPath, entry->d_name);
            strcat(taskPath, "/task");

            DIR *dir;

            if ((dir = opendir(taskPath)) != NULL) {
                struct dirent *tasks;
                while ((tasks = readdir(dir)) != NULL) {
                    size_t len = strlen(tasks->d_name);
                    int i;
                    for (i = 0; i < len; i++) {
                        if (!isdigit(tasks->d_name[i])) {
                            break;
                        }
                    }
                    bool allInteger = i ? true : false;
                    // only if name is all digit and a folder
                    if (allInteger && tasks->d_type == DT_DIR) {
                        taskCount++;
                    }
                }
            }

            closedir(dir);

            //get info about each task in /proc/[pid]/stat
            char *pidPath = malloc(strlen(entry->d_name) + strlen("/stat") + 1);
            strcpy(pidPath, entry->d_name);
            strcat(pidPath, "/stat");
            char statFile[100000];
            readFile(pidPath, statFile);
            char *next_tok = statFile;
            char *curr_tok;
            int count = 0;
            char *sysCall = calloc(25, sizeof(char));
            char *tempState = malloc(3 * sizeof(char));
            while ((curr_tok = next_token(&next_tok, " ()")) != NULL) {
                if (count < 1) {
                    count++;
                } else if (count < 3) {
                    // task name
                    if (count == 1) {
                        //if larger than 25 bytes
                        if (strlen(curr_tok) > 25) {
                            strncpy(sysCall, curr_tok, 25);
                        } else {
                            strcpy(sysCall, curr_tok);
                        }
                    }
                    // state
                    if (count == 2) {
                        strcpy(tempState, curr_tok);
                    }
                    if (count >= 3) {
                        break;
                    }
                    count++;
                }
            }

            // get process name
            struct stat *buf = malloc(100* sizeof(struct stat));
            stat(entry->d_name, buf);
            char *userName = calloc(15, sizeof(char));
            struct passwd *pwd = getpwuid(buf->st_uid);

            // if larger than 15 bytes
            if (strlen(pwd->pw_name) > 15) {
                strncpy(userName, pwd->pw_name, 15);
            } else {
                strcpy(userName, pwd->pw_name);
            }


            char state[15];

            // proper state name
            if (strcmp(tempState, "S") == 0) {
                strcpy(state, "sleeping");
            } else if (strcmp(tempState, "R") == 0) {
                strcpy(state, "running");
            } else if (strcmp(tempState, "I") == 0) {
                strcpy(state, "idle");
            } else if (strcmp(tempState, "X") == 0) {
                strcpy(state, "dead");
            } else if (strcmp(tempState, "Z") == 0) {
                strcpy(state, "zombie");
            } else if (strcmp(tempState, "T") == 0) {
                strcpy(state, "tracing stop");
            } else if (strcmp(tempState, "D") == 0) {
                strcpy(state, "disk sleep");
            }

            printf("%5s | %12s | %25s | %15s | %d \n", entry->d_name, state, sysCall, userName, taskCount);

            //free allocated memory
            free(buf);
            free(sysCall);
            free(tempState);
            free(userName);
            free(taskPath);
            free(pidPath);

        }

    }

    closedir(directory);

}

/**
 * Retrieves the next token from a string.
 *
 * Parameters:
 * - str_ptr: maintains context in the string, i.e., where the next token in the
 *   string will be. If the function returns token N, then str_ptr will be
 *   updated to point to token N+1. To initialize, declare a char * that points
 *   to the string being tokenized. The pointer will be updated after each
 *   successive call to next_token.
 *
 * - delim: the set of characters to use as delimiters
 *
 * Returns: char pointer to the next token in the string.
 */
char *next_token(char **str_ptr, const char *delim)  {
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  <= 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}

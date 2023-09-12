
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>
#include <time.h>
#define BUFSIZE 512
#define TARGET 0
#define SOLUTION 1
#define READ_END 0
#define WRITE_END 1
char* testPath;
char* targetSource;
char* solutionSorce;
int timeLimit;
pid_t target_pid, solution_pid;
int exitStat;

//get information from argc, argv and parse it 
void getArgs(int argc, char** argv){
    if(argc < 9) goto EXIT;
    struct option long_options[] = {
        {"input", required_argument, 0, 'i'},
        {"timeout",  required_argument, 0,  't'},
        {"solution"  ,  required_argument, 0,  's'},
        {"target",  required_argument, 0,  'h'}
    };
    
    int c, option_index = 0 ;
    while((c = getopt_long(argc, argv, "i:t:s:h:", long_options, &option_index)) != -1){
		// printf("%c :%s\n", c, optarg);
        switch (c){
        case 'i': //input
            testPath = strdup(optarg);
            break;
        case 't': //timeout
            timeLimit = atoi(optarg);
            break;
        case 's': //solution
			solutionSorce = strdup(optarg);
			break;
        case 'h': //target
            targetSource = strdup(optarg);
            break;
		case '?': goto EXIT;
		}
    } return;

    EXIT:
    printf("Usage: client [options]\n"
		"Options:\n"
		"  -i, --input     The directory path that test input files are stored.\n"
		"  -t, --timeout   The time limit of program execution(1-10)\n"
		"  -s, --solution  The solution source file.\n"
		"  -h, --target    The target source file.(homework submitted)\n");
	exit(EXIT_FAILURE);

}

//return 0 on success, return 1 on failure
int compileSource(char* soruce, int mode){
    pid_t pid = fork();
    int exitStat;

    if(pid > 0){ //parent
        waitpid(pid, &exitStat, 0);
        if (WIFEXITED(exitStat)) return 0; //exit normaly
        else {
            fprintf(stderr, "   error during compile\n");
            return 1; //exit with error
        }
    }else if (pid == 0){ // child
        close(2);
        close(1);
        if(mode == TARGET) execlp("gcc", "gcc", "-o", "target", soruce, (char *) NULL);
        if(mode == SOLUTION) execlp("gcc", "gcc", "-o", "solution", soruce, (char *) NULL);
        exit(EXIT_FAILURE);
    }else{ //fork error
        printf("cannot fork");
        return 1;
    }
    //./a -i test -t 10 -s solution.c -h target.c
}

//return 0 success, return 1 on failure.
int execWithInput(char* input, int mode){

    // open exe file
    int exec_fd = open("target", O_RDONLY);
    if(exec_fd == -1){
        fprintf(stderr, "   execWithInput: cannot open exec file\n");
        return 1;
    }

    // open input file and use it as stdin (pipe and dup2)
    int input_fd = open(input, O_RDONLY);
    dup2(input_fd, STDIN_FILENO);
    close(input_fd);

    //limit num of fd
    struct rlimit rlp;
    if(getrlimit(RLIMIT_NOFILE, &rlp) == -1) exit(EXIT_FAILURE);
    rlp.rlim_cur = 4; //default 3 + exec one
    if(setrlimit(RLIMIT_NOFILE, &rlp) == -1) exit(EXIT_FAILURE);
    
    //exec...?
    if(mode == TARGET){
        if(execl("target", "target", (char *) NULL) == -1){
            fprintf(stderr, "   execWithInput: cannot exec target\n");
            return 1;
        }else return 0;
    }
    else if(mode == SOLUTION){
        if(execl("solution", "solution", (char *) NULL) == -1){
            fprintf(stderr, "   execWithInput: cannot exec solution\n");
            return 1;
        }else return 0;
    }
    else {
        printf("execWithInput: invalid input\n");
        return 1;
    }
        
}

//handle the signal from SIGALARM
void timeOver(int signo){
    if(WIFSTOPPED(exitStat)){
        // printf("%d: dead\n", target_pid);
    }else {
        //printf("%d: alive\n", target_pid);
        kill(target_pid, SIGKILL);
        fprintf(stderr, "   time over: process terminated\n");
    };
}

int main(int argc, char** argv){

    struct timespec start, fin;
    double maxExecTime = 0, minExecTime = __INT_MAX__, sumExecTime = 0;
    int passedCount = 0, failedCount = 0;
    
    getArgs(argc, argv); //get args
    signal(SIGALRM, timeOver); //set the signal handler

    //compile
    if(compileSource(targetSource, TARGET) || compileSource(solutionSorce, SOLUTION)) {
        exit(EXIT_FAILURE);
    }

    //open directory
    DIR* inputDir = NULL;
    struct dirent* one = NULL;
    char inputFilePath[BUFSIZE];
    if((inputDir = opendir(testPath)) == NULL){
        perror("    cannot open directory");
        exit(EXIT_FAILURE);
    }

    //for each input file per loop... 
    int index = 1;
    while(one = readdir(inputDir)){
        if(one->d_type != DT_REG) continue;
        char oneFile[BUFSIZE];
        sprintf(oneFile, "%s/%s", testPath, one->d_name);
        int isPassed = 1;
        int target_fd[2], solution_fd[2];

        //make pipe
        if((pipe(target_fd) == -1) || (pipe(solution_fd) == -1)){
            perror("    cannot make pipe");
            exit(EXIT_FAILURE);
        }
       
        // exec solution
        solution_pid = fork();
        if(solution_pid == 0){ //child
            close(solution_fd[READ_END]);
            dup2(solution_fd[WRITE_END], STDOUT_FILENO);
            close(solution_fd[WRITE_END]);
            if(execWithInput(oneFile, SOLUTION)){
                fprintf(stderr, "   error during exec solution file. case: %s\n", oneFile);
                isPassed = 0;
            }
        }else if(solution_pid > 0){ //parent
            close(solution_fd[WRITE_END]);
        }else{
            fprintf(stderr, "   cannot fork solution\n");
            exit(EXIT_FAILURE);
        }

        // exec target
        target_pid = fork();
        clock_gettime(CLOCK_REALTIME, &start);
        if(target_pid == 0){ //child
            close(target_fd[READ_END]);
            dup2(target_fd[WRITE_END], STDOUT_FILENO);
            close(target_fd[WRITE_END]);
            if(execWithInput(oneFile, TARGET)){
                fprintf(stderr, "   error during exec target file. case: %s\n", oneFile);
                isPassed = 0;
            }
        }else if(target_pid > 0){ //parent
            alarm(10); // kill the chiild on time
            close(target_fd[WRITE_END]);
        }else{
            fprintf(stderr, "   cannot fork target\n");
            exit(EXIT_FAILURE);
        }

        //wait and check the exit status
        waitpid(solution_pid, NULL, 0);
        waitpid(target_pid, &exitStat, 0);
        //evaluate the time
        clock_gettime(CLOCK_REALTIME, &fin);
        double execTime = (fin.tv_sec - start.tv_sec)*1000.0 + (fin.tv_nsec - start.tv_nsec)/1000000.0;
        
        if(!(WIFEXITED(exitStat) && (WEXITSTATUS(exitStat)== EXIT_SUCCESS))) {
            fprintf(stderr, "   error during exec target file. case: %s\n", oneFile);
            isPassed = 0;
        }
        
        //TODO need to using tmpfile() ...??
        char solution_buf[BUFSIZE], target_buf[BUFSIZE];
        int len = 0;
        while((len = read(solution_fd[READ_END], solution_buf, BUFSIZE)) != 0){
            solution_buf[len] = 0;
            len = read(target_fd[READ_END], target_buf, BUFSIZE);
            target_buf[len] = 0;
            if(strcmp(solution_buf, target_buf)){
                isPassed = 0;
                // printf("#%s (solution)\n#%s (target)\n", solution_buf, target_buf);
                break;
            }
        }
        close(solution_fd[READ_END]);
        close(target_fd[READ_END]);

        if(isPassed) {
            printf("[Test%d] passed!\n", index++);
            passedCount++;
        }else {
            printf("[Test%d] failed...\n", index++); 
            failedCount++;
        }
        // 시간누적계산, min/max update 
        minExecTime = (minExecTime > execTime) ? execTime:minExecTime;
        maxExecTime = (maxExecTime < execTime) ? execTime:maxExecTime;
        sumExecTime += execTime;
        
    }closedir(inputDir);

    //display the summary
    if( failedCount == 0) printf(" ============= Success =============\n");
    else printf(" ============= Failed =============\n");
    printf("[summary of \"%s\"]\n", targetSource);
    printf("passed input: %d\n", passedCount);
    printf("failed input: %d\n", failedCount);
    printf("maximum execution time: %lf msec\n", maxExecTime);
    printf("minimum execution time: %lf msec\n", minExecTime);
    printf("total execution time: %lf msec\n", sumExecTime);
    printf(" ==================================\n");

    free(testPath);
    free(targetSource);
    free(solutionSorce);
    printf("done...\n");
    return 0;

}



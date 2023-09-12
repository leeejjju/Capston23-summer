#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/resource.h>

//https://www.ibm.com/docs/ko/i/7.3?topic=functions-tmpfile-create-temporary-file
//https://damool8.tistory.com/907
//https://reakwon.tistory.com/104

#define BUFSIZE 512
#define TARGET 0
#define SOLUTION 1

char* testPath;
char* targetSource;
char* solutionSorce;
int timeLimit;

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

    if(pid == 0){ //parent
        waitpid(pid, &exitStat, 0);
        if WIFEXITED(exitStat) return 0; //exit normaly
        else {
            printf("error during compile\n");
            return 1; //exit with error
        }
    }else if (pid > 0){ // child
        if(mode == TARGET) execlp("gcc", "gcc", "-o", "target", soruce, (char *) NULL);
        if(mode == SOLUTION) execlp("gcc", "gcc", "-o", "solution", soruce, (char *) NULL);
        exit(EXIT_SUCCESS);
    }else{ //fork error
        printf("cannot fork");
        return 1;
    }
    //./a -i test -t 10 -s solution.c -h target.c
}

//return fd success, return -1 on failure.
int execWithInput(char* input, int mode){
  
    // open exe file
    int exec_fd = open("target", O_RDONLY);
    if(exec_fd == -1){
        fprintf(stderr, "execWithInput: cannot open exec file\n");
        exit(EXIT_FAILURE);
    }

    // open input file and use it as stdin (pipe and dup2)
    int input_fd = open(input, O_RDONLY);
    input_fd = dup2(input_fd, STDIN_FILENO);

    //TODO set the stdout a tmp file

    //TODO limit num of fd 
    // struct rlimit* rlp;
    // getrlimit(RLIMIT_NOFILE, rlp);
    // if(setrlimit(RLIMIT_NOFILE, rlp) == -1){
    //     perror("execWithInput: cannot set limit");
    //     exit(EXIT_FAILURE);
    // }
    
    //exec...?
    if(mode == TARGET){
        if(execl("target", "target", (char *) NULL) == -1){
            fprintf(stderr, "execWithInput: cannot exec target\n");
            exit(EXIT_FAILURE);
        }
    } 
    else if(mode == SOLUTION){
        if(execl("solution", "solution", (char *) NULL) == -1){
            fprintf(stderr, "execWithInput: cannot exec solution\n");
            exit(EXIT_FAILURE);
        }
    }
    else printf("execWithInput: invalid input\n");

    printf("##########################################im alive.\n");
    exit(EXIT_SUCCESS); //! is it needed?
        
}

int main(int argc, char** argv){
    int maxExecTime = 0, minExecTime = __INT_MAX__, sumExecTime = 0;
    int passedCount = 0, failedCount = 0;
    getArgs(argc, argv);

    //compile
    if(compileSource(targetSource, TARGET) || compileSource(solutionSorce, SOLUTION)) {
        fprintf(stderr, "[error] cannot compile\n");
        exit(EXIT_FAILURE);
    }

    //open directory
    DIR* inputDir = NULL;
    struct dirent* one = NULL;
    char inputFilePath[BUFSIZE];
    if((inputDir = opendir(testPath)) == NULL){
        perror("cannot open directory");
        exit(EXIT_FAILURE);
    }

    //for each input file per loop... 
    int index = 1;
    while(one = readdir(inputDir)){
        if(one->d_type != DT_REG) continue;
        char oneFile[BUFSIZE];
        sprintf(oneFile, "%s/%s", testPath, one->d_name);
        int isPassed = 1;
        double execTime = 0;
        int target_fd, solution_fd;
        pid_t target_pid, solution_pid;

        // exec solution
        solution_pid = fork();
        if((solution_pid > 0) && (solution_fd = (execWithInput(oneFile, SOLUTION))) == -1){
            fprintf(stderr, "error during exec solution file. case: %s\n", oneFile);
            isPassed = 0;
        }
        //exec target
        target_pid = fork();
        if((target_pid > 0) && (target_fd = (execWithInput(oneFile, TARGET))) == -1){
            fprintf(stderr, "error during exec target file. case: %s\n", oneFile);
            isPassed = 0;
        }

        //TODO evaluate the time
        //TODO kill the chiild on time
        
        //wait and check the exit status
        int exitStat;
        waitpid(solution_pid, NULL, 0);
        waitpid(target_pid, &exitStat, 0);
        //! whyrano 
        // if(!(WIFEXITED(exitStat) && (WEXITSTATUS(exitStat)== EXIT_SUCCESS))) {
        //     fprintf(stderr, "error during exec target file. case: %s\n", oneFile);
        //     isPassed = 0;
        // }
        

        // TODO 둘이 비교하기 -> 틀리면 isPassed = 0;
        if( /*compair two || */ isPassed) {
            printf("[Test%d] passed!\n", index++);
            passedCount++;
        }else {
            printf("[Test%d] failed...\n", index++); 
            failedCount++;
        }

        //TODO close the tmp files

        // 시간누적계산, min/max update 
        minExecTime = (minExecTime > execTime) ? execTime:minExecTime;
        maxExecTime = (maxExecTime < execTime) ? execTime:maxExecTime;
        sumExecTime += execTime;
        
    }

    //display the summary
    //TODO set the final condition
    if( failedCount == 0) printf(" ========== Success ==========\n");
    else printf(" ========== Failed ==========\n");
    printf("[summary of \"%s\"]\n", targetSource);
    printf("passed input: %d\n", passedCount);
    printf("failed input: %d\n", failedCount);
    printf("maximum execution time: %d\n", maxExecTime);
    printf("minimum execution time: %d\n", minExecTime);
    printf("total execution time: %d\n", sumExecTime);
    printf(" ============================\n");

    free(testPath);
    free(targetSource);
    free(solutionSorce);
    printf("done...\n");
    return 0;

}
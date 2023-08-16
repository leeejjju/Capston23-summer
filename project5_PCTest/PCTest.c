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
    
    int c ;
    int option_index = 0 ;

    while((c = getopt_long(argc, argv, "i:t:s:h:", long_options, &option_index)) != -1){
		printf("%c :%s\n", c, optarg);
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
    }

    return;

    EXIT:
    printf("Usage: client [options]\n"
		"Options:\n"
		"  -i, --input     The directory path that test input files are stored.\n"
		"  -t, --timeout   The time limit of program execution(1-10)\n"
		"  -s, --solution  The solution source file.\n"
		"  -h, --target    The target source file.(homework submitted)\n");
	exit(EXIT_FAILURE);

}

//return 1 on success, return 0 on failure
int compileSource(char* soruce, int mode){

    pid_t pid = fork();
    int exitStat;

    if(pid == 0){ //parent
        waitpid(pid, &exitStat, 0);
        if WIFEXITED(exitStat) return 0;
        else return 1;
    }else if (pid > 0){ // child
        if(mode == TARGET) execlp("gcc", "gcc", "-o", "target", soruce, (char *) NULL);
        if(mode == SOLUTION) execlp("gcc", "gcc", "-o", "solution", soruce, (char *) NULL);
        exit(EXIT_SUCCESS);
    }else{ //error
        printf("cannot fork");
        return 1;
    }

}

//return 0 exection time on success, return 1 on failure
double execWithInput(char* input, int mode){
    pid_t pid = fork();

    if(pid == 0){ //parent
        
        int exitStat;
        waitpid(pid, &exitStat, 0);

        if(WIFEXITED(exitStat) && (WEXITSTATUS(exitStat) == EXIT_SUCCESS)) return 0;
        else return 0; // crash, error code 1 -> isPassed = 0;

    }else if(pid > 0){ //child

        //limit num of fd
        const struct rlimit* rlp;
        if(setrlimit(RLIMIT_NOFILE, rlp) == -1){
            perror("execWithInput: cannot set limit");
            return EXIT_FAILURE;
        }

        // open exe file
        int exec_fd = open("target", O_RDONLY);
        if(exec_fd == -1){
            fprintf(stderr, "execWithInput: cannot open exec file\n");
            return EXIT_FAILURE;
        }

        // open input file and use it as stdin (pipe and dup2)
        int input_fd = open(input, O_RDONLY);
        input_fd = dup2(input_fd, STDIN_FILENO);
        
        //exec...? 
        if(mode == TARGET) execlp("target", "target", (char *) NULL);
        if(mode == SOLUTION) execlp("solution", "solution", (char *) NULL);
        
        exit(EXIT_SUCCESS);
        

    }else{ //error
        printf("cannot fork");
        return 1;
    }


}

int main(int argc, char** argv){
    int maxExecTime = 0, minExecTime = __INT_MAX__, sumExecTime = 0;
    int passedCount = 0, failedCount = 0;

    getArgs(argc, argv);

    //fork() 컴파일 -> 오류나면 알 수 있도록 ??
    if(compileSource(targetSource, TARGET)) fprintf(stderr, "[error] cannot compile %s\n", targetSource);
    if(compileSource(solutionSorce, SOLUTION)) fprintf(stderr, "[error] cannot compile %s\n", solutionSorce);

    DIR* inputDir = NULL;
    struct dirent* one = NULL;
    char inputFilePath[BUFSIZE];

    if((inputDir = opendir(testPath)) == NULL){
        perror("cannot open directory");
        return EXIT_FAILURE;
    }

    //각 input file에 대하여 
    int i = 1;
    while(one = readdir(inputDir)){

        if(one->d_type != DT_REG) continue;
        char oneFile[BUFSIZE];
        sprintf(oneFile, "%s/%s", testPath, one->d_name);
        int isPassed = 1;
        double execTime = 0;
        
        //exec
        if(execTime = (execWithInput(oneFile, TARGET)) == 0){
            isPassed = 0;
        }
        // 시간누적계산, min/max update 
        minExecTime = (minExecTime > execTime) ? execTime:minExecTime;
        maxExecTime = (maxExecTime < execTime) ? execTime:maxExecTime;
        sumExecTime += execTime;

        // // TODO if(isPassed) fork() solution실행 -> 결과 pipe로 받아오기
        if(isPassed){
            if(execWithInput(oneFile, SOLUTION)){
                isPassed = 0;
            }
        }

        // TODO 둘이 비교하기 -> 틀리면 isPassed = 0;
        if( /*compair two || */ isPassed) {
            printf("[Test%d] passed!\n", i++);
            passedCount++;
        }else {
            printf("[Test%d] failed...\n", i++); 
            failedCount++;
        }
        
    }

    //display the summary
    if( 1 /*condition*/ ) printf(" ========== Success ==========\n");
    else printf(" ========== Failed ==========\n");
    printf("[summary of %s]\n", targetSource);
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
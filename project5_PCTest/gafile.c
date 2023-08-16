#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int
main(int argc, char * argv[]){
    int fd[2];
    if(pipe(fd) == -1){
        perror("parent pipe") ;
        return 1 ;
    }


    if(fork() == 0){
        char c_file[100];
        strcpy(c_file,argv[1]);
        printf("argv[1]:%s\n",c_file) ;
        char * executable = strtok(argv[1], ".") ;

        pid_t pid ;
        if( (pid = fork()) == 0){
            if(execlp("gcc", "gcc", "-o", executable, c_file, (char *) NULL) == -1){
                printf("gcc error\n") ;
            }
        }
        waitpid(pid, NULL, 1) ;
        close(fd[0]) ;
        dup2(fd[1],STDOUT_FILENO) ;

        execl(executable, executable,(char *) NULL) ;
        // printf("no") ;
        return 1 ;
    }
    dup2(fd[0],STDIN_FILENO) ;
    close(fd[1]) ;
    char buf[100] ={0};
    // read(fd[0],buf,100) ;
    fgets(buf,100, stdin) ;
    printf("%s",buf) ;

    return 0 ;
}
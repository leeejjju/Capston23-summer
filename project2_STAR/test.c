#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#define BUFSIZE 512

DIR* makeDirStream(char* path);
int enumFiles(DIR* src, char* path);

DIR* src = NULL;

int main(int argc, char** argv){
    char buf[32];

    if(argc < 3){
        printf("[star] invalid usage");
        return 0;
    }

    if(!strcmp(argv[1], "list")){
        printf("[star] start to enum the files...\n");

        if((src = opendir(argv[1])) == NULL){
            printf("directory open error\n"); 
            closedir(src);
        }

        if(enumFiles(src, argv[2])){
            fprintf(stderr, "[error] cannot read the path %s\n", argv[2]);
        }
    }

    return 0;
}


// read the files in the path and print it
int enumFiles(DIR* src, char* path){

    struct dirent* one = NULL;
    char nextPath[128];

    //read all files in this dir
    //if another dir is in there, call enumFiles() again
    while(one = readdir(src)){
        char* dname = one->d_name;
        if(dname[0] == '.') continue; //exceop hidden files/dir

        if(one->d_type == DT_DIR){ //recursively open
            sprintf(nextPath, "%s/%s", path, dname);
            if(enumFiles(src, nextPath)) return 1;
        }else{
            printf("%s/%s\n", path, dname);
        }
    }

    closedir(src);
    if(errno == EBADF) return 1;
    return 0;

}
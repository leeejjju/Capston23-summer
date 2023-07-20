#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#define BUFSIZE 4096 //4KB page size 


int xxd(char* filename);
int enumFiles(char* path);
int copyFiles(char* srcPath, char* destPath);


int main(int argc, char** argv){

    if(argc < 3){

        printf("[star] invalid usage");
        return 0;
    }

    if(!strcmp(argv[1], "archive")){
        
    }else if(!strcmp(argv[1], "list")){

        printf("[star] start to enum the files...\n");
        if(enumFiles(argv[2])){
            fprintf(stderr, "[error] cannot read the path %s\n", argv[2]);
        }

    }else if(!strcmp(argv[1], "extract")){
        
    }else if(!strcmp(argv[1], "xxd")){

        printf("[star] Let's read %s as hexadecimal...\n", argv[2]);
        if(xxd(argv[2])){
            fprintf(stderr, "[error] cannot read the file %s\n", argv[2]);
        }

    }else{

        fprintf(stderr, "[error] invalid command\n");
        exit(EXIT_FAILURE);
    }

}


// read file and display it as hexadecimal numbers 
// return 0 on success, return 1 on failure 
int xxd(char* filename){

    FILE* fd;
    char buf[BUFSIZE];
    size_t readLen = 0;
    int addr = 0;
    int i = 0;
    int pre = 0;
    
    //open file with input filename
    if((fd = fopen(filename, "rb")) == NULL){
        return 1;
    }

    //read the contents
    while(readLen = fread(buf, sizeof(char), sizeof(buf), fd)){

        if(ferror(fd)){
            return EXIT_FAILURE;
        }

        //print the result
        for(i = 0; i < strlen(buf); i++){

            if(!(i%16)){
                printf("\n%08x: ", addr++);
            }

            if(i < readLen){
                printf("%02x", buf[i]);
                if(!((i+1)%2)) printf(" ");
            }

            if((i%16) == 15){
                printf("  ");
                for(int k = pre; k <= i; k++){
                    if(!iscntrl(buf[k])) printf("%c", buf[k]);
                    else printf(".");
                }
                pre = i+1;
            }
            
        }
        printf("  ");
        for(int k = pre; k <= strlen(buf); k++){
            if(!iscntrl(buf[k])) printf("%c", buf[k]);
            else printf(".");
        }
    }

    printf("\n");
    fclose(fd);
    return EXIT_SUCCESS;

}

//read all files in this dir and print it
//if another dir is in there, call enumFiles() recursively
//return 0 on success, return 1 on failure
int enumFiles(char* path){

    DIR* src = NULL;
    struct dirent* one = NULL;
    char nextPath[BUFSIZE];

    if((src = opendir(path)) == NULL){
        closedir(src);
        return EXIT_FAILURE;
    }

    while(one = readdir(src)){
        char* dname = one->d_name;
        if(dname[0] == '.') continue; //except hidden files/dir

        if(one->d_type == DT_DIR){ //if find subdir, recursively open
            sprintf(nextPath, "%s/%s", path, dname);
            if(enumFiles(nextPath)) return EXIT_FAILURE;
        }else{ //else, just print it 
            printf("%s/%s\n", path, dname);
        }
    }

    closedir(src);
    if(errno == EBADF) return EXIT_FAILURE;
    return EXIT_SUCCESS;

}


//TODO 
int copyFiles(char* srcPath, char* destPath){

    DIR* src = NULL;
    DIR* dest = NULL;
    struct dirent* one = NULL;
    char nextPath[128];

    if(((src = opendir(srcPath)) == NULL) || ((dest = opendir(destPath)) == NULL)){
        printf("directory open error\n"); 
        closedir(src);
        closedir(dest);
        return EXIT_FAILURE;
    }

    //read all files in this dir
    //if another dir is in there, call enumFiles() again
    while(one = readdir(src)){
        char* dname = one->d_name;
        if(dname[0] == '.') continue; //exceop hidden files/dir

        if(one->d_type == DT_DIR){ //recursively open
            sprintf(nextPath, "%s/%s", srcPath, dname);
            if(copyFiles(nextPath, destPath)) return 1;
        }else{
            printf("%s/%s\n", srcPath, dname);
        }
    }

    closedir(src);
    if(errno == EBADF) return 1;
    return EXIT_SUCCESS;
}
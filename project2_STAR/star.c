#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#define BUFSIZE 4096 //4KB page size 
#define CHAR_SIZE 1
// #define DUBUG


int xxd(char* filename);
int enumFiles(char* path);
int makeDirectory(char* path, mode_t mode);
int copyFiles(char* srcPath, char* destPath);
int copyOneFile(char* srcPath, char* destPath);



int main(int argc, char** argv){

    if(argc < 3){

        printf("[star] invalid usage");
        return 0;
    }

    if(!strcmp(argv[1], "archive")){

        //TODO 

    }else if(!strcmp(argv[1], "list")){

        printf("[star] start to enum the files...\n");
        if(enumFiles(argv[2])){
            fprintf(stderr, "[error] cannot read the path %s\n", argv[2]);
        }else printf("done!\n");

    }else if(!strcmp(argv[1], "extract")){

        //TODO 
        makeDirectory(argv[2], 0777);

    }else if(!strcmp(argv[1], "xxd")){

        printf("[star] Let's read %s as hexadecimal...\n", argv[2]);
        if(xxd(argv[2])){
            fprintf(stderr, "[error] cannot read the file %s\n", argv[2]);
        }else printf("done!\n");

    }else if(!strcmp(argv[1], "cp")){

        printf("[star] trying to copy %s to %s...\n", argv[2], argv[3]);
        if(copyFiles(argv[2], argv[3])){
            fprintf(stderr, "[error] cannot copy %s to %s\n", argv[2], argv[3]);
        }else printf("done!\n");

    }else{

        fprintf(stderr, "[error] invalid command\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
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
    while(readLen = fread(buf, CHAR_SIZE, sizeof(buf), fd)){

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

    printf("[star] exploring directory %s ...\n", path);

    while(one = readdir(src)){
        char* dname = one->d_name;
        // TODO modify it using stat?
        if(one->d_type == DT_DIR){ //if find subdir, recursively open
            // if(!strcmp(one->d_name, ".") || !strcmp(one->d_name, "..")) continue; //except hidden files/dir
            if((one->d_name)[0] == '.') continue; //except hidden files/dir
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


//make all directorys if not exits : nested mkdir
int makeDirectory(char* path, mode_t mode){

    char* tmp = (char*)malloc(sizeof(char)*strlen(path));
    strcpy(tmp, path);

    for(int i = 0; i < strlen(path); i++){
        if(path[i] == '/'){
            
            tmp[i] = 0; //slicing
            printf("[star] createing directory %s...\n", tmp);
            if(mkdir(tmp, mode) == -1){
                printf("[star] directory %s exist!\n", tmp);
            }
            strcpy(tmp, path);
        } 
    }

    printf("[star] createing directory %s...\n", tmp);
    if(mkdir(tmp, mode) == -1){
        printf("[star] directory %s exist!\n", tmp);
    }

    printf("[star] new directory %s created\n", path);
    return EXIT_SUCCESS;

}


//copy all files and subdirs on srcPath to destPath 
int copyFiles(char* srcPath, char* destPath){

    DIR* src = NULL;
    DIR* dest = NULL;
    struct dirent* one = NULL;
    char nextSrc[BUFSIZE];
    char nextDest[BUFSIZE];
    mode_t mode = 0777;
    
    struct stat srcStat;
    lstat(srcPath, &srcStat);
    mode = srcStat.st_mode;

    //TODO read two's stat (for i-node) and compair,,, 
    // if(0){
    //     printf("[warring] Same path detected\n");
    //     closedir(src);
    //     closedir(dest);
    //     return EXIT_FAILURE;
    // }

    //TODO file 체크 , file이면 바로 거시기 

    if((src = opendir(srcPath)) == NULL){
        if(0){

        }
        fprintf(stderr, "[error] directory open error\n"); 
        perror("[copyFiles]");
        return EXIT_FAILURE;
    }

    //make dest dir if its null
    if((dest = opendir(destPath)) == NULL){
        makeDirectory(destPath, srcStat.st_mode);
        if((dest = opendir(destPath)) == NULL){
            perror("[copyFiles]");
            return EXIT_FAILURE;
        }
    }

    printf("[star] exploring directory %s ...\n", srcPath);


    //read all the file/sudir informations in src path.
    //copy files to dest dir, make subdir on dest dir and recursively explore
    while(one = readdir(src)){
        char* dname = one->d_name;
        sprintf(nextSrc, "%s/%s", srcPath, dname);
        sprintf(nextDest, "%s/%s", destPath, dname);

        // TODO modify it like enum()
        if(one->d_type == DT_DIR){ //dir: recursively explore 
            if(dname[0] == '.') continue; //except . and .. dir 
            if(copyFiles(nextSrc, nextDest)) { 
                perror("[copyFiles]");
                closedir(src);
                closedir(dest);
                return EXIT_FAILURE;
            }
        }else{ //file: copy it to destination path 
            if(copyOneFile(nextSrc, nextDest)){
                perror("[copyFiles]");
                closedir(src);
                closedir(dest);
                return EXIT_FAILURE;
            }
        }

    }

    closedir(src);
    closedir(dest);
    if(errno == EBADF) {
        perror("[copyFiles]");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}

//copy the file to dest path
int copyOneFile(char* srcPath, char* destPath){
    
            FILE* srcFile = NULL;
            FILE* destFile = NULL;
            // ----------------------
            char buf[BUFSIZE];
            size_t readLen = 0;
            size_t writeLen = 0;

            printf("[star] trying to copy %s ...\n", srcPath);


            //TODO 디렉토리 거르기? 
            //TODO 경로/이름 같을 경우 이름 바꿔주기..?? 
            //open file 
            if((srcFile = fopen(srcPath, "rb")) == NULL){
                fprintf(stderr, "[error] cannot open dir %s\n", srcPath);
                perror("[copyOneFile]");
                return EXIT_FAILURE;
            }

            if((destFile = fopen(destPath, "wb+")) == NULL){
                fprintf(stderr, "[error] cannot open dir %s\n", destPath);
                perror("[copyOneFile]");
                return EXIT_FAILURE;
            }

            //read the contents
            while(!feof(srcFile)){
                readLen = fread(buf, CHAR_SIZE, sizeof(buf), srcFile);
                #ifdef DUBUG

                printf("[%s] %s\n", srcPath, buf);
                #endif
                writeLen = fwrite(buf, CHAR_SIZE, readLen, destFile);
                if(ferror(srcFile) || ferror(destFile)){
                    fprintf(stderr, "[error] error on reading/writing\n");
                    return EXIT_FAILURE;
                }
                if(readLen != writeLen){
                    fprintf(stderr, "[error] error on reading/writing\n");
                    return EXIT_FAILURE;
                }
            }

            fclose(srcFile);
            fclose(destFile);

            printf("[star] copied %s to %s \n", srcPath, destPath);
            return EXIT_SUCCESS;


}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#define BUFSIZE 4096 //4KB page size 
#define CHAR_SIZE 1
#define DUBUG


int xxd(char* filename);
int enumFiles(char* path);
int makeDirectory(char* path, mode_t mode);
int copyFiles(char* srcPath, char* destPath);
int copyOneFile(char* srcPath, char* destPath);
int archive(char* srcPath, char* destPath);
int extract(char* filePath);


char absDest[BUFSIZE];

int main(int argc, char** argv){

    if(argc < 3){

        printf("[star] invalid usage");
        return 0;
    }

    if(!strcmp(argv[1], "archive")){

        //TODO 

    }else if(!strcmp(argv[1], "list")){

        #ifdef DEBUG
        printf("[star] start to enum the files...\n");
        #endif
        if(enumFiles(argv[2])){
            fprintf(stderr, "[error] cannot read the path %s\n", argv[2]);
        }else printf("done!\n");

    }else if(!strcmp(argv[1], "extract")){

        //TODO 

    }else if(!strcmp(argv[1], "xxd")){
        
        printf("[star] Let's read %s as hexadecimal...\n", argv[2]);
        if(xxd(argv[2])){
            fprintf(stderr, "[error] cannot read the file %s\n", argv[2]);
        }else printf("done!\n");

    }else if(!strcmp(argv[1], "cp")){

        realpath(argv[3], absDest);
        printf("[star] trying to copy %s to %s...\n", argv[2], argv[3]);
        if(copyFiles(argv[2], argv[3])){
            fprintf(stderr, "[error] cannot copy %s to %s\n", argv[2], argv[3]);
        }else printf("done!\n");

        system("tree");

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
    #ifdef DEBUG
    printf("[star] exploring directory %s ...\n", path);
    #endif

    while(one = readdir(src)){
        char* dname = one->d_name;
        // TODO modify to consider links
        if(one->d_type == DT_DIR){ //if find subdir, recursively open
            // if(!strcmp(one->d_name, ".") || !strcmp(one->d_name, "..")) continue; //except hidden files/dir
            if((one->d_name)[0] == '.') continue; //except hidden files/dir
            sprintf(nextPath, "%s/%s", path, dname);
            if(enumFiles(nextPath)) return EXIT_FAILURE;
        }else if(one->d_type == DT_LNK){
            continue;
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
            #ifdef DEBUG
            printf("[star] createing directory %s...\n", tmp);
            #endif
            if(mkdir(tmp, mode) == -1){
                #ifdef DEBUG
                printf("[star] directory %s exist!\n", tmp);
                #endif
            }
            strcpy(tmp, path);
        } 
    }

    #ifdef DEBUG
    printf("[star] createing directory %s...\n", tmp);
    #endif
    if(mkdir(tmp, mode) == -1){
        #ifdef DEBUG
        printf("[star] directory %s exist!\n", tmp);
        #endif
    }
    #ifdef DEBUG
    printf("[star] new directory %s created\n", path);
    #endif
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
    char absSrc[BUFSIZE]; // src: https://www.it-note.kr/53
    struct stat srcStat;
    lstat(srcPath, &srcStat);
    mode = srcStat.st_mode;

    // if((srcStat.st_mode) == S_IFLNK){
    //     printf("[star] this file is link.\n");
    //     return EXIT_SUCCESS;
    // }

    //if same path detected, skip it 
    realpath(srcPath, absSrc);
    #ifdef DEBUG
    printf("src: %s\ndest:%s\n", absSrc, absDest);
    #endif
    if(!strcmp(absSrc, absDest)){
        printf("[warring] Same path detected\n");
        return EXIT_SUCCESS;
    }

    //file 체크
    if(S_ISREG(srcStat.st_mode)){
        printf("[star] this is file:)\n");
        char* newFile = (char*)malloc(sizeof(char)*strlen(srcPath));
        sprintf(newFile, "%s/%s", destPath, srcPath);
        if(copyOneFile(srcPath, newFile)) return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }


    if((src = opendir(srcPath)) == NULL){
        fprintf(stderr, "[error] directory open error: "); 
        perror("");
        return EXIT_FAILURE;
    }

    //make dest dir if its null
    if((dest = opendir(destPath)) == NULL){
        makeDirectory(destPath, srcStat.st_mode);
        if((dest = opendir(destPath)) == NULL){
            fprintf(stderr, "[error] directory open error: "); 
            perror("[copyFiles]");
            return EXIT_FAILURE;
        }
    }
    #ifdef DEBUG
    printf("[star] exploring directory %s ...\n", srcPath);
    #endif


    //read all the file/sudir informations in src path.
    //copy files to dest dir, make subdir on dest dir and recursively explore
    while(one = readdir(src)){
        char* dname = one->d_name;
        sprintf(nextSrc, "%s/%s", srcPath, dname);
        sprintf(nextDest, "%s/%s", destPath, dname);

        if(one->d_type == DT_DIR){ //dir: recursively explore 
            if(dname[0] == '.') continue; //except . and .. dir 

            if(copyFiles(nextSrc, nextDest)) { 
                perror("[copyFiles:dir]");
                closedir(src);
                closedir(dest);
                return EXIT_FAILURE;
            }

        //TODO how can i handle this...? 
        }else if(one->d_type == DT_LNK){
            // char buf[BUFSIZE];
            // sprintf(buf, "cp %s %s", nextSrc, destPath);
            // system(buf);
            // printf("[star] created link %s\n", one->d_name);
            continue;

        }else{ //file: copy it to destination path 
            if(copyOneFile(nextSrc, nextDest)){
                perror("[copyFiles:file]");
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
            char buf[BUFSIZE];
            size_t readLen = 0;
            size_t writeLen = 0;
            struct stat fs;
            #ifdef DEBUG
            printf("[star] trying to copy %s ...\n", srcPath);
            #endif

            //open file 
            if((srcFile = fopen(srcPath, "rb")) == NULL){
                fprintf(stderr, "[error] cannot open dir %s: ", srcPath);
                perror("");
                return EXIT_FAILURE;
            }

            if((destFile = fopen(destPath, "wb+")) == NULL){
                fprintf(stderr, "[error] cannot open dir %s: ", destPath);
                perror("");
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

            lstat(srcPath, &fs);
            if(chmod(destPath, fs.st_mode)){
                fprintf(stderr, "[error] cannot change the mode of %s\n", destPath);
            }

            fclose(srcFile);
            fclose(destFile);

            #ifdef DEBUG
            printf("[star] copied %s to %s \n", srcPath, destPath);
            #endif
            return EXIT_SUCCESS;


}


// -------------------------------------------------------------------------------



//read all files and subdirs on srcPath and make archive file  
int archive(char* srcPath, char* destPath){

    DIR* src = NULL;
    DIR* dest = NULL;
    struct dirent* one = NULL;
    char nextSrc[BUFSIZE];
    char nextDest[BUFSIZE];
    struct stat srcStat;
    lstat(srcPath, &srcStat);

    //! file 체크 - 필요한가? 
    if(S_ISREG(srcStat.st_mode)){
        printf("[star] this is file, not a directory\n");
        return EXIT_FAILURE;
    }

    if((src = opendir(srcPath)) == NULL){
        fprintf(stderr, "[error] directory open error: "); 
        perror("");
        return EXIT_FAILURE;
    }

    //make dest dir if its null
    if((dest = opendir(destPath)) == NULL){
        makeDirectory(destPath, srcStat.st_mode);
        if((dest = opendir(destPath)) == NULL){
            fprintf(stderr, "[error] directory open error: "); 
            perror("[copyFiles]");
            return EXIT_FAILURE;
        }
    }
    #ifdef DEBUG
    printf("[star] exploring directory %s ...\n", srcPath);
    #endif


    //read all the file/sudir informations in src path.
    //copy files to dest dir, make subdir on dest dir and recursively explore
    while(one = readdir(src)){
        char* dname = one->d_name;
        sprintf(nextSrc, "%s/%s", srcPath, dname);
        sprintf(nextDest, "%s/%s", destPath, dname);

        if(one->d_type == DT_DIR){ //dir: recursively explore 
            if(dname[0] == '.') continue; //except . and .. dir 

            if(copyFiles(nextSrc, nextDest)) { 
                perror("[copyFiles:dir]");
                closedir(src);
                closedir(dest);
                return EXIT_FAILURE;
            }

        //TODO how can i handle this...? 
        }else if(one->d_type == DT_LNK){
            // char buf[BUFSIZE];
            // sprintf(buf, "cp %s %s", nextSrc, destPath);
            // system(buf);
            // printf("[star] created link %s\n", one->d_name);
            continue;

        }else{ //file: copy it to destination path 
            if(copyOneFile(nextSrc, nextDest)){
                perror("[copyFiles:file]");
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


int extract(char* filePath){
    return 0;
}
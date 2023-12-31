#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#define BUFSIZE 4096 //4KB page size 
// #define DEBUG

char absDest[BUFSIZE];
FILE* acvFile = NULL;
const int CHAR_SIZE = 1;

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


//list all file headers on acv-file 
int list(){

    unsigned int pathLen = 0;
    unsigned int size = 0;
    char path[BUFSIZE];
    int readLen = 0;


    while(!feof(acvFile)){
        //4 byte path length n
        readLen = fread((void*)&pathLen, CHAR_SIZE, 4, acvFile);
        if(!readLen) break;

        //n byte path
        fread(path, CHAR_SIZE, pathLen, acvFile);
        path[pathLen] = 0;

        //4 byte contents size m
        fread((void*)&size, CHAR_SIZE, 4, acvFile);
        #ifdef DEBUG
        printf("%3u | %-40s | %6u\n", pathLen, path, size);
        #endif

        //m byte contents
        fseek(acvFile, size, SEEK_CUR);

        if(ferror(acvFile)){
            perror("[list]");
            return EXIT_FAILURE;
        }

    }

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
        fclose(srcFile);
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
            fclose(srcFile);
            fclose(destFile);
            return EXIT_FAILURE;
        }
        if(readLen != writeLen){
            fprintf(stderr, "[error] error on reading/writing\n");
            fclose(srcFile);
            fclose(destFile);
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

        }else if(one->d_type == DT_LNK){
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



//read one file and archive it to archive-file with the format 
int readOneFile(char* fileName){
    
    FILE* srcFile = NULL;
    char buf[BUFSIZE];
    size_t readLen = 0;
    size_t writeLen = 0;
    int size = 0;
    struct stat fs;
    lstat(fileName, &fs);

    #ifdef DEBUG
    printf("[star] trying to archive %s ...\n", fileName);
    #endif

    //open files
    if((srcFile = fopen(fileName, "rb")) == NULL){
        fprintf(stderr, "[error] cannot open dir %s: ", fileName);
        perror("");
        return EXIT_FAILURE;
    }

    //path len, path, size
    unsigned int pathLen= strlen(fileName);
    unsigned int contentSize = fs.st_size;
    fwrite((void*)&pathLen, CHAR_SIZE, 4, acvFile);
    fwrite(fileName, CHAR_SIZE, pathLen, acvFile);
    fwrite((void*)&contentSize, CHAR_SIZE, 4, acvFile);

    //contents
    //read the contents
    while(!feof(srcFile)){
        readLen = fread(buf, CHAR_SIZE, sizeof(buf), srcFile);
        #ifdef DEBUG
        printf("[%s] %s\n", fileName, buf);
        #endif
        writeLen = fwrite(buf, CHAR_SIZE, readLen, acvFile);
        if(ferror(srcFile) || ferror(acvFile)){
            fprintf(stderr, "[error] error on reading/writing\n");
            fclose(srcFile);
            return EXIT_FAILURE;
        }
        if(readLen != writeLen){
            fprintf(stderr, "[error] error on reading/writing\n");
            fclose(srcFile);
            return EXIT_FAILURE;
        }
        size += writeLen;
    }

    #ifdef DEBUG
    printf("[star] succesfully archived %s\n", fileName);
    #endif

    fclose(srcFile);
    return EXIT_SUCCESS;

}



//read and archive the all files in that path to archive file by calling writeOneFile()
int archive(char* srcPath){

    DIR* src = NULL;
    char nextSrc[BUFSIZE];
    struct dirent* one = NULL;
    struct stat srcStat;
    lstat(srcPath, &srcStat);
    
    //open the target directory
    if((src = opendir(srcPath)) == NULL){
        perror("[archive]");
        return EXIT_FAILURE;
    }

    #ifdef DEBUG
    printf("[star] exploring directory %s ...\n", srcPath);
    #endif

    //read and save all the file informations in src path by calling 
    //if found subdir, explore it recursively
    while(one = readdir(src)){
        char* dname = one->d_name;
        sprintf(nextSrc, "%s/%s", srcPath, dname);

        //directory
        if(one->d_type == DT_DIR){ 
            if(dname[0] == '.') continue; //except . and .. dir 
            if(archive(nextSrc)) { 
                perror("[archive:subdir]");
                closedir(src);
                return EXIT_FAILURE;
            }

        //link
        }else if(one->d_type == DT_LNK){
            continue;

        //file
        }else{ 
            if(readOneFile(nextSrc)){
                perror("[archive:file]");
                closedir(src);
                return EXIT_FAILURE;
            }
        }
    }

    closedir(src);
    if(errno == EBADF) {
        perror("[archive]");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}



//make one file from archive-file with the format, named as given filename 
int makeOneFile(char* fileName, int size){
    int readLen = 0, writeLen = 0;
    FILE* newFile = NULL;
    char buf[BUFSIZE];

    //make path as cut all from last '/' 
    char* path = strdup(fileName);
    int endIndex = 0;
    for(int i = 0; i < strlen(path); i++){
        if(path[i] == '/'){
            endIndex = i;
        }
    }
    path[endIndex] = 0;
    

    makeDirectory(path, 0766);
    //open new file with given path
    if((newFile = fopen(fileName, "wb+")) == NULL){
        perror("[makeOneFile]");
        return EXIT_FAILURE;
    }

    #ifdef DEBUG
    printf("[star] extracting %s ...\n", fileName);
    #endif

    //write contents from acv-file as size
    while((size > BUFSIZE) && !feof(acvFile)){

        readLen = fread(buf, CHAR_SIZE, BUFSIZE, acvFile);
        writeLen = fwrite(buf, CHAR_SIZE, readLen, newFile);
        #ifdef DEBUG
        printf("%s\n", buf);
        #endif 

        if(readLen != writeLen){
            perror("[makeOneFile]");
            fclose(newFile);
            return EXIT_FAILURE;
        }
        size -= readLen;
        #ifdef DEBUG
        printf("read %d byte, remain size is %d byte\n", readLen, size);
        #endif 

    }

    if(size > 0) {
        readLen = fread(buf, CHAR_SIZE, size, acvFile);
        writeLen = fwrite(buf, CHAR_SIZE, readLen, newFile);
        #ifdef DEBUG
        printf("%s\n", buf);
        #endif 

        if(readLen != writeLen){
            perror("[makeOneFile]");
            fclose(newFile);
            return EXIT_FAILURE;
        }
        size -= readLen;
        #ifdef DEBUG
        printf("read %d byte, remain size is %d byte\n", readLen, size);
        #endif 
    }

    #ifdef DEBUG
    printf("[star] succesfully extracted %s!\n", fileName);
    #endif

    fclose(newFile);
    return EXIT_SUCCESS;
}


//extract the archive file with filename param
int extract(char* fileName){

    unsigned int pathLen = 0;
    unsigned int size = 0;
    char path[BUFSIZE];
    int readLen = 0;

    while(!feof(acvFile)){
        //4 byte path length n
        readLen = fread((void*)&pathLen, CHAR_SIZE, 4, acvFile);
        if(!readLen) break;

        //n byte path
        fread(path, CHAR_SIZE, pathLen, acvFile);
        path[pathLen] = 0;

        //4 byte contents size m
        fread((void*)&size, CHAR_SIZE, 4, acvFile);
        #ifdef DEBUG
        printf("%u | %s | %u\n", pathLen, path, size);
        #endif

        //m byte contents
        if(makeOneFile(path, (int)size)){
            perror("[extract]");
            return EXIT_FAILURE;
        }

        if(ferror(acvFile)){
            perror("[list]");
            return EXIT_FAILURE;
        }

    }

    return EXIT_SUCCESS;
}



int main(int argc, char** argv){

    if(argc < 3){
        printf("\n[star] invalid command\n");
        printf("usage: star archive [archive-file-name] [target directory path]\n       star list [archive-file-name]\n       star extract [archive-file-name]\n       star xxd [file-name]\n       star cp [source directory path] [destination directory path]\n");
        return EXIT_FAILURE;
    }
    
    //archive 
    if(!strcmp(argv[1], "archive") && argc > 3){
        if((acvFile = fopen(argv[2], "wb+")) == NULL){
            perror("[acvFile]");
            return EXIT_FAILURE;
        }

        printf("[star] start to archive %s to %s ...\n", argv[3], argv[2]);
        if(archive(argv[3])){
            fprintf(stderr, "[error] cannot archive %s to %s\n", argv[3], argv[2]);
        }else printf("done!\n");

        fclose(acvFile);
        system("tree");


    //list
    //TODO fix it to use archive file 
    }else if(!strcmp(argv[1], "list") && argc > 2){

        //open the archived file 
        if((acvFile = fopen(argv[2], "rb")) == NULL){
            fprintf(stderr, "[error] cannot open dir %s: ", argv[2]);
            perror("");
            return EXIT_FAILURE;
        }

        printf("[star] start to list %s ...\n", argv[2]);

        if(list()){
            fprintf(stderr, "[error] cannot read the path %s\n", argv[2]);
        }else printf("done!\n");

        fclose(acvFile);

    //extract 
    }else if(!strcmp(argv[1], "extract") && argc > 2){

        //open the archived file 
        if((acvFile = fopen(argv[2], "rb")) == NULL){
            fprintf(stderr, "[error] cannot open dir %s: ", argv[2]);
            perror("");
            return EXIT_FAILURE;
        }
        
        //extract
        if(extract(argv[2])){
            fprintf(stderr, "[error] cannot extract %s\n", argv[2]);
        }else printf("done!\n");

        fclose(acvFile);
        system("tree");


    //xxd 
    }else if(!strcmp(argv[1], "xxd") && argc > 2){
        
        printf("[star] Let's read %s as hexadecimal...\n", argv[2]);
        if(xxd(argv[2])){
            fprintf(stderr, "[error] cannot read the file %s\n", argv[2]);
        }else printf("done!\n");


    //copy
    }else if(!strcmp(argv[1], "cp") && argc > 3){

        realpath(argv[3], absDest);
        printf("[star] trying to copy %s to %s...\n", argv[2], argv[3]);
        if(copyFiles(argv[2], argv[3])){
            fprintf(stderr, "[error] cannot copy %s to %s\n", argv[2], argv[3]);
        }else printf("done!\n");

        system("tree");


    //etc
    }else{
        printf("\n[star] invalid command\n");
        printf("usage: star archive [archive-file-name] [target directory path]\n       star list [archive-file-name]\n       star extract [archive-file-name]\n       star xxd [file-name]\n       star cp [source directory path] [destination directory path]\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


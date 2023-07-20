#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define BUFSIZE 512

int xxd(char filename[32]);
int enumFiles(char filename[32]);
int copyFiles(char soruce[32], char dest[32]);


int main(int argc, char** argv){
    char buf[32];

    if(argc == 1){
        printf("type the filename: ");
        scanf("%s", buf);
    }else{
        strcpy(buf, argv[1]);
    }

    printf("Let's read %s as hexadecimal...\n", buf);
    if(xxd(buf)){
        fprintf(stderr, "[error] cannot read the file %s\n", buf);
    }

    return 0; 



    if(!strcmp(argv[1], "archive")){
        
    }else if(!strcmp(argv[1], "list")){

    }else if(!strcmp(argv[1], "extract")){
        
    }else{
        fprintf(stderr, "[error] invalid command\n");
        exit(EXIT_FAILURE);
    }

}


// read file and display it as hexadecimal numbers 
// return 0 on success, return 1 on failure 
int xxd(char filename[32]){
    FILE* fd;
    char buf[BUFSIZE];
    size_t readLen = 0;
    int addr = 0;
    int i = 0;
    int pre = 0;
    
    if((fd = fopen(filename, "rb")) == NULL){
        return 1;
    }

    while(readLen = fread(buf, sizeof(char), sizeof(buf), fd)){

        if(ferror(fd)){
            return 1;
        }

        for(i = 0; i < strlen(buf); i++){
            if(!(i%16)){
                printf("\n%08x: ", addr++);
            }
            if(i < readLen){
                printf("%02x", buf[i]);
                if(!((i+1)%2)) printf(" ");
            }else{
                printf("  ", buf[i]);
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
        
        for(int k = (i%16); k < 16; k++) {
            printf("  ");
            if(!((i+1)%2)) printf(" ");
        }
        for(int k = pre; k <= i; k++){
            if(!iscntrl(buf[k])) printf("%c", buf[k]);
            else printf(".");
        }
    }

    printf("\n");
    fclose(fd);
    return 0;

}
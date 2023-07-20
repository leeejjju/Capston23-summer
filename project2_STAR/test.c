#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFSIZE 16

// read file and display it as hexadecimal numbers 
// return 0 on success, return 1 on failure 
int xxd(char filename[32]);
int enumFiles(char filename[32]);

int main(int argc, char** argv){
    char buf[32];

    if(argc == 1){
        printf("type the filename: ");
        scanf("%s", buf);
    }else{
        // for(int i = 0; i < argc; i++) printf("%d: %s\n", i, argv[i]);
        strcpy(buf, argv[1]);
    }
    

    printf("Let's read %s as hexadecimal...\n", buf);
    if(xxd(buf)){
        fprintf(stderr, "[error] cannot read the file %s\n", buf);
    }

    return 0;
}


int xxd(char filename[32]){
    FILE* fd;
    char buf[BUFSIZE];
    size_t maxLen = BUFSIZE;
    size_t readLen = 0;
    
    if((fd = fopen(filename, "rb")) == NULL){
        return 1;
    }

    while(readLen = fread(buf, sizeof(char), sizeof(buf), fd)){

        for(int i = 0; i < readLen-1; i++){
            if(!((i)%BUFSIZE)) printf("\n");
            printf("%02x", buf[i]);
            if(!((i+1)%2)) printf(" ");
            
        }
    }

    printf("\n");
    fclose(fd);
    return 0;

}
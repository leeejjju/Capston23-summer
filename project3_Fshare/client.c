#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/stat.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#define BUFSIZE 4096


int main(int argc, char** argv){

    if(argc < 3){
        goto EXIT;
    }
    char* ip = (char*)malloc(sizeof(char)*strlen(argv[1])+1);
    char cmd[4];
    char* srcPath = (char*)malloc(sizeof(char)*strlen(argv[3])+1);
    strcpy(ip, argv[1]);
    strcpy(cmd, argv[2]);
    strcpy(srcPath, argv[3]);

    //get ip and port from argv[2]
    int index = 0;
    int port = 0;
    for(int i = 0; i < strlen(ip); i++){
        if(ip[i] == ':') index = i;
    }
    ip[index] = 0;
    port = atoi(&ip[index+1]);
    printf("ip: %s\nport: %d\npath:%s\n", ip, port, srcPath);


    //make socket and connect to server using ip and port 
    struct sockaddr_in serv_addr; 
	int sock_fd ;
	int s, len = 0;
	char buffer[BUFSIZE] = {0}; 
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
	if (sock_fd <= 0) {
		perror("socket failed : ") ;
		exit(EXIT_FAILURE) ;
	} 

	memset(&serv_addr, '\0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		exit(EXIT_FAILURE) ;
	} 

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(EXIT_FAILURE) ;
	}


	printf("size of header: %d + %d\n", (int)sizeof(cmd), (int)sizeof(len));

    //send header: cmd
    if(!send(sock_fd, cmd, 4, 0)){
        perror("[cannot send header]");
        free(ip);
        free(srcPath);
        return EXIT_FAILURE;
    }
    

    //list
    if(!strcmp(cmd, "list")){
        printf("[list]\n");
        //send header: payload size: 0
        len = 0;
        printf("size of payload: %d\n", len);
        if(!send(sock_fd, &len, 4, 0)){
            perror("[cannot send header]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }
        shutdown(sock_fd, SHUT_WR) ;
        

    //get 
    }else if(!strcmp(cmd, "get")){
        printf("[get]\n");

        //send header: payload size: strlen(filename)
        len = strlen(srcPath);
        printf("size of payload: %d\n", len);
        if(!send(sock_fd, &len, 4, 0)){
            perror("[cannot send header]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }

        //send payload: filename
        if(!send(sock_fd, srcPath, len, 0)){
            perror("[cannot send payload]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }

        shutdown(sock_fd, SHUT_WR) ;




    //put 
    }else if(!strcmp(cmd, "put")){
        printf("[put]\n");
        
        //open the file
        FILE* srcFile = NULL;
        if((srcFile = fopen(srcPath, "r")) == NULL){
            perror("[cannot make file]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }

        struct stat srcStat;
        lstat(srcPath, &srcStat);
        len = (strlen(srcPath) + sizeof(int)*2 + srcStat.st_size);
        printf("size of payload: %d\n", len);

        //send header: payload size: sizeof(archive(filename))
        if(!send(sock_fd, &len, 4, 0)){
            perror("[cannot send header]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }

        //TODO send payload: archive format 
        //---------------------------------------------

        while((s = fread(buffer, 1, BUFSIZE-1, srcFile)) > 0){

            if(ferror(srcFile)){
                perror("[cannot read contents]");
                free(ip);
                free(srcPath);
                return EXIT_FAILURE;
            }

            buffer[s] = 0x0;

            if(!send(sock_fd, buffer, s, 0)){
                perror("[cannot send contents]");
                free(ip);
                free(srcPath);
                return EXIT_FAILURE;
            }
            if(feof(srcFile)) break;
        }

        fclose(srcFile);
        //---------------------------------------------
            

        shutdown(sock_fd, SHUT_WR) ;


    //show 
    }else if(!strcmp(cmd, "show")){
        printf("[show]\n");
        //send header: payload size: strlen(filename)
        len = strlen(srcPath);
        printf("size of payload: %d\n", len);
        if(!send(sock_fd, &len, 4, 0)){
            perror("[cannot send header]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }

        //send payload: filename
        if(!send(sock_fd, srcPath, len, 0)){
            perror("[cannot send payload]");
            free(ip);
            free(srcPath);
            return EXIT_FAILURE;
        }
        shutdown(sock_fd, SHUT_WR) ;


        printf("\n");

        len = 0;
        while ((s = recv(sock_fd, buffer, BUFSIZE-1, 0))) {
            printf("%s", buffer);
            len += s;
        }

    // invalid 
    }else{
        

        
    }

    


	
	printf("done!\n");
    EXIT:
    free(ip);
    free(srcPath);
    return EXIT_SUCCESS;

}

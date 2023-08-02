#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/stat.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <libgen.h>
#define BUFSIZE 4096
#define DEBUG
#define CHEADER_SIZE 16
#define SHEADER_SIZE 8

typedef enum{
    list,
    get,
    put,
    show,
    N_OP
}cmd;

struct client_header {
	cmd command;           // enum {list:0, get:1, put:2}
	int src_path_len;      // src path when 'put'
	int des_path_len;
	int payload_size;      // size of payload will send
};
struct server_header{
	int is_error;          // flag of error returned, 0 on success, 1 on failure
	int payload_size;      // file contents size
};

FILE* srcFile = NULL;
struct client_header cheader;
struct server_header sheader;
int flag = 0;

/* 
   send_bytes
        return 0 if all given bytes are successfully sent
        return 1 otherwise
*/

int 
send_bytes(int fd, char * buf, size_t len)
{
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len)
    {
        size_t sent ;
        sent = send(fd, p, len - acc, 0) ;
        if (sent == -1)
                return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

//make all directorys if not exits : nested mkdir
int makeDirectory(char* path){

    char* tmp = (char*)malloc(sizeof(char)*strlen(path));
    strcpy(tmp, path);

    for(int i = 0; i < strlen(path); i++){
        if(path[i] == '/'){
            
            tmp[i] = 0; //slicing
            #ifdef DEBUG
            printf("[star] createing directory %s...\n", tmp);
            #endif
            if(mkdir(tmp, 0777) == -1){
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
    if(mkdir(tmp, 0777) == -1){
        #ifdef DEBUG
        printf("[star] directory %s exist!\n", tmp);
        #endif
    }
    #ifdef DEBUG
    printf("[star] new directory %s created\n", path);
    #endif
    return EXIT_SUCCESS;

}

int run_list(int sock_fd){

    int s = 0;
    char buffer[BUFSIZE];

    printf("[list]\n");
    //send header: payload size: 0
    cheader.command = list;
    cheader.des_path_len = 0;
    cheader.src_path_len = 0;
    cheader.payload_size = 0;
    if(!(s = send(sock_fd, &cheader, CHEADER_SIZE, 0)) || (s != CHEADER_SIZE)){
        perror("[cannot send header]");
        return 1;
    }

    shutdown(sock_fd, SHUT_WR) ;

    // recv and printf payload(path) from server
    while (s = recv(sock_fd, &sheader, SHEADER_SIZE, 0)) {
        if(sheader.is_error || (s != SHEADER_SIZE)){
            perror("[error on server]");
            return 1;
        }
        s = recv(sock_fd, buffer, sheader.payload_size, 0);
        if(s != sheader.payload_size){
            perror("[error on recv]");
            return 1;
        }
        buffer[s] = 0;
        if(!s) break;

        #ifdef DEBUG
        printf("%s\n", buffer);
        #endif
    }

    return 0;

}

int run_get(int sock_fd, char* srcPath, char* destPath){

    char buffer[BUFSIZE];
    int s = 0;

    printf("[get]\n");

    //send header
    cheader.command = get;
    cheader.src_path_len = strlen(srcPath);
    cheader.des_path_len = 0;
    cheader.payload_size = 0;
    if(!(s = send(sock_fd, &cheader, CHEADER_SIZE, 0))  || (s != CHEADER_SIZE)){
        perror("[cannot send header]");
        return 1;
    }
    //send payload:srcpath(srcPath)
    if(!(s = send(sock_fd, srcPath, cheader.src_path_len, 0)) || (s != cheader.src_path_len)){
        perror("[cannot send payload]");
        return 1;
    }
    shutdown(sock_fd, SHUT_WR) ;
    
    makeDirectory(destPath);
    strcat(destPath, "/");
    strcat(destPath, basename(srcPath));
    #ifdef DEBUG
    printf("dest: %s\n", destPath);
    #endif

    //recv the header
    if(!(s = recv(sock_fd, &sheader, SHEADER_SIZE, 0)) || (s != SHEADER_SIZE)){
        perror("[cannot recv header]");
        return 1;
    }
    if(flag = sheader.is_error){
        printf("cannot get file from server\n");
        return 1;
    }
    
    //open the file
    if((srcFile = fopen(destPath, "wb")) == NULL){
        perror("[cannot make file]");
        return 1;
    }

    //recv the payload
    while ((s = recv(sock_fd, buffer, BUFSIZE-1, 0))) {
        buffer[s]=0;
        if(!fwrite(buffer, 1, s, srcFile)) break;
        #ifdef DEBUG
        printf("%s", buffer);
        #endif
    }

    fclose(srcFile);

    return 0;
}

int run_put(int sock_fd, char* srcPath, char* destPath){
    printf("[put]\n");
    int s = 0;
    char* buffer[BUFSIZE];
        
    //open the file
    if((srcFile = fopen(srcPath, "rb")) == NULL){
        perror("[cannot make file]");
        return 1;
    }

    struct stat srcStat;
    lstat(srcPath, &srcStat);

    //send header
    cheader.command = put;
    cheader.src_path_len = strlen(srcPath);
    cheader.des_path_len = strlen(destPath);
    cheader.payload_size = srcStat.st_size;
    if(!(s = send(sock_fd, &cheader, CHEADER_SIZE, 0)) || (s != CHEADER_SIZE)){
        perror("[cannot send header]");
        return 1;
    }
    //send payload:srcpath
    if(!(s = send(sock_fd, srcPath, cheader.src_path_len, 0)) || (s != cheader.src_path_len)){
        perror("[cannot send payload]");
        return 1;
    }
    //send payload:destpath
    if(!(s = send(sock_fd, destPath, cheader.des_path_len, 0)) || (s != cheader.des_path_len)){
        perror("[cannot send payload]");
        return 1;
    }
    //send payload: file contents
    while(s = fread(buffer, 1, BUFSIZE-1, srcFile)){

        if(ferror(srcFile)){
            perror("[cannot read contents]");
            return 1;
        }

        buffer[s] = 0;

        if(!(s = send(sock_fd, buffer, s, 0))){
            perror("[cannot send contents]");
            return 1;
        }

        if(feof(srcFile)) break;
    }

    fclose(srcFile);
    shutdown(sock_fd, SHUT_WR);

    if(!(s = recv(sock_fd, &sheader, SHEADER_SIZE, 0)) || (s != SHEADER_SIZE)){
        perror("[cannot recv header]");
        return 1;
    }

    if(flag = sheader.is_error){
        printf("cannot get file from server\n");
        return 1;
    }

    return 0;
}

int run_show(int sock_fd, char* srcPath){
    int s = 0;
    char* buffer[BUFSIZE];

    printf("[show]\n");
    //send header: payload size: strlen(filename)
    cheader.command = show;
    cheader.src_path_len = strlen(srcPath);
    cheader.des_path_len = 0;
    cheader.payload_size = 0;

    if(!(s = send(sock_fd, &cheader, CHEADER_SIZE, 0)) || (s != CHEADER_SIZE)){
        perror("[cannot send header]");
        return 1;
    }
    //send payload: filename
    if(!(s = send(sock_fd, srcPath, cheader.src_path_len, 0)) || (s != cheader.src_path_len)){
        perror("[cannot send payload]");
        return 1;
    }
    shutdown(sock_fd, SHUT_WR) ;

    while ((s = recv(sock_fd, buffer, BUFSIZE-1, 0))) {
        buffer[s] = 0;
        printf("%s", buffer);
    }
    return 0;

}


int main(int argc, char** argv){

    char* srcPath;
    char* destPath;
    char* ip;

    if(argc < 3){
        goto EXIT;
    }
    if(argc >= 4){
        srcPath = strdup(argv[3]);
        srcPath[strlen(argv[3])] = 0;
    } 
    if(argc == 5){
        destPath = strdup(argv[4]);
        destPath[strlen(argv[4])] = 0;
    }

    ip = (char*)malloc(sizeof(char)*strlen(argv[1])+1);
    strcpy(ip, argv[1]);

    cmd command;
    if(!strcmp(argv[2], "list")) command = list;
    else if(!strcmp(argv[2], "get")) command = get;
    else if(!strcmp(argv[2], "put")) command = put;
    else if(!strcmp(argv[2], "show")) command = show;
    else command = N_OP;
    
    //get ip and port from argv[2]
    int index = 0;
    int port = 0;
    for(int i = 0; i < strlen(ip); i++){
        if(ip[i] == ':') index = i;
    }
    ip[index] = 0;
    port = atoi(&ip[index+1]);
    #ifdef DEBUG
    printf("ip: %s\nport: %d\n", ip, port);
    if(argc >= 4) printf("src path: %s\n", srcPath);
    if(argc == 5) printf("dest path: %s\n", destPath);
    #endif


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
    free(ip);

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(EXIT_FAILURE) ;
	}

    

    //send payload size and payload, get apropriate response
    switch (command) {
    case list:
        if(argc < 3){ 
            flag = -1;
            goto EXIT;
        }
        flag = run_list(sock_fd);
        break;

    case get: 
        if(argc < 5){
            flag = -1;
            goto EXIT;
        }
        flag = run_get(sock_fd, srcPath, destPath);
        break;

    case put: 
        if(argc < 5){
            flag = -1;
            goto EXIT;
        }
        flag = run_put(sock_fd, srcPath, destPath);
        
        break;

    case show:
        if(argc < 4){
            flag = -1;
            goto EXIT;
        }
        flag = run_show(sock_fd, srcPath);

        break;

    default:
        flag = -1;
    }
    

	
    EXIT:

    if(argc >= 4) free(srcPath);
    if(argc == 5) free(destPath);

    if(flag == 0) {
        printf("done!\n");
        return EXIT_SUCCESS;
    }else if(flag == -1) {
        printf("usage: ./client [ip]:[port] list\n");
        printf("       ./client [ip]:[port] get [srcPath] [destPath]\n");
        printf("       ./client [ip]:[port] put [srcPath] [destPath]\n");
        printf("       ./client [ip]:[port] show [srcPath]\n");
        EXIT_FAILURE;
    }else{
        printf("error exits\n");
        EXIT_FAILURE;
    }


}

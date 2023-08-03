#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h> 

#include <netinet/in.h> 
#include <libgen.h>

#define BUFSIZE 512
#define DEBUG
#define MODE_INPUT 0
#define MODE_OUTPUT 1
const int SUCCESS = 0;
const int FAILURE = 1;

int port;
char* homePath;

typedef struct message{
	suseconds_t timestamp;
	char contents[BUFSIZE];
} msg;

msg cache[10];


// TODO how to use...?
int send_bytes(int fd, char * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, 0) ;
        if (sent == -1)
                return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

int recv_bytes(int fd, char * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = recv(fd, p, len - acc, 0) ;
        if (sent == -1)
                return 1 ;
        p += sent ;
        acc += sent ;
    }
    return 0 ;
}

// for output client -> send messages after recv timestamp
int listFiles(char* srcPath, int conn){

    DIR* src = NULL;
    struct dirent* one = NULL;
    char nextSrc[BUFSIZE];
    struct stat srcStat;
    lstat(srcPath, &srcStat);
    struct server_header sheader;


    if((src = opendir(srcPath)) == NULL){
        perror("[listFiles]");
        goto SEND_ERROR;
    }

    //read all the file/sudir informations in src path and send to client
    //recursively explore the subdirs 
    while(one = readdir(src)){

        char* dname = one->d_name;
        sprintf(nextSrc, "%s/%s", srcPath, dname);

        if(one->d_type == DT_DIR){ //dir: recursively explore 
            if(dname[0] == '.') continue; //except . and .. dir 

            if(listFiles(nextSrc, conn)) { 
                perror("[listFiles:dir]");
                goto SEND_ERROR;
            }

        }else if(one->d_type == DT_LNK){
            continue;

        }else{ 
            sheader.is_error = 0;
            sheader.payload_size = strlen(&nextSrc[strlen(homePath)+1]);
            //send header
            if(!(s = send(conn, &sheader, SHEADER_SIZE, 0)) || (s != SHEADER_SIZE)){
                perror("[cannot send header]");
                goto SEND_ERROR;
            }
            //send payload 
            if(!(s = send(conn, &nextSrc[strlen(homePath)+1], sheader.payload_size, 0)) || (s != sheader.payload_size)){
                perror("[cannot send payload]");
                goto SEND_ERROR;
            }
        }

    }
    
    closedir(src);
    return EXIT_SUCCESS;

    SEND_ERROR:
    if(src != NULL) closedir(src);
    sheader.is_error = 1;
    sheader.payload_size = 0;
    //send header
    if(!(s = send(conn, &sheader, SHEADER_SIZE, 0)) || (s != SHEADER_SIZE)){
        perror("[cannot send header]");
    }

    return EXIT_FAILURE;

}



int putFile(struct client_header cheader, int conn){
    
    int s = 0;
    char srcPath[BUFSIZE];
    char destPath[BUFSIZE];
    char buf[BUFSIZE];
    FILE* srcFile = NULL;
    printf("size of recv payload: %d\n", cheader.des_path_len);

    //recv the payload from client 
    if(!(s = recv(conn, srcPath, cheader.src_path_len, 0))){
        perror("[cannot read payload]");
        close(conn);
        return EXIT_FAILURE;
    }
    srcPath[s] = 0;
    //recv the payload from client 
    if(!(s = recv(conn, buf, cheader.des_path_len, 0))){
        perror("[cannot read payload]");
        close(conn);
        return EXIT_FAILURE;
    }
    buf[s] = 0;
    
    if(!strcmp(buf, ".")){
        sprintf(destPath, "%s/%s", homePath, basename(srcPath));
    }else{
        sprintf(destPath, "%s/%s", homePath, buf);
        makeDirectory(destPath);
        strcat(destPath, "/");
        strcat(destPath, basename(srcPath));
    }
    
    printf("dest: %s\n", destPath);
    
    //open the file
    if((srcFile = fopen(destPath, "wb")) == NULL){
        perror("[cannot make file]");
        close(conn);
        return EXIT_FAILURE;
    }

    while ((s = recv(conn, buf, BUFSIZE-1, 0))) {
        buf[s]=0;
        if(!fwrite(buf, 1, s, srcFile)) break;
        // printf("%s", buf);
    }

    fclose(srcFile);

}


void* runner(void* arg){


    int conn = *(int*)arg;
    free(arg);

    int s = 0;
    char srcPath[BUFSIZE];
    char destPath[BUFSIZE];
    char buf[BUFSIZE] ;
    
    //recv the header from client
    struct client_header cheader;
    if(!recv(conn, &cheader, sizeof(cheader), 0)){
        perror("[cannot read header: cmd]");
        close(conn);
        return NULL;
    }

    switch (cheader.command){

    case list:

        printf("> list request accepted\n");
        if(listFiles(homePath, conn)){
            perror("cannot list the files\n");
        }
        shutdown(conn, SHUT_WR) ;
        break;

    case get:
        printf("> get request accepted\n");
        if(showFile(cheader, conn)){
            perror("cannot get the files\n");
        }
        shutdown(conn, SHUT_WR) ;
        break;

    case put:
        printf("> put request accepted\n");
        if(putFile(cheader, conn)){
            perror("cannot put the files\n");
        }
        shutdown(conn, SHUT_WR) ;
        break;

    case show:

        printf("> show request accepted\n");
        if(showFile(cheader, conn)){
            perror("cannot show the file\n");
        }
        shutdown(conn, SHUT_WR) ;
        break;
    
    default:
        break;
    }
    
    printf("done......\n");
    close(conn);
    return NULL;
}


int main(int argc, char** argv){

    if(argc < 4){
        printf("[server] invalid command\n");
        printf("usasg: \n");
        return EXIT_FAILURE;
    }
    
    //TODO getopt 
    //-p port, -d directory, & <-background execution 
    port = atoi(argv[2]); 
    homePath = (char*)malloc(sizeof(char)*strlen(argv[4]) + 1);
    strcpy(homePath, argv[4]); 
    printf("port: %d path: %s\n", port, homePath);


	int listen_fd, new_socket;
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	char buffer[BUFSIZE] = {0}; 

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
        free(homePath);
		exit(EXIT_FAILURE); 
	}

	
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(port); 
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
        free(homePath);
		exit(EXIT_FAILURE); 
	} 

	while (1) {
         
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) { 
			perror("listen failed"); 
            free(homePath);
			exit(EXIT_FAILURE); 
		}

        new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept"); 
            free(homePath);
			exit(EXIT_FAILURE); 
		} 

        printf("> someone is here... %d\n", address.sin_port);

        int* passing_sock  = (int*)malloc(sizeof(int));
        (*passing_sock) = new_socket;
        pthread_t client_pid;
        if(pthread_create(&client_pid, NULL, (void*)runner, (void*)passing_sock)){
            perror("make new thread");
            free(homePath);
            close(new_socket);
        }
        
	}
    

    free(homePath);
    return EXIT_SUCCESS;

}
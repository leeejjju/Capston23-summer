// Partly taken from https://www.geeksforgeeks.org/socket-programming-cc/
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#define BUFFER_SIZE 128
#define MAX_MSG_COUNT 10
#define MODE_INPUT 0
#define MODE_OUTPUT 1

typedef struct message{
    char  text[513];
    struct timeval timestamp ;
}message_t ;

message_t saved_msg[MAX_MSG_COUNT];

int next_index = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;

int 
recv_bytes(int socket_fd, void * ptr, int recv_size)
{
    char * local_ptr = (char *) ptr ;
    ssize_t acc_size = 0; 
    ssize_t size;
    while(acc_size < recv_size)
    {
        size = recv(socket_fd, local_ptr + acc_size, recv_size - acc_size, 0);
        acc_size += size;
        
        if(size == -1 || size == 0)
        {
            return -1;
        }
    }
    return acc_size; 
}

int 
send_bytes(int socket_fd, void * ptr, int send_size)
{
    char * local_ptr = (char *) ptr ;
    ssize_t size;
    ssize_t acc_size = 0;
    while(acc_size < send_size)
    {
        size = send(socket_fd, local_ptr + acc_size, send_size - acc_size, MSG_NOSIGNAL);
        acc_size += size;
        if(size == -1 || size == 0)
        {
            return -1;
        }
    }
    return acc_size;
}

/* 
compare two timeval
return 0 if a >= b, 
return 1 if a < b 
*/
int 
timecmp(struct timeval a, struct timeval b)
{
   if(a.tv_sec != b.tv_sec) return (a.tv_sec < b.tv_sec);
   else return (a.tv_usec < b.tv_usec);
}

int 
send_msg(int socket, int index)
{
    int text_len = strlen(saved_msg[index].text);
    
    //send len
    if( send_bytes(socket, &text_len, sizeof(int)) == -1)
    {
        perror("cannot send len");
        return 1;
    }
    //send timestamp
    if( send_bytes(socket, &(saved_msg[index].timestamp), sizeof(struct timeval)) == -1 )
    {
        perror("cannot send timestamp");
        return 1;
    }
    //send text
    if( send_bytes(socket, saved_msg[index].text, text_len) == -1 )
    {
        perror("cannot send text");
        return 1;
    }
   
    printf("send_message: %s\n",saved_msg[index].text) ;
    return 0; 
}

void * 
input_thread (void * arg)
{
    printf("[INPUT] input client connected \n") ;
    int socket = *((int *)arg) ;
    free(arg) ;

    int text_size ; 
    if(recv_bytes(socket, (void *) &text_size, sizeof(int)) == -1){
        close(socket) ;
        return NULL;
    }
    printf("[INPUT] text size:%d\n", text_size) ;

    char buf[BUFFER_SIZE];
    int len;
    
    if((len = recv_bytes(socket, &buf, text_size)) == -1){
        printf("[INPUT] recv_fail\n") ;
        close(socket);
        return NULL;
    }
    buf[len] = 0;
    printf("[INPUT] msg from input_client: %s\n\tsocket:%d\n", buf, socket) ;
    

    pthread_mutex_lock(&mutex);

    strcpy(saved_msg[next_index].text, buf);
    gettimeofday(&(saved_msg[next_index].timestamp), NULL);
    next_index = (next_index + 1)%MAX_MSG_COUNT;

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    

    close(socket) ;    
    printf("[INPUT] input client disconnected \n") ;
    return NULL ;
}



void
* output_thread(void * arg)
{
    printf("[OUTPUT] output client connected \n");
    int socket = *(int*) arg;
    free(arg) ;

    struct timeval client_time ;
    if( recv_bytes(socket, &client_time, sizeof(struct timeval)) == -1)
    {
        close(socket);
        return NULL;
    }

    pthread_mutex_lock(&mutex);

    int latest_index = (next_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
    while(!timecmp(client_time, saved_msg[latest_index].timestamp))
    {
        pthread_cond_wait(&cond, &mutex);
        latest_index = (next_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
    }
    
    //find the msgs to send (from oldest to latest)
    int oldest_index = next_index;
    for(int i= 0; i < MAX_MSG_COUNT; i++)
    {
        int index = (oldest_index + i) % MAX_MSG_COUNT;
        if(timecmp(client_time, saved_msg[index].timestamp))
        {
            if(send_msg(socket, index) == 1)
            {
                close(socket);
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        } 
    }
    pthread_mutex_unlock(&mutex);
   
    close(socket);
    printf("[OUTPUT] output client disconnected \n") ;
    return NULL ;
}

//return 0 on success, return 1 on failure 
int checkPort(char* argv){
    int port;
    
    for(int i=0; i<strlen(argv); i++){
		if(isdigit(argv[i]) == 0 ){
			return 1;
		}
	}

	port = atoi(argv) ;
    if(  port < 1024  && 49151 < port)
    {
		return -1;
    }

    printf("Port: %d\n", port);
    return port;
}


int 
main(int argc, char *argv[]) 
{

    if(argc < 2)
    {
        fprintf(stderr, "usage: %s --player [num of player] --turn [num of ture] --corpus [sentences file]", argv[0]) ;
		exit(EXIT_FAILURE);
    }

    int port;
    if((port = checkPort(argv[1])) == -1){
        fprintf(stderr, "Invalid port: %s\n", argv[1]) ;
        exit(EXIT_FAILURE);
    }

    //make server socket and bind 
	int listen_fd ; 
	struct sockaddr_in address; 
	int addrlen = sizeof(address); 
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == 0){ 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port); 
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(EXIT_FAILURE); 
	} 

    //listen and give a new thread (loop)
	pthread_t tid ;
	while (1) {

		if (listen(listen_fd, 16) < 0) { 
			perror("listen failed"); 
			continue;
		} 

		int * new_socket = malloc(sizeof(int)); 
		*new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept failed"); 
			continue;
		}
		
        //mode check
	    int mode;
        if(recv_bytes(*new_socket, (void *)&mode, sizeof(int)) == -1){
            close(*new_socket);
            perror("cannot recv mode");
            continue;
        }

        //client 구분
        if(mode == MODE_INPUT){
            if (pthread_create(&tid, NULL, input_thread, new_socket) != 0) {
			    close(*new_socket);
                perror("cannot create input thread") ;
			    continue;
		    }
        }
        else if(mode == MODE_OUTPUT){  
            if (pthread_create(&tid, NULL, output_thread, new_socket) != 0) {
                close(*new_socket);
                perror("cannot create output thread") ;
                continue;
            }
        }
        else{ 
            close(*new_socket);
        }
	}

    return 0;
} 
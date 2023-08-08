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

#define BUFFER_SIZE 513
#define MAX_MSG_COUNT 10
#define MODE_INPUT 0
#define MODE_OUTPUT 1

typedef struct message{
    char  text[513];
    struct timeval timestamp ;
}message_t ;

message_t saved_msg[MAX_MSG_COUNT] = {0};

int latest_index = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;

int 
recv_bytes(int socket_fd, void * ptr, int recv_size)
{
    char * local_ptr = (char *) ptr ;
    ssize_t received_size = 0;
    ssize_t size;
    while(received_size < recv_size)
    {
        size = recv(socket_fd, local_ptr + received_size, recv_size - received_size, 0);
        received_size += size;
        
        if(size == -1 || size == 0)
        {
            return -1;
        }
    }
    return received_size;
}

int 
send_bytes(int socket_fd, void * ptr, int send_size)
{
    char * local_ptr = (char *) ptr ;
    ssize_t size;
    ssize_t sended_size = 0;
    while(sended_size < send_size)
    {
        size = send(socket_fd, local_ptr + sended_size, send_size - sended_size, MSG_NOSIGNAL);
        sended_size += size;
        if(size == -1 || size == 0)
        {
            return 1;
        }
    }
    return sended_size;
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
    ssize_t req_send = 0, total_send = 0;
    int msg_len = strlen(saved_msg[index].text);
    
    if( send_bytes(socket, &msg_len, sizeof(int)) == -1)
    {
        perror("cannot send len");
        return 1;
    }

    if( send_bytes(socket, &(saved_msg[index].timestamp), sizeof(struct timeval)) == -1 )
    {
        perror("cannot send timestamp");
        return 1;
    }

    if( send_bytes(socket, saved_msg[index].text, msg_len) == -1 )
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
    printf("[INPUT]input client connected \n") ;
    int socket = *((int *)arg) ;
    free(arg) ;

    int msg_size ; 
    if(recv_bytes(socket, (void *) &msg_size, sizeof(int)) == -1){
        close(socket) ;
        printf("[INPUT] someting wrong with receving...\n");
        return NULL;
    }
    printf("[INPUT] msg size:%d\n", msg_size) ;


    char buf[BUFFER_SIZE];
    int len;
    if((len = recv_bytes(socket, &buf, msg_size)) == -1){
        printf("[INPUT]recv_fail\n") ;
        close(socket);
        return NULL;
    }
    buf[len] = 0;
    printf("[INPUT]msg from input_client: %s\n\tsocket:%d\n", buf, socket) ;
    pthread_mutex_lock(&mutex);

    strcpy(saved_msg[latest_index].text, buf);
    gettimeofday(&(saved_msg[latest_index].timestamp), NULL);
    latest_index = (latest_index + 1)%MAX_MSG_COUNT;
    pthread_cond_broadcast(&cond);
    
    pthread_mutex_unlock(&mutex);
    
    close(socket) ;    
    printf("[INPUT]input client disconnected \n") ;
    return NULL ;
}



void
* output_thread(void * arg)
{
    printf("<OUTPUT>output client connected \n");
    int socket = *(int*) arg;
    free(arg) ;
    struct timeval client_time ;
    
    if( recv_bytes(socket, &client_time, sizeof(struct timeval)) == -1)
    {
        close(socket);
        return NULL;
    }




    pthread_mutex_lock(&mutex);
    int cond_index = (latest_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
    while(!timecmp(client_time, saved_msg[cond_index].timestamp))
    {
        printf("[%d] im waiting ...\n", socket);
        pthread_cond_wait(&cond, &mutex);
        cond_index = (latest_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
        printf("[%d] im reconnected ...\n", socket);
    }
    
    int oldest_index = (latest_index + 1) % MAX_MSG_COUNT;
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
    // printf("<OUTPUT>recent_time : %ld %ld\n",recent_time.tv_sec, recent_time.tv_usec) ;
    printf("<OUTPUT>output client end \n") ;
    return NULL ;
}

int 
main(int argc, char *argv[]) 
{

    if(argc < 2)
    {
        fprintf(stderr, "usage: %s <port>",argv[0]) ;
		exit(EXIT_FAILURE);
    }
	for(int i=0; i<strlen(argv[1]); i++){
		if(isdigit(argv[1][i]) == 0 ){
			fprintf(stderr, "Invalid port: %s\n",argv[1]) ;
			exit(EXIT_FAILURE);
		}
	}

	int port = atoi(argv[1]) ;
    if(  port < 1024  && 49151 < port)
    {
        fprintf(stderr, "Invalid port: %s\n",argv[1]) ;
		exit(EXIT_FAILURE);
    }
    printf("Port: %d\n", port);


	int listen_fd ; 
	struct sockaddr_in address; 
	int addrlen = sizeof(address); 


	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}

	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(port); 
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(EXIT_FAILURE); 
	} 

    char buffer[1024] = {0}; 
	pthread_t tid ;
	while (1) {
        //새로운 연결 생성
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) { 
			perror("listen failed : "); 
			exit(EXIT_FAILURE); 
		} 
		int * new_socket = malloc(sizeof(int)); 
		*new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		}
		
        //mode check

	    int mode;
        if(recv_bytes(*new_socket, (void *)&mode, sizeof(int)) == -1){
            close(*new_socket);
            perror("cannot recv mode");
            return EXIT_FAILURE;
        }

        //client 구분
        if(mode == MODE_INPUT){
            if (pthread_create(&tid, NULL, input_thread, new_socket) != 0) {
			    perror("pthread_create : ") ;
			    exit(EXIT_FAILURE) ;
		    }
        }
        else if(mode == MODE_OUTPUT){  
            if (pthread_create(&tid, NULL, output_thread, new_socket) != 0) {
                perror("pthread_create : ") ;
                exit(EXIT_FAILURE) ;
            }
        }
        else{ //mode err  에러 리턴, 연결 해재
            close(*new_socket) ;
        }

	}

} 
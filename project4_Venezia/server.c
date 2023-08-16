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
#include <getopt.h>

#define BUFSIZE 128
#define MODE_INPUT 0
#define MODE_OUTPUT 1
#define MODE_REGISTER 2

typedef struct user{
    char username[10];
    char password[10];
    int score;
} user;
user* players;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;
int start = 0;
char sentence[BUFSIZE];

int port, num_player, turn;
char* sourceFile;

int recv_bytes(int socket_fd, void * ptr, int recv_size) {
    char * local_ptr = (char *) ptr ;
    ssize_t acc_size = 0; 
    ssize_t size;
    while(acc_size < recv_size){
        size = recv(socket_fd, local_ptr + acc_size, recv_size - acc_size, 0);
        acc_size += size;
        
        if(size == -1 || size == 0){
            return -1;
        }
    }
    return acc_size; 
}

int send_bytes(int socket_fd, void * ptr, int send_size) {
    char * local_ptr = (char *) ptr ;
    ssize_t size;
    ssize_t acc_size = 0;
    while(acc_size < send_size){
        size = send(socket_fd, local_ptr + acc_size, send_size - acc_size, MSG_NOSIGNAL);
        acc_size += size;
        if(size == -1 || size == 0){
            return -1;
        }
    }
    return acc_size;
}

void getArgs(int argc, char** argv){

    if(argc < 9) goto EXIT;
    
    struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"player",  required_argument, 0,  'u'},
        {"turn"  ,  required_argument, 0,  't'},
        {"corpus",  required_argument, 0,  'c'}
    };
    
    int c ;
    int option_index = 0 ;

    do{
        
        c = getopt_long(argc, argv, "p:u:t:c:", long_options, &option_index);
        switch (c){
        case 'p': //port
            port = atoi(optarg);
            break;
        case 'u': //player
            num_player = atoi(optarg);
            break;
        case 't': //turn
            turn = atoi(optarg);
            break;
        case 'c': //corpus
            sourceFile = strdup(optarg);
            break;
        case -1: break;
        default:
            goto EXIT;
        }
    }while(!(c == -1));
    
    printf("port: %d player:%d  turn:%d  corpus:\"%s\"\n", port, num_player, turn, sourceFile);
    players = (user*)malloc(sizeof(user)*num_player);
    return;

    EXIT:
    printf("Usage: server [options]\n"
        "Options:\n"
        "  -p, --port       The port of the server.\n"
        "  -u, --player  The number of user(player)\n"
        "  -t, --turn  The number of turn.\n"
        "  -c, --corpus  The source file path for game\n");
    exit(EXIT_FAILURE);

}


// void* input_thread (void * arg) {
//     printf("[INPUT] input client connected \n") ;
//     int socket = *((int *)arg) ;
//     free(arg) ;

//     int text_size ; 
//     if(recv_bytes(socket, (void *) &text_size, sizeof(int)) == -1){
//         close(socket) ;
//         return NULL;
//     }
//     printf("[INPUT] text size:%d\n", text_size) ;

//     char buf[BUFSIZE];
//     int len;
    
//     if((len = recv_bytes(socket, &buf, text_size)) == -1){
//         printf("[INPUT] recv_fail\n") ;
//         close(socket);
//         return NULL;
//     }
//     buf[len] = 0;
//     printf("[INPUT] msg from input_client: %s\n\tsocket:%d\n", buf, socket) ;
    

//     pthread_mutex_lock(&mutex);

//     strcpy(saved_msg[next_index].text, buf);
//     gettimeofday(&(saved_msg[next_index].timestamp), NULL);
//     next_index = (next_index + 1)%MAX_MSG_COUNT;

//     pthread_cond_broadcast(&cond);
//     pthread_mutex_unlock(&mutex);
    

//     close(socket) ;    
//     printf("[INPUT] input client disconnected \n") ;
//     return NULL ;
// }

// void* output_thread(void * arg) {
//     printf("[OUTPUT] output client connected \n");
//     int socket = *(int*) arg;
//     free(arg) ;

//     struct timeval client_time ;
//     if( recv_bytes(socket, &client_time, sizeof(struct timeval)) == -1)
//     {
//         close(socket);
//         return NULL;
//     }

//     pthread_mutex_lock(&mutex);

//     int latest_index = (next_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
//     while(!timecmp(client_time, saved_msg[latest_index].timestamp))
//     {
//         pthread_cond_wait(&cond, &mutex);
//         latest_index = (next_index + (MAX_MSG_COUNT -1)) % MAX_MSG_COUNT;
//     }
    
//     //find the msgs to send (from oldest to latest)
//     int oldest_index = next_index;
//     for(int i= 0; i < MAX_MSG_COUNT; i++)
//     {
//         int index = (oldest_index + i) % MAX_MSG_COUNT;
//         if(timecmp(client_time, saved_msg[index].timestamp))
//         {
//             if(send_msg(socket, index) == 1)
//             {
//                 close(socket);
//                 pthread_mutex_unlock(&mutex);
//                 return NULL;
//             }
//         } 
//     }
//     pthread_mutex_unlock(&mutex);
   
//     close(socket);
//     printf("[OUTPUT] output client disconnected \n") ;
//     return NULL ;
// }

int main(int argc, char *argv[]) {

    getArgs(argc, argv);

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
    int connected = 0;
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
        if(mode == MODE_REGISTER){
            if(connected >= num_player){
                pritnf("user rejected\n");
                close(*new_sockt);
            }
            if (pthread_create(&tid, NULL, input_thread, new_socket) != 0) {
			    close(*new_socket);
                perror("cannot create input thread") ;
			    continue;
		    }else connected++;
            
        }else if(mode == MODE_INPUT){
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

    //TODO idk if it meaningful... 
    free(sourceFile);
    return 0;

} 
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

#define BUFFER_SIZE 513
#define MAX_MSG_COUNT 10
#define MODE_INPUT 0
#define MODE_OUTPUT 1
#define MODE_LOGIN 2

typedef struct message{
    char  text[513];
    struct timeval timestamp ;
}message_t ;
message_t saved_msg[MAX_MSG_COUNT];
int next_index = 0;

typedef struct player {
	char username[10] ;
	char password[10] ;
	int score ;
    int input_enable ;
} player;
player * players;
int num_player ;
int num_turn ;
char * corpus_file_path ;
int player_count = 0;
int problem_num ;
char problem[128] ;
int flag_correct = 0;

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
// pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;
pthread_mutex_t problem_mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t problem_cond = PTHREAD_COND_INITIALIZER ;
pthread_mutex_t answer_check_mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t answer_check_cond = PTHREAD_COND_INITIALIZER ;
pthread_mutex_t player_mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t player_cond = PTHREAD_COND_INITIALIZER ;

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

    return port;
}

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
    //check enable data
    //send( username + password + text)
    char username[10] ={0};
    char password[10] ={0};
    char player_answer[128] ={0};
    if(recv_bytes(socket, (void *)username, 10) == -1){
        close(socket);
        perror("cannot recv username");
        return NULL;
    }
    if(recv_bytes(socket, (void *)password, 10) == -1){
        close(socket);
        perror("cannot recv username");
        return NULL;
    }
    if(recv_bytes(socket, (void *)player_answer, 128) == -1){
        close(socket);
        perror("cannot recv username");
        return NULL;
    }
    printf("[ANSWER INPUT] USERNAME: %s  PASSWORD: %s\n--ANSWER: %s\n",username, password, player_answer) ;
    int match_player_found = 0 ;
    int player_index ;
    for(int i = 0; i < num_player; i++){
        if(strcmp(players[i].username,username) == 0 && strcmp(players[i].password,password) == 0){
            match_player_found = 1 ;
            player_index = i ;
            printf("match player found\n");
        }
    }

    if(match_player_found == 0 ){
        fprintf(stderr, "no match player data\n") ;
        close(socket) ;
        return NULL ;
    }
    
    if(players[player_index].input_enable == 0){//refuse
        printf("no more input USERNAME: %s!\n",username) ;
        close(socket) ;
        return NULL ;
    }

    pthread_mutex_lock(&answer_check_mutex) ;
    if(strcmp(player_answer, problem) != 0){
        printf("wrong answer\n%s:%ld\n%s:%ld",player_answer,strlen(player_answer),problem,strlen(problem)) ;
        pthread_mutex_unlock(&answer_check_mutex) ;
        return NULL ;
    }
    printf("<correct answer>USERNAME:%s\n\n", username) ;
    flag_correct = 1 ;
    pthread_cond_broadcast(&answer_check_cond) ;
    pthread_mutex_unlock(&answer_check_mutex) ;

    close(socket) ;    
    printf("[INPUT] input client disconnected \n") ;
    return NULL ;
}

void * 
output_thread(void * arg)
{
    int socket = *(int*) arg;
    free(arg) ;
    printf("[OUTPUT%d] output client connected \n",socket);
    pthread_mutex_lock(&problem_mutex) ;
    pthread_mutex_lock(&player_mutex) ;
    while(player_count != num_player){
        printf("[WAIT %d]: wating player...\n",socket) ;
        pthread_cond_wait(&player_cond, &player_mutex) ;
    }
    pthread_mutex_unlock(&player_mutex) ;
    
    //문제 점수 보내기.
    // pthread_mutex_lock(&problem_mutex) ;
    int recent_problem_num = problem_num ;
    while(problem_num > 1 && recent_problem_num == problem_num){
        printf("[WAIT %d]: wating problem...\n",socket) ;
        pthread_cond_wait(&problem_cond, &problem_mutex) ;
    }
    //문제
    printf("<send problem> :%s\n",problem) ;
    if( send_bytes(socket, problem, 128) == -1 ){
        perror("cannot send problem");
        pthread_mutex_unlock(&problem_mutex) ;
        return NULL;
    }
    pthread_mutex_unlock(&problem_mutex) ;

    //이름 점수
    for(int i = 0; i < num_player; i++){
        if( send_bytes(socket, players[i].username, 10) == -1 ){
            perror("cannot send username");
            return NULL;
        }
        if( send_bytes(socket, (void*)&players[i].score, sizeof(int)) == -1 ){
            perror("cannot send score");
            return NULL;
        } 
    }
    shutdown(socket, SHUT_WR) ;
    close(socket);
    printf("[OUTPUT%d] output client disconnected \n", socket) ;
    return NULL ;
}

void *
login_thread (void * arg){
    int socket  = *((int*)arg) ;
    free(arg) ;

    pthread_mutex_lock(&player_mutex) ;
    if(player_count >= num_player){
        close(socket);
        fprintf(stderr, "Login deny") ;
        pthread_mutex_unlock(&player_mutex) ;
        return NULL ;
    }
    if(recv_bytes(socket, (void *)&players[player_count].username, 10) == -1){
        close(socket);
        perror("cannot recv username");
        pthread_mutex_unlock(&player_mutex) ;
        return NULL ;
    }
    if(recv_bytes(socket, (void *)&players[player_count].password, 10) == -1){
        close(socket);
        perror("cannot recv username");
        pthread_mutex_unlock(&player_mutex) ;
        return NULL ;
    }
    players[player_count].input_enable = 1 ;
    printf("[LOGIN] USERNAME: %s  PASSWORD: %s\n",players[player_count].username, players[player_count].password) ;
    player_count += 1 ;
    pthread_cond_broadcast(&player_cond) ;
    pthread_mutex_unlock(&player_mutex) ;

    close(socket) ;
    return NULL ;
}

void *
problem_load_thread (void * arg){
    //preload first problem
    FILE* corpus_fd ;
    if((corpus_fd = fopen(corpus_file_path, "r")) == NULL){
        perror("open corpus\n") ;
        exit(EXIT_FAILURE) ;
    }

    if(fgets( problem, sizeof(problem), corpus_fd) == NULL){
        perror("fgets\n") ;
        exit(EXIT_FAILURE) ;
    }
    problem[strlen(problem) - 1] = 0x0 ;//TODO if
    problem_num += 1 ;
    
    pthread_mutex_lock(&answer_check_mutex) ;
    while(problem_num <= num_turn ){
        while(flag_correct == 0 ){
            pthread_cond_wait(&answer_check_cond, &answer_check_mutex) ;
        }

        pthread_mutex_lock(&problem_mutex) ;
        if(fgets( problem, sizeof(problem), corpus_fd) == NULL){
            perror("fgets\n") ;
            exit(EXIT_FAILURE) ;
        }
        problem[strlen(problem) - 1] = 0x0 ;
        printf("{NEW PROBLEM}:%s\n",problem) ;
        problem_num += 1 ;
        pthread_cond_broadcast(&problem_cond) ;
        pthread_mutex_lock(&problem_mutex) ;
        flag_correct = 0 ;
    }
    pthread_mutex_unlock(&answer_check_mutex) ;

    fclose(corpus_fd) ;
    return NULL ;
}

int 
main(int argc, char *argv[]) 
{
    if(argc < 8)
    {
        fprintf(stderr, "usage: %s --player [num of player] --turn [num of ture] --corpus [sentences file] -p [port] ", argv[0]) ;
		exit(EXIT_FAILURE);
    }

    struct option long_options[] = {
        {"player",  required_argument, 0,  0 },
        {"turn"  ,  required_argument, 0,  0 },
        {"corpus",  required_argument, 0,  0 }
    };
    
    int c ;
    int option_index = 0 ;
    int port;

    while( (c = getopt_long(argc, argv, "p:", long_options, &option_index)) != -1){
        switch (c)
        {
        case 0:
            if(option_index == 0 || option_index == 1){
                int opt_len = strlen(optarg) ;
                for(int i = 0; i< opt_len; i++){
                    if(!isdigit(optarg[i])){
                        fprintf(stderr,"opt error") ;
                        exit(EXIT_FAILURE) ;
                    }
                }
                if(option_index == 0){
                    num_player = atoi(optarg) ;
                }
                else{
                    num_turn = atoi(optarg) ;
                }
            }
            else{
                corpus_file_path = strdup(optarg) ;
            }
            break;
        case 'p':
            if((port = checkPort(optarg)) == -1){
                fprintf(stderr, "Invalid port: %s\n", optarg) ;
                exit(EXIT_FAILURE);
            }
            break ;
        default:
            fprintf(stderr, "wrong option %o\n",c) ;
            break;
        }
    }
    
    printf("player:%d  turn:%d  corpus:%s  port:%d\n",num_player, num_turn, corpus_file_path, port);

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

    players = malloc(sizeof(player) * num_player) ;
    

    // while(player_count < num_player){
	// 	if (listen(listen_fd, 16) < 0) { 
	// 		perror("listen failed"); 
	// 		continue;
	// 	} 
	// 	int * socket = malloc(sizeof(int)); 
	// 	*socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
	// 	if (socket < 0) {
	// 		perror("accept failed"); 
	// 		continue;
	// 	}
    //     if (pthread_create(&tid, NULL, waiting_thread, socket) != 0) {
    //         close(*socket);
    //         free(socket) ;
    //         perror("cannot create output thread") ;
    //         continue;
    //     }
    // }
     
    pthread_t tid ;

    if (pthread_create(&tid, NULL, problem_load_thread, NULL) != 0) {
        perror("cannot create problem load thread") ;
        return 1 ;
    }
	
	while (1) {

		if (listen(listen_fd, 16) < 0) { 
			perror("listen failed"); 
			continue;
		} 

		int * socket = malloc(sizeof(int)); 
		*socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (socket < 0) {
			perror("accept failed"); 
			continue;
		}
		
        //mode check
	    int mode;
        if(recv_bytes(*socket, (void *)&mode, sizeof(int)) == -1){
            close(*socket);
            free(socket) ;
            perror("cannot recv mode");
            continue;
        }

        //client 구분
        if(mode == MODE_INPUT){
            if (pthread_create(&tid, NULL, input_thread, socket) != 0) {
			    close(*socket);
                free(socket) ;
                perror("cannot create input thread") ;
			    continue;
		    }
        }
        else if(mode == MODE_OUTPUT){  
            if (pthread_create(&tid, NULL, output_thread, socket) != 0) {
                close(*socket);
                free(socket) ;
                perror("cannot create output thread") ;
                continue;
            }
        }
        else if(mode == MODE_LOGIN){
            if (pthread_create(&tid, NULL, login_thread, socket) != 0) {
                close(*socket);
                free(socket) ;
                perror("cannot create login thread") ;
                continue;
            }
        }
        else{ 
            close(*socket);
        }
	}

    return 0;
} 
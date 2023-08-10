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
#include <fcntl.h>

#define BUFFER_SIZE 128
#define MAX_MSG_COUNT 10
#define MODE_INPUT 0
#define MODE_OUTPUT 1
#define MODE_LOGIN 2


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t finish = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER ;

int port;
int max_player_count;
int max_game_turn;
int curr_game_turn = 1;
char * game_source;

FILE * game_source_fp;

char problem[128];
char GameEnd[128]={"Game End\0"};

int go_to_next_problem = 1;
int recv_next_problem_player_count = 0;

int enter_player_count = 0;

typedef struct
{
    char playername[10];
    char password[10];
    int score; 
}player;

player * player_info;

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

int check_exist_player(char * playername, char * password)
{
    for(int i=0; i<max_player_count; i++)
    {
        if((strcmp(playername, player_info[i].playername) == 0) && (strcmp(password, player_info[i].password) == 0))
        {
            return 1;
        }
    }
    return 0;
}


int find_player_index(char * playername, char * password)
{
    for(int i=0; i<max_player_count; i++)
    {
        if((strcmp(playername, player_info[i].playername) == 0) && (strcmp(password, player_info[i].password) == 0))
        {
            return i;
        }
    }
    return -1;
}

void check_answer(char * buf, int index)
{
    if(strcmp(buf, problem) == 0)
    {
        fprintf(stderr,"%s is correct \n",player_info[index].playername);
        go_to_next_problem = 1;
        curr_game_turn++;
        player_info[index].score++;
        fgets(problem,sizeof(problem),game_source_fp);
        problem[strlen(problem) - 1] = 0;
    }
}

void * 
input_thread (void * arg)
{
    int socket = *((int *)arg) ;
    free(arg) ;
    fprintf(stderr,"[INPUT %d] input client connected \n",socket) ;
    if(enter_player_count != max_player_count)
    {
        close(socket);
        return NULL;
    }

    char playername[10];
    char password[10];

    if(recv_bytes(socket, (void *)playername, 10) == -1){
        close(socket) ;
        return NULL;
    }
    if(recv_bytes(socket, (void *)password, 10) == -1){
        close(socket) ;
        return NULL;
    }

    if(!check_exist_player(playername,password))
    {
        close(socket);
        return NULL;
    }

    char buf[BUFFER_SIZE];

    if(recv_bytes(socket, (void *)buf, 128) == -1){
        close(socket) ;
        return NULL;
    }
    close(socket) ; 
    fprintf(stderr,"[INPUT %d] %s user answer : %s\n",socket, playername ,buf);

    //정답을 맞추면 모든 player에게 새로운 문제를 보내고 나서 정답을 비교해하기 때문에 go_to_next_problem일 때에 멈추고 아닐 때만 비교
    while(go_to_next_problem);

    pthread_mutex_lock(&mutex);

    int index = find_player_index(playername,password); 
    check_answer(buf, index);

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    

       
    fprintf(stderr,"[INPUT %d] input client disconnected \n",socket) ;
    return NULL ;
}



void
* output_thread(void * arg)
{
    int socket = *(int*) arg;
    free(arg) ;
    fprintf(stderr,"[OUTPUT %d]output client connected \n",socket);

    while(max_player_count != enter_player_count);

    //TODO when end conditon, what the sever will do
    

    pthread_mutex_lock(&mutex);

    while(!go_to_next_problem)
    {
        fprintf(stderr,"%d waintig\n",socket);
        pthread_cond_wait(&cond, &mutex);
        fprintf(stderr,"%d awake\n",socket);
    }

    recv_next_problem_player_count++;
    if(recv_next_problem_player_count == max_player_count)
    {
        go_to_next_problem = 0; //정답을 비교할 수 있게 바꿔준다
        recv_next_problem_player_count = 0; //모든 player에게 보내졌으면 shutdown을 통해 진짜로 보낸다.
        //pthread_cond_broadcast(&wait_cond);
    }
    pthread_mutex_unlock(&mutex);

    if(curr_game_turn == max_game_turn + 1)
    {
        if(send_bytes(socket, GameEnd, 128) == -1)
        {
            // pthread_mutex_lock(&mutex);
            // max_player_count--;
            // pthread_mutex_unlock(&mutex);
        }
        for(int i=0; i<max_player_count; i++)
        {
            send_bytes(socket, player_info[i].playername, 10);
            send_bytes(socket, &player_info[i].score, sizeof(int));
        }
        close(socket);
        pthread_mutex_lock(&finish);
        enter_player_count--;
        pthread_mutex_unlock(&finish);
        while(enter_player_count);
        
        exit(EXIT_SUCCESS);
    }

    
    
    //TODO 게임 도중 나가지면 지속할 건지
    if(send_bytes(socket, problem, 128) == -1)
    {
        // pthread_mutex_lock(&mutex);
        // max_player_count--;
        // pthread_mutex_unlock(&mutex);
    }
    for(int i=0; i<max_player_count; i++)
    {
        send_bytes(socket, player_info[i].playername, 10);
        send_bytes(socket, &player_info[i].score, sizeof(int));
    }
    

    //모든 player에게 next problem을 보내면 그때 send
    while(recv_next_problem_player_count);

    shutdown(socket, SHUT_WR);
    fprintf(stderr,"[OUTPUT %d] sending...%s\n",socket,problem);
    // pthread_mutex_lock(&wait);
    // while(recv_next_problem_player_count)
    // {
    //     pthread_cond_wait(&wait_cond, &wait);
    // }
    // pthread_mutex_unlock(&wait);
    close(socket);
    fprintf(stderr,"[OUTPUT %d] output client disconnected \n",socket) ;
    return NULL ;
}

//return 0 on success, return 1 on failure 
int checkisdigit(char* argv){
    int digit;
    
    for(int i=0; i<strlen(argv); i++)
    {
      if(isdigit(argv[i]) == 0 )
        {
         return -1;
      }
   }
   digit = atoi(argv) ;
    return digit;
}


int 
main(int argc, char *argv[]) 
{

    if(argc < 9)
    {
      exit(EXIT_FAILURE);
    }

    static struct option long_options[]=
   {
      {"port", 1, 0, 0},
      {"player", 1, 0, 0},
      {"turn", 1, 0, 0},
      {"corpus", 1, 0, 0},
      {0, 0, 0, 0}
   };

   int opt_index;
   int option;
   while((option = getopt_long(argc, argv, "", long_options, &opt_index)) != -1)
   {
      
      switch (option)
      {
         case 0:
            if (long_options[opt_index].flag != 0)
                    break;
            if(strcmp(long_options[opt_index].name, "port") == 0)
            {
               port = checkisdigit(optarg);
               if(port == -1 || port < 1024 || 49151 < port)
                    {
                        exit(EXIT_FAILURE);
                    }
            }
            if(strcmp(long_options[opt_index].name, "player") == 0)
            {
               if( (max_player_count = checkisdigit(optarg)) == -1 )
                    {
                        exit(EXIT_FAILURE);
                    }
                    player_info = (player*)malloc(sizeof(player) * max_player_count);
            }
            if(strcmp(long_options[opt_index].name, "turn") == 0)
            {
               if( (max_game_turn = checkisdigit(optarg)) == -1 )
                    {
                        exit(EXIT_FAILURE);
                    }
            }
                if(strcmp(long_options[opt_index].name, "corpus") == 0)
            {
               game_source = strdup(optarg);
                    game_source_fp = fopen(game_source, "r");
                    //free(game_source);
                    if(game_source_fp == NULL)
                    {
                        exit(EXIT_FAILURE);
                    }
                    fgets(problem,sizeof(problem),game_source_fp);
                    problem[strlen(problem) - 1] = 0;
            }
            break;
         case '?':
         fprintf(stderr,"?\n");
            return 1;
      }
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



   pthread_t tid ;
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
            perror("cannot recv mode");
            continue;
        }

        if(mode == MODE_LOGIN)
        {
            if(enter_player_count != max_player_count)
            {
                char playername[10];
                char password[10];
                int exist_player = 0;
                recv_bytes(*socket,playername, 10);
                recv_bytes(*socket,password, 10);
                for(int i=0; i<enter_player_count; i++)
                {
                    if((strcmp(playername, player_info[i].playername) == 0) && (strcmp(password, player_info[i].password) == 0))
                    {
                        exist_player = 1;
                        break;
                    }
                }
                if(!exist_player)
                {
                    strcpy(player_info[enter_player_count].playername, playername);
                    strcpy(player_info[enter_player_count].password, password);
                    enter_player_count++;
                    fprintf(stderr,"user : %s\n",playername);
                }
            }
            close(*socket);
        }
        //client 구분
        else if(mode == MODE_INPUT){
            if (pthread_create(&tid, NULL, input_thread, socket) != 0) {
             close(*socket);
                perror("cannot create input thread") ;
             continue;
          }
        }
        else if(mode == MODE_OUTPUT){  
            fprintf(stderr,"[out]%d socket is connet\n",*socket);
            if (pthread_create(&tid, NULL, output_thread, socket) != 0) {
                close(*socket);
                perror("cannot create output thread") ;
                continue;
            }
        }
        else{ 
            fprintf(stderr,"weired\n");
            close(*socket);
        }

   }

    return 0;
} 
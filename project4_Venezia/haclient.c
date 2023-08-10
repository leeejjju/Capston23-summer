#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h> 
#include <sys/socket.h> 
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>

#define THEME BLUE
#define BUFSIZE 128
const int MODE_INPUT = 0;
const int MODE_OUTPUT = 1;

typedef enum{
   BLACK,
   BLUE,
   GREEN,
   RED,
   NOP
} color;

typedef struct Board{
   WINDOW * game_board;
   WINDOW * score_board;
}Board;

int done;
char username[10];
char ip[16];
int port;
char password[10];
int sen_cnt;

int send_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == 0)
                return 0 ;
        p += sent ;
        acc += sent ;
    }
    return acc ;
}

int recv_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = recv(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == 0)
                return 0 ;
        p += sent ;
        acc += sent ;
    }
    return acc ;
}

//make new sock and connection with ip,port and return the sock_fd(0 on fail)
int makeConnection(){

   int conn;
   struct sockaddr_in serv_addr; 
   conn = socket(AF_INET, SOCK_STREAM, 0) ;
   
   if (conn <= 0) {
      fprintf(stderr,  " socket failed\n");
      return 0;
   } 

   memset(&serv_addr, '\0', sizeof(serv_addr)); 
   serv_addr.sin_family = AF_INET; 
   serv_addr.sin_port = htons(port); 
   if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
      fprintf(stderr, " inet_pton failed\n");
      return 0;
   }
   if (connect(conn, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      fprintf(stderr, " connect failed\n");
      return 0;
   }

   return conn;

}

//init basic about main window
void init(){
   initscr();
   start_color(); //for using color option
   init_pair(0, COLOR_WHITE, COLOR_BLACK); //set color themes
   init_pair(1, COLOR_BLUE, COLOR_WHITE);
   init_pair(2, COLOR_GREEN, COLOR_WHITE);
   init_pair(3, COLOR_RED, COLOR_WHITE);
   bkgd(COLOR_PAIR(THEME));//select theme
   curs_set(2); //set corsor's visibility (0: invisible, 1: normal, 2: very visible)
   keypad(stdscr, TRUE); //enable to reading a function keys 
   cbreak(); //accept the special characters... 
   echo(); 
}

//print basic information to main window
void init_main(){
   mvprintw(2, 1, "2023-summer capston1-study");
   mvprintw(27, COLS-10, "@Jeongwon (c)leejinju");
   move(30, 0);
   
   refresh();

}

//init inputbox window
void init_inputBox(WINDOW* client){

   WINDOW* inBox = newwin(3, COLS, 24, 0);
   wbkgd(inBox, COLOR_PAIR(THEME));
   box(inBox, ACS_VLINE, ACS_HLINE);
   wrefresh(inBox);

   wbkgd(client, COLOR_PAIR(THEME));
   idlok(client, TRUE);
   scrollok(client, TRUE);
   wrefresh(client);
   refresh();
}

void * inputBox(void * c){
   WINDOW * client = (WINDOW*)c;
   char buf[BUFSIZE];
   int s;
   int conn;

   while( (sen_cnt <= 0)){

   }
   
   while(strcmp(buf, "quit") != 0){
      if(!(conn = makeConnection())){
         fprintf(stderr, "ERROR: Can't connect\n");
         return NULL;
      }

      wgetstr(client, buf);

      if(strlen(buf) > 128 || strlen(buf) <= 0){
         werase(client);
         mvwprintw(client, 0, 0, " cannot send :(");
         getch();
         werase(client);
         wrefresh(client);
         continue;
      }

      int len = strlen(buf);
      int mode = 0;

      if( !(s = send_bytes(conn, (void *)&mode, 4)) ){
         fprintf(stderr, " [cannot send your output mode data]\n");
         return NULL;
      }
      if( !(s = send_bytes(conn, username, 10)) ){
         fprintf(stderr, " [cannot send your username data]\n");
         return NULL;
      }
      if( !( s = send_bytes(conn, password, 10)) ){
         fprintf(stderr, " [cannot send your password data]\n");
         return NULL;
      }
      if( !( s = send_bytes(conn, buf, 128)) ){
         fprintf(stderr, " [cannot send your buffer data]\n");
         return NULL;
      }

      werase(client);
      wrefresh(client);
      close(conn);
   }
   done = 1;

   return NULL;
}

void init_outputBox(WINDOW* game_board, WINDOW * score_board){

   WINDOW* outBox_board = newwin(21, COLS/2, 3, 0);
   wbkgd(outBox_board, COLOR_PAIR(THEME));
   box(outBox_board, ACS_VLINE, ACS_HLINE);
   wrefresh(outBox_board);
   refresh();

   WINDOW* outBox_score = newwin(21, COLS/2, 3, COLS/2);
   wbkgd(outBox_score, COLOR_PAIR(THEME));
   box(outBox_score, ACS_VLINE, ACS_HLINE);
   wrefresh(outBox_score);
   mvwprintw(outBox_score, 1, 1, " ***** Score Board *****\n");
   wrefresh(outBox_score);
   refresh();

   wbkgd(game_board, COLOR_PAIR(THEME));
   idlok(game_board, TRUE);
   scrollok(game_board, TRUE);
   wprintw(game_board, " enter \"quit\" to exit!\n");
   wrefresh(game_board);

   wbkgd(score_board, COLOR_PAIR(THEME));
   idlok(score_board, TRUE);
   scrollok(score_board, TRUE);
   wrefresh(score_board);
   refresh();
}

void * outputBox(void * ss){
   Board * board = (Board *) ss; 
   WINDOW * game_board = board->game_board;
   WINDOW * score_board = board->score_board;
   char buf[BUFSIZE];
   int conn;
   int s, i = 0;
   int mode = 1;

   
   while(done != 1){
      if(!(conn = makeConnection())){
         fprintf(stderr," [cannnot connect]\n");
         return NULL;
      }
      if( !(s = send_bytes(conn, (void *)&mode, 4)) ){
         fprintf(stderr, " [cannot send your output mode data]\n");
         return NULL;
      }
      if( !(s = send_bytes(conn, username, 10)) ){
         fprintf(stderr, " [cannot send your username data]\n");
         return NULL;
      }
      if( !( s = send_bytes(conn, password, 10)) ){
         fprintf(stderr, " [cannot send your password data]\n");
         return NULL;
      }

      shutdown(conn, SHUT_WR);

      char sentence[128];
      if(!(s = recv_bytes(conn, sentence, 128)) ){
         fprintf(stderr, " [cannot recv sentence]\n");
         return NULL;
      }
      sen_cnt++;

      wprintw(game_board, "%d.--------------------------------------\n",sen_cnt);
      wrefresh(game_board);
      wprintw(game_board, "-> %s <-\n", sentence);
      wrefresh(game_board);
      wprintw(game_board, "----------------------------------------\n");
      wrefresh(game_board);
      refresh();

      char score_username [10];
      int score;

      werase(score_board);
      wrefresh(score_board);
      while( (s = recv_bytes(conn, score_username, 10))){
         if( !(s = recv_bytes(conn, (void *) &score, 4))){
            wprintw(score_board, "[cannot recv score]\n");
            close(conn);
            return NULL;
         }
         wprintw(score_board, " %10s:  %d\n",score_username, score);
         wrefresh(score_board);
      }
      if(close(conn) == -1){
         fprintf(stderr, " [cannot close the socekt]\n");
         return NULL;
      }
   }
   // free(time);
   return NULL;
}

void getoption(int argc, char *argv[]){
   int opt;
    struct option long_options[] = {
      {"ip", 1, NULL, 'i'},
      {"port", 1, NULL, 'o'},
      {"username", 1, NULL, 'u'},
      {"password", 1, NULL, 'p'},
      {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "i:o:u:p:", long_options, NULL)) != -1) {
      switch (opt) {
         case 'i':
            strcpy(ip, argv[2]);
            break;
         case 'o':
            port = atoi(argv[4]);
            break;
         case 'u':
            strcpy(username, argv[6]);
            break;    
         case 'p':
            strcpy(password, argv[8]);
            break;
         case '?':
            fprintf(stderr, "ERROR: Wrong input option\n");
            exit(EXIT_FAILURE);
            break;
      }
    }
}

int main(int argc, char *argv[]) { 
   getoption(argc, argv);
   //init windows
   init();
   init_main();
   Board * board = malloc(sizeof(Board));
   board->game_board= newwin(19, (COLS/2)-2, 4, 1);
   board->score_board= newwin(17, (COLS/2)-2, 6, (COLS/2)+1);
   init_outputBox(board->game_board, board->score_board);
   WINDOW* input = newwin(1, COLS-2, 25, 1);
   init_inputBox(input);
   refresh();
   //check argvs andn get information from it 
    if(argc < 8){
      mvwprintw(board->game_board, 0, 0, " invalid command!\n\n usage: venezia [ip]:[port]\n");
      wrefresh(board->game_board);
      goto EXIT;
    }

   int conn;
   if(!(conn = makeConnection())){
      mvwprintw(board->game_board, 0, 0, " fail to connect\n");
      wrefresh(board->game_board);
      goto EXIT;
   }

   int s;
   int mode = 2;
   if( !(s = send_bytes(conn, (void *)&mode, 4)) ){
      mvwprintw(board->game_board, 0, 0, " [cannot send your output mode data]\n");
      wrefresh(board->game_board);
      goto EXIT;
   }
   if( !(s = send_bytes(conn, username, 10)) ){
      mvwprintw(board->game_board, 0, 0, " [cannot send your username]\n");
      wrefresh(board->game_board);
      goto EXIT;
   }
   if( !( s = send_bytes(conn, password, 10)) ){
      mvwprintw(board->game_board, 0, 0, " [cannot send your password]\n");
      wrefresh(board->game_board);
      goto EXIT;
   }

   close(conn);

  //make threads
   pthread_t input_pid;
   if(pthread_create(&input_pid, NULL, (void*)inputBox, (void*)input)){
      perror("make new thread");
      mvwprintw(board->game_board, 0, 0, " make thread failed\n");
      wrefresh(board->game_board);
      goto EXIT;
   }
   pthread_t board_pid;
   if(pthread_create(&board_pid, NULL, (void*)outputBox, (void*)board)){
      perror("make new thread");
      mvwprintw(board->game_board, 0, 0, " make thread failed\n");
      wrefresh(board->game_board);
      goto EXIT;
   }

   refresh();

   //exit
   pthread_join(input_pid, NULL);
   noecho();

   mvwprintw(input, 0, 0, " Good bye :)");
   
   EXIT:
   wprintw(board->game_board, " press any key to exit… ");
   wrefresh(input);
   wrefresh(board->game_board);

   getch();

   delwin(input);
   delwin(board->game_board);
   delwin(board->score_board);
   free(board);
   endwin();
   return 0;

}
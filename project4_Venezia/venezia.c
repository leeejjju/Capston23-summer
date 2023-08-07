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

#define THEME BLUE
#define BUFSIZE 512
const int MODE_INPUT = 0;
const int MODE_OUTPUT = 1;

typedef enum{
	BLACK,
	BLUE,
	GREEN,
	RED,
	NOP
} color;

int isError = 0;
int done = 0;
char errorMsg[BUFSIZE];
char* ip;
int port = 0;

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

//get ip and port from args
void getIpAndPort(char** args){

	ip = (char*)malloc(sizeof(char)*strlen(args[1])+1);
    strcpy(ip, args[1]);
    int index = 0;
    for(int i = 0; i < strlen(ip); i++) if(ip[i] == ':') index = i;
    ip[index] = 0;
    port = atoi(&ip[index+1]);

	return;
}

//make new sock and connection with ip,port and return the sock_fd(0 on fail)
int makeConnection(){

	int conn;
	struct sockaddr_in serv_addr; 
	conn = socket(AF_INET, SOCK_STREAM, 0) ;
	
	if (conn <= 0) {
		strcpy(errorMsg,  " socket failed\n");
		return 0;
	} 

	memset(&serv_addr, '\0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		strcpy(errorMsg, " inet_pton failed\n");
		return 0;
	}
	if (connect(conn, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		strcpy(errorMsg, " connect failed\n");
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
	mvprintw(27, COLS-10, "@leeejjju");
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

void* inputBox(void* c){

	WINDOW* client = (WINDOW*)c;
	char buf[BUFSIZE];
	int s;
	int conn;
	
	while(1){

		wgetstr(client, buf);
		if(!strcmp(buf, "quit")){ 
			break;
			done = 1;
		}

		if(strlen(buf) > 512 || strlen(buf) == 0){
			werase(client);
			mvwprintw(client, 0, 0, " cannot send :(");
			getch();
			werase(client);
			wrefresh(client);
			continue;
		}

		if(strlen(buf) <= 0){
			werase(client);
			wrefresh(client);
			continue;
		}

		if(!(conn = makeConnection())){
			isError = 1;
			return NULL;
		}
		int len = strlen(buf);
		

		//TODO modify with send_bytes... 
		//send header
		if(!(s = send_bytes(conn, (void*)&MODE_INPUT, sizeof(int))) || !(s = send_bytes(conn, (void*)&len, sizeof(int)))){
			strcpy(errorMsg, " [cannot send header]\n");
			isError = 1;
			return NULL;
		}
		//send text 
		if(!(s = send_bytes(conn, (void*)buf, len))){
			strcpy(errorMsg, " [cannot send text]\n");
			isError = 1;
			return NULL;
		}
		shutdown(conn, SHUT_WR);

		if(!(s = recv_bytes(conn, (void*)&isError, sizeof(int))) || isError){
			strcpy(errorMsg, " [error on server]\n");
			isError = 1;
			return NULL;
		}

		werase(client);
		wrefresh(client);
		close(conn);
	}

	return NULL;
}

//init outputbox window
void init_outputBox(WINDOW* server){

	WINDOW* outBox = newwin(21, COLS, 3, 0);
	wbkgd(outBox, COLOR_PAIR(THEME));
	box(outBox, ACS_VLINE, ACS_HLINE);
	wrefresh(outBox);
	refresh();

	wbkgd(server, COLOR_PAIR(THEME));
	idlok(server, TRUE);
	scrollok(server, TRUE);
	wprintw(server, " enter \"quit\" to exit!\n");
	wrefresh(server);
	refresh();
}

void* outputBox(void* ss){

	WINDOW *server = (WINDOW*)ss;
	char buf[BUFSIZE];
	int conn;
	int s, i = 0;
	struct timeval lastTime;
	struct tm* time = (struct tm*)malloc(sizeof(struct tm));
	lastTime.tv_sec = 0;

	while(1){
		if(done) break;
		mvprintw(0, 0, "[%d] lasttime: %d", i++, lastTime.tv_sec);
		refresh();

		if(!(conn = makeConnection())){
			isError = 1;
			return NULL;
		}

		//send header
		if(!(s = send_bytes(conn, (void*)&MODE_OUTPUT, sizeof(MODE_OUTPUT))) || !(s = send_bytes(conn, (void*)&lastTime, sizeof(lastTime)))){
			strcpy(errorMsg, " [cannot send header]\n");
			isError = 1;
			return NULL;
		}
		shutdown(conn, SHUT_WR);

		while(1){
			if(done) break;
			int len = 0;
			//recv header
			if((s = recv(conn, &len, sizeof(len), 0)) == -1 || (s = recv(conn, &lastTime, sizeof(lastTime), 0)) == -1){
				strcpy(errorMsg, " [cannot recv header]\n");
				isError = 1;
				return NULL;
			}
			if(s == 0) break;
			//recv text
			if(!(s = recv(conn, buf, len, 0))){
				strcpy(errorMsg, " [cannot recv text]\n");
				isError = 1;
				return NULL;
			}
			if(s < BUFSIZE) buf[s] = 0;
			time = localtime(&(lastTime.tv_sec));
			wprintw(server, " [%d:%d:%d] %s\n", ((time->tm_hour > 12)? (time->tm_hour -12): time->tm_hour) , time->tm_min, time->tm_sec, buf);
			wrefresh(server);
			refresh();
		}
		close(conn);
	}
	free(time);
	return NULL;
}

int main(int argc, char *argv[]) {

	//init windows
	init();
	init_main();

	WINDOW* server = newwin(19, COLS-2, 4, 1);
	init_outputBox(server);
	WINDOW* client = newwin(1, COLS-2, 25, 1);
	init_inputBox(client);
	refresh();

	//check argvs andn get information from it 
    if(argc < 2){
		mvwprintw(server, 0, 0, " invalid command!\n\n usage: venezia [ip]:[port]\n");
		wrefresh(server);
		goto EXIT;
    }else getIpAndPort(argv);

	//make threads
	pthread_t client_pid;
	if(pthread_create(&client_pid, NULL, (void*)inputBox, (void*)client)){
		perror("make new thread");
		mvwprintw(server, 0, 0, " make thread failed\n");
		wrefresh(server);
		goto EXIT;
	}
	pthread_t server_pid;
	if(pthread_create(&server_pid, NULL, (void*)outputBox, (void*)server)){
		perror("make new thread");
		mvwprintw(server, 0, 0, " make thread failed\n");
		wrefresh(server);
		goto EXIT;
	}
	refresh();

	//exit
	pthread_join(client_pid, NULL);
	noecho();

	if(isError){
		mvwprintw(server, 0, 0, "%s", errorMsg);
		wrefresh(server);
		goto EXIT;
	}
	mvwprintw(client, 0, 0, " Good bye :)");
	
	EXIT:
	wprintw(server, " press any key to exit... ");
	wrefresh(client);
	wrefresh(server);

	getch();

	free(ip);
	delwin(client);
	delwin(server);
    endwin();
	return 0;

}

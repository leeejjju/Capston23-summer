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
const int MODE_REGISTER = 2;

typedef enum{
	BLACK,
	BLUE,
	GREEN,
	RED,
	NOP
} color;

typedef struct{
	WINDOW* mainWin;
	WINDOW* scoreWin;
}windows;

int isError = 0;
int done = 0;
char errorMsg[BUFSIZE];
int port = 0;

char *ip = NULL, *port_str = NULL, *username = NULL, *password = NULL;


int send_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t sent ;
        sent = send(fd, p, len - acc, MSG_NOSIGNAL) ;
        if (sent == -1)
                return -1 ;
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
        sent = recv(fd, p, len - acc, 0) ;
        if (sent == -1)
                return -1 ;
        p += sent ;
        acc += sent ;
    }
    return acc ;
}

//TODO remove it
void getIpAndPort(char** args){

	ip = (char*)malloc(sizeof(char)*strlen(args[1])+1);
    strcpy(ip, args[1]);
    int index = 0;
    for(int i = 0; i < strlen(ip); i++) if(ip[i] == ':') index = i;
    ip[index] = 0;
    port = atoi(&ip[index+1]);

	return;
}

void setUserInfo(int argc, char** argv){

	int opt;

	while ((opt = getopt(argc, argv, "hi:p:u:w:")) != -1) {
		switch (opt) {
		case 'h':
			printf("Usage: client [options]\n\n"
				"Options:\n"
				"  -h, --help      Show this help message.\n"
				"  -i, --ip         The IP address of the server.\n"
				"  -p, --port       The port of the server.\n"
				"  -u, --username  The username.\n"
				"  -w, --password  The password.\n");
			exit(0);
		case 'i':
			ip = optarg;
			break;
		case 'p':
			port_str = optarg;
			break;
		case 'u':
			username = optarg;
			break;
		case 'w':
			password = optarg;
			break;
		default:
			fprintf(stderr, "Unknown option: %c\n", optopt);
			exit(1);
		}
	}

	if (ip == NULL || port_str == NULL || username == NULL || password == NULL) {
		fprintf(stderr, "Missing required option.\n");
		exit(1);
	}

	port = atoi(port_str);

	
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
			done = 1;
			break;
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
		
		//send header
		if((s = send_bytes(conn, (void*)&MODE_INPUT, sizeof(int))) == -1 || (s = send_bytes(conn, (void*)&len, sizeof(int))) == -1){
			strcpy(errorMsg, " [cannot send header]\n");
			isError = 1;
			return NULL;
		}
		//send text 
		if((s = send_bytes(conn, (void*)buf, len)) == -1){
			strcpy(errorMsg, " [cannot send text]\n");
			isError = 1;
			return NULL;
		}
		shutdown(conn, SHUT_WR);

		werase(client);
		wrefresh(client);
		close(conn);
	}

	return NULL;
}

//init outputbox window
void init_outputBox(windows* server){

	WINDOW* outBox = newwin(21, COLS, 3, 0);
	wbkgd(outBox, COLOR_PAIR(THEME));
	box(outBox, ACS_VLINE, ACS_HLINE);
	wrefresh(outBox);
	refresh();

	server->mainWin = newwin(19, (COLS-23), 4, 1);
	wbkgd(server->mainWin, COLOR_PAIR(THEME));
	idlok(server->mainWin, TRUE);
	scrollok(server->mainWin, TRUE);
	wprintw(server->mainWin, " enter \"quit\" to exit!\n");
	wrefresh(server->mainWin);
	refresh();

	WINDOW* outBox2 = newwin(19, 20, 4, (COLS-22));
	wbkgd(outBox2, COLOR_PAIR(THEME));
	box(outBox2, ACS_VLINE, ACS_HLINE);
	wrefresh(outBox2);
	refresh();

	server->scoreWin = newwin(17, 18, 5, (COLS-21));
	wbkgd(server->scoreWin, COLOR_PAIR(THEME));
	wprintw(server->scoreWin, " [score]\n");
	wprintw(server->scoreWin, " ip: %s\n port: %s\n", ip, port_str);
	wprintw(server->scoreWin, " user: %s\n pw: %s\n", username, password);
	wrefresh(server->scoreWin);
	refresh();

}

void* outputBox(void* ss){

	windows* wins = (windows*) ss;
	WINDOW* server = wins->mainWin;
	WINDOW* scoreWin = wins->scoreWin;


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
		if((s = send_bytes(conn, (void*)&MODE_OUTPUT, sizeof(MODE_OUTPUT))) == -1 || (s = send_bytes(conn, (void*)&lastTime, sizeof(lastTime))) == -1){
			strcpy(errorMsg, " [cannot send header]\n");
			isError = 1;
			return NULL;
		}
		shutdown(conn, SHUT_WR);

		while(1){
			if(done) break;
			int len = 0;
			//recv header
			//TODO recv_byte() does not work (whyy??)
			if((s = recv(conn, &len, sizeof(len), 0)) == -1 || (s = recv(conn, &lastTime, sizeof(lastTime), 0)) == -1){
			// if((s = recv_bytes(conn, (void*)&len, sizeof(len))) == -1 || (s = recv_bytes(conn, (void*)&lastTime, sizeof(lastTime))) == -1){
				strcpy(errorMsg, " [cannot recv header]\n");
				isError = 1;
				return NULL;
			}
			if(s == 0) break;
			//recv text
			if((s = recv_bytes(conn, (void*)buf, len)) == -1){
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

	//get username and password
	setUserInfo(argc, argv);

	//init windows
	init();
	init_main();
	windows server;
	init_outputBox(&server);
	WINDOW* client = newwin(1, COLS-2, 25, 1);
	init_inputBox(client);
	refresh();

	// check argvs and get information from it 
    // if(argc < 2){
	// 	mvwprintw(server.mainWin, 0, 0, " invalid command!\n\n usage: venezia [ip]:[port]\n");
	// 	wrefresh(server.mainWin);
	// 	goto EXIT;
    // }else getIpAndPort(argv);


	// int conn, s, len;
	// if(!(conn = makeConnection())){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot make connection]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// //send mode
	// if((s = send_bytes(conn, (void*)&MODE_REGISTER, sizeof(int))) == -1 || (s = send_bytes(conn, (void*)&len, sizeof(int))) == -1){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot send mode]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// //send userinfo
	// if((s = send_bytes(conn, (void*)username, sizeof(username))) == -1 || (s = send_bytes(conn, (void*)&len, sizeof(int))) == -1){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot send userinfo]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// if((s = send_bytes(conn, (void*)password, sizeof(password))) == -1 || (s = send_bytes(conn, (void*)&len, sizeof(int))) == -1){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot send userinfo]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// close(conn);


	//make threads
	pthread_t client_pid;
	if(pthread_create(&client_pid, NULL, (void*)inputBox, (void*)client)){
		perror("make new thread");
		mvwprintw(server.mainWin, 0, 0, " make thread failed\n");
		wrefresh(server.mainWin);
		goto EXIT;
	}
	pthread_t server_pid;
	if(pthread_create(&server_pid, NULL, (void*)outputBox, (void*)&server)){
		perror("make new thread");
		mvwprintw(server.mainWin, 0, 0, " make thread failed\n");
		wrefresh(server.mainWin);
		goto EXIT;
	}
	refresh();

	//exit
	pthread_join(client_pid, NULL);
	noecho();

	if(isError){
		mvwprintw(server.mainWin, 0, 0, "%s", errorMsg);
		wrefresh(server.mainWin);
		goto EXIT;
	}
	mvwprintw(client, 0, 0, " Good bye :)");
	
	EXIT:
	wprintw(server.mainWin, " press any key to exit... ");
	wrefresh(client);
	wrefresh(server.mainWin);

	getch();

	free(ip);
	delwin(client);
	delwin(server.mainWin);
	delwin(server.scoreWin);
    endwin();
	return 0;

}




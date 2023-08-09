#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>

#define THEME BLUE
#define BUFSIZE 128
#define LIMITLEN 10
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

int start = 0;
char sentence[BUFSIZE];



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

//get information from argc, argv and parse it 
void getArgs(int argc, char** argv){

    if(argc < 9) goto EXIT;
    
    struct option long_options[] = {
        {"ip", required_argument, 0, 'i'},
        {"port",  required_argument, 0,  'p'},
        {"username"  ,  required_argument, 0,  'u'},
        {"password",  required_argument, 0,  'w'}
    };
    
    int c ;
    int option_index = 0 ;

    do{
		c = getopt_long(argc, argv, "i:p:u:w:", long_options, &option_index);
		switch (c){
        case 'i': //ip
			printf("%c :%s\n", c, optarg);
            ip = strdup(optarg);
            break;
        case 'p': //port
			printf("%c :%s\n", c, optarg);
            port = atoi(optarg);
            break;
        case 'u': //username
			printf("%c :%s\n", c, optarg);
            username = strdup(optarg);
			break;
        case 'w': //password
			printf("%c :%s\n", c, optarg);
            password = strdup(optarg);
            break;
		case -1: break;
		default:
            goto EXIT;
        }
    }while(!(c == -1));

    return;

    EXIT:
    printf("Usage: client [options]\n"
		"Options:\n"
		"  -i, --ip         The IP address of the server.\n"
		"  -p, --port       The port of the server.\n"
		"  -u, --username  The username.\n"
		"  -w, --password  The password.\n");
	exit(EXIT_FAILURE);

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
	char buf[BUFSIZE] = {0};
	int s;
	int conn;

	while(start); //wait for start
	
	while(1){

		werase(client);
		wrefresh(client);

		wgetstr(client, buf); //get one sentence
		int len = strlen(buf);

		if(!strcmp(buf, "quit")){ 
			done = 1;
			break;
		}

		if(len > 128 || len == 0){
			werase(client);
			mvwprintw(client, 0, 0, " cannot send the msg:(");
			getch();
			continue;
		}

		//start connection
		if(!(conn = makeConnection())){
			isError = 1;
			return NULL;
		}
		
		//send mode
		if((s = send_bytes(conn, (void*)&MODE_INPUT, sizeof(int))) == -1){
			strcpy(errorMsg, " [cannot send mode]\n");
			isError = 1;
			return NULL;
		}
		//send userinfo
		if(!(((s = send_bytes(conn, (void*)username, LIMITLEN)) == -1)&&((s = send_bytes(conn, (void*)password, LIMITLEN)) == -1))){
			strcpy(errorMsg, " [cannot send userinfo]\n");
			isError = 1;
			return NULL;
		}
		//send text
		if((s = send_bytes(conn, (void*)buf, BUFSIZE)) == -1){
			strcpy(errorMsg, " [cannot send text]\n");
			isError = 1;
			return NULL;
		}
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
	wrefresh(server->scoreWin);
	refresh();

}

void* outputBox(void* ss){

	windows* wins = (windows*) ss;
	WINDOW* mainWin = wins->mainWin;
	WINDOW* scoreWin = wins->scoreWin;

	char buf[BUFSIZE];
	int conn;
	int s, i = 1;

	while(1){
		if(done) break;

		if(!(conn = makeConnection())){
			isError = 1;
			return NULL;
		}

		//send mode
		if((s = send_bytes(conn, (void*)&MODE_OUTPUT, sizeof(int))) == -1){
			strcpy(errorMsg, " [cannot send mode]\n");
			isError = 1;
			return NULL;
		}

		//recv sentence 
		if((s = recv_bytes(conn, (void*)buf, BUFSIZE)) != -1){
			strcpy(errorMsg, " [cannot recv sentence]\n");
			isError = 1;
			return NULL;
		}else start = 1;
		wprintw(mainWin, " [round %d] %s\n", i++, buf);
		wrefresh(mainWin);
		refresh();

		werase((scoreWin));
		wprintw(scoreWin, " [score]\n");
		//recv score
		while(!(s == 0)){
			int score;
			if(!(((s = recv_bytes(conn, (void*)buf, LIMITLEN)) == -1)&&((s = recv_bytes(conn, (void*)&score, sizeof(int))) == -1))){
				strcpy(errorMsg, " [cannot recv score info]\n");
				isError = 1;
				return NULL;
			}
			buf[LIMITLEN] = 0;
			wprintw(scoreWin, " %10s: %2d\n", buf, score);
			wrefresh(scoreWin);
			refresh();
		}
		close(conn);
	}
	return NULL;
}

int main(int argc, char *argv[]) {

	//get username and password
	getArgs(argc, argv);

	//init windows
	init();
	init_main();
	windows server;
	init_outputBox(&server);
	WINDOW* client = newwin(1, COLS-2, 25, 1);
	init_inputBox(client);
	refresh();

	//register(send username and password)
	// int conn, s, len;
	// if(!(conn = makeConnection())){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot make connection]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// //send mode
	// if((s = send_bytes(conn, (void*)&MODE_REGISTER, sizeof(int))) == -1){
	// 	mvwprintw(server.mainWin, 0, 0, " [cannot send mode]\n");
	// 	wrefresh(server.mainWin);
	// 	close(conn);
	// 	goto EXIT;
	// }
	// //send userinfo
	// if(!(((s = send_bytes(conn, (void*)username, sizeof(username))) == -1)&&((s = send_bytes(conn, (void*)password, sizeof(password))) == -1))){
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


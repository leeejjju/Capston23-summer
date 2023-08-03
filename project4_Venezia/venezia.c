#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h> 
#include <sys/socket.h> 
#include <sys/time.h>
#include <unistd.h>

#include <netinet/in.h> 
#include <arpa/inet.h>
#include <libgen.h>

#define THEME BLUE
#define LINESIZE 20 //TODO remove it
#define HEADERSIZE 4
#define BUFSIZE 512;


typedef enum{
	BLACK,
	BLUE,
	GREEN,
	RED,
	NOP
} color;


pthread_mutex_t readOK, writeOK;
int isError = 0;
char* ip;
int port = 0;

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

//get ip and port from args
void getIpAndPort(char* args){
	
	ip = (char*)malloc(sizeof(char)*strlen(argv[1])+1);
    strcpy(ip, argv[1]);

    int index = 0;
    for(int i = 0; i < strlen(ip); i++) if(ip[i] == ':') index = i;
    ip[index] = 0;
    port = atoi(&ip[index+1]);
	free(ip);

	return;

}

//make new sock and connection with ip,port and return the sock_fd(0 on fail)
int makeConnection(){

	int conn;
	struct sockaddr_in serv_addr; 
	conn = socket(AF_INET, SOCK_STREAM, 0) ;
	
	if (conn <= 0) {
		perror("socket failed : ") ;
		mvwprintw(server, 0, 0, " socker failed\n");
		wrefresh(server);
		return 0;
	} 

	memset(&serv_addr, '\0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		mvwprintw(server, 0, 0, " inet_pton failed\n");
		wrefresh(server);
		return 0;
	}
	if (connect(conn, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		mvwprintw(server, 0, 0, " connect failed\n");
		wrefresh(server);
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
    curs_set(2); //set corsor's visibility (0: invisible, 1: normal, 2: very visible)

    // noecho(); //turn off the echo (for control the screen)
    keypad(stdscr, TRUE); //enable to reading a function keys 
	cbreak(); //accept the special characters... 
	echo();

	pthread_mutex_init(&readOK, NULL);
	pthread_mutex_init(&writeOK, NULL);
	pthread_mutex_lock(&readOK);
    
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
	struct timeval now;
	gettimeofday(&now, NULL);
	suseconds_t lastTime = now.tv_usec;
	
	while(1){

		pthread_mutex_lock(&writeOK);
		wgetstr(client, buf);

		int conn;
		if(conn = makeConnection()){
			isError = 1;
			return;
		}
		int len = strlen(buf);


		//TODO modify with send_bytes... 
		//send header
		if(!(s = send(conn, &len, sizeof(int), 0)) || !(s = send(conn, &lastTime, sizeof(susecond_t), 0))){
			wprintw(server, " [cannot send header]\n");
			wrefresh(server);
			isError = 1;
			return;
		}

		//send payload 
		if(!(s = send(conn, buf, len, 0))){
			wprintw(server, " [cannot send payload]\n");
			wrefresh(server);
			isError = 1;
			return;
		}
		shutdown(sock_fd, SHUT_WR);
		pthread_mutex_unlock(&readOK);

		if(!strcmp(buf, "quit")) break;
		
		werase(client);
		wrefresh(client);
	}

	return NULL;
}

//init outputbox window
void init_outputBox(WINDOW* server){

	WINDOW* outBox = newwin(LINESIZE+1, COLS, 3, 0);
	wbkgd(outBox, COLOR_PAIR(THEME));
	box(outBox, ACS_VLINE, ACS_HLINE);
	wrefresh(outBox);
	refresh();

	wbkgd(server, COLOR_PAIR(THEME));
	idlok(server, TRUE);
	scrollok(server, TRUE);
	wprintw(server, "enter \"quit\" to exit!\n");
	wrefresh(server);
	refresh();
}


void* outputBox(void* s){

	WINDOW *server = (WINDOW*)s;
	char buf[BUFSIZE];
	int y, x;
	time_t timer; //TODO 
	
	while(1){
		
		pthread_mutex_lock(&readOK);
		
		int len;
		if(!recv(conn, &len, HEADERSIZE, 0) || !recv(conn, buf, len, 0)){
			wprintw(server, " [cannot recv message]\n");
			wrefresh(server);
			isError = 1;
			return NULL;
		}
		if(!strcmp(buf, "quit")) break;
		wprintw(server, "[me] %s\n", buf);
		pthread_mutex_unlock(&writeOK);
		
		wrefresh(server);
		refresh();
		
	}
	return NULL;
}


int main(int argc, char *argv[]) {

	//init windows
	init();
    bkgd(COLOR_PAIR(THEME));//select theme
	init_main();

	WINDOW* server = newwin(LINESIZE-1, COLS-2, 4, 1);
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
	pthread_join(server_pid, NULL);
	noecho();
	mvwprintw(client, 0, 0, " Good bye :)");
	
	EXIT:
	wprintw(server, " press any key to exit...");
	wrefresh(client);
	wrefresh(server);

	getch();

	pthread_mutex_destroy(&readOK);
	pthread_mutex_destroy(&writeOK);
	delwin(client);
	delwin(server);
    endwin();
	return 0;

}


// KEY사용한 입력받기
// key = wgetch(client);
		// getyx(client, y, x);
		// mvprintw(33, 25, "x: %-2d, y: %-2d | press ESC to quit...", x, y);
		// refresh();
		
		// if(x >= 58){
		// 	wmove(client, y+1, 2);
		// }
		// if(y > 13){
		// 	wclear(client);
		// 	wmove(client, 1, 2);
		// 	wprintw(client, "> type anthing!"); 
		// 	wmove(client, 2, 2);
		// 	box(client, ACS_VLINE, ACS_HLINE);
		// 	wrefresh(client);
		// }
		// if(key == ESCAPE){
		// 	return;
		// }else if(key == BACKSPACE){ 
		// 	wdelch(client);
		// }
		// getyx(client, y, x);
		// mvprintw(33, 25, "x: %-2d, y: %-2d | press ESC to quit...", x, y);
		// refresh();

//TODO 취소창 -> 테마선택 만들구파... 
// while(1)
    // {
    //     key = getch();

    //     if (key == ESCAPE || key == 'q')
    //     {
    //         WINDOW *check;
	// 		int key;

	// 		check = newwin(3, 40, 5, 10);
	// 		wmove(check, 1, 2);
	// 		wprintw(check, "Exit program (y/n) ? "); 
	// 		wbkgd(check, COLOR_PAIR(2));
	// 		box(check, ACS_VLINE, ACS_HLINE);
	// 		refresh();

	// 		key = wgetch(check);
	// 		delwin(check);

	// 		if (key == 'y')
	// 			goto EXIT;
	// 		else 
	// 			goto EXIT;
    //     }

    //     touchwin(stdscr);
    //     refresh();    
    // }
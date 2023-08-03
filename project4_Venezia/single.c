#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h> 
#include <time.h>

#define THEME BLUE
#define BUFSIZE 512
#define LINESIZE 20

typedef enum{
	BLACK,
	BLUE,
	GREEN,
	RED,
	NOP
} color;

char* sharedBuf;
pthread_mutex_t readOK, writeOK;
int done = 0;

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

	sharedBuf = (char*) malloc(sizeof(char)*BUFSIZE+1);
    
}

void init_main(){
	mvprintw(2, 1, "2023-summer capston1-study");
	mvprintw(27, COLS-10, "@leeejjju");
	move(30, 0);
	
	refresh();

}

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
	
	// TODO clear issue
	while(1){

		pthread_mutex_lock(&writeOK);
		wgetstr(client, sharedBuf);
		pthread_mutex_unlock(&readOK);

		if(!strcmp(sharedBuf, "quit")) break;
		
		werase(client);
		wrefresh(client);
	}

	return NULL;
}

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
	int y, x;

	
	while(1){
		
		pthread_mutex_lock(&readOK);
		if(!strcmp(sharedBuf, "quit")) break;
		wprintw(server, "[me] %s\n", sharedBuf);
		pthread_mutex_unlock(&writeOK);
		
		wrefresh(server);
		refresh();
		
	}
	
	return NULL;

}


int main(int argc, char *argv[]) {

	init();
    bkgd(COLOR_PAIR(THEME));//select theme
	init_main();

	WINDOW* server = newwin(LINESIZE-1, COLS-2, 4, 1);
	init_outputBox(server);
	WINDOW* client = newwin(1, COLS-2, 25, 1);
	init_inputBox(client);
	refresh();

	
	pthread_t client_pid;
	if(pthread_create(&client_pid, NULL, (void*)inputBox, (void*)client)){
		perror("make new thread");
	}
	pthread_t server_pid;
	if(pthread_create(&server_pid, NULL, (void*)outputBox, (void*)server)){
		perror("make new thread");
	}
	refresh();
	

	//exit
	pthread_join(client_pid, NULL);
	pthread_join(server_pid, NULL);
	noecho();
	mvwprintw(client, 0, 0, " Good bye :)");
	
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
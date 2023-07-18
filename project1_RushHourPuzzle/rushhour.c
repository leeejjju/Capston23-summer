#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define _DEBUG

//TODO error출력은 stdout 말고 따로 뺄 것

/* NOFIX --- */

typedef enum {
	start, //0
	left, //1
	right, //2
	up, //3
	down, //4
	quit, //5
	N_op  //6
} commands ;

typedef enum {
	vertical, //0
	horizontal  //1
} direction ;

typedef struct {
	int id ;
	int y1, y2 ;	// y1: the minimum of y, y2: the maximum of y
	int x1, x2 ;	// x1: the minimum of x, x2: the maximum of x
	int span ;		// the number of cells 
	direction dir ;	// the direction of the car
} car_t ;

char * op_str[N_op] = {
	"start", //0
	"left", //1
	"right", //2
	"up", //3
	"down", //4
	"quit" //5
};

int n_cars = 0 ;
car_t * cars = 0x0 ;
int cells[6][6] ; // cells[Y][X]
// A6 -> cells[5][0]
// F4 -> cells[3][5]
// F1 -> cells[0][5]

/* --- NOFIX */

commands get_op_code (char * s){
	// return the corresponding number for the command given as s.
	int i;
	for(i = 0; i < 6; i++){
		if(strcmp(op_str[i], s) == 0) break;
	}
	return (commands)(i);
}



int load_game (char * filename){
	//FIXME
	// load_game returns 0 for a success, or return 1 for a failure.
	// Use fopen, getline, strtok, atoi, strcmp
	// Note that the last character of a line obtained by getline may be '\n'.
	FILE* fp = NULL;
	fp = fopen(filename, "r");

	if(fp == NULL){
		printf("[error] cannot find the file %s\n", filename);
		return 1;
	}

	char* line = NULL;
	size_t len = 0;
	int i = 0;
	char cell[10], direction[10];
	int span; 

	//get the first line - num of cars
	getline(&line, &len, fp);
	line[strlen(line)-1] = 0;
	
	for(int k = 0; k < strlen(line); k++){
		if(!isdigit(line[k])){
			printf("[error] invalid type of n_cars\n");
			return 1;
		}
	}

	n_cars = atoi(line);
	
	if(!(n_cars >= 2 && n_cars <= 35)){
		printf("[error] invalid range of n_cars\n");
		return 1;
	}

	//allcate the space for cars 
	cars = (car_t*)malloc(sizeof(car_t)*n_cars);
	
	//get the information about each cars
	while(getline(&line, &len, fp) != -1){
		line[strlen(line)-1] = 0;
		#ifdef _DEBUG
		printf("[debug] car%d : %s\n", i+1, line);
		#endif 

		strcpy(cell, strtok(line, ":"));
		strcpy(direction, strtok(NULL, ":"));
		span = *(strtok(NULL, ":")) - '0';
		cars[i].id = i+1;

		//check the position of c1 (red car)
		if(i == 0 && !(!strcmp(direction, "horizontal") && cell[1] == '4')){
			printf("[error] invalid position of car1\n");
			return 1;
		}


		//check the pivot
		if(!(cell[0] >= 'A' && cell[0] <= 'F') || !(cell[1] >= '1' && cell[1] <= '6')){
			printf("[error] invalid cell value\n");
			return 1;
		}else{
			cars[i].x1 = (cell[0]- 'A');
			cars[i].x2 = (cell[0]- 'A');
			cars[i].y1 = (cell[1] - '0') -1;
			cars[i].y2 = (cell[1] - '0') -1;
		}

		//check the span
		if(span < 1 || span > 6){
			printf("[error] invalid span value\n");
			return 1;
		}else{
			cars[i].span = span;
		}

		//check the direction and fill the values
		if(!strcmp(direction, "vertical")){
			cars[i].dir = vertical;
			cars[i].y2 = cars[i].y2 - span + 1;
			if(cars[i].y2 > 5){
				printf("[error] invalid range of car%d\n", i);
				return 1;
			}
		}else if(!strcmp(direction, "horizontal")){
			cars[i].dir = horizontal;
			cars[i].x2 = cars[i].x2 + span - 1;
			if(cars[i].x2 < 0){
				printf("[error] invalid range of car%d\n", i);
				return 1;
			}
		}else {
			printf("[error] invalid direction\n");
			return 1;
		}

		
		#ifdef _DEBUG
		printf("[debug] token : %d:%d, %d:%d, %d, %d \n", cars[i].y1, cars[i].x1, cars[i].y2, cars[i].x2, cars[i].dir, cars[i].span);
		#endif 

		if(i++ > n_cars){
			printf("[error] wrong num of cars\n");
			return 1;
		}

	}
	return 0;
	
}


void display (){
	/* The beginning state of board1.txt must be shown as follows: 
 	 + + 2 + + +
 	 + + 2 + + +
	 1 1 2 + + +
	 3 3 3 + + 4
	 + + + + + 4
	 + + + + + 4
	*/
	for(int y = 5; y >= 0; y--){
		for(int x = 0; x < 6; x++){
			if(cells[y][x]) printf("%d ", cells[y][x]); 
			else printf("+ ");
		}
		printf("\n");
	}
	//FIXME
}

int update_cells (){
	memset(cells, 0, sizeof(int) * 36) ; // clear cells before the write.

	for(int i = 0; i < n_cars; i++){

		if(cars[i].dir == 0){
			for(int k = 0; k < cars[i].span; k++){
				if(!cells[cars[i].y1 - k][cars[i].x1]) cells[cars[i].y1 - k][cars[i].x1] = cars[i].id;
				else {
					printf("[error] for car%d, duplicated cars on %d:%d\n", cars[i].id, cars[i].y1 - k, cars[i].x1);
					return 1;
				}
			}
		}else if(cars[i].dir == 1){
			for(int k = 0; k < cars[i].span; k++){
				if(!cells[cars[i].y1][cars[i].x1 + k]) cells[cars[i].y1][cars[i].x1 + k] = cars[i].id;
				else {
					printf("[error] for car%d, duplicated cars on %d:%d\n", cars[i].id, cars[i].y1, cars[i].x1 + k);
					return 1;
				}
			}
		}
	}

	return 0;

	// return 0 for sucess
	// return 1 if the given car information (cars) has a problem
}


int move (int id, int op){
	//FIXME
	// move returns 1 when the given input is invalid.
	// or return 0 for a success.

	// Update cars[id].x1, cars[id].x2, cars[id].y1 and cars[id].y2
	//   according to the given command (op) if it is possible.

	// The condition that car_id can move left is when 
	//  (1) car_id is horizontally placed, and
	//  (2) the minimum x value of car_id is greather than 0, and
	//  (3) no car is placed at cells[cars[id].y1][cars[id].x1-1].
	// You can find the condition for moving right, up, down as
	//   a similar fashion.

	printf("[system] impossible!\n");

}

int main (){
	char buf[128] ;
	int op ;
	int id ;

	//! for test
	strcpy(buf, "board1.txt");
	#ifdef _DEBUG
	printf("[debug] start the game with file %s\n", buf);
	#endif
	load_game(buf) ;
	update_cells() ;
	display();
	//! for test

	while (1) {
		scanf("%s", buf) ;
		getchar();

		switch (op = get_op_code(buf)) {
			case start:

				
				scanf("%s", buf) ;
				getchar();
				#ifdef _DEBUG
				printf("[debug] start the game with file %s\n", buf);
				#endif
				load_game(buf) ;
				update_cells() ;
				display();
				break;

			case left:
			case right:
			case up:
			case down:
				scanf("%d", &id) ;
				getchar();
				#ifdef _DEBUG
				printf("[debug] move car%d to %s\n", id, op_str[op]);
				#endif
				move(id, op) ;
				update_cells() ;
				display() ;
				break;
			case quit:
				printf("[system] game terminated. Bye bye~\n");
				return 0;
			default:
				printf("[error] invalid command\n");
		}
	}
}

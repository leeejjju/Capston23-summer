#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// #include <windef.h>
//#define DEBUG

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


// return the corresponding number for the command given as s.
commands get_op_code (char * s){
	int i;
	for(i = 0; i < 6; i++){
		if(strcmp(op_str[i], s) == 0) break;
	}
	return (commands)(i);
}


// read input file and allocate cars 
// load_game returns 0 for a success, or return 1 for a failure.
int load_game (char * filename){
	// Use fopen, getline, strtok, atoi, strcmp

	FILE* fp = NULL;
	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "[error] cannot find the file %s\n", filename);
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
			fprintf(stderr, "[error] invalid type of n_cars : %c\n", line[k]);
			fclose(fp);
			return 1;
		}
	}

	n_cars = atoi(line);
	
	if((n_cars < 2) || (n_cars > 35)){
		fprintf(stderr, "[error] invalid range of n_cars\n");
		return 1;
	}

	//allcate the space for cars 
	cars = (car_t*)malloc(sizeof(car_t)*n_cars);
	
	//get the information about each cars
	while(getline(&line, &len, fp) != -1){
		line[strlen(line)-1] = 0;
		char tmp[20];
		#ifdef DEBUG
		printf("[debug] car%d : %s\n", i+1, line);
		#endif 

		strcpy(cell, strtok(line, ":"));
		strcpy(direction, strtok(NULL, ":"));
		strcpy(tmp, strtok(NULL, ":"));
		
		if(cell == NULL || direction == NULL || tmp == NULL){
			fprintf(stderr, "[error] invalid file format\n");
			return 1;
		}
		if(isdigit(span)){
			fprintf(stderr, "[error] invalid file format\n");
			return 1;
		}
		span = *(tmp) - '0';
		cars[i].id = i+1;

		//check the position of c1 (red car)
		if(i == 0 && !(!strcmp(direction, "horizontal") && cell[1] == '4')){
			fprintf(stderr, "[error] invalid position of car1\n");
			fclose(fp);
			return 1;
		}


		//check the pivot
		if((strlen(cell) != 2) || !(cell[0] >= 'A' && cell[0] <= 'F') || !(cell[1] >= '1' && cell[1] <= '6')){
			fprintf(stderr, "[error] invalid cell value\n");
			fclose(fp);
			return 1;
		}else{
			cars[i].x1 = (cell[0]- 'A');
			cars[i].x2 = (cell[0]- 'A');
			cars[i].y1 = (cell[1] - '0') -1;
			cars[i].y2 = (cell[1] - '0') -1;
		}

		//check the span
		if(span < 1 || span > 6){
			fprintf(stderr, "[error] invalid span value\n");
			fclose(fp);
			return 1;
		}else{
			cars[i].span = span;
		}

		//check the direction and fill the values
		if(!strcmp(direction, "vertical")){
			cars[i].dir = vertical;
			cars[i].y2 = cars[i].y2 - span + 1;
			if(cars[i].y2 > 5){
				fprintf(stderr, "[error] invalid range of car%d\n", i);
				fclose(fp);
				return 1;
			}
		}else if(!strcmp(direction, "horizontal")){
			cars[i].dir = horizontal;
			cars[i].x2 = cars[i].x2 + span - 1;
			if(cars[i].x2 < 0){
				fprintf(stderr, "[error] invalid range of car%d\n", i);
				fclose(fp);
				return 1;
			}
		}else {
			fprintf(stderr, "[error] invalid direction\n");
			fclose(fp);
			return 1;
		}

		
		#ifdef DEBUG
		printf("[debug] token : %d:%d, %d:%d, %d, %d \n", cars[i].y1, cars[i].x1, cars[i].y2, cars[i].x2, cars[i].dir, cars[i].span);
		#endif 

		if(++i > n_cars){
			fprintf(stderr, "[error] wrong num of cars\n");
			fclose(fp);
			return 1;
		}

	}
	
	fclose(fp);
	return 0;
	
}


//display the current cells array in format
void display (){
	/* The beginning state of board1.txt must be shown as follows: 
 	 + + 2 + + +
 	 + + 2 + + +
	 1 1 2 + + +
	 3 3 3 + + 4
	 + + + + + 4
	 + + + + + 4
	*/
	printf(" \\  ");
	for(int i = 0; i < 6; i++){
		printf("%c  ", 'A'+i);
	}
	printf("\n");

	for(int y = 5; y >= 0; y--){
		printf("%-2d: ", y+1);
		for(int x = 0; x < 6; x++){
			if(cells[y][x]){
				//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
				printf("%-2d ", cells[y][x]); 
			} 
			else printf("+  ");
		}
		printf("\n");
	}
	printf("\n");
}



// return 1 if input y,x is already used by other cars
int isOccupied(int index, int y, int x){

	for(int i = 0; i < n_cars; i++){
		if(cars[i].dir == vertical){
			for(int k = 0; k < cars[i].span; k++){
				if((cars[i].y1 - k == y) && (cars[i].x1 == x)) return 1;
			} 
		}else if(cars[i].dir == horizontal){
			for(int k = 0; k < cars[i].span; k++){
				if((cars[i].y1 == y) && (cars[i].x1 + k == x)) return 1;
			}
		}
	}

	return 0;

}


// Update cars[id].x1, cars[id].x2, cars[id].y1 and cars[id].y2 according to the given command (op) if it is possible.
// move returns 1 when the given input is invalid / return 0 for a success.
// car관련만 다루도록
// car(모델) -> cell(뷰)로 이어지는 흐름. view의 보장이 없으므로 확실한 분리 - 함수와 관련 없는 변수를 고려할 필요 없도록
int move (int id, int op){
	int index = id - 1;  //한 개념이 여러 용도로 쓰이면 안 됨

	switch (op){
	case left:
		//check the direction
		if(cars[index].dir == vertical){
			return 1;
		}

		//check the area 
		if((cars[index].x1-1 < 0) || isOccupied(index, cars[index].y1 , cars[index].x1-1)){
			return 1;
		}

		//move and update
		cars[index].x1--;
		cars[index].x2--;

		break;

	case right:
		//check the direction
		if(cars[index].dir == vertical){
			return 1;
		}

		//check the area 
		if((cars[index].x2+1 > 5) || isOccupied(index, cars[index].y2, cars[index].x2+1)){
			return 1;
		}

		//move and update
		cars[index].x1++;
		cars[index].x2++;
		break;

	case down:
		//check the direction
		if(cars[index].dir == horizontal){
			return 1;
		}

		//check the area 
		if((cars[index].y2-1 < 0) || isOccupied(index, cars[index].y2-1,cars[index].x2)){
			return 1;
		}

		//move and update
		cars[index].y1--;
		cars[index].y2--;
		break;

	case up:
		//check the direction
		if(cars[index].dir == horizontal){
			return 1;
		}

		//check the area 
		if((cars[index].y1+1 > 5) || isOccupied(index, cars[index].y1+1, cars[index].x1)){
			return 1;
		}

		//move and update
		cars[index].y1++;
		cars[index].y2++;
		break;	

	}

	return 0;

	// The condition that car_id can move left is when 
	//  (1) car_id is horizontally placed, and
	//  (2) the minimum x value of car_id is greather than 0, and
	//  (3) no car is placed at cells[cars[id].y1][cars[id].x1-1].
	// You can find the condition for moving right, up, down as a similar fashion.

}


// update cells array by information of cars array
// return 0 for sucess / return 1 if the given car information (cars) has a problem
int update_cells (){
	memset(cells, 0, sizeof(int) * 36) ; // clear cells before the write.

	for(int i = 0; i < n_cars; i++){

		if(cars[i].dir == vertical){
			for(int k = 0; k < cars[i].span; k++){
				if(!cells[cars[i].y1 - k][cars[i].x1]) cells[cars[i].y1 - k][cars[i].x1] = cars[i].id;
				else  return 1;
			} 

		}else if(cars[i].dir == horizontal){
			for(int k = 0; k < cars[i].span; k++){
				if(!cells[cars[i].y1][cars[i].x1 + k]) cells[cars[i].y1][cars[i].x1 + k] = cars[i].id;
				else return 1;
			}
		}
	}

	return 0;

}

// print success msg and terminate program
void success(){
	printf("\n*..☆'.｡.:*.SUCCESS･*..☆.'｡.:*.\n\n");
	display();
	printf(".｡.:.+*:ﾟ+｡.ﾟ･*..☆.｡.:*.☆.｡.:'\n\n");
	exit(EXIT_SUCCESS);
}

int main (){
	char buf[128] ;
	int op ;
	int id ;

	// //! for test
	// strcpy(buf, "error.txt");
	// #ifdef DEBUG
	// printf("[debug] start the game with file %s\n", buf);
	// #endif
	// load_game(buf) ;
	// update_cells() ;
	// //! for test

	while (1) {
		
		// get the first command
		scanf("%s", buf) ;
		getchar();

		switch (op = get_op_code(buf)) {

			case start:
				//get filename
				scanf("%s", buf) ;
				getchar();

				if(load_game(buf)){
					printf("[RushHourPuzzle] invalid file\n");
					continue;
				}

				if(update_cells()){
					printf("[RushHourPuzzle] invalid file\n");
					fprintf(stderr, "[error] duplicated cars on cells\n");
					continue;
				}
				display();
				break;

			case left:
			case right:
			case up:
			case down:
				if(cars == NULL){
					printf("[RushHourPuzzle] please start the game first! \n");
					getchar();
					continue;
				}

				//get the number i 
				scanf("%s", buf) ;
				getchar();

				if(!sscanf(buf, "%d", &id) || id < 1 || id > n_cars){
					printf("[RushHourPuzzle] invalid car number\n");
					continue;
				}

				if(move(id, op)){
					printf("[RushHourPuzzle] impossible! \n");
					continue;
				}
				if(update_cells()) {
					fprintf(stderr, "[error] duplicated cars on cells\n");
					continue;
				}
				display();
				break;
			case quit:
				printf("[RushHourPuzzle] game terminated\n");
				return 0;
			default:
				printf("[RushHourPuzzle] invalid command\n");
				// getchar();

		}

		if(cells[3][5] == 1){
		success();
	}

	}

}

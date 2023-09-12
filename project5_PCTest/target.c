#include <stdio.h>

//this is target program
int main(int argc, int** argv){

    printf("input the two number:");
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d + %d = %d\n", a, b, a+b);

    return 0;
}
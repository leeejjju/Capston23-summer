#include <stdio.h>
#include <unistd.h>

//this is target program
int main(int argc, int** argv){

    printf("input the two number:");
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d + %d = %d\n", a, b, a+b);
    if(a == 1) sleep(10);
    else sleep(a*0.1);

    return 0;
}
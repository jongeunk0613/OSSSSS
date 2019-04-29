#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int Fib(int n){
	if (n < 2)
		return n;
	else 
		return Fib(n-1) + Fib(n-2);
}

int main(int argc, char * argv[]){
	syntax errrr
	int n;
	int result = 0;
	scanf("%d",&n);
	result = Fib(n);
	printf("%d\n",result);

return 0;
}

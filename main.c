#include<stdio.h>

//extern double greater(double x, double y);
extern double lesser(double x, double y);
extern double fibonacci(double x);

int main() {
	//printf("%lf \n", greater(2,3));
	printf("%lf \n", lesser(2,3));
	
	//printf(fibonacci(5));

	return 0;
}

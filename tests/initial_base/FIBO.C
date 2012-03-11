/* Fibonacci */

#include <stdio.h>

int main()
{ int i, numtimes, number;
  unsigned value, fib();

    printf("Input number of iterations: ");
    scanf ("%d", &numtimes);
    for (i = 1; i <= numtimes; i++)
    {
	printf ("Input number: ");
	scanf ("%d", &number);
	value = fib(number);
	printf("fibonacci(%d) = %u\n", number, value);
    }
    exit(0);
}

unsigned fib(x) 		/* compute fibonacci number recursively */
int x;
{
    if (x > 2)
	return (fib(x - 1) + fib(x - 2));
    else
	return (1);
}

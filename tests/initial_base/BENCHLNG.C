/* benchlng - benchmark for  long  integers
 * Thomas Plum, Plum Hall Inc, 609-927-3770
 * If machine traps overflow, use an  unsigned  type
 * Let  T  be the execution time in milliseconds
 * Then  average time per operator  =  T/major  usec
 * (Because the inner loop has exactly 1000 operations)
 */
#define STOR_CL auto
#define TYPE long
#include <stdio.h>
main(int ac, char *av[])
{ TYPE a, b, c;
  long d, major;

	scanf ("%ld", &major);
        printf("executing %ld iterations\n", major);
	scanf ("%ld", &a);
	scanf ("%ld", &b);
        for (d = 1; d <= major; ++d)
	{
                /* inner loop executes 1000 selected operations */
                for (c = 1; c <= 40; ++c)
		{
                        a = a + b + c;
                        b = a >> 1;
                        a = b % 10;
                        a = b == c;
                        b = a | c;
                        a = !b;
                        b = a + c;
                        a = b > c;
		}
	}
        printf("a=%d\n", a);
}

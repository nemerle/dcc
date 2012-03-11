/* benchmul - benchmark for  int multiply
 * Thomas Plum, Plum Hall Inc, 609-927-3770
 * If machine traps overflow, use an  unsigned  type
 * Let  T  be the execution time in milliseconds
 * Then  average time per operator  =  T/major  usec
 * (Because the inner loop has exactly 1000 operations)
 */
#define STOR_CL auto
#define TYPE int
#include <stdio.h>
main(int ac, char *av[])
{ STOR_CL TYPE a, b, c;
  long d, major;

	printf ("enter number of iterations\n");
	scanf ("%ld", &major);
        printf("executing %ld iterations\n", major);
	scanf ("%d", &a);
	scanf ("%d", &b);
	for (d = 1; d <= major; ++d)
	{
                /* inner loop executes 1000 selected operations */
                for (c = 1; c <= 40; ++c)
		{
                        a = 3 *a*a*a*a*a*a*a*a * a*a*a*a*a*a*a*a * a*a*a*a*a*a*a*a * a; /* 25 * */
		}
	}
        printf("a=%d\n", a);
  }

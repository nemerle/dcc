/* benchfn - benchmark for function calls
 * Thomas Plum, Plum Hall Inc, 609-927-3770
 * Let  T  be the execution time in milliseconds
 * Then  average time per operator  =  T/major  usec
 * (Because the inner loop has exactly 1000 operations)
 */
#include <stdio.h>

f3() { ;}
f2() { f3();f3();f3();f3();f3();f3();f3();f3();f3();f3();} /* 10 */
f1() { f2();f2();f2();f2();f2();f2();f2();f2();f2();f2();} /* 10 */
f0() { f1();f1();f1();f1();f1();f1();f1();f1();f1();} /* 9 */

main(int ac, char *av[])
{ long d, major;

	printf ("enter number of iterations ");
	scanf ("%ld", &major);
        printf("executing %ld iterations\n", major);
        for (d = 1; d <= major; ++d)
                f0(); /* executes 1000 calls */
	printf ("finished\n");
}
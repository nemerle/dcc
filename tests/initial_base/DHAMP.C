/*      The dhampstone benchmark.  Written by Jack purdum. */
/*      version 1.0, August 1,1985                         */

#include "stdio.h"

#define BELL 7          /* ASCII BELL code */
#define FIB 24
#define TINY 100
#define MAXINT 179
#define LITTLE 1000
#define SMALL 9000
#define PRECISION .000001
#define FILENAME "zyxw.vut"
#define NUMTEST 6

#ifndef ERR
    #define ERR -1
#endif

struct
    {
    int cresult;
    int iresult;
    int cprsult;
    unsigned uresult;
    long lresult;
    double dresult;
    } results;

main()
{
char buf1[TINY], buf2[TINY];
int i = 0;
unsigned fib ();
long square, sq ();
double dmath, sroot (), dply ();

printf("Start...%c\n\n",BELL);
while (i < NUMTEST)
    {
    switch (i)
        {
    case (0):                                   /* Character test       */
        results.cresult = stest (buf1,buf2);
        printf ("\ncresult = %d\n",results.cresult);
        break;
    case (1):                                   /* Integer test         */
        results.iresult = intest ();
        printf ("\niresult = %d\n",results.iresult);
        break;
    case (2):                                   /* Unsigned test        */
        results.uresult = fib (FIB);
        printf ("\nuresult = %u\n",results.uresult);
        break;
    case (3):                                   /* Long test            */
        square = 0L;
        results.lresult = sq (square);
        square = sq (results.lresult);          /* Check the value      */
        printf ("\nlresult = %ld",results.lresult);
        printf ("\n square = %ld\n",square);
        break;
    case (4):                                   /* Double test          */
        results.dresult = sroot ((double) results.lresult);
        printf ("\ndresult = %f\n",results.dresult);
        dmath = dply (results.dresult);
        printf ("  dmath = %f\n",dmath);
        break;
    case (5):                                   /* Disk copy            */
        results.cprsult = mcopy ();
        printf ("\b   copy = %d",results.cprsult);
        break;
    default:
        break;
        }
    ++i;
    }                                           /* End while i          */
printf ("\n\n...End%c",BELL);
}

long sq (big)           /* Function to square a number by iteration */
long big;
{
int i;
static long j = 1L;

if (!big)
    for (i = 0; i < SMALL; ++i)
        {
        big += j;
        j += 2;
        }
else
    for (i = 0; i < SMALL; ++i)
        {
        j -= 2;
        big -= j;
        }
return (big);
}

double sroot (num)      /* Find square root of number */
double num;
{
double temp1, temp2, abs ();

temp2 = num / 2.0;
temp1 = num;
while (temp1 > PRECISION * temp2)
    {
    temp1 = (num / temp2) - temp2;
    temp1 = abs (temp1);
    temp2 = ((num / temp2) + temp2) / 2.0;
    }
return (temp2);
}

double abs (x)          /* Absolute value of a double */
double x;
{

return (x < 0 ? -x : x);
}

double dply (x)         /* Exercise some doubles */
double x;
{
int i = TINY;
double y;

while (i--)
    {
    y = x * x * x * x * x * x * x;
    y = y / x / x / x / x / x / x;

    y = y + x + x + x + x + x + x;
    y = y - x - x - x - x - x - x;
    }
return (y);
}

unsigned fib (x)        /* Common Fibonacci function */
int x;
{

if (x > 2)
    return (fib (x-1) + fib (x-2));
else
    return (1);
}

int stest (b1,b2)       /* String test using strcpy() and strcmp() */
char *b1, *b2;
{
int i,j;
void mstrcpy ();

for (i = 0, j = 0; i < SMALL; ++i)
    {
    mstrcpy (b1, "0123456789abcdef");
    mstrcpy (b2, "0123456789abcdee"); /* Note it's a different string. */
    j += mstrcmp (b1,b2);
    }
return (j);
}

int mstrcmp (c,d)       /* External string compare */
char *c, *d;
{

while (*c == *d)
    {
    if (!*c)
        return (0);
    ++c;
    ++d;
    }
return (*c - *d);
}

void mstrcpy (c,d)      /* External string copy */
char *c, *d;
{

while (*c++ = *d++)
    ;
}

int mcopy ()            /* Disk copy.  Test assumes file doesn't exist */
{
FILE *fp, *fopen ();
char buf[TINY];
int i, j;

mstrcpy (buf, "Disk I/O test");
if ((fp = fopen(FILENAME,"w")) == NULL)
    {
    printf ("Cannot open file");
    exit (ERR);
    }
i = 0;
while (++i < LITTLE)
    for (j = 0; buf[j]; ++j)
        putc (buf[j], fp);
fclose (fp);
return (i);
}

int intest ()           /* Square an integer by iteration */
{
int i, j, k, sum;

for (i = 0; i < LITTLE; ++i)
    {
    sum = 0;
    for (j = 0, k = 1; j < MAXINT; ++j)
        {
        sum += k;
        k += 2;
        }
    }
return (sum);
}

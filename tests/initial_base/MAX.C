main()
{ int a, b;

	printf ("Enter 2 numbers: ");
	scanf ("%d %d", &a, &b);
	if (a != b)
	   printf ("Maximum: %d\n", max (a,b));
}

max (int x, int y)
{
	if (x > y)
	   return (x);
	return (y);
}
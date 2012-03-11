#define TYPE int

main()
{ TYPE a, b;

	a = 255;
	b = 143;
	b = a + b;
	a = a - b;
	a = a * b;
	b = b / a;
	b = b % a;
	a = a << 5;
	b = b >> a;
	printf ("a = %d, b = %d\n", a, b);
}
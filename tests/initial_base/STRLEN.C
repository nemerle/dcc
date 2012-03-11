main()
{ char *s = "test";

	strlen(s);
}

strlen(char *s)
{ int n = 0;

	while (*s++)
	  n++;
	return (n);
}
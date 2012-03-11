#define n 5
#define m 4

static void multMatrix (int a[n][m], int b[m][n], int c[n][n])
{ int i,j,k;

	for (i=0; i<n; i++)
	    for (j=0; j<m; j++)
		for (k=0; k<m; k++)
		    c[i][j] = a[i][k] * b[k][j] + c[i][j];
}

main()
{ int a[n][m], b[n][m], c[n][m];

	multMatrix (a,b,c);
}

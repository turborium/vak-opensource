#include <stdio.h>

unsigned short a, b;

void p (unsigned short v)
{
	if (v)
		printf ("\t%d / %d = %d\r\n", a, b, v);
}

#define DO(n) b = n; p (a/n)

void main ()
{
	for (a=1; a<65536-211; a+=211) {
		DO (1);
		DO (2);
		DO (3);
		DO (4);
		DO (5);
		DO (6);
		DO (7);
		DO (8);
		DO (9);
		DO (10);
		DO (100);
		DO (1000);
		DO (10000);
	}
}

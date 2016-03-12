#include <stdlib.h>
#include <stdio.h>
#include "XY_433.h"

void foo(char* str) {
	str[3] = '*';
	printf("%s\n", str);
}

int main(int argv, char** argc) {
	int i;
	XY_Transmitter trans;
	XY_initialise(&trans, 24);
	XY_send(&trans, 5592412, 24);
}





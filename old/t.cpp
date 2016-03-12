#include <stdlib.h>
#include <stdio.h>

#include "RCSwitch.h"

RCSwitch mySwitch = RCSwitch();

int main(int argv, char** argc) {
	int i;
	mySwitch.enableTransmit(24);
	mySwitch.send(12345678, 24);
}


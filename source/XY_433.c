#include <wiringPi.h>
#include <stdint.h>
#include <stdio.h>

#define PULSE 350
#define REPEAT 10

typedef struct XY_Transmitter {
	int gpio_pin;
} XY_Transmitter;

int XY_initialise(XY_Transmitter* transmitter, int gpio_pin) {
	transmitter->gpio_pin = gpio_pin;
	wiringPiSetup();
	pinMode(transmitter->gpio_pin, OUTPUT);
	return 0;
}

int XY_send(XY_Transmitter* transmitter, long code, int bits) {
	int i, j;
	int r;
	long mask;
	for(r = 0; r < REPEAT; r++) {
		mask = 0x1;
		mask <<= bits - 1;
		for(i = 0; i < bits; i++) {
			if((code & mask)> 0) {
				digitalWrite(transmitter->gpio_pin, HIGH);
				delayMicroseconds(PULSE * 3);
				digitalWrite(transmitter->gpio_pin, LOW);
				delayMicroseconds(PULSE * 1);
			}
			else { 
				digitalWrite(transmitter->gpio_pin, HIGH);
				delayMicroseconds(PULSE * 1);
				digitalWrite(transmitter->gpio_pin, LOW);
				delayMicroseconds(PULSE * 3);
			}
			mask >>= 1;
		}
		digitalWrite(transmitter->gpio_pin, HIGH);
		delayMicroseconds(PULSE * 1);
		digitalWrite(transmitter->gpio_pin, LOW);
		delayMicroseconds(PULSE * 31);
	}
	return 0;
}


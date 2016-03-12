#ifndef XY_433
#define XY_433

typedef struct XY_Transmitter {
	int gpio_pin;
} XY_Transmitter;

int XY_initialise(XY_Transmitter* transmitter, int gpio_pin);
int XY_send(XY_Transmitter* transmitter, long code, int bits);
#endif

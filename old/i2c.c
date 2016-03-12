#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
//#include <linux/i2c.h>


int main() {
	int file;
	uint16_t v;
	file  = open("/dev/i2c-1", O_RDWR);
	if(file < 0) {
		printf("Unabel to open i2c\n");
		exit(1);
	}
	if(ioctl(file, I2C_SLAVE, 0x48) < 0) {
		printf("i2c probelm\n");
		exit(0);
	}
	v = i2c_smbus_read_word_data(file, 0x48);
	printf("%d\n", v);
	v >> 5;
	printf("%d\n", v);
	return 0;
}
	

#include <stdio.h>
#include <stdlib.h>
#define PATH_TO_EPD_DISPLAY "/dev/epd/LE/display_inverse"
#define PATH_TO_EPD_COMMAND "/dev/epd/command"

int main() {
	int i;
	unsigned char image[5808];
	for(i = 0; i < 5808; i ++) {
		image[i] = 0xFF;
	}
	for(i = 0; i < 330; i ++) {
		image[i] = 0x00;
	}
	FILE *epdDisplay, *epdCommand;
  epdDisplay = fopen(PATH_TO_EPD_DISPLAY, "wb");
	if(epdDisplay == NULL) {
    printf("unable to open screen for writing\n");
    exit(1);
  }
  fwrite(image, sizeof(unsigned char), 5808 , epdDisplay);
  epdCommand = fopen(PATH_TO_EPD_COMMAND, "w");
  if(epdCommand == NULL) {
    printf("unable to open screen for writing\n");
    exit(1);
  }
  fputc('U', epdCommand);
  fclose(epdDisplay);
  fclose(epdCommand);
  return 0;
	
}

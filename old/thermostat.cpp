//#include <stdint.h>
//#include <stdbool.h>
//#include <stdarg.h>
//#include <string.h>
#include <unistd.h>
//#include <err.h>
//#include <stdlib.h>
//#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "RCSwitch.h"

//#include "gpio.h"
//#include "spi.h"
//#include "epd.h"
//#include "epd_io.h"

#define PATH_TO_EPD_DISPLAY "/dev/epd/display"
#define PATH_TO_EPD_COMMAND "/dev/epd/command"

#define HEATINGON 5592407
#define HEATINGOFF 5592412
#define BITS 24

#define PATH_TO_THERM "/sys/bus/w1/devices/28-000006c855b0/w1_slave"
typedef struct Element {
  int height;
  int width;
  int pos_x;
  int pos_y;
  int pixels;
  unsigned char *buffer;
  Element* child;
  int numChildren;
} Element;

typedef struct TextElement {
  FT_Library _ft_library;
  FT_Face _ft_face;
  FT_GlyphSlot slot;
  int font_size;
  Element element;
  char* text;
  int textLength;
  int spacing;
  int border;
 } TextElement;

Element* create_Element(int height, int width, int pos_x, int pos_y) {
  Element element;
  element.height = height;
  element.pixels = height * width;
  element.buffer = (unsigned char*) calloc(element.pixels, sizeof(unsigned char));
  if(element.buffer == NULL) {
    return NULL;
  }
}

TextElement create_textElement(int height, int width, int pos_x, int pos_y, char* font, int font_size, char* text, int spacing) {
  int error;
  TextElement textElement;
  textElement.element = create_Element(height, width, pos_x, pos_y);
  textElement.font_size = font_size;
  textElement.spacing = spacing;
  textElement.text = text;
  textElement.textLength = strlen(textElement.text);
  error  = FT_Init_FreeType(&textElement._ft_library);
  if(error) {
    return NULL;
  }
  error = FT_New_Face(textElement._ft_library, font, 0, &textElement._ft_face);
  if(error == FT_Err_Unknown_File_Format) {
    return NULL;
  }
  error = FT_Set_Char_Size(textElement._ft_face, 0, font_size * 64, 150, 150);
   if(error) {
    return NULL;
  }
  return textElement;
}

int readBit(unsigned char* buffer, int pixel) {
  unsigned char readByte;
  int pixelByte;
  int shift;
  unsigned int value = 0x00;
  pixelByte = pixel / 8;
  shift = pixel % 8;
  readByte = 0x01 << shift;
  value = buffer[pixelByte] && readByte;
  return value;
}

void writeBit(unsigned char* buffer, int pixel, int bit) {
  unsigned char writeByte;
  int pixelByte;
  int shift;
  pixelByte = pixel / 8;
  shift = pixel % 8;
  if(bit > 0) {
    writeByte = 0x01 << shift;
    buffer[pixelByte] |= writeByte;
  }
  else {
    writeByte = 0xFE << shift;
    buffer[pixelByte] &= writeByte;
  }
}

int renderChar(unsigned char* buffer, FT_Bitmap* bitmap, int pen, int buffWidth) {
  int i, j, n = 0;
  uint8_t imageByte = 0x00;
  for(i = 0; i < bitmap->rows; i++){
    for(j = 0; j < bitmap->width; j++) {
      writeBit(buffer, pen, bitmap->buffer[n]);
      n++;
      pen++;
    }
    pen+= buffWidth - bitmap->width;
  }
}

int renderText(TextElement textElement) {
  FT_Face face;
  FT_GlyphSlot slot;
  int pen_x = 0, pen_y = 0;
  int pen;
  int n;
  int error;
  slot = face->glyph; 
  pen = textElement.border * textElement.element.width;
  pen +=  textElement.border;
  for(n = 0; n < textElement.textLength; n++) {
    FT_UInt glyph_index;
    glyph_index = FT_Get_Char_Index(face, textElement.text[n]);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    renderChar(textElement.element.buffer, &slot->bitmap, textElement.element.width, pen);
    pen +=slot->bitmap.width;
  }
  return 0;
}

int renderToElement(Element source, Element sink) {
  int i, j;
  int a = 0;
  int b = 0;
  a =  (source.pos_y * sink.width) + source.pos_x;
  for(i = 0; i < source.height; i++) {
    for(j = 0; source.width; j++) {
      writeBit(sink.buffer, a, readBit(source.buffer, b));
      b++;
    }
    a += sink.width - source.width;
  }
}

float tempTarget = 19;
boolean heatingOn = false;
RCSwitch mySwitch = RCSwitch();


void setup() {
  mySwitch.enableTransmit(10);
}

float getTemp() {
  float temp, tempRmd;
  int end = 0;
  char c;
  char tempStr[7];
  int tempInt = 0;
  int i = 0;
  FILE* thermistor;
  thermistor = fopen(PATH_TO_THERM, "r");
  if(thermistor == NULL) {
    printf("unable to open thermometer\n");
    exit(-1);
  }
  while(end == 0) {
    c = (char) fgetc(thermistor);
    if(c == 't') {
      fseek(thermistor, 1, SEEK_CUR);
      while(c != EOF && i < 6) {
        c = fgetc(thermistor);
        tempStr[i] = c;
        i++;
      }
      tempStr[6] = '\0';
      end = 1;
    }
  }
  fclose(thermistor);
  tempInt = atoi(tempStr);
  temp = tempInt / 1000;
  tempRmd = tempInt % 1000;
  tempRmd = tempRmd / 1000;
  temp = temp + tempRmd;
  return temp;
}

int display_onScreen(unsigned char* image) {
  FILE *epdDisplay, *epdCommand;
  epdDisplay = fopen(PATH_TO_EPD_DISPLAY, "wb");
  if(epdDisplay == NULL) {
    printf("unable to open screen for writing\n");
    exit(1);
  }
  fwrite(image, sizeof(unsigned char), 5808, epdDisplay);
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




int drawBitmap(unsigned char* pic, FT_Bitmap* bitmap, int offset) {
  int i, j, n = 0, k = 0;
  uint8_t imageByte = 0x00;
  n = offset;
  for(i = 0; i < bitmap->rows; i++){
    for(j = 0; j < bitmap->width; j++) {
      writeBit(pic, n, bitmap->buffer[k]);
      k++;
      n++;
    }
  n+= 264 - bitmap->width;
	} 
}


int drawBitmap_2(unsigned char* image, FT_Bitmap* bitmap) {
  int i, j, n, k, a = 0;
  uint8_t imageByte = 0x00;
  for(i = 0; i < bitmap->rows; i++){
    for(j = 0; j < bitmap->width; j++) {
		   for (a = 0; a < 8; a++) {
			    if(bitmap->buffer[k] == 0x00) {
				    imageByte = 0x01;
				    imageByte = imageByte << a;
				    image[n] += imageByte;
        }
        k++;
			}
      n++;
		}
    n+= 33;
	}
}

int displayFramebuffer(uint8_t* image) {
  FILE* file;
  int i, j, k = 0;
  unsigned char readByte = 0x00;
  unsigned char mask;
  char str[46640];
  file = fopen("./framebuffer.txt", "wr");
  for(i = 0; i < 5808; i++) {
   mask = 0x01;
   if(i % 33 == 0) {
    str[k] = '\n';
    k++;
    }
   for(j = 0; j < 8; j++) {
      readByte = image[i] ^ (mask << j);
      if(image[i] > 0x00) {
        str[k] = '#';
     }
      else {
        str[k] = '*';
      }
      k++;
    }
  }
  fwrite(str, sizeof(char), 46640, file);
}

int displayTemp(float temp) {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlot slot;
  int pen_x = 0, pen_y = 0;
  char tempStr[5];
  int n;
  int error;
  unsigned char* image;
  image = (unsigned char*)  calloc(5808, sizeof(unsigned char));
  sprintf(tempStr, "%.2f", temp);
  error = FT_Init_FreeType(&library);
  if(error) {
    return 1;
  }
  error = FT_New_Face(library, "/usr/share/fonts/truetype/freefont/FreeMono.ttf", 0, &face);
  if(error == FT_Err_Unknown_File_Format) {
    return 2;
  }
  else if (error) {
    return 3;
  }
  error = FT_Set_Char_Size(face, 0, 24*64, 264, 176);
  //error = FT_Set_Pixel_Sizes(face, 0, 16);
  slot = face->glyph; 
 
  for(n = 0; n < 5; n++) {
    FT_UInt glyph_index;
    glyph_index = FT_Get_Char_Index(face, tempStr[n]);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    //drawBitmap(&image, &slot->bitmap, pen_x + slot->bitmap_left, pen_y - slot->bitmap_top);
   drawBitmap(image, &slot->bitmap, pen_x);
   pen_x +=slot->bitmap.width;
  }
  displayFramebuffer(image);
  display_onScreen(image);
  return 0;
}

 
void turn_on_heating() {
  heatingOn = true;
  mySwitch.send(HEATINGON, BITS);
  printf("heating on\n");
}

void turn_off_heating() {
  heatingOn = false;
  printf("heating off\n");
  mySwitch.send(HEATINGOFF, BITS);
}

void mainLoop() {
  int r = 0;
  float temp = 0;
  while(1) {
    temp = getTemp();
    sleep(3);
    printf("temperature: %f\n", temp);
    r = displayTemp(temp);
    if(r != 0) {
      printf("display error: %d\n", r);
    }
    if((temp < tempTarget) && !heatingOn) {
      turn_on_heating();
    }
    else if((temp > tempTarget) && heatingOn) {
      turn_off_heating();
    }
  }
}

int main(int argv, char** argc) {
  setup();
  mainLoop();
  return 0;

}

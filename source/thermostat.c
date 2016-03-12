#include <stdint.h>
//#include <stdbool.h>
//#include <stdarg.h>
#include <string.h>
#include <unistd.h>
//#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <ft2build.h>
#include <wiringPi.h>
#include FT_FREETYPE_H

#include "XY_433.h"

//#include "gpio.h"
//#include "spi.h"
//#include "epd.h"
//#include "epd_io.h"

#define PATH_TO_EPD_DISPLAY "/dev/epd/display"
#define PATH_TO_EPD_COMMAND "/dev/epd/command"
#define GPIO_PIN 24
#define HEATINGON 5592407
#define HEATINGOFF 5592412
#define BITS 24

#define FONT "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf"

#define PAP_UPDATE 'U'
#define PAP_PARTIAL 'P'
#define PAP_2_7 1

#define PATH_TO_THERM "/sys/bus/w1/devices/28-000006c855b0/w1_slave"

#define TARGET_BUTTON 29
#define UP_BUTTON 28
#define DOWN_BUTTON 25

typedef struct Papirus_Display {
  unsigned char* image;
  unsigned char* old_image;
  int image_size;
  int screen_size;
  int height;
  int width;
  int pixels;
} Papirus_Display;

typedef struct Element {
  int height;
  int width;
  int pos_x;
  int pos_y;
  int pixels;
  unsigned char *buffer;
  int buffer_size;
  struct Element** child;
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
  int cursor;
  int lineHeight;
 } TextElement;

typedef struct Thermostat {
  float tempTarget;
  int heatingOn;
  XY_Transmitter transmitter;
  Papirus_Display display;
  Element element;
  TextElement textElement;
} Thermostat;

int create_Display(Papirus_Display* display, int screen_size) {
  display->screen_size = screen_size;
	switch(display->screen_size) {
    case PAP_2_7:
      display->height = 176;
      display->width = 264;
      break;
    default:
      return 1;
  }
  display->pixels = display->height * display->height;
  display->image_size = display->pixels / 8;
  return 0;
}

int set_Display(Papirus_Display* display, Element* element) {
  display->image = element->buffer;
  display_onScreen(display->image, display->image_size, PAP_UPDATE);
}

int initialise_buffer(unsigned char** buffer, int size) {
  if(*buffer != NULL) {
    free(*buffer);
  }
  *buffer = (unsigned char*) calloc(size, sizeof(unsigned char));
  if(*buffer == NULL) {
    return 1;
  }
  return 0;
}

int create_Element(Element* element, int width, int height, int pos_x, int pos_y) {
  element->height = height;
  element->width = width;
  element->pixels = height * width;
  element->buffer_size = element->pixels / 8;
  if(element->pixels % 8 > 0) {
    element->buffer_size++;
  }
  element->pos_x = pos_x;
  element->pos_y = pos_y;
  element->child = NULL;
  element->numChildren = 0;
  element->buffer = NULL;
  return initialise_buffer(&element->buffer, element->buffer_size);
}

int destroy_Element(Element* element) {
  free(element->buffer);
  return 0;
}

int append_child(Element* parent, Element* child) {
  parent->numChildren++;
  Element** temp = malloc(sizeof(Element *) * parent->numChildren);
  if(parent->child != NULL) {
    memcpy(temp, parent->child, parent->numChildren - 1);
    free(parent->child);
  }
  parent->child = temp;
  parent->child[parent->numChildren - 1] = child;
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
    writeByte = 0xFF - (0x01 << shift);
    buffer[pixelByte] &= writeByte;
  }
}

int renderToElement(Element* sink, Element* source) {
  int i, j;
  int a = 0;
  int b = 0;
  initialise_buffer(&sink->buffer, sink->buffer_size);
  a =  (source->pos_y * sink->width) + source->pos_x;
  for(i = 0; i < source->height; i++) {
    for(j = 0; j < source->width; j++) {
      writeBit(sink->buffer, a, readBit(source->buffer, b));
      b++;
      a++;
    }
    a += (sink->width - source->width);
  }
}

Element* renderElement(Element* element) {
  int i;
  for(i = 0; i < element->numChildren; i++) {
    renderElement(element->child[i]);
    renderToElement(element, renderElement(element->child[i]));
  }
  return element;
}

int create_textElement(TextElement* textElement, int width, int height, int pos_x, int pos_y, char* font, int font_size, int border) {
  int error;
  create_Element(&textElement->element, width, height, pos_x, pos_y);
  textElement->font_size = font_size;
  textElement->border = border;
  textElement->textLength = strlen(textElement->text);
  error  = FT_Init_FreeType(&textElement->_ft_library);
  if(error) {
    return 2;
  }
  error = FT_New_Face(textElement->_ft_library, font, 0, &textElement->_ft_face);
  if(error == FT_Err_Unknown_File_Format) {
    return 3;
  }
  error = FT_Set_Char_Size(textElement->_ft_face, 0, font_size * 64, 115, 150);
   if(error) {
    return 4;
  }
  textElement->lineHeight = textElement->_ft_face->size->metrics.height / 64;
  textElement->cursor = textElement->lineHeight * textElement->element.width;
  textElement->cursor += textElement->border * textElement->element.width;
  textElement->cursor +=  textElement->border;
  return 0;
}

int setText(TextElement* element, char* text){
  element->text = text;
  element->textLength = strlen(element->text);
  return renderText(element);
}

int destroy_textElement(TextElement* textElement) {
  return destroy_Element(&textElement->element);
}

int readBit(unsigned char* buffer, int pixel) {
  unsigned char readByte;
  int pixelByte;
  int shift;
  unsigned int value = 0x00;
  pixelByte = pixel / 8;
  shift = pixel % 8;
  readByte = 0x01 << shift;
  value = buffer[pixelByte] & readByte;
  value >>= shift;
  return value;
}

int renderChar(unsigned char* buffer, int bufferSize, FT_Bitmap* bitmap, int pen, int buffWidth) {
  int i, j, n = (bitmap->rows * bitmap->width) - bitmap->width;
  for(i = bitmap->rows; i > 0; i--){
    for(j = 0; j < bitmap->width; j++) {
      if(pen > bufferSize || pen < 0) {
        return 1;
      }
      if(bitmap->buffer[n] > 0) {
        writeBit(buffer, pen, 1);
      }
      n++;
      pen++;
    }
    n -= (bitmap->width * 2);
    pen-= (buffWidth + bitmap->width);
  }
}

int renderText(TextElement* textElement) {
  FT_GlyphSlot slot;
  int pen = textElement->cursor;
  int n;
  int error;
  int line_num = 1;
  int line_advance = textElement->cursor;
  initialise_buffer(&textElement->element.buffer, textElement->element.buffer_size);
  slot = textElement->_ft_face->glyph; 
  for(n = 0; n < textElement->textLength; n++) {
    FT_UInt glyph_index;
    glyph_index = FT_Get_Char_Index(textElement->_ft_face, textElement->text[n]);
    error = FT_Load_Glyph(textElement->_ft_face, glyph_index, FT_LOAD_DEFAULT);
    error = FT_Render_Glyph(textElement->_ft_face->glyph, FT_RENDER_MODE_NORMAL);
    line_advance += slot->advance.x >> 6;
    if(line_advance > (textElement->cursor + textElement->element.width - textElement->border)) {
      line_num++;
      pen = (textElement->lineHeight * textElement->element.width) * line_num;
      pen += textElement->border;
      line_advance = textElement->cursor;
      line_advance += slot->advance.x >> 6;
    }
    renderChar(textElement->element.buffer, textElement->element.pixels, &slot->bitmap, pen, textElement->element.width);
    pen += slot->advance.x >> 6;
  }
  return 0;
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
    return 0;
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

int display_onScreen(unsigned char* image, int image_size, int command) {
  FILE *epdDisplay, *epdCommand;
  epdDisplay = fopen(PATH_TO_EPD_DISPLAY, "wb");
  if(epdDisplay == NULL) {
    return 1;
  }
  if(fwrite(image, sizeof(unsigned char), image_size, epdDisplay) != image_size) {
    return 2;
   }
  epdCommand = fopen(PATH_TO_EPD_COMMAND, "w");
  if(epdCommand == NULL) {
    return 3;
  }
  fputc(command, epdCommand);
  fclose(epdDisplay);
  fclose(epdCommand);
  return 0;
}

int update_Display(Papirus_Display* display, Element* element) {
  renderElement(element);
  display->image = element->buffer;
  display_onScreen(display->image, display->image_size, PAP_PARTIAL);
}

int refresh_Display(Papirus_Display* display, Element* element) {
  renderElement(element);
  display->image = element->buffer;
  display_onScreen(display->image, display->image_size, PAP_UPDATE);
}


int displayTemp(float temp, Thermostat* thermo) {
  int pen_x = 0, pen_y = 0;
  char tempStr[4];
  int n;
  int error;
  unsigned char* image;
  sprintf(tempStr, "%.1f", temp);
  setText(&thermo->textElement, tempStr);
  update_Display(&thermo->display, &thermo->element);
}

 
void turn_on_heating(Thermostat* thermo) {
  thermo->heatingOn = 1;
  XY_send(&thermo->transmitter, HEATINGON, BITS);
  printf("heating on\n");
}

void turn_off_heating(Thermostat* thermo) {
  thermo->heatingOn = 0;
  printf("heating off\n");
  XY_send(&thermo->transmitter, HEATINGOFF, BITS);
}

int targetMode(Thermostat* thermo) {
  int wait = 0;
  char tempStr[4];
  sprintf(tempStr, "%.1f", thermo->tempTarget);
  setText(&thermo->textElement, tempStr);
  refresh_Display(&thermo->display, &thermo->element);
  while(wait < 8000) {
    delay(5);
    wait+= 5;
    if(digitalRead(UP_BUTTON) == 0) {
      printf("up\n");
      thermo->tempTarget += 0.5;
      sprintf(tempStr, "%.1f", thermo->tempTarget);
      setText(&thermo->textElement, tempStr);
      update_Display(&thermo->display, &thermo->element);
      wait = 0;
    }
    if(digitalRead(DOWN_BUTTON) == 0) {
      printf("down\n");
      thermo->tempTarget -= 0.5;
      sprintf(tempStr, "%.1f", thermo->tempTarget);
      setText(&thermo->textElement, tempStr);
      update_Display(&thermo->display, &thermo->element);
      wait = 0;
    }
  }
}

void mainLoop(Thermostat* thermo) {
  int r = 0;
  float temp_prev = 0;
  float temp = 0;
  int wait = 0;
  while(1) {
    if(digitalRead(TARGET_BUTTON) == 0) {
      printf("target\n");
      targetMode(thermo);
      displayTemp(temp, thermo);
    }
    delay(50);
    wait+= 50;
    if(wait == 6000 ) {
      wait = 0;
      temp = getTemp();
      if(temp == 0) {
        temp = temp_prev;
      }
      else {
        temp_prev = temp;
      }
      printf("temperature: %f\n", temp);
      r = displayTemp(temp, thermo);
      if(r != 0) {
        printf("display error: %d\n", r);
      }
      if((temp < thermo->tempTarget) && (thermo->heatingOn == 0)) {
        turn_on_heating(thermo);
      }
      else if((temp > thermo->tempTarget) && (thermo->heatingOn == 1)) {
        turn_off_heating(thermo);
      }
    }
  }
}

int gpio_setup() {
  pinMode(TARGET_BUTTON, INPUT);
  pinMode(UP_BUTTON, INPUT);
  pinMode(DOWN_BUTTON, INPUT);
}

int main(int argv, char** argc) {
  int error = 0;
  Thermostat thermo;
  thermo.tempTarget = 19;
  thermo.heatingOn = 0;
  error = create_Element(&thermo.element, 264, 176, 0, 0);
  error = create_textElement(&thermo.textElement, 150, 100, 60, 40, FONT, 30,  2);
  error = append_child(&thermo.element, &thermo.textElement.element);
  error =XY_initialise(&thermo.transmitter, GPIO_PIN);
  error = create_Display(&thermo.display, PAP_2_7);
  error = set_Display(&thermo.display, &thermo.element);
  if(error > 0) {
    printf("error\n");
    return 1;
  }
  gpio_setup();
  mainLoop(&thermo);
  return 0;

}

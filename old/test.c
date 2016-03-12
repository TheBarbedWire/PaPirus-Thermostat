#include <stdlib.h>
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct image {
  unsigned int pixel : 1;
 } image;

int main(int argv, char** argc) {
  FT_Library library;
  FT_Face face;
  FT_GlyphSlot slot;
  int pen_x, pen_y;
  char ch = 'A';
  char str[500];
  int a, n, i, j, k = 0;
  int error;
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
  //error = FT_Set_Char_Size(face, 0, 16*64, 264, 176);
  error = FT_Set_Pixel_Sizes(face, 0, 16);
  slot = face->glyph; 
  FT_UInt glyph_index;
  glyph_index = FT_Get_Char_Index(face, ch);
  error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  image *pic;
  pic = calloc((slot->bitmap.rows * slot->bitmap.width) / 8, 1);
  for(a = 0; a < (slot->bitmap.rows * slot->bitmap.width); a++) {
    pic[a].pixel = slot->bitmap.buffer[a];
  }
  for(i = 0; i < slot->bitmap.rows; i++) {
    for(j = 0; j < slot->bitmap.width; j++) {
      if(pic[k].pixel > 0) {
        str[n] = '#';
      }
      else {
        str[n] = '*';
      }
      k++;
      n++;
    }
    str[n] = '\n';
    n++;
  }
  printf("%s\n", str);
  return 0;

}

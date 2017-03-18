#ifndef __BLIT_H__
#define __BLIT_H__

#include <psp2/display.h>

#define COLOR_WHITE   0x00FFFFFF
#define COLOR_CYAN    0x00FFFF00
#define COLOR_MAGENTA 0x00FF00FF
#define COLOR_YELLOW  0x0000FFFF
#define TRANSPARENT   0xff000000

#define RGB(R,G,B)    (((B)<<16)|((G)<<8)|(R))
#define RGBT(R,G,B,T) (((T)<<24)|((B)<<16)|((G)<<8)|(R))

#define CENTER_TEXT(num) ((960 / 2) - (num * (16 / 2)))
#define ALIGN_LEFT(num)  ((960 / 2) - (num * 16))
#define ALIGN_RIGHT(num) ((960 / 2))

#define CENTER_SQ(num) ((960 / 2) - ( num / 2))

int blit_setup(void);
void blit_set_color(int fg_col,int bg_col);
int blit_string(int sx,int sy,const char *msg);
int blit_string_ctr(int sy,const char *msg);
int blit_stringf(int sx, int sy, const char *msg, ...);
int blit_set_frame_buf(const SceDisplayFrameBuf *param);
void blit_square(uint32_t bgcolor, int sx, int sy, int width, int height);

#endif
/*

  u8g_dogm128_api.h
  
  Simulation of the dogm128 api for the u8g library
  
*/

#ifndef _U8G_DOGM128_API_H
#define _U8G_DOGM128_API_H

#include "u8g.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef u8g_pgm_uint8_t dog_pgm_uint8_t;

#define DOG_PGM_P const dog_pgm_uint8_t *
#define DOG_PSTR_P const dog_pgm_uint8_t *
#define DOG_PSTR(s) U8G_PSTR(s)

#if defined(__AVR__)
#define DOG_PROGMEM PROGMEM
#else
#define DOG_PROGMEM
#endif

/* rom keyword on the chipkit environment */
#define DOG_ROM

#define DOG_ATTR_FN_NOINLINE U8G_NOINLINE

#define dog_pgm_read(adr) u8g_pgm_read(adr)


extern u8g_t u8g_dogm128_obj;

#define DOG_WIDTH (u8g_dogm128_obj.width)
#define DOG_HEIGHT (u8g_dogm128_obj.height)


/* this is a special u8g procedure to start up the dogm128 interface */
uint8_t u8g_InitDogm128API(u8g_dev_t *dev);


/* the following procedures are supported in the dogm128 API */
void dog_StartPage(void);
uint8_t dog_NextPage(void);
void dog_SetPixel(uint8_t x, uint8_t y);
void dog_ClrPixel(uint8_t x, uint8_t y);
void dog_SetHLine(uint8_t x1, uint8_t x2, uint8_t y);
void dog_ClrHLine(uint8_t x1, uint8_t x2, uint8_t y);
void dog_SetVLine(uint8_t x, uint8_t y1, uint8_t y2);
void dog_ClrVLine(uint8_t x, uint8_t y1, uint8_t y2);
void dog_SetBitmapP(uint8_t x, uint8_t y, DOG_PGM_P bitmap, uint8_t w, uint8_t h);
void dog_SetBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void dog_ClrBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
uint8_t dog_DrawStr(uint8_t x, uint8_t y, DOG_PGM_P font, const char *s);
uint8_t dog_DrawStrP(uint8_t x, uint8_t y, DOG_PGM_P font, const u8g_pgm_uint8_t  *s);
char *dog_itoa(unsigned long v);

/* mapping from dogm128 fonts to u8g fonts */
#define font_4x6 u8g_font_4x6r
#define font_5x7 u8g_font_5x7r
#define font_5x8 u8g_font_5x8r
#define font_6x9 u8g_font_6x9r
#define font_6x10 u8g_font_6x10r
#define font_6x12 u8g_font_6x12r
#define font_6x13 u8g_font_6x13r
#define font_7x13 u8g_font_7x13r
#define font_7x14 u8g_font_7x14r
#define font_8x13 u8g_font_8x13r
#define font_9x15 u8g_font_9x15r
#define font_9x18 u8g_font_9x18r

#ifdef __cplusplus
}
#endif

#endif /* _U8G_DOGM128_API_H */

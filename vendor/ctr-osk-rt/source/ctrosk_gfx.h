/* Internal framebuffer primitives for ctrosk. Not part of the public API.
 *
 * Everything here works in screen coordinates and hides the surface's layout,
 * which is described by signed strides so a rotated or bottom-up framebuffer
 * needs no special case. See CtrOskSurface.
 */

#ifndef CTROSK_GFX_H
#define CTROSK_GFX_H

#include <stdint.h>

#include "ctrosk.h"

/* Bind the surface to draw into. Call once per redraw, before anything else. */
void oskFbBind(const CtrOskSurface *s);

/* Dimensions of the bound surface. */
int oskWidth(void);
int oskHeight(void);

void oskPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void oskFill(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void oskClear(uint8_t r, uint8_t g, uint8_t b);
void oskHLine(int x, int y, int w, uint8_t r, uint8_t g, uint8_t b);
void oskVLine(int x, int y, int h, uint8_t r, uint8_t g, uint8_t b);

/* Filled rounded rectangle, 2px corner radius. */
void oskRRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);

/* A key: rounded face with a highlight along the top and a shadow underneath. */
void oskKey3D(int x, int y, int w, int h,
              uint8_t fr, uint8_t fg, uint8_t fb,   /* face      */
              uint8_t hr, uint8_t hg, uint8_t hb,   /* highlight */
              uint8_t sr, uint8_t sg, uint8_t sb);  /* shadow    */

/* 8x8 bitmap text. `scale` multiplies both axes. */
void oskChar(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b, int scale);
void oskText(int x, int y, const char *s, uint8_t r, uint8_t g, uint8_t b, int scale);
void oskTextCentered(int x, int y, int w, const char *s,
                     uint8_t r, uint8_t g, uint8_t b, int scale);

#endif /* CTROSK_GFX_H */

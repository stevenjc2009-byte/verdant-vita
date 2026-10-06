/* ctrosk - a pointer-driven on-screen keyboard for homebrew.
 *
 * The keyboard draws straight into a framebuffer you describe and hand it, and
 * takes pointer state you sample yourself, so it has no dependency on any one
 * console's SDK. You call ctrOskUpdate() once per frame to get key presses out
 * of it, and ctrOskDraw() to put it on screen. Nothing else is required.
 *
 *   CtrOsk osk;
 *   ctrOskInit(&osk);
 *   for (;;) {
 *     CtrOskSurface dst = { fb, 320, 240, 240 * 3, -3, 0 };
 *     CtrOskPointer p   = { touch_x, touch_y, touching };
 *     CtrOskEvent ev;
 *     if (ctrOskUpdate(&osk, &p, &ev)) handle_byte(ev.byte);
 *     ctrOskDraw(&osk, &dst);
 *   }
 *
 * Licensed under the GNU General Public License v3.0. See LICENSE.
 */

#ifndef CTROSK_H
#define CTROSK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTR_OSK_ROWS 5
#define CTR_OSK_COLS 10

/* Semantic key codes. The non-printable ones live in the C0 range so a single
   `char` in a layout table can hold either one of these or plain ASCII. */
enum {
  CTR_OSK_KEY_NONE  = 0,
  CTR_OSK_KEY_SHIFT = 1,   /* toggles lower <-> upper                        */
  CTR_OSK_KEY_SYM   = 2,   /* -> sym1 layer                                  */
  CTR_OSK_KEY_ABC   = 3,   /* -> lower layer                                 */
  CTR_OSK_KEY_SYM2  = 4,   /* -> sym2 layer                                  */
  CTR_OSK_KEY_CTRL  = 5,   /* arms the Ctrl modifier for the next key        */
  CTR_OSK_KEY_TAB   = '\t',
  CTR_OSK_KEY_ESC   = 27,
  CTR_OSK_KEY_ENTER = '\n',
  CTR_OSK_KEY_BKSP  = '\b',
  CTR_OSK_KEY_SPACE = ' '
};

typedef enum {
  CTR_OSK_LAYER_LOWER = 0,
  CTR_OSK_LAYER_UPPER,
  CTR_OSK_LAYER_SYM1,
  CTR_OSK_LAYER_SYM2,
  CTR_OSK_LAYER_COUNT
} CtrOskLayer;

/* ---------------------------------------------------------------- layout -- */

/* One layer is a flat 5x10 grid. Row 4 is the function row and is drawn
   specially: columns 3-6 are fused into one wide space bar, so whatever you
   put in those four cells is treated as a single key. */
typedef struct {
  char keys[CTR_OSK_ROWS][CTR_OSK_COLS];
} CtrOskLayerMap;

typedef struct {
  CtrOskLayerMap layer[CTR_OSK_LAYER_COUNT];
} CtrOskLayout;

extern const CtrOskLayout ctrOskLayoutQwerty;

/* Where the key grid sits and how big the keys are. The status bar fills
   everything above `y`, so `y` needs to leave room for it (the default 38px
   fits the title, subtitle and layer badge). */
typedef struct {
  int x, y;
  int key_w, key_h;
  int gap;
} CtrOskGeometry;

extern const CtrOskGeometry ctrOskGeometryDefault;

/* ----------------------------------------------------------------- theme -- */

typedef struct { uint8_t r, g, b; } CtrOskColor;

/* Where to draw. Pixels are addressed by signed strides rather than assumed
   linear: a rotated framebuffer (the 3DS's bottom screen runs bottom-to-top in
   columns) is described by a negative y_stride and a base pointing at pixel
   (0,0), not at the allocation. `rgb_first` picks the byte order within a
   pixel; a 32bpp surface's fourth byte is never written. */
typedef struct {
  uint8_t *base;
  int      w, h;
  int      x_stride;   /* signed bytes from a pixel to the one at x+1 */
  int      y_stride;   /* signed bytes from a pixel to the one at y+1 */
  int      rgb_first;  /* nonzero: bytes go R,G,B. zero: B,G,R        */
} CtrOskSurface;

/* Pointer state for this frame, in surface coordinates. `down` false means
   nothing is touching, which is what ends a press. */
typedef struct {
  int  x, y;
  bool down;
} CtrOskPointer;

typedef struct {
  CtrOskColor bg, status_bar, separator, badge;

  /* Key faces. `key_hi`/`key_sh` are the top highlight and bottom shadow of
     the 3D bevel; every other key type derives its own from the face. */
  CtrOskColor key, key_hi, key_sh;
  CtrOskColor num;                 /* number row, slightly darker            */
  CtrOskColor mod, mod_hi, mod_sh; /* TAB/ESC/SHF/CTL when inactive          */
  CtrOskColor shift_on;            /* SHF face while the upper layer is up   */
  CtrOskColor ctrl_on;             /* CTL face while Ctrl is armed           */
  CtrOskColor sym;                 /* the ?#1 / #+= / ABC toggles            */
  CtrOskColor enter, del, space;

  /* Text. */
  CtrOskColor text, text_num, text_dim;
  CtrOskColor label_tab, label_esc, label_abc, label_sym;
  CtrOskColor label_enter, label_del, label_ctrl_on;
} CtrOskTheme;

extern const CtrOskTheme ctrOskThemeDark;

/* ----------------------------------------------------------------- state -- */

typedef struct CtrOsk CtrOsk;

/* What one key press produced. */
typedef struct {
  int  byte;   /* the byte to feed your app, or -1 for modifier-only presses */
  int  key;    /* CTR_OSK_KEY_* or a printable ASCII code                    */
  bool ctrl;   /* the Ctrl modifier was applied to this press                */
} CtrOskEvent;

struct CtrOsk {
  /* --- configuration: set these after ctrOskInit(), before drawing ------- */
  const CtrOskLayout *layout;
  const CtrOskTheme  *theme;
  CtrOskGeometry      geom;

  const char *title;     /* drawn top-left, may be NULL                      */
  const char *subtitle;  /* drawn next to the title, dimmed, may be NULL     */

  /* Called instead of the built-in title/subtitle/badge if non-NULL, with the
     status bar already cleared. Use it to put your own state up there. */
  void (*draw_status)(CtrOsk *osk, void *user);
  void *user;

  /* Backspace is ambiguous: terminals want DEL (127), text fields want BS
     (8). True (the default) emits 127. `key` is CTR_OSK_KEY_BKSP either way. */
  bool backspace_as_del;

  /* Whether a press on the upper layer falls back to lower afterwards, the
     way a phone keyboard does. Default true. */
  bool shift_is_oneshot;

  /* --- internal --------------------------------------------------------- */
  int  layer;
  bool ctrl;
  int  press_r, press_c;
  bool touch_held;
  bool dirty;
};

/* Fills `osk` with defaults: QWERTY layout, dark theme, no title. Also turns
   off double buffering on the bottom screen: the keyboard draws in place and
   never swaps, so with two buffers half its redraws would land on the one
   that isn't being scanned out. */
void ctrOskInit(CtrOsk *osk);

/* Call once per frame, after hidScanInput(). Returns true and fills `ev` when
   a key press produced something to consume; `ev` may be NULL if you only
   care about the keyboard drawing itself. Modifier presses return false. */
bool ctrOskUpdate(CtrOsk *osk, const CtrOskPointer *p, CtrOskEvent *ev);

/* Redraws the bottom screen, but only if something actually changed. Cheap to
   call every frame. */
void ctrOskDraw(CtrOsk *osk, const CtrOskSurface *dst);

/* Force a full redraw on the next ctrOskDraw(). Needed after you scribble on
   the bottom screen yourself, or after changing the theme or layout. */
void ctrOskInvalidate(CtrOsk *osk);

/* Focus navigation, for a console with no pointer. The focused key is the one
   the touch path would highlight, so both input styles drive one grid. */
void ctrOskMoveFocus(CtrOsk *osk, int dr, int dc);
bool ctrOskActivate(CtrOsk *osk, CtrOskEvent *ev);

/* Switch layers and disarm Ctrl, as if the user had pressed the toggles. */
void ctrOskSetLayer(CtrOsk *osk, CtrOskLayer layer);
void ctrOskSetCtrl(CtrOsk *osk, bool armed);

static inline int  ctrOskLayerOf(const CtrOsk *osk)  { return osk->layer; }
static inline bool ctrOskCtrlArmed(const CtrOsk *osk) { return osk->ctrl; }

/* True if (px, py) from a touchPosition falls inside the key grid. Use it to
   decide whether a touch belongs to the keyboard or to your own bottom-screen
   UI drawn alongside it. */
bool ctrOskHitTest(const CtrOsk *osk, int px, int py);

/* "SHF", "TAB", "ENT" ... for the keys that are drawn as words, NULL for the
   ones drawn as their own character. */
const char *ctrOskKeyLabel(int key);

/* True for the layer-switching and Ctrl keys, which never emit a byte. */
bool ctrOskKeyIsModifier(int key);

#ifdef __cplusplus
}
#endif

#endif /* CTROSK_H */

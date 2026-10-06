#include "ctrosk.h"
#include "ctrosk_gfx.h"

#include <string.h>

/* --------------------------------------------------------------- defaults - */

#define K_SHF CTR_OSK_KEY_SHIFT
#define K_SYM CTR_OSK_KEY_SYM
#define K_ABC CTR_OSK_KEY_ABC
#define K_SY2 CTR_OSK_KEY_SYM2
#define K_CTL CTR_OSK_KEY_CTRL
#define K_TAB CTR_OSK_KEY_TAB
#define K_ESC CTR_OSK_KEY_ESC
#define K_ENT CTR_OSK_KEY_ENTER
#define K_BSP CTR_OSK_KEY_BKSP
#define K_SPC CTR_OSK_KEY_SPACE

/* The function row is identical on every layer apart from its layer toggle,
   which is why it is spelled out four times below rather than shared. */
const CtrOskLayout ctrOskLayoutQwerty = {{
  [CTR_OSK_LAYER_LOWER] = {{
    {'1','2','3','4','5','6','7','8','9','0'},
    {'q','w','e','r','t','y','u','i','o','p'},
    {'a','s','d','f','g','h','j','k','l','\''},
    {K_SHF,'z','x','c','v','b','n','m',',','.'},
    {K_TAB,K_ESC,K_SYM,K_SPC,K_SPC,K_SPC,K_SPC,K_CTL,K_ENT,K_BSP},
  }},
  [CTR_OSK_LAYER_UPPER] = {{
    {'!','@','#','$','%','^','&','*','(',')'},
    {'Q','W','E','R','T','Y','U','I','O','P'},
    {'A','S','D','F','G','H','J','K','L','"'},
    {K_SHF,'Z','X','C','V','B','N','M',';',':'},
    {K_TAB,K_ESC,K_SYM,K_SPC,K_SPC,K_SPC,K_SPC,K_CTL,K_ENT,K_BSP},
  }},
  [CTR_OSK_LAYER_SYM1] = {{
    {'1','2','3','4','5','6','7','8','9','0'},
    {'+','-','*','/','=','|','\\','`','~','_'},
    {'@','#','$','%','^','&','(',')','[',']'},
    {K_SY2,'{','}','<','>','?','!',':',';','"'},
    {K_TAB,K_ESC,K_ABC,K_SPC,K_SPC,K_SPC,K_SPC,K_CTL,K_ENT,K_BSP},
  }},
  [CTR_OSK_LAYER_SYM2] = {{
    {'`','~','|','\\','_','-','+','=','.',','},
    {'[',']','{','}','<','>','(',')','/','?'},
    {'!','@','#','$','%','^','&','*','\'','"'},
    {K_ABC,':',';','-','_','+','=','\\','|','~'},
    {K_TAB,K_ESC,K_ABC,K_SPC,K_SPC,K_SPC,K_SPC,K_CTL,K_ENT,K_BSP},
  }},
}};

/* 10 keys of 30px with 2px gaps is 318, so one pixel of slack on the left. */
const CtrOskGeometry ctrOskGeometryDefault = {
  .x = 1, .y = 38, .key_w = 30, .key_h = 36, .gap = 2
};

#define RGB(R, G, B) {(R), (G), (B)}

const CtrOskTheme ctrOskThemeDark = {
  .bg          = RGB(30, 30, 35),
  .status_bar  = RGB(22, 22, 28),
  .separator   = RGB(45, 45, 55),
  .badge       = RGB(45, 45, 52),

  .key         = RGB(58, 58, 65),
  .key_hi      = RGB(75, 75, 82),
  .key_sh      = RGB(40, 40, 48),
  .num         = RGB(50, 50, 58),
  .mod         = RGB(45, 45, 52),
  .mod_hi      = RGB(58, 58, 65),
  .mod_sh      = RGB(32, 32, 40),
  .shift_on    = RGB(40, 75, 50),
  .ctrl_on     = RGB(100, 35, 35),
  .sym         = RGB(80, 70, 35),
  .enter       = RGB(38, 80, 50),
  .del         = RGB(90, 40, 40),
  .space       = RGB(52, 52, 60),

  .text        = RGB(230, 230, 235),
  .text_num    = RGB(255, 200, 100),
  .text_dim    = RGB(130, 130, 140),
  .label_tab   = RGB(100, 200, 220),
  .label_esc   = RGB(200, 140, 220),
  .label_abc   = RGB(130, 180, 255),
  .label_sym   = RGB(255, 200, 100),
  .label_enter = RGB(200, 255, 210),
  .label_del   = RGB(255, 200, 200),
  .label_ctrl_on = RGB(255, 220, 220),
};

/* ---------------------------------------------------------------- helpers - */

/* Nudge a colour lighter or darker to make a bevel out of a flat face. */
static CtrOskColor shade(CtrOskColor c, int d) {
  int r = c.r + d, g = c.g + d, b = c.b + d;
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;
  CtrOskColor out = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
  return out;
}

#define SHADE_HI 14
#define SHADE_SH (-10)

bool ctrOskKeyIsModifier(int key) {
  unsigned char u = (unsigned char)key;
  return u == CTR_OSK_KEY_SHIFT || u == CTR_OSK_KEY_SYM ||
         u == CTR_OSK_KEY_ABC   || u == CTR_OSK_KEY_SYM2 ||
         u == CTR_OSK_KEY_CTRL;
}

const char *ctrOskKeyLabel(int key) {
  switch ((unsigned char)key) {
    case CTR_OSK_KEY_SHIFT: return "SHF";
    case CTR_OSK_KEY_SYM:   return "?#1";
    case CTR_OSK_KEY_ABC:   return "ABC";
    case CTR_OSK_KEY_SYM2:  return "#+=";
    case CTR_OSK_KEY_CTRL:  return "CTL";
    case CTR_OSK_KEY_TAB:   return "TAB";
    case CTR_OSK_KEY_ESC:   return "ESC";
    case CTR_OSK_KEY_ENTER: return "ENT";
    case CTR_OSK_KEY_BKSP:  return "DEL";
    default: return NULL;
  }
}

static const CtrOskLayerMap *active_map(const CtrOsk *osk) {
  int l = osk->layer;
  if (l < 0 || l >= CTR_OSK_LAYER_COUNT) l = CTR_OSK_LAYER_LOWER;
  return &osk->layout->layer[l];
}

static int key_x(const CtrOsk *osk, int c) {
  return osk->geom.x + c * (osk->geom.key_w + osk->geom.gap);
}
static int key_y(const CtrOsk *osk, int r) {
  return osk->geom.y + r * (osk->geom.key_h + osk->geom.gap);
}

/* Columns 3-6 of the function row are one wide space bar, so every touch in
   that span reports as the leftmost of the four. */
static int normalize_col(int r, int c) {
  if (r == CTR_OSK_ROWS - 1 && c >= 3 && c <= 6) return 3;
  return c;
}

/* ---------------------------------------------------------------- drawing - */

static void draw_key_body(const CtrOsk *osk, int x, int y, int w, int h,
                          CtrOskColor face, CtrOskColor hi, CtrOskColor sh,
                          bool pressed) {
  (void)osk;
  /* A pressed key is drawn one pixel in and two down, with the highlight and
     shadow swapped, so it reads as pushed into the screen. */
  if (pressed)
    oskKey3D(x + 1, y + 2, w - 2, h - 2,
             sh.r, sh.g, sh.b, face.r, face.g, face.b, hi.r, hi.g, hi.b);
  else
    oskKey3D(x, y, w, h,
             face.r, face.g, face.b, hi.r, hi.g, hi.b, sh.r, sh.g, sh.b);
}

static bool key_pressed(const CtrOsk *osk, int r, int c) {
  return osk->press_r == r && osk->press_c == c;
}

/* One key at grid slot (r, c). `label` wins over `ch` when both are given;
   `span` is how many columns wide the key is. */
static void draw_key(const CtrOsk *osk, int r, int c, char ch,
                     const char *label, int span,
                     CtrOskColor face, CtrOskColor hi, CtrOskColor sh,
                     CtrOskColor text, int scale) {
  int x = key_x(osk, c);
  int y = key_y(osk, r);
  int w = osk->geom.key_w * span + osk->geom.gap * (span - 1);
  bool pressed = key_pressed(osk, r, c);
  int text_y = y + (pressed ? 2 : 0);

  draw_key_body(osk, x, y, w, osk->geom.key_h, face, hi, sh, pressed);

  char buf[2];
  if (!label && ch >= 32 && ch <= 126) {
    buf[0] = ch; buf[1] = 0;
    label = buf;
  }
  if (label) {
    /* The caller's scale is a preference, not a promise. Nothing clips a
       label to its key, so on a panel short enough that a doubled 8x8 glyph
       does not fit between the key's edges it spills onto the rows above and
       below instead - a 480x272 screen split with a terminal leaves 13px
       keys, which is where this first showed up. Four pixels of the key's
       height are kept for the bezel and the gap. */
    int scale_max = (osk->geom.key_h - 4) / 8;
    if (scale_max < 1) scale_max = 1;
    if (scale > scale_max) scale = scale_max;

    int th = 8 * scale;
    oskTextCentered(x, text_y + (osk->geom.key_h - th) / 2 - 1, w,
                    label, text.r, text.g, text.b, scale);
  }
}

static void draw_status_bar(CtrOsk *osk) {
  const CtrOskTheme *t = osk->theme;

  oskFill(0, 0, oskWidth(), osk->geom.y,
          t->status_bar.r, t->status_bar.g, t->status_bar.b);

  if (osk->draw_status) {
    osk->draw_status(osk, osk->user);
    return;
  }

  if (osk->title)
    oskText(4, 4, osk->title, t->text.r, t->text.g, t->text.b, 1);
  if (osk->subtitle) {
    int tx = osk->title ? 4 + (int)strlen(osk->title) * 8 + 8 : 4;
    oskText(tx, 4, osk->subtitle, t->text_dim.r, t->text_dim.g, t->text_dim.b, 1);
  }

  /* Layer badge, top right. */
  const char *tag;
  CtrOskColor tc;
  switch (osk->layer) {
    case CTR_OSK_LAYER_UPPER: tag = "ABC"; tc = t->label_abc; break;
    case CTR_OSK_LAYER_SYM1:  tag = "?#1"; tc = t->label_sym; break;
    case CTR_OSK_LAYER_SYM2:  tag = "#+="; tc = t->label_sym; break;
    default:                  tag = "abc"; tc = t->label_abc; break;
  }
  oskRRect(270, 2, 46, 13, t->badge.r, t->badge.g, t->badge.b);
  oskTextCentered(270, 4, 46, tag, tc.r, tc.g, tc.b, 1);

  if (osk->ctrl) {
    oskRRect(4, 18, 50, 14, t->ctrl_on.r, t->ctrl_on.g, t->ctrl_on.b);
    oskText(8, 21, "CTRL", 255, 255, 255, 1);
    oskText(60, 21, "tap a key...",
            t->text_dim.r, t->text_dim.g, t->text_dim.b, 1);
  }
}

/* The function row is laid out by hand rather than from the keymap: its keys
   carry word labels and their own accent colours, and the space bar spans
   four columns. The keymap still drives what they *do*. */
static void draw_function_row(CtrOsk *osk) {
  const CtrOskTheme *t = osk->theme;
  const int r = CTR_OSK_ROWS - 1;

  draw_key(osk, r, 0, 0, "TAB", 1, t->mod, t->mod_hi, t->mod_sh, t->label_tab, 1);
  draw_key(osk, r, 1, 0, "ESC", 1, t->mod, t->mod_hi, t->mod_sh, t->label_esc, 1);

  /* Symbol toggle: shows where it will take you, not where you are. */
  {
    bool on_sym = osk->layer >= CTR_OSK_LAYER_SYM1;
    draw_key(osk, r, 2, 0, on_sym ? "ABC" : "?#1", 1,
             t->sym, shade(t->sym, SHADE_HI), shade(t->sym, SHADE_SH),
             on_sym ? t->label_abc : t->label_sym, 1);
  }

  /* Space bar: four columns fused, marked with a line rather than a word. */
  {
    int sx = key_x(osk, 3), sy = key_y(osk, r);
    int sw = osk->geom.key_w * 4 + osk->geom.gap * 3;
    bool pressed = key_pressed(osk, r, 3);
    draw_key_body(osk, sx, sy, sw, osk->geom.key_h,
                  t->space, shade(t->space, SHADE_HI), shade(t->space, SHADE_SH),
                  pressed);
    oskFill(sx + sw / 2 - 15, sy + osk->geom.key_h / 2 + (pressed ? 2 : 0),
            30, 1, t->text_dim.r, t->text_dim.g, t->text_dim.b);
  }

  {
    CtrOskColor face = osk->ctrl ? t->ctrl_on : t->mod;
    CtrOskColor text = osk->ctrl ? t->label_ctrl_on : t->text;
    draw_key(osk, r, 7, 0, "CTL", 1,
             face, shade(face, SHADE_HI), shade(face, SHADE_SH), text, 1);
  }
  draw_key(osk, r, 8, 0, "ENT", 1, t->enter, shade(t->enter, SHADE_HI),
           shade(t->enter, SHADE_SH), t->label_enter, 1);
  draw_key(osk, r, 9, 0, "DEL", 1, t->del, shade(t->del, SHADE_HI),
           shade(t->del, SHADE_SH), t->label_del, 1);
}

void ctrOskDraw(CtrOsk *osk, const CtrOskSurface *dst) {
  if (!osk->dirty) return;
  osk->dirty = false;

  const CtrOskTheme *t = osk->theme;
  const CtrOskLayerMap *map = active_map(osk);

  oskFbBind(dst);
  oskClear(t->bg.r, t->bg.g, t->bg.b);
  draw_status_bar(osk);
  oskHLine(0, osk->geom.y + 2, oskWidth(),
           t->separator.r, t->separator.g, t->separator.b);

  for (int r = 0; r < CTR_OSK_ROWS - 1; r++) {
    for (int c = 0; c < CTR_OSK_COLS; c++) {
      char key = map->keys[r][c];

      if (ctrOskKeyIsModifier(key)) {
        CtrOskColor face = t->mod, hi = t->mod_hi, sh = t->mod_sh, text = t->text;
        unsigned char u = (unsigned char)key;

        if (u == CTR_OSK_KEY_SHIFT && osk->layer == CTR_OSK_LAYER_UPPER) {
          face = t->shift_on;
          hi = shade(face, SHADE_HI); sh = shade(face, SHADE_SH);
        } else if (u == CTR_OSK_KEY_SYM || u == CTR_OSK_KEY_SYM2) {
          face = t->sym;
          hi = shade(face, SHADE_HI); sh = shade(face, SHADE_SH);
          text = t->label_sym;
        } else if (u == CTR_OSK_KEY_ABC) {
          text = t->label_abc;
        } else if (u == CTR_OSK_KEY_CTRL && osk->ctrl) {
          face = t->ctrl_on;
          hi = shade(face, SHADE_HI); sh = shade(face, SHADE_SH);
          text = t->label_ctrl_on;
        }
        draw_key(osk, r, c, 0, ctrOskKeyLabel(key), 1, face, hi, sh, text, 1);
      } else {
        bool num_row = (r == 0);
        draw_key(osk, r, c, key, NULL, 1,
                 num_row ? t->num : t->key, t->key_hi, t->key_sh,
                 num_row ? t->text_num : t->text, 2);
      }
    }
  }

  draw_function_row(osk);
}

void ctrOskInvalidate(CtrOsk *osk) { osk->dirty = true; }

/* ------------------------------------------------------------------ input - */

bool ctrOskHitTest(const CtrOsk *osk, int px, int py) {
  /* The gap around a key is part of its touch target, so the row of keys
     extends half a gap past the grid on each side. Without the left-hand
     slack the default geometry, which starts at x=1, would ignore taps on the
     very first column of pixels. The trailing gap is already inside x1. */
  int x0 = osk->geom.x - osk->geom.gap;
  int x1 = osk->geom.x + CTR_OSK_COLS * (osk->geom.key_w + osk->geom.gap);
  int y0 = osk->geom.y;
  int y1 = y0 + CTR_OSK_ROWS * (osk->geom.key_h + osk->geom.gap);
  return px >= x0 && px < x1 && py >= y0 && py < y1;
}

/* Turn a press into an event, updating layer/ctrl state. Returns true when it
   produced a byte. */
static bool apply_key(CtrOsk *osk, char key, CtrOskEvent *ev) {
  unsigned char u = (unsigned char)key;

  switch (u) {
    case CTR_OSK_KEY_SHIFT:
      osk->layer = (osk->layer == CTR_OSK_LAYER_UPPER) ? CTR_OSK_LAYER_LOWER
                                                       : CTR_OSK_LAYER_UPPER;
      return false;
    case CTR_OSK_KEY_SYM:  osk->layer = CTR_OSK_LAYER_SYM1;  return false;
    case CTR_OSK_KEY_SYM2: osk->layer = CTR_OSK_LAYER_SYM2;  return false;
    case CTR_OSK_KEY_ABC:  osk->layer = CTR_OSK_LAYER_LOWER; return false;
    case CTR_OSK_KEY_CTRL: osk->ctrl = !osk->ctrl;           return false;
  }

  ev->key  = u;
  ev->ctrl = false;

  switch (u) {
    case CTR_OSK_KEY_TAB:   ev->byte = '\t'; return true;
    case CTR_OSK_KEY_ESC:   ev->byte = 27;   return true;
    case CTR_OSK_KEY_ENTER: ev->byte = '\n'; return true;
    case CTR_OSK_KEY_SPACE: ev->byte = ' ';  return true;
    case CTR_OSK_KEY_BKSP:
      ev->byte = osk->backspace_as_del ? 127 : '\b';
      return true;
  }

  if (osk->ctrl) {
    /* Ctrl folds a letter down into its C0 control code; anything else passes
       through unchanged so Ctrl never silently eats a key. */
    if (key >= 'a' && key <= 'z')      ev->byte = key - 'a' + 1;
    else if (key >= 'A' && key <= 'Z') ev->byte = key - 'A' + 1;
    else                               ev->byte = (unsigned char)key;
    ev->ctrl = true;
    osk->ctrl = false;
  } else {
    ev->byte = (unsigned char)key;
    if (osk->shift_is_oneshot && osk->layer == CTR_OSK_LAYER_UPPER)
      osk->layer = CTR_OSK_LAYER_LOWER;
  }
  return true;
}

bool ctrOskUpdate(CtrOsk *osk, const CtrOskPointer *p, CtrOskEvent *ev) {
  CtrOskEvent scratch;
  if (!ev) ev = &scratch;
  ev->byte = -1;
  ev->key  = CTR_OSK_KEY_NONE;
  ev->ctrl = false;

  if (!p || !p->down) {
    /* Release: drop the pressed-key highlight if there was one. */
    if (osk->touch_held || osk->press_r != -1) {
      osk->touch_held = false;
      osk->press_r = -1;
      osk->press_c = -1;
      osk->dirty = true;
    }
    return false;
  }

  /* Only the first frame of a touch counts, so holding a key does not repeat.
     The 3DS touchscreen is resistive and single-point; there is no second
     finger to track. */
  if (osk->touch_held) return false;
  osk->touch_held = true;

  if (!ctrOskHitTest(osk, p->x, p->y)) return false;

  int r = (p->y - osk->geom.y) / (osk->geom.key_h + osk->geom.gap);
  int c = (p->x - osk->geom.x) / (osk->geom.key_w + osk->geom.gap);
  if (r < 0) r = 0; else if (r >= CTR_OSK_ROWS) r = CTR_OSK_ROWS - 1;
  if (c < 0) c = 0; else if (c >= CTR_OSK_COLS) c = CTR_OSK_COLS - 1;
  c = normalize_col(r, c);

  osk->press_r = r;
  osk->press_c = c;
  osk->dirty = true;

  return apply_key(osk, active_map(osk)->keys[r][c], ev);
}

/* --------------------------------------------------------------- focus nav */

/* press_r/press_c double as the focus position: it is already the cell the
   drawing code highlights, so a console with no pointer walks the same grid
   the touch path lands on. */
void ctrOskMoveFocus(CtrOsk *osk, int dr, int dc) {
  int r = osk->press_r < 0 ? 0 : osk->press_r;
  int c = osk->press_c < 0 ? 0 : osk->press_c;

  r += dr;
  if (r < 0) r = CTR_OSK_ROWS - 1; else if (r >= CTR_OSK_ROWS) r = 0;
  c += dc;
  if (c < 0) c = CTR_OSK_COLS - 1; else if (c >= CTR_OSK_COLS) c = 0;

  osk->press_r = r;
  osk->press_c = normalize_col(r, c);
  osk->dirty = true;
}

/* Commits whatever the focus is on, exactly as a touch on that key would. */
bool ctrOskActivate(CtrOsk *osk, CtrOskEvent *ev) {
  CtrOskEvent scratch;
  if (!ev) ev = &scratch;
  ev->byte = -1;
  ev->key  = CTR_OSK_KEY_NONE;
  ev->ctrl = false;
  if (osk->press_r < 0 || osk->press_c < 0) return false;
  osk->dirty = true;
  return apply_key(osk, active_map(osk)->keys[osk->press_r][osk->press_c], ev);
}

/* ---------------------------------------------------------------- lifecycle */

void ctrOskInit(CtrOsk *osk) {
  memset(osk, 0, sizeof(*osk));
  osk->layout   = &ctrOskLayoutQwerty;
  osk->theme    = &ctrOskThemeDark;
  osk->geom     = ctrOskGeometryDefault;
  osk->layer    = CTR_OSK_LAYER_LOWER;
  osk->press_r  = -1;
  osk->press_c  = -1;
  osk->dirty    = true;
  osk->backspace_as_del  = true;
  osk->shift_is_oneshot  = true;
}

void ctrOskSetLayer(CtrOsk *osk, CtrOskLayer layer) {
  if ((unsigned)layer >= CTR_OSK_LAYER_COUNT) return;
  osk->layer = layer;
  osk->ctrl = false;
  osk->dirty = true;
}

void ctrOskSetCtrl(CtrOsk *osk, bool armed) {
  osk->ctrl = armed;
  osk->dirty = true;
}

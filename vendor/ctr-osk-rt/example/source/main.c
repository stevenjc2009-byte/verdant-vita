/* ctrosk example: a scratchpad.
 *
 * The keyboard runs on the bottom screen, and whatever you type lands in a
 * buffer shown on the top screen along with the raw event the keyboard
 * reported, so you can see exactly what each key produces.
 *
 * Build with `make` from this directory, after building the library above it.
 */

#include <3ds.h>
#include <ctrosk.h>

#include <stdio.h>
#include <string.h>

#define BUF_MAX 512

static char  text[BUF_MAX + 1];
static int   text_len = 0;

/* The last event, for the readout at the bottom of the top screen. */
static char  last_desc[64] = "(nothing yet)";

static void describe(const CtrOskEvent *ev) {
  const char *name = ctrOskKeyLabel(ev->key);
  if (name)
    snprintf(last_desc, sizeof(last_desc), "%s -> byte %d", name, ev->byte);
  else if (ev->ctrl)
    snprintf(last_desc, sizeof(last_desc), "Ctrl-%c -> byte %d",
             ev->key, ev->byte);
  else
    snprintf(last_desc, sizeof(last_desc), "'%c' -> byte %d",
             (char)ev->key, ev->byte);
}

static void apply(const CtrOskEvent *ev) {
  switch (ev->key) {
    case CTR_OSK_KEY_BKSP:
      if (text_len > 0) text[--text_len] = '\0';
      return;
    case CTR_OSK_KEY_ESC:
      /* Ctrl is armed separately; Esc here just clears the scratchpad. */
      text_len = 0;
      text[0] = '\0';
      return;
    case CTR_OSK_KEY_ENTER:
      if (text_len < BUF_MAX) text[text_len++] = '\n';
      break;
    case CTR_OSK_KEY_TAB:
      if (text_len < BUF_MAX) text[text_len++] = ' ';
      break;
    default:
      /* Control codes from Ctrl-<letter> are not printable, so show them as
         ^A..^Z rather than writing a stray byte into the buffer. */
      if (ev->ctrl) {
        if (text_len < BUF_MAX - 1 && ev->byte >= 1 && ev->byte <= 26) {
          text[text_len++] = '^';
          text[text_len++] = 'A' + ev->byte - 1;
        }
        break;
      }
      if (ev->byte >= 32 && ev->byte <= 126 && text_len < BUF_MAX)
        text[text_len++] = (char)ev->byte;
      break;
  }
  text[text_len] = '\0';
}

static void draw_top(const CtrOsk *osk) {
  static const char *layer_name[] = {"lower", "UPPER", "?#1", "#+="};

  printf("\x1b[2J\x1b[H");   /* clear, home */
  printf("\x1b[47;30m ctrosk example - scratchpad          \x1b[0m\n\n");
  printf("%s\n", text);

  /* Status block, pinned to the bottom of the 30-row console. */
  printf("\x1b[26;1H");
  printf("\x1b[36m%d/%d chars\x1b[0m\n", text_len, BUF_MAX);
  printf("last: \x1b[33m%s\x1b[0m\n", last_desc);
  printf("layer: %s   ctrl: %s\n",
         layer_name[ctrOskLayerOf(osk)],
         ctrOskCtrlArmed(osk) ? "\x1b[31marmed\x1b[0m" : "off");
  printf("ESC clears  -  START exits");
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;

  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  /* The keyboard redraws in place and never swaps, so the bottom screen must
     not be double buffered. */
  gfxSetDoubleBuffering(GFX_BOTTOM, false);

  CtrOsk osk;
  ctrOskInit(&osk);
  osk.title    = "ctrosk";
  osk.subtitle = "scratchpad";
  /* This is a text field, not a terminal, so backspace should mean BS. */
  osk.backspace_as_del = false;

  text[0] = '\0';
  bool top_dirty = true;

  while (aptMainLoop()) {
    hidScanInput();
    if (hidKeysDown() & KEY_START) break;

    /* The keyboard takes pointer state rather than reading the hardware
       itself, so it can be driven by anything that produces a position. */
    touchPosition tp;
    hidTouchRead(&tp);
    CtrOskPointer ptr = { tp.px, tp.py, (hidKeysHeld() & KEY_TOUCH) != 0 };

    CtrOskEvent ev;
    if (ctrOskUpdate(&osk, &ptr, &ev)) {
      describe(&ev);
      apply(&ev);
      top_dirty = true;
    } else if (osk.dirty) {
      /* A modifier press changes the layer/ctrl readout even though it
         produced no byte. */
      top_dirty = true;
    }

    /* The 3DS bottom screen is 320x240 BGR8 and rotated a quarter turn: a
       column is contiguous and y runs backwards, which the strides describe.
       base points at pixel (0,0), not at the start of the allocation. */
    u8 *fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    CtrOskSurface dst = { fb + (240 - 1) * 3, 320, 240, 240 * 3, -3, 0 };
    ctrOskDraw(&osk, &dst);

    if (top_dirty) {
      draw_top(&osk);
      top_dirty = false;
    }

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }

  gfxExit();
  return 0;
}

#ifndef PLAT_CFG_H
#define PLAT_CFG_H

/* Sony PlayStation Vita (VitaSDK). See source/core/plat.h. */

#define PLAT_NAME        "Vita"
#define PLAT_SLUG        "vita"

/* ux0: is the active writable storage mount configured on the user's system. */
#define PLAT_SD          "ux0:/"

/* SceUID is an int32, the mutex gets stored as one */
#define PLAT_MUTEX_SIZE  8

/* One 960x544 screen, split: terminal across the top 352 rows, panel across
   the full width of the bottom 192 */
#define PLAT_TERM_W      960
#define PLAT_TERM_H      352
#define PLAT_PANEL_W     960
#define PLAT_PANEL_H     192
#define VITA_SCREEN_W    960
#define VITA_SCREEN_H    544

/* The display scans A8B8G8R8, so red is the low byte and alpha is the high
   one. same order as the psp, which was a nice surprise for once */
#define PLAT_PIXEL_RGB

/* 960x352 at the 8x8 font, so the whole grid fits with no panning at all */
#define TERM_COLS        120
#define TERM_ROWS        44
#define TERM_SCROLLBACK  200

/* An app's share of the 512MB is ~256MB once the system takes its cut, and
   plat.c asks newlib for 192MB*/
#define PLAT_RAM_MAX_MB  128
#define PLAT_RAM_MIN_MB  8

/* With 128MB the guest has zero use for one */
#define PLAT_WANT_SWAP   false

/* This console has a network stack; see virtio_net.h. */
#define PLAT_HAS_NET

/* Verdant requires the patched service image, shipped in its install ZIP. */

#endif /* PLAT_CFG_H */

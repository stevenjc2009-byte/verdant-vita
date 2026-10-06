#ifndef PLAT_CFG_H
#define PLAT_CFG_H
#define PLAT_NAME "Host test"
#define PLAT_SLUG "3ds"
#define PLAT_SD "./"
#define PLAT_MUTEX_SIZE 64
#ifdef VERDANT_TEST_VITA
#define PLAT_TERM_W 960
#define PLAT_TERM_H 352
#define PLAT_PANEL_W 960
#define PLAT_PANEL_H 192
#define TERM_COLS 120
#define TERM_ROWS 44
#else
#define PLAT_TERM_W 400
#define PLAT_TERM_H 240
#define PLAT_PANEL_W 320
#define PLAT_PANEL_H 240
#define TERM_COLS 80
#define TERM_ROWS 30
#endif
#define TERM_SCROLLBACK 200
#define PLAT_RAM_MAX_MB 64
#define PLAT_RAM_MIN_MB 8
#define PLAT_WANT_SWAP true
#define PLAT_HAS_NET
#endif

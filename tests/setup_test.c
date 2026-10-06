/* Run the actual first-launch installer against temporary host directories. */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#define PLAT_SLUG "vita"
#define PLAT_SD "./"
static int plat_update_title(const char *s){return 0;}
static uint64_t plat_us(void){static uint64_t tick;return tick+=1000001;}
static void PresentTopScreen(uint64_t *tick){*tick=plat_us();}
static void term_printf(const char *format,...){va_list args;va_start(args,format);vprintf(format,args);va_end(args);}
#include "updater.h"
#include "setup.h"
int main(void){uint64_t tick=0;return vs_install(&tick)?0:1;}

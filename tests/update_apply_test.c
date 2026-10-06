/* Standalone transaction test target; Python fixtures supply the temporary SD. */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "plat_cfg.h"
#ifdef VERDANT_APPLY_VITA
#undef PLAT_SLUG
#define PLAT_SLUG "vita"
#endif
static int plat_update_title(const char *package) { return 0; }
#include "updater.h"
int main(void) { char message[256]={0};bool ok=vu_apply(message,sizeof(message));puts(message);return ok?0:1; }

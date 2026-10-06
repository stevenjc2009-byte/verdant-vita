#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define SCE_SYSMODULE_NET 1
typedef struct {void *memory;int size;int flags;} SceNetInitParam;
static int modules,inits,exits;static bool fail;
static int sceSysmoduleLoadModule(int m){modules++;return 0;}
static int sceNetInit(SceNetInitParam *p){inits++;return 0;}
static int sceNetCtlInit(void){return fail?-1:0;}
static int sceNetCtlTerm(void){return 0;}
static int sceNetTerm(void){exits++;return 0;}
#include "../source/platform/vita/network_start.h"
int main(void){assert(plat_net_init());assert(net_up && modules==1 && inits==1);assert(plat_net_init() && inits==1);plat_net_exit();assert(!net_up && !net_pool && exits==1);fail=true;assert(!plat_net_init() && !net_up && !net_pool && exits==2);puts("Native network startup succeeds without any Wi-Fi association wait, reuses the stack and cleans failed initialization.");}

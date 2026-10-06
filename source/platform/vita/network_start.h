/* Vita socket-stack startup is independent of access point association. */
static bool net_up,net_ours;
static void *net_pool;
#define VITA_NET_POOL (1024 * 1024)
static void vita_net_teardown(void) {
 if(net_ours){sceNetTerm();net_ours=false;}free(net_pool);net_pool=NULL;
}
bool plat_net_init(void) {
 if(net_up)return true;
 if(sceSysmoduleLoadModule(SCE_SYSMODULE_NET)<0)return false;
 net_pool=malloc(VITA_NET_POOL);if(!net_pool)return false;
 SceNetInitParam param={net_pool,VITA_NET_POOL,0};net_ours=sceNetInit(&param)>=0;
 if(sceNetCtlInit()<0){vita_net_teardown();return false;}
 /* No association/DHCP wait: offline desktop/boot work immediately. */
 net_up=true;return true;
}
void plat_net_exit(void) {
 if(!net_up)return;
 sceNetCtlTerm();vita_net_teardown();net_up=false;
}

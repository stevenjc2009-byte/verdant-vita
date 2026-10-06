/* Asynchronous native storage queue on core 1; UI never performs a scan/read. */
#include "native_files.h"
#define VF_QUEUE 8
typedef struct {unsigned id;char op[16],path[768];char *data;} VFRequest;
static VFRequest vf_queue[VF_QUEUE];
static unsigned vf_read,vf_write;
static plat_mutex_t vf_lock;
static SceUID vf_thread=-1,vf_ready=-1;
static atomic_bool vf_stop;
static void vf_delete_lock(void){SceUID id;memcpy(&id,&vf_lock,sizeof(id));sceKernelDeleteMutex(id);}
static int vf_entry(SceSize size,void *arg) {
 static char response[VF_TEXT+1];
 while(sceKernelWaitSema(vf_ready,1,NULL)>=0) {
  plat_mutex_lock(&vf_lock);
  if(vf_read==vf_write){bool stop=atomic_load(&vf_stop);plat_mutex_unlock(&vf_lock);if(stop)break;continue;}
  VFRequest request=vf_queue[vf_read%VF_QUEUE];vf_read++;plat_mutex_unlock(&vf_lock);
  bool ok=vf_execute(request.op,request.path,request.data,response,VF_TEXT+1);free(request.data);
  char part[256],dest[256];snprintf(part,sizeof(part),PLAT_SD "verdant/bridge/%u.native-part",request.id);snprintf(dest,sizeof(dest),PLAT_SD "verdant/bridge/%u.res",request.id);
  FILE *f=fopen(part,"wb");if(f){fprintf(f,"%s\n%s",ok?"OK":"ERROR",response);bool saved=!ferror(f);if(fclose(f))saved=false;if(saved)rename(part,dest);else remove(part);}
 }
 return 0;
}
static bool vf_start(void) {
 if(vf_thread>=0)return true;
 plat_mutex_init(&vf_lock);vf_ready=sceKernelCreateSema("verdant_files_ready",0,0,VF_QUEUE+1,NULL);if(vf_ready<0){vf_delete_lock();return false;}
 atomic_store(&vf_stop,false);vf_read=vf_write=0;
 vf_thread=sceKernelCreateThread("verdant_files",vf_entry,0x10000110,65536,0,SCE_KERNEL_CPU_MASK_USER_1,NULL);
 if(vf_thread<0 || sceKernelStartThread(vf_thread,0,NULL)<0){if(vf_thread>=0)sceKernelDeleteThread(vf_thread);vf_thread=-1;sceKernelDeleteSema(vf_ready);vf_ready=-1;vf_delete_lock();return false;}
 return true;
}
bool plat_files_request(unsigned id,const char *op,const char *a,const char *b) {
 if(!vf_supported(op,a,b) || !vf_start())return false;
 char *copy=b?strdup(b):NULL;if(b && !copy)return false;
 plat_mutex_lock(&vf_lock);
 if(vf_write-vf_read>=VF_QUEUE){plat_mutex_unlock(&vf_lock);free(copy);return false;}
 VFRequest *r=&vf_queue[vf_write++%VF_QUEUE];memset(r,0,sizeof(*r));r->id=id;r->data=copy;snprintf(r->op,sizeof(r->op),"%s",op);snprintf(r->path,sizeof(r->path),"%s",a);
 plat_mutex_unlock(&vf_lock);sceKernelSignalSema(vf_ready,1);return true;
}
static void vf_cleanup(void) {
 if(vf_thread<0)return;
 atomic_store(&vf_stop,true);sceKernelSignalSema(vf_ready,1);sceKernelWaitThreadEnd(vf_thread,NULL,NULL);sceKernelDeleteThread(vf_thread);vf_thread=-1;sceKernelDeleteSema(vf_ready);vf_ready=-1;vf_delete_lock();
}

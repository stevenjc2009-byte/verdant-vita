/* Test the production native storage queue with real host threads. */
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#define PLAT_SD "./"
#define SCE_KERNEL_CPU_MASK_USER_1 0x20000
typedef int SceUID;typedef unsigned SceSize;typedef pthread_mutex_t plat_mutex_t;
static plat_mutex_t *test_lock;
static pthread_t thread;static sem_t semaphore;static int (*entry_fn)(SceSize,void*);static bool fail_create;
static void plat_mutex_init(plat_mutex_t *m){test_lock=m;assert(!pthread_mutex_init(m,NULL));}
static void plat_mutex_lock(plat_mutex_t *m){assert(!pthread_mutex_lock(m));}
static void plat_mutex_unlock(plat_mutex_t *m){assert(!pthread_mutex_unlock(m));}
static int sceKernelDeleteMutex(int id){return pthread_mutex_destroy(test_lock);}
static int sceKernelCreateSema(const char *n,int attr,int count,int max,void *opts){assert(max==9);assert(!sem_init(&semaphore,0,count));return 1;}
static int sceKernelDeleteSema(int id){return sem_destroy(&semaphore);}
static int sceKernelWaitSema(int id,int n,void *timeout){return sem_wait(&semaphore);}
static int sceKernelSignalSema(int id,int n){return sem_post(&semaphore);}
static void *trampoline(void *arg){entry_fn(0,NULL);return NULL;}
static int sceKernelCreateThread(const char *n,int (*entry)(SceSize,void*),int priority,int stack,int attr,int mask,void *opts){assert(mask==0x20000);entry_fn=entry;return fail_create?-1:2;}
static int sceKernelStartThread(int id,int size,void *arg){return pthread_create(&thread,NULL,trampoline,NULL);}
static int sceKernelWaitThreadEnd(int id,void *status,void *timeout){return pthread_join(thread,NULL);}
static int sceKernelDeleteThread(int id){return 0;}
#include "../source/platform/vita/file_worker.h"
static void wait_result(unsigned id,const char *text){char path[128],value[VF_TEXT+32];snprintf(path,sizeof(path),"verdant/bridge/%u.res",id);FILE *f=NULL;for(int i=0;i<1000 && !(f=fopen(path,"rb"));i++)usleep(1000);assert(f);size_t got=fread(value,1,sizeof(value)-1,f);value[got]=0;fclose(f);assert(strstr(value,text));}
int main(void){
 mkdir("verdant",0777);mkdir("verdant/bridge",0777);mkdir("Folder",0777);
 char path[768],response[VF_TEXT+1];
 const char *bad[]={"/mnt/vita/ux0/../root","/mnt/vita/ux0/verdant//bridge/req","/mnt/vita/ux0/verdant///Image","/mnt/vita/ux0/verdant/rootfs.ext2","/mnt/vita/ux0/other:device","/mnt/vita/ux0evil/file"};
 for(int i=0;i<6;i++)assert(!vf_path(bad[i],path,sizeof(path)));
 assert(vf_path("/mnt/vita/ux0/Folder//",path,sizeof(path)) && !strcmp(path,"./Folder"));
 assert(!plat_files_request(1,"read","/root/Linux-only.txt",NULL));
 fail_create=true;assert(!plat_files_request(1,"list","/mnt/vita/ux0",NULL));fail_create=false;
 assert(plat_files_request(1,"write","/mnt/vita/ux0/Folder/b.txt","touch text\n"));wait_result(1,"OK\nSaved");
 assert(plat_files_request(2,"read","/mnt/vita/ux0/Folder/b.txt",NULL));wait_result(2,"OK\ntouch text\n");
 assert(plat_files_request(3,"list","/mnt/vita/ux0",NULL));wait_result(3,"D Folder");
 assert(plat_files_request(4,"mkdir","/mnt/vita/ux0/Folder/sub",NULL));wait_result(4,"Folder ready");
 assert(vf_execute("write","/mnt/vita/ux0/Folder/b.txt","second",response,sizeof(response)));
 assert(!vf_execute("write","/mnt/vita/ux0/Folder/sub","not a directory",response,sizeof(response)));
 assert(!vf_execute("mkdir","/mnt/vita/ux0/Folder/b.txt",NULL,response,sizeof(response)));
 assert(!rename("Folder/b.txt","Folder/b.txt.verdant-save-backup"));
 assert(vf_execute("write","/mnt/vita/ux0/Folder/b.txt","recovered",response,sizeof(response)));
 assert(vf_execute("read","/mnt/vita/ux0/Folder/b.txt",NULL,response,sizeof(response)) && !strcmp(response,"recovered"));
 assert(vf_execute("list","/mnt/vita/ux0/Folder",NULL,response,sizeof(response)) && !strcmp(response,"D sub\nF b.txt"));
 FILE *f=fopen("Folder/bad.txt","wb");assert(f);fwrite("\xc0\x80",1,2,f);fclose(f);assert(!vf_execute("read","/mnt/vita/ux0/Folder/bad.txt",NULL,response,sizeof(response)));
 assert(plat_files_request(5,"copy","/mnt/vita/ux0/Folder/b.txt","/mnt/vita/ux0/Folder/copy.txt"));wait_result(5,"Complete");
 assert(!vf_execute("copy","/mnt/vita/ux0/Folder/b.txt","/mnt/vita/ux0/Folder/copy.txt",response,sizeof(response)));
 assert(plat_files_request(6,"move","/mnt/vita/ux0/Folder/copy.txt","/mnt/vita/ux0/Folder/moved.txt"));wait_result(6,"Complete");assert(access("Folder/copy.txt",F_OK));
 for(unsigned i=10;i<18;i++){char body[24];snprintf(body,sizeof(body),"queued %u",i);assert(plat_files_request(i,"write","/mnt/vita/ux0/Folder/queued.txt",body));}
 vf_cleanup();for(unsigned i=10;i<18;i++)wait_result(i,"OK\nSaved");
 assert(vf_execute("read","/mnt/vita/ux0/Folder/queued.txt",NULL,response,sizeof(response)) && !strcmp(response,"queued 17"));
 char msg[15001];FILE *prefs=fopen("verdant/preferences.cfg","wb");assert(prefs);fputs("touch_x=7\n",prefs);fclose(prefs);
 assert(vf_execute("settingsbackup","/mnt/vita/ux0/verdant",NULL,msg,sizeof(msg)));
 prefs=fopen("verdant/preferences.cfg","wb");fputs("touch_x=0\n",prefs);fclose(prefs);
 assert(vf_execute("settingsrestore","/mnt/vita/ux0/verdant",NULL,msg,sizeof(msg)));assert(vf_execute("read","/mnt/vita/ux0/verdant/preferences.cfg",NULL,msg,sizeof(msg))&&!strcmp(msg,"touch_x=7\n"));
 char backup_name[100];prefs=fopen("verdant/backups/latest-settings.txt","rb");assert(prefs);assert(fgets(backup_name,sizeof(backup_name),prefs));fclose(prefs);backup_name[strcspn(backup_name,"\n")]=0;char badbackup[300];snprintf(badbackup,sizeof(badbackup),"verdant/backups/%s/preferences.cfg",backup_name);prefs=fopen(badbackup,"wb");fputs("corrupt",prefs);fclose(prefs);assert(!vf_execute("settingsrestore","/mnt/vita/ux0/verdant",NULL,msg,sizeof(msg)));assert(vf_execute("read","/mnt/vita/ux0/verdant/preferences.cfg",NULL,msg,sizeof(msg))&&!strcmp(msg,"touch_x=7\n"));
 assert(vf_execute("list","/mnt/vita/ux0/verdant/backups/documents",NULL,msg,sizeof(msg))&&strstr(msg,".txt"));
 vf_read=0;vf_write=2;vf_queue[0]=(VFRequest){.op="copy",.path="/mnt/vita/ux0/large.bin",.data="/mnt/vita/ux0/dest.bin"};vf_queue[1]=(VFRequest){.op="read",.path="/mnt/vita/ux0/notes.txt"};assert(vf_pick(0)==1&&vf_pick(4)==0);strcpy(vf_queue[1].path,"/mnt/vita/ux0/dest.bin");assert(vf_pick(0)==0);vf_read=vf_write=0;
 puts("Native Vita storage: protected paths, normalization, sorting, text/UTF-8 limits, asynchronous results, FIFO drain and failed-start fallback passed without Linux.");
}

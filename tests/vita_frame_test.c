/* Execute the actual production handoff on host threads/semaphores. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
typedef struct {int x,y,w,h;} plat_damage_t;
typedef int SceUID;
typedef unsigned SceSize;
typedef struct { size_t size; void *base; unsigned pitch,pixelformat,width,height; } SceDisplayFrameBuf;
#define VITA_FB_PITCH_PX 4
#define VITA_FB_PITCH 16
#define VITA_SCREEN_W 4
#define VITA_SCREEN_H 4
#define PLAT_TERM_H 2
#define PLAT_PANEL_H 2
static int vita_desktop_scale=100;
#define SCE_DISPLAY_PIXELFORMAT_A8B8G8R8 0
#define SCE_DISPLAY_SETBUF_NEXTFRAME 1
#define SCE_KERNEL_CPU_MASK_USER_1 0x20000
static uint8_t drawing[64], scanout[2][64];
static uint8_t *vita_fb = drawing, *vita_scanout[2] = {scanout[0],scanout[1]};
static int vita_front;
static SceUID vita_display_thread = -1;
static SceUID vita_frame_ready = -1, vita_copy_done = -1, vita_frame_done = -1;
static atomic_bool vita_display_stop;
static sem_t sems[64];
static int next_sema=1, fail_create, fail_start, fail_sema, frames, locked_core=1;
static pthread_t thread;
static int (*entry)(SceSize,void *);
static uint8_t *pending;
static bool check_pattern=true;
static int sceKernelCreateSema(const char *n,unsigned a,int count,int max,void *o) {
  if (fail_sema && next_sema==fail_sema) { next_sema++; return -5; }
  int id=next_sema++; assert(id<64 && max==1); assert(!sem_init(&sems[id],0,count));return id;
}
static int sceKernelDeleteSema(int id) { return sem_destroy(&sems[id]); }
static int sceKernelWaitSema(int id,int count,void *timeout) { assert(count==1);return sem_wait(&sems[id]); }
static int sceKernelSignalSema(int id,int count) {
  int value;sem_getvalue(&sems[id],&value);assert(count==1 && value<1);return sem_post(&sems[id]);
}
static int sceDisplaySetFrameBuf(SceDisplayFrameBuf *fb,int sync) {
  assert(sync==1);pending=fb->base;return 0;
}
static int sceDisplayWaitSetFrameBuf(void) {
  usleep(1000); /* Main may already be painting its next frame. */
  frames++;
  if(check_pattern)for(int i=0;i<64;i++)assert(pending[i]==(uint8_t)frames);
  return 0;
}
static void *trampoline(void *arg) { entry(0,NULL);return NULL; }
static int sceKernelCreateThread(const char *n,int (*f)(SceSize,void*),int p,int stack,int attr,int affinity,void *opts) {
  assert(affinity==SCE_KERNEL_CPU_MASK_USER_1 || affinity==0x80000);
  if(affinity==0x80000 && locked_core)return -44;entry=f;return fail_create?-42:17;
}
static int sceKernelStartThread(int id,int size,void *arg) {
  return fail_start?-43:pthread_create(&thread,NULL,trampoline,NULL);
}
static int sceKernelWaitThreadEnd(int id,void *status,void *timeout) { return pthread_join(thread,NULL); }
static int sceKernelDeleteThread(int id) { return 0; }
#include "../source/platform/vita/frame_worker.h"
int main(void) {
  assert(!vita_display_start());assert(vita_display_core==1);
  for(int i=1;i<=40;i++) { memset(drawing,i,sizeof(drawing));plat_present(3); }
  /* Stop with the last flip in flight; cleanup must wait without deadlocking. */
  vita_display_cleanup();assert(frames==40 && vita_display_thread<0);
  fail_create=1;assert(vita_display_start()==-42);
  memset(drawing,41,sizeof(drawing));plat_present(3);assert(frames==41);fail_create=0;
  fail_start=1;assert(vita_display_start()==-43);fail_start=0;
  fail_sema=next_sema+1;assert(vita_display_start()<0);fail_sema=0;
  assert(!vita_display_start());
  memset(drawing,42,sizeof(drawing));plat_present(3);vita_display_cleanup();assert(frames==42);
  locked_core=0;assert(!vita_display_start());assert(vita_display_core==3);vita_display_cleanup();
  check_pattern=false;vita_desktop_scale=200;
  for(int i=0;i<16;i++)((uint32_t*)drawing)[i]=0xff000000+i;
  vita_transfer_frame(false);
  uint32_t *scaled=(uint32_t*)vita_scanout[vita_front];
  for(int y=0;y<4;y++)for(int x=0;x<4;x++)assert(scaled[y*4+x]==0xff000000u+(y>=2?8:0)+x/2);
  vita_desktop_scale=100;vita_damage_full=true;vita_transfer_frame(false);
  plat_damage_t regions[2]={{0,0,1,1},{0,0,0,0}};
  ((uint32_t*)drawing)[0]=1234;vita_damage_full=false;memcpy(vita_damage,regions,sizeof(regions));vita_transfer_frame(false);
  regions[0]=(plat_damage_t){1,0,1,1};((uint32_t*)drawing)[1]=5678;memcpy(vita_damage,regions,sizeof(regions));uint64_t before=vita_transfer_bytes;vita_transfer_frame(false);
  assert(vita_transfer_bytes-before==8);assert(!memcmp(vita_scanout[vita_front],drawing,64));
  vita_desktop_scale=200;vita_damage_full=true;vita_transfer_frame(false);
  regions[0]=(plat_damage_t){0,0,1,1};regions[1]=(plat_damage_t){0,0,0,0};((uint32_t*)drawing)[0]=4321;vita_damage_full=false;memcpy(vita_damage,regions,sizeof(regions));vita_transfer_frame(false);
  regions[0]=(plat_damage_t){1,0,1,1};((uint32_t*)drawing)[1]=8765;memcpy(vita_damage,regions,sizeof(regions));before=vita_transfer_bytes;vita_transfer_frame(false);assert(vita_transfer_bytes-before==32);
  scaled=(uint32_t*)vita_scanout[vita_front];for(int y=0;y<4;y++)for(int x=0;x<4;x++)assert(scaled[y*4+x]==((uint32_t*)drawing)[(y>=2?8:0)+x/2]);
   puts("Production display worker: frame ownership, backpressure, core selection, shutdown, restart and failure fallback passed.");
}

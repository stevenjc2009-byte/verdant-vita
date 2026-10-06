/* Shared production display handoff; tested with host semaphore bindings. */
static int vita_display_core=1;
static bool vita_damage_full=true,vita_previous_full=true;
static plat_damage_t vita_damage[2],vita_previous_damage[2];
static uint64_t vita_transfer_bytes;
static plat_damage_t vita_damage_union(plat_damage_t a,plat_damage_t b) {
 if(!a.w || !a.h)return b;
 if(!b.w || !b.h)return a;
 int right=a.x+a.w>b.x+b.w?a.x+a.w:b.x+b.w,bottom=a.y+a.h>b.y+b.h?a.y+a.h:b.y+b.h;
 int x=a.x<b.x?a.x:b.x,y=a.y<b.y?a.y:b.y;return (plat_damage_t){x,y,right-x,bottom-y};
}
static int vita_map_scale=-1;
static int vita_xmap[VITA_SCREEN_W],vita_ymap[VITA_SCREEN_H];
static void vita_transfer_frame(bool notify_copy) {
  int back = vita_front ^ 1;
  int source_w=VITA_SCREEN_W*100/vita_desktop_scale;
  if(vita_map_scale!=vita_desktop_scale) {
    for(int x=0;x<VITA_SCREEN_W;x++)vita_xmap[x]=x*source_w/VITA_SCREEN_W;
    for(int y=0;y<VITA_SCREEN_H;y++){
      int lower=y>=PLAT_TERM_H,height=lower?PLAT_PANEL_H:PLAT_TERM_H;
      vita_ymap[y]=(lower?PLAT_TERM_H:0)+(y-(lower?PLAT_TERM_H:0))*(height*100/vita_desktop_scale)/height;
    }
    vita_map_scale=vita_desktop_scale;
  }
  if(vita_desktop_scale==100 && (vita_damage_full || vita_previous_full)){memcpy(vita_scanout[back],vita_fb,VITA_FB_PITCH*VITA_SCREEN_H);vita_transfer_bytes+=VITA_FB_PITCH*VITA_SCREEN_H;}
  else for(int screen=0;screen<2;screen++) {
    int height=screen?PLAT_PANEL_H:PLAT_TERM_H,offset=screen?PLAT_TERM_H:0,source_h=height*100/vita_desktop_scale;
    plat_damage_t region=(vita_damage_full || vita_previous_full)?(plat_damage_t){0,0,source_w,source_h}:vita_damage_union(vita_damage[screen],vita_previous_damage[screen]);
    if(region.w<=0 || region.h<=0)continue;
    int left=(region.x*VITA_SCREEN_W+source_w-1)/source_w,right=((region.x+region.w)*VITA_SCREEN_W+source_w-1)/source_w;
    int top=(region.y*height+source_h-1)/source_h,bottom=((region.y+region.h)*height+source_h-1)/source_h;
    if(left<0)left=0;
    if(top<0)top=0;
    if(right>VITA_SCREEN_W)right=VITA_SCREEN_W;
    if(bottom>height)bottom=height;
    for(int y=offset+top;y<offset+bottom;y++) {
      uint32_t *src=(uint32_t*)(vita_fb+vita_ymap[y]*VITA_FB_PITCH),*dst=(uint32_t*)(vita_scanout[back]+y*VITA_FB_PITCH);
      if(vita_desktop_scale==100)memcpy(dst+left,src+left,(right-left)*4);
      else for(int x=left;x<right;x++)dst[x]=src[vita_xmap[x]];
      vita_transfer_bytes+=(right-left)*4;
    }
  }
  vita_previous_full=vita_damage_full;memcpy(vita_previous_damage,vita_damage,sizeof(vita_damage));
  /* After this acknowledgement the UI may safely edit its cached buffer. */
  if (notify_copy) sceKernelSignalSema(vita_copy_done, 1);
  SceDisplayFrameBuf fb = {
    .size = sizeof(fb), .base = vita_scanout[back],
    .pitch = VITA_FB_PITCH_PX, .pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
    .width = VITA_SCREEN_W, .height = VITA_SCREEN_H,
  };
  if (sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME) < 0) return;
  sceDisplayWaitSetFrameBuf();
  vita_front = back;
}
static int vita_display_entry(SceSize size, void *arg) {
  while (sceKernelWaitSema(vita_frame_ready, 1, NULL) >= 0) {
    if (atomic_load(&vita_display_stop)) break;
    vita_transfer_frame(true);
    sceKernelSignalSema(vita_frame_done, 1);
  }
  return 0;
}
static void vita_display_cleanup(void) {
  if (vita_display_thread >= 0) {
    sceKernelWaitSema(vita_frame_done, 1, NULL);
    atomic_store(&vita_display_stop, true);
    sceKernelSignalSema(vita_frame_ready, 1);
    sceKernelWaitThreadEnd(vita_display_thread, NULL, NULL);
    sceKernelDeleteThread(vita_display_thread);
    vita_display_thread = -1;
  }
  if (vita_frame_ready >= 0) sceKernelDeleteSema(vita_frame_ready);
  if (vita_copy_done >= 0) sceKernelDeleteSema(vita_copy_done);
  if (vita_frame_done >= 0) sceKernelDeleteSema(vita_frame_done);
  vita_frame_ready = vita_copy_done = vita_frame_done = -1;
}
static int vita_display_start(void) {
  atomic_store(&vita_display_stop, false);
  vita_frame_ready = sceKernelCreateSema("verdant_frame_ready", 0, 0, 1, NULL);
  vita_copy_done = sceKernelCreateSema("verdant_copy_done", 0, 0, 1, NULL);
  vita_frame_done = sceKernelCreateSema("verdant_frame_done", 0, 1, 1, NULL);
  if (vita_frame_ready < 0 || vita_copy_done < 0 || vita_frame_done < 0) {
    vita_display_cleanup(); return -1;
  }
  int masks[]={0x80000,SCE_KERNEL_CPU_MASK_USER_1},last_error=-1;
  for(int i=0;i<2;i++) {
    SceUID thread=sceKernelCreateThread("verdant_vita_display",vita_display_entry,0x10000100,32768,0,masks[i],NULL);
    if(thread<0){last_error=thread;continue;}
    int result=sceKernelStartThread(thread,0,NULL);
    if(result<0){sceKernelDeleteThread(thread);last_error=result;continue;}
    vita_display_thread=thread;vita_display_core=i==0?3:1;return 0;
  }
  vita_display_core=1;vita_display_cleanup();return last_error;
}
static void vita_present_damage(const plat_damage_t *regions) {
 if(!vita_fb)return;
 if(vita_display_thread>=0)sceKernelWaitSema(vita_frame_done,1,NULL);
 vita_damage_full=regions==NULL;if(regions)memcpy(vita_damage,regions,sizeof(vita_damage));
 if(vita_display_thread<0){vita_transfer_frame(false);return;}
 sceKernelSignalSema(vita_frame_ready,1);sceKernelWaitSema(vita_copy_done,1,NULL);
}
void plat_present_regions(const plat_damage_t regions[2]){vita_present_damage(regions);}
void plat_present(unsigned mask){if(mask)vita_present_damage(NULL);}

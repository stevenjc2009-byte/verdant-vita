/* Headless validation backend. No simulated device success: unsupported hardware
 * reports false. Runs the actual emulator and guest; screenshots are raw PPM. */
#include "plat.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
static uint8_t fb[2][PLAT_TERM_W * PLAT_TERM_H * 3];
static uint64_t start;
static pthread_t thread;
static void (*thread_entry)(void *);
static void *thread_arg;
static plat_caps_t caps = {.emu_thread = true, .pointer = true, .net = true, .rng = true};
uint64_t plat_us(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (uint64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
}
bool plat_init(void) {
  start = plat_us();
  return true;
}
void plat_exit(void) {}
bool plat_running(void) {
  const char *s = getenv("VERDANT_TEST_SECONDS");
  return plat_us() - start < (uint64_t)(s ? atoi(s) : 120) * 1000000;
}
const plat_caps_t *plat_caps(void) { return &caps; }
const char *plat_model(void) { return "Headless host validation (not console hardware)"; }
void plat_ui_cadence(uint32_t *r, uint32_t *p) {
  *r = 50000;
  *p = 10000;
}
uint64_t plat_wallclock_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return (uint64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
void plat_sleep_us(uint64_t us) { usleep(us); }
bool plat_surface(plat_surf s, plat_fb_t *out) {
  int w=s ? PLAT_PANEL_W : PLAT_TERM_W, h=s ? PLAT_PANEL_H : PLAT_TERM_H;
  *out = (plat_fb_t){fb[s], w, h, 3, w * 3, 3};
  return true;
}
void plat_present(unsigned mask) {
  static uint64_t last;
  if (plat_us() - last < 1000000)
    return;
  last = plat_us();
  for (int s = 0; s < 2; s++) {
    char path[32];
    snprintf(path, sizeof(path), "screen-%d.ppm", s);
    FILE *f = fopen(path, "wb");
    if (!f)
      continue;
    int w=s ? PLAT_PANEL_W : PLAT_TERM_W, h=s ? PLAT_PANEL_H : PLAT_TERM_H;
    fprintf(f, "P6\n%d %d\n255\n", w,h);
    for (int i = 0; i < w * h; i++) {
      uint8_t rgb[3] = {fb[s][i * 3 + 2], fb[s][i * 3 + 1], fb[s][i * 3]};
      fwrite(rgb, 1, 3, f);
    }
    fclose(f);
  }
}
void plat_poll_input(plat_input_t *out) { memset(out, 0, sizeof(*out)); }
void plat_poll_keyboard(void) {}
static void *run(void *p) {
  thread_entry(thread_arg);
  return NULL;
}
bool plat_thread_start(void (*entry)(void *), void *arg) {
  thread_entry = entry;
  thread_arg = arg;
  return pthread_create(&thread, NULL, run, NULL) == 0;
}
void plat_thread_join(void) { pthread_join(thread, NULL); }
const char *plat_thread_desc(void) { return "POSIX thread"; }
_Static_assert(sizeof(pthread_mutex_t) <= PLAT_MUTEX_SIZE, "lock storage");
void plat_mutex_init(plat_mutex_t *m) { pthread_mutex_init((pthread_mutex_t *)m, NULL); }
void plat_mutex_lock(plat_mutex_t *m) { pthread_mutex_lock((pthread_mutex_t *)m); }
void plat_mutex_unlock(plat_mutex_t *m) { pthread_mutex_unlock((pthread_mutex_t *)m); }
void pkbd_init(void) {}
void pkbd_apply(const struct AdaPalette *p, bool b, bool s, int m) {}
void pkbd_invalidate(void) {}
void pkbd_update(const plat_input_t *in) {}
void pkbd_draw(void) {}
bool plat_net_init(void) { return true; }
void plat_net_exit(void) {}
bool plat_random(void *buf, size_t n) { return getrandom(buf, n, 0) == (ssize_t)n; }
void plat_sample_axes(int32_t *out) { memset(out, 0, VI_NAXES * sizeof(*out)); }
static const plat_tree_t trees[] = {{"sd", "./", false}};
int plat_v9p_trees(const plat_tree_t **out) {
  *out = trees;
  return 1;
}
bool plat_v9p_mount(int idx) { return idx == 0; }
void plat_v9p_unmount_all(void) {}
int64_t plat_v9p_free_bytes(void) {
  struct statvfs s;
  return statvfs(".", &s) ? -1 : (int64_t)s.f_bavail * s.f_frsize;
}
static int console_size(char *b, int n) { return snprintf(b, n, "80 30\n"); }
const plat_hw_ent plat_hw_files[] = {{"console_size", 0444, console_size, NULL, HWS_NONE}};
const int plat_hw_count = 1;
int plat_hw_camera(bool inner, uint8_t **frame) {
  *frame = NULL;
  return 0;
}
int plat_hw_mic_read(uint8_t *out, int n) { return 0; }
int plat_hw_audio_write(const uint8_t *data, int n) { return 0; }
int plat_update_title(const char *package) { return 0; }

#ifdef PLAT_VITA
#include "../vita/native_files.h"
void plat_present_regions(const plat_damage_t regions[2]){plat_present(3);}
bool plat_files_request(unsigned id,const char *op,const char *a,const char *b) {
 if(!getenv("VERDANT_NATIVE_FILES") || !vf_supported(op,a,b))return false;
 char response[VF_TEXT+1],path[256];bool ok=vf_execute(op,a,b,response,sizeof(response));snprintf(path,sizeof(path),PLAT_SD "verdant/bridge/%u.res",id);
 FILE *f=fopen(path,"wb");if(!f)return false;fprintf(f,"%s\n%s",ok?"OK":"ERROR",response);fclose(f);return true;
}
void plat_desktop_scale(int percent) {}
/* Exercise the same native HTTPS worker in integration tests. */
#include <stdatomic.h>
typedef int SceUID;
typedef unsigned SceSize;
#define SCE_KERNEL_CPU_MASK_USER_1 0x20000
static pthread_t http_thread;
static int (*http_entry)(SceSize,void*);
static void *http_run(void *arg) { http_entry(0,NULL);return NULL; }
static int sceKernelCreateThread(const char *name,int (*entry)(SceSize,void*),int priority,int size,int attr,int mask,void *option) { http_entry=entry;return 1; }
static int sceKernelStartThread(int id,int size,void *arg) { return pthread_create(&http_thread,NULL,http_run,NULL); }
static int sceKernelWaitThreadEnd(int id,void *status,void *timeout) { return pthread_join(http_thread,NULL); }
static int sceKernelDeleteThread(int id) { return 0; }
static void sceKernelDelayThread(int delay) { usleep(delay); }
#define plat_http_start test_http_start
#include "../vita/native_http.h"
#undef plat_http_start
void plat_http_start(void) { if(!getenv("VERDANT_DISABLE_NATIVE_HTTP"))test_http_start(); }

void plat_performance(plat_performance_t *out) {
  memset(out,0,sizeof(*out));
  for(int i=0;i<4;i++) out->cores[i]=-1;
  out->wifi_state=out->signal=-1;
}
#endif

#ifndef VERDANT_PROFILER_H
#define VERDANT_PROFILER_H
#include <stdatomic.h>
/* One writer (emulator), lock-free UI snapshots. Microseconds wrap safely at
   32 bits; interval differences are sampled once per second. No per-op locks. */
static struct {atomic_uint execute_us,poll_us,idle_us,batches,wfi,pc;} vp;
static struct {uint64_t tick;unsigned execute,poll,idle,batches,wfi;float active,devices,waits,bps,wps;unsigned pc;} vp_view;
static void vp_sample(uint64_t now){if(now-vp_view.tick<1000000)return;unsigned e=atomic_load(&vp.execute_us),p=atomic_load(&vp.poll_us),i=atomic_load(&vp.idle_us),b=atomic_load(&vp.batches),w=atomic_load(&vp.wfi);if(vp_view.tick){double dt=now-vp_view.tick;vp_view.active=(unsigned)(e-vp_view.execute)*100.0/dt;vp_view.devices=(unsigned)(p-vp_view.poll)*100.0/dt;vp_view.waits=(unsigned)(i-vp_view.idle)*100.0/dt;vp_view.bps=(unsigned)(b-vp_view.batches)*1000000.0/dt;vp_view.wps=(unsigned)(w-vp_view.wfi)*1000000.0/dt;}vp_view.tick=now;vp_view.execute=e;vp_view.poll=p;vp_view.idle=i;vp_view.batches=b;vp_view.wfi=w;vp_view.pc=atomic_load(&vp.pc);}
#endif

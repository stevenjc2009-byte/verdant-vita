/* Shared production display handoff; tested with host semaphore bindings. */
static void vita_transfer_frame(bool notify_copy) {
  int back = vita_front ^ 1;
  if (vita_desktop_scale == 100) {
    memcpy(vita_scanout[back], vita_fb, VITA_FB_PITCH * VITA_SCREEN_H);
  } else {
    int source_w = VITA_SCREEN_W * 100 / vita_desktop_scale;
    int xmap[VITA_SCREEN_W];
    for (int x = 0; x < VITA_SCREEN_W; x++) xmap[x] = x * source_w / VITA_SCREEN_W;
    for (int y = 0; y < VITA_SCREEN_H; y++) {
      int lower = y >= PLAT_TERM_H;
      int height = lower ? PLAT_PANEL_H : PLAT_TERM_H;
      int local_y = y - (lower ? PLAT_TERM_H : 0);
      int source_y = local_y * (height * 100 / vita_desktop_scale) / height;
      uint32_t *src = (uint32_t *)(vita_fb + ((lower ? PLAT_TERM_H : 0) + source_y) * VITA_FB_PITCH);
      uint32_t *dst = (uint32_t *)(vita_scanout[back] + y * VITA_FB_PITCH);
      for (int x = 0; x < VITA_SCREEN_W; x++) dst[x] = src[xmap[x]];
    }
  }
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
  SceUID thread = sceKernelCreateThread("verdant_vita_display", vita_display_entry,
                        0x10000100, 32768, 0, SCE_KERNEL_CPU_MASK_USER_1, NULL);
  if (thread < 0) { vita_display_cleanup(); return thread; }
  int result = sceKernelStartThread(thread, 0, NULL);
  if (result < 0) { sceKernelDeleteThread(thread); vita_display_cleanup(); return result; }
  vita_display_thread = thread;
  return 0;
}
void plat_present(unsigned mask) {
  if (!mask || !vita_fb) return;
  if (vita_display_thread < 0) { vita_transfer_frame(false); return; }
  /* Queue depth one: never let old frames accumulate input latency. */
  sceKernelWaitSema(vita_frame_done, 1, NULL);
  sceKernelSignalSema(vita_frame_ready, 1);
  sceKernelWaitSema(vita_copy_done, 1, NULL);
}


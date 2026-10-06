/* Original Verdant Vita media adapter. Fixed shared hardware-file formats:
 * RGB565 400x240, mono PCM16 16360 Hz, stereo PCM16 32730 Hz.
 * Native capture/playback runs outside the emulator thread. */
#pragma once
#include <psp2/audioin.h>
#include <psp2/audioout.h>
#include <psp2/camera.h>
#include <stdatomic.h>
#define VM_RING 8192
static plat_mutex_t vm_lock;
static plat_mutex_t vm_io_lock;
static atomic_bool vm_audio_run, vm_mic_run;
static SceUID vm_audio_thread = -1, vm_mic_thread = -1;
static int vm_audio_port = -1, vm_mic_port = -1;
static int16_t vm_audio[VM_RING][2], vm_mic[VM_RING];
static unsigned vm_ar, vm_aw, vm_mr, vm_mw;
static unsigned vm_audio_phase, vm_mic_phase;
static int16_t vm_last_left, vm_last_right;

static int vm_audio_entry(SceSize n, void *data) {
  int16_t output[1024 * 2];
  while (atomic_load(&vm_audio_run)) {
    plat_mutex_lock(&vm_lock);
    for (int i = 0; i < 1024; i++) {
      vm_audio_phase += 32730;
      if (vm_audio_phase >= 48000) {
        vm_audio_phase -= 48000;
        if (vm_ar != vm_aw) {
          vm_last_left = vm_audio[vm_ar][0];
          vm_last_right = vm_audio[vm_ar][1];
          vm_ar = (vm_ar + 1) % VM_RING;
        } else
          vm_last_left = vm_last_right = 0;
      }
      output[i * 2] = vm_last_left;
      output[i * 2 + 1] = vm_last_right;
    }
    plat_mutex_unlock(&vm_lock);
    if (sceAudioOutOutput(vm_audio_port, output) < 0)
      break;
  }
  atomic_store(&vm_audio_run, false);
  return 0;
}
static int vm_mic_entry(SceSize n, void *data) {
  int16_t input[256];
  while (atomic_load(&vm_mic_run)) {
    if (sceAudioInInput(vm_mic_port, input) < 0)
      break;
    plat_mutex_lock(&vm_lock);
    for (int i = 0; i < 256; i++) {
      vm_mic_phase += 16360;
      while (vm_mic_phase >= 16000) {
        vm_mic_phase -= 16000;
        unsigned next = (vm_mw + 1) % VM_RING;
        if (next == vm_mr)
          vm_mr = (vm_mr + 1) % VM_RING;
        vm_mic[vm_mw] = input[i];
        vm_mw = next;
      }
    }
    plat_mutex_unlock(&vm_lock);
  }
  atomic_store(&vm_mic_run, false);
  return 0;
}
static void vm_stop_mic(void) {
  atomic_store(&vm_mic_run, false);
  if (vm_mic_thread >= 0) {
    sceKernelWaitThreadEnd(vm_mic_thread, NULL, NULL);
    sceKernelDeleteThread(vm_mic_thread);
    vm_mic_thread = -1;
  }
  if (vm_mic_port >= 0) {
    sceAudioInReleasePort(vm_mic_port);
    vm_mic_port = -1;
  }
  vm_mr = vm_mw = vm_mic_phase = 0;
}
static int vm_mic_control(const char *b, int len) {
  if (len < 4 || memcmp(b, "stop", 4))
    return -1;
  plat_mutex_lock(&vm_io_lock);
  vm_stop_mic();
  plat_mutex_unlock(&vm_io_lock);
  return len;
}
static int vm_mic_read(uint8_t *out, int max) {
  if (sceKernelIsPSVitaTV() || max < 2)
    return 0;
  if (vm_mic_thread >= 0 && !atomic_load(&vm_mic_run))
    vm_stop_mic();
  if (vm_mic_thread < 0) {
    vm_mic_port = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_RAW, 256, 16000,
                                     SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
    if (vm_mic_port < 0)
      return 0;
    atomic_store(&vm_mic_run, true);
    vm_mic_thread =
        sceKernelCreateThread("verdant_mic", vm_mic_entry, 0x10000100, 32768, 0, 0, NULL);
    if (vm_mic_thread < 0 || sceKernelStartThread(vm_mic_thread, 0, NULL) < 0) {
      atomic_store(&vm_mic_run, false);
      if (vm_mic_thread >= 0)
        sceKernelDeleteThread(vm_mic_thread);
      vm_mic_thread = -1;
      sceAudioInReleasePort(vm_mic_port);
      vm_mic_port = -1;
      return 0;
    }
  }
  int n = 0;
  plat_mutex_lock(&vm_lock);
  while (vm_mr != vm_mw && n + 2 <= max) {
    g_st16(out + n, (uint16_t)vm_mic[vm_mr]);
    vm_mr = (vm_mr + 1) % VM_RING;
    n += 2;
  }
  plat_mutex_unlock(&vm_lock);
  if (!n) {
    n = max < 1024 ? max : 1024;
    n -= n % 2;
    memset(out, 0, n);
  }
  return n;
}
static int vm_audio_write(const uint8_t *data, int len) {
  if (vm_audio_thread >= 0 && !atomic_load(&vm_audio_run)) {
    sceKernelWaitThreadEnd(vm_audio_thread, NULL, NULL);
    sceKernelDeleteThread(vm_audio_thread);
    vm_audio_thread = -1;
    sceAudioOutReleasePort(vm_audio_port);
    vm_audio_port = -1;
  }
  if (vm_audio_thread < 0) {
    vm_audio_port =
        sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, 1024, 48000, SCE_AUDIO_OUT_MODE_STEREO);
    if (vm_audio_port < 0)
      return 0;
    atomic_store(&vm_audio_run, true);
    vm_audio_thread =
        sceKernelCreateThread("verdant_audio", vm_audio_entry, 0x10000100, 32768, 0, 0, NULL);
    if (vm_audio_thread < 0 || sceKernelStartThread(vm_audio_thread, 0, NULL) < 0) {
      atomic_store(&vm_audio_run, false);
      if (vm_audio_thread >= 0)
        sceKernelDeleteThread(vm_audio_thread);
      vm_audio_thread = -1;
      sceAudioOutReleasePort(vm_audio_port);
      vm_audio_port = -1;
      return 0;
    }
  }
  plat_mutex_lock(&vm_lock);
  for (int i = 0; i + 4 <= len; i += 4) {
    unsigned next = (vm_aw + 1) % VM_RING;
    if (next == vm_ar)
      break;
    vm_audio[vm_aw][0] = (int16_t)g_ld16(data + i);
    vm_audio[vm_aw][1] = (int16_t)g_ld16(data + i + 2);
    vm_aw = next;
  }
  plat_mutex_unlock(&vm_lock);
  return len;
}
static int vm_camera(bool inner, uint8_t **frame) {
  *frame = NULL;
  if (sceKernelIsPSVitaTV())
    return 0;
  int device = inner ? SCE_CAMERA_DEVICE_FRONT : SCE_CAMERA_DEVICE_BACK;
  SceUID memory = sceKernelAllocMemBlock("verdant_camera", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                                         512 * 1024, NULL);
  if (memory < 0)
    return 0;
  uint8_t *pixels = NULL;
  int got = 0;
  bool opened = false;
  if (sceKernelGetMemBlockBase(memory, (void **)&pixels) < 0)
    goto done;
  memset(pixels, 0, 512 * 1024);
  SceCameraInfo info = {.size = sizeof(info),
                        .format = SCE_CAMERA_FORMAT_ABGR,
                        .resolution = SCE_CAMERA_RESOLUTION_320_240,
                        .framerate = SCE_CAMERA_FRAMERATE_30_FPS,
                        .sizeIBase = 320 * 240 * 4,
                        .pIBase = pixels,
                        .pitch = 0};
  if (sceCameraOpen(device, &info) < 0)
    goto done;
  opened = true;
  if (sceCameraStart(device) < 0)
    goto done;
  for (int retry = 0; retry < 30; retry++) {
    SceCameraRead read = {.size = sizeof(read), .mode = 1};
    if (sceCameraRead(device, &read) >= 0 && (read.frame || read.timestamp)) {
      uint8_t *raw = calloc(1, 400 * 240 * 2);
      if (!raw)
        break;
      for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++) {
          uint8_t *p = pixels + (y * 320 + x) * 4;
          uint16_t rgb = ((p[0] >> 3) << 11) | ((p[1] >> 2) << 5) | (p[2] >> 3);
          g_st16(raw + (y * 400 + x + 40) * 2, rgb);
        }
      *frame = raw;
      got = 400 * 240 * 2;
      break;
    }
    sceKernelDelayThread(33333);
  }
done:
  if (opened) {
    sceCameraStop(device);
    sceCameraClose(device);
  }
  sceKernelFreeMemBlock(memory);
  return got;
}
static void vm_init(void) {
  plat_mutex_init(&vm_lock);
  plat_mutex_init(&vm_io_lock);
}
static void vm_exit(void) {
  vm_stop_mic();
  atomic_store(&vm_audio_run, false);
  if (vm_audio_thread >= 0) {
    sceKernelWaitThreadEnd(vm_audio_thread, NULL, NULL);
    sceKernelDeleteThread(vm_audio_thread);
    vm_audio_thread = -1;
  }
  if (vm_audio_port >= 0) {
    sceAudioOutReleasePort(vm_audio_port);
    vm_audio_port = -1;
  }
}

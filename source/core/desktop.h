#ifndef VERDANT_DESKTOP_H
#define VERDANT_DESKTOP_H
/* Shared native desktop; guest execution stays in Linux through the SD mailbox.
 */
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

#define VD_MAX 12
#define VD_TEXT 16384
#define VD_SCALE (vd.ui_scale ? vd.ui_scale : 100)
#ifdef PLAT_VITA
#define VD_TERM_W (PLAT_TERM_W * 100 / VD_SCALE)
#define VD_TERM_H (PLAT_TERM_H * 100 / VD_SCALE)
#define VD_PANEL_W (PLAT_PANEL_W * 100 / VD_SCALE)
#define VD_PANEL_H (PLAT_PANEL_H * 100 / VD_SCALE)
#else
#define VD_TERM_W PLAT_TERM_W
#define VD_TERM_H PLAT_TERM_H
#define VD_PANEL_W PLAT_PANEL_W
#define VD_PANEL_H PLAT_PANEL_H
#endif
#define VD_W VD_TERM_W
#define VD_H (VD_TERM_H + VD_PANEL_H)
#define VD_PANEL_X ((VD_TERM_W - VD_PANEL_W) / 2)
#define VD_BRIDGE PLAT_SD "verdant/bridge/"
#define VD_BG 0x10241d
#define VD_SURFACE 0x182d25
#define VD_TEXT_COLOR 0xe2efe6
#define VD_ACCENT 0x88c572
#include "updater.h"
#include "setup.h"
enum {
  VD_TERM,
  VD_FILES,
  VD_EDIT,
  VD_CALC,
  VD_TASKS,
  VD_SETTINGS,
  VD_SSH,
  VD_DOWNLOAD,
  VD_MEDIA,
  VD_PACKAGES,
  VD_REMOTE,
  VD_BACKUP,
  VD_UPDATER,
  VD_APP_COUNT
};
static const char *vd_names[] = {
    "Terminal",     "Files",    "Text editor",    "Calculator",
    "Task manager", "Settings", "SSH & transfer", "Downloads",
    "Media",        "Packages", "Remote desktop", "Backups",
    "System update"};
typedef struct {
  bool used, minimized, maximized;
  bool toolbar_page;
  int app, x, y, w, h, ox, oy, ow, oh, workspace, scroll, selection, session;
  unsigned pending, job;
  int entry_mode, image_w, image_h, cursor;
  int task_tab, task_category; uint64_t performance_tick;
  unsigned char *image;
  char pending_op[24];
  long output_offset;
  TermState *terminal;
  char title[64], path[512], input[2048], previous_input[2048], text[VD_TEXT];
} VDWindow;
static struct {
  bool active, dirty, menu, keyboard, touchpad, joined, ptr_was_down, shift,
      ctrl, alt, symbols, cut, recovery, quitting, resizing;
  bool auto_update, update_checked;
  bool pointer_dirty;
  int ui_scale;
  int workspace, focused, px, py, drag, drag_dx, drag_dy, pan_x, pan_y,
      wallpaper;
  int select_start, select_end;
  bool selecting, has_selection;
  unsigned serial;
  uint64_t last_poll, last_render, last_clock;
  uint64_t last_job, last_media;
  uint64_t pointer_tick;
  int64_t pointer_fraction_x, pointer_fraction_y;
  uint64_t quit_deadline;
  FILE *recording, *playing;
  uint32_t recorded;
  int playback_channels, playback_rate;
  char notice[128], clipboard[4096], input_bytes[4096];
  char status[96];
  char file_clipboard[512], bookmark[512];
  size_t input_len;
  plat_fb_t fb[2];
  VDWindow windows[VD_MAX];
  TermState *sessions[4];
  char terminal_input[4][512];
  size_t terminal_input_len[4];
} vd;

static void vd_notice(const char *s) {
  snprintf(vd.notice, sizeof(vd.notice), "%.127s", s);
  vd.dirty = true;
}
static bool vd_path_copy(char *dest, size_t size, const char *source) {
  size_t n = strlen(source);
  if (n >= size) {
    vd_notice("Path is too long");
    return false;
  }
  memcpy(dest, source, n + 1);
  return true;
}
static void vd_remote_event(VDWindow *w, const char *event) {
  char p[256];
  snprintf(p, sizeof(p), VD_BRIDGE "vnc-%u.events", w->job);
  FILE *f = fopen(p, "ab");
  if (f) {
    fprintf(f, "%s\n", event);
    fclose(f);
  }
}
static VDWindow *vd_focus(void) {
  return vd.focused >= 0 && vd.focused < VD_MAX && vd.windows[vd.focused].used
             ? &vd.windows[vd.focused]
             : NULL;
}
static void vd_px(int x, int y, uint32_t c) {
  x -= vd.pan_x;
  y -= vd.pan_y;
  int screen = y >= VD_TERM_H;
  if (screen) {
    x -= VD_PANEL_X;
    y -= VD_TERM_H;
  }
  plat_fb_t *fb = &vd.fb[screen];
  if (x < 0 || y < 0 || x >= fb->w || y >= fb->h || !fb->base)
    return;
  uint8_t *p =
      fb->base + (ptrdiff_t)x * fb->x_stride + (ptrdiff_t)y * fb->y_stride;
  PLAT_PX(p, c >> 16, c >> 8, c);
}
static void vd_rect(int x, int y, int w, int h, uint32_t c) {
  if (w <= 0 || h <= 0) return;
  if (w == 1 && h == 1) { vd_px(x, y, c); return; }
  /* Clip once per physical surface, then write contiguous rows. */
  for (int s = 0; s < 2; s++) {
    plat_fb_t *fb = &vd.fb[s];
    if (!fb->base) continue;
    int left = x - vd.pan_x - (s ? VD_PANEL_X : 0);
    int top = y - vd.pan_y - (s ? VD_TERM_H : 0);
    int right = left + w, bottom = top + h;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > fb->w) right = fb->w;
    if (bottom > fb->h) bottom = fb->h;
    if (left >= right || top >= bottom) continue;
    union { uint32_t word; uint8_t bytes[4]; } pixel = {0};
    PLAT_PX(pixel.bytes, c >> 16, c >> 8, c);
    for (int yy = top; yy < bottom; yy++) {
      uint8_t *row = fb->base + (ptrdiff_t)yy * fb->y_stride + (ptrdiff_t)left * fb->x_stride;
      if (fb->bpp == 4 && fb->x_stride == 4 && !((uintptr_t)row & 3)) {
        uint32_t *words = (uint32_t *)row;
        for (int xx = 0; xx < right - left; xx++)
          words[xx] = (words[xx] & 0xff000000u) | pixel.word;
      } else {
        for (int xx = left; xx < right; xx++, row += fb->x_stride)
          PLAT_PX(row, c >> 16, c >> 8, c);
      }
    }
  }
}
static void vd_char(int x, int y, char c, uint32_t fg, int scale) {
  if ((unsigned char)c < 32 || (unsigned char)c > 126)
    c = '?';
  const unsigned char *g = font8x8[(int)c - 32];
  for (int yy = 0; yy < 8; yy++)
    for (int xx = 0; xx < 8; xx++)
      if (g[yy] & (1 << xx))
        vd_rect(x + xx * scale, y + yy * scale, scale, scale, fg);
}
static void vd_label(int x, int y, const char *s, uint32_t color, int maxw) {
  for (; *s && maxw >= 8; s++, x += 8, maxw -= 8)
    vd_char(x, y, *s, color, 1);
}
static void vd_icon(int x, int y, int app) {
  vd_rect(x, y, 22, 19, 0x31563c);
  vd_rect(x + 2, y + 2, 18, 15, VD_ACCENT);
  if (app == VD_FILES) {
    vd_rect(x + 1, y - 3, 10, 5, VD_ACCENT);
  } else {
    vd_rect(x + 4, y + 4, 14, 11, VD_SURFACE);
    vd_char(x + 7, y + 5, app == VD_TERM ? '>' : vd_names[app][0],
            VD_TEXT_COLOR, 1);
  }
}
static void vd_hex(FILE *f, const char *s) {
  for (; *s; s++)
    fprintf(f, "%02x", (unsigned char)*s);
  fputc('\n', f);
}
static unsigned vd_request(VDWindow *w, const char *op, const char *a,
                           const char *b, const char *c, const char *d) {
  char path[256], final[256];
  unsigned id = ++vd.serial;
  snprintf(path, sizeof(path), VD_BRIDGE "%u.part", id);
  snprintf(final, sizeof(final), VD_BRIDGE "%u.req", id);
  FILE *f = fopen(path, "wb");
  if (!f) {
    vd_notice("Storage mailbox could not be opened");
    return 0;
  }
  fprintf(f, "%s\n", op);
  if (a)
    vd_hex(f, a);
  if (b)
    vd_hex(f, b);
  if (c)
    vd_hex(f, c);
  if (d)
    vd_hex(f, d);
  bool ok = !ferror(f);
  if (fclose(f))
    ok = false;
  if (!ok || rename(path, final)) {
    remove(path);
    vd_notice("Request could not be saved");
    return 0;
  }
  if (w)
    w->pending = id;
  if (w)
    snprintf(w->pending_op, sizeof(w->pending_op), "%s", op);
  if (w && w->app == VD_FILES && !strcmp(op, "list"))
    snprintf(w->title, sizeof(w->title), "Files: %.50s", a ? a : "");
  vd_notice("Working...");
  return id;
}
static void vd_save_preferences(void) {
  if (vd.recovery)
    return;
  char temp[256];
  snprintf(temp, sizeof(temp), PLAT_SD "verdant/preferences.part");
  FILE *f = fopen(temp, "wb");
  if (!f)
    return;
  fprintf(f,
          "joined=%d\ntouchpad=%d\nwallpaper=%d\nworkspace=%d\nbookmark=%s\nui_scale=%d\n",
          vd.joined, vd.touchpad, vd.wallpaper, vd.workspace, vd.bookmark, VD_SCALE);
  if (!fclose(f))
    rename(temp, PLAT_SD "verdant/preferences.cfg");
  cfg_save(&g_cfg);
}
static void vd_terminal_preferences(TermState *t) {
  const AdaPalette *p = ada_palette(g_cfg.theme);
  uint32_t colors[16];
  ada_ansi16(p, g_cfg.theme, colors);
  term_set_palette(t, colors, p->text, g_cfg.top_black ? 0 : p->base);
  t->use_5x7 = g_cfg.use_5x7;
  t->zoom_x = g_cfg.zoom_x;
  t->zoom_y = g_cfg.zoom_y;
}
static int vd_new(int app) {
  if(app==VD_UPDATER) for(int i=0;i<VD_MAX;i++) {
    VDWindow *w=&vd.windows[i];
    if(w->used && w->app==VD_UPDATER) {
      w->minimized=false;w->workspace=vd.workspace;vd.focused=i;vd.menu=false;vd.dirty=true;return i;
    }
  }
  int slot = -1;
  for (int i = 0; i < VD_MAX; i++)
    if (!vd.windows[i].used) {
      slot = i;
      break;
    }
  if (slot < 0) {
    vd_notice("Close a window first");
    return -1;
  }
  VDWindow *w = &vd.windows[slot];
  memset(w, 0, sizeof(*w));
  w->used = true;
  w->app = app;
  w->workspace = vd.workspace;
  w->session = -1;
  w->w = VD_W > 500 ? 500 : 300;
  w->h = 190;
  if (app == VD_SETTINGS || app == VD_TASKS) { w->w = VD_W - 24; w->h = VD_H - 58; }
  w->x = VD_PANEL_X + 6 + (slot % 3) * 6;
  w->y = 28 + (slot % 3) * 8;
  snprintf(w->title, sizeof(w->title), "%s", vd_names[app]);
  vd.focused = slot;
  vd.has_selection = false;
  vd.menu = false;
  vd.dirty = true;
  strcpy(w->path, PLAT_GUEST_STORAGE);
  if (app == VD_TERM) {
    bool taken[4] = {false};
    for (int i = 0; i < VD_MAX; i++)
      if (vd.windows[i].used && vd.windows[i].app == VD_TERM &&
          vd.windows[i].session >= 0 && vd.windows[i].session < 4)
        taken[vd.windows[i].session] = true;
    for (int s = 0; s < 4; s++)
      if (!taken[s] && vd.sessions[s]) {
        w->session = s;
        w->terminal = vd.sessions[s];
        term_init(w->terminal);
        vd_terminal_preferences(w->terminal);
        char sid[12];
        snprintf(sid, sizeof(sid), "%d", s);
        char in[256];
        snprintf(in, sizeof(in), VD_BRIDGE "term-%d.in", s);
        remove(in);
        snprintf(in, sizeof(in), VD_BRIDGE "term-%d.out", s);
        remove(in);
        vd_request(w, "terminal", sid, NULL, NULL, NULL);
        break;
      }
    if (w->session < 0) {
      strcpy(w->text, "Four terminals maximum. Close one to start another.");
    }
  } else if (app == VD_FILES)
    vd_request(w, "list", w->path, NULL, NULL, NULL);
  else if (app == VD_TASKS)
    vd_request(w, "performance", NULL, NULL, NULL, NULL);
  else if (app == VD_EDIT) {
    strcpy(w->path, "/root/notes.txt");
    strcpy(w->text, "");
  } else if (app == VD_CALC)
    strcpy(w->text, "Enter an expression, then press Enter.\nSupports + - * / "
                    "and parentheses.");
  else if (app == VD_SSH) {
    strcpy(w->input, "user@192.168.1.10");
    w->path[0] = 0;
    strcpy(w->text,
           "Enter user@host. Connect opens a real SSH terminal.\nKey selects a "
           "guest "
           "private key (optional).\nSave keeps this connection "
           "profile.\nVerify the host "
           "key in the terminal.\nSCP transfers work in any terminal.");
    FILE *f = fopen(PLAT_SD "verdant/ssh-profile.cfg", "rb");
    if (f) {
      if (!fgets(w->input, sizeof(w->input), f))
        strcpy(w->input, "user@192.168.1.10");
      w->input[strcspn(w->input, "\r\n")] = 0;
      if (fgets(w->path, sizeof(w->path), f))
        w->path[strcspn(w->path, "\r\n")] = 0;
      fclose(f);
    }
  } else if (app == VD_DOWNLOAD) {
    strcpy(w->path, "/root/download.bin");
    strcpy(w->text, "Enter an HTTP(S) URL. Destination can be changed\nwith "
                    "Path. Active jobs "
                    "continue in the background.");
  } else if (app == VD_MEDIA)
    strcpy(w->text, "Camera and audio controls use available "
                    "hardware.\nImages: open supported "
                    "files through Files.\nRec/Stop saves a WAV; Play plays "
                    "the recording.");
  else if (app == VD_PACKAGES)
    strcpy(w->text, "Open the package terminal to inspect opkg.\nOnly "
                    "compatible RV32 ILP32 "
                    "packages can run.\nA repository must be configured before "
                    "updates.\nWindows "
                    "winget does not run on this Linux guest.");
  else if (app == VD_REMOTE) {
    strcpy(w->input, "192.168.1.10:5900");
    w->path[0] = 0;
    strcpy(w->text,
           "VNC desktop for Linux or Windows servers.\nSet server resolution "
           "to 640 x 480 "
           "or smaller.\nClassic VNC password supported; no TLS.\nUse a "
           "private LAN or "
           "SSH tunnel.\nTouch image to send left-click; keyboard types.");
  } else if (app == VD_BACKUP)
    strcpy(w->text, "Back up /root documents using the button below.\nFull "
                    "disk-image backups must "
                    "be taken offline.\nUse tools/manage_install.py on your "
                    "computer.\nRecovery: "
                    "rename the preferences file while closed.");
  else if (app == VD_UPDATER) {
    strcpy(w->text,
           "GitHub system updater\nCheck: latest stable release\nUpdate: "
           "download, verify and stage\nAuto: download on each launch when "
           "newer\nApply on relaunch; user files are preserved.");
  }
  return slot;
}
static void vd_close(int idx) {
  VDWindow *w = &vd.windows[idx];
  if (w->app == VD_REMOTE && w->job) {
    char id[16];
    snprintf(id, sizeof(id), "%u", w->job);
    vd_request(NULL, "cancel", id, NULL, NULL, NULL);
  }
  if (w->session >= 0) {
    char sid[12];
    snprintf(sid, sizeof(sid), "%d", w->session);
    vd_request(NULL, "close", sid, NULL, NULL, NULL);
  }
  stbi_image_free(w->image);
  w->image = NULL;
  w->used = false;
  if (vd.focused == idx)
    vd.focused = -1;
  vd.dirty = true;
}
static void vd_move_screen(VDWindow *w) {
  if (!w)
    return;
  bool lower = w->y < VD_TERM_H;
  w->y = lower ? VD_TERM_H + 4 : 26;
  int max_height = (lower ? VD_PANEL_H : VD_TERM_H) - 30;
  if (w->h > max_height)
    w->h = max_height;
  if (lower && w->w > VD_PANEL_W)
    w->w = VD_PANEL_W;
  w->maximized = false;
  if (w->x < VD_PANEL_X)
    w->x = VD_PANEL_X;
  if (w->x + w->w > VD_PANEL_X + VD_PANEL_W)
    w->x = VD_PANEL_X + VD_PANEL_W - w->w;
  vd.dirty = true;
}
static void vd_cycle(void) {
  for (int n = 1; n <= VD_MAX; n++) {
    int i = (vd.focused + n + VD_MAX) % VD_MAX;
    if (vd.windows[i].used && vd.windows[i].workspace == vd.workspace) {
      vd.focused = i;
      vd.windows[i].minimized = false;
      break;
    }
  }
  vd.dirty = true;
}
static const char *vd_expr;
static bool vd_math_error;
static double vd_sum(void);
static double vd_factor(void) {
  while (isspace((unsigned char)*vd_expr))
    vd_expr++;
  if (*vd_expr == '(') {
    vd_expr++;
    double v = vd_sum();
    if (*vd_expr == ')')
      vd_expr++;
    else
      vd_math_error = true;
    return v;
  }
  char *end;
  double v = strtod(vd_expr, &end);
  if (end == vd_expr) {
    vd_math_error = true;
    return 0;
  }
  vd_expr = end;
  return v;
}
static double vd_product(void) {
  double v = vd_factor();
  for (;;) {
    while (isspace((unsigned char)*vd_expr))
      vd_expr++;
    char c = *vd_expr;
    if (c != '*' && c != '/')
      break;
    vd_expr++;
    double r = vd_factor();
    if (c == '/') {
      if (r == 0)
        vd_math_error = true;
      else
        v /= r;
    } else
      v *= r;
  }
  return v;
}
static double vd_sum(void) {
  double v = vd_product();
  for (;;) {
    while (isspace((unsigned char)*vd_expr))
      vd_expr++;
    char c = *vd_expr;
    if (c != '+' && c != '-')
      break;
    vd_expr++;
    double r = vd_product();
    v += c == '+' ? r : -r;
  }
  return v;
}

static void vd_selected_path(VDWindow *w, char *out, size_t n);
static bool vd_image_extension(const char *p) {
  const char *e = strrchr(p, '.');
  return e && (!strcasecmp(e, ".png") || !strcasecmp(e, ".jpg") ||
               !strcasecmp(e, ".jpeg") || !strcasecmp(e, ".bmp"));
}
static void vd_load_image(VDWindow *w, const char *path) {
  int x, y, n;
  if (!stbi_info(path, &x, &y, &n) || x > 640 || y > 480 || x < 1 || y < 1) {
    vd_notice("Use an image of at most 640 x 480");
    return;
  }
  unsigned char *p = stbi_load(path, &x, &y, &n, 3);
  if (!p) {
    vd_notice("Image could not be decoded");
    return;
  }
  stbi_image_free(w->image);
  w->image = p;
  w->image_w = x;
  w->image_h = y;
  vd.dirty = true;
}
static bool desktop_input_byte(char c) {
  if (!vd.active)
    return false;
  VDWindow *w = vd_focus();
  if (!w)
    return true;
  if (w->app == VD_REMOTE && w->job && !w->entry_mode) {
    char event[32];
    snprintf(event, sizeof(event), "key %u", (unsigned char)c);
    vd_remote_event(w, event);
    return true;
  }
  if (w->terminal && w->session < 0)
    return false; /* Linux boot console */
  if (w->terminal && w->session >= 0) {
    size_t *n = &vd.terminal_input_len[w->session];
    if (*n < sizeof(vd.terminal_input[0]))
      vd.terminal_input[w->session][(*n)++] = c;
    vd.dirty = true;
    return true;
  }
  char *s = w->app == VD_EDIT && !w->entry_mode ? w->text : w->input;
  size_t max = s == w->text ? sizeof(w->text) : sizeof(w->input), n = strlen(s);
  if (s == w->text) {
    if (w->cursor < 0 || w->cursor > (int)n)
      w->cursor = n;
    if (c == 19) {
      vd_request(w, "write", w->path, w->text, NULL, NULL);
      return true;
    }
    if (c == 3) {
      snprintf(vd.clipboard, sizeof(vd.clipboard), "%.4095s", w->text);
      vd_notice("Copied editor text");
      return true;
    }
    if (c == 22) {
      size_t len = strlen(vd.clipboard);
      if (n + len < max) {
        memmove(s + w->cursor + len, s + w->cursor, n - w->cursor + 1);
        memcpy(s + w->cursor, vd.clipboard, len);
        w->cursor += len;
      }
      vd.dirty = true;
      return true;
    }
    if (c == 127 || c == 8) {
      if (w->cursor) {
        memmove(s + w->cursor - 1, s + w->cursor, n - w->cursor + 1);
        w->cursor--;
      }
    } else if (c == '\r' || c == '\n' || (unsigned char)c >= 32) {
      if (n + 1 < max) {
        memmove(s + w->cursor + 1, s + w->cursor, n - w->cursor + 1);
        s[w->cursor++] = c == '\r' ? '\n' : c;
      }
    }
    vd.dirty = true;
    return true;
  }
  if (c == 127 || c == 8) {
    if (n)
      s[n - 1] = 0;
  } else if (c == '\r' || c == '\n') {
    if (w->entry_mode) {
      if (strlen(s) >= sizeof(w->path)) {
        vd_notice("Path too long");
        return true;
      }
      if (w->entry_mode == 1 || w->entry_mode == 6) {
        strcpy(w->path, s);
        if (w->app == VD_FILES)
          vd_request(w, "list", w->path, NULL, NULL, NULL);
      } else if (w->entry_mode == 2 || w->entry_mode == 3) {
        char source[1024];
        vd_selected_path(w, source, sizeof(source));
        if (source[0])
          vd_request(w, w->entry_mode == 2 ? "move" : "copy", source, s, NULL,
                     NULL);
      } else if (w->entry_mode == 4)
        vd_request(w, "search", w->path, s, NULL, NULL);
      else if (w->entry_mode == 5)
        vd_request(w, "mkdir", s, NULL, NULL, NULL);
      int mode = w->entry_mode;
      w->entry_mode = 0;
      s[0] = 0;
      if (mode == 6)
        strcpy(w->input, w->previous_input);
      vd.keyboard = false;
    } else if (w->app == VD_EDIT) {
      if (n + 1 < max) {
        s[n++] = '\n';
        s[n] = 0;
      }
    } else if (w->app == VD_CALC) {
      vd_expr = s;
      vd_math_error = false;
      double answer = vd_sum();
      while (isspace((unsigned char)*vd_expr))
        vd_expr++;
      if (*vd_expr || vd_math_error || !isfinite(answer))
        strcpy(w->text, "Invalid expression");
      else {
        char result[2304];
        snprintf(result, sizeof(result), "%s\n= %.10g", w->input, answer);
        memcpy(w->text, result, strlen(result) + 1);
      }
    } else if (w->app == VD_FILES && s[0]) {
      if (n < sizeof(w->path)) {
        strcpy(w->path, s);
        vd_request(w, "list", w->path, NULL, NULL, NULL);
      } else
        vd_notice("Path too long");
      s[0] = 0;
    }
  } else if ((unsigned char)c >= 32 && n + 1 < max) {
    s[n++] = c;
    s[n] = 0;
  }
  vd.dirty = true;
  return true;
}
static void vd_navigation(int k) {
  VDWindow *w = vd_focus();
  if (!w)
    return;
  if (w->app == VD_EDIT && !w->entry_mode) {
    int n = strlen(w->text), pos = w->cursor;
    if (pos < 0 || pos > n)
      pos = n;
    if (k == 0 && pos > 0)
      pos--;
    else if (k == 3 && pos < n)
      pos++;
    else if (k == 4) {
      while (pos > 0 && w->text[pos - 1] != '\n')
        pos--;
    } else if (k == 5) {
      while (pos < n && w->text[pos] != '\n')
        pos++;
    } else if (k == 1 || k == 2) {
      int start = pos;
      while (start > 0 && w->text[start - 1] != '\n')
        start--;
      int col = pos - start;
      if (k == 2 && start > 0) {
        int end = start - 1;
        start = end;
        while (start > 0 && w->text[start - 1] != '\n')
          start--;
        pos = start + col;
        if (pos > end)
          pos = end;
      } else if (k == 1) {
        int end = pos;
        while (end < n && w->text[end] != '\n')
          end++;
        if (end < n) {
          start = end + 1;
          end = start;
          while (end < n && w->text[end] != '\n')
            end++;
          pos = start + col;
          if (pos > end)
            pos = end;
        }
      }
    } else if (k == 6) {
      if (w->scroll > 0)
        w->scroll--;
    } else if (k == 7)
      w->scroll++;
    w->cursor = pos;
    vd.dirty = true;
    return;
  }
  if (w->app == VD_REMOTE && w->job) {
    unsigned keys[] = {0xff51, 0xff54, 0xff52, 0xff53,
                       0xff50, 0xff57, 0xff55, 0xff56};
    char e[32];
    snprintf(e, sizeof(e), "special %u", keys[k]);
    vd_remote_event(w, e);
    return;
  }
  const char *seq[] = {"\033[D", "\033[B", "\033[A",  "\033[C",
                       "\033[H", "\033[F", "\033[5~", "\033[6~"};
  rx_push_str(seq[k]);
}
static struct {
#ifdef PLAT_VITA
  plat_performance_t native;
#endif
  float guest_cpu, rx, tx, read, write;
  unsigned long memory_total, memory_used;
  unsigned long long disk_total,disk_free;
  int count, history_at, history_count;
  int history[7][60];
} vd_performance;
static void vd_performance_parse(const char *payload) {
  const char *p=payload;
  while(*p) {
    if(!strncmp(p,"CPU|",4)) sscanf(p+4,"%f",&vd_performance.guest_cpu);
    else if(!strncmp(p,"MEM|",4)) sscanf(p+4,"%lu|%lu",&vd_performance.memory_total,&vd_performance.memory_used);
    else if(!strncmp(p,"NET|",4)) sscanf(p+4,"%f|%f",&vd_performance.rx,&vd_performance.tx);
    else if(!strncmp(p,"DISK|",5)) sscanf(p+5,"%f|%f|%llu|%llu",&vd_performance.read,&vd_performance.write,&vd_performance.disk_total,&vd_performance.disk_free);
    else if(!strncmp(p,"PROCESSES|",10)) sscanf(p+10,"%d",&vd_performance.count);
    p=strchr(p,'\n'); if(!p) break; p++;
  }
  int n=vd_performance.history_at;
  for(int c=0;c<4;c++) {
#ifdef PLAT_VITA
    vd_performance.history[c][n]=vd_performance.native.cpu_valid ? vd_performance.native.cores[c] : -1;
#else
    vd_performance.history[c][n]=-1;
#endif
  }
  vd_performance.history[4][n]=(int)vd_performance.guest_cpu;
  vd_performance.history[5][n]=vd_performance.memory_total ? vd_performance.memory_used*100/vd_performance.memory_total : -1;
  vd_performance.history[6][n]=(int)(vd_performance.rx+vd_performance.tx);
  vd_performance.history_at=(n+1)%60;
  if(vd_performance.history_count<60) vd_performance.history_count++;
}
static void vd_poll_bridge(void) {
  uint64_t now = plat_us();
  if (now - vd.last_poll < 100000)
    return;
  vd.last_poll = now;
  for (int s = 0; s < 4; s++)
    if (vd.terminal_input_len[s]) {
      char p[256];
      snprintf(p, sizeof(p), VD_BRIDGE "term-%d.in", s);
      FILE *f = fopen(p, "ab");
      if (f) {
        fwrite(vd.terminal_input[s], 1, vd.terminal_input_len[s], f);
        fclose(f);
        vd.terminal_input_len[s] = 0;
      }
    }
  for (int i = 0; i < VD_MAX; i++) {
    VDWindow *w = &vd.windows[i];
    if (!w->used)
      continue;
    char p[256];
    if (w->pending) {
      snprintf(p, sizeof(p), VD_BRIDGE "%u.res", w->pending);
      FILE *f = fopen(p, "rb");
      if (f) {
        char result[VD_TEXT];
        size_t n = fread(result, 1, sizeof(result) - 1, f);
        result[n] = 0;
        fclose(f);
        remove(p);
        bool ok = !strncmp(result, "OK\n", 3);
        char *payload = result + (ok                               ? 3
                                  : !strncmp(result, "ERROR\n", 6) ? 6
                                                                   : 0);
        unsigned completed = w->pending;
        w->pending = 0;
        if (!ok)
          vd_notice(payload);
        else if (!strcmp(w->pending_op, "download") ||
                 !strcmp(w->pending_op, "vnc") ||
                 !strcmp(w->pending_op, "sysupdate")) {
          w->job = completed;
          snprintf(w->text, sizeof(w->text), "%s", payload);
          vd_notice("Started");
        } else if (!strcmp(w->pending_op, "image")) {
          char image_path[256];
          snprintf(image_path, sizeof(image_path), VD_BRIDGE "image-%u.bin",
                   completed);
          vd_load_image(w, image_path);
          remove(image_path);
        } else if (!strcmp(w->pending_op, "list") ||
                   !strcmp(w->pending_op, "read") ||
                   !strcmp(w->pending_op, "tasks") ||
                   !strcmp(w->pending_op, "performance") ||
                   !strcmp(w->pending_op, "search") ||
                   !strcmp(w->pending_op, "job")) {
          snprintf(w->text, sizeof(w->text), "%s", payload);
          vd_notice("Complete");
          if (!strcmp(w->pending_op, "performance")) vd_performance_parse(payload);
          if (!strcmp(w->pending_op, "read"))
            w->cursor = strlen(w->text);
          if (!strcmp(w->pending_op, "job") && strncmp(payload, "running", 7))
            w->job = 0;
        } else {
          vd_notice(payload);
          if (w->app == VD_FILES)
            vd_request(w, "list", w->path, NULL, NULL, NULL);
        }
        vd.dirty = true;
      }
    }
    if (w->job && !w->pending && now - vd.last_job > 1000000) {
      char jid[16];
      snprintf(jid, sizeof(jid), "%u", w->job);
      vd_request(w, "job", jid, NULL, NULL, NULL);
      vd.last_job = now;
      if (w->app == VD_REMOTE) {
        char frame[256];
        snprintf(frame, sizeof(frame), VD_BRIDGE "vnc-%u.ppm", w->job);
        FILE *f = fopen(frame, "rb");
        if (f) {
          fclose(f);
          vd_load_image(w, frame);
        }
      }
    }
    if (w->terminal && w->session >= 0) {
      snprintf(p, sizeof(p), VD_BRIDGE "term-%d.out", w->session);
      FILE *f = fopen(p, "rb");
      if (f) {
        fseek(f, w->output_offset, SEEK_SET);
        char b[4096];
        size_t n = fread(b, 1, sizeof(b), f);
        fclose(f);
        w->output_offset += n;
        for (size_t j = 0; j < n; j++)
          term_write_char(w->terminal, b[j]);
        if (n)
          vd.dirty = true;
      }
    }
  }
}
static void vd_button(int x, int y, int width, const char *label) {
  vd_rect(x, y, width, 15, 0x31563c);
  vd_label(x + 3, y + 4, label, VD_TEXT_COLOR, width - 5);
}
static void vd_lines(VDWindow *w, int x, int y, int width, int height) {
  const char *s = w->text;
  int row = 0, skip = w->scroll;
  while (*s && skip)
    if (*s++ == '\n')
      skip--;
  int cols = width / 8;
  if (cols < 1)
    return;
  while (*s && row * 11 + 8 < height) {
    int col = 0;
    if (w->app == VD_FILES && row + w->scroll == w->selection)
      vd_rect(x, y + row * 11, width, 11, 0x375940);
    while (*s && *s != '\n') {
      if (col < cols) {
        vd_char(x + col * 8, y + row * 11, *s, VD_TEXT_COLOR, 1);
        if (w->app == VD_EDIT && s - w->text == w->cursor)
          vd_rect(x + col * 8, y + row * 11, 1, 9, VD_ACCENT);
      }
      col++;
      s++;
    }
    if (w->app == VD_EDIT && s - w->text == w->cursor && col < cols)
      vd_rect(x + col * 8, y + row * 11, 1, 9, VD_ACCENT);
    if (*s)
      s++;
    row++;
  }
}
static void vd_terminal_draw(VDWindow *w, int x, int y, int width, int height) {
  TermState *t = w->terminal;
  if (!t)
    return;
  int z = t->zoom_x, fw = t->use_5x7 ? 5 : 8, cw = fw * z, ch = 8 * z;
  int cols = width / cw, rows = height / ch;
  if (cols > TERM_COLS)
    cols = TERM_COLS;
  if (rows > TERM_ROWS)
    rows = TERM_ROWS;
  /* Copy under the guest lock; rasterization must not block UART output. */
  static TermState snapshot;
  plat_mutex_lock(&ui_lock);
  if (t->auto_track) {
    if (t->cx >= t->scroll_x + cols)
      t->scroll_x = t->cx - cols + 1;
    if (t->cy >= t->scroll_y + rows)
      t->scroll_y = t->cy - rows + 1;
  }
  if (t->scroll_x < 0)
    t->scroll_x = 0;
  if (t->scroll_y < -t->sb_count)
    t->scroll_y = -t->sb_count;
  memcpy(&snapshot, t, sizeof(snapshot));
  t->dirty = false;
  plat_mutex_unlock(&ui_lock);
  t = &snapshot;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++) {
      int gx = c + t->scroll_x, gy = r + t->scroll_y;
      if (gx >= TERM_COLS || gy >= TERM_ROWS)
        continue;
      TermCell cell;
      if (gy < 0) {
        int slot =
            (t->sb_head + t->sb_count + gy + TERM_SCROLLBACK) % TERM_SCROLLBACK;
        cell = t->sb_buf[slot][gx];
      } else
        cell = t->grid[gy * TERM_COLS + gx];
      bool reverse = cell.flags & TERM_FLAG_REVERSE;
      if (gy == t->cy && gx == t->cx && t->cursor_visible && term_blink_on())
        reverse = !reverse;
      uint32_t fg = reverse ? cell.bg : cell.fg,
               bg = reverse ? cell.fg : cell.bg;
      int index = gy * TERM_COLS + gx;
      int sel_min =
          vd.select_start < vd.select_end ? vd.select_start : vd.select_end;
      int sel_max =
          vd.select_start > vd.select_end ? vd.select_start : vd.select_end;
      if (vd.focused == (int)(w - vd.windows) && vd.has_selection &&
          index >= sel_min && index <= sel_max)
        bg = 0x375940;
      vd_rect(x + c * cw, y + r * ch, cw, ch, bg);
      unsigned char uc = cell.c;
      if (uc < 32 || uc > 126)
        continue;
      const unsigned char *glyph =
          t->use_5x7 ? &font5x7[uc * 5] : font8x8[uc - 32];
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < fw; xx++)
          if (t->use_5x7 ? (glyph[xx] & (1 << yy)) : (glyph[yy] & (1 << xx)))
            vd_rect(x + c * cw + xx * z, y + r * ch + yy * z, z, z, fg);
    }
}
/* Settings cards share their geometry with hit testing. */
static void vd_change_scale(int delta) {
#ifdef PLAT_VITA
  int value=VD_SCALE+delta;
  if(value<100)value=100;
  if(value>200)value=200;
  vd.ui_scale=value;
  vd.pan_x=vd.pan_y=0;
  if(vd.px>=VD_W)vd.px=VD_W-1;
  if(vd.py>=VD_H)vd.py=VD_H-1;
  for(int i=0;i<VD_MAX;i++) {
    VDWindow *w=&vd.windows[i]; if(!w->used)continue;
    if(w->app==VD_SETTINGS || w->app==VD_TASKS) { w->w=VD_W-24;w->h=VD_H-58;w->x=12;w->y=28; }
    if(w->w>VD_W)w->w=VD_W;
    if(w->h>VD_H-24)w->h=VD_H-24;
    if(w->x+w->w>VD_W)w->x=VD_W-w->w;
    if(w->y+w->h>VD_H-22)w->y=VD_H-22-w->h;
  }
  vd_save_preferences(); vd.dirty=true;
#else
  vd_notice("Global scaling is available on Vita; use terminal font controls here");
#endif
}
static void vd_settings_draw(VDWindow *w,int x,int y,int width,int height) {
  char size[64];snprintf(size,sizeof(size),"Display size: %d%%",VD_SCALE);
  vd_rect(x,y,width,36,0x243e30);
  vd_label(x+8,y+7,size,VD_TEXT_COLOR,width-110);
  vd_label(x+8,y+21,"Text, windows, keyboard & taskbar",VD_ACCENT,width-110);
  vd_rect(x+width-96,y+4,40,28,0x477647);vd_label(x+width-80,y+14,"-",VD_TEXT_COLOR,16);
  vd_rect(x+width-48,y+4,40,28,0x477647);vd_label(x+width-32,y+14,"+",VD_TEXT_COLOR,16);
  const char *names[]={"Desktop layout","Pointer input","Appearance","Terminal font","System update","Save preferences"};
  char states[6][64];
  snprintf(states[0],64,"%s",vd.joined?"Joined desktop":"Separate regions");
  snprintf(states[1],64,"%s",vd.touchpad?"Touchpad":"Direct touch");
  snprintf(states[2],64,"Green wallpaper %d",vd.wallpaper+1);
  snprintf(states[3],64,"%s",g_cfg.use_5x7?"Compact":"Normal");
  snprintf(states[4],64,"GitHub / " VU_VERSION);snprintf(states[5],64,"Saved automatically");
  int cell=(width-8)/2,row=(height-44)/3;
  for(int i=0;i<6;i++) {
    int xx=x+(i%2)*(cell+8),yy=y+44+(i/2)*row;
    vd_rect(xx,yy,cell,row-5,0x243e30);
    vd_icon(xx+7,yy+7,i==4?VD_UPDATER:VD_SETTINGS);
    vd_label(xx+37,yy+7,names[i],VD_TEXT_COLOR,cell-42);
    vd_label(xx+7,yy+27,states[i],VD_ACCENT,cell-14);
  }
}
static void vd_settings_click(VDWindow *w,int x,int y) {
  int rx=x-w->x-5,ry=y-w->y-21,width=w->w-10,height=w->h-45;
  if(ry>=0 && ry<36) {
    if(rx>=width-96 && rx<width-56)vd_change_scale(-25);
    else if(rx>=width-48 && rx<width-8)vd_change_scale(25);
    return;
  }
  int row=(height-44)/3,cell=(width-8)/2;
  if(ry<44 || row<1 || rx<0)return;
  int col=rx/(cell+8),r=(ry-44)/row;if(col>1 || r>2)return;
  switch(r*2+col) {
    case 0:vd.joined=!vd.joined;break;
    case 1:vd.touchpad=!vd.touchpad;break;
    case 2:vd.wallpaper=(vd.wallpaper+1)%3;break;
    case 3:g_cfg.use_5x7=!g_cfg.use_5x7;
      for(int s=0;s<4;s++)if(vd.sessions[s])vd.sessions[s]->use_5x7=g_cfg.use_5x7;
      break;
    case 4:vd_new(VD_UPDATER);break;
    case 5:vd_notice("Preferences saved");break;
  }
  vd_save_preferences();vd.dirty=true;
}
static void vd_graph(int x,int y,int width,int height,int series,const char *label,int maximum) {
  vd_rect(x,y,width,height,0x10241d);
  vd_label(x+4,y+4,label,VD_ACCENT,width-8);
  for(int i=1;i<4;i++)vd_rect(x,y+14+(height-15)*i/4,width,1,0x243e30);
  int count=vd_performance.history_count;
  int first=(vd_performance.history_at-count+60)%60;
  for(int i=0;i<count;i++) {
    int v=vd_performance.history[series][(first+i)%60];if(v<0)continue;
    if(v>maximum)v=maximum;
    int xx=x+2+i*(width-4)/60, yy=y+height-2-v*(height-18)/maximum;
    vd_rect(xx,yy,2,y+height-1-yy,VD_ACCENT);
  }
}
static void vd_tasks_draw(VDWindow *w,int x,int y,int width,int height) {
  vd_rect(x,y,110,24,w->task_tab?0x243e30:0x477647);
  vd_label(x+8,y+8,"Processes",VD_TEXT_COLOR,100);
  vd_rect(x+116,y,122,24,w->task_tab?0x477647:0x243e30);
  vd_label(x+124,y+8,"Performance",VD_TEXT_COLOR,110);
  if(!w->task_tab) {
    vd_label(x,y+32,"PID    CPU %    RAM KiB   Process (Linux)",VD_ACCENT,width);
    const char *p=w->text;int row=0,drawn=0;
    while(p && *p && drawn<(height-48)/14) {
      if(!strncmp(p,"PROC|",5)) {
        int pid;float cpu;unsigned long rss;char name[80];
        if(sscanf(p+5,"%d|%f|%lu|%79[^\n]",&pid,&cpu,&rss,name)==4 && row++>=w->scroll) {
          char line[160],usage[16];
          if(cpu<0)strcpy(usage," --");else snprintf(usage,16,"%5.1f",cpu);
          snprintf(line,sizeof(line),"%-6d %s %9lu   %.48s",pid,usage,rss,name);
          if(drawn%2==0)vd_rect(x,y+48+drawn*14,width,14,0x20372a);
          vd_label(x,y+51+drawn*14,line,VD_TEXT_COLOR,width);drawn++;
        }
      }
      p=strchr(p,'\n');if(p)p++;
    }
    return;
  }
  const char *categories[]={"CPU","Memory","GPU","Wi-Fi","Storage"};
  for(int i=0;i<5;i++) {
    vd_rect(x,y+32+i*30,96,26,i==w->task_category?0x477647:0x243e30);
    vd_label(x+8,y+41+i*30,categories[i],VD_TEXT_COLOR,84);
  }
  int xx=x+106,yy=y+32,ww=width-106;char info[2048];info[0]=0;
#ifdef PLAT_VITA
  plat_performance_t *n=&vd_performance.native;
  if(w->task_category==0) {
    int gw=(ww-6)/2,gh=(height-105)/2;if(gh<24)gh=24;
    for(int i=0;i<4;i++) {
      char title[48];
      if(n->cpu_valid)snprintf(title,48,"Core %d: %d%%%s",i,n->cores[i],i==3?" sys":"");
      else snprintf(title,48,"Core %d: unavailable",i);
      vd_graph(xx+(i%2)*(gw+6),yy+(i/2)*(gh+4),gw,gh,i,title,100);
    }
    yy+=2*(gh+4);
    snprintf(info,sizeof(info),"Vita ARM Cortex-A9 / %d MHz current\n3 application cores; core 3 system\nGuest: RV32 Linux / 1 virtual CPU\nGuest utilization: %.1f%%\nBase speed: not exposed by API",n->cpu_mhz,vd_performance.guest_cpu);
  } else if(w->task_category==1) {
    vd_graph(xx,yy,ww,60,5,"Linux RAM used (%)",100);yy+=68;
    snprintf(info,sizeof(info),"Linux RAM: %lu / %lu MiB used\nApp heap: %.1f / %.1f MiB\nVita free user pool: %.1f MiB\nVita free CDRAM: %.1f MiB\nMemory type/clock: unavailable",
      vd_performance.memory_used/1024,vd_performance.memory_total/1024,n->heap_used/1048576.,n->heap_total/1048576.,n->free_user/1048576.,n->free_cdram/1048576.);
    if(!n->memory_valid)strcat(info,"\nNative free pool query unavailable");
  } else if(w->task_category==2) {
    snprintf(info,sizeof(info),"Vita GPU / %d MHz current\nDesktop renderer: CPU software\nGPU utilization: unavailable\nGPU memory usage: unavailable\nNo estimated/fabricated readings",n->gpu_mhz);
  } else if(w->task_category==3) {
    vd_graph(xx,yy,ww,60,6,"Guest network (KiB/s)",1024);yy+=68;
    snprintf(info,sizeof(info),"Vita Wi-Fi: %s\nSignal: %d%% (-1 unavailable)\nGuest RX: %.1f KiB/s\nGuest TX: %.1f KiB/s\nPhysical link speed: unavailable\nTraffic excludes native OS/apps",n->wifi_state==3?"connected":"disconnected / unavailable",n->signal,vd_performance.rx,vd_performance.tx);
  } else {
    snprintf(info,sizeof(info),"Vita ux0: %.1f / %.1f GiB free\nLinux disk: %.1f / %.1f MiB free\nGuest read: %.1f KiB/s\nGuest write: %.1f KiB/s\nPhysical device speed: unavailable\nLinux filesystem: ext4",
      n->storage_free/1073741824.,n->storage_total/1073741824.,vd_performance.disk_free/1048576.,vd_performance.disk_total/1048576.,vd_performance.read,vd_performance.write);
  }
#else
  snprintf(info,sizeof(info),"Linux CPU: %.1f%%\nLinux RAM: %lu / %lu MiB\nRX/TX: %.1f / %.1f KiB/s\nNative metrics unavailable",vd_performance.guest_cpu,vd_performance.memory_used/1024,vd_performance.memory_total/1024,vd_performance.rx,vd_performance.tx);
#endif
  const char *p=info;int line=0;
  while(p && *p && yy+line*14+8<y+height) {
    char b[180];size_t len=strcspn(p,"\n");if(len>=sizeof(b))len=sizeof(b)-1;
    memcpy(b,p,len);b[len]=0;vd_label(xx,yy+line*14,b,VD_TEXT_COLOR,ww);line++;
    p=strchr(p,'\n');if(p)p++;
  }
}

static void vd_render_window(int i) {
  VDWindow *w = &vd.windows[i];
  if (!w->used || w->minimized || w->workspace != vd.workspace)
    return;
  vd_rect(w->x + 3, w->y + 3, w->w, w->h, 0x08130e);
  vd_rect(w->x, w->y, w->w, w->h, VD_SURFACE);
  vd_rect(w->x, w->y, w->w, 17, i == vd.focused ? 0x477647 : 0x2b4135);
  vd_label(w->x + 5, w->y + 5, w->title, VD_TEXT_COLOR, w->w - 76);
  vd_label(w->x + w->w - 68, w->y + 5, "v _ [] X", VD_TEXT_COLOR, 64);
  int x = w->x + 5, y = w->y + 21, width = w->w - 10,
      height = w->h - (w->app == VD_FILES ? 63 : 45);
  if (w->terminal)
    vd_terminal_draw(w, x, y, width, height);
  else if (w->app == VD_SETTINGS) {
    vd_settings_draw(w,x,y,width,height);
  } else if (w->app == VD_TASKS) {
    vd_tasks_draw(w,x,y,width,height);
  } else if (w->image) {
    int dw = width, dh = w->image_h * width / w->image_w;
    if (dh > height) {
      dh = height;
      dw = w->image_w * height / w->image_h;
    }
    for (int yy = 0; yy < dh; yy++)
      for (int xx = 0; xx < dw; xx++) {
        unsigned char *p = w->image + 3 * ((yy * w->image_h / dh) * w->image_w +
                                           xx * w->image_w / dw);
        vd_px(x + xx, y + yy, (p[0] << 16) | (p[1] << 8) | p[2]);
      }
  } else
    vd_lines(w, x, y, width, height);
  int by = w->y + w->h - 18;
  if (w->app == VD_FILES) {
    vd_button(x, by, 36, w->toolbar_page ? "New" : "Open");
    vd_button(x + 40, by, 36, w->toolbar_page ? "Rest" : "Up");
    vd_button(x + 80, by, 44, w->toolbar_page ? "Bin" : "Copy");
    vd_button(x + 128, by, 44, w->toolbar_page ? "Path" : "Paste");
    vd_button(x + 176, by, 44, w->toolbar_page ? "Mark" : "Trash");
    vd_button(x + 224, by, 54, "More");
    const char *extra[] = {"Cut", "Name", "Find", "Mark", PLAT_STORAGE_LABEL, "Root"};
    for (int b = 0; b < 6; b++)
      vd_button(x + b * 47, by - 18, 44, extra[b]);
  } else if (w->app == VD_EDIT) {
    vd_button(x, by, 44, "Save");
    vd_button(x + 48, by, 44, "Path");
    vd_label(x + 98, by + 4, w->path, VD_ACCENT, width - 98);
  } else if (w->app == VD_TASKS)
    vd_button(x, by, 70, "Refresh");
  else if (w->app == VD_SSH) {
    vd_button(x, by, 70, "Connect");
    vd_button(x + 74, by, 44, "Key");
    vd_button(x + 122, by, 44, "Save");
    vd_label(x + 170, by + 4, w->input, VD_ACCENT, width - 170);
  } else if (w->app == VD_DOWNLOAD) {
    vd_button(x, by, 54, "Start");
    vd_button(x + 58, by, 54, "Cancel");
    vd_button(x + 116, by, 44, "Path");
    vd_label(x + 164, by + 4, w->input, VD_ACCENT, width - 164);
  } else if (w->app == VD_CALC)
    vd_label(x, by + 4, w->input, VD_ACCENT, width);
  else if (w->app == VD_PACKAGES) {
    vd_button(x, by, 54, "List");
    vd_button(x + 58, by, 62, "Install");
    vd_button(x + 124, by, 62, "Update");
  } else if (w->app == VD_BACKUP)
    vd_button(x, by, 110, "Backup /root");
  else if (w->app == VD_UPDATER) {
    vd_button(x, by, 54, "Check");
    vd_button(x + 58, by, 54, "Update");
    vd_button(x + 116, by, 54, vd.auto_update ? "Auto+" : "Auto-");
    vd_button(x + 174, by, 54, "Cancel");
  } else if (w->app == VD_MEDIA) {
    vd_button(x, by, 48, "Cam");
    vd_button(x + 52, by, 48, "Inner");
    vd_button(x + 104, by, 48, "Rec");
    vd_button(x + 156, by, 48, "Stop");
    vd_button(x + 208, by, 48, "Play");
  } else if (w->app == VD_REMOTE) {
    vd_button(x, by, 70, "Connect");
    vd_button(x + 74, by, 54, "Pass");
    vd_button(x + 132, by, 54, "Stop");
    vd_button(x + 190, by, 48, "Keys");
  }
  if (w->entry_mode)
    vd_label(x, w->y + w->h - 38,
             w->entry_mode == 6 ? "Sensitive entry (hidden)" : w->input,
             VD_ACCENT, width);
  else if (w->terminal) {
    vd_button(x, by, 44, "Copy");
    vd_button(x + 48, by, 44, "Paste");
    vd_button(x + 96, by, 36, "^Pg");
    vd_button(x + 136, by, 36, "vPg");
    vd_button(x + 176, by, 36, "Key");
  }
}
static struct {
  uint8_t *address[70];
  uint8_t bytes[70][4];
  uint8_t size[70];
  int count;
} vd_cursor_saved;
static void vd_cursor_restore(void) {
  for (int i = 0; i < vd_cursor_saved.count; i++)
    memcpy(vd_cursor_saved.address[i], vd_cursor_saved.bytes[i], vd_cursor_saved.size[i]);
}
static void vd_cursor_draw(void) {
  vd_cursor_saved.count = 0;
  for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 7; xx++) {
    int x = vd.px + xx - vd.pan_x, y = vd.py + yy - vd.pan_y;
    int s = y >= VD_TERM_H;
    if (s) { x -= VD_PANEL_X; y -= VD_TERM_H; }
    plat_fb_t *fb = &vd.fb[s];
    if (!fb->base || x < 0 || y < 0 || x >= fb->w || y >= fb->h) continue;
    int i = vd_cursor_saved.count++;
    vd_cursor_saved.address[i] = fb->base + (ptrdiff_t)y * fb->y_stride + (ptrdiff_t)x * fb->x_stride;
    vd_cursor_saved.size[i] = fb->bpp;
    memcpy(vd_cursor_saved.bytes[i], vd_cursor_saved.address[i], fb->bpp);
  }
  vd_rect(vd.px, vd.py, 2, 10, 0xffffff);
  vd_rect(vd.px, vd.py, 7, 2, 0xffffff);
}
static void vd_render(void) {
  if (!vd.active)
    return;
  uint64_t now = plat_us();
  if (!vd.dirty && !vd.pointer_dirty && now - vd.last_clock < 500000)
    return;
  if (now - vd.last_render < g_top_refresh_us)
    return;
  vd.last_render = now;
  plat_surface(PLAT_SURF_TERM, &vd.fb[0]);
  plat_surface(PLAT_SURF_PANEL, &vd.fb[1]);
#ifdef PLAT_VITA
  plat_desktop_scale(VD_SCALE);
  vd.fb[0].w = VD_TERM_W; vd.fb[0].h = VD_TERM_H;
  vd.fb[1].w = VD_PANEL_W; vd.fb[1].h = VD_PANEL_H;
#endif
#ifdef PLAT_VITA
  if (!vd.dirty && vd.pointer_dirty && now - vd.last_clock < 500000) {
    vd_cursor_restore();
    vd_cursor_draw();
    plat_present(PLAT_SURF_BIT(PLAT_SURF_TERM) | PLAT_SURF_BIT(PLAT_SURF_PANEL));
    vd.pointer_dirty = false;
    return;
  }
#endif
  vd.last_clock = now;
  {
    uint32_t c = vd.wallpaper == 1   ? 0x102219
                 : vd.wallpaper == 2 ? 0x112b25
                                     : 0x12271e;
    vd_rect(vd.pan_x, vd.pan_y, VD_W, VD_H, c);
  }
  /* Code-native wallpaper and icons, cheap enough for every supported model. */
  for (int k = 0; k < 6; k++) {
    int x = VD_W - 100 + k * 10, y = 50 + k * 9;
    vd_rect(x, y, 60 - k * 6, 6, 0x244631);
  }
  vd_label(14, 12, "VERDANT", VD_ACCENT, 100);
  vd_label(110, 12, vd.status, VD_ACCENT, VD_W - 112);
  for (int a = 0; a < 4; a++) {
    vd_icon(14, 38 + a * 38, a);
    vd_label(41, 44 + a * 38, vd_names[a], VD_TEXT_COLOR, 120);
  }
  plat_mutex_lock(&ui_lock);
  term_state.dirty = false;
  plat_mutex_unlock(&ui_lock);
  for (int i = 0; i < VD_MAX; i++)
    if (i != vd.focused)
      vd_render_window(i);
  if (vd.focused >= 0)
    vd_render_window(vd.focused);
  int bar = VD_H - 22;
  vd_rect(VD_PANEL_X, bar, VD_PANEL_W, 22, 0x0c1711);
  vd_button(VD_PANEL_X + 2, bar + 3, 46, "Menu");
  int tx = VD_PANEL_X + 52;
  for (int i = 0; i < VD_MAX; i++)
    if (vd.windows[i].used && vd.windows[i].workspace == vd.workspace) {
      vd_rect(tx, bar + 3, 22, 15, i == vd.focused ? 0x477647 : 0x24392b);
      vd_char(tx + 7, bar + 6, vd.windows[i].title[0], VD_TEXT_COLOR, 1);
      tx += 25;
      if (tx > VD_PANEL_X + VD_PANEL_W - 96)
        break;
    }
  time_t t = (time_t)(plat_wallclock_ms() / 1000);
  struct tm *tm = localtime(&t);
  char clock[16];
  if (tm)
    snprintf(clock, sizeof(clock), "%02d:%02d", tm->tm_hour, tm->tm_min);
  else
    strcpy(clock, "--:--");
  vd_label(VD_PANEL_X + VD_PANEL_W - 44, bar + 7, clock, VD_ACCENT, 44);
  if (vd.menu) {
    int x = VD_PANEL_X + 2, y = VD_H - 22 - VD_APP_COUNT * 16;
    vd_rect(x, y, 164, VD_APP_COUNT * 16, 0x1c3528);
    for (int a = 0; a < VD_APP_COUNT; a++)
      vd_label(x + 6, y + a * 16 + 4, vd_names[a], VD_TEXT_COLOR, 152);
  }
  if (vd.keyboard) {
    vd_rect(VD_PANEL_X, VD_TERM_H, VD_PANEL_W, VD_PANEL_H, VD_SURFACE);
    const char *normal[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
    const char *symbols[] = {"!@#$%^&*()", "-_=+[]{}\\|", ";:'\"`,.<>?", "/~"};
    const char **rows = vd.symbols ? symbols : normal;
    int kh = (VD_PANEL_H - 70) / 4, kw = VD_PANEL_W / 10;
    for (int r = 0; r < 4; r++)
      for (int c = 0; rows[r][c]; c++) {
        int x = VD_PANEL_X + c * kw, y = VD_TERM_H + 22 + r * kh;
        vd_rect(x + 1, y + 1, kw - 2, kh - 2, 0x294634);
        vd_char(x + kw / 2 - 4, y + kh / 2 - 4,
                vd.shift ? toupper(rows[r][c]) : rows[r][c], VD_TEXT_COLOR, 1);
      }
    vd_label(VD_PANEL_X + 4, VD_TERM_H + 6,
             vd.symbols ? "ABC  |  " PLAT_BACK_LABEL ": hide" : "SYM  |  " PLAT_BACK_LABEL ": hide", VD_ACCENT,
             VD_PANEL_W - 8);
    const char *keys[] = {"CTL", "ALT", "SHF", "TAB",
                          "ESC", "DEL", "ENT", "SPC"};
    int bw = VD_PANEL_W / 8;
    for (int c = 0; c < 8; c++)
      vd_button(VD_PANEL_X + c * bw, VD_H - 44, bw - 1, keys[c]);
    const char *nav[] = {"<", "v", "^", ">", "Home", "End", "PgUp", "PgDn"};
    for (int c = 0; c < 8; c++)
      vd_button(VD_PANEL_X + c * bw, VD_H - 23, bw - 1, nav[c]);
  }
  if (vd.notice[0] && !vd.keyboard)
    vd_label(VD_PANEL_X + 4, VD_H - 33, vd.notice, VD_ACCENT, VD_PANEL_W - 8);
  vd_cursor_draw();
  plat_present(PLAT_SURF_BIT(PLAT_SURF_TERM) | PLAT_SURF_BIT(PLAT_SURF_PANEL));
  vd.dirty = false;
  vd.pointer_dirty = false;
}
static void vd_selected_path(VDWindow *w, char *out, size_t n) {
  const char *s = w->text;
  int line = w->selection;
  while (*s && line)
    if (*s++ == '\n')
      line--;
  if ((s[0] != 'D' && s[0] != 'F') || s[1] != ' ') {
    out[0] = 0;
    return;
  }
  s += 2;
  char name[256];
  size_t j = 0;
  while (*s && *s != '\n' && j + 1 < sizeof(name))
    name[j++] = *s++;
  name[j] = 0;
  if (name[0] == '/')
    snprintf(out, n, "%s", name);
  else
    snprintf(out, n, "%s/%s", w->path, name);
}
static void vd_copy_terminal(VDWindow *w) {
  if (!w || !w->terminal)
    return;
  TermState *t = w->terminal;
  size_t n = 0;
  int start = vd.has_selection ? vd.select_start : 0,
      end = vd.has_selection ? vd.select_end : TERM_ROWS * TERM_COLS - 1;
  if (start > end) {
    int tmp = start;
    start = end;
    end = tmp;
  }
  for (int k = start; k <= end && n + 2 < sizeof(vd.clipboard); k++) {
    if (k < -t->sb_count * TERM_COLS || k >= TERM_ROWS * TERM_COLS)
      continue;
    int gy = k / TERM_COLS, gx = k % TERM_COLS;
    if (gx < 0) {
      gx += TERM_COLS;
      gy--;
    }
    if (gy < 0) {
      int slot =
          (t->sb_head + t->sb_count + gy + TERM_SCROLLBACK) % TERM_SCROLLBACK;
      vd.clipboard[n++] = t->sb_buf[slot][gx].c;
    } else
      vd.clipboard[n++] = t->grid[k].c;
    if (gx == TERM_COLS - 1)
      vd.clipboard[n++] = '\n';
  }
  vd.clipboard[n] = 0;
  vd_notice("Copied terminal text");
}
static void vd_stop_media(void) {
  if (vd.recording) {
    for (int i = 0; i < plat_hw_count; i++)
      if (!strcmp(plat_hw_files[i].name, "mic_control") && plat_hw_files[i].wr)
        plat_hw_files[i].wr("stop", 4);
    unsigned char h[44] = {0};
    memcpy(h, "RIFF", 4);
    g_st32(h + 4, vd.recorded + 36);
    memcpy(h + 8, "WAVEfmt ", 8);
    g_st32(h + 16, 16);
    g_st16(h + 20, 1);
    g_st16(h + 22, 1);
    g_st32(h + 24, 16360);
    g_st32(h + 28, 32720);
    g_st16(h + 32, 2);
    g_st16(h + 34, 16);
    memcpy(h + 36, "data", 4);
    g_st32(h + 40, vd.recorded);
    fseek(vd.recording, 0, SEEK_SET);
    fwrite(h, 1, 44, vd.recording);
    fclose(vd.recording);
    vd.recording = NULL;
    vd_notice("Saved verdant/recording.wav");
  }
  if (vd.playing) {
    fclose(vd.playing);
    vd.playing = NULL;
  }
}
static void vd_media_action(VDWindow *w, int button) {
  if (button < 2) {
    uint8_t *raw = NULL;
    int bytes = plat_hw_camera(button == 1, &raw);
    if (bytes != 400 * 240 * 2) {
      free(raw);
      vd_notice("Camera unavailable");
      return;
    }
    unsigned char *rgb = malloc(400 * 240 * 3);
    if (!rgb) {
      free(raw);
      vd_notice("Insufficient image memory");
      return;
    }
    for (int i = 0; i < 400 * 240; i++) {
      uint16_t p = g_ld16(raw + i * 2);
      rgb[i * 3] = ((p >> 11) & 31) * 255 / 31;
      rgb[i * 3 + 1] = ((p >> 5) & 63) * 255 / 63;
      rgb[i * 3 + 2] = (p & 31) * 255 / 31;
    }
    free(raw);
    stbi_image_free(w->image);
    w->image = rgb;
    w->image_w = 400;
    w->image_h = 240;
    vd_notice("Captured camera snapshot");
  } else if (button == 2) {
    vd_stop_media();
    if (!plat_caps()->mic) {
      vd_notice("Microphone unavailable");
      return;
    }
    vd.recording = fopen(PLAT_SD "verdant/recording.wav", "wb");
    if (vd.recording) {
      unsigned char zero[44] = {0};
      fwrite(zero, 1, 44, vd.recording);
      vd.recorded = 0;
      vd_notice("Recording; Stop saves WAV");
    }
  } else if (button == 3)
    vd_stop_media();
  else {
    vd_stop_media();
    if (!plat_caps()->audio) {
      vd_notice("Audio unavailable");
      return;
    }
    vd.playing = fopen(PLAT_SD "verdant/recording.wav", "rb");
    if (vd.playing) {
      unsigned char h[44];
      if (fread(h, 1, 44, vd.playing) != 44 || memcmp(h, "RIFF", 4) ||
          memcmp(h + 8, "WAVE", 4) || g_ld16(h + 20) != 1 ||
          g_ld16(h + 34) != 16 || g_ld16(h + 22) != 1 ||
          g_ld32(h + 24) != 16360) {
        vd_stop_media();
        vd_notice("Unsupported recording format");
      } else
        vd_notice("Playing recording");
    }
  }
}
static void vd_media_tick(void) {
  uint64_t now = plat_us();
  if (now - vd.last_media < 20000)
    return;
  vd.last_media = now;
  if (vd.recording) {
    uint8_t b[2048];
    int n = plat_hw_mic_read(b, sizeof(b));
    if (n > 0) {
      size_t written = fwrite(b, 1, n, vd.recording);
      vd.recorded += written;
      if (written != (size_t)n)
        vd_stop_media();
    }
  }
  if (vd.playing) {
    int16_t mono[320], stereo[1280];
    size_t n = fread(mono, 2, 320, vd.playing);
    for (size_t i = 0; i < n; i++)
      stereo[i * 4] = stereo[i * 4 + 1] = stereo[i * 4 + 2] =
          stereo[i * 4 + 3] = mono[i];
    if (n)
      plat_hw_audio_write((uint8_t *)stereo, n * 8);
    else {
      vd_stop_media();
      vd_notice("Playback complete");
    }
  }
}
static void vd_action(VDWindow *w, int button) {
  char path[1024];
  if (w->app == VD_FILES) {
    vd_selected_path(w, path, sizeof(path));
    if (button == 0 && path[0]) {
      const char *s = w->text;
      int line = w->selection;
      while (*s && line)
        if (*s++ == '\n')
          line--;
      if (*s == 'D') {
        if (!vd_path_copy(w->path, sizeof(w->path), path))
          return;
        w->selection = w->scroll = 0;
        vd_request(w, "list", w->path, NULL, NULL, NULL);
      } else {
        if (strlen(path) >= sizeof(w->path)) {
          vd_notice("Path is too long");
          return;
        }
        bool image = vd_image_extension(path);
        int idx = vd_new(image ? VD_MEDIA : VD_EDIT);
        if (idx >= 0) {
          VDWindow *e = &vd.windows[idx];
          vd_path_copy(e->path, sizeof(e->path), path);
          vd_request(e, image ? "image" : "read", e->path, NULL, NULL, NULL);
        }
      }
    } else if (button == 1) {
      char *s = strrchr(w->path, '/');
      if (s && s != w->path)
        *s = 0;
      else
        strcpy(w->path, "/");
      w->selection = w->scroll = 0;
      vd_request(w, "list", w->path, NULL, NULL, NULL);
    } else if ((button == 2 || button == 6) && path[0]) {
      if (!vd_path_copy(vd.file_clipboard, sizeof(vd.file_clipboard), path))
        return;
      vd.cut = button == 6;
      vd_notice(vd.cut ? "Cut; Paste moves the file"
                       : "Copied; Paste copies the file");
    } else if (button == 3 && vd.file_clipboard[0]) {
      const char *name = strrchr(vd.file_clipboard, '/');
      snprintf(path, sizeof(path), "%s/%s", w->path,
               name ? name + 1 : vd.file_clipboard);
      vd_request(w, vd.cut ? "move" : "copy", vd.file_clipboard, path, NULL,
                 NULL);
      if (vd.cut)
        vd.file_clipboard[0] = 0;
    } else if (button == 4 && path[0])
      vd_request(w, "trash", path, NULL, NULL, NULL);
    else if (button == 5 || button == 21)
      w->toolbar_page = !w->toolbar_page;
    else if (button == 19 || button == 7 || button == 8 || button == 16) {
      w->entry_mode = button == 7 ? 2 : button == 8 ? 4 : button == 16 ? 5 : 1;
      w->input[0] = 0;
      vd.keyboard = true;
      vd_notice(button == 7   ? "Enter full destination path"
                : button == 8 ? "Enter a search term"
                              : "Type path, then Enter");
    } else if (button == 9 || button == 20) {
      if (!vd.bookmark[0]) {
        strcpy(vd.bookmark, w->path);
        vd_notice("Bookmarked. Mark returns here.");
      } else {
        strcpy(w->path, vd.bookmark);
        vd_request(w, "list", w->path, NULL, NULL, NULL);
      }
    } else if (button == 17 && path[0])
      vd_request(w, "restore", path, NULL, NULL, NULL);
    else if (button == 18) {
      strcpy(w->path, "/root/.local/share/Trash/files");
      w->selection = w->scroll = 0;
      vd_request(w, "list", w->path, NULL, NULL, NULL);
    } else if (button == 10 || button == 11) {
      strcpy(w->path, button == 10 ? PLAT_GUEST_STORAGE : "/root");
      w->selection = w->scroll = 0;
      vd_request(w, "list", w->path, NULL, NULL, NULL);
    }
  } else if (w->app == VD_EDIT) {
    if (button == 0)
      vd_request(w, "write", w->path, w->text, NULL, NULL);
    else {
      w->entry_mode = 1;
      w->input[0] = 0;
      vd.keyboard = true;
      vd_notice("Type save path, then Enter");
    }
  } else if (w->app == VD_SETTINGS) {
    if (button == 0)
      vd.joined = !vd.joined;
    else if (button == 1)
      vd.touchpad = !vd.touchpad;
    else if (button == 2)
      vd.wallpaper = (vd.wallpaper + 1) % 3;
    else if (button == 3) {
      g_cfg.use_5x7 = !g_cfg.use_5x7;
      for (int s = 0; s < 4; s++)
        if (vd.sessions[s])
          vd.sessions[s]->use_5x7 = g_cfg.use_5x7;
    } else if (button == 5)
      vd_new(VD_UPDATER);
    else
      vd_save_preferences();
  } else if (w->app == VD_UPDATER) {
    if (button == 2) {
      vd.auto_update = !vd.auto_update;
      FILE *f = fopen(PLAT_SD "verdant/update-auto.cfg", "wb");
      if (f) {
        fprintf(f, "%d\n", vd.auto_update);
        fclose(f);
      }
      vd_notice(vd.auto_update ? "Automatic downloads enabled"
                               : "Automatic downloads disabled");
    } else if (button == 3) {
      char id[20];
      snprintf(id, sizeof(id), "%u", w->job);
      vd_request(w, "cancel", id, NULL, NULL, NULL);
    } else if (!w->job && !w->pending)
      vd_request(w, "sysupdate", button == 0 ? "check" : "stage", PLAT_SLUG,
                 VU_VERSION, NULL);
  } else if (w->app == VD_TASKS)
    vd_request(w, "performance", NULL, NULL, NULL, NULL);
  else if (w->app == VD_MEDIA)
    vd_media_action(w, button);
  else if (w->app == VD_REMOTE) {
    if (button == 0)
      vd_request(w, "vnc", w->input, w->path, NULL, NULL);
    else if (button == 1) {
      w->entry_mode = 6;
      strcpy(w->previous_input, w->input);
      w->input[0] = 0;
      vd.keyboard = true;
      vd_notice("Enter VNC password, then Enter");
    } else if (button == 2) {
      char jid[16];
      snprintf(jid, sizeof(jid), "%u", w->job);
      vd_request(w, "cancel", jid, NULL, NULL, NULL);
      w->job = 0;
      stbi_image_free(w->image);
      w->image = NULL;
    } else
      vd.keyboard = !vd.keyboard;
  } else if (w->app == VD_SSH) {
    if (button == 1) {
      w->entry_mode = 6;
      strcpy(w->previous_input, w->input);
      w->input[0] = 0;
      vd.keyboard = true;
      vd_notice("Enter guest private-key path");
    } else if (button == 2) {
      FILE *f = fopen(PLAT_SD "verdant/ssh-profile.cfg", "wb");
      if (f) {
        fprintf(f, "%s\n%s\n", w->input, w->path);
        fclose(f);
        vd_notice("Saved SSH profile");
      }
    } else {
      char *at = strchr(w->input, '@');
      if (at) {
        *at = 0;
        int i = vd_new(VD_TERM);
        if (i >= 0) {
          VDWindow *t = &vd.windows[i];
          if (t->session >= 0) {
            char sid[12];
            snprintf(sid, sizeof(sid), "%d", t->session);
            vd_request(NULL, "close", sid, NULL, NULL, NULL);
            vd_request(t, "ssh", sid, at + 1, w->input, w->path);
          }
        }
        *at = '@';
      } else
        vd_notice("Enter user@host");
    }
  } else if (w->app == VD_DOWNLOAD) {
    if (button == 0)
      vd_request(w, "download", w->input, w->path, NULL, NULL);
    else if (button == 1) {
      char id[16];
      snprintf(id, sizeof(id), "%u", w->job);
      vd_request(w, "cancel", id, NULL, NULL, NULL);
      w->job = 0;
    } else {
      w->entry_mode = 1;
      w->input[0] = 0;
      vd.keyboard = true;
      vd_notice("Type destination path, then Enter");
    }
  } else if (w->app == VD_PACKAGES || w->app == VD_BACKUP) {
    int i = vd_new(VD_TERM);
    if (i >= 0) {
      VDWindow *t = &vd.windows[i];
      if (t->session >= 0) {
        char sid[12];
        snprintf(sid, sizeof(sid), "%d", t->session);
        vd_request(NULL, "close", sid, NULL, NULL, NULL);
        if (w->app == VD_PACKAGES && button == 1)
          vd_request(t, "pkginstall", sid, w->input, NULL, NULL);
        else
          vd_request(t, "shell", sid,
                     w->app == VD_PACKAGES
                         ? (button == 2
                                ? "opkg update; opkg list-upgradable; exec "
                                  "/bin/bash -l"
                                : "opkg list-installed; exec /bin/bash -l")
                         : "tar -czf " PLAT_GUEST_STORAGE "/verdant/root-backup-$(date "
                           "+%Y%m%d-%H%M%S).tar.gz "
                           "/root; exec /bin/bash -l",
                     NULL, NULL);
      }
    }
  } else if (w->terminal) {
    if (button == 0)
      vd_copy_terminal(w);
    else if (button == 1)
      for (const char *s = vd.clipboard; *s; s++)
        desktop_input_byte(*s);
    else if (button == 2) {
      w->terminal->auto_track = false;
      w->terminal->scroll_y -= 8;
    } else if (button == 3)
      w->terminal->scroll_y += 8;
    else
      vd.keyboard = !vd.keyboard;
  }
  vd.dirty = true;
}
static void vd_keyboard_click(int x, int y) {
  int xx = x - VD_PANEL_X, yy = y - VD_TERM_H, kh = (VD_PANEL_H - 70) / 4,
      kw = VD_PANEL_W / 10;
  if (yy < 22) {
    vd.symbols = !vd.symbols;
    vd.dirty = true;
    return;
  }
  if (y < VD_H - 48) {
    const char *normal[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
    const char *symbols[] = {"!@#$%^&*()", "-_=+[]{}\\|", ";:'\"`,.<>?", "/~"};
    const char **rows = vd.symbols ? symbols : normal;
    int r = (yy - 22) / kh, c = xx / kw;
    if (r >= 0 && r < 4 && c >= 0 && c < (int)strlen(rows[r])) {
      char ch = rows[r][c];
      if (vd.shift)
        ch = toupper(ch);
      if (vd.ctrl) {
        ch = (char)(toupper(ch) & 31);
        vd.ctrl = false;
      }
      if (vd.alt) {
        desktop_input_byte(27);
        vd.alt = false;
      }
      desktop_input_byte(ch);
      vd.shift = false;
    }
  } else {
    int k = xx / (VD_PANEL_W / 8);
    if (k < 0 || k > 7)
      return;
    if (y < VD_H - 24) {
      if (k == 0)
        vd.ctrl = !vd.ctrl;
      else if (k == 1)
        vd.alt = !vd.alt;
      else if (k == 2)
        vd.shift = !vd.shift;
      else {
        char chars[] = {0, 0, 0, 9, 27, 127, 13, 32};
        desktop_input_byte(chars[k]);
      }
    } else
      vd_navigation(k);
  }
  vd.dirty = true;
}
static void vd_click(int x, int y) {
  if (vd.keyboard && y >= VD_TERM_H) {
    vd_keyboard_click(x, y);
    return;
  }
  if (vd.menu) {
    int yy = VD_H - 22 - VD_APP_COUNT * 16;
    if (x >= VD_PANEL_X + 2 && x < VD_PANEL_X + 166 && y >= yy &&
        y < VD_H - 22) {
      vd_new((y - yy) / 16);
      return;
    }
    vd.menu = false;
  }
  if (y >= VD_H - 22) {
    if (x < VD_PANEL_X + 50) {
      vd.menu = !vd.menu;
      vd.dirty = true;
      return;
    }
    int tx = VD_PANEL_X + 52;
    for (int i = 0; i < VD_MAX; i++)
      if (vd.windows[i].used && vd.windows[i].workspace == vd.workspace) {
        if (x >= tx && x < tx + 22) {
          vd.focused = i;
          vd.windows[i].minimized = false;
          vd.dirty = true;
          return;
        }
        tx += 25;
      }
    return;
  }
  int hit = -1;
  if (vd.focused >= 0) {
    VDWindow *w = vd_focus();
    if (w && !w->minimized && x >= w->x && x < w->x + w->w && y >= w->y &&
        y < w->y + w->h)
      hit = vd.focused;
  }
  if (hit < 0)
    for (int i = VD_MAX - 1; i >= 0; i--) {
      VDWindow *w = &vd.windows[i];
      if (w->used && !w->minimized && w->workspace == vd.workspace &&
          x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h) {
        hit = i;
        break;
      }
    }
  if (hit < 0) {
    if (x < 150 && y >= 38 && y < 190)
      vd_new((y - 38) / 38);
    return;
  }
  if (vd.focused != hit)
    vd.has_selection = false;
  vd.focused = hit;
  VDWindow *w = &vd.windows[hit];
  if (x >= w->x + w->w - 7 && y >= w->y + w->h - 7) {
    vd.drag = hit;
    vd.resizing = true;
    vd.dirty = true;
    return;
  }
  if (y < w->y + 17) {
    int rel = x - w->x;
    if (rel >= w->w - 16) {
      vd_close(hit);
      return;
    }
    if (rel >= w->w - 40) {
      if (!w->maximized) {
        w->ox = w->x;
        w->oy = w->y;
        w->ow = w->w;
        w->oh = w->h;
        bool lower = w->y >= VD_TERM_H;
        w->x = lower ? VD_PANEL_X : 0;
        w->y = lower ? VD_TERM_H : 0;
        w->w = lower ? VD_PANEL_W : VD_TERM_W;
        w->h = VD_TERM_H - 22;
      } else {
        w->x = w->ox;
        w->y = w->oy;
        w->w = w->ow;
        w->h = w->oh;
      }
      w->maximized = !w->maximized;
    } else if (rel >= w->w - 56)
      w->minimized = true;
    else if (rel >= w->w - 72)
      vd_move_screen(w);
    else {
      vd.drag = hit;
      vd.resizing = false;
      vd.drag_dx = x - w->x;
      vd.drag_dy = y - w->y;
    }
  } else if (w->app == VD_SETTINGS) {
    vd_settings_click(w,x,y);
  } else if (w->app == VD_TASKS && y < w->y+w->h-19) {
    int rx=x-w->x-5, ry=y-w->y-21;
    if(ry<26) { w->task_tab=rx>=116; w->scroll=0; }
    else if(w->task_tab && rx<100 && ry>=32) w->task_category=(ry-32)/30;
    if(w->task_category>4) w->task_category=4;
    vd.dirty=true;
  } else if (y >= w->y + w->h - (w->app == VD_FILES ? 37 : 19)) {
    int rel = x - w->x - 5;
    int b = 0;
    if (w->app == VD_FILES)
      b = y < w->y + w->h - 19 ? 6 + rel / 47
          : rel < 40           ? 0
          : rel < 80           ? 1
          : rel < 128          ? 2
          : rel < 176          ? 3
          : rel < 224          ? 4
                               : 5;
    else if (w->app == VD_SETTINGS)
      b = rel / 50;
    else if (w->app == VD_UPDATER)
      b = rel < 58 ? 0 : rel < 116 ? 1 : rel < 174 ? 2 : 3;
    else if (w->app == VD_MEDIA)
      b = rel / 52;
    else if (w->app == VD_REMOTE)
      b = rel < 74 ? 0 : rel < 132 ? 1 : rel < 190 ? 2 : 3;
    else if (w->app == VD_SSH)
      b = rel < 74 ? 0 : rel < 122 ? 1 : 2;
    else if (w->app == VD_PACKAGES)
      b = rel < 58 ? 0 : rel < 124 ? 1 : 2;
    else if (w->app == VD_EDIT)
      b = rel < 48 ? 0 : 1;
    else if (w->app == VD_DOWNLOAD)
      b = rel < 58 ? 0 : rel < 116 ? 1 : 2;
    else if (w->terminal)
      b = rel < 48 ? 0 : rel < 96 ? 1 : rel < 136 ? 2 : rel < 176 ? 3 : 4;
    if (w->app == VD_FILES && w->toolbar_page && y >= w->y + w->h - 19)
      b += 16;
    vd_action(w, b);
  } else if (w->app == VD_FILES)
    w->selection = w->scroll + (y - w->y - 21) / 11;
  else if (w->app == VD_REMOTE && w->job && w->image) {
    int width = w->w - 10, height = w->h - 45, dw = width,
        dh = w->image_h * width / w->image_w;
    if (dh > height) {
      dh = height;
      dw = w->image_w * height / w->image_h;
    }
    int px = (x - w->x - 5) * w->image_w / dw,
        py = (y - w->y - 21) * w->image_h / dh;
    char e[64];
    snprintf(e, sizeof(e), "ptr %d %d 1", px, py);
    vd_remote_event(w, e);
    snprintf(e, sizeof(e), "ptr %d %d 0", px, py);
    vd_remote_event(w, e);
  } else if (w->terminal) {
    TermState *t = w->terminal;
    int fw = t->use_5x7 ? 5 : 8;
    int gx = (x - w->x - 5) / (fw * t->zoom_x) + t->scroll_x,
        gy = (y - w->y - 21) / (8 * t->zoom_y) + t->scroll_y;
    if (gx >= 0 && gx < TERM_COLS && gy >= -t->sb_count && gy < TERM_ROWS) {
      vd.select_start = vd.select_end = gy * TERM_COLS + gx;
      vd.selecting = vd.has_selection = true;
    }
  } else {
    if (w->app == VD_EDIT) {
      const char *s = w->text;
      int row = w->scroll + (y - w->y - 21) / 11;
      while (*s && row)
        if (*s++ == '\n')
          row--;
      int col = (x - w->x - 5) / 8;
      while (*s && *s != '\n' && col > 0) {
        s++;
        col--;
      }
      w->cursor = s - w->text;
    }
    vd.keyboard = true;
  }
  vd.dirty = true;
}
static void vd_init(void) {
  memset(&vd, 0, sizeof(vd));
  vd.focused = vd.drag = -1;
  vd.select_start = vd.select_end = -1;
  vd.joined = true;
#ifdef PLAT_VITA
  vd.ui_scale = 150;
#else
  vd.ui_scale = 100;
#endif
  vd.px = 100;
  vd.py = 80;
  mkdir(PLAT_SD "verdant", 0777);
  mkdir(PLAT_SD "verdant/bridge", 0777);
  FILE *platform_file = fopen(VD_BRIDGE "platform.txt", "wb");
  if (platform_file) {
    fprintf(platform_file, "%s\n", PLAT_SLUG);
    fclose(platform_file);
  }
  FILE *host_clock = fopen(VD_BRIDGE "host-time", "wb");
  if (host_clock) {
    fprintf(host_clock, "%llu\n",
            (unsigned long long)(plat_wallclock_ms() / 1000));
    fclose(host_clock);
  }
  plat_input_t boot;
  plat_poll_input(&boot);
  vd.recovery = (boot.held & PLAT_BTN_B) != 0;
  /* No command from a previous launch may run against the new guest. */
  DIR *dir = opendir(VD_BRIDGE);
  if (dir) {
    struct dirent *e;
    while ((e = readdir(dir))) {
      const char *ext = strrchr(e->d_name, '.');
      if (ext && (!strcmp(ext, ".req") || !strcmp(ext, ".res") ||
                  !strcmp(ext, ".part") || !strcmp(ext, ".in") ||
                  !strcmp(ext, ".out"))) {
        char p[512];
        snprintf(p, sizeof(p), VD_BRIDGE "%s", e->d_name);
        remove(p);
      }
    }
    closedir(dir);
  }
  remove(VD_BRIDGE "ready");
  remove(VD_BRIDGE "host-http.enabled");
  /* Reserve terminals before the guest consumes the remaining heap. */
  for (int s = 0; s < 4; s++) {
    vd.sessions[s] = calloc(1, sizeof(TermState));
    if (vd.sessions[s])
      term_init(vd.sessions[s]);
  }
  FILE *f = fopen(PLAT_SD "verdant/preferences.cfg", "rb");
  if (f) {
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
      int v;
      if (sscanf(line, "ui_scale=%d", &v) == 1)
        vd.ui_scale = v >= 100 && v <= 200 && v % 25 == 0 ? v : 150;
      else if (sscanf(line, "joined=%d", &v) == 1)
        vd.joined = v != 0;
      else if (sscanf(line, "touchpad=%d", &v) == 1)
        vd.touchpad = v != 0;
      else if (sscanf(line, "wallpaper=%d", &v) == 1)
        vd.wallpaper = v >= 0 && v < 3 ? v : 0;
      else if (sscanf(line, "workspace=%d", &v) == 1)
        vd.workspace = (v >= 0 && v < 4) ? v : 0;
      else if (!strncmp(line, "bookmark=", 9)) {
        line[strcspn(line, "\r\n")] = 0;
        snprintf(vd.bookmark, sizeof(vd.bookmark), "%.511s", line + 9);
      }
    }
    fclose(f);
  }
#ifdef PLAT_VITA
  if (!strncmp(vd.bookmark, "/mnt/3ds/sd", 11) &&
      (!vd.bookmark[11] || vd.bookmark[11] == '/')) {
    char old[512];
    strcpy(old, vd.bookmark);
    snprintf(vd.bookmark, sizeof(vd.bookmark), PLAT_GUEST_STORAGE "%.480s", old + 11);
  }
#endif
  /* Preserve the real kernel console during boot and in recovery. */
  VDWindow *w = &vd.windows[0];
  w->used = true;
  w->app = VD_TERM;
  w->terminal = &term_state;
  w->session = -1;
  w->w = VD_W - 24;
  w->h = 210;
  w->x = 12;
  w->y = 24;
  strcpy(w->title, "Linux console");
  vd.focused = 0;
}
/* At full deflection cross one-and-a-half desktop widths per second.
   Fractional pixels survive between polls; rendering speed cannot change
   pointer sensitivity. A long stall must not throw the pointer offscreen. */
static int vd_pointer_step(int axis, uint64_t elapsed, int64_t *fraction) {
  if (!axis) { *fraction = 0; return 0; }
  if (elapsed > 50000) elapsed = 50000;
  const int64_t denominator = 256000000;
  int64_t distance = *fraction + (int64_t)axis * VD_W * 3 * elapsed;
  int pixels = (int)(distance / denominator);
  *fraction = distance % denominator;
  return pixels;
}
static void vd_update(const plat_input_t *in) {
  vd_poll_bridge();
  if (!vd.update_checked && !vd.recovery) {
    FILE *ready = fopen(VD_BRIDGE "ready", "rb");
    if (ready) {
      fclose(ready);
      vd.update_checked = true;
#ifdef PLAT_VITA
      plat_http_start();
#endif
      FILE *prefs = fopen(PLAT_SD "verdant/update-auto.cfg", "rb");
      int enabled = 0;
      if (prefs) {
        if(fscanf(prefs, "%d", &enabled)!=1) enabled=0;
        fclose(prefs);
      }
      vd.auto_update = enabled == 1;
      int old = vd.focused, i = vd_new(VD_UPDATER);
      if (i >= 0) {
        vd.windows[i].minimized = true;
        vd.focused = old;
        vd_request(&vd.windows[i], "sysupdate",
                   vd.auto_update ? "stage" : "check", PLAT_SLUG, VU_VERSION,
                   NULL);
      }
    }
  }
  for(int i=0;i<VD_MAX;i++) {
    VDWindow *task=&vd.windows[i];
    if(task->used && task->app==VD_TASKS && !task->minimized && task->workspace==vd.workspace
        && plat_us()-task->performance_tick>2000000 && !task->pending) {
      task->performance_tick=plat_us();
#ifdef PLAT_VITA
      plat_performance(&vd_performance.native);
#endif
      vd_request(task,"performance",NULL,NULL,NULL,NULL);
    }
  }
  vd_media_tick();
  VDWindow *w = vd_focus();
  static uint64_t status_tick;
  uint64_t now = plat_us();
  uint64_t pointer_elapsed = vd.pointer_tick ? now - vd.pointer_tick : 16667;
  vd.pointer_tick = now;
  if (now - status_tick > 2000000) {
    FILE *clock_file = fopen(VD_BRIDGE "host-time", "wb");
    if (clock_file) {
      fprintf(clock_file, "%llu\n",
              (unsigned long long)(plat_wallclock_ms() / 1000));
      fclose(clock_file);
    }
    char bat[12] = "?", chg[12] = "?", wifi[12] = "?";
    for (int i = 0; i < plat_hw_count; i++) {
      const plat_hw_ent *h = &plat_hw_files[i];
      char *b = !strcmp(h->name, "battery")    ? bat
                : !strcmp(h->name, "charging") ? chg
                : (!strcmp(h->name, "wifi") || !strcmp(h->name, "network"))
                    ? wifi
                    : NULL;
      if (b && h->rd_text) {
        h->rd_text(b, 12);
        b[strcspn(b, "\r\n")] = 0;
      }
    }
    int64_t freebytes = plat_v9p_free_bytes();
    snprintf(vd.status, sizeof(vd.status), "B%s C%s W%s " PLAT_STORAGE_LABEL " %lldM", bat, chg,
             wifi, (long long)(freebytes < 0 ? 0 : freebytes / (1024 * 1024)));
    status_tick = now;
  }
  if (in->down & PLAT_BTN_SETTINGS) {
    vd.menu = !vd.menu;
    vd.dirty = true;
  }
  if (in->down & PLAT_BTN_B) {
    vd.keyboard = false;
    vd.menu = false;
    vd.dirty = true;
  }
  if (in->down & PLAT_BTN_X) {
    vd_cycle();
  }
  if (in->down & PLAT_BTN_Y) {
    vd_move_screen(w);
  }
  if ((in->held & PLAT_BTN_SETTINGS) && (in->down & PLAT_BTN_Y)) {
    for (int i = 0; i < VD_MAX; i++)
      if (vd.windows[i].used && &vd.windows[i] != w)
        vd_move_screen(&vd.windows[i]);
  }
  if (in->down & PLAT_BTN_FOLLOW) {
    vd.workspace = (vd.workspace + 1) % 4;
    vd.focused = -1;
    vd.dirty = true;
  }
  if (in->down & PLAT_BTN_FONT) {
    vd.keyboard = !vd.keyboard;
    vd.dirty = true;
  }
  if ((in->held & PLAT_BTN_SETTINGS) && (in->down & PLAT_BTN_RIGHT)) {
    vd.workspace = (vd.workspace + 1) % 4;
    vd.focused = -1;
    vd.dirty = true;
  }
  if (in->pan_x || in->pan_y) {
    if (in->held & PLAT_BTN_SETTINGS) {
      for (int i = 0; i < VD_MAX; i++)
        if (vd.windows[i].used && vd.windows[i].workspace == vd.workspace) {
          vd.windows[i].x -= in->pan_x / 24;
          vd.windows[i].y += in->pan_y / 24;
        }
      vd.dirty = true;
    } else {
      vd.px += vd_pointer_step(in->pan_x, pointer_elapsed, &vd.pointer_fraction_x);
      vd.py += vd_pointer_step(-in->pan_y, pointer_elapsed, &vd.pointer_fraction_y);
    }
    if (vd.px < 0)
      vd.px = 0;
    if (vd.px >= VD_W)
      vd.px = VD_W - 1;
    if (vd.py < 0)
      vd.py = 0;
    if (vd.py >= VD_H)
      vd.py = VD_H - 1;
    vd.pointer_dirty = true;
  } else {
    vd.pointer_fraction_x = vd.pointer_fraction_y = 0;
  }
  int x = in->ptr_x + VD_PANEL_X, y = in->ptr_y + VD_TERM_H;
#ifdef PLAT_VITA
  x = in->ptr_x * VD_TERM_W / PLAT_TERM_W;
  int physical_y = in->ptr_y + PLAT_TERM_H;
  y = physical_y < PLAT_TERM_H ? physical_y * VD_TERM_H / PLAT_TERM_H
      : VD_TERM_H + (physical_y - PLAT_TERM_H) * VD_PANEL_H / PLAT_PANEL_H;
#endif
  if (in->ptr_down) {
    if (vd.touchpad && !vd.keyboard) {
      static int lx, ly;
      if (vd.ptr_was_down) {
        vd.px += x - lx;
        vd.py += y - ly;
      }
      lx = x;
      ly = y;
      x = vd.px;
      y = vd.py;
    } else {
      vd.px = x;
      vd.py = y;
    }
    vd.pointer_dirty = true;
  }
  if (in->ptr_tapped)
    vd_click(x, y);
  if (in->down & PLAT_BTN_A)
    vd_click(vd.px, vd.py);
  if (vd.drag >= 0 && (in->ptr_down || (in->held & PLAT_BTN_A))) {
    VDWindow *d = &vd.windows[vd.drag];
    if (vd.resizing) {
      d->w = vd.px - d->x + 1;
      d->h = vd.py - d->y + 1;
      if (d->w < 300)
        d->w = 300;
      if (d->w > VD_W)
        d->w = VD_W;
      if (d->h < 110)
        d->h = 110;
      if (d->h > VD_H - 22)
        d->h = VD_H - 22;
    } else {
      d->x = vd.px - vd.drag_dx;
      d->y = vd.py - vd.drag_dy;
      if (!vd.joined) {
        int lo = d->y < VD_TERM_H ? 0 : VD_TERM_H,
            hi = lo + VD_TERM_H - d->h;
        if (d->y > hi)
          d->y = hi;
      }
    }
    vd.dirty = true;
  } else
    vd.drag = -1;
  if (vd.selecting && (in->ptr_down || (in->held & PLAT_BTN_A))) {
    VDWindow *s = vd_focus();
    if (s && s->terminal) {
      TermState *t = s->terminal;
      int fw = t->use_5x7 ? 5 : 8;
      int gx = (vd.px - s->x - 5) / (fw * t->zoom_x) + t->scroll_x;
      int gy = (vd.py - s->y - 21) / (8 * t->zoom_x) + t->scroll_y;
      if (gx >= 0 && gx < TERM_COLS && gy >= -t->sb_count && gy < TERM_ROWS) {
        vd.select_end = gy * TERM_COLS + gx;
        vd.dirty = true;
      }
    }
  } else
    vd.selecting = false;
  vd.ptr_was_down = in->ptr_down;
  if (w && w->terminal) {
    if ((in->down & PLAT_BTN_ZOOM_IN) && !(in->down & PLAT_BTN_X)) {
      if (w->terminal->zoom_x < 3)
        w->terminal->zoom_x++;
      w->terminal->zoom_y = w->terminal->zoom_x;
      g_cfg.zoom_x = g_cfg.zoom_y = w->terminal->zoom_x;
      vd.dirty = true;
    }
    if ((in->down & PLAT_BTN_ZOOM_OUT) && !(in->down & PLAT_BTN_Y)) {
      if (w->terminal->zoom_x > 1)
        w->terminal->zoom_x--;
      w->terminal->zoom_y = w->terminal->zoom_x;
      g_cfg.zoom_x = g_cfg.zoom_y = w->terminal->zoom_x;
      vd.dirty = true;
    }
    if (in->down & PLAT_BTN_UP)
      vd_navigation(2);
    if (in->down & PLAT_BTN_DOWN)
      vd_navigation(1);
    if (in->down & PLAT_BTN_LEFT)
      vd_navigation(0);
    if (in->down & PLAT_BTN_RIGHT)
      vd_navigation(3);
  } else if (w && (w->app == VD_EDIT || w->app == VD_REMOTE)) {
    if (in->down & PLAT_BTN_UP)
      vd_navigation(2);
    if (in->down & PLAT_BTN_DOWN)
      vd_navigation(1);
    if (in->down & PLAT_BTN_LEFT)
      vd_navigation(0);
    if (in->down & PLAT_BTN_RIGHT)
      vd_navigation(3);
  } else if (w) {
    if (in->down & PLAT_BTN_UP) {
      if (w->scroll > 0)
        w->scroll--;
      vd.dirty = true;
    }
    if (in->down & PLAT_BTN_DOWN) {
      w->scroll++;
      vd.dirty = true;
    }
  }
  if (term_state.dirty)
    vd.dirty = true;
  vd_render();
}
static void vd_shutdown(void) {
#ifdef PLAT_VITA
  plat_http_stop();
#endif
  vd_stop_media();
  vd_save_preferences();
  for (int s = 0; s < 4; s++)
    free(vd.sessions[s]);
  for (int i = 0; i < VD_MAX; i++)
    stbi_image_free(vd.windows[i].image);
  vd.active = false;
}
#endif

/* Run with the headless backend and address/undefined-behaviour sanitizers. */
#define main verdant_application_main
#include "../source/core/machine.c"
#undef main
#include <assert.h>
static void response(VDWindow *w, const char *data) {
  char p[256];
  snprintf(p, sizeof(p), VD_BRIDGE "%u.res", w->pending);
  FILE *f = fopen(p, "wb");
  assert(f);
  fputs(data, f);
  fclose(f);
  vd.last_poll = 0;
  vd_poll_bridge();
}
int main(void) {
  if (getenv("VERDANT_BOOT_GUEST")) return verdant_application_main(0, NULL);
  int64_t fine = 0, coarse = 0;
  int fine_distance = 0, coarse_distance = 0;
  for (int i = 0; i < 100; i++) fine_distance += vd_pointer_step(128, 10000, &fine);
  for (int i = 0; i < 20; i++) coarse_distance += vd_pointer_step(128, 50000, &coarse);
  assert(fine_distance == VD_W * 3 / 2);
  assert(coarse_distance == fine_distance);
  fine = 0;
  int gentle = 0;
  for (int i = 0; i < 100; i++) gentle += vd_pointer_step(1, 10000, &fine);
  assert(gentle > 0); /* Slow movement must not disappear through truncation. */
  vd_pointer_step(0, 10000, &fine);
  assert(fine == 0);
  assert(vd_pointer_step(128, 10000000, &fine) <= VD_W * 3 / 40);
  assert(plat_init());
  plat_mutex_init(&ui_lock);
  cfg_defaults(&g_cfg);
  term_init(&term_state);
  vd_init();
  FILE *profile = fopen(VD_BRIDGE "platform.txt", "rb");
  assert(profile);
  char slug[16]; assert(fgets(slug, sizeof(slug), profile)); fclose(profile);
  assert(!strncmp(slug, PLAT_SLUG, strlen(PLAT_SLUG)));
#ifdef PLAT_VITA
  assert(!strcmp(PLAT_SENSOR_LABEL, "PS Vita motion sensors"));
  assert(!strcmp(PLAT_STORAGE_LABEL, "ux0:"));
  assert(!strcmp(PLAT_GUEST_STORAGE, "/mnt/vita/ux0"));
#endif
  vd.active = true;
  g_top_refresh_us = 0;
  int edit = vd_new(VD_EDIT);
  VDWindow *w = &vd.windows[edit];
  desktop_input_byte('a');
  desktop_input_byte('b');
  desktop_input_byte('c');
  vd_navigation(0);
  desktop_input_byte('X');
  assert(!strcmp(w->text, "abXc"));
  vd_action(w, 0);
  response(w, "OK\nSaved");
  assert(!strcmp(w->text, "abXc"));
  vd_request(w, "read", "/root/notes.txt", NULL, NULL, NULL);
  response(w, "OK\nfirst\nsecond\n");
  assert(!strcmp(w->text, "first\nsecond\n"));
  assert(w->cursor == 13);
  desktop_input_byte(3);
  assert(!strcmp(vd.clipboard, w->text));
  int files = vd_new(VD_FILES);
  assert(!strcmp(vd.windows[files].path, PLAT_GUEST_STORAGE));
  VDWindow *f = &vd.windows[files];
  response(f, "OK\nD folder\nF notes.txt");
  f->selection = 1;
  char path[1024];
  vd_selected_path(f, path, sizeof(path));
  assert(!strcmp(path, PLAT_GUEST_STORAGE "/notes.txt"));
  vd_action(f, 2);
  assert(!strcmp(vd.file_clipboard, path));
  assert(!vd.cut);
  vd_action(f, 6);
  assert(vd.cut);
  int calc = vd_new(VD_CALC);
  VDWindow *c = &vd.windows[calc];
  strcpy(c->input, "2*(3+4)");
  desktop_input_byte(13);
  assert(strstr(c->text, "= 14"));
  strcpy(c->input, "1/0");
  desktop_input_byte(13);
  assert(!strcmp(c->text, "Invalid expression"));
  vd.keyboard = true;
  vd.symbols = true;
  vd_keyboard_click(VD_PANEL_X + PLAT_PANEL_W/10 + 1, PLAT_TERM_H + 30);
  assert(strchr(c->input, '@'));
  vd.keyboard = false;
  int terminal = vd_new(VD_TERM);
  VDWindow *t = &vd.windows[terminal];
  int sid = t->session;
  assert(sid >= 0);
  term_write_char(t->terminal, 'A');
  vd_move_screen(t);
  assert(t->session == sid && t->terminal->grid[0].c == 'A');
  assert(t->y >= PLAT_TERM_H);
  vd_close(terminal);
  for (int j = 0; j < 4; j++) {
    int idx = vd_new(VD_TERM);
    assert(vd.windows[idx].session >= 0);
  }
  int overflow = vd_new(VD_TERM);
  assert(vd.windows[overflow].session < 0);
  /* Actual native renderer, including both physical framebuffer sizes. */
  vd.focused = files;
  vd.windows[files].y = PLAT_TERM_H + 20;
  vd.windows[files].h = 190;
  vd.dirty = true;
  vd_render();
  vd.focused = edit;
  vd.windows[edit].x = -200;
  vd.windows[edit].y = -100;
  vd.dirty = true;
  vd_render();
  puts("Desktop state, mailbox responses, editor, file selection, calculator, keyboard, session "
       "movement, limits and clipped rendering passed.");
  vd_shutdown();
  plat_exit();
  return 0;
}

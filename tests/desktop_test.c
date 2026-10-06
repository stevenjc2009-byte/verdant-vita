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
static void rectangle_reference(int x, int y, int w, int h, uint32_t color) {
  for (int yy = y; yy < y+h; yy++)
    for (int xx = x; xx < x+w; xx++) vd_px(xx, yy, color);
}
static void raster_test(void) {
  plat_fb_t saved[2] = {vd.fb[0], vd.fb[1]};
  for (int bytes = 3; bytes <= 4; bytes++) {
    size_t size = (size_t)VD_W * VD_H * bytes;
    uint8_t *fast = malloc(size), *reference = malloc(size);
    assert(fast && reference);
    for (size_t i = 0; i < size; i++) fast[i] = reference[i] = (uint8_t)(i * 31);
    vd.fb[0] = (plat_fb_t){fast, PLAT_TERM_W, PLAT_TERM_H, bytes, VD_W*bytes, bytes};
    vd.fb[1] = (plat_fb_t){fast + (size_t)VD_W*PLAT_TERM_H*bytes, PLAT_PANEL_W, PLAT_PANEL_H, bytes, VD_W*bytes, bytes};
    vd.pan_x = 13; vd.pan_y = 17;
    for (int i = 0; i < 24; i++)
      vd_rect(-23+i*37, -13+i*29, 137+i*9, 91+i*11, 0x193a57+i);
    vd.fb[0].base = reference;
    vd.fb[1].base = reference + (size_t)VD_W*PLAT_TERM_H*bytes;
    for (int i = 0; i < 24; i++)
      rectangle_reference(-23+i*37, -13+i*29, 137+i*9, 91+i*11, 0x193a57+i);
    assert(!memcmp(fast, reference, size)); /* Includes untouched alpha bytes. */
    if (bytes == 4) {
      vd.pan_x = vd.pan_y = 0;
      uint64_t begin = plat_us();
      for (int i = 0; i < 80; i++) rectangle_reference(0, 0, VD_W, VD_H, 0x294831+i);
      uint64_t scalar = plat_us() - begin;
      begin = plat_us();
      for (int i = 0; i < 80; i++) vd_rect(0, 0, VD_W, VD_H, 0x294831+i);
      uint64_t bulk = plat_us() - begin;
      printf("Host full-frame fill benchmark: scalar=%llu us, bulk=%llu us (not Vita FPS)\n",
             (unsigned long long)scalar, (unsigned long long)bulk);
    }
    free(fast); free(reference);
  }
  vd.pan_x = vd.pan_y = 0;
  vd.fb[0] = saved[0]; vd.fb[1] = saved[1];
}
static void capture_desktop(const char *name) {
  FILE *out=fopen(name,"wb");assert(out);
  fprintf(out,"P6\n%d %d\n255\n",PLAT_TERM_W,PLAT_TERM_H+PLAT_PANEL_H);
  for(int y=0;y<PLAT_TERM_H+PLAT_PANEL_H;y++) {
    int lower=y>=PLAT_TERM_H, h=lower?PLAT_PANEL_H:PLAT_TERM_H;
    plat_fb_t *fb=&vd.fb[lower];int yy=(y-(lower?PLAT_TERM_H:0))*fb->h/h;
    for(int x=0;x<PLAT_TERM_W;x++) {
      uint8_t *p=fb->base+yy*fb->y_stride+(x*fb->w/PLAT_TERM_W)*fb->x_stride;
      uint8_t rgb[]={p[2],p[1],p[0]};fwrite(rgb,1,3,out);
    }
  }
  fclose(out);
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
  plat_surface(PLAT_SURF_TERM, &vd.fb[0]);
  plat_surface(PLAT_SURF_PANEL, &vd.fb[1]);
  vd.ui_scale=100;
  raster_test();
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
  vd.last_render=0;vd_render();
  /* The cursor-only path must restore the scene without leaving trails. */
  plat_fb_t display; assert(plat_surface(PLAT_SURF_TERM, &display));
  size_t display_bytes = (size_t)display.y_stride * display.h;
  uint8_t *scene = malloc(display_bytes); assert(scene);
  vd_cursor_restore(); memcpy(scene, display.base, display_bytes); vd_cursor_draw();
  vd.px = 240; vd.py = 150; vd.dirty = false; vd.pointer_dirty = true;
  vd.last_clock = plat_us(); vd.last_render=0;vd_render(); vd_cursor_restore();
  assert(!memcmp(scene, display.base, display_bytes));
  free(scene);
  vd.focused = edit;
  vd.windows[edit].x = -200;
  vd.windows[edit].y = -100;
  vd.dirty = true;
  vd.last_render=0;vd_render();
  puts("Desktop state, mailbox responses, editor, file selection, calculator, keyboard, session "
       "movement, limits and clipped rendering passed.");
#ifdef PLAT_VITA
  int settings=vd_new(VD_SETTINGS);vd.focused=settings;
  vd_change_scale(50);assert(vd.ui_scale==150 && VD_W==640);
  VDWindow *settings_window=&vd.windows[settings];
  vd_settings_click(settings_window,settings_window->x+settings_window->w-30,settings_window->y+32);
  assert(vd.ui_scale==175);
  vd_change_scale(25);assert(vd.ui_scale==200);
  vd.dirty=true;vd.last_render=0;vd_render();
  vd_change_scale(-50);assert(vd.ui_scale==150);
  vd.focused=settings;vd.dirty=true;vd.last_render=0;vd_render();capture_desktop("settings-150.ppm");
  vd_change_scale(-50);assert(vd.ui_scale==100);
  vd_performance_parse("CPU|33.5\nMEM|98304|32768\nNET|4.5|1.5\nDISK|2.0|3.0|100000|40000\nPROCESSES|2\nPROC|1|0.5|100|init\n");
  assert(vd_performance.memory_total==98304 && vd_performance.count==2);
  int tasks=vd_new(VD_TASKS);VDWindow *manager=&vd.windows[tasks];
  strcpy(manager->text,"PROC|1|0.5|100|init\nPROC|2|-1|200|bash\n");
  manager->task_tab=0;vd.dirty=true;vd.last_render=0;vd_render();
  manager->task_tab=1;vd_change_scale(50);
  for(int category=0;category<5;category++) { manager->task_category=category;vd.dirty=true;vd.last_render=0;vd_render();
    if(category==0)capture_desktop("tasks-150.ppm"); }
#endif
  vd_shutdown();
  plat_exit();
  return 0;
}

/* Vita 60% keyboard. One shared layout defines both rendering and hit areas. */
enum { VK_SHIFT=256,VK_CTRL,VK_ALT,VK_CAPS,VK_HIDE,VK_LEFT,VK_DOWN,VK_UP,VK_RIGHT,VK_HOME,VK_END,VK_PGUP,VK_PGDN };
typedef struct {const char *label;int code,units;} VKSpec;
typedef struct {int x,y,w,h,code;const char *label;} VKButton;
static VKButton vk_buttons[80];static int vk_count;
static int vk_touch=-1;static bool vk_tracking;static int vk_last_x,vk_last_y;
static int vd_keyboard_top(void) {return VD_H*2/5;}
static void vd_keyboard_layout(void) {
 static const VKSpec rows[][16]={
  {{"`",'`',2},{"1",'1',2},{"2",'2',2},{"3",'3',2},{"4",'4',2},{"5",'5',2},{"6",'6',2},{"7",'7',2},{"8",'8',2},{"9",'9',2},{"0",'0',2},{"-",'-',2},{"=",'=',2},{"Back",127,4}},
  {{"Tab",9,3},{"Q",'q',2},{"W",'w',2},{"E",'e',2},{"R",'r',2},{"T",'t',2},{"Y",'y',2},{"U",'u',2},{"I",'i',2},{"O",'o',2},{"P",'p',2},{"[",'[',2},{"]",']',2},{"\\",'\\',3}},
  {{"Caps",VK_CAPS,4},{"A",'a',2},{"S",'s',2},{"D",'d',2},{"F",'f',2},{"G",'g',2},{"H",'h',2},{"J",'j',2},{"K",'k',2},{"L",'l',2},{";",';',2},{"'",'\'',2},{"Enter",13,4}},
  {{"Shift",VK_SHIFT,5},{"Z",'z',2},{"X",'x',2},{"C",'c',2},{"V",'v',2},{"B",'b',2},{"N",'n',2},{"M",'m',2},{",",',',2},{".",'.',2},{"/",'/',2},{"Shift",VK_SHIFT,5}},
  {{"Ctrl",VK_CTRL,3},{"Alt",VK_ALT,3},{"Esc",27,3},{"Space",32,10},{"<",VK_LEFT,3},{"v",VK_DOWN,3},{"^",VK_UP,3},{">",VK_RIGHT,3}},
  {{"Home",VK_HOME,4},{"End",VK_END,4},{"PgUp",VK_PGUP,4},{"PgDn",VK_PGDN,4},{"Delete",-127,4},{"Hide keyboard",VK_HIDE,10}}
 };
 vk_count=0;int top=vd_keyboard_top()+18,height=VD_H-top-3;
 for(int r=0;r<6;r++) {
  int total=0;for(int c=0;c<16 && rows[r][c].label;c++)total+=rows[r][c].units;
  int units=0;for(int c=0;c<16 && rows[r][c].label;c++) {
   const VKSpec *s=&rows[r][c];int left=3+units*(VD_W-6)/total;units+=s->units;int right=3+units*(VD_W-6)/total;
   vk_buttons[vk_count++]=(VKButton){left,top+r*height/6,right-left,(r+1)*height/6-r*height/6,s->code,s->label};
  }
 }
}
static char vd_keyboard_character(int code) {
 char ch=(char)code;
 if(isalpha((unsigned char)ch)) {if(vd.shift!=vd.caps_lock)ch=(char)toupper((unsigned char)ch);}
 else if(vd.shift) {const char *normal="`1234567890-=[]\\;',./",*shifted="~!@#$%^&*()_+{}|:\"<>?";const char *p=strchr(normal,ch);if(p)ch=shifted[p-normal];}
 return ch;
}
static void vd_keyboard_draw_vita(void) {
 vd_keyboard_layout();vd_rect(0,vd_keyboard_top(),VD_W,VD_H-vd_keyboard_top(),VD_SURFACE);
 VDWindow *focused=vd_focus();
 vd_label(5,vd_keyboard_top()+5,focused && focused->entry_mode?focused->input:"Keyboard  |  Shift / Ctrl / Alt apply to the next key",VD_ACCENT,VD_W-10);
 for(int i=0;i<vk_count;i++) {
  VKButton *b=&vk_buttons[i];bool on=(b->code==VK_SHIFT && vd.shift)||(b->code==VK_CTRL && vd.ctrl)||(b->code==VK_ALT && vd.alt)||(b->code==VK_CAPS && vd.caps_lock);
  vd_rect(b->x+1,b->y+1,b->w-2,b->h-2,i==vk_touch?0x688d4f:on?0x477647:0x294634);
  char text[2]={0};const char *label=b->label;if(b->code>=33 && b->code<127){text[0]=vd_keyboard_character(b->code);label=text;}
  int tx=b->x+(b->w-(int)strlen(label)*8)/2;if(tx<b->x+2)tx=b->x+2;
  vd_label(tx,b->y+(b->h-8)/2,label,VD_TEXT_COLOR,b->x+b->w-tx-2);
 }
 if(vk_tracking){char preview[80];snprintf(preview,sizeof(preview),"Release to type: %s",vk_touch>=0?vk_buttons[vk_touch].label:"cancelled");vd_rect(0,vd_keyboard_top(),VD_W,17,VD_SURFACE);vd_label(5,vd_keyboard_top()+5,preview,VD_ACCENT,VD_W-10);}
}
static void vd_keyboard_click_vita(int x,int y) {
 vd_keyboard_layout();
 for(int i=0;i<vk_count;i++) {VKButton *b=&vk_buttons[i];if(x<b->x||x>=b->x+b->w||y<b->y||y>=b->y+b->h)continue;
  int k=b->code;
  if(k==VK_HIDE)vd.keyboard=false;
  else if(k==VK_SHIFT)vd.shift=!vd.shift;
  else if(k==VK_CTRL)vd.ctrl=!vd.ctrl;
  else if(k==VK_ALT)vd.alt=!vd.alt;
  else if(k==VK_CAPS)vd.caps_lock=!vd.caps_lock;
  else {
   if(vd.alt)rx_push(27);
   if(k>=VK_LEFT && k<=VK_PGDN)vd_navigation(k-VK_LEFT);
   else if(k==-127){
    VDWindow *w=vd_focus();
    if(w && w->app==VD_EDIT && !w->entry_mode){int n=(int)strlen(w->text),pos=w->cursor;if(pos>=0 && pos<n){int end=pos+1;while(end<n && ((unsigned char)w->text[end]&0xc0)==0x80)end++;memmove(w->text+pos,w->text+end,(size_t)(n-end+1));}}
    else if(w && w->app==VD_REMOTE && w->job && !w->entry_mode)vd_remote_event(w,"special 65535");
    else if(w && !w->terminal)rx_push(127);
    else rx_push_str("\033[3~");
   }
   else {char ch=k>=32&&k<127?vd_keyboard_character(k):(char)k;if(vd.ctrl && k>=32 && k<127)ch=(char)(toupper((unsigned char)ch)&31);rx_push(ch);}
   vd.shift=vd.ctrl=vd.alt=false;
  }
  vd.dirty=true;return;
 }
}
/* A touch commits on release. Slide into another key's interior to correct
   the choice; small boundary jitter retains the highlighted key. */
static bool vd_keyboard_touch_vita(bool down,bool tapped,int x,int y) {
 if(!vd.keyboard){vk_tracking=false;vk_touch=-1;return false;}
 if(tapped && y>=vd_keyboard_top()){vk_tracking=true;vk_touch=-1;vk_last_x=x;vk_last_y=y;}
 if(!vk_tracking)return false;
 vd_keyboard_layout();
 if(down){
  int candidate=-1;for(int i=0;i<vk_count;i++){VKButton *b=&vk_buttons[i];if(x>=b->x&&x<b->x+b->w&&y>=b->y&&y<b->y+b->h){candidate=i;break;}}
  if(candidate>=0 && candidate!=vk_touch){VKButton *b=&vk_buttons[candidate];if(vk_touch<0 || (x>=b->x+3 && x<b->x+b->w-3 && y>=b->y+3 && y<b->y+b->h-3)){vk_touch=candidate;vd.dirty=true;}}
  if(candidate<0 && vk_touch>=0){vk_touch=-1;vd.dirty=true;}
  vk_last_x=x;vk_last_y=y;
 }else{
  int selected=vk_touch;vk_tracking=false;vk_touch=-1;vd.dirty=true;
  if(selected>=0){VKButton b=vk_buttons[selected];vd_keyboard_click_vita(b.x+b.w/2,b.y+b.h/2);}
 }
 return true;
}

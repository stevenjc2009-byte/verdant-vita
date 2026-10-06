/* Native calculator panels; drawing and hit testing use the same button map. */
enum { VCB_KEY,VCB_MODE_MENU,VCB_MODE,VCB_HISTORY,VCB_DEGREES,VCB_BASE,VCB_BITS,
 VCB_UNIT,VCB_FROM,VCB_TO,VCB_SWAP,VCB_CURRENCY_FROM,VCB_CURRENCY_TO,VCB_RATES,
 VCB_DATE_ACTION,VCB_DATE_FIELD,VCB_DATE_DIGIT,VCB_GRAPH_EDIT,VCB_GRAPH_ZOOM,
 VCB_GRAPH_PAN,VCB_COPY,VCB_KEYBOARD,VCB_RECALL,VCB_FUNCTIONS,VCB_WORD_PAGE };
typedef struct {int x,y,w,h,action,value;char label[48],key[16];} VCB;
static VCB vc_buttons[80];static int vc_button_count;
static void vd_calculator_button(int x,int y,int width,int height,const char *label,const char *key,int action,int value) {
 if(vc_button_count>=80 || width<1 || height<1)return;
 VCB *b=&vc_buttons[vc_button_count++];*b=(VCB){.x=x,.y=y,.w=width,.h=height,.action=action,.value=value};
 snprintf(b->label,sizeof(b->label),"%s",label);snprintf(b->key,sizeof(b->key),"%s",key?key:"");
}
static void vd_calculator_layout(VDWindow *w,int x,int y,int width,int height) {
 VCState *c=&w->calc;vc_button_count=0;
 vd_calculator_button(x,y,74,24,"Mode",NULL,VCB_MODE_MENU,0);
 vd_calculator_button(x+width-76,y,76,24,c->history_open?"Back":"History",NULL,VCB_HISTORY,0);
 if(c->menu) {
  for(int i=0;i<VC_MODES;i++)vd_calculator_button(x+(i%2)*(width/2),y+30+(i/2)*31,width/2-5,27,vc_modes[i],NULL,VCB_MODE,i);
  return;
 }
 if(c->history_open) {
  int rows=(height-34)/27;if(rows>c->history_count)rows=c->history_count;
  for(int i=0;i<rows;i++)vd_calculator_button(x,y+32+i*27,width,24,c->history[i],NULL,VCB_RECALL,i);
  return;
 }
 int top=y+76,cols=4,rows=5;bool compact=height<245;
 const char *standard[]={"%","CE","C","Del","1/x","x^2","sqrt","/","7","8","9","*","4","5","6","-","1","2","3","+","+/-","0",".","="};
 const char *scientific[]={"sin","cos","tan","ln","log","sqrt","asin","acos","atan","abs","exp","fact","7","8","9","(",")","/","4","5","6","pi","e","*","1","2","3","^","%","-","0",".","Del","CE","=","+"};
 const char *programmer[]={"A","B","C","D","E","F","7","8","9","&","|","^","4","5","6","<<",">>","~","1","2","3","+","-","*","0","(",")","/","%","=","CE","Del"};
 const char *numeric[]={"7","8","9","Del","4","5","6","CE","1","2","3","-",".","0","+/-","="};
 const char *compact_standard[]={"CE","Del","%","/","7","8","9","*","4","5","6","-","1","2","3","+","+/-","0",".","="};
 const char *compact_numbers[]={"7","8","9","/","4","5","6","*","1","2","3","-","0",".","=","+"};
 const char *compact_functions[]={"sin","cos","tan","sqrt","ln","log","exp","fact","asin","acos","atan","^","pi","e","CE","Del"};
 const char *compact_words[]={"A","B","C","D","E","F","&","|","^","~","<<",">>","(",")","%","Del"};
 const char *word_numbers[]={"7","8","9","/","4","5","6","*","1","2","3","-","0","CE","=","+"};
 const char *date_keys[]={"7","8","9","Del","4","5","6","CE","1","2","3","-","Clear","0","Today","="};
 const char **keys=standard;int count=24;
 if(c->mode==VC_GRAPH && !c->graph_edit) {
  const char *labels[]={"Function","Zoom +","Zoom -","<",">","Up","Down"};
  int actions[]={VCB_GRAPH_EDIT,VCB_GRAPH_ZOOM,VCB_GRAPH_ZOOM,VCB_GRAPH_PAN,VCB_GRAPH_PAN,VCB_GRAPH_PAN,VCB_GRAPH_PAN};
  int values[]={0,-1,1,0,1,2,3};
  for(int i=0;i<7;i++)vd_calculator_button(x+i*width/7,y+height-27,width/7-3,24,labels[i],NULL,actions[i],values[i]);
  return;
 }
 if(c->mode==VC_SCIENTIFIC || c->mode==VC_GRAPH) {
  cols=6;rows=6;keys=scientific;count=36;
  if(compact){cols=4;rows=4;keys=c->functions_page?compact_functions:compact_numbers;count=16;}
  int tw=width/4;
  vd_calculator_button(x,top,tw-4,22,c->degrees?"DEG":"RAD",NULL,VCB_DEGREES,0);
  vd_calculator_button(x+tw,top,tw-4,22,compact?(c->functions_page?"Numbers":"Fn keys"):"x",compact?NULL:"x",compact?VCB_FUNCTIONS:VCB_KEY,0);
  vd_calculator_button(x+2*tw,top,tw-4,22,c->mode==VC_GRAPH?"Plot":"Copy",NULL,c->mode==VC_GRAPH?VCB_GRAPH_EDIT:VCB_COPY,0);
  vd_calculator_button(x+3*tw,top,tw-4,22,"Keyboard",NULL,VCB_KEYBOARD,0);top+=27;
 } else if(c->mode==VC_PROGRAMMER) {
  cols=6;rows=6;keys=programmer;count=32;
  if(compact){cols=4;rows=4;keys=c->word_page?compact_words:word_numbers;count=16;}
  char base[24],bits[24];snprintf(base,24,"%s",c->base==16?"HEX":c->base==10?"DEC":c->base==8?"OCT":"BIN");snprintf(bits,24,"%d-bit",c->bits);
  vd_calculator_button(x,top,100,24,base,NULL,VCB_BASE,0);
  vd_calculator_button(x+106,top,90,24,bits,NULL,VCB_BITS,0);
  vd_calculator_button(x+202,top,width-202,24,compact?(c->word_page?"Digits":"Bit keys"):"Copy",NULL,compact?VCB_WORD_PAGE:VCB_COPY,0);top+=compact?27:56;
 } else if(c->mode==VC_CONVERT || c->mode==VC_CURRENCY) {
  rows=4;keys=numeric;count=16;
  if(c->mode==VC_CONVERT) {
   const VCGroup *group=&vc_groups[c->unit_group];
   vd_calculator_button(x,top,width/(compact?4:3)-4,25,group->name,NULL,VCB_UNIT,0);
   vd_calculator_button(x+width/(compact?4:3),top,width/(compact?4:3)-4,25,group->units[c->from].name,NULL,VCB_FROM,0);
   vd_calculator_button(x+2*width/(compact?4:3),top,width/(compact?4:3)-4,25,group->units[c->to].name,NULL,VCB_TO,0);
  } else {
   vd_calculator_button(x,top,width/(compact?4:3)-4,25,vc_currencies[c->currency_from],NULL,VCB_CURRENCY_FROM,0);
   vd_calculator_button(x+width/(compact?4:3),top,width/(compact?4:3)-4,25,vc_currencies[c->currency_to],NULL,VCB_CURRENCY_TO,0);
   vd_calculator_button(x+2*width/(compact?4:3),top,width/(compact?4:3)-4,25,w->job||w->pending?"Loading...":"Refresh",NULL,VCB_RATES,0);
  }
  if(compact)vd_calculator_button(x+3*width/4,top,width/4-4,25,"Swap",NULL,VCB_SWAP,0);
  else vd_calculator_button(x,top+29,62,20,"Swap",NULL,VCB_SWAP,0);
  top+=compact?29:53;
 } else if(c->mode==VC_DATE) {
  keys=date_keys;count=16;rows=4;
  const char *actions[]={"Difference","Add days","Subtract days"};
  vd_calculator_button(x,y+30,width,22,actions[c->date_action],NULL,VCB_DATE_ACTION,0);
  vd_calculator_button(x,top,width/2-4,24,c->date[0],NULL,VCB_DATE_FIELD,0);
  vd_calculator_button(x+width/2,top,width/2-4,24,c->date_action?c->days:c->date[1],NULL,VCB_DATE_FIELD,c->date_action?2:1);top+=29;
 } else {
  const char *memory[]={"MC","MR","M+","M-","Copy"};
  for(int i=0;i<5;i++)vd_calculator_button(x+i*width/5,top,width/5-3,22,memory[i],memory[i],i==4?VCB_COPY:VCB_KEY,0);
  top+=27;rows=6;
  if(compact){keys=compact_standard;rows=5;count=20;}
 }
 int rh=(y+height-top)/rows,cw=width/cols;if(rh<12)rh=12;
 for(int i=0;i<count;i++) {
  const char *key=keys[i];
  vd_calculator_button(x+(i%cols)*cw,top+(i/cols)*rh,cw-3,rh-3,key,key,c->mode==VC_DATE?VCB_DATE_DIGIT:VCB_KEY,0);
 }
}
static void vd_calculator_line(int x0,int y0,int x1,int y1,uint32_t color) {
 int dx=abs(x1-x0),dy=-abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,error=dx+dy;
 for(;;){vd_px(x0,y0,color);if(x0==x1 && y0==y1)break;int e=2*error;if(e>=dy){error+=dy;x0+=sx;}if(e<=dx){error+=dx;y0+=sy;}}
}
static void vd_calculator_draw(VDWindow *w,int x,int y,int width,int height) {
 VCState *c=&w->calc;vd_calculator_layout(w,x,y,width,height);
 char heading[80];snprintf(heading,sizeof(heading),"%s",vc_modes[c->mode]);
 if(c->mode==VC_CURRENCY && height<245)snprintf(heading,sizeof(heading),"ECB %s",c->rates_date[0]?c->rates_date:"no rates");
 vd_label(x+84,y+8,heading,VD_ACCENT,width-168);
 if(!c->menu && !c->history_open) {
  if(c->mode!=VC_DATE) {
   vd_rect(x,y+30,width,41,0x0c1d15);
   const char *expr=c->expression;int max=width/8-2;size_t len=strlen(expr);if(len>(size_t)max)expr+=len-max;
   vd_label(x+7,y+34,expr,VD_ACCENT,width-14);
  }
  const char *result=c->error[0]?c->error:c->answer;
  vd_label(x+7,y+54,result,c->error[0]?0xf3aa91:VD_TEXT_COLOR,width-14);
  if(height>=245 && c->mode==VC_CONVERT)vd_label(x+70,y+108,"Tap category/units to choose",VD_ACCENT,width-74);
  if(c->mode==VC_CURRENCY && height>=245) {
   char label[144];snprintf(label,sizeof(label),"ECB %s: %s",c->rates_date[0]?c->rates_date:"no data",c->rates_status);
   vd_label(x+70,y+108,label,VD_ACCENT,width-74);
  }
  if(c->mode==VC_PROGRAMMER && height>=245) {
   uint64_t value=c->integer;vc_integer(c->expression,c->base,c->bits,&value);
   char hex[32],decimal[32],line[128];vc_integer_text(value,16,hex,sizeof(hex));vc_integer_text(value,10,decimal,sizeof(decimal));
   snprintf(line,sizeof(line),"HEX %s   DEC %s",hex,decimal);vd_label(x,y+106,line,VD_ACCENT,width);
   if(height>=245){char binary[65];vc_integer_text(value,2,binary,sizeof(binary));snprintf(line,sizeof(line),"BIN %.64s",binary);vd_label(x,y+119,line,VD_TEXT_COLOR,width);}
  }
  if(c->mode==VC_DATE)vd_label(x,y+66,c->date_field==0?"Start date: YYYY-MM-DD":c->date_field==1?"End date: YYYY-MM-DD":"Number of days",VD_ACCENT,width);
  if(c->mode==VC_GRAPH && !c->graph_edit) {
   int gx=x,gy=y+78,gw=width,gh=height-109;vd_rect(gx,gy,gw,gh,0x0c1d15);
   for(int i=1;i<10;i++){vd_rect(gx+i*gw/10,gy,1,gh,0x244332);vd_rect(gx,gy+i*gh/10,gw,1,0x244332);}
   int zero_x=(int)((-c->graph_x/c->graph_span+1)*gw/2),zero_y=(int)((c->graph_y/c->graph_span+1)*gh/2);
   if(zero_x>=0 && zero_x<gw)vd_rect(gx+zero_x,gy,1,gh,0x607e66);
   if(zero_y>=0 && zero_y<gh)vd_rect(gx,gy+zero_y,gw,1,0x607e66);
   bool previous=false;int py=0;
   for(int px=0;px<gw;px++) {
    double fx=c->graph_x+(2.0*px/(gw-1)-1)*c->graph_span,fy;
    bool valid=vc_real(c->expression,c->degrees,fx,&fy);
    double screen_y=valid?(c->graph_y+c->graph_span-fy)*gh/(2*c->graph_span):-1;
    if(!valid || screen_y<0 || screen_y>=gh){previous=false;continue;}
    int yy=(int)screen_y;
    if(previous && abs(yy-py)<gh/3)vd_calculator_line(gx+px-1,gy+py,gx+px,gy+yy,VD_ACCENT);
    else vd_px(gx+px,gy+yy,VD_ACCENT);
    py=yy;previous=true;
   }
   char bounds[96];snprintf(bounds,sizeof(bounds),"x %.3g..%.3g   y %.3g..%.3g",c->graph_x-c->graph_span,c->graph_x+c->graph_span,c->graph_y-c->graph_span,c->graph_y+c->graph_span);
   vd_label(gx+5,gy+5,bounds,VD_TEXT_COLOR,gw-10);
  }
 }
 for(int i=0;i<vc_button_count;i++) {
  VCB *b=&vc_buttons[i];uint32_t color=!strcmp(b->label,"=")?0x507b3b:0x294333;
  if(b->action==VCB_MODE && b->value==c->mode)color=0x477647;
  if(b->action==VCB_DATE_FIELD && b->value==c->date_field)color=0x477647;
  vd_rect(b->x,b->y,b->w,b->h,color);
  int label_width=(int)strlen(b->label)*8,offset=(b->w-label_width)/2;if(offset<5)offset=5;
  vd_label(b->x+offset,b->y+(b->h-8)/2,b->label,VD_TEXT_COLOR,b->w-offset-3);
 }
}
static void vd_calculator_date_key(VCState *c,const char *key) {
 char *value=c->date_field==2?c->days:c->date[c->date_field];size_t max=c->date_field==2?sizeof(c->days):11;
 if(!strcmp(key,"=")){vc_evaluate(c);return;}
 if(!strcmp(key,"Today") && c->date_field!=2) {
  time_t now=(time_t)(plat_wallclock_ms()/1000);struct tm *today=localtime(&now);if(today)strftime(value,11,"%Y-%m-%d",today);return;
 }
 if(!strcmp(key,"CE") || !strcmp(key,"C") || !strcmp(key,"Clear")){value[0]=0;return;}
 if(!strcmp(key,"Del")){size_t n=strlen(value);if(n)value[n-1]=0;return;}
 if(strlen(key)!=1 || (!isdigit((unsigned char)*key) && strcmp(key,"-")))return;
 size_t n=strlen(value);if(n+1<max){value[n]=*key;value[n+1]=0;}c->error[0]=0;
}
static void vd_calculator_click(VDWindow *w,int x,int y) {
 VCState *c=&w->calc;vd_calculator_layout(w,w->x+5,w->y+21,w->w-10,w->h-27);
 for(int i=0;i<vc_button_count;i++) {
  VCB b=vc_buttons[i];if(x<b.x || y<b.y || x>=b.x+b.w || y>=b.y+b.h)continue;
  switch(b.action) {
   case VCB_KEY:
    if(c->mode==VC_PROGRAMMER && strlen(b.key)==1 && isalnum((unsigned char)*b.key)) {
     int digit=isdigit((unsigned char)*b.key)?*b.key-'0':*b.key-'A'+10;
     if(digit>=c->base){vd_notice("Digit unavailable in this base");break;}
    }
    vc_key(c,b.key);break;
   case VCB_MODE_MENU:c->menu=!c->menu;c->history_open=false;break;
   case VCB_MODE:vc_mode(c,b.value);vd.keyboard=false;break;
   case VCB_FUNCTIONS:c->functions_page=!c->functions_page;break;
   case VCB_WORD_PAGE:c->word_page=!c->word_page;break;
   case VCB_HISTORY:c->history_open=!c->history_open;c->menu=false;break;
   case VCB_DEGREES:c->degrees=!c->degrees;break;
   case VCB_BASE:{int bases[]={16,10,8,2},next=0;for(int j=0;j<4;j++)if(bases[j]==c->base)next=(j+1)%4;
    uint64_t n;if(vc_integer(c->expression,c->base,c->bits,&n)){c->base=bases[next];vc_integer_text(n,c->base,c->expression,sizeof(c->expression));c->integer=n;vc_integer_text(n,c->base,c->answer,sizeof(c->answer));c->done=false;}else strcpy(c->error,"Evaluate or clear before changing base");break;}
   case VCB_BITS:{int bits[]={64,32,16,8};for(int j=0;j<4;j++)if(bits[j]==c->bits){c->bits=bits[(j+1)%4];break;}c->integer&=vc_mask(c->bits);break;}
   case VCB_UNIT:c->unit_group=(c->unit_group+1)%VC_GROUPS;c->from=0;c->to=1;break;
   case VCB_FROM:c->from=(c->from+1)%vc_groups[c->unit_group].count;break;
   case VCB_TO:c->to=(c->to+1)%vc_groups[c->unit_group].count;break;
   case VCB_CURRENCY_FROM:c->currency_from=(c->currency_from+1)%8;break;
   case VCB_CURRENCY_TO:c->currency_to=(c->currency_to+1)%8;break;
   case VCB_SWAP:{int *a=c->mode==VC_CURRENCY?&c->currency_from:&c->from,*z=c->mode==VC_CURRENCY?&c->currency_to:&c->to;int old=*a;*a=*z;*z=old;break;}
   case VCB_RATES:if(!w->job && !w->pending){strcpy(c->rates_status,"Connecting...");vd_request(w,"currency",NULL,NULL,NULL,NULL);}break;
   case VCB_DATE_ACTION:c->date_action=(c->date_action+1)%3;c->date_field=0;break;
   case VCB_DATE_FIELD:c->date_field=b.value;break;
   case VCB_DATE_DIGIT:vd_calculator_date_key(c,b.key);break;
   case VCB_GRAPH_EDIT:c->graph_edit=!c->graph_edit;vd.keyboard=false;break;
   case VCB_GRAPH_ZOOM:c->graph_span*=b.value<0?.5:2;if(c->graph_span<.0001)c->graph_span=.0001;if(c->graph_span>1e6)c->graph_span=1e6;break;
   case VCB_GRAPH_PAN:if(b.value<2)c->graph_x+=(b.value?1:-1)*c->graph_span/2;else c->graph_y+=(b.value==2?1:-1)*c->graph_span/2;break;
   case VCB_COPY:snprintf(vd.clipboard,sizeof(vd.clipboard),"%s",c->answer);vd_notice("Calculator result copied");break;
   case VCB_KEYBOARD:vd.keyboard=!vd.keyboard;break;
   case VCB_RECALL:{const char *end=strstr(c->history[b.value]," = ");size_t n=end?(size_t)(end-c->history[b.value]):0;if(n<sizeof(c->expression)){memcpy(c->expression,c->history[b.value],n);c->expression[n]=0;c->done=false;}c->history_open=false;break;}
  }
  if(b.action==VCB_UNIT || b.action==VCB_FROM || b.action==VCB_TO || b.action==VCB_SWAP || b.action==VCB_CURRENCY_FROM || b.action==VCB_CURRENCY_TO)vc_evaluate(c);
  vd.dirty=true;return;
 }
 if(c->mode==VC_GRAPH && !c->graph_edit && y>=w->y+99 && y<w->y+w->h-33) {
  double xx=c->graph_x+(2.0*(x-w->x-5)/(w->w-11)-1)*c->graph_span,yy;
  if(vc_real(c->expression,c->degrees,xx,&yy))snprintf(c->answer,sizeof(c->answer),"x = %.7g   y = %.7g",xx,yy);else strcpy(c->error,"Function undefined here");
 }
 vd.dirty=true;
}
static bool vd_calculator_input(VDWindow *w,unsigned char byte) {
 VCState *c=&w->calc;
 if(byte==3){snprintf(vd.clipboard,sizeof(vd.clipboard),"%s",c->answer);return true;}
 if(byte==22){vc_append(c,vd.clipboard);vd.dirty=true;return true;}
 if(c->mode==VC_DATE) {char key[2]={(char)byte,0};vd_calculator_date_key(c,byte==8||byte==127?"Del":byte==10||byte==13?"=":key);}
 else if(byte==8 || byte==127)vc_key(c,"Del");
 else if(byte==10 || byte==13)vc_evaluate(c);
 else if(byte>=32 && byte<127){char key[2]={(char)byte,0};vc_append(c,key);}
 vd.dirty=true;return true;
}

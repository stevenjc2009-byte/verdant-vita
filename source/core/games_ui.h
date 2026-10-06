static void vd_games_draw(VDWindow *w,int x,int y,int width,int height){
 VGState *g=&w->game;const char *names[]={"Games","Snake","Falling Blocks","Brick Breaker"};
 if(!g->kind){vd_label(x,y+5,"Games folder - native Vita games",VD_ACCENT,width);for(int i=1;i<=3;i++){vd_rect(x,y+30+(i-1)*37,width,32,0x31563c);vd_label(x+10,y+42+(i-1)*37,names[i],VD_TEXT_COLOR,width-20);}vd_label(x,y+147,"Touch buttons or use the D-pad. Saves high scores.",VD_ACCENT,width);return;}
 vd_button(x,y,62,"Games");vd_button(x+66,y,78,"Restart");vd_button(x+148,y,62,g->paused?"Resume":"Pause");
 char label[120];snprintf(label,sizeof(label),"%s  Score %d%s",names[g->kind],g->score,g->over?"  Game over":g->paused?"  Paused":"");vd_label(x,y+23,label,VD_ACCENT,width);
 int top=y+38,bottom=y+height-28,bh=bottom-top;
 if(g->kind==1||g->kind==2){int cols=g->kind==1?20:10,rows=g->kind==1?14:16,cell=bh/rows;if(cell>width/cols)cell=width/cols;if(cell<1)cell=1;int left=x+(width-cols*cell)/2;vd_rect(left,top,cols*cell,rows*cell,0x0c1d15);
  if(g->kind==1){vd_rect(left+g->food_x*cell,top+g->food_y*cell,cell-1,cell-1,0xdf8352);for(int i=0;i<g->len;i++)vd_rect(left+g->sx[i]*cell,top+g->sy[i]*cell,cell-1,cell-1,i?0x79ac69:0xc6ed9b);}
  else {for(int yy=0;yy<16;yy++)for(int xx=0;xx<10;xx++)if(g->grid[yy][xx])vd_rect(left+xx*cell,top+yy*cell,cell-1,cell-1,0x88c572);if(!g->over)for(int yy=0;yy<4;yy++)for(int xx=0;xx<4;xx++)if(vg_cell(g,xx,yy))vd_rect(left+(g->x+xx)*cell,top+(g->y+yy)*cell,cell-1,cell-1,0xe2c679);}
 }else{vd_rect(x,top,width,bh,0x0c1d15);for(int yy=0;yy<4;yy++)for(int xx=0;xx<10;xx++)if(g->grid[yy][xx])vd_rect(x+xx*width/10+1,top+(25+yy*14)*bh/180,width/10-2,12*bh/180,0x88c572);vd_rect(x+g->paddle*width/280,top+168*bh/180,60*width/280,4,VD_TEXT_COLOR);vd_rect(x+(int)g->ball_x*width/280-2,top+(int)g->ball_y*bh/180-2,4,4,0xe2c679);}
 const char *keys[]={"Left","Down","Up","Right","Drop"};for(int i=0;i<(g->kind==2?5:4);i++)vd_button(x+i*58,y+height-20,54,keys[i]);
}
static void vd_games_save(VGState *g){char path[200];snprintf(path,sizeof(path),PLAT_SD "verdant/games/high-score-%d.txt",g->kind);FILE *f=fopen(path,"rb");int best=0;if(f){if(fscanf(f,"%d",&best)!=1)best=0;fclose(f);}if(g->score>best){f=fopen(path,"wb");if(f){fprintf(f,"%d\n",g->score);fclose(f);}}}
static void vd_games_click(VDWindow *w,int x,int y){VGState *g=&w->game;int rx=x-w->x-5,ry=y-w->y-21,height=w->h-27;
 if(!g->kind){if(ry>=30&&ry<141){int kind=(ry-30)/37+1;vg_start(g,kind,(uint32_t)plat_us());}vd.dirty=true;return;}
 if(ry<18){if(rx<66){vd_games_save(g);g->kind=0;}else if(rx<148){vd_games_save(g);vg_start(g,g->kind,(uint32_t)plat_us());}else if(rx<210)g->paused=!g->paused;}
 else if(ry>=height-22){int key=rx/58;if(key>=0&&key<5)vg_control(g,key);}
 else if(g->kind==3){g->paddle=rx*280/(w->w-10)-30;if(g->paddle<0)g->paddle=0;if(g->paddle>220)g->paddle=220;}
 vd.dirty=true;
}
static void vd_games_tick(const plat_input_t *in){uint64_t now=plat_us();for(int i=0;i<VD_MAX;i++){VDWindow *w=&vd.windows[i];if(!w->used||w->app!=VD_GAMES)continue;VGState *g=&w->game;if(vd_window_obscured(i)||g->paused||g->over||!g->kind){g->tick=now;continue;}if(i==vd.focused){if(in->down&PLAT_BTN_LEFT)vg_control(g,0);if(in->down&PLAT_BTN_DOWN)vg_control(g,1);if(in->down&PLAT_BTN_UP)vg_control(g,2);if(in->down&PLAT_BTN_RIGHT)vg_control(g,3);if(in->down&(PLAT_BTN_LEFT|PLAT_BTN_DOWN|PLAT_BTN_UP|PLAT_BTN_RIGHT))vd.dirty=true;}
 uint64_t interval=g->kind==1?180000:g->kind==2?600000:20000;if(!g->tick)g->tick=now;if(now-g->tick>=interval){g->tick=now;vg_step(g);if(g->over)vd_games_save(g);vd.dirty=true;}}
}

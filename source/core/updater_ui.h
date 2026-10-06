static void vd_updater_draw(VDWindow *w,int x,int y,int width,int height) {
 const char *phase="Ready to check",*detail="Tap Check now to query your Vita GitHub release channel.";
#ifdef PLAT_VITA
 if(w->update_check.running) {phase="Checking for updates";detail=w->update_check.status;}
 else if(w->update_check.status[0]) {phase=w->update_check.failed?"Check failed":w->update_check.available?"Update available":"Check complete";detail=w->update_check.status;}
#endif
 if(w->pending){phase="Preparing update";detail="Waiting for the Linux service to accept the request...";}
 if(w->job) {
  phase="Starting update download";detail="Preparing the verified download. Please wait...";
  if(strstr(w->text,"Connecting")){phase="Connecting to GitHub";detail="Using the Vita's native verified HTTPS transport.";}
  if(strstr(w->text,"HTTPS:") || strstr(w->text,"Downloaded")){phase="Downloading update";detail="You can cancel this transfer without changing installed files.";}
  if(strstr(w->text,"Verifying") || strstr(w->text,"Unpacking")){phase="Verifying and staging";detail="Checking hashes and unpacking files. Linux files are preserved.";}
 }
 if(!w->job && strstr(w->text,"Up to date:")){phase="Up to date";detail="You already have the latest stable release.";}
 if(strstr(w->text,"Verified v")){phase="Ready to install";detail="Exit with Start, relaunch to apply, then launch once more.";}
 else if(strstr(w->text,"Update failed") || strstr(w->text,"timed out")){phase="Update failed";detail="See the message below. Check Wi-Fi, storage and system date/time.";}
 vd_rect(x,y,width,30,0x243e30);char installed[96];snprintf(installed,sizeof(installed),"Installed: " VU_VERSION "  /  PS Vita");vd_label(x+8,y+11,installed,VD_ACCENT,width-16);
 vd_label(x+8,y+43,phase,VD_TEXT_COLOR,width-16);vd_label(x+8,y+60,detail,VD_ACCENT,width-16);
 bool busy=w->pending || w->job;
#ifdef PLAT_VITA
 busy|=w->update_check.running;
#endif
 int bar_y=y+78;
 vd_rect(x+8,bar_y,width-16,9,0x10241d);
 unsigned long long received=0,total=0;const char *progress=NULL;
 for(const char *q=w->text;(q=strstr(q,"HTTPS:"));q+=6)progress=q;
 if(progress)sscanf(progress,"HTTPS: %llu / %llu KiB",&received,&total);
 if(busy && total && received<=total)vd_rect(x+8,bar_y,(int)((width-16)*(double)received/total),9,0x88c572);
 else if(busy) {int span=(width-16)/4,pos=(int)(plat_us()/15000)%(width-16-span);vd_rect(x+8+pos,bar_y,span,9,0x88c572);}
 else if(!strcmp(phase,"Ready to install") || !strcmp(phase,"Check complete"))vd_rect(x+8,bar_y,width-16,9,0x477647);
 vd_label(x+8,y+100,"Channel: stevenjc2009-byte/verdant-vita",VD_TEXT_COLOR,width-16);
 int yy=y+117;
 if(busy && total){char percent[80];snprintf(percent,sizeof(percent),"Download: %.0f%% (%llu / %llu KiB)",100.0*received/total,received,total);vd_label(x+8,yy,percent,VD_ACCENT,width-16);yy+=15;}
 if(w->job || strstr(w->text,"exit=") || strstr(w->text,"Verified v")) {
  const char *p=w->text;const char *last=p;
  for(const char *q=p;*q;q++)if(*q=='\n' && q[1])last=q+1;
  vd_label(x+8,yy,!strncmp(last,"running",7)?"The Linux service is preparing the download...":last,VD_ACCENT,width-16);yy+=15;
 }
 if(yy+10<y+height-39)vd_label(x+8,yy,"Downloads are verified before installation.",VD_TEXT_COLOR,width-16);
 const char *labels[]={"Check now","Download",vd.auto_update?"Auto: on":"Auto: off","Cancel"};
 int button_width=width/4;
 for(int i=0;i<4;i++) {
  int xx=x+i*button_width,by=y+height-31;
  vd_rect(xx,by,button_width-4,27,i==1?0x477647:0x294333);
  vd_label(xx+6,by+10,labels[i],VD_TEXT_COLOR,button_width-16);
 }
}
static void vd_updater_click(VDWindow *w,int x,int y) {
 int width=w->w-10,height=w->h-27,rx=x-w->x-5,ry=y-w->y-21;
 if(ry>=height-31 && ry<height-4 && rx>=0 && rx<width)vd_action(w,rx/(width/4));
}

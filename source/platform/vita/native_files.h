/* Native Vita storage operations. Linux-internal paths stay with the guest. */
#ifndef VERDANT_NATIVE_FILES_H
#define VERDANT_NATIVE_FILES_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#define VF_TEXT 15000
static bool vf_path(const char *guest,char *native,size_t size) {
 const char *relative=NULL;
 if(!guest)return false;
 const char *prefixes[]={"/mnt/vita/ux0","/mnt/3ds/sd"};
 for(int i=0;i<2;i++){size_t n=strlen(prefixes[i]);if(!strncmp(guest,prefixes[i],n) && (!guest[n] || guest[n]=='/'))relative=guest+n;}
 if(!relative)return false;
 while(*relative=='/')relative++;
 if(strlen(relative)+strlen(PLAT_SD)+1>=size)return false;
 for(const char *s=relative;*s;s++)if(*s==':' || *s=='\\' || (unsigned char)*s<32)return false;
 const char *segment=relative;
 for(const char *s=relative;;s++)if(*s=='/' || !*s){size_t n=s-segment;if((n==1 && *segment=='.') || (n==2 && !strncmp(segment,"..",2)))return false;if(!*s)break;segment=s+1;}
 if(!strcmp(relative,"verdant/Image") || !strcmp(relative,"verdant/rootfs.ext2") || !strcmp(relative,"verdant/swap.img") || !strcmp(relative,"verdant/bridge") || !strncmp(relative,"verdant/bridge/",15))return false;
 snprintf(native,size,"%s",PLAT_SD);size_t used=strlen(native);bool slash=false;
 for(const char *r=relative;*r;r++){if(*r=='/' && slash)continue;native[used++]=*r;slash=*r=='/';}if(used>strlen(PLAT_SD) && native[used-1]=='/')used--;native[used]=0;
 const char *normal=native+strlen(PLAT_SD);
 if(!strcmp(normal,"verdant/Image") || !strcmp(normal,"verdant/rootfs.ext2") || !strcmp(normal,"verdant/swap.img") || !strcmp(normal,"verdant/bridge") || !strncmp(normal,"verdant/bridge/",15))return false;
 return true;
}
static bool vf_supported(const char *op,const char *a,const char *b) {
 char path[768],dest[768];if(!vf_path(a,path,sizeof(path)))return false;
 if(!strcmp(op,"move"))return vf_path(b,dest,sizeof(dest));
 if(!strcmp(op,"copy")){struct stat st;return vf_path(b,dest,sizeof(dest)) && !stat(path,&st) && S_ISREG(st.st_mode);}
 return !strcmp(op,"exists") || !strcmp(op,"settingsbackup") || !strcmp(op,"settingsrestore") || !strcmp(op,"list") || !strcmp(op,"read") || !strcmp(op,"write") || !strcmp(op,"mkdir");
}
typedef struct {char name[256];bool directory;} VFEntry;
static int vf_game_kind(const char *guest){char path[768],line[64];int kind=0;if(!vf_path(guest,path,sizeof(path)))return 0;FILE *f=fopen(path,"rb");if(!f)return 0;bool read=fgets(line,sizeof(line),f)!=NULL;fclose(f);if(read && sscanf(line,"VERDANT-GAME %d",&kind)==1 && kind>=1 && kind<=7)return kind;return 0;}
static int vf_compare(const VFEntry *a,const VFEntry *b) {
 if(a->directory!=b->directory)return a->directory?-1:1;
 int result=strcasecmp(a->name,b->name);return result?result:strcmp(a->name,b->name);
}
static bool vf_utf8(const unsigned char *p) {
 while(*p){unsigned c=*p++;if(c<128)continue;int n=c>=0xc2&&c<=0xdf?1:c>=0xe0&&c<=0xef?2:c>=0xf0&&c<=0xf4?3:-1;if(n<0)return false;
 unsigned first=*p;if((c==0xe0&&first<0xa0)||(c==0xed&&first>=0xa0)||(c==0xf0&&first<0x90)||(c==0xf4&&first>=0x90))return false;
 for(int i=0;i<n;i++)if(!*p || (*p++&0xc0)!=0x80)return false;
 }return true;
}
static bool vf_execute(const char *op,const char *a,const char *b,char *response,size_t limit);
#include "backups.h"
static bool vf_execute(const char *op,const char *a,const char *b,char *response,size_t limit) {
 char path[768];response[0]=0;
 if(!vf_path(a,path,sizeof(path))){snprintf(response,limit,"Unsupported or reserved storage path");return false;}
 if(!strcmp(op,"exists")){struct stat st;if(!stat(path,&st)){snprintf(response,limit,"%s",S_ISDIR(st.st_mode)?"DIRECTORY":S_ISREG(st.st_mode)?"FILE":"OTHER");return true;}if(errno==ENOENT){snprintf(response,limit,"MISSING");return true;}goto error;}
 if(!strcmp(op,"settingsbackup"))return vf_settings_backup(response,limit);
 if(!strcmp(op,"settingsrestore"))return vf_settings_restore(response,limit);
 if(!strcmp(op,"list")) {
  DIR *dir=opendir(path);if(!dir)goto error;
  VFEntry *rows=calloc(128,sizeof(*rows));if(!rows){closedir(dir);goto error;}int count=0;struct dirent *entry;
  while((entry=readdir(dir))) {
   if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))continue;
   VFEntry next={0};snprintf(next.name,sizeof(next.name),"%s",entry->d_name);
   char full[1100];snprintf(full,sizeof(full),"%s/%s",path,next.name);struct stat st;next.directory=!stat(full,&st) && S_ISDIR(st.st_mode);
   for(char *s=next.name;*s;s++)if(*s=='\n' || *s=='\r')*s='?';
   int pos=0;while(pos<count && vf_compare(&rows[pos],&next)<=0)pos++;
   if(pos<128){int last=count<128?count++:127;for(int j=last;j>pos;j--)rows[j]=rows[j-1];rows[pos]=next;}
  }
  closedir(dir);size_t used=0;
  for(int i=0;i<count;i++){int n=snprintf(response+used,limit-used,"%s%c %s",i?"\n":"",rows[i].directory?'D':'F',rows[i].name);if(n<0 || (size_t)n>=limit-used)break;used+=n;}
  free(rows);return true;
 }
 if(!strcmp(op,"read")) {
  struct stat st;if(stat(path,&st))goto error;
  if(st.st_size>VF_TEXT || !S_ISREG(st.st_mode)){snprintf(response,limit,"Text editor limit is 15000 bytes; use Vim or Nano for larger files");return false;}
  FILE *f=fopen(path,"rb");if(!f)goto error;
  size_t n=fread(response,1,limit-1,f);bool ok=!ferror(f);fclose(f);response[n]=0;
  if(!ok)goto error;
  if(memchr(response,0,n) || !vf_utf8((unsigned char*)response)){snprintf(response,limit,"Binary or invalid UTF-8 file; use an image viewer or terminal");return false;}
  return true;
 }
 if(!strcmp(op,"write")) {
  if(!b || strlen(b)>VF_TEXT){snprintf(response,limit,"Text editor limit is 15000 bytes");return false;}
  if(!strcmp(a,"/mnt/vita/ux0") || !strcmp(a,"/mnt/vita/ux0/") || !strcmp(path,PLAT_SD "verdant")){snprintf(response,limit,"Storage root is reserved");return false;}
  /* Vita newlib removes an existing target before rename. Preserve the old
     document first, so a failed commit does not silently destroy it. */
  char part[800],backup[800];struct stat st;
  snprintf(part,sizeof(part),"%s.verdant-save-part",path);
  snprintf(backup,sizeof(backup),"%s.verdant-save-backup",path);
  if(!stat(backup,&st)) {
   if(stat(path,&st)){if(rename(backup,path))goto error;}
   else {snprintf(response,limit,"A saved backup already exists; recover or rename it before saving");return false;}
  }
  if(!stat(path,&st) && !S_ISREG(st.st_mode)){snprintf(response,limit,"Choose a regular text file, not a directory");return false;}
  if(!stat(part,&st)){snprintf(response,limit,"An unfinished save exists; recover or rename the .verdant-save-part file");return false;}
  if(!vf_document_snapshot(path)){snprintf(response,limit,"Previous document backup failed; save cancelled");return false;}
  FILE *f=fopen(part,"wb");if(!f)goto error;
  bool ok=fwrite(b,1,strlen(b),f)==strlen(b) && !fflush(f);if(fclose(f))ok=false;
  if(!ok){remove(part);goto error;}
  bool existed=!stat(path,&st);
  if(existed && rename(path,backup)){remove(part);goto error;}
  if(rename(part,path)){
   int failure=errno;if(existed && rename(backup,path)){snprintf(response,limit,"Save failed; previous document is retained in .verdant-save-backup");return false;}
   remove(part);errno=failure;goto error;
  }
  if(existed)remove(backup);
  snprintf(response,limit,"Saved");return true;
 }
 if(!strcmp(op,"copy") || !strcmp(op,"move")) {
  char dest[768];struct stat st;
  if(!vf_path(b,dest,sizeof(dest)) || !strcmp(path,PLAT_SD) || !strcmp(path,PLAT_SD "verdant")){snprintf(response,limit,"Reserved source or destination");return false;}
  if(!stat(dest,&st)){snprintf(response,limit,"Destination exists; choose a different name");return false;}
  if(!strcmp(op,"move")){if(rename(path,dest))goto error;snprintf(response,limit,"Complete");return true;}
  if(stat(path,&st) || !S_ISREG(st.st_mode))goto error;
  char part[800];snprintf(part,sizeof(part),"%s.native-copy-part",dest);
  FILE *in=fopen(path,"rb"),*out=in?fopen(part,"wb"):NULL;unsigned char *buffer=malloc(65536);bool ok=in && out && buffer;
  if(ok){size_t n;while((n=fread(buffer,1,65536,in)))if(fwrite(buffer,1,n,out)!=n){ok=false;break;}if(ferror(in) || ferror(out) || fflush(out))ok=false;}
  if(in)fclose(in);
  if(out && fclose(out))ok=false;
  free(buffer);
  if(!ok || !stat(dest,&st) || rename(part,dest)){remove(part);goto error;}snprintf(response,limit,"Complete");return true;
 }
 if(!strcmp(op,"mkdir")){if(mkdir(path,0777)){struct stat st;if(errno!=EEXIST || stat(path,&st) || !S_ISDIR(st.st_mode))goto error;}snprintf(response,limit,"Folder ready");return true;}
 snprintf(response,limit,"Unsupported storage operation");return false;
error:
 snprintf(response,limit,"Storage operation failed: %s",strerror(errno));return false;
}
#endif

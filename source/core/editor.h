#ifndef VERDANT_EDITOR_H
#define VERDANT_EDITOR_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#define VE_HISTORY 8
typedef struct {char *text;int cursor;} VESnapshot;
typedef struct {VESnapshot undo[VE_HISTORY],redo[VE_HISTORY];int un,rn,anchor,end,zoom,confirm,after_save;uint64_t recorded;bool selecting;uint32_t clean;char find[128],replacement[128];bool lines;} VEState;
static uint32_t ve_hash(const char *s){uint32_t h=2166136261u;while(*s)h=(h^(unsigned char)*s++)*16777619u;return h;}
static void ve_clear(VESnapshot *a,int *n){while(*n)free(a[--*n].text);}
static void ve_free(VEState *e){ve_clear(e->undo,&e->un);ve_clear(e->redo,&e->rn);}
static void ve_push(VESnapshot *a,int *n,const char *text,int cursor){char *copy=strdup(text);if(!copy)return;if(*n==VE_HISTORY){free(a[0].text);memmove(a,a+1,(VE_HISTORY-1)*sizeof(*a));(*n)--;}a[(*n)++]=(VESnapshot){copy,cursor};}
static void ve_record(VEState *e,const char *s,int cursor,uint64_t now,bool force){if(force||!e->un||now-e->recorded>700000){ve_push(e->undo,&e->un,s,cursor);e->recorded=now;}ve_clear(e->redo,&e->rn);}
static bool ve_history(VEState *e,char *s,int *cursor,bool redo){VESnapshot *from=redo?e->redo:e->undo,*to=redo?e->undo:e->redo;int *fn=redo?&e->rn:&e->un,*tn=redo?&e->un:&e->rn;if(!*fn)return false;ve_push(to,tn,s,*cursor);VESnapshot snap=from[--*fn];strcpy(s,snap.text);*cursor=snap.cursor;free(snap.text);e->anchor=e->end=*cursor;e->recorded=0;return true;}
static int ve_previous(const char *s,int pos){if(pos>0){pos--;while(pos>0&&((unsigned char)s[pos]&0xc0)==0x80)pos--;}return pos;}
static int ve_next(const char *s,int pos){if(s[pos]){pos++;while(s[pos]&&((unsigned char)s[pos]&0xc0)==0x80)pos++;}return pos;}
static void ve_bounds(VEState *e,int n,int *a,int *b){*a=e->anchor<e->end?e->anchor:e->end;*b=e->anchor>e->end?e->anchor:e->end;if(*a<0)*a=0;if(*a>n)*a=n;if(*b<0)*b=0;if(*b>n)*b=n;}
static bool ve_insert(VEState *e,char *s,int *cursor,size_t size,const char *data,uint64_t now,bool force){int n=strlen(s),a,b;ve_bounds(e,n,&a,&b);if(a==b)a=b=*cursor;size_t len=strlen(data);if(a<0||b>n||a>n||a+(n-b)+len>=size)return false;ve_record(e,s,*cursor,now,force);memmove(s+a+len,s+b,n-b+1);memcpy(s+a,data,len);*cursor=a+len;e->anchor=e->end=*cursor;return true;}
static bool ve_delete(VEState *e,char *s,int *cursor,size_t size,bool forward,uint64_t now){int a,b,n=strlen(s);ve_bounds(e,n,&a,&b);if(a==b){a=*cursor;b=*cursor;if(forward)b=ve_next(s,b);else a=ve_previous(s,a);}if(a==b)return false;e->anchor=a;e->end=b;return ve_insert(e,s,cursor,size,"",now,false);}
static bool ve_find(VEState *e,const char *s,int *cursor){if(!e->find[0])return false;const char *p=strstr(s+*cursor,e->find);if(!p)p=strstr(s,e->find);if(!p)return false;e->anchor=p-s;e->end=e->anchor+strlen(e->find);*cursor=e->end;return true;}
static int ve_replace_all(VEState *e,char *s,int *cursor,size_t size,uint64_t now){if(!e->find[0])return 0;char *out=malloc(size);if(!out)return -1;size_t used=0,fl=strlen(e->find),rl=strlen(e->replacement);const char *p=s,*match;int count=0;while((match=strstr(p,e->find))){size_t before=match-p;if(used+before+rl>=size){free(out);return -1;}memcpy(out+used,p,before);used+=before;memcpy(out+used,e->replacement,rl);used+=rl;p=match+fl;count++;}size_t rest=strlen(p);if(used+rest>=size){free(out);return -1;}memcpy(out+used,p,rest+1);if(count){ve_record(e,s,*cursor,now,true);strcpy(s,out);*cursor=0;e->anchor=e->end=0;}free(out);return count;}
#endif

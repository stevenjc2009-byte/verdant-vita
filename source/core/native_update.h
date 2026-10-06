#ifndef VERDANT_NATIVE_UPDATE_H
#define VERDANT_NATIVE_UPDATE_H
/* Direct native release check: no guest boot, Python or guest TLS on this path. */
typedef struct {bool running,available,failed;uint64_t started;char latest[48],status[256];} VUNativeCheck;
static const char *vu_json_value(const char *json,const char *key) {
 int depth=0;const char *p=json;
 while(*p) {
  if(*p=='{' || *p=='['){depth++;p++;continue;}
  if(*p=='}' || *p==']'){depth--;p++;continue;}
  if(*p!='"'){p++;continue;}
  p++;const char *start=p;bool escaped=false;
  while(*p && *p!='"'){if(*p=='\\'){escaped=true;if(p[1])p++;}p++;}
  if(!*p)return NULL;
  size_t n=p-start;p++;
  const char *next=p;while(isspace((unsigned char)*next))next++;
  if(depth==1 && !escaped && n==strlen(key) && !strncmp(start,key,n) && *next==':') {
   next++;while(isspace((unsigned char)*next))next++;return next;
  }
 }
 return NULL;
}
static bool vu_json_string(const char *json,const char *key,char *out,size_t length) {
 const char *p=vu_json_value(json,key);if(!p || *p!='"')return false;p++;size_t count=0;
 while(*p && *p!='"'){if(*p=='\\' || (unsigned char)*p<32 || count+1>=length)return false;out[count++]=*p++;}
 if(*p!='"')return false;
 out[count]=0;return true;
}
static bool vu_json_false(const char *json,const char *key) {
 const char *p=vu_json_value(json,key);return p && !strncmp(p,"false",5) && (p[5]==',' || p[5]=='}' || isspace((unsigned char)p[5]));
}
static bool vu_version_parts(const char *text,unsigned *a,unsigned *b,unsigned *c) {
 if(*text=='v')text++;
 if(!isdigit((unsigned char)*text))return false;
 int dots=0;for(const char *p=text;*p;p++){if(*p=='.'){dots++;if(!isdigit((unsigned char)p[1]))return false;}else if(!isdigit((unsigned char)*p))return false;}
 if(dots!=2)return false;
 int end=0;
 return sscanf(text,"%u.%u.%u%n",a,b,c,&end)==3 && end>0 && !text[end] && *a<100000 && *b<100000 && *c<100000;
}
static bool vu_release_newer(const char *latest,const char *current,bool *newer) {
 unsigned a,b,c,x,y,z;if(!vu_version_parts(latest,&a,&b,&c)||!vu_version_parts(current,&x,&y,&z))return false;
 *newer=a>x || (a==x && (b>y || (b==y && c>z)));return true;
}
#define VUN_HTTP PLAT_SD "verdant/bridge/host-http"
static void vu_native_cancel(VUNativeCheck *check) {
 if(!check->running)return;
 FILE *f=fopen(VUN_HTTP ".cancel","wb");if(f){fputs("1",f);fclose(f);}
 remove(VUN_HTTP ".owner");check->running=false;snprintf(check->status,sizeof(check->status),"Check cancelled");
}
static bool vu_native_start(VUNativeCheck *check) {
 if(check->running)return true;
 check->failed=true;
 if(!vu_exists(VUN_HTTP ".enabled")){strcpy(check->status,"Native network service unavailable; relaunch Verdant");return false;}
 if(vu_exists(VUN_HTTP ".req") || vu_exists(VUN_HTTP ".busy") || vu_exists(VUN_HTTP ".owner")) {
  strcpy(check->status,"Another network transfer is active. Retry after it finishes.");return false;
 }
 FILE *owner=fopen(VUN_HTTP ".owner","wb");if(!owner){strcpy(check->status,"Could not reserve update check");return false;}fputs("native-update",owner);fclose(owner);
 remove(VUN_HTTP ".res");remove(VUN_HTTP ".cancel");remove(VUN_HTTP ".data");remove(VUN_HTTP ".progress");
 FILE *request=fopen(VUN_HTTP ".req.part","wb");
 if(!request){remove(VUN_HTTP ".owner");strcpy(check->status,"Could not save update request");return false;}
 fputs("https://api.github.com/repos/stevenjc2009-byte/verdant-vita/releases/latest\n",request);
 bool ok=!ferror(request);if(fclose(request))ok=false;
 if(!ok || rename(VUN_HTTP ".req.part",VUN_HTTP ".req")){remove(VUN_HTTP ".owner");strcpy(check->status,"Could not start update check");return false;}
 check->running=true;check->failed=false;check->started=plat_us();check->available=false;
 strcpy(check->status,"Connecting to GitHub over verified HTTPS...");return true;
}
static bool vu_native_poll(VUNativeCheck *check) {
 if(!check->running)return false;
 char result[384];FILE *f=fopen(VUN_HTTP ".res","rb");
 if(!f) {
  if(plat_us()-check->started>45000000){vu_native_cancel(check);check->failed=true;strcpy(check->status,"Check timed out. Check Vita Wi-Fi and system date/time.");return true;}
  FILE *progress=fopen(VUN_HTTP ".progress","rb");
  if(progress){char buffer[128]={0};size_t got=fread(buffer,1,sizeof(buffer)-1,progress);buffer[got]=0;fclose(progress);if(buffer[0]){char status[256];snprintf(status,sizeof(status),"Checking GitHub: %.120s",buffer);if(strcmp(status,check->status)){strcpy(check->status,status);return true;}}}
  return false;
 }
 size_t n=fread(result,1,sizeof(result)-1,f);result[n]=0;fclose(f);
 check->failed=true;
 if(strncmp(result,"OK\n",3))snprintf(check->status,sizeof(check->status),"%.250s",!strncmp(result,"ERROR\n",6)?result+6:result);
 else {
  struct stat info;bool valid=false;char *json=NULL;
  if(!stat(VUN_HTTP ".data",&info) && info.st_size>0 && info.st_size<=1024*1024) {
   json=malloc((size_t)info.st_size+1);FILE *data=json?fopen(VUN_HTTP ".data","rb"):NULL;
   if(data){size_t got=fread(json,1,(size_t)info.st_size,data);json[got]=0;fclose(data);
    valid=got==(size_t)info.st_size && vu_json_false(json,"draft") && vu_json_false(json,"prerelease") && vu_json_string(json,"tag_name",check->latest,sizeof(check->latest)) && vu_release_newer(check->latest,VU_VERSION,&check->available);
   }
  }
  free(json);
  if(valid) {
   check->failed=false;
   remove(PLAT_SD "verdant/update-release.json");rename(VUN_HTTP ".data",PLAT_SD "verdant/update-release.json");
   snprintf(check->status,sizeof(check->status),check->available?"Update available: %s. Tap Download update.":"Up to date. Installed version: %s",check->available?check->latest:VU_VERSION);
  } else strcpy(check->status,"GitHub returned an invalid release response");
 }
 remove(VUN_HTTP ".res");remove(VUN_HTTP ".owner");check->running=false;return true;
}
#endif

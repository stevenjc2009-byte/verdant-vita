/* Native verified HTTPS transport for the guest updater. One fixed mailbox,
 * no guest-controlled output paths; release staging still validates SHA-256. */
#include <curl/curl.h>
#define VH_BASE PLAT_SD "verdant/bridge/host-http"
static atomic_bool vita_http_stop;
static SceUID vita_http_thread=-1;
static uint64_t vita_http_progress_tick;
static CURL *vita_http_client;
static size_t vita_http_received;
static bool vita_http_url(const char *url) {
  if(!strcmp(url,"https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml"))return true;
  const char *hosts[]={"api.github.com/","github.com/","release-assets.githubusercontent.com/","objects.githubusercontent.com/"};
  if(strncmp(url,"https://",8))return false;
  for(int i=0;i<4;i++) if(!strncmp(url+8,hosts[i],strlen(hosts[i])))return true;
  return false;
}
static void vita_http_result(const char *message) {
  FILE *f=fopen(VH_BASE ".res.part","wb");
  if(f) { fputs(message,f);fclose(f);rename(VH_BASE ".res.part",VH_BASE ".res"); }
  remove(VH_BASE ".busy");
}
static size_t vita_http_write(char *data,size_t size,size_t count,void *out) {
  size_t n=size*count;
  if(vita_http_received+n>160*1024*1024)return 0;
  vita_http_received+=n;
  return fwrite(data,1,n,(FILE*)out);
}
static int vita_http_progress(void *arg,curl_off_t total,curl_off_t got,curl_off_t u,curl_off_t v) {
  if(atomic_load(&vita_http_stop) || access(VH_BASE ".cancel",F_OK)==0)return 1;
  uint64_t now=plat_us();
  if(now-vita_http_progress_tick>1000000) {
    FILE *f=fopen(VH_BASE ".progress","wb");
    if(f) { fprintf(f,"HTTPS: %llu / %llu KiB",(unsigned long long)got/1024,(unsigned long long)total/1024);fclose(f); }
    vita_http_progress_tick=now;
  }
  return 0;
}
static size_t vita_http_header(char *data,size_t size,size_t count,void *arg) {
  size_t n=size*count;char *location=arg;
  if(n>10 && !strncasecmp(data,"Location:",9)) {
    size_t begin=9;while(begin<n && (data[begin]==' ' || data[begin]=='\t'))begin++;
    size_t len=n-begin;while(len && (data[begin+len-1]=='\r' || data[begin+len-1]=='\n'))len--;
    if(len<4096) { memcpy(location,data+begin,len);location[len]=0; }
  }
  return n;
}
static void vita_http_process(void) {
  if(rename(VH_BASE ".req",VH_BASE ".busy"))return;
  char url[4096];FILE *request=fopen(VH_BASE ".busy","rb");
  bool read=request && fgets(url,sizeof(url),request);
  if(request)fclose(request);
  if(!read) { vita_http_result("ERROR\nMissing URL");return; }
  url[strcspn(url,"\r\n")]=0;
  remove(VH_BASE ".res");remove(VH_BASE ".data");
  CURL *curl=vita_http_client;FILE *out=fopen(VH_BASE ".data.part","wb");
  if(!curl || !out) {

    if(out)fclose(out);
    vita_http_result("ERROR\nNative HTTPS allocation failed");return;
  }
  setvbuf(out,NULL,_IOFBF,65536);
  CURLcode rc=CURLE_OK;long status=0;char redirect[4096],error[CURL_ERROR_SIZE]={0};
  for(int redirects=0;redirects<9;redirects++) {
    if(!vita_http_url(url)) { rc=CURLE_URL_MALFORMAT;break; }
    rewind(out);
    if(ftruncate(fileno(out),0)) { rc=CURLE_WRITE_ERROR;break; }
    vita_http_received=0;redirect[0]=0;
    curl_easy_setopt(curl,CURLOPT_URL,url);
    curl_easy_setopt(curl,CURLOPT_CAINFO,PLAT_SD "verdant/guest/github-ca.pem");
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"Verdant-Vita/0.3.2");
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,20L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,600L);
    curl_easy_setopt(curl,CURLOPT_LOW_SPEED_TIME,30L);
    curl_easy_setopt(curl,CURLOPT_LOW_SPEED_LIMIT,128L);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,vita_http_write);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,out);
    curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,vita_http_header);
    curl_easy_setopt(curl,CURLOPT_HEADERDATA,redirect);
    curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,error);
    curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);
    curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,vita_http_progress);
    rc=curl_easy_perform(curl);
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    if(rc || status<300 || status>=400)break;
    if(!redirect[0] || !vita_http_url(redirect)) { rc=CURLE_URL_MALFORMAT;break; }
    strcpy(url,redirect);
  }
  bool good=!ferror(out) && !fflush(out);if(fclose(out))good=false;

  if(rc==CURLE_OK && status==200 && good && !rename(VH_BASE ".data.part",VH_BASE ".data"))vita_http_result("OK\n");
  else {
    remove(VH_BASE ".data.part");char message[384];
    snprintf(message,sizeof(message),"ERROR\nHTTPS failed (HTTP %ld): %.220s",status,error[0]?error:curl_easy_strerror(rc));
    vita_http_result(message);
  }
}
static int vita_http_entry(SceSize size,void *arg) {
  while(!atomic_load(&vita_http_stop)) { vita_http_process();sceKernelDelayThread(40000); }
  return 0;
}
void plat_http_start(void) {
  if(vita_http_thread>=0)return;
  remove(VH_BASE ".enabled");
  if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return;
  vita_http_client=curl_easy_init();if(!vita_http_client)return;
  atomic_store(&vita_http_stop,false);
  remove(VH_BASE ".req");remove(VH_BASE ".busy");remove(VH_BASE ".cancel");remove(VH_BASE ".res");
  SceUID thread=sceKernelCreateThread("verdant_https",vita_http_entry,0x10000110,131072,0,SCE_KERNEL_CPU_MASK_USER_1,NULL);
  if(thread<0){curl_easy_cleanup(vita_http_client);vita_http_client=NULL;return;}
  if(sceKernelStartThread(thread,0,NULL)<0) { sceKernelDeleteThread(thread);curl_easy_cleanup(vita_http_client);vita_http_client=NULL;return; }
  vita_http_thread=thread;
  FILE *f=fopen(VH_BASE ".enabled","wb");if(f) { fputs("1\n",f);fclose(f); }
}
void plat_http_stop(void) {
  if(vita_http_thread<0)return;
  atomic_store(&vita_http_stop,true);
  sceKernelWaitThreadEnd(vita_http_thread,NULL,NULL);sceKernelDeleteThread(vita_http_thread);vita_http_thread=-1;
  curl_easy_cleanup(vita_http_client);vita_http_client=NULL;
  remove(VH_BASE ".enabled");
}

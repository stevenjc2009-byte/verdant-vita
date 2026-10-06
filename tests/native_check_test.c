/* Native release metadata handling and state machine, without the Linux guest. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#define PLAT_SD "./"
#define VU_VERSION "0.3.2"
static uint64_t clock_us=1000000;
static uint64_t plat_us(void){return clock_us;}
static bool vu_exists(const char *path){return access(path,F_OK)==0;}
#include "../source/core/native_update.h"
static void write_file(const char *name,const char *text){FILE *f=fopen(name,"wb");assert(f);fputs(text,f);fclose(f);}
int main(void) {
 char tag[48];bool newer;
 assert(vu_json_string("{\"body\":\"a fake \\\"tag_name\\\":\\\"v9.9.9\\\"\",\"nested\":{\"tag_name\":\"v8.0.0\"},\"tag_name\":\"v0.3.2\"}","tag_name",tag,48));assert(!strcmp(tag,"v0.3.2"));
 assert(vu_release_newer("v0.3.3","0.3.2",&newer)&&newer);
 assert(vu_release_newer("v0.3.2","0.3.2",&newer)&&!newer);
 assert(vu_release_newer("v0.3.20","0.3.9",&newer)&&newer);
 for(int i=0;i<5;i++){const char *bad[]={"v-1.2.3","v+1.2.3","v0.3.2beta","v0.3","v0.3.2.2"};assert(!vu_release_newer(bad[i],"0.3.2",&newer));}
 assert(vu_json_false("{\"draft\":false}","draft"));assert(!vu_json_false("{\"draft\":true}","draft"));
 mkdir("verdant",0777);mkdir("verdant/bridge",0777);write_file(VUN_HTTP ".enabled","1");
 VUNativeCheck c={0};assert(vu_native_start(&c)&&c.running);assert(vu_exists(VUN_HTTP ".owner"));
 write_file(VUN_HTTP ".data","{\"tag_name\":\"v0.3.3\",\"draft\":false,\"prerelease\":false}");
 remove(VUN_HTTP ".req");write_file(VUN_HTTP ".res","OK\n");assert(vu_native_poll(&c)&&!c.running&&c.available);
 assert(vu_exists("verdant/update-release.json") && !vu_exists(VUN_HTTP ".owner"));
 assert(vu_native_start(&c));clock_us+=46000000;assert(vu_native_poll(&c)&&!c.running && strstr(c.status,"timed out"));assert(vu_exists(VUN_HTTP ".cancel"));
 remove(VUN_HTTP ".req");assert(vu_native_start(&c));vu_native_cancel(&c);assert(!c.running && !vu_exists(VUN_HTTP ".owner"));
 puts("Native updater: immediate start, strict metadata/version parsing, cached release, timeout and cancellation passed without Linux.");
}

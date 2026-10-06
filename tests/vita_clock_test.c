#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "../source/platform/vita/clock_policy.h"
static int current,calls,target;static bool fail;
static int get(void){return current;}
static int set(int value){calls++;target=value;return fail?-1:0;}
int main(void){current=500;assert(vita_cpu_floor(get,set) && !calls);current=444;assert(vita_cpu_floor(get,set) && !calls);current=333;assert(vita_cpu_floor(get,set) && calls==1 && target==444);current=-1;assert(!vita_cpu_floor(get,set) && calls==1);current=333;fail=true;assert(!vita_cpu_floor(get,set));puts("CPU clock policy preserves 500/444 MHz, only raises a lower clock and handles failed/unknown queries.");}

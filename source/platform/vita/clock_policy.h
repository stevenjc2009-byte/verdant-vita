/* Respect clocks already selected by an overclock plugin. */
static bool vita_cpu_floor(int (*get_clock)(void),int (*set_clock)(int)) {
 int current=get_clock();return current>=444 || (current>0 && set_clock(444)>=0);
}

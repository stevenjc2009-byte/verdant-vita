#include <assert.h>
#include <stdio.h>
#include "../source/core/games.h"
int main(void){VGState g;vg_start(&g,1,42);vg_control(&g,0);assert(g.next==3);g.food_x=9;g.food_y=7;vg_step(&g);assert(g.len==5&&g.score==10&&g.sx[0]==9);g.paused=true;vg_step(&g);assert(g.sx[0]==9);g.paused=false;for(int i=0;i<30&&!g.over;i++)vg_step(&g);assert(g.over);
 vg_start(&g,1,42);g.len=280;for(int i=0;i<280;i++){g.sx[i]=i%20;g.sy[i]=i/20;}vg_food(&g);assert(g.over);
 vg_start(&g,2,7);g.shape=0;g.rotation=0;g.x=3;g.y=14;for(int x=0;x<10;x++)g.grid[15][x]=x<3||x>6;assert(vg_fits(&g,3,14));assert(!vg_fits(&g,3,15));vg_lock(&g);assert(g.score==100);for(int x=0;x<10;x++)assert(!g.grid[15][x]);
 vg_start(&g,3,12);g.ball_y=167;g.ball_x=140;g.dy=2;vg_step(&g);assert(g.dy<0);g.ball_x=1;g.dx=-2;vg_step(&g);assert(g.dx>0);g.ball_y=179;g.ball_x=5;g.dy=2;vg_step(&g);assert(g.over);
 for(int kind=1;kind<=3;kind++)for(int seed=0;seed<100;seed++){vg_start(&g,kind,seed);for(int i=0;i<1000&&!g.over;i++){vg_control(&g,(int)(vg_random(&g)%5));vg_step(&g);}assert(g.len<=280);}
 puts("Native games: Snake growth/reversal/collision/full board, block collision/line clearing and randomized rotations/drops, paddle/wall/end states passed.");return 0;}

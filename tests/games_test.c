#include <assert.h>
#include <stdio.h>
#include "../source/core/games.h"
int main(void){VGState g;vg_start(&g,1,42);vg_control(&g,0);assert(g.next==3);g.food_x=9;g.food_y=7;vg_step(&g);assert(g.len==5&&g.score==10&&g.sx[0]==9);g.paused=true;vg_step(&g);assert(g.sx[0]==9);g.paused=false;for(int i=0;i<30&&!g.over;i++)vg_step(&g);assert(g.over);
 vg_start(&g,1,42);g.len=280;for(int i=0;i<280;i++){g.sx[i]=i%20;g.sy[i]=i/20;}vg_food(&g);assert(g.over);
 vg_start(&g,2,7);g.shape=0;g.rotation=0;g.x=3;g.y=14;for(int x=0;x<10;x++)g.grid[15][x]=x<3||x>6;assert(vg_fits(&g,3,14));assert(!vg_fits(&g,3,15));vg_lock(&g);assert(g.score==100);for(int x=0;x<10;x++)assert(!g.grid[15][x]);
 vg_start(&g,3,12);g.ball_y=167;g.ball_x=140;g.dy=2;vg_step(&g);assert(g.dy<0);g.ball_x=1;g.dx=-2;vg_step(&g);assert(g.dx>0);g.ball_y=179;g.ball_x=5;g.dy=2;vg_step(&g);assert(g.over);
 vg_start(&g,4,100);vg_mines_touch(&g,0,0);assert(!(g.grid[0][0]&16));int mines=0;for(int y=0;y<8;y++)for(int x=0;x<8;x++)if(g.grid[y][x]&16)mines++;assert(mines==10);for(int y=0;y<8;y++)for(int x=0;x<8;x++)if(!(g.grid[y][x]&16))vg_mines_touch(&g,x,y);assert(g.won&&g.over&&g.score==540);
 vg_start(&g,6,55);memset(g.tiles,0,sizeof(g.tiles));g.tiles[0]=g.tiles[1]=2;g.tiles[2]=g.tiles[3]=4;vg_control(&g,0);assert(g.tiles[0]==4&&g.tiles[1]==8&&g.score==12);
 vg_start(&g,5,5);g.ball_x=8;g.ball_y=g.paddle+20;g.dx=-2;vg_step(&g);assert(g.dx>0&&g.score==10);g.ball_x=-3;g.ball_y=1;g.dx=-2;vg_step(&g);assert(g.lives==2);
 vg_start(&g,7,7);int seen[52]={0};for(int c=0;c<7;c++)for(int r=0;r<g.card_count[c];r++)seen[g.cards[c][r]]++;for(int i=0;i<g.stock_count;i++)seen[g.stock[i]]++;for(int i=0;i<52;i++)assert(seen[i]==1);vg_sol_deal(&g);assert(g.stock_count==23&&g.waste_count==1);g.waste[0]=0;g.card_source=7;g.card_row=0;assert(vg_sol_move(&g,7)&&g.foundation[0]==1&&!g.waste_count);g.waste[0]=12;g.waste_count=1;g.card_count[0]=0;g.card_source=7;g.card_row=0;assert(vg_sol_move(&g,0)&&g.cards[0][0]==12);assert(!vg_sol_move(&g,1));
 for(int kind=1;kind<=7;kind++)for(int seed=0;seed<100;seed++){vg_start(&g,kind,seed);for(int i=0;i<1000&&!g.over;i++){vg_control(&g,(int)(vg_random(&g)%5));if(kind==7){vg_sol_deal(&g);vg_sol_select(&g,7,0);vg_sol_move(&g,vg_random(&g)%11);}if(kind==4)vg_mines_touch(&g,vg_random(&g)%8,vg_random(&g)%8);vg_step(&g);}assert(g.len<=280);}
 puts("Native games: Snake growth/reversal/collision/full board, block collision/line clearing and randomized rotations/drops, paddle/wall/end states passed.");return 0;}

#ifndef VERDANT_GAMES_H
#define VERDANT_GAMES_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
typedef struct {int kind,score,len,dir,next,food_x,food_y,x,y,shape,rotation,paddle;bool paused,over;uint32_t random;uint64_t tick;unsigned char grid[16][20],sx[280],sy[280];float ball_x,ball_y,dx,dy;} VGState;
static uint32_t vg_random(VGState *g){g->random=g->random*1664525u+1013904223u;return g->random;}
static void vg_food(VGState *g){int empty[280],count=0;for(int y=0;y<14;y++)for(int x=0;x<20;x++){bool used=false;for(int i=0;i<g->len;i++)if(g->sx[i]==x&&g->sy[i]==y)used=true;if(!used)empty[count++]=y*20+x;}if(!count){g->over=true;return;}int cell=empty[vg_random(g)%count];g->food_x=cell%20;g->food_y=cell/20;}
static const uint16_t vg_shapes[]={0x00f0,0x0066,0x0072,0x0036,0x0063,0x0071,0x0074};
static bool vg_cell(const VGState *g,int x,int y){for(int r=0;r<(g->rotation%4);r++){int tmp=x;x=y;y=3-tmp;}return (vg_shapes[g->shape]>>(y*4+x))&1;}
static bool vg_fits(VGState *g,int x,int y){for(int yy=0;yy<4;yy++)for(int xx=0;xx<4;xx++)if(vg_cell(g,xx,yy)){int dx=x+xx,dy=y+yy;if(dx<0||dx>=10||dy<0||dy>=16||g->grid[dy][dx])return false;}return true;}
static void vg_spawn(VGState *g){g->shape=(int)(vg_random(g)%7);g->rotation=0;g->x=3;g->y=0;if(!vg_fits(g,g->x,g->y))g->over=true;}
static void vg_start(VGState *g,int kind,uint32_t seed){memset(g,0,sizeof(*g));g->kind=kind;g->random=seed;g->dir=g->next=3;g->len=4;for(int i=0;i<4;i++){g->sx[i]=8-i;g->sy[i]=7;}if(kind==1)vg_food(g);if(kind==2)vg_spawn(g);if(kind==3){g->paddle=110;g->ball_x=140;g->ball_y=150;g->dx=2;g->dy=-2;for(int y=0;y<4;y++)for(int x=0;x<10;x++)g->grid[y][x]=1;}}
static void vg_lock(VGState *g){for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(vg_cell(g,x,y))g->grid[g->y+y][g->x+x]=(unsigned char)(g->shape+1);for(int y=15;y>=0;y--){bool full=true;for(int x=0;x<10;x++)if(!g->grid[y][x])full=false;if(full){for(int j=y;j>0;j--)memcpy(g->grid[j],g->grid[j-1],20);memset(g->grid[0],0,20);g->score+=100;y++;}}vg_spawn(g);}
static void vg_step(VGState *g){
 if(g->paused||g->over||!g->kind)return;
 if(g->kind==1){static const int dx[]={-1,0,0,1},dy[]={0,1,-1,0};g->dir=g->next;int x=g->sx[0]+dx[g->dir],y=g->sy[0]+dy[g->dir];bool grow=x==g->food_x&&y==g->food_y;if(x<0||x>=20||y<0||y>=14){g->over=true;return;}for(int i=0;i<g->len-(grow?0:1);i++)if(g->sx[i]==x&&g->sy[i]==y){g->over=true;return;}int last=grow?g->len:g->len-1;for(int i=last;i>0;i--){g->sx[i]=g->sx[i-1];g->sy[i]=g->sy[i-1];}g->sx[0]=(unsigned char)x;g->sy[0]=(unsigned char)y;if(grow){g->len++;g->score+=10;vg_food(g);}}
 else if(g->kind==2){if(vg_fits(g,g->x,g->y+1))g->y++;else vg_lock(g);}
 else {g->ball_x+=g->dx;g->ball_y+=g->dy;if(g->ball_x<3){g->ball_x=3;g->dx=-g->dx;}if(g->ball_x>277){g->ball_x=277;g->dx=-g->dx;}if(g->ball_y<3){g->ball_y=3;g->dy=-g->dy;}if(g->dy>0&&g->ball_y>=165&&g->ball_y<=172&&g->ball_x>=g->paddle-3&&g->ball_x<=g->paddle+63){g->ball_y=165;g->dy=-g->dy;}int col=(int)g->ball_x/28,row=((int)g->ball_y-25)/14;if(g->ball_y>=25&&row>=0&&row<4&&col>=0&&col<10&&g->grid[row][col]){g->grid[row][col]=0;g->score+=10;g->dy=-g->dy;if(g->score==400)g->over=true;}if(g->ball_y>180)g->over=true;}
}
static void vg_control(VGState *g,int key){
 if(g->paused||g->over)return;
 if(g->kind==1){static const int opposite[]={3,2,1,0};if(key<4&&key>=0&&key!=opposite[g->dir])g->next=key;}
 else if(g->kind==2){if(key==0||key==3){int dx=key==0?-1:1;if(vg_fits(g,g->x+dx,g->y))g->x+=dx;}else if(key==1){vg_step(g);g->score++;}else if(key==2){int old=g->rotation;g->rotation=(old+1)%4;static const int kicks[]={0,-1,1,-2,2};bool fits=false;for(int i=0;i<5;i++)if(vg_fits(g,g->x+kicks[i],g->y)){g->x+=kicks[i];fits=true;break;}if(!fits)g->rotation=old;}else if(key==4){while(vg_fits(g,g->x,g->y+1)){g->y++;g->score+=2;}vg_lock(g);}}
 else if(g->kind==3){g->paddle+=key==0?-20:key==3?20:0;if(g->paddle<0)g->paddle=0;if(g->paddle>220)g->paddle=220;}
}
#endif

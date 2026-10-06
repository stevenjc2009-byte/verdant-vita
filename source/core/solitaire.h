#ifndef VERDANT_SOLITAIRE_H
#define VERDANT_SOLITAIRE_H
static int vg_rank(int card){return card%13+1;}
static bool vg_red(int card){return card/13==1||card/13==2;}
static void vg_sol_start(VGState *g){unsigned char deck[52];for(int i=0;i<52;i++)deck[i]=i;for(int i=51;i>0;i--){int j=vg_random(g)%(i+1);unsigned char tmp=deck[i];deck[i]=deck[j];deck[j]=tmp;}int at=0;for(int c=0;c<7;c++){g->card_count[c]=c+1;g->face[c]=c;for(int r=0;r<=c;r++)g->cards[c][r]=deck[at++];}for(int i=0;i<24;i++)g->stock[i]=deck[at++];g->stock_count=24;g->card_source=-1;}
static void vg_sol_deal(VGState *g){if(g->paused||g->over)return;g->card_source=-1;if(g->stock_count)g->waste[g->waste_count++]=g->stock[--g->stock_count];else {for(int i=0;i<g->waste_count;i++)g->stock[i]=g->waste[g->waste_count-1-i];g->stock_count=g->waste_count;g->waste_count=0;}}
static bool vg_sol_select(VGState *g,int col,int row){if(g->paused||g->over)return false;if(col==7){if(!g->waste_count)return false;g->card_source=7;g->card_row=g->waste_count-1;return true;}if(col<0||col>=7||row<g->face[col]||row>=g->card_count[col])return false;g->card_source=col;g->card_row=row;return true;}
static bool vg_sol_move(VGState *g,int dest){if(g->paused||g->over||g->card_source<0)return false;int src=g->card_source,row=g->card_row,card=src==7?g->waste[row]:g->cards[src][row],count=src==7?1:g->card_count[src]-row;if(dest<7&&dest==src)return false;
 if(dest>=7&&dest<11){int suit=dest-7;if(count!=1||card/13!=suit||vg_rank(card)!=g->foundation[suit]+1)return false;g->foundation[suit]++;g->score+=10;}
 else if(dest>=0&&dest<7){int n=g->card_count[dest];if(n+count>20)return false;if(n){int top=g->cards[dest][n-1];if(vg_red(top)==vg_red(card)||vg_rank(top)!=vg_rank(card)+1)return false;}else if(vg_rank(card)!=13)return false;if(src==7)g->cards[dest][n]=card;else memcpy(g->cards[dest]+n,g->cards[src]+row,count);g->card_count[dest]+=count;}
 else return false;
 if(src==7)g->waste_count--;else {g->card_count[src]=row;if(row&&g->face[src]>=row){g->face[src]=row-1;g->score+=5;}}g->card_source=-1;int total=0;for(int i=0;i<4;i++)total+=g->foundation[i];if(total==52)g->over=g->won=true;return true;}
#endif

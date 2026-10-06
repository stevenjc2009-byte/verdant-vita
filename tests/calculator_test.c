/* Math engine validation; no UI or hardware bindings. */
#include <assert.h>
#include "../source/core/calculator.h"
static void close_to(double actual,double expected) {assert(fabs(actual-expected)<=1e-9*fmax(1,fabs(expected)));}
int main(void) {
 double n;uint64_t word;
 assert(vc_real("2*(3+4)",true,0,&n));close_to(n,14);
 assert(vc_real("-2^2",true,0,&n));close_to(n,-4);
 assert(vc_real("2^3^2",true,0,&n));close_to(n,512);
 assert(vc_real("sin(30)+cos(60)",true,0,&n));close_to(n,1);
 assert(vc_real("sin(pi/2)",false,0,&n));close_to(n,1);
 assert(vc_real("sqrt(81)+ln(e)+fact(5)",true,0,&n));close_to(n,130);
 assert(vc_real("x^2+2*x+1",true,3,&n));close_to(n,16);
 for(int i=0;i<6;i++){const char *bad[]={"1/0","sqrt(-1)","log(0)","tan(90)","fact(1.5)","2garbage"};assert(!vc_real(bad[i],true,0,&n));}
 char nested[256];memset(nested,'(',100);nested[100]='1';memset(nested+101,')',100);nested[201]=0;assert(!vc_real(nested,true,0,&n));
 assert(vc_integer("18446744073709551615",10,64,&word) && word==UINT64_MAX);
 assert(vc_integer("FFFFFFFFFFFFFFFF+1",16,64,&word) && word==0);
 assert(vc_integer("255+1",10,8,&word) && word==0);
 assert(vc_integer("~0",10,32,&word) && word==UINT32_MAX);
 assert(vc_integer("(15&7)<<2",10,64,&word) && word==28);
 assert(vc_integer("-1",10,16,&word) && word==65535);
 assert(!vc_integer("1<<64",10,64,&word));assert(!vc_integer("8",8,64,&word));assert(!vc_integer("1/0",10,64,&word));
 assert(!vc_integer("18446744073709551616",10,64,&word));
 char hex[65];vc_integer_text(UINT64_MAX,16,hex,sizeof(hex));assert(!strcmp(hex,"FFFFFFFFFFFFFFFF"));
 int y,m,d;assert(vc_date_parse("2024-02-29",&y,&m,&d));assert(!vc_date_parse("2023-02-29",&y,&m,&d));assert(!vc_date_parse("1900-02-29",&y,&m,&d));assert(vc_date_parse("2000-02-29",&y,&m,&d));
 for(int year=1;year<=9999;year+=37)for(int month=1;month<=12;month++){
  char date[11];assert(vc_date_from_days(vc_date_days(year,month,15),date));assert(vc_date_parse(date,&y,&m,&d));assert(y==year && m==month && d==15);
 }
 assert(vc_convert(1,0,7,0,&n));close_to(n,1609.344);
 assert(vc_convert(32,2,1,0,&n));close_to(n,0);
 assert(vc_convert(100,2,0,1,&n));close_to(n,212);assert(!vc_convert(-274,2,0,1,&n));
 assert(vc_convert(1,9,2,0,&n));close_to(n,1048576);
 VCState c;vc_init(&c);vc_key(&c,"2");vc_key(&c,"+");vc_key(&c,"3");vc_key(&c,"=");close_to(c.value,5);vc_key(&c,"=");close_to(c.value,8);
 vc_key(&c,"CE");vc_append(&c,"200+10");vc_key(&c,"%");close_to(c.value,220);vc_key(&c,"=");close_to(c.value,220);
 vc_key(&c,"CE");vc_append(&c,"90");vc_key(&c,"sin");close_to(c.value,1);
 vc_key(&c,"M+");vc_key(&c,"CE");vc_key(&c,"MR");vc_key(&c,"=");close_to(c.value,1);
 vc_mode(&c,VC_PROGRAMMER);c.base=16;vc_key(&c,"C");vc_key(&c,"=");assert(c.integer==12);
 vc_mode(&c,VC_DATE);strcpy(c.date[0],"2024-02-28");strcpy(c.date[1],"2024-03-01");vc_evaluate(&c);assert(strstr(c.answer,"2 days"));c.date_action=1;strcpy(c.days,"1");vc_evaluate(&c);assert(!strcmp(c.answer,"2024-02-29"));
 vc_mode(&c,VC_CURRENCY);vc_evaluate(&c);assert(c.error[0]);c.rates[1]=.8;strcpy(c.expression,"10");vc_evaluate(&c);close_to(c.value,8);
 VCState graph;vc_init(&graph);strcpy(graph.expression,"x^2");vc_graph_prepare(&graph,320,180);assert(graph.graph_samples==1);vc_graph_prepare(&graph,320,180);assert(graph.graph_samples==1);graph.graph_x+=1;vc_graph_prepare(&graph,320,180);assert(graph.graph_samples==2);strcpy(graph.expression,"sin(x)");vc_graph_prepare(&graph,320,180);assert(graph.graph_samples==3);
 puts("Calculator arithmetic, scientific/graph functions, 64-bit words, dates, units, currency, memory, percent and repeated equals passed.");
}

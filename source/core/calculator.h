#ifndef VERDANT_CALCULATOR_H
#define VERDANT_CALCULATOR_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#define VC_PI 3.14159265358979323846
#define VC_HISTORY 8
enum { VC_STANDARD,VC_SCIENTIFIC,VC_GRAPH,VC_PROGRAMMER,VC_DATE,VC_CONVERT,VC_CURRENCY,VC_MODES };
static const char *vc_modes[]={"Standard","Scientific","Graphing","Programmer","Date calculation","Unit converter","Currency"};
typedef struct {
 int mode,base,bits,unit_group,from,to,currency_from,currency_to,date_action,date_field;
 bool degrees,done,menu,history_open,graph_edit,functions_page,word_page;
 double value,memory,graph_span,graph_x,graph_y,repeat_value;
 char repeat_op;
 uint64_t integer;
 char expression[256],answer[128],error[128],saved[VC_MODES][256];
 char history[VC_HISTORY][384];int history_count;
 char date[2][11],days[16];
 double rates[8];char rates_date[16],rates_status[96];
} VCState;
static const char *vc_currencies[]={"EUR","GBP","USD","CAD","JPY","AUD","CHF","CNY"};
typedef struct {const char *p;bool error,degrees;int depth;double x;} VCFParser;
static double vc_fsum(VCFParser *p);
static void vc_space(const char **p) {while(isspace((unsigned char)**p))(*p)++;}
static double vc_factorial(double n) { if(n<0 || n>170 || floor(n)!=n)return NAN;double r=1;for(int i=2;i<=(int)n;i++)r*=i;return r; }
static double vc_primary(VCFParser *p) {
 vc_space(&p->p);if(++p->depth>32){p->error=true;p->depth--;return 0;}
 double result=0;
 if(*p->p=='(') {p->p++;result=vc_fsum(p);vc_space(&p->p);if(*p->p!=')')p->error=true;else p->p++;}
 else if(isalpha((unsigned char)*p->p)) {
  char name[16];int n=0;while(isalpha((unsigned char)*p->p)){if(n<15)name[n++]=*p->p;p->p++;}name[n]=0;
  if(!strcmp(name,"pi"))result=VC_PI;
  else if(!strcmp(name,"e"))result=2.71828182845904523536;
  else if(!strcmp(name,"x"))result=p->x;
  else {
   vc_space(&p->p);if(*p->p!='(')p->error=true;
   else {p->p++;double a=vc_fsum(p);vc_space(&p->p);if(*p->p!=')')p->error=true;else p->p++;
    double angle=p->degrees?a*VC_PI/180:a;
    if(!strcmp(name,"sin"))result=sin(angle);
    else if(!strcmp(name,"cos"))result=cos(angle);
    else if(!strcmp(name,"tan"))result=fabs(cos(angle))<1e-14?NAN:tan(angle);
    else if(!strcmp(name,"asin"))result=asin(a)*(p->degrees?180/VC_PI:1);
    else if(!strcmp(name,"acos"))result=acos(a)*(p->degrees?180/VC_PI:1);
    else if(!strcmp(name,"atan"))result=atan(a)*(p->degrees?180/VC_PI:1);
    else if(!strcmp(name,"sqrt"))result=sqrt(a);
    else if(!strcmp(name,"log"))result=log10(a);
    else if(!strcmp(name,"ln"))result=log(a);
    else if(!strcmp(name,"exp"))result=exp(a);
    else if(!strcmp(name,"abs"))result=fabs(a);
    else if(!strcmp(name,"floor"))result=floor(a);
    else if(!strcmp(name,"ceil"))result=ceil(a);
    else if(!strcmp(name,"fact"))result=vc_factorial(a);
    else p->error=true;
   }
  }
 } else {char *end;result=strtod(p->p,&end);if(end==p->p)p->error=true;else p->p=end;}
 p->depth--;return result;
}
static double vc_unary(VCFParser *p);
static double vc_power(VCFParser *p) {
 double a=vc_primary(p);vc_space(&p->p);
 if(*p->p=='^'){p->p++;if(++p->depth>32){p->error=true;p->depth--;return 0;}a=pow(a,vc_unary(p));p->depth--;}
 return a;
}
static double vc_unary(VCFParser *p) {
 vc_space(&p->p);
 if(*p->p=='+' || *p->p=='-') {
  char sign=*p->p++;if(++p->depth>32){p->error=true;p->depth--;return 0;}
  double a=vc_unary(p);p->depth--;return sign=='-'?-a:a;
 }
 return vc_power(p);
}
static double vc_fproduct(VCFParser *p) {
 double a=vc_unary(p);for(;;){vc_space(&p->p);char op=*p->p;if(op!='*' && op!='/' && op!='%')break;
  p->p++;double b=vc_unary(p);if(op=='*')a*=b;else if(b==0){p->error=true;return 0;}else a=op=='/'?a/b:fmod(a,b);
 }return a;
}
static double vc_fsum(VCFParser *p) {
 double a=vc_fproduct(p);for(;;){vc_space(&p->p);char op=*p->p;if(op!='+' && op!='-')break;
 p->p++;double b=vc_fproduct(p);a=op=='+'?a+b:a-b;}return a;
}
static bool vc_real(const char *expr,bool degrees,double x,double *out) {
 VCFParser p={.p=expr,.degrees=degrees,.x=x};double n=vc_fsum(&p);vc_space(&p.p);
 if(p.error || *p.p || !isfinite(n))return false;
 *out=n;return true;
}
/* Unsigned word arithmetic never passes through floating point. */
typedef struct {const char *p;int base,depth;uint64_t mask;bool error;} VCIntegerParser;
static uint64_t vc_integer_expr(VCIntegerParser *p,int minimum);
static uint64_t vc_integer_atom(VCIntegerParser *p) {
 vc_space(&p->p);if(++p->depth>32){p->error=true;p->depth--;return 0;}uint64_t n=0;
 if(*p->p=='('){p->p++;n=vc_integer_expr(p,1);vc_space(&p->p);if(*p->p!=')')p->error=true;else p->p++;}
 else if(*p->p=='~' || *p->p=='-' || *p->p=='+'){char op=*p->p++;n=vc_integer_atom(p);if(op=='~')n=~n;else if(op=='-')n=0-n;}
 else {
  int count=0;
  while(isalnum((unsigned char)*p->p)) {
   int digit=isdigit((unsigned char)*p->p)?*p->p-'0':toupper((unsigned char)*p->p)-'A'+10;
   if(digit<0 || digit>=p->base || n>(UINT64_MAX-(unsigned)digit)/(unsigned)p->base){p->error=true;break;}
   n=n*p->base+digit;p->p++;count++;
  }
  if(!count)p->error=true;
 }
 p->depth--;return n&p->mask;
}
static uint64_t vc_integer_expr(VCIntegerParser *p,int minimum) {
 uint64_t a=vc_integer_atom(p);
 for(;;){vc_space(&p->p);char op=*p->p;int priority=0,len=1;
  if(op=='|')priority=1;else if(op=='^')priority=2;else if(op=='&')priority=3;
  else if((op=='<' && p->p[1]=='<') || (op=='>' && p->p[1]=='>')){priority=4;len=2;}
  else if(op=='+' || op=='-')priority=5;else if(op=='*' || op=='/' || op=='%')priority=6;
  if(priority<minimum)break;
  p->p+=len;uint64_t b=vc_integer_expr(p,priority+1);
  switch(op){case '|':a|=b;break;case '^':a^=b;break;case '&':a&=b;break;
   case '<':case '>':if(b>=64)p->error=true;else a=op=='<'?a<<b:a>>b;break;
   case '+':a+=b;break;case '-':a-=b;break;case '*':a*=b;break;
   case '/':case '%':if(!b)p->error=true;else a=op=='/'?a/b:a%b;break;
  }a&=p->mask;
 }
 return a;
}
static uint64_t vc_mask(int bits){return bits==64?UINT64_MAX:(UINT64_C(1)<<bits)-1;}
static bool vc_integer(const char *expr,int base,int bits,uint64_t *out) {
 VCIntegerParser p={.p=expr,.base=base,.mask=vc_mask(bits)};uint64_t n=vc_integer_expr(&p,1);vc_space(&p.p);
 if(p.error || *p.p)return false;
 *out=n;return true;
}
static void vc_integer_text(uint64_t n,int base,char *out,size_t length) {
 char reverse[65];int count=0;do {reverse[count++]="0123456789ABCDEF"[n%base];n/=base;}while(n && count<64);
 size_t used=0;while(count && used+1<length)out[used++]=reverse[--count];out[used]=0;
}
/* Proleptic Gregorian dates, with strict leap-day validation (years 1..9999). */
static bool vc_date_parse(const char *text,int *y,int *m,int *d) {
 int tail=0;if(strlen(text)!=10 || sscanf(text,"%4d-%2d-%2d%n",y,m,d,&tail)!=3 || tail!=10 || text[4]!='-' || text[7]!='-')return false;
 for(int i=0;i<10;i++)if(i!=4 && i!=7 && !isdigit((unsigned char)text[i]))return false;
 if(*y<1 || *y>9999 || *m<1 || *m>12)return false;
 int month[]={31,28,31,30,31,30,31,31,30,31,30,31};month[1]+=(*y%4==0 && (*y%100!=0 || *y%400==0));return *d>=1 && *d<=month[*m-1];
}
static int64_t vc_date_days(int y,int m,int d) {
 y-=m<=2;int era=y/400;unsigned yo=y-era*400,mp=m>2?m-3:m+9;
 return (int64_t)era*146097+yo*365+yo/4-yo/100+(153*mp+2)/5+d-1-719468;
}
static bool vc_date_from_days(int64_t days,char out[11]) {
 days+=719468;int64_t era=(days>=0?days:days-146096)/146097;unsigned doe=days-era*146097;
 unsigned yo=(doe-doe/1460+doe/36524-doe/146096)/365;int y=yo+era*400;
 unsigned doy=doe-(365*yo+yo/4-yo/100),mp=(5*doy+2)/153,d=doy-(153*mp+2)/5+1;int m=mp<10?mp+3:mp-9;y+=m<=2;
 if(y<1 || y>9999 || m<1 || m>12 || d<1 || d>31)return false;
 snprintf(out,11,"%04d-%02d-%02u",y,m,d);return true;
}
typedef struct {const char *name;double factor,offset;} VCUnit;
typedef struct {const char *name;int count;VCUnit units[8];} VCGroup;
static const VCGroup vc_groups[]={
 {"Length",8,{{"metres",1,0},{"kilometres",1000,0},{"centimetres",.01,0},{"millimetres",.001,0},{"inches",.0254,0},{"feet",.3048,0},{"yards",.9144,0},{"miles",1609.344,0}}},
 {"Mass",6,{{"kilograms",1,0},{"grams",.001,0},{"milligrams",.000001,0},{"pounds",.45359237,0},{"ounces",.028349523125,0},{"tonnes",1000,0}}},
 {"Temperature",3,{{"Celsius",1,273.15},{"Fahrenheit",5.0/9,255.37222222222222},{"Kelvin",1,0}}},
 {"Area",6,{{"square metres",1,0},{"square km",1000000,0},{"square cm",.0001,0},{"square feet",.09290304,0},{"acres",4046.8564224,0},{"hectares",10000,0}}},
 {"Volume",6,{{"litres",1,0},{"millilitres",.001,0},{"cubic metres",1000,0},{"US gallons",3.785411784,0},{"UK gallons",4.54609,0},{"US fluid oz",.0295735295625,0}}},
 {"Speed",4,{{"metres/sec",1,0},{"km/hour",1.0/3.6,0},{"miles/hour",.44704,0},{"knots",.5144444444444444,0}}},
 {"Time",5,{{"seconds",1,0},{"minutes",60,0},{"hours",3600,0},{"days",86400,0},{"weeks",604800,0}}},
 {"Energy",6,{{"joules",1,0},{"kilojoules",1000,0},{"calories",4.184,0},{"kilocalories",4184,0},{"watt hours",3600,0},{"kilowatt hours",3600000,0}}},
 {"Pressure",5,{{"pascals",1,0},{"kilopascals",1000,0},{"bar",100000,0},{"atmospheres",101325,0},{"psi",6894.757293168,0}}},
 {"Data",6,{{"bytes",1,0},{"KiB",1024,0},{"MiB",1048576,0},{"GiB",1073741824,0},{"TiB",1099511627776.,0},{"bits",.125,0}}},
 {"Angle",3,{{"degrees",VC_PI/180,0},{"radians",1,0},{"gradians",VC_PI/200,0}}},
 {"Power",3,{{"watts",1,0},{"kilowatts",1000,0},{"horsepower",745.6998715822702,0}}}
};
#define VC_GROUPS ((int)(sizeof(vc_groups)/sizeof(vc_groups[0])))
static bool vc_convert(double value,int group,int from,int to,double *answer) {
 if(group<0 || group>=VC_GROUPS || from<0 || to<0 || from>=vc_groups[group].count || to>=vc_groups[group].count)return false;
 const VCUnit *a=&vc_groups[group].units[from],*b=&vc_groups[group].units[to];double canonical=value*a->factor+a->offset;
 if(group==2 && canonical<0)return false;
 *answer=(canonical-b->offset)/b->factor;return isfinite(*answer);
}
static void vc_init(VCState *c) {
 memset(c,0,sizeof(*c));c->base=10;c->bits=64;c->degrees=true;c->graph_span=10;c->to=1;c->currency_to=1;
 strcpy(c->answer,"0");strcpy(c->expression,"0");strcpy(c->saved[VC_GRAPH],"x^2");strcpy(c->date[0],"2026-01-01");strcpy(c->date[1],"2026-01-01");strcpy(c->days,"0");
 c->rates[0]=1;strcpy(c->rates_status,"Refresh ECB rates to convert");
}
static void vc_history_add(VCState *c,const char *expression,const char *answer) {
 for(int i=VC_HISTORY-1;i>0;i--)memcpy(c->history[i],c->history[i-1],sizeof(c->history[0]));
 snprintf(c->history[0],sizeof(c->history[0]),"%.250s = %.120s",expression,answer);if(c->history_count<VC_HISTORY)c->history_count++;
}
static size_t vc_operand_start(const char *expr) {
 int depth=0;size_t start=0;
 for(size_t i=0;expr[i];i++) {
  if(expr[i]=='(')depth++;else if(expr[i]==')')depth--;
  else if(!depth && strchr("+-*/",expr[i]) && i>0 && !strchr("+-*/^(eE",expr[i-1]))start=i+1;
 }
 return start;
}
static void vc_evaluate(VCState *c) {
 c->error[0]=0;
 if(c->mode==VC_STANDARD && c->done && c->repeat_op) {
  char previous[128];strcpy(previous,c->answer);
  snprintf(c->expression,sizeof(c->expression),"%s%c(%.13g)",previous,c->repeat_op,c->repeat_value);c->done=false;
 }
 if(c->mode==VC_PROGRAMMER) {
  if(!vc_integer(c->expression,c->base,c->bits,&c->integer))strcpy(c->error,"Invalid integer / divide by zero / shift");
  else {vc_integer_text(c->integer,c->base,c->answer,sizeof(c->answer));c->value=(double)c->integer;}
 } else if(c->mode==VC_DATE) {
  int y,m,d;if(!vc_date_parse(c->date[0],&y,&m,&d)){strcpy(c->error,"Enter a valid start date: YYYY-MM-DD");return;}int64_t first=vc_date_days(y,m,d);
  if(!c->date_action) {
   if(!vc_date_parse(c->date[1],&y,&m,&d)){strcpy(c->error,"Enter a valid end date: YYYY-MM-DD");return;}
   int64_t delta=vc_date_days(y,m,d)-first;snprintf(c->answer,sizeof(c->answer),"%lld days (%lld weeks, %lld days)",(long long)delta,(long long)(delta/7),(long long)llabs(delta%7));
  } else {char *end;long n=strtol(c->days,&end,10);char date[11];if(!c->days[0] || *end || n<0 || n>3652059 || !vc_date_from_days(first+(c->date_action==1?n:-n),date))strcpy(c->error,"Days must keep the date in years 1..9999");else strcpy(c->answer,date);}
 } else {
  double n;if(!vc_real(c->expression,c->degrees,0,&n)){strcpy(c->error,"Invalid expression / math domain");return;}
  if(c->mode==VC_CONVERT) {if(!vc_convert(n,c->unit_group,c->from,c->to,&n)){strcpy(c->error,"Invalid unit conversion");return;}}
  else if(c->mode==VC_CURRENCY) {if(c->rates[c->currency_from]<=0 || c->rates[c->currency_to]<=0){strcpy(c->error,"No rates. Tap Refresh (Wi-Fi required)");return;}n=n/c->rates[c->currency_from]*c->rates[c->currency_to];}
  if(c->mode==VC_STANDARD) {
   size_t start=vc_operand_start(c->expression);c->repeat_op=0;
   if(start && vc_real(c->expression+start,c->degrees,0,&c->repeat_value))c->repeat_op=c->expression[start-1];
  }
  c->value=n;snprintf(c->answer,sizeof(c->answer),"%.13g",n);
 }
 if(!c->error[0]){vc_history_add(c,c->expression,c->answer);c->done=true;}
}
static void vc_mode(VCState *c,int mode) {
 if(mode<0 || mode>=VC_MODES)return;
 strcpy(c->saved[c->mode],c->expression);c->mode=mode;
 snprintf(c->expression,sizeof(c->expression),"%s",c->saved[mode][0]?c->saved[mode]:"0");
 strcpy(c->answer,"0");c->error[0]=0;c->done=false;c->menu=false;c->history_open=false;c->graph_edit=false;
}
static void vc_append(VCState *c,const char *text) {
 size_t n=strlen(c->expression),len=strlen(text);
 bool number=isalnum((unsigned char)*text) || *text=='.' || (c->mode==VC_PROGRAMMER && strchr("ABCDEF",*text));
 if(c->done){if(number)c->expression[0]=0;else snprintf(c->expression,sizeof(c->expression),"%s",c->answer);c->done=false;n=strlen(c->expression);}
 if(number && !strcmp(c->expression,"0") && *text!='.')c->expression[0]=0;
 n=strlen(c->expression);if(n+len<sizeof(c->expression))memcpy(c->expression+n,text,len+1);c->error[0]=0;
}
static void vc_key(VCState *c,const char *key) {
 if(c->mode==VC_PROGRAMMER && c->base==16 && !strcmp(key,"C")){vc_append(c,key);return;}
 if(c->mode==VC_PROGRAMMER && !strcmp(key,"%")){vc_append(c,key);return;}
 if(c->mode==VC_PROGRAMMER && !strcmp(key,"~")){char old[256];strcpy(old,c->expression);snprintf(c->expression,sizeof(c->expression),"~(%.245s)",old);c->done=false;return;}
 if(!strcmp(key,"C") || !strcmp(key,"CE")){strcpy(c->expression,"0");strcpy(c->answer,"0");c->done=false;c->repeat_op=0;c->error[0]=0;return;}
 if(!strcmp(key,"Del")){size_t n=strlen(c->expression);if(n)c->expression[n-1]=0;if(!c->expression[0])strcpy(c->expression,"0");c->done=false;c->error[0]=0;return;}
 if(!strcmp(key,"=")){vc_evaluate(c);return;}
 if(!strcmp(key,"MC")){c->memory=0;return;}
 if(!strcmp(key,"MR")){snprintf(c->expression,sizeof(c->expression),"%.13g",c->memory);c->done=false;return;}
 if(!strcmp(key,"M+") || !strcmp(key,"M-")){double n;if(vc_real(c->expression,c->degrees,0,&n))c->memory+=!strcmp(key,"M+")?n:-n;else strcpy(c->error,"Cannot store this expression");return;}
 if(!strcmp(key,"pi") || !strcmp(key,"e") || !strcmp(key,"x")) {
  if(c->done){c->expression[0]=0;c->done=false;}
  size_t n=strlen(c->expression);
  if(n && strcmp(c->expression,"0") && (isdigit((unsigned char)c->expression[n-1]) || c->expression[n-1]==')'))vc_append(c,"*");
  vc_append(c,key);return;
 }
 bool unary=!strcmp(key,"+/-") || !strcmp(key,"1/x") || !strcmp(key,"x^2") || !strcmp(key,"%");
 bool function=isalpha((unsigned char)*key) && strlen(key)>1;
 if(unary || function) {
  char old[256],result[256];
  snprintf(old,sizeof(old),"%s",c->done?c->answer:c->expression);
  size_t start=c->mode==VC_GRAPH?0:vc_operand_start(old);char prefix[256];memcpy(prefix,old,start);prefix[start]=0;
  const char *operand=old+start;
  if(!strcmp(key,"%") && c->mode==VC_STANDARD && start && (old[start-1]=='+' || old[start-1]=='-')) {
   char left[256];memcpy(left,old,start-1);left[start-1]=0;double value;
   if(vc_real(left,c->degrees,0,&value))snprintf(result,sizeof(result),"%.110s(%.13g*(%.100s)/100)",prefix,value,operand);else snprintf(result,sizeof(result),"%.115s(%.110s)/100",prefix,operand);
  } else if(unary) {
   const char *format=!strcmp(key,"+/-")?"%.115s-(%.110s)":!strcmp(key,"1/x")?"%.115s1/(%.110s)":!strcmp(key,"x^2")?"%.115s(%.110s)^2":"%.115s(%.110s)/100";
   snprintf(result,sizeof(result),format,prefix,operand);
  } else snprintf(result,sizeof(result),"%.105s%.12s(%.110s)",prefix,key,operand);
  snprintf(c->expression,sizeof(c->expression),"%s",result);c->done=false;
  if(c->mode!=VC_GRAPH){vc_evaluate(c);c->repeat_op=0;}
  return;
 }

 vc_append(c,key);
}
#endif

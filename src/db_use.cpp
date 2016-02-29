#include "db_use.h"

extern FARSTANDARDFUNCTIONS FSF;
extern HANDLE LookHeap;

void * operator new(size_t size)
{
return HeapAlloc(LookHeap,HEAP_ZERO_MEMORY, size);
}
void operator delete(void *block)
{
if(block)HeapFree(LookHeap,0,block);
}
void *operator new[](size_t size) {return ::operator new(size);}
void *operator new(size_t size, void *p) {return p;}
void operator delete[](void *ptr) {::operator delete(ptr);}

void _pure_error_ () {};

BYTE Upper(BYTE c)
{
if(c<97)return c;
if(c<123)return c-32;
if(c<224)return c;
return c-32;
}

char *Upper(char *text)
{
BYTE *a=(BYTE *)text;
while(*a){ *a=Upper(*a); a++; }
return text;
}

//--------- Двухсвязный список объектов ---------------

Link *Link::Head(void)
{
Link *c=this;
if(c)while(c->prev)c=c->prev;
return c;
}

Link *Link::Tail(void)
{
Link *c=this;
if(c)while(c->next)c=c->next;
return c;
}

Link *Link::Find(int num)
{
Link *c=(Link *)Head();
while(c && num) { num--; c=c->next; }
return c;
}

void Link::After(Link *m)
{
if(!m)return;
Link *c=next;
next=m;
m->prev=this;
m->next=c;
if(c)c->prev=m;
}

void Link::Before(Link *m)
{
if(!m)return;
Link *c=prev;
prev=m;
m->prev=c;
m->next=this;
if(c)c->next=m;
}

Link *Link::Add(Link *m)
{
if(this) { Link *c=Tail(); c->After(m); }
else m->prev=NULL;
m->next=NULL;
return m;
}

void Link::Extract(void)
{
if(prev)prev->next=next;
if(next)next->prev=prev;
}

Link *Link::Step(char &dir)
{
Link *after;
if(dir=='n'||dir=='N') {
  if(next) after=next;
  else { dir=0; after=this; }
  }
else {
  if(prev) after=prev;
  else { dir=0; after=this; }
  }
return after;
}

Link *Link::Destroy(char &dir)
{
Link *after;
if(dir=='n'||dir=='N') {
  if(next) after=next;
  else { dir=0; after=prev; }
  }
else {
  if(prev) after=prev;
  else { dir=0; after=next; }
  }
if(prev)prev->next=next;
if(next)next->prev=prev;
delete this;
return after;
}

Link *Link::DestroyAll(void)
{
Link *c;
char flag='N';
c=Head();
while(flag)c=c->Destroy(flag);
return NULL;
}

//================== Преобразование данных ==================

__int64 a_i64(char *s,__int64 def, char dec)
{
__int64 rez=0;
int z,sign=1;
char *c, first=1,dot=0;
if(dec<0) dec=0;
for(c=s;*c;c++) {
  if(first) {
    if(*c==' ') continue;
    first=0; if(*c=='+') continue;
    if(*c=='-') { sign=-1; continue; }
    if(*c=='.') { dot=1; continue; }
    }
  if(*c=='.') {
    if(dot) break;
    dot=1; continue;
    }
  z=*c-'0';  if(z<0 ||z>9) break;
  if(dot) {
    if(!dec)return rez*sign;
    --dec;
    }
  rez=rez*10+z;
  }
if(first) return def;
for(z=0;z<dec;z++)rez*=10;
return rez*sign;
}

char *i64_a(char *s, __int64 val, int mins, int dec)
{
int i,l;
char t[64],sign,fil;
__int64 a;
bool one=true;
i=62; t[63]=0; sign=0; fil=' ';
if(val<0) { sign='-'; val=-val; }
if(mins<0) { fil='0'; mins=-mins; }
if(mins>62) mins=62;
for(l=0;l<mins||val>0;l++,i--) {
  if(dec) {
    if(dec==l) { t[i]='.'; dec=0; }
    else { t[i]=val%10+'0';  val/=10; }
    }
  else {
    if(val) { t[i]=val%10+'0';  val/=10; }
    else {
      if(one) t[i]='0';
      else {
        if(sign) { t[i]=sign; sign=0; }
        else t[i]=fil;
        }
      }
    one=false;
    }
  }
if(sign) t[i]=sign; else ++i;
lstrcpy(s,t+i);
return s;
}

__int64 ah_i64(char *s,__int64 def)
{
long z;
char *c;
__int64 rez=0;
for(c=s;*c;c++) {
  if(*c<'0') return def;
  z=*c-'0';  if(z<10) goto STORE;
  if(*c<'A') return def;
  z=*c-'A'+10;  if(z<16) goto STORE;
  if(*c<'a') return def;
  z=*c-'a'+10;  if(z>15) return def;
STORE:
  rez<<=4; rez|=z;
  }
return rez;
}

double a_dbl(char *s)
{
double d;
int i,e,sign;
d=0; e=1; sign=1;
for(i=0;s[i];i++) {
  if(s[i]=='+'||s[i]==' ') continue;
  if(s[i]=='-') { sign=-1; continue; }
  if(s[i]=='e') break; //<-------------- встречена экспонента
  if(s[i]=='.') { e=0; continue; }
  if(s[i]>'9'||s[i]<'0') return d*sign;
  d=d*10.+(s[i]-'0');
  if(e<1)e--;
  }
if(e<0) for(;e;e++)d/=10.; //<--- реализуется экспонента, набранная после точки
d*=sign;                   //<--- учет знака
if(!s[i]) return d;        //<--- Нет экспоненты
e=sign=0;
for(++i;s[i];i++) {
  if(s[i]=='+') continue;
  if(s[i]=='-') { sign=1; continue; }
  if(s[i]>'9'||s[i]<'0') break;
  e=e*10+(s[i]-'0');
  }
if(!e) return d;
if(sign) for(i=0;i<e;i++)d/=10.; //<---экспонента меньше 0
else for(i=0;i<e;i++)d*=10.;     //<---экспонента больше 0
return d;
}

void Bin_Hex(char *s, BYTE *b, int nb)
{
int i;
for(i=0;i<nb;i++) {
  *s=(b[i]>>4)&0x0F;
  if(*s>9) *s+=55; else *s+=48;
  s++;
  *s=b[i]&0x0F;
  if(*s>9) *s+=55; else *s+=48;
  s++;
  }
*s=0;
}

void Hex_Bin(char *s, BYTE *b, int nb)
{
int i,j,n;
for(i=0;i<nb;i++) b[i]=0;
for(i=j=0;s[j];j++) {
  if(s[j]<'0') return;
  n=s[j]-'0';
  if(s[j]>'9') switch(s[j]) {
    case 'A': case 'a': case 'А': case 'а': n=10; break;
    case 'B': case 'b': case 'В': case 'в': n=11; break;
    case 'C': case 'c': case 'С': case 'с': n=12; break;
    case 'D': case 'd': case 'Д': case 'д': n=13; break;
    case 'E': case 'e': case 'Е': case 'е': n=14; break;
    case 'F': case 'f': case 'Ф': case 'ф': n=15; break;
    default: return;
    }
  b[i]|=n;
  if(j&0x01) i++; else b[i]<<=4;
  }
}

//================== Операции с датой и временем ==================

BYTE Mon[12]={31,28,31,30,31,30,31,31,30,31,30,31};
char *DTcode[7]={"dd","mm","yyyy","hh","mi","se","iii"};

bool DTbad(SYSTEMTIME *t)
{
if(!t->wMonth||t->wMonth>12) return true;
Mon[1]=(t->wYear%4)? 28 : 29;
if(!t->wDay||t->wDay>Mon[t->wMonth-1]) return true;
if(t->wMilliseconds > 999) return true;
if(t->wHour > 24) return true;
if(t->wMinute > 59) return true;
if(t->wSecond > 59) return true;
return false;
}
//---------------------------------------------------------------------------

void DTd2t(__int32 d, SYSTEMTIME *t)
{
if(!d) { ZeroMemory(t,sizeof(SYSTEMTIME)); return; }
WORD da;
__int32 l=d; l<<=2;
t->wYear=l/1461;
da=d-365u*t->wYear-t->wYear/4;
if(da)t->wYear++; else da=366;
Mon[1]=(t->wYear%4)? 28 : 29;
for(t->wMonth=0; da>Mon[t->wMonth]; da-=Mon[t->wMonth++]);
t->wMonth++;
t->wDay=da;
if(DTbad(t)) ZeroMemory(t,sizeof(SYSTEMTIME));
}
//---------------------------------------------------------------------------

void DTt2t(__int32 ti, SYSTEMTIME *t)
{
ZeroMemory(t,sizeof(SYSTEMTIME));
if(ti<0 || ti>86400000L) return;
t->wMilliseconds=ti%1000;
t->wHour=ti/3600000L; ti-=t->wHour*3600000L;
t->wMinute=ti/60000L; t->wSecond=(ti-t->wMinute*60000L)/1000L;
}
//---------------------------------------------------------------------------

DWORD DTt2dat(SYSTEMTIME *t)  // SYSTEMTIME --> целая дата
{
DWORD dat;
WORD m,y;
if(DTbad(t)) return 0;
y=t->wYear-1; m=t->wMonth-1;
dat=y*365+y/4+t->wDay;
for(y=0; y<m; dat+=Mon[y++]);
return dat;
}
//---------------------------------------------------------------------------

DWORD DTt2tim(SYSTEMTIME *t)  // SYSTEMTIME --> целое время
{
DWORD tim;
if(DTbad(t)) return 0;
tim=t->wMilliseconds;
tim+=t->wSecond*1000L;
tim+=t->wMinute*60000L;
tim+=t->wHour*3600000L;
return tim;
}
//---------------------------------------------------------------------------

// Строка s, содержащая дату-время в формате f переводится в числа -
// элементы структуры SYSTEMTIME *t, используя следующие умолчания:
//- если год отсутствует в формате, считается, что он 2000;
//- если год задан двумя цифрами, то >50 трактуются как 19yy, иначе 20yy;
//- если месяц отсутствует в формате, считается, что он 01 - январь;
//- если день отсутствует в формате, считается, что он 01;
//- если время отсутствует в формате, считается, что оно 00:00:00.000;

void DTsf2t(char *s, char *f, SYSTEMTIME *t)
{
int i,j,k,l,m,n;
bool y2;
WORD *w;
ZeroMemory(t,sizeof(SYSTEMTIME));
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { j++; i++; continue; }
  ++i; l=f[i]&0x0f; k=f[i]>>4; n=l-1; ++i;
  m=1; y2=false;
  switch(k) {
    case 0: w=&(t->wDay); break;
    case 1: w=&(t->wMonth); break;
    case 2: w=&(t->wYear); y2=(l==2); break;
    case 3: w=&(t->wHour); break;
    case 4: w=&(t->wMinute); break;
    case 5: w=&(t->wSecond); break;
    case 6: w=&(t->wMilliseconds); if(l==3)break;
            m=(l==2)? 10 : 100; break;
    default: continue;
    }
  *w=0; while(n>=0) { (*w)+=(s[j+n]-'0')*m; m*=10; --n; }
  j+=l;
  }
if(!t->wYear)t->wYear=2000;
if(y2)t->wYear+=(t->wYear>50)? 1900 : 2000;
if(!t->wMonth)t->wMonth=1;
if(!t->wDay)t->wDay=1;
if(DTbad(t)) ZeroMemory(t,sizeof(SYSTEMTIME));
}
//---------------------------------------------------------------------------

int MyCmp(char *s, char *f)
{
int i=0;
while(s[i] && f[i] && (s[i] == f[i] || s[i]+32 == f[i]) )i++;
return i;
}
//---------------------------------------------------------------------------
// Cтрока формата f со специальными кодами переводится в
// строку формата s с буквенными кодами (см.DTcode)

char *DTf2s(char *f, char *s) {
int i,j,k,l;
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { s[j++]=f[i++]; continue; }
  ++i; l=f[i]&0x0f; k=f[i]>>4; ++i;
  CopyMemory(s+j,DTcode[k],l);
  j+=l;
  }
s[j]=0;
return s;
}
//---------------------------------------------------------------------------

// Cтрока формата f со специальными кодами переводится в форматную строку
// FAR для элемента диалога DI_FIXEDIT

char *DTf29(char *f, char *s) {
int i,j,l;
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { s[j++]=f[i++]; continue; }
  ++i; l=f[i]&0x0f; ++i;
  while(l) { s[j++]='9'; --l; }
  }
s[j]=0;
return s;
}
//---------------------------------------------------------------------------

// Вычисляется ширина строки даты времени по формату f

int DTw(char *f) {
int i,j;
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { j++; i++; continue; }
  ++i; j+=f[i]&0x0f; ++i;
  }
return j;
}
//---------------------------------------------------------------------------

// Строка формата s с буквенными кодами (см.DTcode) переводится в
// строку формата f со специальными кодами

char *DTs2f(char *s, char *f)
{
int i,j,k,c;
for(i=j=0; s[i] && i<63 && j<63;) {
  c=0; k=MyCmp(s+i,DTcode[0]);   //------ Day=0;
  if(k==2) goto SPECIAL;
  c=0x10; k=MyCmp(s+i,DTcode[1]);//------ Month=1;
  if(k==2) goto SPECIAL;
  c=0x20; k=MyCmp(s+i,DTcode[2]);//------ Year=2;
  if(k==3)k=2; if(k>1) goto SPECIAL;
  c=0x30; k=MyCmp(s+i,DTcode[3]);//------ Hours=3;
  if(k==2) goto SPECIAL;
  c=0x40; k=MyCmp(s+i,DTcode[4]);//------ Minutes=4;
  if(k==2) goto SPECIAL;
  c=0x50; k=MyCmp(s+i,DTcode[5]);//------ Seconds=5;
  if(k==2) goto SPECIAL;
  c=0x60; k=MyCmp(s+i,DTcode[6]);//------ Milliseconds=6;
  if(k) goto SPECIAL;
  f[j++]=s[i++];
  continue;
SPECIAL:
  f[j++]=0x01; f[j++]=c|k; i+=k;
  }
f[j]=0;
return f;
}
//---------------------------------------------------------------------------

// Содержимое структуры SYSTEMTIME *t переводится в строку s по формату f

char *DTstr(char *s, SYSTEMTIME *t, char *f)
{
int i,j,k,l,n;
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { s[j++]=f[i++]; continue; }
  ++i; l=f[i]&0x0f; k=f[i]>>4; n=l-1; ++i;
  switch(k) {
    case 0: k=t->wDay; break;
    case 1: k=t->wMonth; break;
    case 2: k=t->wYear; break;
    case 3: k=t->wHour; break;
    case 4: k=t->wMinute; break;
    case 5: k=t->wSecond; break;
    case 6: k=t->wMilliseconds; if(l==3)break;
            k=(l==2)? (k+5)/10 : (k+50)/100;
            break;
    default: k=0; break;
    }
  if(t->wDay) while(n>=0) { s[j+n]=(k%10)+'0'; k/=10; --n; }
  else for(n=0;n<l;n++) s[j+n]=' ';
  j+=l;
  }
s[j]=0;
return s;
}
//---------------------------------------------------------------------------

// Стандартная строка d (формат = yyyymmyyhhmise)
// переводится в строку s по формату f

char *DTstr(char *s, char *d, char *f)
{
int c[7]={6,4,0,8,10,12,-1};
int i,j,k,l,n;
for(i=j=0; f[i];) {
  if(f[i]!=0x01) { s[j++]=f[i++]; continue; }
  ++i; l=f[i]&0x0f; k=f[i]>>4; k=c[k]; ++i;
  if(k==0 && l==2) k=2; // Две цифры года должны быть последними
  for(n=0;n<l;n++) s[j+n]=(k<0)? '0' : d[k+n];
  j+=l;
  }
s[j]=0;
return s;
}

//================== Прочее ==================

bool NotNum(char c)
{
if(c<'0' || c>'9') return true;
return false;
}

bool LatAlphaNum(BYTE c)
{
if(c<0x30) return false;
if(c<0x3a) return true;
if(c<0x41) return false;
if(c<0x5b) return true;
if(c<0x61) return false;
if(c<0x7b) return true;
return false;
}

bool WinAlphaNum(BYTE c)
{
if(c<0x7b) return LatAlphaNum(c);
if(c<0xc0) return false;
return true;
}

bool DosAlphaNum(BYTE c)
{
if(c<0x7b) return LatAlphaNum(c);
if(c<0x80) return false;
if(c<0xb0) return true;
if(c<0xe0) return false;
if(c<0xf8) return true;
return false;
}

BYTE MyWrite(HANDLE h, void *buf, DWORD len)
{
DWORD nbr;
BYTE *b=(BYTE*)buf;
if(!len)len=lstrlen(b);
if(!WriteFile(h,b,len,&nbr,NULL) || nbr<len) return 1;
return 0;
}

BYTE MyRead(HANDLE h, void *buf, DWORD len)
{
DWORD nbr;
BYTE *b=(BYTE*)buf;
if(!ReadFile(h,b,len,&nbr,NULL) || nbr<len) return 1;
return 0;
}

//--------- Работа с базами данных формата DBF ---------------

void dbBase::SaveHeader(void)
{
if(f==INVALID_HANDLE_VALUE) return;
DWORD nbr;
if(upd) {
  SetFilePointer(f,0,NULL,FILE_BEGIN);
  if(!WriteFile(f,&dbH,32,&nbr,NULL)) return;
  if(nbr<32) return;
  if(upd&0xfe) {
    dbField *d;
    for(d=dbF; d; d=(dbField*)(d->Next())) {
      if(d->type=='#') continue;
      if(!WriteFile(f,d->name,32,&nbr,NULL)) return;
      if(nbr<32) return;
      }
    if(!WriteFile(f,Hext,lhext,&nbr,NULL)) return;
    }
  }
upd=0;
}
//-----------------------------------

void dbBase::Close(void)
{
if(f!=INVALID_HANDLE_VALUE) { SaveHeader(); CloseHandle(f); }
if(m!=INVALID_HANDLE_VALUE) CloseHandle(m);
if(dbF)dbF->DestroyAll();
if(rec)delete rec;
ZeroMemory(this,sizeof(dbBase));
f=m=INVALID_HANDLE_VALUE;
}
//-----------------------------------

WORD ParseMemoExt(char *mem, char *mext)
{
WORD n,r;
for(n=r=0; mem[n]; n++) {
  if(mem[n]=='.')r=n;
  if(mem[n]=='\\')r=0;
  }
if(!mext) { if(r) return r;  return n; }
char *e, *me;
int i;
if(r) {
  e=mem+r+1;
  for(me=mext;*me;) {
    if(*me!=':') {
      i=MyCmp(me,e);
      if(me[i]==':') {                    // extention definition found
        ++i;
        if(!me[i] || me[i]==',') {        // memo extention is empty
          mem[r]=0;
          break;
          }
        for(me+=i;*me;e++,me++) {         // memo extention added
          *e=*me; if(*e==',') { *e=0; return 0; }
          }
        return 0;
        }
      }
    for(;*me;) { ++me; if(*me==',') {++me; break; } }
    }
  }
else {
  for(me=mext;*me;) {
    if(*me==':') {                        // empty extention defined
      ++me; if(*me==',' || *me==0) break; // empty extention defined as empty!
      e=mem+n; *e='.'; ++e;
      for(;*me;e++,me++) {                // memo extention for empty added
        *e=*me; if(*e==',') { *e=0; return 0; }
        }
      return 0;
      }
    for(;*me;) { ++me; if(*me==',') {++me; break; } }
    }
  }
if(r) return r;
return n;
}

//-----------------------------------

void dbBase::OpenMemo(char *file, char *mext)
{
if(m!=INVALID_HANDLE_VALUE) return;
switch(dbH.type) {
  case 0x83: // ---> dBaseIII memo
    tmem=0; break;
  case 0x8b: // ---> dBaseIV memo
    tmem=1; break;
  case 0xf5: case 0x30: case 0x31: case 0x32: case 0xfb: case 0x43: // ---> FoxPro memo
    tmem=2; break;
  default: return;  // ---> No memo or unknown
  }
BYTE b[512];
WORD n,r;
DWORD nbr;
lstrcpy(b,file);
n=ParseMemoExt(b,mext);
if(n) {
  if(tmem==2) lstrcpy(b+n,".FPT");
  else lstrcpy(b+n,".DBT");
  }
m=CreateFile(b,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,NULL);
if(m==INVALID_HANDLE_VALUE) return;
if(!ReadFile(m,b,512,&nbr,NULL) || nbr<512) {
  CloseHandle(m); m=INVALID_HANDLE_VALUE;
  return;
  }
switch(tmem) {
  case 0: // --------------------------------> dBaseIII memo
    lmem=512; break;
  case 1: // --------------------------------> dBaseIV memo
    lmem=b[21]; lmem<<=8; lmem|=b[20]; break;
  case 2: // --------------------------------> FoxPro memo
    lmem=b[6]; lmem<<=8; lmem|=b[7]; break;
  default: // -------------------------------> No memo or unknown
    CloseHandle(m); m=INVALID_HANDLE_VALUE;
  }
}
//-----------------------------------

BYTE dbBase::Open(char *file, BYTE ronly)
{
WORD k,n,rl;
int lh;
BYTE ret,ind,msk;
DWORD nbr;
ret=0;
if(f!=INVALID_HANDLE_VALUE) return 3;
if(!ronly) {
  f=CreateFile(file,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,
    OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,NULL);
  if(f==INVALID_HANDLE_VALUE) ronly=1;
  }
if(ronly)  {
  f=CreateFile(file,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
    OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,NULL);
  if(f==INVALID_HANDLE_VALUE) return 1;
  ret=0x10;
  }
if(!ReadFile(f,&dbH,32,&nbr,NULL) || nbr<32) return 2;
if(dbH.reclen==0||dbH.start<=64||dbH.nrec<0) return 2;
dbField *d, *h;
rl=1; h=NULL, lh=0;
for(nfil=0;;) {
  lh+=32;
  if(lh+1 > dbH.start) break;
  d=new dbField;
  ZeroMemory(d,sizeof(dbField));
  if(!ReadFile(f,d->name,32,&nbr,NULL)) {
    DB_ERROR:
    delete d; nfil=0; return 2;
    }
  if(d->name[0]==0x0d) { delete d; break; }
  if(nbr<32) goto DB_ERROR;
  d->loc=rl;
  rl+=d->filen;
  h=(dbField*)(h->Add(d));
  nfil++;
  }
if(rl > dbH.reclen) return 2;
k=0;
while(rl < dbH.reclen) {
  n=dbH.reclen-rl; if(n>255)n=255;
  d=new dbField; ZeroMemory(d,sizeof(dbField));
  FSF.sprintf(d->name,"#%d#",++k);
  d->type='#'; d->filen=n;
  rl+=d->filen;
  h=(dbField*)(h->Add(d));
  nfil++;
  }
cf=dbF=(dbField*)(h->Head());
if(lh>=dbH.start) { nfil=0; return 2; }
lhext=dbH.start-lh;
nbr=rl+lhext+18; nbr=(nbr+7)/8; nbr*=8;
rec=new BYTE[nbr];
if(!rec) return 4;
//-------- Read Header Extention after field's headers
//-------- 1 byte value 0x0d as minimum
nbr=(rl+9)/8; nbr*=8;
Hext=rec+nbr;
if(SetFilePointer(f,lh,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
if(!ReadFile(f,Hext,lhext,&nbr,NULL) || nbr<lhext) return 2;
Hext[0]=0x0d;
//-------- _NullFlags field for nulable and variable length fields
//-------- recognized and set if it present
ind=0; msk=1; Nflg=NULL;
for(d=dbF;d;d=(dbField*)(d->Next())) {
  if((d->spare[0]&0x05)!=5) continue;
  if(MyCmp(d->name,"_NullFlags")!=10) continue;
  Nflg=rec+d->loc;
  }
if(Nflg) for(d=dbF;d;d=(dbField*)(d->Next())) {
  if(d->type=='V'||d->type=='Q') { // Variable length field
    d->mskV=msk; d->indV=ind;
    if(msk&0x80) { ind++; msk=1; } else msk<<=1;
    }
  if(d->spare[0]&0x02) {           // Nullable field
    d->mskN=msk; d->indN=ind;
    if(msk&0x80) { ind++; msk=1; } else msk<<=1;
    }
  }
//OpenMemo(file);
pos=dbH.start;
if(!dbH.nrec) return ret;
if(SetFilePointer(f,pos,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
if(!ReadFile(f,rec,dbH.reclen,&nbr,NULL) || nbr<dbH.reclen) return 2;
pos+=dbH.reclen;
cur=1;
return ret;
}
//-----------------------------------

BYTE dbBase::Create(char *file, BYTE t, dbBase *dc)
{
WORD n;
dbField *d;
DWORD nbr,lh;
SYSTEMTIME ti;
if(f!=INVALID_HANDLE_VALUE) return 1;
dbH.nrec=0;
dbH.type=t;
dbH.start=32;
dbH.reclen=1;
dbH.ind=0;
GetSystemTime(&ti);
dbH.upd[0]=ti.wYear-1900u;
dbH.upd[1]=ti.wMonth;
dbH.upd[2]=ti.wDay;
nbr=0;
for(d=dbF; d; d=(dbField*)(d->Next())) {
  d->loc=dbH.reclen;
  if((d->spare[0]&0x05)==5)nbr=dbH.reclen;
  dbH.reclen+=d->filen;
  dbH.start+=32;
  }
if(dbH.reclen==1)return 3;
lhext=1;
if(dc) lhext=dc->lhext;
else if((dbH.type&0x30) == 0x30)lhext=264; // Visual FoxPro Header Extention
dbH.start+=lhext;
lh=dbH.reclen+lhext+18; lh=(lh+7)/8; lh*=8;
rec=new BYTE[lh]; if(!rec) return 4;
lh=(dbH.reclen+9)/8; lh*=8;
Hext=rec+lh;
Hext[0]=0x0d;
if(dc) {
  CopyMemory(dbH.spare1,dc->dbH.spare1,20);
  CopyMemory(Hext,dc->Hext,lhext);
  }
if(nbr)Nflg=rec+nbr;
f=CreateFile(file,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,
             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,NULL);
if(f==INVALID_HANDLE_VALUE) return 2;
if(!WriteFile(f,&dbH,32,&nbr,NULL) || nbr<32) goto BAD_WRITE;
for(d=dbF; d; d=(dbField*)(d->Next()))
  if(!WriteFile(f,d->name,32,&nbr,NULL) || nbr<32) goto BAD_WRITE;
if(!WriteFile(f,Hext,lhext,&nbr,NULL) || nbr<lhext) goto BAD_WRITE;
rec[0]=' ';//Clear();
cur=0;
pos=dbH.start;
return 0;

BAD_WRITE:
CloseHandle(f);
f=INVALID_HANDLE_VALUE;
return 2;
}
//-----------------------------------

void dbBase::Add(char *fname, char ftype, BYTE flen, BYTE fdec)
{
dbField *d;
d=new dbField;
ZeroMemory(d,sizeof(dbField));
lstrcpy(d->name,fname);
d->type=ftype;
d->filen=flen;
d->dec=fdec;
if(dbF) (dbF->Tail())->After(d);
else dbF=d;
nfil++;
}
//-----------------------------------

void dbBase::AddF(dbField *of, char *fname, char ftype, BYTE flen, BYTE fdec)
{
dbField *d;
d=new dbField;
ZeroMemory(d,sizeof(dbField));
CopyMemory(d->spare,of->spare,14);
if(fname)lstrcpy(d->name,fname); else lstrcpy(d->name,of->name);
if(ftype)d->type=ftype; else d->type=of->type;
if(flen)d->filen=flen;  else d->filen=of->filen;
if(fdec)d->dec=fdec;    else d->dec=of->dec;
if(dbF) (dbF->Tail())->After(d);
else dbF=d;
nfil++;
}
//-----------------------------------

BYTE dbBase::AddNull(void)
{
BYTE ind,msk;
dbField *d;
ind=0; msk=1;
for(d=dbF;d;d=(dbField *)(d->Next())) {
  if(d->type=='V'||d->type=='Q') { // Variable length field
    d->mskV=msk; d->indV=ind;
    if(msk&0x80) { ind++; msk=1; } else msk<<=1;
    }
  if(d->spare[0]&0x02) {           // Nullable field
    d->mskN=msk; d->indN=ind;
    if(msk&0x80) { ind++; msk=1; } else msk<<=1;
    }
  }
if((!ind) && msk==1) return 0;
d=new dbField;
ZeroMemory(d,sizeof(dbField));
lstrcpy(d->name,"_NullFlags");
d->type='0';
d->filen=(msk==1)? ind : ind+1;
d->dec=0;
d->spare[0]=5;
(dbF->Tail())->After(d);
nfil++;
return d->filen;
}
//-----------------------------------

BYTE dbBase::Read(DWORD rn)
{
DWORD nbr;
if(rn>dbH.nrec)return 1;
if(cur==rn)return 0;
pos=dbH.start+(rn-1)*dbH.reclen;
if(SetFilePointer(f,pos,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
if(!ReadFile(f,rec,dbH.reclen,&nbr,NULL) || nbr<dbH.reclen) return 2;
cur=rn;
pos+=dbH.reclen;
return 0;
}
//-----------------------------------

BYTE dbBase::GetMemo(char *file, DWORD *blocknum)
{
DWORD nb,nbr;
WORD i;
if(cf->type!='M') return 1;
if(m==INVALID_HANDLE_VALUE)return 11;
nb=(cf->filen<10)? GetBinary() : Get64();
if(blocknum)*blocknum=nb;
if(!nb) return 2;
BYTE *b=new BYTE[lmem];
if(!b) return 12;
HANDLE mf;
mf=CreateFile(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
if(mf==INVALID_HANDLE_VALUE) { delete b; return 13; }
nb*=lmem;
if(SetFilePointer(m,nb,NULL,FILE_BEGIN)==0xFFFFFFFF) goto BAD_MEMO;
if(!ReadFile(m,b,lmem,&nbr,NULL)) goto BAD_MEMO;
switch(tmem) {
  case 0: // --------------------------------> dBaseIII memo
    for(;;) {
      for(i=0; i<lmem; i++) if(b[i]==0x1a) break;
      if(!i) break;
      if(!WriteFile(mf,b,i,&nbr,NULL) || nbr<i) goto BAD_MEMO;
      if(i<lmem) break;
      if(!ReadFile(m,b,lmem,&nbr,NULL) || nbr<lmem) goto BAD_MEMO;
      }
    break;
  case 1: // --------------------------------> dBaseIV memo
    if(b[0]!=0xff) goto BAD_MEMO;
    if(b[1]!=0xff) goto BAD_MEMO;
    if(b[2]!=0x08) goto BAD_MEMO;
    if(b[3]) goto BAD_MEMO;
    nb=*((DWORD *)(b+4))-8;
    goto READ_MEMO;
  case 2: // --------------------------------> FoxPro memo
    nb=b[4]; nb<<=8; nb|=b[5]; nb<<=8; nb|=b[6]; nb<<=8; nb|=b[7];
  READ_MEMO:
    if(nbr<lmem) { // first block is last and it is not full
      i=nbr-8;
      if(!WriteFile(mf,b+8,i,&nbr,NULL) || nbr<i) goto BAD_MEMO;
      break;
      }
    i=lmem-8; if(nb<i)i=nb;
    if(!WriteFile(mf,b+8,i,&nbr,NULL) || nbr<i) goto BAD_MEMO;
    nb-=i; i=lmem;
    while(nb) {
      if(!ReadFile(m,b,lmem,&nbr,NULL)) goto BAD_MEMO;
      if(nbr<lmem) { // last block is not full
        i=nbr;
        if(!WriteFile(mf,b,i,&nbr,NULL) || nbr<i) goto BAD_MEMO;
        break;
        }
      if(nb<i)i=nb;
      if(!WriteFile(mf,b,i,&nbr,NULL) || nbr<i) goto BAD_MEMO;
      nb-=i;
      }
    break;
  default: goto BAD_MEMO;  // ---------------> No memo or unknown
  }
CloseHandle(mf);
delete b;
return 0;
BAD_MEMO:
CloseHandle(mf);
DeleteFile(file);
delete b; return 14;
}
//-----------------------------------

BYTE dbBase::NextRec(void)
{
DWORD nbr;
if(cur>=dbH.nrec)return 1;
if(!ReadFile(f,rec,dbH.reclen,&nbr,NULL) || nbr<dbH.reclen) return 2;
pos+=dbH.reclen;
cur++;
return 0;
}
//-------------------------------

dbField *dbBase::FiNum(WORD n)
{
for(cf=dbF;n&&cf;n--) cf=(dbField*)(cf->Next());
return cf;
}
//===========================================================================
WORD dbBase::FiWidth(void)
{
WORD i=cf->filen;
switch(cf->type) {
//  case 'D': if(fmtD)i=DTw(fmtD); else i=10; break;
  case 'D': i=DTw(fmtD); break;
  case 'L': i=1; break;
  case 'M': case 'G': case 'P': i=4; break;
//  case 'T': if(fmtT)i=DTw(fmtT); else i=19; break;
  case 'T': i=DTw(fmtT); break;
  case '0': i*=2; break;
  case 'I': i=11; break;
  case 'B': case 'Y': i=21; break;
  }
if(cf->mskN && i<4) i=4;
return i;
}
//===========================================================================

char *dbBase::FiType(char *s)
{
char *c="%c%3u.%u  %s";
switch(cf->type) {
  case 'C': FSF.sprintf(s,c,'C',cf->filen,cf->dec,"Character"); break;
  case 'N': FSF.sprintf(s,c,'N',cf->filen,cf->dec,"Number"); break;
  case 'D': FSF.sprintf(s,c,'D',cf->filen,cf->dec,"Date"); break;
  case 'L': FSF.sprintf(s,c,'L',cf->filen,cf->dec,"Logical"); break;
  case 'M': FSF.sprintf(s,c,'M',cf->filen,cf->dec,"Memo"); break;
  case 'T': FSF.sprintf(s,c,'T',cf->filen,cf->dec,"DateTime"); break;
  case 'I': FSF.sprintf(s,c,'I',cf->filen,cf->dec,"Integer"); break;
  case 'F': FSF.sprintf(s,c,'F',cf->filen,cf->dec,"Float"); break;
  case 'B': FSF.sprintf(s,c,'B',cf->filen,cf->dec,"Double"); break;
  case 'Y': FSF.sprintf(s,c,'Y',cf->filen,cf->dec,"Currency"); break;
  case '0': FSF.sprintf(s,c,'0',cf->filen,cf->dec,"System"); break;
  case 'G': FSF.sprintf(s,c,'G',cf->filen,cf->dec,"General"); break;
  case 'P': FSF.sprintf(s,c,'P',cf->filen,cf->dec,"Picture"); break;
  case 'V': FSF.sprintf(s,c,'V',cf->filen,cf->dec,"VarChar"); break;
  case 'Q': FSF.sprintf(s,c,'Q',cf->filen,cf->dec,"VarBinary"); break;
  case '#': FSF.sprintf(s,c,'#',cf->filen,cf->dec,"Dummy"); break;
  default : FSF.sprintf(s,c,cf->type,cf->filen,cf->dec,"Unknown");
  }
if(cf->mskN) lstrcat(s," Null");
return s;
}
//-----------------------------------

BYTE dbBase::FiChar(void)
{
if(cf->type=='C') return 1;
if(cf->type=='V') return 1;
if(cf->type=='Q') return 1;
return 0;
}
//-----------------------------------

BYTE dbBase::FiNull(void)
{
if(!cf->mskN) return 0;
if(cf->mskN & Nflg[cf->indN]) return 1;
return 0;
}
//-----------------------------------

BYTE dbBase::FiNotFull(void)
{
if(!cf->mskV) return 0;
if(cf->mskV & Nflg[cf->indV]) return 1;
return 0;
}
//-----------------------------------

WORD dbBase::FiDisp(char *s, BYTE nll)
{
WORD n;
BYTE *c, fle;
if(!cf)return 0;
if(nll && FiNull()) { lstrcpy(s,"Null"); return 4; }
c=rec+cf->loc;
fle=cf->filen;
switch(cf->type) {
 case 'T': // DateTime
           if(fle == 8) { // Binary format
             union { DWORD w; BYTE c[4]; } u,v;
             __int32 d;
             SYSTEMTIME t;
             for(n=0;n<4;n++)u.c[n]=c[n];
             for(n=4;n<8;n++)v.c[n-4]=c[n];
             DTt2t(v.w,&t); // целое время --> SYSTEMTIME
             d=(u.w>1721410U)? u.w-1721410U : 0;
             DTd2t(d,&t);  // целая дата --> SYSTEMTIME
             DTstr(s,&t,fmtT);
             }
           else { // Character format
             char r[24];
             n=fle; if(n>23)n=23;
             CopyMemory(r,c,n); r[n]=0;
             DTstr(s,r,fmtT);
             }
           break;
 case 'I': { // Integer
             union { __int32 w; BYTE c[4]; } u;
             for(n=0;n<4;n++)u.c[n]=c[n];
             FSF.sprintf(s,"%11ld",u.w);
             }
           break;
 case 'B': { // Double
             union { double w; BYTE c[8]; } u;
             int dec;
             for(n=0;n<8;n++)u.c[n]=c[n];
             dec=cf->dec; if(!dec)dec=14;
             FSF.sprintf(s,"%21.*g",dec,u.w);
             }
           break;
 case 'Y': { // Currency
             union { __int64 w; BYTE c[8]; } u;
             for(n=0;n<8;n++)u.c[n]=c[n];
             i64_a(s,u.w,21,4);
             }
           break;
 case 'G': /*{ // General
             union { WORD w; BYTE c[2]; } u,v;
             for(n=0;n<2;n++)v.c[n]=c[n];
             for(n=2;n<4;n++)u.c[n-2]=c[n];
             FSF.sprintf(s,"%04X:%04X",u.w,v.w);
             } */
           lstrcpy(s,"Gnrl");
           break;
 case 'P':  // Picture
           lstrcpy(s,"Pict");
           break;
 case 'D': {// ASCII Date
             char r[16];
             n=fle; if(n>15)n=15;
             CopyMemory(r,c,n); r[n]=0;
             DTstr(s,r,fmtD);
             }
           break;
 case 'M': {// Memo
             __int64 w=(fle<10)? GetBinary(): Get64();
             if(w)lstrcpy(s,"Memo");
//             FSF.sprintf(s,"m%10Ld",w);
             }
           break;
 case '0': Bin_Hex(s,c,fle);
           break;
 case 'V': case 'Q': // Variable length char or binary
           if(FiNotFull()) fle=c[cf->filen-1];
 default : for(n=0;n<fle;n++) {
             s[n]=c[n];
             if(!s[n])s[n]=' ';
             }
           s[n]=0;
 }
n=lstrlen(s);
return n;
}
//-----------------------------------

WORD dbBase::FiDispE(char *s)
{
if(!cf)return 0;
WORD i,n;
n = FiDisp(s,0);
if(cf->type=='C' || cf->type=='#') {
  FSF.RTrim(s); return lstrlen(s);
  }
if(cf->type!='I' && cf->type!='B' && cf->type!='Y' &&
   cf->type!='N' && cf->type!='F') return n;
for(i=0; s[i] && s[i]==' '; i++);
if(!i) return n;
MoveMemory(s,s+i,n+1);
return n;
}
//-----------------------------------

void dbBase::Accum(dbVal *V)
{
if(!cf)return;
int n;
BYTE *c=rec+cf->loc;
switch(cf->type) {
  case 'I': { // Integer
           union {__int32 w; BYTE c[4]; } u;
           for(n=0;n<4;n++)u.c[n]=c[n];
           V->I+=u.w; return; }
  case 'B': { // Double
           union { double w; BYTE c[8]; } u;
           for(n=0;n<8;n++)u.c[n]=c[n];
           V->D+=u.w; return; }
  case 'Y': { // Currency
           union {__int64 w; BYTE c[8]; } u;
           for(n=0;n<8;n++)u.c[n]=c[n];
           V->I+=u.w; return; }
  case 'N': // Number
  case 'F': // Float
           if(cf->filen<21) V->I+=Get64();
           else V->D+=GetDouble();
  }
}
//-----------------------------------

__int64 dbBase::Get64(void)
{
if(!cf) return 0;
if(cf->type!='N' && cf->type!='F' && cf->type!='M')return 0;
char *c=rec+cf->loc;
int i, sign=1,j=0;
__int64 q=0;
for(i=0;i<cf->filen;c++,i++) {
  if((*c==' ')||(*c=='+'))continue;
  if(*c=='-') {sign=-1; continue;}
  if(*c=='.') {j=cf->dec-cf->filen+i+1; continue;}// Legal size overflow detected
  if((*c<'0')||(*c>'9')) break;
  q=q*10+(*c-'0');
  }
if(j>0) for(;j;j--)q*=10;
q*=sign;
return q;
}
//-----------------------------------

__int64 dbBase::GetBinary(void)
{
if(!cf)return 0;
int n;
BYTE *c=rec+cf->loc;
if(cf->filen<8) {
 union { __int32 w; BYTE c[4]; } u;
 for(n=0;n<4;n++)u.c[n]=c[n];
 return u.w;
 }
union {__int64 w; BYTE c[8]; } u;
for(n=0;n<8;n++)u.c[n]=c[n];
return u.w;
}
//-----------------------------------

BYTE *dbBase::GetByte(BYTE *s)
{
CopyMemory(s,rec+cf->loc,cf->filen);
s[cf->filen]=0;
return s;
}
//-----------------------------------

void dbBase::SetByte(BYTE *s)
{
CopyMemory(rec+cf->loc,s,cf->filen);
}
//-----------------------------------

double dbBase::GetDouble(void)
{
if(!cf) return 0;
if(cf->type!='N' && cf->type!='F' && cf->type!='C')return 0;
char *c;
int i,sign,j;
double q,qq;
sign=1; j=0;
c=rec+cf->loc;
q=0;
for(i=0;i<cf->filen;c++,i++) {
  if((*c==' ')||(*c=='+')) { if(j)break; continue; }
  if(*c=='-') { if(j)break; sign=-1; continue; }
  if(*c=='.') { if(j)break; j=10; continue; }
  if((*c<'0')||(*c>'9')) break;
  if(j) { qq=(*c-'0'); q+=qq/j; j*=10; }
  else q=q*10+(*c-'0');
  }
q*=sign;
return q;
}
//-----------------------------------

void dbBase::SetNull(void)
{
if(!cf->mskN) return;
Nflg[cf->indN] |= cf->mskN;
return;
}
//-----------------------------------

void dbBase::SetNotNull(void)
{
if(!cf->mskN) return;
Nflg[cf->indN] &= ~cf->mskN;
return;
}
//-----------------------------------

void dbBase::SetNotFull(void)
{
if(!cf->mskV) return;
Nflg[cf->indV] |= cf->mskV;
return;
}
//-----------------------------------

void dbBase::SetFull(void)
{
if(!cf->mskV) return;
Nflg[cf->indV] &= ~cf->mskV;
return;
}
//-----------------------------------

BYTE dbBase::Numeric(void)
{
if(!cf) return 0;
if(cf->type=='N'||cf->type=='F'||cf->type=='I'
                ||cf->type=='B'||cf->type=='Y')return 1;
return 0;
}
//-----------------------------------

BYTE dbBase::Write(void)
{
DWORD nbr;
if(!WriteFile(f,rec,dbH.reclen,&nbr,NULL) || nbr<dbH.reclen) return 2;
pos+=dbH.reclen;
upd|=1;
cur++;
if(cur>dbH.nrec)dbH.nrec++;
return 0;
}
//-----------------------------------

BYTE dbBase::Append(void)
{
DWORD nbr;
pos=dbH.start+dbH.nrec*dbH.reclen;
if(SetFilePointer(f,pos,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
if(!WriteFile(f,rec,dbH.reclen,&nbr,NULL) || nbr<dbH.reclen) return 2;
pos+=dbH.reclen; upd|=1; dbH.nrec++;
cur=dbH.nrec;
return 0;
}
//-------------------------------

BYTE dbBase::ReWrite(void)
{
if(!cur) return 1;
DWORD p;
p=pos-dbH.reclen;
if(SetFilePointer(f,p,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
if(!WriteFile(f,rec,dbH.reclen,&p,NULL) || p<dbH.reclen) return 2;
if(SetFilePointer(f,pos,NULL,FILE_BEGIN)==0xFFFFFFFF) return 2;
return 0;
}
//-------------------------------

void dbBase::SetLeft(char *fval)
{
WORD i;
for(i=0; i<cf->filen; i++) {
  if(!fval[i]) break;
  rec[cf->loc+i]=fval[i];
  }
if(cf->mskV) {
  if(i<cf->filen) { rec[cf->loc+cf->filen-1]=i; SetNotFull(); }
  else SetFull();
  }
}
//-------------------------------

void dbBase::SetRight(char *fval)
{
int l=lstrlen(fval);
if(!l) return;
l--;
int i;
for(i=cf->loc+cf->filen-1; i>=cf->loc&&l>=0; i--,l--) rec[i]=fval[l];
}
//-------------------------------

void dbBase::SetEmpty(void)
{
BYTE filler=0x20;
if(!cf)return;
if(cf->type=='M'||cf->type=='0'||cf->type=='I'
                ||cf->type=='B'||cf->type=='Y'||cf->type=='G')filler=0;
if(cf->type=='T'&&cf->filen==8)filler=0;
for(WORD i=0; i<cf->filen; i++) rec[cf->loc+i]=filler;
if(cf->mskV) { rec[cf->loc+cf->filen-1]=0; SetNotFull(); }
}
//-------------------------------

BYTE dbBase::IsEmpty(void)
{
if(!cf)return 1;
BYTE filler=0x20;
if(cf->type=='M'||cf->type=='0'||cf->type=='I'
                ||cf->type=='B'||cf->type=='Y'||cf->type=='G')filler=0;
if(cf->type=='T'&&cf->filen==8)filler=0;
for(WORD i=0; i<cf->filen; i++) if(rec[cf->loc+i]!=filler) return 0;
return 1;
}
//-------------------------------

void dbBase::SetField(char *s)
{
WORD n,i;
BYTE *c;
SYSTEMTIME t;
if(!cf)return;
SetNotNull();
if(!(*s)) { SetEmpty(); return; }
for(i=0; s[i] && i<FiWidth(); i++) if(s[i]!=' ') goto NON_ZERO;
SetEmpty(); return;
NON_ZERO:
c=rec+cf->loc;
switch(cf->type) {
 case 'T': // DateTime
           if(i) { SetEmpty(); break; }
           DTsf2t(s,fmtT,&t);
           if(!t.wDay) { SetEmpty(); break; }
           if(cf->filen == 8) { // Binary format
             union { DWORD w; BYTE c[4]; } u,v;
             v.w=t.wHour*3600000L+t.wMinute*60000L+t.wSecond*1000L+t.wMilliseconds;
             u.w=(t.wYear-1)*365+(t.wYear-1)/4+t.wDay;
             for(i=1; i<t.wMonth; i++)u.w+=Mon[i-1];
             if(u.w)u.w+=1721410L;
             for(n=0;n<4;n++)c[n]=u.c[n];
             for(n=4;n<8;n++)c[n]=v.c[n-4];
             }
           else { // Character format s=fmtT -> yyyymmddhhmmss=c
             char r[16];
             DTstr(r,&t,"$2BR");
             CopyMemory(c,r,14);
             }
           break;
 case 'I': { // Integer
           union { long w; BYTE c[4]; } u;
           u.w=a_i64(s+i,0);
           for(n=0;n<4;n++)c[n]=u.c[n];
           }
           break;
 case 'B': { // Double
           union { double w; BYTE c[8]; } u;
           u.w=a_dbl(s+i);
           for(n=0;n<8;n++)c[n]=u.c[n];
//           u.w=1; for(n=0;n<cf->dec;n++)u.w*=10;
//           u.w=a_i64(s+i,0,cf->dec)/u.w;
//           for(n=0;n<8;n++)c[n]=u.c[n];
           }
           break;
 case 'Y': { // Currency
           union {__int64 w; BYTE c[8]; } u;
           u.w=a_i64(s+i,0,4);
           for(n=0;n<8;n++)c[n]=u.c[n];
           }
           break;
 case 'G': // General
 case 'M': // Memo
           break;
 case 'D': // ASCII Date s=dd/mm/yyyy -> yyyymmdd=c
                  //       0123456789    01234567
           if(i) SetEmpty();
           else {
             char r[16];
             DTsf2t(s,fmtD,&t);
             DTstr(r,&t,"$");
             CopyMemory(c,r,8);
             }
           break;
 case 'N': case 'F': {
             char r[32];
             __int64 q=a_i64(s+i,0,cf->dec);
             i64_a(r,q,cf->filen,cf->dec);
             SetEmpty(); SetRight(r);
             }
           break;
 case '0': Hex_Bin(s,c,cf->filen);
           break;
 default : SetEmpty(); SetLeft(s);
 }
return;
}
//-----------------------------------
/*

void MyDebug(char *fmt,char *txt,__int64 p1,__int64 p2,__int64 p3,__int64 p4)
{
HANDLE f;
char buff[512];
f=CreateFile("dbg_dbg.dbg",GENERIC_WRITE,FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,
             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,NULL);
if(f==INVALID_HANDLE_VALUE) return;
SetFilePointer(f,0,NULL,FILE_END);
if(!txt)lstrcpy(buff,fmt);
else if(p1==-999)FSF.sprintf(buff,fmt,txt);
else if(p2==-999)FSF.sprintf(buff,fmt,txt,p1);
else if(p3==-999)FSF.sprintf(buff,fmt,txt,p1,p2);
else if(p4==-999)FSF.sprintf(buff,fmt,txt,p1,p2,p3);
else FSF.sprintf(buff,fmt,txt,p1,p2,p3,p4);
MyWrite(f,buff);
CloseHandle(f);
}
//------------------------------*/


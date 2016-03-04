#define WIN32_LEAN_AND_MEAN

#include "plugin.hpp"

//--------- ����������� ������ �������� ---------------

class Link {
	Link *prev;
	Link *next;
public:
	Link() { prev = next = NULL; };
	void Clear(void) { prev = next = NULL; };
	Link *Prev(void) { return prev; };
	Link *Next(void) { return next; };
	Link *Head(void);         // ������� ������ �������
	Link *Tail(void);         // ������� ����� �������
	Link *Find(int num);     // ������� ����� � ������� num (���� � ������ �� 0)
	void After(Link *m);      // ��������� ����� ����� m ����� this
	void Before(Link *m);     // ��������� ����� ����� m ����� this
	Link *Add(Link *m);       // ���������� ����� ����� m � ����� �������. ���������� m.
							  // this ����� ��������� �� ����� �����. ����� ���� NULL.
	void Extract(void);       // ��������� ����� this � ������� �������
	Link *DestroyAll(void);   // ������� ��� �������. ���������� NULL.
	Link *Step(char &dir);    // ���������� ��������� �������� dir.
	Link *Destroy(char &dir); // ������� ����� � ������� �������.
  /*   ���������� ��������� �� ������ ����� (���� dir='N' �� next, ����� �� prev).
	���� �������������� � ����������� ��������� NULL (��������� ����� ��� ������
	�������), ���������� dir � ������������ ��������� ��������� �����������.
		 ����� �������, � ����� ������ ����� ���������� ������� ������ ���������
	�� ������������ ����� �������. ���� �� �������� NULL, ��� ����� ��������, ���
	������� ��������� ��� �������).
  */
};

//================== �������������� ������ ==================
BYTE Upper(BYTE c);
char *Upper(char *text);
// Dec-������ c ������ � �����. dec ����� ����� �����.
// ��� ������� ���������� def
// ��������: a_i64("12.35",0,3) --> 12350
__int64 a_i64(char *s, __int64 def, char dec = 0);

// ����� � Dec-������. ��� ������� mins ������ �����.
// mins>=0 - ����������� ����� ������, mins<0 - ����������� ����.
// dec ����� ����� �����. ���������� s.
char *i64_a(char *s, __int64 val, int mins = 1, int dec = 0);

// Hex-������ � �����. ��� ������� ���������� def
__int64 ah_i64(char *s, __int64 def);

// ���������� double �� ������ ������� g
double a_dbl(char *s);

// ��������������� ������ ������ b ����� nb � ������ 16-������ ���� s
void Bin_Hex(char *s, BYTE *b, int nb);

// ��������������� ������ 16-������ ���� s � ������ ������ b ����� nb
// ���� s ������, ��� ��������� �� �����, ������� ����������� ������
void Hex_Bin(char *s, BYTE *b, int nb);

//================== �������� � ����� � �������� ==================

//������������� ����:
//  __int32 - ����� ���� � ������ ���
//  Digit (char*) - dd.mm.yy
//  Text  (char*) - 12 �������� 1993 �.

extern BYTE Mon[];  // ������ �������
extern char *DTcode[]; // ������ ����� �������

bool DTbad(SYSTEMTIME *t); // ���������� true, ���� ������ ����
void DTd2t(__int32 d, SYSTEMTIME *t);  // ����� ���� --> SYSTEMTIME
void DTt2t(__int32 ti, SYSTEMTIME *t); // ����� ����� --> SYSTEMTIME
DWORD DTt2dat(SYSTEMTIME *t);  // SYSTEMTIME --> ����� ����
DWORD DTt2tim(SYSTEMTIME *t);  // SYSTEMTIME --> ����� �����
void DTsf2t(char *s, char *f, SYSTEMTIME *t); // ������ �� ������� --> SYSTEMTIME
int MyCmp(const char *s, const char *f);         // ������� = ���-�� ����������. ���������
char *DTs2f(const char *s, char *f);       // ������ ������� � ������
char *DTf2s(const char *f, char *s);       // ������ � ������ �������
char *DTf29(char *f, char *s);       // ������ � ����� ��������������
int DTw(char *f);                    // ������ �� �������
char *DTstr(char *s, SYSTEMTIME *t, char *f); // SYSTEMTIME � ������ �� �������
char *DTstr(char *s, char *d, char *f);  // ��������. ������ � ������ �� �������

//================== ������ ==================

bool NotNum(char c);
bool LatAlphaNum(BYTE c);
bool WinAlphaNum(BYTE c);
bool DosAlphaNum(BYTE c);
BYTE MyWrite(HANDLE h, void *buf, DWORD len = 0);
BYTE MyRead(HANDLE h, void *buf, DWORD len);
void MyDebug(char *fmt, char *txt = NULL, __int64 p1 = -999,
			 __int64 p2 = -999, __int64 p3 = -999, __int64 p4 = -999);

//--------- ������ � ������ ������ ������� DBF ---------------
union dbVal {
	__int64 I;
	double  D;
};

struct dbHeader {
	BYTE  type;
	BYTE  upd[3];
	DWORD nrec;
	WORD  start;
	WORD  reclen;
	WORD  spare1[8];
	BYTE  ind;
	BYTE  spare2[3];
};

struct dbField :Link {
	char  name[11];
	BYTE  type;
	DWORD loc;
	BYTE  filen;
	BYTE  dec;
	BYTE  spare[14];
	BYTE  mskN;
	BYTE  indN;
	BYTE  mskV;
	BYTE  indV;
};

struct dbBase {
	HANDLE f, m;
	char *fmtD, *fmtT; // ��������� �� ������� ���� � ���� �������
	dbHeader  dbH;
	dbField  *dbF;
	dbField  *cf;   // ��������� �� ������� ���� � ������� dbF
	DWORD    pos;   // �������� �� ������ ����� �� ������ ����. ����� �������
	DWORD    cur;   // ����� ������� ������ ���� �� 1. 0 - ������ �� ���������
	BYTE     *rec;  // ������� ������
	WORD     nfil;  // ����� ����� � ������
	WORD     tmem;  // ��� Memo-����� 0-dBaseIII, 1-dBaseIV, 2-FoxPro
	WORD     lmem;  // ����� ����� � Memo-�����
	BYTE     *Nflg; // ������ �� ��������� ���� _NullFlags
	BYTE     *Hext; // ������ �� ���������� ���������
	WORD     lhext; // ����� ���������� ���������
	WORD     upd;

	dbBase() { ZeroMemory(this, sizeof(dbBase)); f = m = INVALID_HANDLE_VALUE; }
	~dbBase() { Close(); }

	BYTE Invalid(void) { return (rec[0] == '*'); };
	// ���������� 1, ���� ������ �������� ��� ��������. ����� 0.
	void Add(char *fname, char ftype, BYTE flen, BYTE fdec);
	// ��������� ��������� ���� � ������� dbF
	void AddF(dbField *of, char *fname = NULL, char ftype = 0, BYTE flen = 0, BYTE fdec = 0);
	// ��������� ����� ��������� ���� of � ������� dbF
	// ������ ����� ����� ���������, ���� ������.
	BYTE AddNull(void);
	// ��������� ��������� ���� _NullFlags � ������� dbF.
	// ���������� ����� ���� _NullFlags.
	// ���� � ���� ���� ��� �������������, ������ �� �����������
	// � ������������ 0
	BYTE Open(char *file, BYTE ronly = 0);
	// ��������� ������������ �� � ��������� ������ ������
	// 0 - OK, 1 - �����
	void OpenMemo(const char *file, const char *mext = NULL);
	// ���� ���� ����, ��������� ��� � ���������� ��� � ����� �����
	// file - ��� ����� �������
	// mext - ������ ����������� ���������� ����-����� �� ���������� �������
	BYTE Create(char *file, BYTE t, dbBase *dc = NULL);
	// ������� ����� ��, ��������� ������ �� �������� dbBase
	// ���� ������ dc, � ����� ���� ���������� ���������� ����������
	// � ��� ��������� ����
	// 0 - OK, 1 - �����
	void SaveHeader(void);
	// �������������� ���������
	void Close(void);
	// ��������� ��

	BYTE NextRec(void);
	// ������ ��������� ������. ������������� pos � cur.
	// �������: 0 - OK, 1 - ������� ������ ���� ���������,
	//          2 - ������ ������ �����.
	BYTE CurrRec(void);
	// ������������ �������� ������. ������������� pos � cur.
	// �������: 0 - OK, 1 - ������� ������ �� ���� (cur=0),
	//          2 - ������ ������ �����.
	BYTE PrevRec(void);
	// ������ ����������� ������. ������������� pos � cur.
	// �������: 0 - OK, 1 - ������� ������ ���� ������,
	//          2 - ������ ������ �����.
	BYTE Read(DWORD rn);
	// ������ ������ � ������� rn (���� �� 1).
	// ������������� pos � cur. ������ ���������� �������.
	// �������: 0 - OK, 1 - rn ��������� ����� �������,
	//          2 - ������ ������ �����.
	BYTE Write(void);
	// ���������� rec � ������� ������� pos.
	// ������������� pos � cur. ������ ���������� �������.
	// �������: 0 - OK, 2 - ������ ������ �����.
	BYTE ReWrite(void);
	// �������������� rec ��� ������� ������.
	// pos � cur �� ����������.
	// �������: 0 - OK, 1 - ������� ������ �� ���� (cur=0),
	//          2 - ������ ������ �����.
	BYTE Append(void);
	// ��������� rec ��� ����� ������ � ����� �����.
	// ������������� pos � cur, ���������� dbH.nrec. ������ ���������� �������.
	// �������: 0 - OK, 2 - ������ ������ �����.

	dbField *FiName(char *fname);
	// ������� � ������� dbF ���� � ������ fname.
	// ������������� cf, ���������� cf.
	// ���� �� �����, ������������� cf=NULL.
	dbField *FiNum(WORD fnum);
	// ������� � ������� dbF ���� � ������� fnum (���� �� 0).
	// ������������� cf, ���������� cf.
	// ���� �� �����, ������������� cf=NULL.
	BYTE FiNull(void);
	// ���������� 1, ���� ������� ���� �� ����� ��������, ����� 0.
	BYTE FiChar(void);
	// ���������� 1, ���� ������� ���� ����������(C,Q,V), ����� 0.
	BYTE FiNotFull(void);
	// ���������� 1, ���� ������� ���� ���������� ����� ��������, ����� 0.
	WORD FiDisp(char* s, BYTE nll = 1);
	// ����������� �������� ���� � ����������� ������ � ����������� ��
	// ��� ���� � �������� � ����� s. ��������� ������ ������� ������.
	// ���������� ����� ������.
	// ���� nll=1 � � ���� ��� �������� (NULL), ���������� ������ "Null".
	// ����� ���������� ���������� ����, ���� ���� ��� �������� ��� Null.
	WORD FiDispE(char* s);
	// �� ��, �� ������� ����� �������. ������������ ��� ��������������.
	// ���������� ���������� ����, ���� ���� ��� �������� ��� Null.
	char *FiType(char *s);
	// ���������� ��������� �������� ���� ������ � ������, ���������
	// � ����-������ 
	WORD FiWidth(void);
	// ���������� ����� �������� ��� ���������� ������������� ����

	void Clear(void);
	// �������� ��������� rec
	void SetEmpty(void);
	// ��������� ��������� ��� ������ ������� ���� � rec.
	void SetNull(void);
	// ������������� NullFlag ��� �������� ����.
	void SetNotNull(void);
	// ���������� NullFlag ��� �������� ����.
	void SetNotFull(void);
	// ������������� ���� ��������� ��� �������� ���� ���������� �����.
	void SetFull(void);
	// ���������� ���� ��������� ��� �������� ���� ���������� �����.
	BYTE IsEmpty(void);
	// ���������� ����� �� ������� ����
	BYTE SetFiEmpty(char *fname);
	// �� �� � ��������������� ������� ���� �� ����� fname.
	// ���� ��, ���������� 0. ���� ���� �� ������� ���������� 1.
	void SetLeft(char *fval);
	// ��������� fval � ������� ���� � rec. ������������ �����.
	BYTE SetFiLeft(char *fname, char *fval);
	// �� �� � ��������������� ������� ���� �� ����� fname.
	// ���� ��, ���������� 0. ���� ���� �� ������� ���������� 1.
	void SetRight(char *fval);
	// ��������� fval � ������� ���� � rec. ������������ ������.
	BYTE SetFiRight(char *fname, char *fval);
	// �� �� � ��������������� ������� ���� �� ����� fname.
	// ���� ��, ���������� 0. ���� ���� �� ������� ���������� 1.
	void SetLong(long fval, char dec = 1);
	// ���� ��� ���� I (Integer*4), �� ��� � �����������. �����,
	// ����������� fval � �����, ��������� ����� � ������ ����� �
	// ��������� ��� � ������� ���� � rec. ������������ ������.
	// ����� �����������, ���� dec=1. ���� �� dec=0 �� ���.
	// ��������:
	// ���� Number 8.2 SetLong(12345[,1]) -->   123.45
	// ���� Number 8.2 SetLong(12345,0)   --> 12345.00
	// ���� Number 8.0 SetLong(12345[,1]) -->    12345
	// ���� Number 8.0 SetLong(12345,0)   -->    12345
	// ���� Number 6.2 SetLong(12345[,1]) -->   123.45
	// ���� Number 6.2 SetLong(12345,0)   -->   ***.**
	BYTE SetFiLong(char *fname, long fval, char dec = 1);
	// �� �� � ��������������� ������� ���� �� ����� fname.
	// ���� ��, ���������� 0. ���� ���� �� ������� ���������� 1.
	void SetDouble(double fval);
	// ����������� fval � ����� � ��������� ��� � ������� ���� � rec.
	// ������������ ������. ��������� ����� ���������� ������.
	BYTE SetFiDouble(char *fname, double fval);
	// �� �� � ��������������� ������� ���� �� ����� fname.
	// ���� ��, ���������� 0. ���� ���� �� ������� ���������� 1.
	void SetField(char *s);
	// ��������� ������� ���� �� ������� ������, ���������� ��� �
	// ����������� �� ���� ����.

	void Accum(dbVal *V);
	BYTE Numeric(void);
	BYTE *GetByte(BYTE *s);
	void SetByte(BYTE *fval);
	BYTE GetMemo(char *file, DWORD *blocknum = NULL);
	char *GetRight(void);
	// ������� ����� ������ � ��������� ���� ������� ����.
	// ���������� ��������� �� ��� ������.
	// ����� ������������� ����� ������� delete.
	// ���������� NULL, ���� �������� ���� ���.
	char *GetRight(char *s);
	// ��������� ������� ���� � ������ s.
	// ���������� ��������� �� ��� ������.
	// ���������� NULL, ���� �������� ���� ���.
	char *GetRight(char *s, WORD n);
	// ��������� ���� � ������� n � ������ s.
	// ���������� ��������� �� ��� ������.
	// ���������� NULL, ���� �������� ���� ���.
	char *GetLeft(char *s);
	// ��������� ������� ���� � ������ s, ����������� ����� (�� ����
	// ������� ����� �������).
	// ���������� ��������� �� ��� ������.
	// ���������� NULL, ���� �������� ���� ���.
	__int64 Get64(void);
	__int64 GetBinary(void);
	long GetLong(void);
	// ����������� ����� � ������� ���� � ����� ����� � ���������� ���.
	// ��������� ��������� � ����� ����. ���� ��� T (DateTime), ���������� 0.
	// ���� ��� I (Integer*4), ���������� ���������� �������� ����.
	// ���� ��� L (Logical), ���������� �������������� 0(false) ��� 1(true).
	// � ��������� ������� �������� ���� ��� ����� � ��������� ����, ������
	// ���������� ����� ������������ � ����������� �������� dec ��� ����:
	// ���� Number 6.2 �������� 123.45 --> GetLong()=12345
	// ���� Number 6.2 �������� 1234.5 --> GetLong()=123450
	char *GetDate(char *s);
	// ��������������� ���� � ������� ���� ���� D (Date) � ������ DateDigit
	// ��������� �������� � s � ���������� ��������� �� ����
	WORD GetDate(void);
	// �������� ����������� ������� ���� ��� ����.
	// ���������� ���� � ������� ������� ������� (��. ����).
	// ���������� ����, ���� ������� �� �������.
	void GetDT(WORD *d, WORD *t);
	// �������� ������� ���� ��� 8-�������� DateTime binary (��� T).
	// ���������� ���� d � ����� t � ������� ������� ������� (��. ����).
	// ���������� ����, ���� �������� ���� ���.
	double GetDouble(void);
	// ����������� ����� � ������� ���� � double ����� � ���������� ���.
};
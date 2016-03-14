#include "stdafx.h"
#include "db_use.h"

#define DTFmtL 32

#define WinCode       0x00000001 // Win or Dos coding
#define FindAllFields 0x00000002 // Search in all fields
#define FindCaseSens  0x00000004 // CaseSensitive searching
#define ExpHeads      0x00000008 // Field names export flag
#define ExpActual     0x00000010 // Only actual records export flag
#define ExpSpaces     0x00000020 // Spaces export flag
#define ExpSeparator  0x00000040 // Use separator with text export flag
#define ExpFileType   0x00000080 // Change File type with dbf export flag
#define CondSearch    0x00000100 // Conditional search flag
#define LineNums      0x00000200 // Show record numbers flag
#define FindInvert    0x00000400 // Find inversion flag
#define FullMemo      0x00000800 // Full screen memo showing flag
#define WholeWords    0x00001000 // Whole words only searching
#define FindReplace   0x00002000 // Search & replace mode
#define ConfReplace   0x00004000 // Replace Confirmation
#define AutoSave      0x00008000 // Tepmlate Autosave
#define ExpEmpty      0x00010000 // Export empty file

enum {
	mColors,
	//-------------------- Errors
	mError, mNoOpen, mBadFile, mBadWrite, mNoMemory, mNoSelect, mNoNumeric,
	mErExport, mLookOnly, mNoMemFile, mNoMemBlock, mBadRepl, mNoTemplate,
	//-------------------- Buttons
	mButOK, mButFind, mButSave, mButAdd, mButCancel,
	//-------------------- Function Keys
	mBarF1, mBarF2, mBarF4, mBarF6, mBarF7, mBarF9, mBarF10,
	mBarSF7, mBarHF5, mBarAF5,
	//----------------------- Configuration
	mCfgCodeDef, mCfgRecNum, mCfgMemo, mCfgSep, mCfgAutoSave, mCfgShow, mCfgEdit,
	mCfgCodeTable, mCfgMemExt,
	//----------------------- Information
	mFileInfo, mEmpty, mFileType, mLastUpdate, mIndexFile, mHeadLen, mRecLen,
	mNumField, mFieldHead, mFieldEmpty,
	//----------------------- Replace
	mReplTitle, mReplFind, mReplRepl, mReplWords, mReplConf, mReplMask, mReplMark,
	mReplUn, mReplAll, mReplAsk, mReplYes,
	//----------------------- Search
	mFindTitle, mFindAll, mFindCase, mFindFirst, mFindMark, mFindUnmark,
	mFindInvert, mFindNo, mFindYes,
	//----------------------- Packing
	mPkTitle, mPkGood,
	//----------------------- Export
	mExTitle, mExName, mExFormat, mExCoding, mExOriginal, mExHead, mExSpace, mExSep,
	mExActual, mExGood, mExBuff, mExpEmpty, mEsc,
	//----------------------- Import
	mImpTitle, mImpFileName,
	//----------------------- Others
	mYes, mNo, mAll, mColIns, mSum, mColName, mEditTitle, mTempFile, mGoTo, mTemplate,
	//----------------------- Html-taggs
	mTabS, mTabF, mTabRowS, mTabRowF, mTabCellS, mTabCellF, mTabSpace
};
//===========================================================================


const char *GetMsg(int MsgId);
//===========================================================================

typedef char Str256[256];

union CondValue {
	__int64 i64;
	double  dbl;
	DWORD   dw[2];
	BYTE    bt[256];
};

struct RecHead {
	WORD Kind;        // Code of record kind
	WORD Len;         // Record length
};

struct FindData {
	short Pos;        // Position of found sample
	short Step;       // Increment of Position of found sample
	short Len;        // Searching Sample Length
	short LenF;       // Found String Length
	short nMM;        // Number of MultyMask characters in sample
	short nMMr;       // Number of MultyMask characters in replase substring
	char Mask[2];     // Masking Characters
	char FD[256];     // Searching Text OEM coding
	char FU[256];     // Searching Text coding ready for compare
	char RU[256];     // Replace Text coding ready for replace
	void Clear(void); // Clear structure
};

struct CondData {
	WORD Oper;        // Operation bitween relations
	WORD Field[2];    // Field numbers for conditions
	WORD Rel[2];      // Relations
	Str256 Str[2];    // Strings for compare
	CondValue V[2];   // Values for compare
};

struct ExportData {
	char File[MAX_PATH];  // Export file name
	char Sep[2];          // Export file separator
	char Type[2];         // Export file type for DBF export
	char code;            // Export file coding operation
	char mode;            // Export mode
	WORD Form;            // Export file type (txt, htm or dbf)
	WORD CoType;          // Export file coding type;
	short fi[3];          // Field numbers for sorting (-1 - no sorting)
	short dir[3];         // Sorting direction 0 - (0->99), 1 - (99->0)
	DWORD recnum;         // Current record number
	DWORD count;          // Count of exported records
	short BufLim;         // Sorting buffer size
	short BufLast;        // Records q-ty in Sorting buffer
	short BufCurr;        // Record number for output from Sorting buffer
	DWORD *RN;            // Record numbers buffer
	CondValue *V0, *V1, *V2;// Sorting keys buffers
};

struct Indicator {
	DWORD limit, count;
	WORD tot, already, X;
	void Start(const char *title, DWORD lim);
	bool Move(DWORD step);
};

struct Column :Link {
	short wid;
	short finum;
	short pos;
	BYTE dinum, idnum;
	char  name[26];
	void Put(Column *c);
};

struct LOOK {
	Column *C;        // Array of columns information
	CHAR_INFO *VBuf;  // Screen virtual buffer
	DWORD *recV;      // Numbers of visible records
	WORD   *M;        // Array of record marks
	WORD   *S;        // Array of record sorted marks
	dbBase db;        // Data Base structure

	short LookOnly;   // Prohibit edit, append and detete functions
	short MarkOnly;   // On/Off marked records only mode of displaying
	DWORD MarkNum;    // Number of marked records
	DWORD MarkMax;    // Maximum number of marked records
	short sw, sh;      // Screen width and height-1
	BYTE  at[24];     // Colors array
	short Wrec;       // Width of current line number showing
	short Wcol;       // Width of current column number showing
	short FindMax;    // Maximum Length of Searching Text showing
	short sXcode;     // X of Win/Dos coding showing
	short sXfind;     // X of find string showing
	short sXcol;      // X of columns number showing
	short sXrec;      // X of current record number showing

	WORD HidNum;      // Number of hidden columns
	Column *Hid;      // Array of hidden columns
	Column *coFirst;  // Pointer to 1 visible column structure
	Column *coCurr;   // Pointer to current column structure
	Column *coLast;   // Pointer to Last visible column
	short coTail;     // Number of reduced characters in Last visible column
	short Xcur;       // Current column start coordinate
	short curX;       // Current column number
	short curY;       // Current line number
	short botY;       // Last line number
	char FileName[MAX_PATH];

	DWORD Flags;          // Any Flags Set
	DWORD FlagsEntry;     // Flags Set without template influence
	FindData Find;        // Find data container
	CondData Cond;        // Condition data container
	ExportData Exp;       // Export data container
	int ctsNum;           // Decoding table number
	CharTableSet *ChaTa;  // Pointer to decoding table
	WORD(LOOK::*ExpRec)(void);
	Indicator Indic;

	void Set(DWORD flg) { Flags |= flg; }
	void Clear(DWORD flg) { Flags &= ~flg; }
	bool Yes(DWORD flg) { return (Flags&flg) == flg; }
	bool No(DWORD flg) { return (Flags&flg) != flg; }
	bool YesAlphaNum(BYTE c);

	WORD SortAlloc(void);
	void SortFree(void);
	void SortSet(DWORD recnum);
	void SortClear(DWORD recnum);
	bool Sorted(DWORD recnum);
	void SortSetAll(void);
	void SortClearAll(void);
	void SortSetMarked(void);

	WORD MarkAlloc(void);
	void MarkFree(void);
	void MarkSet(DWORD recnum);
	void MarkClear(DWORD recnum);
	void MarkInvert(DWORD recnum);
	bool Marked(DWORD recnum);
	void MarkSetAll(void);
	void MarkClearAll(void);
	void MarkInvertAll(void);
	void MarkFindFirst(void);
	void MarkNonActual(void);

	WORD GoNextMark(void);
	WORD GoPrevMark(void);
	WORD GoUp(WORD delta);
	WORD GoDn(WORD delta);
	WORD GoTop(void);
	WORD GoBot(void);
	WORD GoLeft(void);
	WORD GoRight(void);
	WORD GoFirst(void);
	WORD GoLast(void);
	WORD GoFind(short ma = 3);
	WORD GoField(int id);
	WORD GoAsk(void);
	WORD GoMouse(MOUSE_EVENT_RECORD *m);
	WORD TopMouse(int Line, MOUSE_EVENT_RECORD *m);

	bool BuildBuffers(bool ResizeFlag = false);
	void InitColumns(void);
	void GetScreenSize(void);
	void GetConfig(bool inrun = false);
	void DefColors(char *u);
	void AttrRect(WORD left, WORD top, WORD width, WORD height, WORD Attr);
	void AttrLine(WORD left, WORD width, WORD Y, WORD Attr);
	void ClearRect(WORD left, WORD top, WORD width, WORD height);

	short ReplaceDlg(void);
	short ReplaceAsk(void);
	short Replace(void);
	short ReplaceStr(char *cc);
	void ReplaceAll(short ma, DWORD mrn);
	bool NotRepl(DWORD rn, short ma);
	short FindAskSample(void);
	DWORD FindNext(short *f, short ma);
	DWORD FindMark(WORD m);
	short FindCompare(WORD fn);
	short CondAsk(void);
	void CondVal(WORD n);
	bool Like(BYTE *sample, BYTE *line);
	bool CondCheck(short n, BYTE *fi);
	bool CondCheck(short n, __int64 fi);
	bool CondCheck(short n, double fi);
	bool CondCheck(short n, DWORD fi1, DWORD fi2);
	bool CondCompare(short n);
	bool CondYes(void);
	DWORD CondNext(void);

	Column *FindFin(short fin);
	Column *FieldItem(short it);
	char *FieldItemType(short it, char *buf);

	WORD Export(void);
	WORD ExpTxt(void);
	WORD ExpHtm(void);
	WORD ExpDbf(void);
	WORD PackDbf(void);
	char *NameScratch(char *nm);
	WORD ExpRec0(void);
	WORD ExpRec1(void);
	WORD ExpRec2(void);
	void FieldValue(WORD finum, CondValue *val);
	short FieldCompare(short finum, CondValue *v1, CondValue *v2);
	WORD Import(void);

	WORD ClmnDelete(void);
	WORD ClmnInsert(void);
	WORD ClmnMove(char d);
	WORD ClmnNarrow(void);
	WORD ClmnEnlarge(void);
	WORD ClmnRename(void);
	WORD ClmnScroll(short d);

	WORD ActualRecord(void);
	WORD ActualSelected(BYTE a);
	WORD EditRecord(void);
	WORD EditField(BYTE nll = 0);
	void EditHeader(void);
	char CurType(void);
	void KeyShow(void);
	void ChangeCode(void);
	void Clipboard(void);
	void ClearCur(void);

	int AskMsg(char *s);
	int OkMsg(char *s);
	void ShowCur(void);
	void ShowError(int index);
	void ShowExpMsg(DWORD msec);
	void ShowFindMsg(DWORD nr);
	void ShowReplMsg(DWORD nr);
	void ShowPage(void);
	int  ShowFields(void);
	void ShowMemo(short id);
	void ShowStr(const char *str, WORD x, WORD y, BYTE atn = 0, WORD L = 0);
	void ShowStrI(const char *str, WORD x, WORD y, WORD L);
	void ShowChar(char c, WORD x, WORD y, BYTE atn = 0);
	void ShowF5(void);
	void ShowStatus(int index);
	void ShowSum(void);
	void ShowDBF(void);
	void NameTemplate(char *nm);
	void GetTemplate(void);
	void PutTemplate(bool msg = false);
	void DelTemplate(void);
	int  TableSelect(void);
	int  TableSet(int nt);
	void ToOem(TCHAR *src, TCHAR *dst = NULL);
	void ToAlt(TCHAR *src, TCHAR *dst = NULL);
};

#include "LookDBF.h"
#include "farkeys.hpp"

#define DM_LOOKDBF DM_USER + 1

extern PluginStartupInfo Info;
extern FARSTANDARDFUNCTIONS FSF;
extern const char *Title;
extern LOOK *data;
static const BYTE atd[24] = { 0x07,0x30,0x1f,0x1b,0x30,0x4b,0x1e,0x3e,
						   0x4e,0x30,0x30,0x31,0x3b,0x30,0x3e,0x3b,
						   0x78,0x5b,0x5e,0x30,0x30,0x30,0x30,0x30 };
static char *aw;
static char fmtD[DTFmtL], fmtT[DTFmtL], fmtDE[DTFmtL], fmtTE[DTFmtL], fmtDV[DTFmtL], fmtTV[DTFmtL];
static char T_Mask[DTFmtL] = "99/99/9999-99:99:99";
static char D_Mask[DTFmtL] = "99/99/9999";
const char *DefD = "dd/mm/yyyy";
const char *DefT = "dd/mm/yyyy-hh:mi:se";
const char *DefMemExt = "DBC:DCT,FRX:FRT,SCX:SCT,MNX:MNT,VCX:VCT,PJX:PJT";
char DefExpSep = '\xb3';
static const char *C_OPER[7] = { " N/A"," AND"," OR"," XOR"," NXO",
						"0 \x1A 99","99 \x1A 0" };
static const char *C_REL[6] = { "  ="," <>","  >"," >=","  <"," <=" };
//================== Прочее ==================

static BYTE MyWrite(HANDLE h, LPVOID buf, DWORD len = 0)
{
	DWORD nbr;
	if (!len)len = lstrlen((TCHAR *) buf) * sizeof(TCHAR);
	if (!WriteFile(h, buf, len, &nbr, NULL) || nbr < len) return 1;
	return 0;
}

static BYTE MyRead(HANDLE h, LPVOID buf, DWORD len)
{
	DWORD nbr;
	if (!ReadFile(h, buf, len, &nbr, NULL) || nbr < len) return 1;
	return 0;
}
//===========================================================================

static BOOL CheckForEsc(void)
{
	DWORD NumberOfEvents;
	DWORD ReadCnt, i;
	BOOL result = FALSE;
	HANDLE Console = GetStdHandle(STD_INPUT_HANDLE);
	if (!GetNumberOfConsoleInputEvents(Console, &NumberOfEvents)) return result;
	if (!NumberOfEvents) return result;
	INPUT_RECORD *InputRec = new INPUT_RECORD[NumberOfEvents];
	if (!InputRec) return result;
	if (ReadConsoleInput(Console, InputRec, NumberOfEvents, &ReadCnt))
		for (i = 0; i < ReadCnt; ++i) {
			if (InputRec[i].EventType != KEY_EVENT) continue;
			if (!InputRec[i].Event.KeyEvent.bKeyDown) continue;
			if (InputRec[i].Event.KeyEvent.wVirtualKeyCode != VK_ESCAPE) continue;
			result = TRUE;
			break;
		}
	delete[] InputRec;
	return result;
}
//===========================================================================

void FindData::Clear(void)
{
	ZeroMemory(this, sizeof(FindData));
	Mask[0] = '?'; Mask[1] = '%';
}
//===========================================================================

void Column::Put(Column *c)
{
	wid = c->wid; pos = c->pos; finum = c->finum;
	dinum = c->dinum; idnum = c->idnum;
	lstrcpy(name, c->name);
}
//===========================================================================

void Indicator::Start(const char *title, DWORD lim)
{
	const char *Esc = GetMsg(mEsc);
	int i;
	limit = lim; count = 0; already = 0; X = 0;
	Info.Text(0, data->sh, data->at[1], title);
	X = data->sw - lstrlen(Esc);
	Info.Text(X, data->sh, data->at[1], Esc);
	i = lstrlen(title); tot = X - i; X = i;
	for (i = 0; i < tot; i++) Info.Text(X + i, data->sh, data->at[1], "\xb2");
	Info.Text(X, data->sh, data->at[1], NULL);
}

bool Indicator::Move(DWORD step)
{
	int i;
	WORD x;
	if (CheckForEsc()) return true;
	Sleep(0);
	count += step;
	x = (count*tot + (limit >> 1)) / limit;
	if (x <= already) return false;
	if (x > tot) return false;
	for (i = already; i < x; i++) Info.Text(X + i, data->sh, data->at[1], "\xdb");
	already = x;
	Info.Text(X, data->sh, data->at[1], NULL);
	return false;
}
//===========================================================================

bool LOOK::YesAlphaNum(BYTE c)
{
	if (Yes(WinCode)) return IsCharAlphaNumeric(c) != FALSE;
	return FSF.LIsAlphanum(c) != FALSE;
}
//===========================================================================

void LOOK::ToOem(TCHAR *src, TCHAR *dst)
{
	if (!dst) dst = src;
	if (!ChaTa) { CharToOem(src, dst); return; }
	for (; *src; src++, dst++) *dst = ChaTa->DecodeTable[*src];
	*dst = 0;
}
//===========================================================================

void LOOK::ToAlt(TCHAR *src, TCHAR *dst)
{
	if (!dst) dst = src;
	if (!ChaTa) { OemToChar(src, dst); return; }
	for (; *src; src++, dst++) *dst = ChaTa->EncodeTable[*src];
	*dst = 0;
}
//===========================================================================

void LOOK::ShowError(int index)
{
	if (!index) return;
	const TCHAR *Msg[6];
	BYTE ndx = 0;
	if (index < 4) {
		Msg[ndx++] = Title;
		Msg[ndx++] = FileName;
	}
	else {
		Msg[ndx++] = GetMsg(mError);
		if (index > 40) { Msg[ndx++] = GetMsg(mTempFile); index %= 20; }
		if (index > 20) { Msg[ndx++] = Exp.File; index %= 20; }
	}
	Msg[ndx++] = TEXT("");
	const TCHAR *MsgDetail;
	switch (index) {
	case 1: MsgDetail = GetMsg(mNoOpen); break;
	case 2: MsgDetail = GetMsg(mBadFile); break;
	case 3: MsgDetail = GetMsg(mBadWrite); break;
	case 4: MsgDetail = GetMsg(mNoMemory); break;
	case 5: MsgDetail = GetMsg(mNoSelect); break;
	case 6: MsgDetail = GetMsg(mNoNumeric); break;
	case 7: MsgDetail = GetMsg(mErExport); break;
	case 8: MsgDetail = GetMsg(mLookOnly); break;
	case 9: MsgDetail = GetMsg(mNoMemFile); break;
	case 10: MsgDetail = GetMsg(mNoMemBlock); break;
	case 11: MsgDetail = GetMsg(mBadRepl); break;
	case 12: MsgDetail = GetMsg(mNoTemplate); break;
	default: MsgDetail = TEXT("*<*>*!*<*>*");
	}
	Msg[ndx++] = MsgDetail;
	Msg[ndx++] = TEXT("\x01");
	Info.Message(Info.ModuleNumber, FMSG_WARNING | FMSG_MB_OK, TEXT("Functions"), Msg, ndx, 0);
}
//===========================================================================

void LOOK::ShowMemo(short id)
{
	DWORD nb;
	if (db.Read(recV[curY])) { ShowError(2); return; }
	char fname[MAX_PATH];
	FSF.MkTemp(fname, "memo");
	switch (db.GetMemo(fname, &nb)) {
	case 1: case 2: return;
	case 11: ShowError(9); return;
	case 12: ShowError(4); return;
	case 13: ShowError(43); return;
	case 14: ShowError(10); return;
	}
	int y1, y2 = (sh + 1) >> 1;
	if (Yes(FullMemo)) { y1 = 0; y2 = -1; }
	else if (curY + 2 < y2) { y1 = y2; y2 = sh; }
	else { y1 = 0; y2--; }
	char title[64], nafi[32];
	if (Yes(WinCode)) ToOem(coCurr->name, nafi);
	FSF.sprintf(title, "%sД%luД%lu", nafi, recV[curY], nb);
	if (id) Info.Editor(fname, title, 0, y1, -1, y2,
						VF_DISABLEHISTORY | VF_DELETEONLYFILEONCLOSE, 0, 1);
	else Info.Viewer(fname, title, 0, y1, -1, y2,
					 VF_DISABLEHISTORY | VF_DELETEONLYFILEONCLOSE);
}
//===========================================================================

void LOOK::ShowStatus(int index)
{
	short i;
	char s[256];
	/*           WrecWcol           Wcol     Wrec  Wrec
	filename Dos#####hhh a!(Search) ###/### #####/#####
			 |           |          |       |
		   sXcode       sXfind     sXcol   sXrec
	*/
	if (index < 1) { //<------- Initialization
		AttrRect(0, 0, sw, 1, at[9]);  ShowStr(FileName, 1, 0);
		sXcode = lstrlen(FileName) + 2;
		if (Yes(WinCode)) { ShowStr(aw + 1, sXcode, 0, 10); ShowStr(" DOS", 58, sh, 0, 6); }
		else { ShowStr("DOS", sXcode, 0, 10); ShowStr(aw, 58, sh, 0, 6); }
		FSF.sprintf(s, "1/%lu", db.dbH.nrec); Wrec = lstrlen(s) - 1;
		sXrec = sw - Wrec * 2 - 1;  ShowStr(s, sXrec + Wrec - 1, 0, 13);
		FSF.sprintf(s, "1/%u", db.nfil); Wcol = lstrlen(s) - 2;
		sXcol = sXrec - 1 - Wcol * 2;  ShowStr(s, sXcol + Wcol - 1, 0, 12);
		sXfind = sXcode + Wrec + Wcol + 4; FindMax = sXcol - sXfind - 1;
		if (index == 0) return;
	}
	if (index < 2) { //<------------ Change current record
		FSF.sprintf(s, "%*lu", Wrec, recV[curY]);
		ShowStr(s, sXrec, 0, 13, Wrec);
		if (index > 0) return;
	}
	if (index < 3) { //<------------ Change current column
		FSF.sprintf(s, "%*u", Wcol, coCurr->finum + 1);
		ShowStr(s, sXcol, 0, 12, Wcol);
		if (index > 0) return;
	}
	if (index < 4) { //<------------- Change marked records number
		i = sXcode + 3; ClearRect(i, 0, Wrec, 1);
		if (MarkNum) { FSF.sprintf(s, "%*lu", Wrec, MarkNum); ShowStr(s, i, 0, 14, Wrec); }
		if (index > 0) return;
	}
	if (index < 5) { //<------------- Change hidden columns number
		i = sXcode + 3 + Wrec; ClearRect(i, 0, Wcol, 1);
		if (HidNum) { FSF.sprintf(s, "%*u", Wcol, HidNum); ShowStr(s, i, 0, 15, Wcol); }
		if (index > 0) return;
	}
	if (index<6) { //<------------- Fill Search string place
		ClearRect(sXfind, 0, FindMax, 1);
		if (Yes(CondSearch)) {
			Column *c = FindFin(Cond.Field[0]);
			FSF.sprintf(s, "(%s%s %s)", c->name, C_REL[Cond.Rel[0]], Cond.Str[0]);
			if (Cond.Oper) {
				c = FindFin(Cond.Field[1]);
				FSF.sprintf(s + lstrlen(s), "%s (%s%s %s)", C_OPER[Cond.Oper],
							c->name, C_REL[Cond.Rel[1]], Cond.Str[1]);
			}
		}
		else {
			lstrcpy(Find.FU, Find.FD);
			if (No(FindCaseSens))FSF.LStrupr(Find.FU);
			if (Yes(WinCode))ToAlt(Find.FU);
			s[0] = Yes(FindCaseSens) ? 'a' : 'A'; s[1] = Yes(FindAllFields) ? '\x13' : '!';
			s[2] = '<'; s[3] = 0; lstrcat(s, Find.FD); lstrcat(s, ">");
		}
		if (lstrlen(s)>FindMax) {
			s[FindMax] = 0; s[FindMax - 1] = s[FindMax - 2] = s[FindMax - 3] = '.';
		}
		i = FindMax - lstrlen(s); i >>= 1;
		ShowStr(s, sXfind + i, 0, 11);
		if (index > 0) return;
	}
	if (index < 7) { //<------------ Change Total number of records
		FSF.sprintf(s, "%ld", db.dbH.nrec);
		ShowStr(s, sw - Wrec, 0, 13);
		if (index > 0) return;
	}
}
//===========================================================================

void LOOK::ShowExpMsg(DWORD msec)
{
	TCHAR Buffer1[MAX_PATH + 4 + 1];
	TCHAR Buffer2[256];
	const TCHAR *Msg[] = {
		GetMsg(mExTitle),
		Buffer1,
		TEXT(""),
		Buffer2,
		TEXT("\x01")
	};
	lstrcpyn(Buffer1, Exp.File, MAX_PATH); switch (Exp.Form) {
	case 0: lstrcat(Buffer1, ".txt"); break;
	case 1: lstrcat(Buffer1, ".htm"); break;
	case 2: lstrcat(Buffer1, ".dbf"); break;
	}
	FSF.snprintf(Buffer2, 255, GetMsg(mExGood), Exp.count, msec);
	Info.Message(Info.ModuleNumber, FMSG_MB_OK, TEXT("Export"), Msg, ARRAYSIZE(Msg), 0);
}
//===========================================================================

void LOOK::InitColumns(void)
{
	WORD i, n;
	C[0].Clear();
	for (i = 0; i < db.nfil; i++) {
		db.FiNum(i);  C[i].finum = i;
		lstrcpy(C[i].name, db.cf->name);
		C[i].wid = db.FiWidth() + 1;
		n = lstrlen(C[i].name) + 2; if (C[i].wid < n) C[i].wid = n;
		n = sw - 3; if (C[i].wid > n)C[i].wid = n;
		if (i)C[i - 1].After(C + i);
	}
	Hid = coLast = NULL; coCurr = coFirst = C;
	curX = 0; curY = 0;
}
//===========================================================================

bool LOOK::BuildBuffers(bool ResizeFlag)
{
	if (!ResizeFlag) {
		if (C) { delete[] C; }
		C = new Column[db.nfil];
		if (!C) return true;
		InitColumns();
	}
	if (VBuf) { delete VBuf; VBuf = NULL; }
	DWORD Vir = (sh - 2) * sizeof(DWORD) + 16; Vir >>= 3; Vir <<= 3;
	DWORD Vis = sw * (sh + 1) * sizeof(CHAR_INFO) + 32; Vis >>= 3; Vis <<= 3;
	BYTE *v = new BYTE[Vis + Vir];
	if (!v) return true;
	VBuf = (CHAR_INFO *) v; v += Vis;
	recV = (DWORD *) v;
	return false;
}
//===========================================================================

WORD LOOK::SortAlloc(void)
{
	if (S) return 0;
	DWORD i;
	i = db.dbH.nrec + 256; i >>= 6; i <<= 2;
	S = new WORD[i];
	if (!S) { ShowError(4); return 1; }
	Exp.mode = 0; if (Exp.fi[1] >= 0)Exp.mode = 1; if (Exp.fi[2] >= 0)Exp.mode = 2;
	Exp.RN = new DWORD[Exp.BufLim];
	if (!Exp.RN) { ShowError(4); SortFree(); return 1; }
	i = Exp.BufLim*(Exp.mode + 1);
	Exp.V0 = new CondValue[i];
	if (!Exp.V0) { ShowError(4); SortFree(); return 1; }
	Exp.V1 = Exp.V2 = NULL;
	if (Exp.mode) {
		Exp.V1 = Exp.V0 + Exp.BufLim;
		if (Exp.mode > 1) Exp.V2 = Exp.V1 + Exp.BufLim;
	}
	Exp.BufCurr = Exp.BufLast = 0;
	return 0;
}
//===========================================================================

void LOOK::SortFree(void)
{
	if (Exp.V0) { delete Exp.V0; Exp.V0 = Exp.V1 = Exp.V2 = NULL; }
	if (Exp.RN) { delete Exp.RN; Exp.RN = NULL; }
	if (S) { delete S; S = NULL; }
}
//===========================================================================

void LOOK::SortSet(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4; i &= 0x0fffffff;
	if (j)k >>= j; S[i] |= k;
}
//===========================================================================

void LOOK::SortClear(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4;  i &= 0x0fffffff;
	if (j)k >>= j; S[i] &= ~k;
}
//===========================================================================

bool LOOK::Sorted(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4; i &= 0x0fffffff;
	if (j)k >>= j; return (S[i] & k) != 0;
}
//===========================================================================

void LOOK::SortSetAll(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;  j &= 0x0fffffff;
	for (i = 0; i < j; i++) S[i] = 0xffff;
}
//===========================================================================

void LOOK::SortClearAll(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;
	for (i = 0; i < j; i++) S[i] = 0;
}
//===========================================================================

void LOOK::SortSetMarked(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;
	for (i = 0; i < j; i++) S[i] = M[i];
}
//===========================================================================

WORD LOOK::MarkAlloc(void)
{
	if (M) return 0;
	DWORD i;
	i = db.dbH.nrec + 256; i >>= 6; i <<= 2;
	M = new WORD[i];
	if (!M) { ShowError(4); return 1; }
	MarkMax = i << 4; return 0;
}
//===========================================================================

void LOOK::MarkFree(void)
{
	if (!M) return;
	delete M; M = NULL; MarkMax = MarkNum = 0;
}
//===========================================================================

void LOOK::MarkSet(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4; i &= 0x0fffffff;
	if (j)k >>= j; if (!(M[i] & k))MarkNum++; M[i] |= k;
}
//===========================================================================

void LOOK::MarkClear(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4;  i &= 0x0fffffff;
	if (j)k >>= j; if (M[i] & k)MarkNum--; M[i] &= ~k;
}
//===========================================================================

void LOOK::MarkInvert(DWORD recnum)
{
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4; i &= 0x0fffffff; if (j)k >>= j;
	if (M[i] & k) { M[i] &= ~k; MarkNum--; }
	else { M[i] |= k; MarkNum++; }
}
//===========================================================================

bool LOOK::Marked(DWORD recnum)
{
	if (!MarkNum) return false;
	DWORD i, j;
	WORD k = 0x8000;
	i = recnum - 1; j = i & 0x0000000f; i >>= 4; i &= 0x0fffffff;
	if (j)k >>= j; return (M[i] & k) != 0;
}
//===========================================================================

void LOOK::MarkSetAll(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;  j &= 0x0fffffff;
	for (i = 0; i < j; i++) M[i] = 0xffff;
	MarkNum = db.dbH.nrec;
}
//===========================================================================

void LOOK::MarkClearAll(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;
	for (i = 0; i < j; i++) M[i] = 0;
	MarkNum = 0; MarkOnly = 0;
}
//===========================================================================

static WORD BitNum(WORD sam, WORD num)
{
	WORD i, n, k = 0x8000;
	if (!num) return 0;
	for (i = n = 0; i < num; i++) {
		if (i)k >>= 1; if (sam&k)n++;
	}
	return n;
}

void LOOK::MarkInvertAll(void)
{
	DWORD i, j;
	j = db.dbH.nrec + 16; j >>= 4;  j &= 0x0fffffff; MarkNum = 0;
	for (i = 0; i < j; i++) {
		M[i] = ~M[i];
		if (j - i > 1) MarkNum += BitNum(M[i], 16);
		else MarkNum += BitNum(M[i], db.dbH.nrec & 0x0000000f);
	}
}
//===========================================================================

void LOOK::MarkFindFirst(void)
{
	if (!M) return;
	short i;
	curY = 0;
	for (i = 0; i <= botY; i++) if (Marked(recV[i])) { recV[0] = recV[i]; return; }
	DWORD j;
	for (j = 1; j <= db.dbH.nrec; j++) if (Marked(j)) break;
	recV[0] = j;
}
//===========================================================================

void LOOK::MarkNonActual(void)
{
	if (!M) return;
	DWORD rn;
	for (rn = recV[curY]; rn <= db.dbH.nrec; rn++) {
		if (db.Read(rn)) { ShowError(2); break; }
		if (db.Invalid()) MarkSet(rn);
	}
}
//===========================================================================

WORD LOOK::GoNextMark(void)
{
	if (!M) return 0;
	if (curY < botY) {
		WORD Y;
		for (Y = curY + 1; Y <= botY; Y++) {
			if (Marked(recV[Y])) { ClearCur(); curY = Y; return 1; }
		}
	}
	DWORD i;
	if (recV[botY] == db.dbH.nrec) return 0;
	for (i = recV[botY] + 1; i <= db.dbH.nrec; i++) if (Marked(i)) break;
	if (i > db.dbH.nrec) return 0;
	ClearCur(); recV[0] = i; curY = 0;
	return 2;
}
//===========================================================================

WORD LOOK::GoPrevMark(void)
{
	if (!M) return 0;
	if (curY) {
		int Y;
		for (Y = curY - 1; Y >= 0; Y--) {
			if (Marked(recV[Y])) { ClearCur(); curY = Y; return 1; }
		}
	}
	if (recV[0] < 2) return 0;
	DWORD i;
	for (i = recV[0] - 1; i; i--) if (Marked(i)) break;
	if (!i) return 0;
	ClearCur(); recV[0] = i; curY = 0;
	return 2;
}
//===========================================================================

char LOOK::CurType(void)
{
	db.FiNum(coCurr->finum);
	return db.cf->type;
}
//===========================================================================

void LOOK::ShowSum(void)
{
	if (!MarkNum) { ShowError(5); return; }// There are no selected records
	int i, j, n, k, dec;
	Column *c;
	for (n = k = 0, c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
		db.FiNum(c->finum); if (!db.Numeric()) continue;
		i = lstrlen(c->name); if (i > k)k = i;  n++;
	}
	if (!n) { ShowError(6); return; }// There are no visible numeric fields
	DWORD rn;
	dbVal *V = new dbVal[n]; if (!V) { ShowError(4); return; }
	FarMenuItem *fm = new FarMenuItem[n]; if (!fm) { delete[] V; ShowError(4); return; }
	for (rn = 1; rn <= db.dbH.nrec; rn++) {
		if (!Marked(rn)) continue;
		if (db.Read(rn)) { delete[] fm; delete[] V; ShowError(2); return; }
		for (i = 0, c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			db.FiNum(c->finum); if (db.Numeric()) { db.Accum(V + i); ++i; }
		}
	}
	fm[0].Selected = 1;
	for (i = 0, c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
		db.FiNum(c->finum); if (!db.Numeric()) continue;
		lstrcpy(fm[i].Text, c->name);
		for (j = 0; j < k; j++)if (!fm[i].Text[j])fm[i].Text[j] = ' ';
		switch (db.cf->type) {
		case 'I': // Integer
			i64_a(fm[i].Text + k, V[i].I, 21, 0);
			break;
		case 'B': // Double
			dec = db.cf->dec; if (!dec)dec = 14;
			FSF.sprintf(fm[i].Text + k, "%21.*g", dec, V[i].D);
			break;
		case 'Y': // Currency
			i64_a(fm[i].Text + k, V[i].I, 21, 4);
			break;
		case 'N': // Number
		case 'F': // Float
			if (db.cf->filen < 21) i64_a(fm[i].Text + k, V[i].I, 21, db.cf->dec);
			else FSF.sprintf(fm[i].Text + k, "%21.*lf", db.cf->dec, V[i].D);
		}
		i++;
	}
	Info.Menu(Info.ModuleNumber, -1, -1, sh - 6, FMENU_WRAPMODE, GetMsg(mSum),
			  NULL, "Functions", NULL, NULL, fm, n);
	delete[] fm; delete[] V;
}
//===========================================================================

int LOOK::ShowFields(void)
{
	int i, h;
	char *s, nafi[16];
	bool emp;
	FarList fl;
	FarDialogItem di[11];
	ZeroMemory(di, sizeof(di));
	emp = false;
	switch (db.dbH.type) {
	case 0x03: case 0x83: s = "dBase III+"; break;
	case 0x8b: s = "dBase IV"; break;
	case 0xf5: case 0xfb: case 0x30: case 0x43: s = "FoxPro"; break;
	case 0x31: case 0x32: s = "FoxPro(ai)"; break;
	default: s = "<Unknown>";
	}
	i = 0;
	di[i].Type = DI_DOUBLEBOX; di[i].X1 = 3; di[i].X2 = 40; di[i].Y1 = 1; di[i].Y2 = 10;
	if (db.dbH.nrec) lstrcpy(di[i].Data, GetMsg(mFileInfo));
	else {
		lstrcpy(di[i].Data, GetMsg(mEmpty));
		if (!LookOnly)emp = true;
	}
	++i; //1
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-16s %10s  %02Xh", GetMsg(mFileType), s, db.dbH.type);
	++i; //2
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-22s%2u/%02u/%04u", GetMsg(mLastUpdate),
				db.dbH.upd[2], db.dbH.upd[1], db.dbH.upd[0] + 1900u);
	++i; //3
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-24s   %5s", GetMsg(mIndexFile),
				db.dbH.ind ? GetMsg(mYes) : GetMsg(mNo));
	++i; //4
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-24s   %5u", GetMsg(mHeadLen), db.dbH.start);
	++i; //5
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-24s   %5u", GetMsg(mRecLen), db.dbH.reclen);
	++i; //6
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = i + 1;
	FSF.sprintf(di[i].Data, "%-24s   %5u", GetMsg(mNumField), db.nfil);
	++i; //7
	di[i].Type = DI_TEXT; di[i].Y1 = i + 1; di[i].Flags = DIF_SEPARATOR;
	++i; //8
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = i + 1;
	lstrcpy(di[i].Data, GetMsg(mFieldHead));
	++i; //9
	if (emp)h = sh - 17; else h = sh - 15;
	if (h > db.nfil)h = db.nfil;
	di[i].Type = DI_LISTBOX; di[i].X1 = 3; di[i].Y1 = i + 1; di[i].X2 = 40; di[i].Y2 = 11 + h;
	di[i].Focus = true; di[i].ListItems = &fl; di[i].DefaultButton = true;
	fl.ItemsNumber = db.nfil; fl.Items = new FarListItem[db.nfil];
	if (!fl.Items) { ShowError(4); return -1; }
	fl.Items[0].Flags = LIF_SELECTED;
	for (i = 0; i < db.nfil; i++) {
		db.FiNum(i);
		if (Yes(WinCode)) ToOem(db.cf->name, nafi);
		else lstrcpy(nafi, db.cf->name);
		FSF.sprintf(fl.Items[i].Text, "%-10s  ", nafi);
		db.FiType(fl.Items[i].Text + 12);
	}
	if (emp) {
		di[0].Y2 = 13 + h;
		i = 10; di[i].Type = DI_BUTTON; di[i].Flags = DIF_CENTERGROUP;
		di[i].Y1 = 12 + h; lstrcpy(di[i].Data, GetMsg(mFieldEmpty));
		i = Info.Dialog(Info.ModuleNumber, -1, -1, 44, 15 + h, "Functions", di, 11);
	}
	else i = Info.Dialog(Info.ModuleNumber, -1, -1, 44, 13 + h, "Functions", di, 10);
	delete[] fl.Items;
	if (i == 9) return di[9].ListPos;
	if (i == 10) return -2;
	return -1;
}
//===========================================================================

int LOOK::TableSet(int nt)
{
	if (nt == ctsNum) return Yes(WinCode);
	if (!nt) {
		if (ChaTa) delete ChaTa;
		ChaTa = NULL; ctsNum = 0;
		aw = " Win";
		return 0;
	}
	if (!ChaTa) {
		ChaTa = new CharTableSet;
		if (!ChaTa) { ShowError(4); ctsNum = 0; aw = " Win"; return 0; }
	}
	ctsNum = nt;
	aw = " Alt";
	if (Info.CharTable(nt - 1, (char*) ChaTa, sizeof(CharTableSet)) < 0) {
		delete ChaTa; ChaTa = NULL;
		ctsNum = 0; aw = " Win";
	}
	return 0;
}
//===========================================================================

int LOOK::TableSelect(void)
{
	int i, n;
	CharTableSet cts;
	n = 1;
	while (Info.CharTable(n - 1, (char*) (&cts), sizeof(CharTableSet)) >= 0)n++;
	FarMenuItem *fm = new FarMenuItem[n];
	if (!fm) { ShowError(4); return -1; }
	lstrcpy(fm[0].Text, "Current Windows Code Table");
	if (!ctsNum) fm[0].Selected = LIF_SELECTED;
	for (i = 1; i < n; i++) {
		Info.CharTable(i - 1, (char*) (&cts), sizeof(CharTableSet));
		lstrcpyn(fm[i].Text, cts.TableName, 128);
		if (i == ctsNum) fm[i].Selected = LIF_SELECTED;
	}
	i = Info.Menu(Info.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE,
				  GetMsg(mCfgCodeTable), NULL, "Functions", NULL, NULL, fm, n);
	delete[] fm;
	if (i < 0) return 1;
	if (TableSet(i)) return 1;
	Clear(WinCode); ChangeCode();
	return 0;
}
//===========================================================================

void LOOK::EditHeader(void)
{
	short i, j, k;
	FarDialogItem di[10];
	ZeroMemory(di, sizeof(di));
	i = 0;
	di[i].Type = DI_DOUBLEBOX; di[i].X1 = 3; di[i].X2 = 40; di[i].Y1 = 1; di[i].Y2 = 7;
	lstrcpy(di[i].Data, GetMsg(mEditTitle));
	++i; //1
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = 2;
	lstrcpy(di[i].Data, GetMsg(mFileType));
	++i; //2
	di[i].Type = DI_FIXEDIT; di[i].X1 = 36; di[i].X2 = 37; di[i].Y1 = 2;
	FSF.sprintf(di[i].Data, "%02X", db.dbH.type); di[i].Focus = true;
	++i; //3
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = 3;
	lstrcpy(di[i].Data, GetMsg(mLastUpdate));
	++i; //4
	di[i].Type = DI_FIXEDIT; di[i].X1 = 28; di[i].X2 = 37; di[i].Y1 = 3;
	FSF.sprintf(di[i].Data, "%02u/%02u/%04u", db.dbH.upd[2], db.dbH.upd[1], db.dbH.upd[0] + 1900u);
	di[i].Flags = DIF_MASKEDIT; di[i].Mask = D_Mask;
	++i; //5
	di[i].Type = DI_TEXT; di[i].X1 = 6; di[i].Y1 = 4;
	lstrcpy(di[i].Data, GetMsg(mIndexFile));
	++i; //6
	di[i].Type = DI_FIXEDIT; di[i].X1 = 36; di[i].X2 = 37; di[i].Y1 = 4;
	FSF.sprintf(di[i].Data, "%02X", db.dbH.ind);
	++i; //7
	di[i].Type = DI_TEXT; di[i].Y1 = 5; di[i].Flags = DIF_SEPARATOR;
	++i; //8
	di[i].Type = DI_BUTTON; di[i].Flags = DIF_CENTERGROUP; di[i].Y1 = 6;
	di[i].DefaultButton = 1; lstrcpy(di[i].Data, GetMsg(mButSave));
	++i; //9
	di[i].Type = DI_BUTTON; di[i].Flags = DIF_CENTERGROUP; di[i].Y1 = 6;
	lstrcpy(di[i].Data, GetMsg(mButCancel));
	++i; //10
	if (Info.Dialog(Info.ModuleNumber, -1, -1, 44, 9, "Contents", di, i) != 8) return;
	di[2].Data[2] = 0; i = ah_i64(di[2].Data, 300);
	if (i >= 0 && i < 256) { db.dbH.type = i; db.upd = 1; }
	di[6].Data[2] = 0; i = ah_i64(di[6].Data, 300);
	if (i >= 0 && i < 256) { db.dbH.ind = i;  db.upd = 1; }
	di[4].Data[2] = 0; i = a_i64(di[4].Data, 300);
	if (i < 0 || i>255) goto FINISH;
	di[4].Data[5] = 0; j = a_i64(di[4].Data + 3, 300);
	if (j < 0 || j>255) goto FINISH;
	di[4].Data[10] = 0; k = a_i64(di[4].Data + 6, -1);
	if (k < 1900) goto FINISH;
	k -= 1900; db.upd = 1;
	db.dbH.upd[0] = k;
	db.dbH.upd[1] = j;
	db.dbH.upd[2] = i;
FINISH:
	db.SaveHeader();
}
//===========================================================================

static LONG_PTR WINAPI DiField(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_CTLCOLORDLGITEM) {
		Param2 = data->at[16]; Param2 = Param2 << 16;
		if (data->Marked(data->recV[data->curY])) Param2 |= data->at[18];
		else Param2 |= data->at[17];
		return Param2;
	}
	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}
//===========================================================================

WORD LOOK::EditField(BYTE nll)
{
	FarDialogItem di[1];
	ZeroMemory(di, sizeof(di));
	if (db.Read(recV[curY])) { ShowError(2); return 1; }
	db.FiNum(coCurr->finum);
	if (nll) {
		if (!(db.cf->mskN))return 1;
		db.SetNull();
		goto REFRESH;
	}
	if (db.cf->type == 'G' || db.cf->type == 'P' ||
		db.cf->type == 'M' || db.cf->type == '0') return 1;
	db.fmtD = fmtD; db.fmtT = fmtT;
	db.FiDispE(di[0].Data);
	if (Yes(WinCode) && db.FiChar())ToOem(di[0].Data);
	switch (db.cf->type) {
	case 'T': di[0].Type = DI_FIXEDIT; di[0].Mask = T_Mask;
		di[0].Flags = DIF_MASKEDIT; break;
	case 'D': di[0].Type = DI_FIXEDIT; di[0].Mask = D_Mask;
		di[0].Flags = DIF_MASKEDIT; break;
	default:  di[0].Type = DI_EDIT; break;
	}
	di[0].X1 = 0;  di[0].X2 = coCurr->wid - 2;
	di[0].Y1 = 0;    di[0].Focus = true; di[0].DefaultButton = 1;
	if (Info.DialogEx(Info.ModuleNumber, Xcur, curY + 2, Xcur + coCurr->wid - 2,
					  curY + 2, "Edit", di, 1, 0, FDLG_SMALLDIALOG | FDLG_NODRAWPANEL,
					  DiField, 0) < 0) {
		db.fmtD = fmtDV; db.fmtT = fmtTV; return 1;
	}
	if (Yes(WinCode) && db.FiChar())ToAlt(di[0].Data);
	db.SetField(di[0].Data);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
REFRESH:
	if (db.ReWrite()) { ShowError(2); return 1; }
	return 0;
}
//===========================================================================

void LOOK::GetScreenSize(void)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	sw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	sh = csbi.srWindow.Bottom - csbi.srWindow.Top;
}
//===========================================================================

void LOOK::DefColors(char *u)
{
	char as[4], k;
	int i, j;
	for (i = 0; i < 19; i++) {
		k = i + 'a';
		for (j = 0; u[j]; j++) if (u[j] == k) break;
		if (u[j]) {
			as[0] = u[++j]; as[1] = u[++j]; as[2] = 0;
			at[i] = (BYTE) ah_i64(as, atd[i]);
		}
		else at[i] = atd[i];
	}
}
//===========================================================================

void LOOK::ClearRect(WORD left, WORD top, WORD width, WORD height)
{
	int x, y;
	CHAR_INFO *v;
	for (y = 0; y < height; y++) {
		v = VBuf + (sw) *(top + y) + left;
		for (x = 0; x < width; x++)v[x].Char.AsciiChar = ' ';
	}
}
//===========================================================================

void LOOK::ShowStr(const char *str, WORD x, WORD y, BYTE atn, WORD L)
{
	if (x >= sw) return;
	int l = lstrlen(str); if (!L) L = l; if (L + x > sw) L = sw - x;
	CHAR_INFO *v = VBuf + sw * y + x;
	for (int i = 0; i < L; i++) {
		v[i].Char.AsciiChar = i < l ? str[i] : ' ';
		if (atn) v[i].Attributes = at[atn];
	}
}
//===========================================================================

void LOOK::ShowStrI(const char *str, WORD x, WORD y, WORD L)
{
	if (x >= sw) return;
	bool sep = true;
	int l = lstrlen(str); if (L + x > sw) { L = sw - x; sep = false; }
	CHAR_INFO *v = VBuf + sw * y + x;
	for (int i = 0; i < L; i++) v[i].Char.AsciiChar = i < l ? str[i] : ' ';
	if (sep) v[L - 1].Char.AsciiChar = DefExpSep;
}
//===========================================================================

void LOOK::ShowChar(char c, WORD x, WORD y, BYTE atn)
{
	CHAR_INFO *v = VBuf + (sw) *y + x;
	v->Char.AsciiChar = c; if (atn)v->Attributes = at[atn];
}
//===========================================================================

void LOOK::AttrRect(WORD left, WORD top, WORD width, WORD height, WORD Attr)
{
	if (left >= sw) return;
	if (left + width > sw)width = sw - left;
	for (int y = 0; y < height; y++) {
		CHAR_INFO *v = VBuf + sw * (top + y) + left;
		for (int x = 0; x < width; x++) v[x].Attributes = Attr;
	}
}
//===========================================================================

void LOOK::AttrLine(WORD left, WORD width, WORD Y, WORD Attr)
{
	if (left >= sw) return;
	if (left + width > sw)width = sw - left;
	CHAR_INFO *v = VBuf + sw * Y + left;
	for (int x = 0; x < width; x++) v[x].Attributes = Attr;
}
//===========================================================================

void LOOK::ClearCur(void)
{
	ShowChar(' ', 0, 0);
	WORD a = Marked(recV[curY]) ? at[6] : at[3];
	AttrLine(0, sw, curY + 2, a);
}
//===========================================================================

void LOOK::ShowCur(void)
{
	WORD a_cur, a_fil, w;
	if (Marked(recV[curY])) { a_cur = at[7]; a_fil = at[8]; }
	else { a_cur = at[4]; a_fil = at[5]; }
	AttrLine(0, sw, curY + 2, a_cur);
	db.FiNum(coCurr->finum); a_cur = db.FiWidth();
	w = coCurr->wid - 1; if (w > a_cur) w = a_cur;
	AttrLine(Xcur, w, curY + 2, a_fil);
	ShowStatus(1); ShowStatus(2);
}
//===========================================================================

void LOOK::ShowPage(void)
{
	char s[512];
	WORD i, j, x;
	DWORD rn;
	Column *c;
	ClearRect(0, 1, sw, sh - 1); AttrRect(0, 2, sw, sh - 2, at[3]);
	//---------------- DB fields names showing
	if (Yes(LineNums)) {
		x = Wrec + 2; for (i = 0; i < Wrec; i++)s[i] = '*';
		s[Wrec] = '*'; s[Wrec + 1] = 0; ShowStrI(s, 0, 1, x);
	}
	else { ShowStrI("*", 0, 1, 2); x = 2; }
	for (i = 0, c = coFirst; x < sw && c; c = (Column *) c->Next()) {
		if (Yes(WinCode)) {
			ToOem(c->name, s);
			ShowStrI(s, x + 1, 1, c->wid - 1);
		}
		else ShowStrI(c->name, x + 1, 1, c->wid - 1);
		if (c->pos) ShowChar(0x1b, x, 1);
		db.FiNum(c->finum);
		if (c->wid - 1 < db.FiWidth() - c->pos) ShowChar(0x1a, x + c->wid - 2, 1);
		if (i == curX) { coCurr = c; Xcur = x; }
		++i; x += c->wid; coLast = c;
		coTail = x - sw; if (coTail < 0)coTail = 0;
	}
	rn = recV[0]; db.Read(recV[0]);
	for (j = 2; j < sh; j++) {
		if (!db.Invalid())db.rec[0] = ' ';
		if (Yes(LineNums)) { x = Wrec + 2; FSF.sprintf(s, "%*ld%c", Wrec, rn, db.rec[0]); }
		else { s[0] = db.rec[0]; s[1] = 0; x = 2; }
		ShowStrI(s, 0, j, x);
		for (c = coFirst; x < sw && c; c = (Column *) c->Next()) {
			db.FiNum(c->finum); s[0] = 0; db.FiDisp(s);
			if (Yes(WinCode) && db.FiChar())ToOem(s);
			ShowStrI(s + c->pos, x, j, c->wid);
			x += c->wid;
		}
		botY = j - 2; recV[botY] = rn;
		if (MarkOnly) {
			AttrLine(0, sw, j, at[6]); ++rn;
			for (; rn <= db.dbH.nrec; rn++)if (Marked(rn)) break;
			if (rn > db.dbH.nrec) break;
			if (db.Read(rn)) break;
		}
		else {
			if (Marked(rn))AttrLine(0, sw, j, at[6]);
			++rn; if (db.NextRec())break;
		}
	}
	if (curY > botY)curY = botY;
	ShowCur();
}
//===========================================================================

static LONG_PTR WINAPI DiExp(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	short i;
	FarDialogItem *di = (FarDialogItem *) Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	switch (Msg) {
	case DN_INITDIALOG:
		Info.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
		return 1;
	case DN_LISTCHANGE:
		switch (Param1) {
		case 4:
			lstrcpy(di[11].Data, GetMsg((Param2 == 2) ? mFileType : mExSep));
			//i=lstrlen(di[11].Data); di[11].X1=di[0].X2-i-8;
			switch (Param2) {
			case 0: di[10].Flags = di[11].Flags = di[8].Flags = 0;
				di[11].Selected = data->Yes(ExpSeparator);
				di[12].Data[0] = data->Exp.Sep[0];
				di[12].Data[1] = data->Exp.Sep[1];
				break;
			case 1: di[10].Flags = di[11].Flags = di[12].Flags = DIF_DISABLE;
				di[8].Flags = 0; break;
			case 2: di[10].Flags = di[8].Flags = DIF_DISABLE;
				di[11].Flags = di[12].Flags = 0;
				di[11].Selected = data->Yes(ExpFileType);
				di[12].Data[0] = data->Exp.Type[0];
				di[12].Data[1] = data->Exp.Type[1];
			}
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 8, (LONG_PTR) (di + 8));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 10, (LONG_PTR) (di + 10));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 11, (LONG_PTR) (di + 11));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 12, (LONG_PTR) (di + 12));
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		case 19: case 22: case 25:
			i = Param1 - 1;
			data->FieldItemType(Param2 - 1, di[i].Data);
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, i, (LONG_PTR) (di + i));
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		}
		return 1;
	}
	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}
//===========================================================================

WORD LOOK::Export(void)
{
	short i, j, k, n, ColQty;
	Column *c;
	BYTE idOk, idEmp, idType, idCode, idHead, idSpace, idSep, idSepV, idFile, idActual;
	BYTE idFD, idFT, idBuff, idSort[3];
	FarList flt, flc, lf1, lf2, lf3, ld1, ld2, ld3;
	idSort[0] = idSort[1] = idSort[2] = 0;
	ColQty = 0;
	for (c = (Column*) coFirst->Head(); c; c = (Column*) c->Next()) {
		db.FiNum(c->finum);
		if (db.cf->type == 'M' || db.cf->type == '0') continue;
		if (db.cf->type == 'P' || db.cf->type == 'G') continue;
		ColQty++;
	}
	j = ColQty; if (j > 3)j = 3;  ++ColQty;
	FarDialogItem *di = new FarDialogItem[23 + j * 3];
	flt.Items = new FarListItem[6 + (ColQty + 2)*j];
	if (!flt.Items) { delete[] di; ShowError(4); return 1; }
	flt.ItemsNumber = 3;  flt.Items[Exp.Form].Flags = LIF_SELECTED;
	lstrcpy(flt.Items[0].Text, ".txt");
	lstrcpy(flt.Items[1].Text, ".htm");
	lstrcpy(flt.Items[2].Text, ".dbf");
	flc.Items = flt.Items + 3;
	flc.ItemsNumber = 3; flc.Items[Exp.CoType].Flags = LIF_SELECTED;
	lstrcpy(flc.Items[0].Text, GetMsg(mExOriginal));
	lstrcpy(flc.Items[1].Text, "<<DOS>>");
	lstrcpy(flc.Items[2].Text, "<<");
	lstrcat(flc.Items[2].Text, aw + 1);
	lstrcat(flc.Items[2].Text, ">>");
	ld1.Items = flc.Items + 3;
	ld1.ItemsNumber = 2; ld1.Items[Exp.dir[0]].Flags = LIF_SELECTED;
	lstrcpy(ld1.Items[0].Text, C_OPER[5]);
	lstrcpy(ld1.Items[1].Text, C_OPER[6]);
	lf1.Items = ld1.Items + 2; lf1.ItemsNumber = ColQty;
	if (j > 1) {
		ld2.Items = lf1.Items + ColQty;
		ld2.ItemsNumber = 2; ld2.Items[Exp.dir[1]].Flags = LIF_SELECTED;
		lstrcpy(ld2.Items[0].Text, C_OPER[5]);
		lstrcpy(ld2.Items[1].Text, C_OPER[6]);
		lf2.Items = ld2.Items + 2; lf2.ItemsNumber = ColQty;
		if (j > 2) {
			ld3.Items = lf2.Items + ColQty;
			ld3.ItemsNumber = 2; ld3.Items[Exp.dir[2]].Flags = LIF_SELECTED;
			lstrcpy(ld3.Items[0].Text, C_OPER[5]);
			lstrcpy(ld3.Items[1].Text, C_OPER[6]);
			lf3.Items = ld3.Items + 2; lf3.ItemsNumber = ColQty;
		}
	}
	ColQty = j; j = k = n = 1; i = 0;
	lstrcpy(lf1.Items[0].Text, C_OPER[0]);
	if (ColQty > 1) lstrcpy(lf2.Items[0].Text, C_OPER[0]);
	if (ColQty > 2) lstrcpy(lf3.Items[0].Text, C_OPER[0]);
	for (c = (Column*) coFirst->Head(); c; c = (Column*) c->Next()) {
		db.FiNum(c->finum);
		if (db.cf->type == 'M' || db.cf->type == '0') continue;
		if (db.cf->type == 'P' || db.cf->type == 'G') continue;
		i++;
		lstrcpy(lf1.Items[i].Text, c->name);
		if (j && c->finum == Exp.fi[0]) { lf1.Items[i].Flags = LIF_SELECTED; j = 0; }
		if (ColQty < 2) continue;
		lstrcpy(lf2.Items[i].Text, c->name);
		if (k && c->finum == Exp.fi[1]) { lf2.Items[i].Flags = LIF_SELECTED; k = 0; }
		if (ColQty < 3) continue;
		lstrcpy(lf3.Items[i].Text, c->name);
		if (n && c->finum == Exp.fi[2]) { lf3.Items[i].Flags = LIF_SELECTED; n = 0; }
	}
	if (j) lf1.Items[0].Flags = LIF_SELECTED;
	if (k && ColQty>1) lf2.Items[0].Flags = LIF_SELECTED;
	if (n && ColQty>2) lf3.Items[0].Flags = LIF_SELECTED;

	i = 0;
	di[i].Type = DI_DOUBLEBOX; di[i].X1 = 3; di[i].X2 = 57; di[i].Y1 = 1;
	di[i].Y2 = 15;
	lstrcpy(di[i].Data, GetMsg(mExTitle));
	++i;  //1
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 2;
	lstrcpy(di[i].Data, GetMsg(mExName));
	++i;  //2
	di[i].Type = DI_TEXT; di[i].X1 = 39; di[i].Y1 = 2;
	lstrcpy(di[i].Data, GetMsg(mExFormat));
	++i;  //3
	di[i].Type = DI_EDIT; di[i].X1 = 5; di[i].X2 = 36; di[i].Y1 = 3;
	di[i].Focus = true; di[i].History = "LookDBFfiles"; di[i].Flags = DIF_HISTORY;
	lstrcpy(di[i].Data, Exp.File);
	idFile = i++; //4
	di[i].Type = DI_COMBOBOX; di[i].X1 = 39; di[i].Y1 = 3; di[i].X2 = 44;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &flt;
	idType = i++; //5
	di[i].Type = DI_TEXT; di[i].Y1 = 4; di[i].Flags = DIF_SEPARATOR;
	++i;        //6
	di[i].Type = DI_TEXT; di[i].X1 = 47; di[i].Y1 = 2;
	lstrcpy(di[i].Data, GetMsg(mExCoding));
	++i;        //7
	di[i].Type = DI_COMBOBOX; di[i].Y1 = 3; di[i].X1 = 47; di[i].X2 = 55;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &flc;
	idCode = i++; //8
	di[i].Type = DI_CHECKBOX; di[i].Y1 = 5; di[i].X1 = 5;
	lstrcpy(di[i].Data, GetMsg(mExHead));  di[i].Selected = Yes(ExpHeads);
	if (Exp.Form == 2)di[i].Flags = DIF_DISABLE;
	idHead = i++; //9
	di[i].Type = DI_CHECKBOX; di[i].Y1 = 6; di[i].X1 = 5;
	lstrcpy(di[i].Data, GetMsg(mExActual));
	di[i].Selected = Yes(ExpActual);
	idActual = i++; //10
	di[i].Type = DI_CHECKBOX; di[i].Y1 = 7; di[i].X1 = 5;
	lstrcpy(di[i].Data, GetMsg(mExSpace));
	di[i].Selected = Yes(ExpSpaces);
	if (Exp.Form != 0)di[i].Flags = DIF_DISABLE;
	idSpace = i++;  //11
	di[i].Type = DI_CHECKBOX; di[i].Y1 = 8;
	lstrcpy(di[i].Data, GetMsg((Exp.Form == 2) ? mFileType : mExSep));
	j = lstrlen(di[i].Data); di[i].X1 = 5; j = di[i].X1 + j + 5;
	if (Exp.Form == 1)di[i].Flags = DIF_DISABLE;
	di[i].Selected = (Exp.Form == 2) ? Yes(ExpFileType) : Yes(ExpSeparator);
	idSep = i++;   //12
	di[i].Type = DI_FIXEDIT; di[i].X1 = j; di[i].X2 = j + 1; di[i].Y1 = 8;
	di[i].Data[0] = Exp.Sep[0]; di[i].Data[1] = Exp.Sep[1]; di[i].Data[2] = 0;
	switch (Exp.Form) {
	case 1: di[i].Flags = DIF_DISABLE; break;
	case 2: di[i].Data[0] = Exp.Type[0]; di[i].Data[1] = Exp.Type[1]; di[i].Data[2] = 0;
	}
	idSepV = i++; //13
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 9;
	lstrcpy(di[i].Data, GetMsg(mExFormat)); lstrcat(di[i].Data, " D (Date)");
	di[i].Flags = DIF_DISABLE;
	++i;       //14
	di[i].Type = DI_EDIT; di[i].X1 = 5; di[i].X2 = 30; di[i].Y1 = 10;
	DTf2s(fmtDE, di[i].Data);       // формат в строку формата
	idFD = i++;  //15
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 11;
	lstrcpy(di[i].Data, GetMsg(mExFormat)); lstrcat(di[i].Data, " T (DateTime)");
	di[i].Flags = DIF_DISABLE;
	++i;       //16
	di[i].Type = DI_EDIT; di[i].X1 = 5; di[i].X2 = 30; di[i].Y1 = 12;
	DTf2s(fmtTE, di[i].Data);       // формат в строку формата
	idFT = i++;  //17
	di[i].Type = DI_SINGLEBOX; di[i].Y1 = 5; di[i].Y2 = 12; di[i].X1 = di[i].X2 = 32;
	++i;       //18
	di[i].Type = DI_TEXT; di[i].X1 = 34; di[i].Y1 = 5;
	FieldItemType(Exp.fi[0] - 1, di[i].Data); di[i].Flags = DIF_DISABLE;
	++i;       //19
	di[i].Type = DI_COMBOBOX; di[i].X1 = 34; di[i].Y1 = 6; di[i].X2 = di[i].X1 + 13;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lf1;
	idSort[0] = i++;       //20
	di[i].Type = DI_COMBOBOX; di[i].X1 = 50; di[i].Y1 = 6; di[i].X2 = di[i].X1 + 5;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &ld1;
	if (ColQty > 1) {
		++i;     //21
		di[i].Type = DI_TEXT; di[i].X1 = 34; di[i].Y1 = 7;
		FieldItemType(Exp.fi[1] - 1, di[i].Data); di[i].Flags = DIF_DISABLE;
		++i;     //22
		di[i].Type = DI_COMBOBOX; di[i].X1 = 34; di[i].Y1 = 8; di[i].X2 = di[i].X1 + 13;
		di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lf2;
		idSort[1] = i++;     //23
		di[i].Type = DI_COMBOBOX; di[i].X1 = 50; di[i].Y1 = 8; di[i].X2 = di[i].X1 + 5;
		di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &ld2;
		if (ColQty > 2) {
			++i;   //24
			di[i].Type = DI_TEXT; di[i].X1 = 34; di[i].Y1 = 9;
			FieldItemType(Exp.fi[2] - 1, di[i].Data); di[i].Flags = DIF_DISABLE;
			++i;   //25
			di[i].Type = DI_COMBOBOX; di[i].X1 = 34; di[i].Y1 = 10; di[i].X2 = di[i].X1 + 13;
			di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lf3;
			idSort[2] = i++;   //26
			di[i].Type = DI_COMBOBOX; di[i].X1 = 50; di[i].Y1 = 10; di[i].X2 = di[i].X1 + 5;
			di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &ld3;
		}
	}
	++i;       //21 or 24 or 27
	di[i].Type = DI_TEXT; di[i].Y1 = 12; di[i].X1 = 34;
	lstrcpy(di[i].Data, GetMsg(mExBuff));
	++i;
	di[i].Type = DI_EDIT; di[i].Y1 = 12; di[i].X1 = 51; di[i].X2 = di[i].X1 + 4;
	i64_a(di[i].Data, Exp.BufLim); idBuff = i;
	++i;
	di[i].Type = DI_TEXT; di[i].Y1 = 13; di[i].Flags = DIF_SEPARATOR;
	++i;
	di[i].Type = DI_BUTTON; di[i].Flags = DIF_CENTERGROUP; di[i].Y1 = 14;
	di[i].DefaultButton = 1; lstrcpy(di[i].Data, GetMsg(mButSave));
	idOk = i; ++i;
	di[i].Type = DI_BUTTON; di[i].Flags = DIF_CENTERGROUP; di[i].Y1 = 14;
	lstrcpy(di[i].Data, GetMsg(mExpEmpty));
	idEmp = i; ++i;
	j = Info.DialogEx(Info.ModuleNumber, -1, -1, 61, 17,
					  "Export", di, i, 0, 0, DiExp, (LONG_PTR) di);
	if (!di[idFile].Data[0]) { delete[] flt.Items; delete[] di; return 1; }
	if (j == idEmp) { Set(ExpEmpty); j = idOk; }
	if (j != idOk) { delete[] flt.Items; delete[] di; return 1; }
	lstrcpy(Exp.File, di[idFile].Data);
	DTs2f(di[idFD].Data, fmtDE); DTs2f(di[idFT].Data, fmtTE);
	Exp.Form = di[idType].ListPos; Exp.CoType = di[idCode].ListPos;
	if (di[idHead].Selected)  Set(ExpHeads);  else Clear(ExpHeads);
	if (di[idSpace].Selected) Set(ExpSpaces); else Clear(ExpSpaces);
	if (di[idActual].Selected)Set(ExpActual); else Clear(ExpActual);
	for (i = 0; i < 3; i++) { Exp.fi[i] = -1; Exp.dir[i] = 0; }
	if (di[idSort[0]].ListPos) {
		Exp.BufLim = a_i64(di[idBuff].Data, 2);
		if (Exp.BufLim < 2) Exp.BufLim = 2;
		c = FieldItem(di[idSort[0]].ListPos - 1); Exp.fi[0] = c->finum;
		Exp.dir[0] = di[idSort[0] + 1].ListPos;
		if (idSort[1] && di[idSort[1]].ListPos) {
			c = FieldItem(di[idSort[1]].ListPos - 1); Exp.fi[1] = c->finum;
			Exp.dir[1] = di[idSort[1] + 1].ListPos;
			if (idSort[2] && di[idSort[2]].ListPos) {
				c = FieldItem(di[idSort[2]].ListPos - 1); Exp.fi[2] = c->finum;
				Exp.dir[2] = di[idSort[2] + 1].ListPos;
			}
		}
	}
	switch (Exp.Form) {
	case 0: if (di[idSep].Selected)Set(ExpSeparator); else Clear(ExpSeparator);
		Exp.Sep[0] = di[idSepV].Data[0]; Exp.Sep[1] = di[idSepV].Data[1];
		break;
	case 2: if (di[idSep].Selected)Set(ExpFileType); else Clear(ExpFileType);
		Exp.Type[0] = di[idSepV].Data[0]; Exp.Type[1] = di[idSepV].Data[1];
	}
	delete[] flt.Items; delete[] di;
	Exp.code = Exp.CoType; Exp.recnum = Exp.count = 0;
	switch (Exp.CoType) {
	case 1: Exp.code = Yes(WinCode) ? 2 : 0; break;
	case 2: Exp.code = Yes(WinCode) ? 0 : 1; break;
	}
	if (MarkNum) {
		Exp.mode = 1; ExpRec = &LOOK::ExpRec1;
		if (Exp.BufLim > MarkNum) Exp.BufLim = MarkNum;
		Indic.Start(GetMsg(mExTitle), MarkNum);
	}
	else {
		Exp.mode = 0; ExpRec = &LOOK::ExpRec0;
		if (Exp.BufLim > db.dbH.nrec) Exp.BufLim = db.dbH.nrec;
		Indic.Start(GetMsg(mExTitle), db.dbH.nrec);
		if (db.Read(1)) { ShowError(2); Exp.mode = 11; return 1; }
		Exp.recnum = 0;
	}
	if (Exp.fi[0] >= 0) {
		if (SortAlloc()) return 1;
		if (MarkNum) SortSetMarked(); else SortSetAll();
		ExpRec = &LOOK::ExpRec2; Exp.recnum = 1;
		while (!Sorted(Exp.recnum) && Exp.recnum <= db.dbH.nrec) Exp.recnum++;
	}
	char a[MAX_PATH], b[MAX_PATH];
	lstrcpy(a, FileName); lstrcpy(b, Exp.File);
	switch (Exp.Form) {
	case 0: lstrcat(b, ".TXT"); break;
	case 1: lstrcat(b, ".HTM"); break;
	case 2: lstrcat(b, ".DBF");
	}
	FSF.LStrupr(a); FSF.LStrupr(b);
	if (lstrcmp(a, b)) return 0;
	ShowError(7);
	return 1;
}
//===========================================================================

WORD LOOK::ExpRec0(void)  // No Sorted. No marked. Export all of records
{
	DWORD step;
	step = 0;
	for (;;) {
		step++; Exp.recnum++;
		if (Exp.recnum > 1) if (db.NextRec()) return 0;
		if (No(ExpActual) || !db.Invalid()) break;
	}
	if (Indic.Move(step)) return 0;
	Exp.count++;
	return 1;
}
//===========================================================================

WORD LOOK::ExpRec1(void) // No Sorted. Export marked records only
{
	DWORD step;
	step = 0;
	for (;;) {
		Exp.recnum++;
		if (Exp.recnum > db.dbH.nrec) return 0;
		if (!Marked(Exp.recnum))continue;
		++step;
		if (db.Read(Exp.recnum)) { ShowError(2); Exp.mode = 11; return 0; }
		if (No(ExpActual) || !db.Invalid()) break;
	}
	if (Indic.Move(step)) return 0;
	Exp.count++;
	return 1;
}
//===========================================================================

WORD LOOK::ExpRec2(void) // Sorted. Marked array is copied to Sort array
{
	short j, k, n, nB;
	DWORD i;
	CondValue vc[3];
	if (Exp.BufLast) {
		++Exp.BufCurr; if (Exp.BufCurr < Exp.BufLast) goto REC_OUT;
		if (Exp.BufLast < Exp.BufLim) { SortFree(); return 0; }
		Exp.BufCurr = Exp.BufLast = 0;
	}
	nB = 0;
	for (i = Exp.recnum; i <= db.dbH.nrec; i++) {
		if (!Sorted(i))continue;
		if (db.Read(i)) { ShowError(2); Exp.mode = 11; SortFree(); return 0; }
		if (Yes(ExpActual) && db.Invalid()) continue;
		if (!nB) {
			Exp.RN[0] = i;
			FieldValue(Exp.fi[0], Exp.V0);
			if (Exp.mode) {
				FieldValue(Exp.fi[1], Exp.V1);
				if (Exp.mode > 1) FieldValue(Exp.fi[2], Exp.V2);
			}
			nB = 1;
			continue;
		}
		FieldValue(Exp.fi[0], vc);
		if (Exp.mode) {
			FieldValue(Exp.fi[1], vc + 1);
			if (Exp.mode > 1) FieldValue(Exp.fi[2], vc + 2);
		}
		for (k = 0; k < nB; k++) {
			j = FieldCompare(Exp.fi[0], Exp.V0 + k, vc);
			if (j == Exp.dir[0]) goto SHIFT_OLD;
			if (j < 2) continue;
			if (!Exp.mode) continue;
			j = FieldCompare(Exp.fi[1], Exp.V1 + k, vc + 1);
			if (j == Exp.dir[1]) goto SHIFT_OLD;
			if (j < 2) continue;
			if (Exp.mode < 2) continue;
			j = FieldCompare(Exp.fi[2], Exp.V2 + k, vc + 2);
			if (j == Exp.dir[2]) goto SHIFT_OLD;
		}
		k = nB;
		if (nB < Exp.BufLim) goto SET_NEW;
		continue;
SHIFT_OLD:
		if (nB == Exp.BufLim)nB--;
		for (n = nB; n > k; n--) {
			Exp.RN[n] = Exp.RN[n - 1];
			Exp.V0[n] = Exp.V0[n - 1];
			if (Exp.mode) {
				Exp.V1[n] = Exp.V1[n - 1];
				if (Exp.mode > 1) Exp.V2[n] = Exp.V2[n - 1];
			}
		}
SET_NEW:
		Exp.RN[k] = i; Exp.V0[k] = vc[0];
		if (Exp.mode) { Exp.V1[k] = vc[1]; if (Exp.mode > 1) Exp.V2[k] = vc[2]; }
		nB++;
	}
	if (!nB) { SortFree(); return 0; }
	Exp.BufLast = nB; Exp.BufCurr = 0;
REC_OUT:
	SortClear(Exp.RN[Exp.BufCurr]);
	if (Exp.recnum == Exp.RN[Exp.BufCurr])
		while (!Sorted(Exp.recnum) && Exp.recnum <= db.dbH.nrec) Exp.recnum++;
	if (db.Read(Exp.RN[Exp.BufCurr])) {
		ShowError(2); Exp.mode = 11; SortFree(); return 0;
	}
	if (Indic.Move(1)) { SortFree(); return 0; }
	Exp.count++;
	return 1;
}
//===========================================================================

void LOOK::FieldValue(WORD fnum, CondValue *val)
{
	BYTE *c, *b;
	short i;
	val->i64 = 0;
	b = val->bt;
	db.FiNum(fnum);
	c = db.rec + db.cf->loc;
	switch (db.cf->type) {
	case 'T': // DateTime
		if (db.cf->filen == 8) { // Binary format
			union { DWORD w; BYTE c[4]; } u;
			for (i = 0; i < 4; i++)u.c[i] = c[i];
			val->dw[0] = u.w;
			for (i = 4; i < 8; i++)u.c[i - 4] = c[i];
			val->dw[1] = u.w;
			break;
		}
		else goto BYTE_COPY;    // Character format yyyymmddhhmmss
	case 'I':
	{// Integer
		union { __int32 w; BYTE c[4]; } u;
		for (i = 0; i < 4; i++)u.c[i] = c[i];
		val->i64 = u.w;
	}
	break;
	case 'B':
	{// Double
		union { double w; BYTE c[8]; } u;
		for (i = 0; i < 8; i++)u.c[i] = c[i];
		val->dbl = u.w;
	}
	break;
	case 'Y':
	{// Currency
		union { __int64 w; BYTE c[8]; } u;
		for (i = 0; i < 8; i++)u.c[i] = c[i];
		val->i64 = u.w;
	}
	break;
	default:
		BYTE_COPY:  CopyMemory(b, c, db.cf->filen);
					b[db.cf->filen] = 0;
	}
	if (db.cf->type == 'N' || db.cf->type == 'F')val->i64 = a_i64((char *) b, 0, db.cf->dec);
}
//===========================================================================

short LOOK::FieldCompare(short fnum, CondValue *v1, CondValue *v2)
{
	short i;
	if (fnum<0) return 2;
	db.FiNum(fnum);
	switch (db.cf->type) {
	case 'T': // DateTime
		if (db.cf->filen == 8) { // Binary format
			if (v1->dw[0] > v2->dw[0]) return 0;
			if (v1->dw[0] < v2->dw[0]) return 1;
			if (v1->dw[1] > v2->dw[1]) return 0;
			if (v1->dw[1] < v2->dw[1]) return 1;
			return 2;
		}
		else goto BYTE_CMP;    // Character format yyyymmddhhmmss
	case 'I': // Integer
	case 'Y': // Currency
	case 'N':  case 'F': // Numeric
		if (v1->i64 > v2->i64) return 0;
		if (v1->i64 < v2->i64) return 1;
		return 2;
	case 'B': // Double
		if (v1->dbl > v2->dbl) return 0;
		if (v1->dbl < v2->dbl) return 1;
		return 2;
BYTE_CMP:
	default:
		for (i = 0; i<db.cf->filen; i++) {
			if (v1->bt[i] > v2->bt[i]) return 0;
			if (v1->bt[i] < v2->bt[i]) return 1;
		}
	}
	return 2;
}
//===========================================================================

WORD LOOK::ExpTxt(void)
{
	char s[384], r[512];
	HANDLE ef;
	Column *c;
	short w, k;
	BYTE sep;

	lstrcpy(s, Exp.File); lstrcat(s, ".txt");
	ef = CreateFile(s, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ef == INVALID_HANDLE_VALUE) { ShowError(21); return 1; }
	sep = Yes(ExpSeparator); s[0] = Exp.Sep[0]; s[1] = Exp.Sep[1]; s[2] = 0;
	db.fmtD = fmtDE; db.fmtT = fmtTE;
	if (sep) {
		if (s[0]) {
			if (s[1]) { sep = ah_i64(s, 0); if (!sep)sep = s[0]; }
			else sep = s[0];
		}
		else sep = 0;
	}
	if (sep == 0xb3 && (Exp.code == 1 || (Exp.code == 0 && Yes(WinCode)))) sep = 0xa6;
	if (Yes(ExpHeads)) {
		for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			lstrcpy(s, c->name); w = c->wid - 1; k = w;
			db.FiNum(c->finum); if (db.cf->type == 'D') k = DTw(db.fmtD);
			if (db.cf->type == 'T') k = DTw(db.fmtT); if (k > w) w = k;
			if (Exp.code == 1)ToAlt(s); else if (Exp.code == 2) ToOem(s);
			if (Yes(ExpSpaces)) { s[w] = 0; FSF.sprintf(r, "%-*s", w, s); }
			else { FSF.Trim(s); FSF.sprintf(r, "%s", s); }
			k = lstrlen(r);
			if (sep && c->Next()) r[k++] = sep;
			if (MyWrite(ef, r, k)) goto BAD_WRITE;
		}
		if (MyWrite(ef, "\r\n", 2)) goto BAD_WRITE;
	}
	if (Yes(ExpEmpty)) goto FINISH;
	while ((this->*ExpRec)()) {
		for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			db.FiNum(c->finum); k = db.FiDisp(s); w = c->wid - 1;
			if (db.FiChar()) {
				if (Exp.code == 1)ToAlt(s); else if (Exp.code == 2) ToOem(s);
			}
			if (Yes(ExpSpaces)) {
				if (db.Numeric()) { k = k - w; if (k<0)k = 0; FSF.sprintf(r, "%*s", w, s + k); }
				else {
					if ((db.cf->type == 'D' || db.cf->type == 'T') && k>w) w = k;
					s[w] = 0; FSF.sprintf(r, "%-*s", w, s);
				}
			}
			else { FSF.Trim(s); FSF.sprintf(r, "%s", s); }
			k = lstrlen(r);
			if (sep && c->Next()) r[k++] = sep;
			if (MyWrite(ef, r, k)) goto BAD_WRITE;
		}
		if (MyWrite(ef, "\r\n", 2)) goto BAD_WRITE;
	}
FINISH:
	CloseHandle(ef);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
	if (Exp.mode > 10) return 1;
	return 0;
BAD_WRITE:
	CloseHandle(ef);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
	ShowError(21); return 1;
}
//===========================================================================

WORD LOOK::ExpHtm(void)
{
	char s[384], r[512];
	HANDLE ef;
	Column *c;
	const char *rs, *rf, *cs, *cf, *fi, *fs, *fsn, *fss;

	lstrcpy(s, Exp.File); lstrcat(s, ".htm");
	ef = CreateFile(s, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ef == INVALID_HANDLE_VALUE) { ShowError(21); return 1; }
	db.fmtD = fmtDE; db.fmtT = fmtTE;
	rs = GetMsg(mTabRowS); rf = GetMsg(mTabRowF);
	cs = GetMsg(mTabCellS); cf = GetMsg(mTabCellF); fi = GetMsg(mTabSpace);
	fs = "%s"; fsn = "%s\r\n"; fss = "%s%s%s";
	FSF.sprintf(r, fsn, GetMsg(mTabS));
	if (MyWrite(ef, r)) goto BAD_WRITE;
	if (Yes(ExpHeads)) {
		FSF.sprintf(r, fs, rs);
		if (MyWrite(ef, r)) goto BAD_WRITE;
		for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			if (!c->name[0]) FSF.sprintf(r, fss, cs, fi, cf);
			else {
				lstrcpy(s, c->name); FSF.Trim(s);
				if (Exp.code == 1)ToAlt(c->name, s);
				else if (Exp.code == 2) ToOem(c->name, s);
				FSF.sprintf(r, fss, cs, s, cf);
			}
			if (MyWrite(ef, r)) goto BAD_WRITE;
		}
		FSF.sprintf(r, fsn, rf);
		if (MyWrite(ef, r)) goto BAD_WRITE;
	}
	if (Yes(ExpEmpty)) goto FINISH;
	while ((this->*ExpRec)()) {
		FSF.sprintf(r, fs, rs);
		if (MyWrite(ef, r)) goto BAD_WRITE;
		for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			db.FiNum(c->finum); db.FiDisp(s); FSF.Trim(s);
			if (!s[0]) FSF.sprintf(r, fss, cs, fi, cf);
			else {
				if (db.FiChar()) {
					if (Exp.code == 1)ToAlt(s);
					else if (Exp.code == 2) ToOem(s);
				}
				FSF.sprintf(r, fss, cs, s, cf);
			}
			if (MyWrite(ef, r)) goto BAD_WRITE;
		}
		FSF.sprintf(r, fsn, rf);
		if (MyWrite(ef, r)) goto BAD_WRITE;
	}
FINISH:
	FSF.sprintf(r, fsn, GetMsg(mTabF));
	if (MyWrite(ef, r)) goto BAD_WRITE;
	CloseHandle(ef);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
	if (Exp.mode > 10) return 1;
	return 0;
BAD_WRITE:
	CloseHandle(ef);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
	ShowError(21); return 1;
}
//===========================================================================

WORD LOOK::ExpDbf(void)
{
	char s[384];
	Column *c;
	dbBase a;
	WORD n;
	BYTE sep, nn;
	sep = db.dbH.type; s[0] = Exp.Type[0]; s[1] = Exp.Type[1]; s[2] = 0;
	if (Yes(ExpFileType) && s[0] && s[1]) sep = ah_i64(s, sep);

	for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
		db.FiNum(c->finum);
		if ((db.cf->spare[0] & 0x05) == 5 && db.cf->type == '0') continue;
		lstrcpy(s, c->name); s[10] = 0;
		if (Exp.code == 1)ToAlt(s); else if (Exp.code == 2) ToOem(s);
		a.AddF(db.cf, s);
	}
	nn = a.AddNull();
	CopyMemory(a.dbH.spare1, db.dbH.spare1, 20);
	lstrcpy(s, Exp.File); lstrcat(s, ".dbf");
	if (a.Create(s, sep)) { ShowError(1); return 1; }
	if (Yes(ExpEmpty)) goto FINISH;
	while ((this->*ExpRec)()) {
		a.rec[0] = db.rec[0];
		if (a.Nflg && nn) ZeroMemory(a.Nflg, nn);
		for (n = 0, c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			db.FiNum(c->finum);
			if ((db.cf->spare[0] & 0x05) == 5 && db.cf->type == '0') continue;
			a.FiNum(n); ++n; db.GetByte((BYTE*) s);
			if (db.FiChar()) {
				if (Exp.code == 1)ToAlt(s);
				else if (Exp.code == 2) ToOem(s);
			}
			a.SetByte((BYTE*) s);
			if (db.FiNull())a.SetNull();
			if (db.FiNotFull())a.SetNotFull();
		}
		if (a.Write()) { ShowError(23); a.Close(); return 1; }
	}
FINISH:
	a.Close();
	if (Exp.mode > 10) return 1;
	return 0;
}
//===========================================================================

char *LOOK::NameScratch(char *nm)
{
	int i;
	//------- Construct Scratch file name
	lstrcpy(nm, FileName); i = lstrlen(nm) - 1;
	while (i) {  // Remove extension, if it exist
		if (nm[i] == '\\') break;
		if (nm[i] == '.') { nm[i] = '_'; break; }
		--i;
	}
	lstrcat(nm, "._$_");
	return nm;
}
//===========================================================================

int LOOK::AskMsg(const TCHAR *s)
{
	TCHAR Msg[256];
	lstrcpyn(Msg, s, 254);
	lstrcat(Msg, TEXT("\n\x01"));
	return Info.Message(Info.ModuleNumber, FMSG_ALLINONE | FMSG_MB_OKCANCEL, TEXT("Functions"), (const TCHAR *const *) Msg, 0, 0);
}
//===========================================================================

void LOOK::OkMsg(const TCHAR *s)
{
	TCHAR Msg[256];
	lstrcpyn(Msg, s, 254);
	lstrcat(Msg, TEXT("\n\x01"));
	Info.Message(Info.ModuleNumber, FMSG_ALLINONE | FMSG_MB_OK, TEXT("Functions"), (const TCHAR *const *) Msg, 0, 0);
}
//===========================================================================

WORD LOOK::PackDbf(void)
{
	char scratch[MAX_PATH];
	lstrcpy(scratch, "\n");
	lstrcat(scratch, GetMsg(mPkTitle));
	if (AskMsg(scratch)) return 1;
	dbBase a;
	for (auto c = db.dbF; c; c = (dbField *) c->Next()) a.AddF(c);
	if (a.Create(NameScratch(scratch), db.dbH.type, &db)) { ShowError(1); return 1; }
	if (db.Read(1)) { ShowError(2); a.Close(); return 1; }
	Indic.Start(GetMsg(mPkTitle), db.dbH.nrec);
	for (;;) {
		DWORD step = 1;
		while (db.Invalid()) { ++step; if (db.NextRec()) goto FINISH; }
		if (Indic.Move(step)) goto ESC_BREAK;
		CopyMemory(a.rec, db.rec, db.dbH.reclen);
		if (a.Write()) { ShowError(23); a.Close(); return 1; }
		if (db.NextRec()) break;
	}
FINISH:
	a.Close(); db.Close();
	DeleteFile(FileName);
	MoveFile(scratch, FileName);
	MarkFree();
	db.Open(FileName, LookOnly);
	db.fmtD = fmtD; db.fmtT = fmtT;
	lstrcpy(scratch, GetMsg(mPkTitle));
	lstrcat(scratch, "\n");
	lstrcat(scratch, GetMsg(mPkGood));
	OkMsg(scratch);
	if (!db.dbH.nrec) {
		if (ShowFields() > -2) return 2;
		if (db.Append()) { ShowError(2); return 2; }
	}
	curY = 0; recV[0] = 1;
	ShowStatus(6);
	ShowF5();
	return 0;
ESC_BREAK:
	a.Close(); DeleteFile(scratch);
	return 1;
}
//===========================================================================

WORD LOOK::Import(void)
{
	char ImpFile[MAX_PATH];
	ImpFile[0] = 0;
	if (!Info.InputBox(GetMsg(mImpTitle), GetMsg(mImpFileName), "LookDBFfiles",
					   ImpFile, ImpFile, MAX_PATH, "Import", FIB_BUTTONS)) return 1;
	WORD i, j, k;
	dbBase a;
	i = a.Open(ImpFile, 1);
	if (i & 0x000f) {
		const TCHAR *Msg[] = {
			Title,
			ImpFile,
			TEXT(""),
			GetMsg(mNoOpen),
			TEXT("\x01")
		};
		Info.Message(Info.ModuleNumber, FMSG_WARNING | FMSG_MB_OK, TEXT("Import"), Msg, ARRAYSIZE(Msg), 0);
		a.Close(); return 1;
	}
	if (!a.dbH.nrec) { a.Close(); return 1; }
	k = 1;
	for (i = 0; i < db.nfil; i++) {
		db.FiNum(i); db.cf->spare[13] = 0;
		for (j = 0; j < a.nfil; j++) {
			a.FiNum(j);
			if (lstrcmp(a.cf->name, db.cf->name)) continue;
			if (a.cf->type != db.cf->type) continue;
			if (a.cf->filen != db.cf->filen) continue;
			if (a.cf->dec != db.cf->dec) continue;
			db.cf->spare[13] = j + 1; k = 0;
		}
	}
	if (k) { a.Close(); return 1; }
	BYTE *ar, *br;
	DWORD rn = db.dbH.nrec;
	db.pos = db.dbH.start + db.dbH.nrec*db.dbH.reclen;
	db.cur = db.dbH.nrec;
	SetFilePointer(db.f, db.pos, NULL, FILE_BEGIN);
	for (;;) {
		db.rec[0] = a.rec[0];
		for (i = 0; i < db.nfil; i++) {
			db.FiNum(i); db.SetEmpty();
			if (!db.cf->spare[13]) continue;
			a.FiNum(db.cf->spare[13] - 1);
			ar = a.rec + a.cf->loc; br = db.rec + db.cf->loc;
			for (k = 0; k < a.cf->filen; k++)br[k] = ar[k];
		}
		db.Write();
		if (a.NextRec()) break;
	}
	a.Close();
	for (i = 0; i<db.nfil; i++) {
		db.FiNum(i); db.cf->spare[13] = 0;
	}
	db.SaveHeader();
	if (MarkMax && db.dbH.nrec>MarkMax) { MarkFree(); MarkAlloc(); }
	ShowStatus(6);
	recV[0] = rn; curY = 0;
	ShowPage();
	return 0;
}
//===========================================================================

Column *LOOK::FieldItem(short it)
{
	short i;
	Column *c;
	c = (Column*) coFirst->Head();
	for (i = 0; c; c = (Column*) c->Next()) {
		db.FiNum(c->finum);
		switch (db.cf->type) {
		case 'M': case '0': case 'P': case 'G': break;
		default: if (i == it) return c; i++;
		}
	}
	return coFirst;
}
//===========================================================================

char *LOOK::FieldItemType(short it, char *buf)
{
	if (it < 0) { lstrcpy(buf, C_OPER[0]); return buf; }
	data->FieldItem(it); db.FiType(buf);
	return buf;
}
//===========================================================================

static LONG_PTR WINAPI DiCond(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	FarDialogItem *di;
	di = (FarDialogItem *) Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	switch (Msg) {
	case DN_INITDIALOG:
		Info.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
		return 1;
	case DN_LISTCHANGE:
		switch (Param1) {
		case 1:
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 3, (LONG_PTR) (di + 3));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 4, (LONG_PTR) (di + 4));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 14, (LONG_PTR) (di + 14));
			data->FieldItemType(Param2, di[14].Data);
			switch (data->db.cf->type) {
			case 'T': di[4].Flags &= ~DIF_HIDDEN; di[4].Mask = T_Mask;
				di[3].Flags |= DIF_HIDDEN;
				break;
			case 'D': di[4].Flags &= ~DIF_HIDDEN; di[4].Mask = D_Mask;
				di[3].Flags |= DIF_HIDDEN;
				break;
			default:  di[3].Flags &= ~DIF_HIDDEN; di[4].Flags |= DIF_HIDDEN;
			}
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 3, (LONG_PTR) (di + 3));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 4, (LONG_PTR) (di + 4));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 14, (LONG_PTR) (di + 14));
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		case 6:
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 8, (LONG_PTR) (di + 8));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 9, (LONG_PTR) (di + 9));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 15, (LONG_PTR) (di + 15));
			data->FieldItemType(Param2, di[15].Data);
			switch (data->db.cf->type) {
			case 'T': di[9].Flags &= ~DIF_HIDDEN; di[9].Mask = T_Mask;
				di[8].Flags |= DIF_HIDDEN;
				break;
			case 'D': di[9].Flags &= ~DIF_HIDDEN; di[9].Mask = D_Mask;
				di[8].Flags |= DIF_HIDDEN;
				break;
			default:  di[8].Flags &= ~DIF_HIDDEN; di[9].Flags |= DIF_HIDDEN;
			}
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 8, (LONG_PTR) (di + 8));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 9, (LONG_PTR) (di + 9));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 15, (LONG_PTR) (di + 15));
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		case 5:
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 6, (LONG_PTR) (di + 6));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 7, (LONG_PTR) (di + 7));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 8, (LONG_PTR) (di + 8));
			Info.SendDlgMessage(hDlg, DM_GETDLGITEM, 9, (LONG_PTR) (di + 9));
			if (Param2) {
				di[6].Flags &= ~DIF_DISABLE; di[7].Flags &= ~DIF_DISABLE;
				di[8].Flags &= ~DIF_DISABLE; di[9].Flags &= ~DIF_DISABLE;
			}
			else {
				di[6].Flags |= DIF_DISABLE; di[7].Flags |= DIF_DISABLE;
				di[8].Flags |= DIF_DISABLE; di[9].Flags |= DIF_DISABLE;
			}
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 6, (LONG_PTR) (di + 6));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 7, (LONG_PTR) (di + 7));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 8, (LONG_PTR) (di + 8));
			Info.SendDlgMessage(hDlg, DM_SETDLGITEM, 9, (LONG_PTR) (di + 9));
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		}
	}
	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}
//===========================================================================

short LOOK::CondAsk(void)
{
	short i, j, k;
	Column *c;
	FarList lf1, lf2, lo1, lo2, op;
	FarDialogItem di[16];
	ZeroMemory(di, sizeof(di));
	c = (Column*) coFirst->Head();
	for (j = 0; c; c = (Column*) c->Next()) {
		db.FiNum(c->finum);
		switch (db.cf->type) {
		case 'M': case '0': case 'P': case 'G': break;
		default: j++;
		}
	}
	op.ItemsNumber = 5;
	op.Items = new FarListItem[17 + j * 2];
	if (!op.Items) { ShowError(4); return 0; }
	op.Items[Cond.Oper].Flags = LIF_SELECTED;
	for (i = 0; i < 5; i++)lstrcpy(op.Items[i].Text, C_OPER[i]);
	lo1.ItemsNumber = lo2.ItemsNumber = 6;
	lo1.Items = op.Items + 5; lo2.Items = op.Items + 11;
	for (i = 0; i < 6; i++) {
		lstrcpy(lo1.Items[i].Text, C_REL[i]);
		lstrcpy(lo2.Items[i].Text, C_REL[i]);
	}
	lo1.Items[Cond.Rel[0]].Flags = LIF_SELECTED;
	lo2.Items[Cond.Rel[1]].Flags = LIF_SELECTED;
	lf1.ItemsNumber = lf2.ItemsNumber = j;
	lf1.Items = op.Items + 17; lf2.Items = lf1.Items + j;
	c = (Column*) coFirst->Head();
	j = k = 1;
	for (i = 0; c; c = (Column*) c->Next()) {
		db.FiNum(c->finum);
		switch (db.cf->type) {
		case 'M': case '0': case 'P': case 'G': break;
		default:
			lstrcpy(lf1.Items[i].Text, c->name);
			if (j && c->finum == Cond.Field[0]) { lf1.Items[i].Flags = LIF_SELECTED; j = 0; }
			lstrcpy(lf2.Items[i].Text, c->name);
			if (k && c->finum == Cond.Field[1]) { lf2.Items[i].Flags = LIF_SELECTED; k = 0; }
			i++;
		}
	}
	if (j) lf1.Items[0].Flags = LIF_SELECTED;
	if (k) lf2.Items[0].Flags = LIF_SELECTED;

	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 3; di[0].X2 = 53; di[0].Y1 = 1; di[0].Y2 = 12;
	di[0].Flags = DIF_LEFTTEXT; lstrcpy(di[0].Data, GetMsg(mFindTitle));
	i = 1;
	di[i].Type = DI_COMBOBOX; di[i].X1 = 6; di[i].Y1 = 3; di[i].X2 = 18;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lf1; di[i].Focus = true;
	i = 2;
	di[i].Type = DI_COMBOBOX; di[i].X1 = 21; di[i].Y1 = 3; di[i].X2 = 24;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lo1;
	i = 3;
	di[i].Type = DI_EDIT; di[i].X1 = 27; di[i].X2 = 49; di[i].Y1 = 3;
	di[i].History = "LookDBFCond"; di[i].Flags = DIF_HISTORY;
	lstrcpy(di[i].Data, Cond.Str[0]);
	i = 4;
	di[i].Type = DI_FIXEDIT; di[i].X1 = 27; di[i].X2 = 49; di[i].Y1 = 3;
	di[i].Flags = DIF_MASKEDIT;
	db.FiNum(Cond.Field[0]);
	if (db.cf->type != 'D' && db.cf->type != 'T') di[i].Flags |= DIF_HIDDEN;
	else {
		di[i - 1].Flags |= DIF_HIDDEN;
		lstrcpy(di[i].Data, Cond.Str[0]);
		di[i].Mask = (db.cf->type == 'D') ? D_Mask : T_Mask;
	}
	i = 5;
	di[i].Type = DI_COMBOBOX; di[i].X1 = 21; di[i].Y1 = 5; di[i].X2 = 24;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &op;
	i = 6;
	di[i].Type = DI_COMBOBOX; di[i].X1 = 6; di[i].Y1 = 7; di[i].X2 = 18;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lf2;
	if (!Cond.Oper)di[i].Flags |= DIF_DISABLE;
	i = 7;
	di[i].Type = DI_COMBOBOX; di[i].X1 = 21; di[i].Y1 = 7; di[i].X2 = 24;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &lo2;
	if (!Cond.Oper)di[i].Flags |= DIF_DISABLE;
	i = 8;
	di[i].Type = DI_EDIT; di[i].X1 = 27; di[i].X2 = 49; di[i].Y1 = 7;
	di[i].History = "LookDBFCond"; di[i].Flags = DIF_HISTORY;
	lstrcpy(di[i].Data, Cond.Str[1]);
	if (!Cond.Oper)di[i].Flags |= DIF_DISABLE;
	i = 9;
	di[i].Type = DI_FIXEDIT; di[i].X1 = 27; di[i].X2 = 49; di[i].Y1 = 7;
	di[i].Flags = DIF_MASKEDIT;
	if (!Cond.Oper)di[i].Flags |= DIF_DISABLE;
	db.FiNum(Cond.Field[1]);
	if (db.cf->type != 'D' && db.cf->type != 'T') di[i].Flags |= DIF_HIDDEN;
	else {
		di[i - 1].Flags |= DIF_HIDDEN;
		lstrcpy(di[i].Data, Cond.Str[0]);
		di[i].Mask = (db.cf->type == 'D') ? D_Mask : T_Mask;
	}
	i = 10;
	di[i].Type = DI_TEXT; di[i].Y1 = 8; di[i].Flags = DIF_SEPARATOR;
	i = 11;
	di[i].Type = DI_BUTTON; di[i].X1 = 32; di[i].Y1 = 9;
	lstrcpy(di[i].Data, GetMsg(mFindMark));
	di[i].Flags = DIF_CENTERGROUP; if (MarkNum >= db.dbH.nrec)di[i].Flags |= DIF_DISABLE;
	i = 12;
	di[i].Type = DI_BUTTON; di[i].X1 = 32; di[i].Y1 = 9;
	lstrcpy(di[i].Data, GetMsg(mFindUnmark));
	di[i].Flags = DIF_CENTERGROUP; if (!MarkNum)di[i].Flags |= DIF_DISABLE;
	i = 13;
	di[i].Type = DI_BUTTON; di[i].X1 = 3; di[i].Y1 = 11; di[i].Flags = DIF_CENTERGROUP;
	di[i].DefaultButton = 1; lstrcpy(di[i].Data, GetMsg(mFindFirst));
	i = 14;
	di[i].Type = DI_TEXT; di[i].X1 = 27; di[i].Y1 = 2; di[i].Flags = DIF_DISABLE;
	db.FiNum(Cond.Field[0]);
	db.FiType(di[i].Data);
	i = 15;
	di[i].Type = DI_TEXT; di[i].X1 = 27; di[i].Y1 = 6; di[i].Flags = DIF_DISABLE;
	db.FiNum(Cond.Field[1]);
	db.FiType(di[i].Data);

	i = Info.DialogEx(Info.ModuleNumber, -1, -1, 57, 14, "Cond", di, 16, 0, 0, DiCond, (LONG_PTR) di);
	delete[] op.Items;
	if (i < 11)return 0;
	c = FieldItem(di[1].ListPos); Cond.Field[0] = c->finum;
	j = (db.cf->type == 'D' || db.cf->type == 'T') ? 4 : 3;
	di[j].Data[255] = 0; lstrcpy(Cond.Str[0], FSF.RTrim(di[j].Data));
	Cond.Rel[0] = di[2].ListPos; CondVal(0);
	Cond.Oper = di[5].ListPos;
	if (Cond.Oper) {
		c = FieldItem(di[6].ListPos); Cond.Field[1] = c->finum;
		Cond.Rel[1] = di[7].ListPos;
		j = (db.cf->type == 'D' || db.cf->type == 'T') ? 9 : 8;
		di[j].Data[255] = 0; lstrcpy(Cond.Str[1], FSF.RTrim(di[j].Data));
		CondVal(1);
	}
	Set(CondSearch);  ShowStatus(5); // Fill Search string place
	if (i == 12) return 3;
	if (i == 13) return 1;
	if (MarkAlloc()) return 0;
	return 2;
}
//===========================================================================

void LOOK::CondVal(WORD n)
{
	char *s, *b;
	short i;
	SYSTEMTIME t;
	s = Cond.Str[n]; b = (char *) Cond.V[n].bt;
	db.FiNum(Cond.Field[n]);
	switch (db.cf->type) {
	case 'T': // DateTime
		DTsf2t(s, fmtT, &t);
		if (db.cf->filen == 8) { // Binary format
			Cond.V[n].dw[1] = DTt2tim(&t);
			Cond.V[n].dw[0] = DTt2dat(&t);
			if (Cond.V[n].dw[0])Cond.V[n].dw[0] += 1721410L;
		}
		else { // Character format s=dd/mm/yyyy-hh:mm:ss -> yyyymmddhhmmss=b
			DTstr((char *) b, &t, "$2BR");
		}
		break;
	case 'I': // Integer
		Cond.V[n].i64 = a_i64(s, 0);
		break;
	case 'B': // Double
		Cond.V[n].dbl = 1; for (i = 0; i < db.cf->dec; i++)Cond.V[n].dbl *= 10;
		Cond.V[n].dbl = a_i64(s, 0, db.cf->dec) / Cond.V[n].dbl;
		break;
	case 'Y': // Currency
		Cond.V[n].i64 = a_i64(s, 0, 4);
		break;
	case 'D': // ASCII Date s=dd/mm/yyyy -> yyyymmdd=b
		DTsf2t(s, fmtD, &t);
		DTstr((char *) b, &t, "$");
		break;
	case 'N': case 'F': Cond.V[n].i64 = a_i64(s, 0, db.cf->dec);
		break;
	default:
		if (Yes(WinCode) && db.FiChar())ToAlt(s, b);
		else lstrcpy(b, s);
		b[db.cf->filen] = 0;
	}
}
//===========================================================================

bool LOOK::Like(BYTE *sample, BYTE *line)
{
	if (!sample[0]) return (line[0] == 0);
	if (!line[0]) return (sample[0] == 0);
	short s, l, nM, eq;
	BYTE MskS, MskM;
	MskS = Find.Mask[0]; MskM = Find.Mask[1];
	s = l = 0; nM = eq = -1;
	while (line[l]) {
		if (sample[s] == MskM) {  // Multy mask
			while (sample[++s] == MskM);
			if (!sample[s]) return true;
			nM = s; eq = -1;       // Store position after Multy mask
		}
		if (sample[s] == MskS) goto SINGLE_MATCH;   // Single mask
		if (sample[s] != line[l]) {
			if (nM < 0) return false;
			if (eq < 0) l++; else { l = eq; eq = -1; }
			s = nM; continue;
		}
SINGLE_MATCH:
		++l; if (nM >= 0 && eq < 0)eq = l;         // Store position after first match char
		if (!sample[++s]) {
			if (!line[l]) return true;
			return false;
		}
	}
	if (sample[s] == MskM) {
		while (sample[++s] == MskM);
		if (!sample[s]) return true;
	}
	return false;
}
//===========================================================================

bool LOOK::CondCheck(short n, BYTE *fi)
{
	short i;
	BYTE *sa = Cond.V[n].bt;
	switch (Cond.Rel[n]) {
	case 0: // =
		return Like(sa, fi);
	case 1: // <>
		return !Like(sa, fi);
	case 2: // >
		for (i = 0; sa[i]; i++) {
			if (fi[i] > sa[i]) return true;
			if (fi[i] < sa[i]) return false;
		}
		return false;
	case 3: // >=
		for (i = 0; sa[i]; i++) {
			if (fi[i] > sa[i]) return true;
			if (fi[i] < sa[i]) return false;
		}
		return true;
	case 4: // <
		for (i = 0; sa[i]; i++) {
			if (fi[i] < sa[i]) return true;
			if (fi[i] > sa[i]) return false;
		}
		return false;
	case 5: // <=
		for (i = 0; sa[i]; i++) {
			if (fi[i] < sa[i]) return true;
			if (fi[i] > sa[i]) return false;
		}
		return true;
	}
	return false;
}
//===========================================================================

bool LOOK::CondCheck(short n, __int64 fi)
{
	switch (Cond.Rel[n]) {
	case 0: // =
		if (fi == Cond.V[n].i64) return true; else return false;
	case 1: // <>
		if (fi != Cond.V[n].i64) return true; else return false;
	case 2: // >
		if (fi > Cond.V[n].i64) return true; else return false;
	case 3: // >=
		if (fi >= Cond.V[n].i64) return true; else return false;
	case 4: // <
		if (fi < Cond.V[n].i64) return true; else return false;
	case 5: // <=
		if (fi <= Cond.V[n].i64) return true; else return false;
	}
	return false;
}
//===========================================================================

bool LOOK::CondCheck(short n, double fi)
{
	switch (Cond.Rel[n]) {
	case 0: // =
		if (fi == Cond.V[n].dbl) return true; else return false;
	case 1: // <>
		if (fi != Cond.V[n].dbl) return true; else return false;
	case 2: // >
		if (fi > Cond.V[n].dbl) return true; else return false;
	case 3: // >=
		if (fi >= Cond.V[n].dbl) return true; else return false;
	case 4: // <
		if (fi < Cond.V[n].dbl) return true; else return false;
	case 5: // <=
		if (fi <= Cond.V[n].dbl) return true; else return false;
	}
	return false;
}
//===========================================================================

bool LOOK::CondCheck(short n, DWORD fi1, DWORD fi2)
{
	switch (Cond.Rel[n]) {
	case 0: // =
		if (fi1 != Cond.V[n].dw[0]) return false;
		if (fi2 == Cond.V[n].dw[1]) return true;
		return false;
	case 1: // <>
		if (fi1 == Cond.V[n].dw[0] && fi2 == Cond.V[n].dw[1]) return false;
		return true;
	case 2: // >
		if (fi1 > Cond.V[n].dw[0]) return true;
		if (fi1 < Cond.V[n].dw[0]) return false;
		if (fi2 > Cond.V[n].dw[1]) return true;
		return false;
	case 3: // >=
		if (fi1 > Cond.V[n].dw[0]) return true;
		if (fi1 < Cond.V[n].dw[0]) return false;
		if (fi2 >= Cond.V[n].dw[1]) return true;
		return false;
	case 4: // <
		if (fi1 < Cond.V[n].dw[0]) return true;
		if (fi1 > Cond.V[n].dw[0]) return false;
		if (fi2 < Cond.V[n].dw[1]) return true;
		return false;
	case 5: // <=
		if (fi1 < Cond.V[n].dw[0]) return true;
		if (fi1 > Cond.V[n].dw[0]) return false;
		if (fi2 <= Cond.V[n].dw[1]) return true;
		return false;
	}
	return false;
}
//===========================================================================

bool LOOK::CondCompare(short n)
{
	BYTE *s, b[256];
	short i;
	db.FiNum(Cond.Field[n]);
	s = db.rec + db.cf->loc;
	switch (db.cf->type) {
	case 'T': // DateTime
		if (db.cf->filen == 8) { // Binary format
			union { DWORD w; BYTE c[4]; } u, v;
			for (i = 0; i < 4; i++)u.c[i] = s[i];
			for (i = 4; i < 8; i++)v.c[i - 4] = s[i];
			return CondCheck(n, u.w, v.w);
		}
		break;
	case 'Y': // Currency
		union { __int64 w; BYTE c[8]; } cy;
		for (i = 0; i < 8; i++)cy.c[i] = s[i];
		return CondCheck(n, cy.w);
	case 'N': case 'F': // Number, Float
		for (i = 0; i < db.cf->filen; i++)b[i] = s[i];
		b[db.cf->filen] = 0;
		return CondCheck(n, a_i64((char *) b, 0, db.cf->dec));
	case 'B': // Double
		union { double w; BYTE c[8]; } uu;
		for (i = 0; i < 8; i++)uu.c[i] = s[i];
		return CondCheck(n, uu.w);
	case '0': // System
	case 'G': // General
	case 'M': // Memo
	case 'P': // Picture
		return false;
	case 'I': // Integer
		union { __int32 w; BYTE c[4]; } uuu;
		for (i = 0; i < 4; i++)uuu.c[i] = s[i];
		return CondCheck(n, __int64(uuu.w));
	}
	i = db.cf->filen; if (i > 255)i = 255;
	CopyMemory(b, s, i); b[i] = 0;
	if (!i)return CondCheck(n, b);
	while (--i)if (b[i] == 0 || b[i] == ' ')b[i] = 0;
	return CondCheck(n, b);
}
//===========================================================================

bool LOOK::CondYes(void)
{
	bool cd1, cd2;
	cd1 = CondCompare(0);
	if (!Cond.Oper) return cd1;
	if (Cond.Oper == 1 && !cd1) return false;
	if (Cond.Oper == 2 && cd1) return true;
	cd2 = CondCompare(1);
	switch (Cond.Oper) {
	case 1: return cd1 && cd2;
	case 2: return cd1 || cd2;
	case 3: return cd1 ^ cd2;
	case 4: return !(cd1 ^ cd2);
	}
	return false;
}
//===========================================================================

DWORD LOOK::CondNext(void)
{
	DWORD rn = recV[curY];
	for (;;) {
		if (++rn > db.dbH.nrec) return 0;
		if (MarkOnly) while (!Marked(rn)) if (++rn > db.dbH.nrec) return 0;
		if (db.Read(rn)) { ShowError(2); return 0; }
		if (CondYes()) return rn;
	}
}
//===========================================================================

static LONG_PTR WINAPI DiRepl(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}
//===========================================================================

short LOOK::ReplaceAsk(void)
{
	int i, j, x;
	FarDialogItem *di = new FarDialogItem[5];
	di[0].Type = DI_TEXT;   di[0].X1 = 1;
	lstrcpy(di[0].Data, GetMsg(mReplAsk)); x = lstrlen(di[0].Data) + 2;
	di[1].Type = DI_BUTTON; di[1].X1 = x;
	lstrcpy(di[1].Data, GetMsg(mYes));     x += lstrlen(di[1].Data) + 6;
	di[2].Type = DI_BUTTON; di[2].X1 = x;
	lstrcpy(di[2].Data, GetMsg(mNo));      x += lstrlen(di[2].Data) + 6;
	di[3].Type = DI_BUTTON; di[3].X1 = x;
	lstrcpy(di[3].Data, GetMsg(mAll));     x += lstrlen(di[3].Data) + 5;
	di[4].Type = DI_FIXEDIT; di[4].Y1 = 0;
	ReplaceStr(di[4].Data); i = lstrlen(di[4].Data);
	j = sw - x; if (i > j)i = j; j = (j - i) / 2;
	di[4].X1 = x + j; di[4].X2 = di[4].X1 + i - 1;
	i = Info.DialogEx(Info.ModuleNumber, 0, sh, sw - 1, sh, "Edit", di, 5, 0,
					  FDLG_SMALLDIALOG, DiRepl, 0);
	delete[] di;
	return i;
}
//===========================================================================

short LOOK::ReplaceDlg(void)
{
	int i, j;
	FarDialogItem *di = new FarDialogItem[14];

	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 3; di[0].X2 = sw - 8;
	di[0].Y1 = 1; di[0].Y2 = 11;
	lstrcpy(di[0].Data, GetMsg(mReplTitle));

	di[1].Type = DI_TEXT; di[1].X1 = 5; di[1].Y1 = 2;
	lstrcpy(di[1].Data, GetMsg(mReplFind));

	di[2].Type = DI_EDIT; di[2].X1 = 5; di[2].X2 = di[0].X2 - 2; di[2].Y1 = 3;
	di[2].Focus = true; di[2].History = "SearchText";
	di[2].Flags = DIF_HISTORY; lstrcpy(di[2].Data, Find.FD);

	di[3].Type = DI_TEXT; di[3].X1 = 5; di[3].Y1 = 4;
	lstrcpy(di[3].Data, GetMsg(mReplRepl));

	di[4].Type = DI_EDIT; di[4].X1 = 5; di[4].X2 = di[2].X2; di[4].Y1 = 5;
	di[4].Focus = true; di[4].History = "ReplaceText";
	di[4].Flags = DIF_HISTORY; di[4].Data[0] = 0;

	di[5].Type = DI_CHECKBOX; di[5].X1 = 5; di[5].Y1 = 6;
	di[5].Selected = Yes(FindAllFields); lstrcpy(di[5].Data, GetMsg(mFindAll));

	di[6].Type = DI_CHECKBOX; di[6].X1 = 5; di[6].Y1 = 7;
	di[6].Selected = Yes(FindCaseSens); lstrcpy(di[6].Data, GetMsg(mFindCase));

	di[7].Type = DI_CHECKBOX; lstrcpy(di[7].Data, GetMsg(mReplWords));
	di[7].X1 = 5; di[7].Y1 = 8;  di[7].Selected = Yes(WholeWords);

	di[8].Type = DI_CHECKBOX; lstrcpy(di[8].Data, GetMsg(mReplConf));
	di[8].X1 = 5; di[8].Y1 = 9;  di[8].Selected = Yes(ConfReplace);

	di[9].Type = DI_TEXT; lstrcpy(di[9].Data, GetMsg(mReplMask));
	di[9].X1 = 9; di[9].Y1 = 10;

	di[10].Type = DI_FIXEDIT; di[10].Data[0] = Find.Mask[0];
	di[10].Data[1] = Find.Mask[1];  di[10].Data[2] = 0;
	di[10].X1 = 5; di[10].X2 = 7; di[10].Y1 = 10;

	di[11].Type = DI_BUTTON; lstrcpy(di[11].Data, GetMsg(mReplUn));
	di[11].X1 = di[0].X2 - 5 - lstrlen(di[11].Data); di[11].Y1 = 8;
	if (MarkNum >= db.dbH.nrec || MarkOnly)di[11].Flags = DIF_DISABLE;

	di[12].Type = DI_BUTTON; lstrcpy(di[12].Data, GetMsg(mReplMark));
	di[12].X1 = di[0].X2 - 5 - lstrlen(di[12].Data); di[12].Y1 = 6;
	if (!MarkNum)di[12].Flags = DIF_DISABLE;

	di[13].Type = DI_BUTTON; lstrcpy(di[13].Data, GetMsg(mReplAll));
	di[13].X1 = di[0].X2 - 5 - lstrlen(di[13].Data); di[13].Y1 = 10;
	di[13].DefaultButton = 1;

	i = Info.Dialog(Info.ModuleNumber, -1, -1, sw - 4, 13, "Search", di, 14);
	data->Find.Step = data->Find.Pos = 0; data->Clear(FindReplace);
	if (i < 11) { delete[] di; return 0; }
	if (di[5].Selected)Set(FindAllFields); else Clear(FindAllFields);
	if (di[6].Selected)Set(FindCaseSens);  else Clear(FindCaseSens);
	if (di[7].Selected)Set(WholeWords);    else Clear(WholeWords);
	if (di[8].Selected)Set(ConfReplace);   else Clear(ConfReplace);
	di[2].Data[256] = 0; lstrcpy(Find.FD, di[2].Data);
	Clear(CondSearch); Set(FindReplace);
	Find.Len = lstrlen(Find.FD); if (!Find.Len) return 0;
	di[4].Data[256] = 0; lstrcpy(Find.RU, di[4].Data);
	Find.nMM = 0; for (j = 0; j < Find.Len; j++) if (Find.FD[j] == Find.Mask[1])Find.nMM++;
	Find.nMMr = 0; for (j = 0; Find.RU[j]; j++) if (Find.RU[j] == Find.Mask[1])Find.nMMr++;
	if (Find.Len != lstrlen(Find.RU)) {
		db.FiNum(coCurr->finum);
		if (db.cf->type == 'D' || db.cf->type == 'T') { ShowError(11); return 0; }
		if (Yes(FindAllFields)) {
			Column *c = (Column *) coFirst->Head();
			for (; c; c = (Column *) c->Next()) {
				db.FiNum(c->finum);
				if (db.cf->type == 'D' || db.cf->type == 'T') { ShowError(11); return 0; }
			}
		}
	}
	ShowStatus(5); // Fill Search string place
	if (di[10].Data[0])Find.Mask[0] = di[10].Data[0];
	if (di[10].Data[1])Find.Mask[1] = di[10].Data[1];
	delete[] di;
	if (Yes(WinCode))ToAlt(Find.RU);
	// 1 - Unmarked, 2 - Marked, 3 - All
	return i - 10;
}
//===========================================================================

short LOOK::FindAskSample(void)
{
	int i, j;
	FarDialogItem di[11];
	ZeroMemory(di, sizeof(di));

	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 3; di[0].X2 = sw - 8; di[0].Y1 = 1; di[0].Y2 = 8;
	di[0].Flags = DIF_LEFTTEXT; lstrcpy(di[0].Data, GetMsg(mFindTitle));

	di[1].Type = DI_EDIT; di[1].X1 = 5; di[1].X2 = di[0].X2 - 2; di[1].Y1 = 2;
	di[1].Focus = true; di[1].History = "SearchText";
	di[1].Flags = DIF_HISTORY; lstrcpy(di[1].Data, Find.FD);

	di[2].Type = DI_CHECKBOX; di[2].X1 = 5; di[2].Y1 = 3;
	di[2].Selected = Yes(FindInvert); lstrcpy(di[2].Data, GetMsg(mFindInvert));

	di[3].Type = DI_CHECKBOX; di[3].X1 = 5; di[3].Y1 = 4;
	di[3].Selected = Yes(FindAllFields); lstrcpy(di[3].Data, GetMsg(mFindAll));

	di[4].Type = DI_CHECKBOX; di[4].X1 = 5; di[4].Y1 = 5;
	di[4].Selected = Yes(FindCaseSens); lstrcpy(di[4].Data, GetMsg(mFindCase));

	di[5].Type = DI_CHECKBOX; di[5].X1 = 5; di[5].Y1 = 6;
	di[5].Selected = Yes(WholeWords); lstrcpy(di[5].Data, GetMsg(mReplWords));

	di[6].Type = DI_TEXT; lstrcpy(di[6].Data, GetMsg(mReplMask));
	di[6].X1 = 9; di[6].Y1 = 7;

	di[7].Type = DI_FIXEDIT; di[7].Data[0] = Find.Mask[0];
	di[7].Data[1] = Find.Mask[1]; di[7].Data[2] = 0;
	di[7].X1 = 5; di[7].X2 = 7; di[7].Y1 = 7;

	di[8].Type = DI_BUTTON; lstrcpy(di[8].Data, GetMsg(mFindFirst));
	di[8].X1 = di[0].X2 - lstrlen(di[8].Data) - 5;
	di[8].Y1 = 5; di[8].DefaultButton = 1;

	di[9].Type = DI_BUTTON; lstrcpy(di[9].Data, GetMsg(mFindMark));
	di[9].X1 = di[0].X2 - lstrlen(di[9].Data) - 5; di[9].Y1 = 6;
	if (MarkNum >= db.dbH.nrec)di[9].Flags = DIF_DISABLE;

	di[10].Type = DI_BUTTON; lstrcpy(di[10].Data, GetMsg(mFindUnmark));
	di[10].X1 = di[0].X2 - lstrlen(di[10].Data) - 5; di[10].Y1 = 7;
	if (!MarkNum)di[10].Flags = DIF_DISABLE;

	i = Info.Dialog(Info.ModuleNumber, -1, -1, sw - 4, 10, "Search", di, 11);
	if (i < 8)return 0;
	if (di[7].Data[0])Find.Mask[0] = di[7].Data[0];
	if (di[7].Data[1])Find.Mask[1] = di[7].Data[1];
	Find.Pos = Find.Step = 0;
	if (di[2].Selected)Set(FindInvert); else Clear(FindInvert);
	if (di[3].Selected)Set(FindAllFields); else Clear(FindAllFields);
	if (di[4].Selected)Set(FindCaseSens);  else Clear(FindCaseSens);
	if (di[5].Selected)Set(WholeWords);    else Clear(WholeWords);
	di[1].Data[255] = 0; lstrcpy(Find.FD, di[1].Data);
	Clear(CondSearch); Clear(FindReplace);
	Find.Len = lstrlen(Find.FD); if (!Find.Len) return 0;
	Find.nMM = 0; for (j = 0; j < Find.Len; j++) if (Find.FD[j] == Find.Mask[1])Find.nMM++;
	ShowStatus(5); // Fill Search string place
	if (i == 8) return 1; if (i == 10) return 3;
	if (MarkAlloc()) return 0;
	return 2;
}
//===========================================================================

void LOOK::ShowFindMsg(DWORD nr)
{
	TCHAR *s, *Msg[6];
	Msg[0] = (TCHAR *) GetMsg(mFindTitle);
	BYTE ndx = 1;
	if (Yes(CondSearch)) {
		Column *c = FindFin(Cond.Field[0]);
		s = Msg[ndx++] = new TCHAR[lstrlen(c->name) + lstrlen(C_REL[Cond.Rel[0]]) + sizeof(TCHAR) + lstrlen(Cond.Str[0]) + 1];
		lstrcpy(s, c->name); lstrcat(s, C_REL[Cond.Rel[0]]); lstrcat(s, TEXT(" ")); lstrcat(s, Cond.Str[0]);
		if (Cond.Oper) {
			Msg[ndx++] = (TCHAR *) C_OPER[Cond.Oper];
			c = FindFin(Cond.Field[1]);
			s = Msg[ndx++] = new TCHAR[lstrlen(c->name) + lstrlen(C_REL[Cond.Rel[1]]) + sizeof(TCHAR) + lstrlen(Cond.Str[1]) + 1];
			lstrcpy(s, c->name); lstrcat(s, C_REL[Cond.Rel[1]]); lstrcat(s, TEXT(" ")); lstrcat(s, Cond.Str[1]);
		}
	}
	else {
		s = Msg[ndx++] = new TCHAR[(sizeof(TCHAR) << 1) + lstrlen(Find.FD) + (sizeof(TCHAR) << 1) + 1];
		lstrcpy(s, TEXT("<<")); lstrcat(s, Find.FD); lstrcat(s, TEXT(">>"));
	}
	s = Msg[ndx++] = new TCHAR[256];
	if (nr) FSF.snprintf(s, 255, TEXT("%s %lu"), GetMsg(mFindYes), nr);
	else lstrcpyn(s, GetMsg(mFindNo), 255);
	lstrcat(s, TEXT("."));
	Msg[ndx++] = TEXT("\x01");
	Info.Message(Info.ModuleNumber, FMSG_MB_OK, TEXT("Status"), Msg, ndx, 0);
	delete Msg[----ndx];
	delete Msg[--ndx];
	if (Yes(CondSearch) && Cond.Oper) {
		delete Msg[----ndx];
	}
}
//===========================================================================

void LOOK::ShowReplMsg(DWORD nr)
{
	TCHAR Buffer[256];
	const TCHAR *Msg[] = {
		GetMsg(mReplTitle),
		Buffer,
		TEXT("\x01")
	};
	FSF.snprintf(Buffer, 255, TEXT("%lu %s"), nr, GetMsg(mReplYes));
	Info.Message(Info.ModuleNumber, FMSG_MB_OK, TEXT("Status"), Msg, ARRAYSIZE(Msg), 0);
}
//===========================================================================

Column *LOOK::FindFin(short fin)
{
	Column *c;
	for (c = (Column *) coFirst->Head(); c; c = (Column*) c->Next()) {
		if (c->finum == fin) return c;
	}
	if (!Hid) return NULL;
	for (c = Hid; c; c = (Column*) c->Prev()) {
		if (c->finum == fin) return c;
	}
	return NULL;
}
//===========================================================================

short LOOK::FindCompare(WORD fn)
{
	char c[512];
	short i, j, L, sBack, lBack, ret;
	bool NoWord, YesWholeWords;
	db.FiNum(fn); L = db.FiDispE(c);
	ret = 1; if (Yes(FindInvert)) ret = 0;
	if (Find.Len - Find.nMM > L) goto NOT_FOUND;
	if (No(FindCaseSens)) if (Yes(WinCode)) CharUpperBuff(c, lstrlen(c)); else FSF.LStrupr(c);
	L -= (Find.Len - Find.nMM) - 1; NoWord = false;
	YesWholeWords = Yes(WholeWords);
	j = 0;
	if (Yes(FindReplace)) {
		j = Find.Pos + Find.Step;
		ret = 1;
		if (!Find.RU[0]) {
			if (db.IsEmpty()) goto NOT_FOUND;
			for (i = j; c[i] == ' '; i++);
			if (!c[i]) goto NOT_FOUND;
		}
		if (j >= L) goto NOT_FOUND;
	}
	sBack = i = 0; Find.LenF = 0; Find.Pos = lBack = -1;
	if (Find.FU[i] == Find.Mask[1]) {
		Find.Pos = j;
		while (Find.FU[++i] == Find.Mask[1]);
		if (!Find.FU[i]) goto YES_FOUND;
		sBack = i;
	}
	while (c[j]) {
		if (Find.FU[i] == Find.Mask[1]) {
			while (Find.FU[++i] == Find.Mask[1]);
			if (!Find.FU[i]) {
				if (!YesWholeWords) goto YES_FOUND;      // Не нужны целые слова
				if (!YesAlphaNum(c[j])) goto YES_FOUND;  // Было целое слово
				goto NOT_MATCH;
			}
			if (Find.Pos < 0) Find.Pos = j; // 1-st position of founded string
			sBack = i;                   // Store back position in sample (after mask)
			lBack = -1;                  // Undefine back position in testing line
		}
		if (Find.FU[i] == Find.Mask[0]) goto YES_MATCH;           // Single mask
		if (YesWholeWords) {
			if (j)NoWord = YesAlphaNum(c[j - 1]);
			if (NoWord) goto NOT_MATCH;
		}
		if (Find.FU[i] != c[j]) {
NOT_MATCH:
			if (!sBack) {     // If multymask not stored -> exit
				if (i)i = 0; else j++;
				Find.Pos = lBack = -1;
				continue;
			}
			if (lBack < 0) j++; else { j = lBack; lBack = -1; }
			i = sBack; continue;
		}
YES_MATCH:
		if (Find.Pos < 0) Find.Pos = j;           // 1-st position of founded string
		++j; if (sBack >= 0 && lBack < 0)lBack = j; // Store position after first match char
		if (!Find.FU[++i]) goto YES_FOUND;
	}
	if (Find.FU[i] == Find.Mask[1]) while (Find.FU[++i] == Find.Mask[1]);
	if (!Find.FU[i]) goto YES_FOUND;
NOT_FOUND:
	Find.Pos = Find.Step = Find.LenF = 0;
	return 1 - ret;

YES_FOUND:
	Find.LenF = j - Find.Pos;               // Длина найденной подстроки
	return ret;
}
//===========================================================================

short LOOK::ReplaceStr(char *cc)
{
	char *c, R[512];
	WORD i, j;
	db.FiDispE(cc); c = cc + Find.Pos; i = lstrlen(c);
	j = lstrlen(Find.RU) - Find.nMMr;
	if (!j) {
		if (Find.nMMr) return Find.LenF;
		MoveMemory(c, c + Find.LenF, i - Find.LenF + 1);
		return 0;
	}
	lstrcpy(R, Find.RU);
	if (Find.nMMr) {
		WORD k, *n;
		char *r = Find.RU;
		n = new WORD[Find.nMMr];
		if (j < Find.LenF) {
			j = Find.LenF - j;
			while (j)for (k = 0; k < Find.nMMr && j; k++, j--)n[k]++;
		}
		k = j = 0;
		for (; *r;) {
			if (*r == Find.Mask[1]) {
				while (n[j]) { R[k++] = Find.Mask[0]; n[j]--; }
				r++; j++;
				continue;
			}
			R[k++] = *r; r++;
		}
		R[k] = 0;
	}
	j = lstrlen(R);
	if (i > Find.LenF && j != Find.LenF) {
		MoveMemory(c + j, c + Find.Len, i - Find.Len + 1);
		if (j > Find.Len)FillMemory(c + Find.Len, j - Find.Len, ' ');
	}
	for (i = 0; i < j; i++) if (R[i] != Find.Mask[0])c[i] = R[i];
	return j;
}
//===========================================================================

short LOOK::Replace()
{
	char cc[512];
	short j;
	j = ReplaceStr(cc);
	db.SetField(cc);
	if (db.ReWrite()) { ShowError(2); return 1; }
	Find.Step = j;
	return 0;
}
//===========================================================================

void LOOK::ReplaceAll(short ma, DWORD mrn = 0)
{
	DWORD rn;
	Column *c;
	if (mrn) { rn = recV[curY] - 1; c = coCurr; }
	else { rn = 0; c = NULL; }
	data->Find.Step = 0;
	for (;;) {
		if (++rn > db.dbH.nrec) break;
		if (MarkOnly) {
			if (ma<2) goto SHOW;
			while (!Marked(rn)) if (++rn>db.dbH.nrec) goto SHOW;
		}
		else if (NotRepl(rn, ma)) continue;
		if (db.Read(rn)) { ShowError(2); break; }
		if (Yes(FindAllFields)) {
			if (!c) c = (Column *) coFirst->Head();
			for (; c; c = (Column *) c->Next()) {
				while (FindCompare(c->finum)) { if (Replace()) goto SHOW; ++mrn; }
			}
			c = NULL;
			continue;
		}
		while (FindCompare(coCurr->finum)) { if (Replace()) goto SHOW; ++mrn; }
	}
SHOW:
	ShowReplMsg(mrn);
}
//===========================================================================

bool LOOK::NotRepl(DWORD rn, short ma = 3)
{
	// 1 - Unmarked, 2 - Marked, 3 - All
	if (ma > 2) return 0;
	if (ma == 2) return !Marked(rn);
	return Marked(rn);
}
//===========================================================================

DWORD LOOK::FindNext(short *f, short ma = 3)
{
	DWORD rn;
	Column *c;
	rn = recV[curY];
	if ((Yes(FindAllFields) && coCurr->Next()) || Yes(FindReplace))--rn;
	for (;;) {
		if (++rn > db.dbH.nrec) return 0;
		if (MarkOnly) {
			if (ma<2) return 0;
			while (!Marked(rn)) if (++rn>db.dbH.nrec) return 0;
		}
		else if (NotRepl(rn, ma)) continue;
		if (db.Read(rn)) { ShowError(2); return 0; }
		if (Yes(FindAllFields)) {
			c = (Column *) coFirst->Head();
			if (rn == recV[curY]) c = Yes(FindReplace) ? coCurr : (Column *) coCurr->Next();
			for (; c; c = (Column *) c->Next()) if (FindCompare(c->finum)) break;
			if (!c) continue;
			*f = c->finum; return rn;
		}
		if (FindCompare(coCurr->finum)) { *f = coCurr->finum; return rn; }
	}
}
//===========================================================================

DWORD LOOK::FindMark(WORD m)
{
	DWORD rn, mrn, ncur;
	Column *c;
	rn = recV[curY] - 1;
	ncur = 0;
	for (mrn = 0;;) {
		if (++rn > db.dbH.nrec) goto SHOW;
		if (MarkOnly && !m) while (!Marked(rn)) if (++rn > db.dbH.nrec) goto SHOW;
		if (db.Read(rn)) { ShowError(2); goto SHOW; }
		if (Yes(CondSearch)) { if (CondYes()) goto MARK; continue; }
		if (FindCompare(coCurr->finum)) goto MARK;
		if (No(FindAllFields)) continue;
		for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
			if (c == coCurr) continue;
			if (FindCompare(c->finum)) goto MARK;
		}
		if (!ncur)ncur = rn;
		continue;
MARK:
		if (m)MarkSet(rn); else MarkClear(rn);
		++mrn;
	}
SHOW:
	ShowFindMsg(mrn);
	if (m || !MarkOnly || !MarkNum || !mrn) return mrn;
	if (ncur == recV[curY]) return mrn;
	if (ncur) { recV[0] = ncur; curY = 0; return mrn; }
	for (rn = recV[curY] - 1; rn; rn--)if (Marked(rn)) { recV[0] = rn; curY = 0; return mrn; }
	MarkOnly = 0; MarkNum = 0;
	return mrn;
}
//===========================================================================

void LOOK::KeyShow(void)
{
	int i;
	AttrLine(0, sw, sh, at[1]);
	for (i = 0; i < 10; i++) {
		AttrLine(i * 8, 2, sh, at[0]);
		if (i < 9)ShowChar('1' + i, i * 8 + 1, sh);
		else { ShowChar('1', i * 8, sh); ShowChar('0', i * 8 + 1, sh); }
	}
	ShowStr(GetMsg(mBarF1), 2, sh, 0, 6);
	ShowStr(GetMsg(mBarF2), 10, sh, 0, 6);
	ShowStr(GetMsg(mBarSF7), 18, sh, 0, 6);
	ShowStr(GetMsg(mBarF4), 26, sh, 0, 6);
	ShowStr(GetMsg(mBarF6), 42, sh, 0, 6);
	ShowStr(GetMsg(mBarF7), 50, sh, 0, 6);
	ShowStr(GetMsg(mBarF9), 66, sh, 0, 6);
	ShowStr(GetMsg(mBarF10), 74, sh, 0, 6);
	ShowStr(Yes(WinCode) ? "DOS" : aw + 1, 58, sh, 0, 6);
}
//===========================================================================

void LOOK::ChangeCode(void)
{
	if (Yes(WinCode)) {
		Clear(WinCode); ShowStr("DOS", sXcode, 0, 10); ShowStr(aw + 1, 58, sh, 0, 6);
		if (Find.FD[0]) ToOem(Find.FU);
	}
	else {
		Set(WinCode); ShowStr(aw + 1, sXcode, 0, 10); ShowStr("DOS", 58, sh, 0, 6);
		if (Find.FD[0]) ToAlt(Find.FU);
	}
	ShowPage();
}
//===========================================================================

struct EditDialog {
	WORD nItems;       // Quantity of dialog items
	WORD wText;        // Maximum width of text line
	WORD wEdit;        // Maximum width of edit line
	WORD width;        // Width of dialog
	short iOk;         // Index of "Ok" item
	short iAdd;        // Index of "Add" item
	short iNext;       // Index of "Go to next dialog" item
	short iPrev;       // Index of "Go to previous dialog" item
	WORD iLast;        // Index of element that ready for init
	WORD yLine;        // Y for next init line
	FarDialogItem *di; // Dialog items array

	~EditDialog() { if (di) { delete[] di; di = NULL; } };
	WORD Init(WORD nL, WORD wT, WORD wE, BYTE but);
	short Line(char *t, char *e, short w, const char *mask = NULL);
	short Exec(void);
};

short EditDialog::Exec(void)
{
	short r;
	r = Info.Dialog(Info.ModuleNumber, -1, -1, width, yLine + 4, "Edit", di, nItems);
	if (r == iOk) return 0;
	if (r == iAdd) return 1;
	if (r == iPrev) return 2;
	if (r == iNext) return 3;
	return -1;
}

short EditDialog::Line(char  *t, char *e, short w, const char *mask)
{
	if (iLast == nItems) return 0;
	di[iLast].Type = DI_TEXT; di[iLast].X1 = 4; di[iLast].Y1 = yLine;
	FSF.sprintf(di[iLast].Data, "%*s", wText, t);
	++iLast; di[iLast].X1 = wText + 5; if (yLine == 2) di[iLast].Focus = true;
	di[iLast].Type = DI_EDIT;
	if (mask) {
		di[iLast].Type = DI_FIXEDIT;
		di[iLast].Mask = mask;
		di[iLast].Flags = DIF_MASKEDIT;
	}
	di[iLast].X2 = (w > wEdit) ? wEdit : w;
	di[iLast].X2 += di[iLast].X1 - 1; di[iLast].Y1 = yLine; lstrcpy(di[iLast].Data, e);
	++iLast; ++yLine; return iLast - 1;
}

WORD EditDialog::Init(WORD nL, WORD wT, WORD wE, BYTE but)
{
	short w;
	WORD wMin;         // Minimum width of edit+text line
	BYTE bNext, bPrev;
	bNext = but & 0x01; bPrev = (but >> 1) & 0x01;
	nItems = nL * 2 + 4 + bNext + bPrev;
	di = new FarDialogItem[nItems]; if (!di) return 1;
	wMin = 14 + lstrlen(GetMsg(mButSave)) + lstrlen(GetMsg(mButAdd));
	if (bNext)wMin += 8; if (bPrev)wMin += 8;
	w = lstrlen(GetMsg(mEditTitle)) + 4;
	if (wMin < w)wMin = w; wText = wT; wEdit = wE; w = wT + wE + 1; width = w;
	if (wMin > w) { width = wMin; wMin = (wMin - w) / 2; wText += wMin; wEdit = width - wText - 1; }
	width += 8; iLast = 2; yLine = 2;
	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 3; di[0].Y1 = 1;
	di[0].X2 = width - 4; di[0].Y2 = nL + 4;
	lstrcpy(di[0].Data, GetMsg(mEditTitle));
	di[1].Type = DI_TEXT; di[1].Y1 = di[0].Y2 - 2; di[1].Flags = DIF_SEPARATOR;
	if (bPrev) {
		iPrev = iLast; di[iLast].Type = DI_BUTTON; di[iLast].Y1 = di[1].Y1 + 1;
		di[iLast].Data[0] = 0x11; di[iLast].Data[1] = (char) 0xc4; di[iLast].Data[2] = 0;
		di[iLast].Flags = DIF_CENTERGROUP; ++iLast;
	}
	iOk = iLast; di[iLast].Type = DI_BUTTON; di[iLast].Y1 = di[1].Y1 + 1;
	di[iLast].DefaultButton = 1; lstrcpy(di[iLast].Data, GetMsg(mButSave));
	di[iLast].Flags = DIF_CENTERGROUP; ++iLast;

	iAdd = iLast; di[iLast].Type = DI_BUTTON; di[iLast].Y1 = di[1].Y1 + 1;
	lstrcpy(di[iLast].Data, GetMsg(mButAdd));
	di[iLast].Flags = DIF_CENTERGROUP; ++iLast;
	if (bNext) {
		iNext = iLast; di[iLast].Type = DI_BUTTON; di[iLast].Y1 = di[1].Y1 + 1;
		di[iLast].Data[0] = (char) 0xc4; di[iLast].Data[1] = 0x10; di[iLast].Data[2] = 0;
		di[iLast].Flags = DIF_CENTERGROUP; ++iLast;
	}
	return 0;
}

WORD LOOK::EditRecord(void)
{
	short i, j, nL, nD, dL, rL, wT, wE;
	char *r, s[384];
	BYTE but;
	Column *c = (Column *) coFirst->Head();
	if (db.Read(recV[curY])) { ShowError(2); return 1; }
	db.fmtD = fmtD; db.fmtT = fmtT;
	for (nL = wT = wE = 0; c; c = (Column *) c->Next()) {
		++nL; i = lstrlen(c->name); if (i > wT) wT = i;
		db.FiNum(c->finum); i = db.FiWidth();
		if (i > wE) wE = i;
	}
	j = sh - 6; nD = (nL + j - 1) / j; dL = nL / nD; rL = nL%nD;
	EditDialog *ed = new EditDialog[nD];
	if (!ed) { ShowError(4); db.fmtD = fmtDV; db.fmtT = fmtTV; return 1; }
	j = sw - 13; if (wT + wE > j) wE = j - wT;
	for (i = 0; i < nD; i++) {
		but = 0x03; j = dL; if (rL) { ++j; --rL; }
		if (!i) but &= 0x01; if (nD - i == 1) but &= 0x02;
		if (ed[i].Init(j, wT, wE, but)) {
			delete[] ed; ShowError(4);
			db.fmtD = fmtDV; db.fmtT = fmtTV;
			return 1;
		}
	}
	c = (Column *) coFirst->Head();
	for (i = 0; c; c = (Column *) c->Next()) {
		db.FiNum(c->finum); wE = db.FiWidth(); db.FiDispE(s);
		if (Yes(WinCode) && db.FiChar())ToOem(s);
		for (j = 0; !j;) {
			switch (db.cf->type) {
			case 'T': j = ed[i].Line(c->name, s, wE, T_Mask); break;
			case 'D': j = ed[i].Line(c->name, s, wE, D_Mask); break;
			default:  j = ed[i].Line(c->name, s, wE); break;
			}
			if (!j)i++;
		}
		c->dinum = i; c->idnum = j;
	}
	for (i = 0; i < nD; ) {
		j = ed[i].Exec(); if (j < 2) break;
		if (j - 2)i++; else i--;
	}
	if (j<0) { delete[] ed; db.fmtD = fmtDV; db.fmtT = fmtTV; return 1; }
	if (data->LookOnly) {
		data->ShowError(8);
		db.fmtD = fmtDV; db.fmtT = fmtTV;
		return 1;
	}
	for (c = (Column *) coFirst->Head(); c; c = (Column *) c->Next()) {
		db.FiNum(c->finum); r = ed[c->dinum].di[c->idnum].Data;
		if (Yes(WinCode) && db.FiChar())ToAlt(r);
		db.SetField(r);
	}
	delete[] ed;
	db.fmtD = fmtDV; db.fmtT = fmtTV;
	if (j) {
		for (c = Hid; c; c = (Column *) c->Prev()) { db.FiNum(c->finum); db.SetEmpty(); }
		if (db.Append()) { ShowError(2); return 1; }
		if (MarkMax && db.dbH.nrec>MarkMax) { MarkFree(); MarkAlloc(); }
		ShowStatus(6);
		return 0;
	}
	if (db.ReWrite()) { ShowError(2); return 1; }
	return 0;
}
//===========================================================================

WORD LOOK::ActualRecord(void)
{
	if (db.Read(recV[curY])) { ShowError(2); return 1; }
	if (!db.Invalid())db.rec[0] = '*'; else db.rec[0] = ' ';
	if (db.ReWrite()) { ShowError(3); return 1; }
	ShowChar(db.rec[0], Yes(LineNums) ? Wrec : 0, curY + 2);
	return 0;
}
//===========================================================================

WORD LOOK::ActualSelected(BYTE a)
{
	DWORD i;
	for (i = 1; i <= db.dbH.nrec; i++) {
		if (!Marked(i)) continue;
		if (db.Read(i)) { ShowError(2); return 1; }
		if (db.rec[0] == a) continue;
		db.rec[0] = a; if (db.ReWrite()) { ShowError(3); return 1; }
	}
	ShowPage();
	return 0;
}
//===========================================================================

void LOOK::Clipboard(void)
{
	char s[512];
	if (db.Read(recV[curY])) { ShowError(2); return; }
	db.FiNum(coCurr->finum); s[0] = 0; db.FiDisp(s);
	if (Yes(WinCode) && db.FiChar())ToOem(s);
	FSF.Trim(s);
	FSF.CopyToClipboard(s);
}
//===========================================================================

void LOOK::ShowF5(void)
{
	if (!MarkNum) { ClearRect(34, sh, 6, 1); MarkOnly = 0; ShowStatus(3); ShowPage(); return; }
	if (MarkOnly) {
		ShowStr(GetMsg(mBarAF5), 34, sh, 0, 6);
		if (!Marked(recV[0])) recV[0] = recV[1];
	}
	else ShowStr(GetMsg(mBarHF5), 34, sh, 0, 6);
	ShowStatus(3); ShowPage();
}
//===========================================================================

WORD LOOK::GoUp(WORD delta)
{
	if (!curY && recV[0] < 2) return 0;
	ClearCur();
	if (curY >= delta) { curY -= delta; return 1; }
	curY = 0;
	if (recV[0] < 2) return 1;
	if (!MarkOnly) { recV[0] = (recV[0] < delta + 1) ? 1 : recV[0] - delta; return 2; }
	DWORD i, j;
	WORD n, k;
	i = recV[0] - 1; j = i & 0x0000000f; i >>= 4;  i &= 0x0fffffff;
	if (!j) { if (!i)return 1; j = 15; --i; }
	else --j;
	for (;; i--) {
		k = 0x8000;
		for (;; j--) {
			n = j ? k >> j : k;
			if (M[i] & n) { recV[0] = (i << 4) | j; recV[0] += 1; if (!(--delta))return 2; }
			if (!j) break;
		}
		if (!i)return 2;
		j = 15;
	}
}
//===========================================================================

WORD LOOK::GoTop(void)
{
	if (!curY && recV[0] < 2) return 0;
	ClearCur(); curY = 0;
	if (recV[0] < 2) return 1;
	if (!MarkOnly) { recV[0] = 1; return 2; }
	DWORD j;
	for (j = 1; j <= db.dbH.nrec; j++) if (Marked(j)) break;
	recV[0] = j;
	return 2;
}
//===========================================================================

WORD LOOK::GoDn(WORD delta)
{
	if (curY == botY && (recV[botY] == db.dbH.nrec || botY < sh - 3)) return 0;
	ClearCur(); if (curY + delta <= botY) { curY += delta; return 1; }
	curY = botY; if ((delta > 1 && curY < botY) || (botY < sh - 3)) return 1;
	if (!MarkOnly) {
		DWORD K = db.dbH.nrec + 3 - sh;
		recV[0] = (recV[0] + delta > K) ? K : recV[0] + delta;
		return 2;
	}
	WORD k = 0;
	DWORD i;
	for (i = recV[botY] + 1; i <= db.dbH.nrec; i++) {
		if (!Marked(i))continue;
		++k; if (k >= delta) break;
	}
	if (!k) return 0;
	if (k > sh - 3)k = sh - 3;
	recV[0] = recV[k];
	return 2;
}
//===========================================================================

WORD LOOK::GoBot(void)
{
	if (curY == botY && recV[botY] == db.dbH.nrec) return 0;
	ClearCur(); curY = botY;  if (botY < sh - 3) return 1;
	if (!MarkOnly) {
		if (db.dbH.nrec <= sh - 3) { recV[0] = 1; curY = db.dbH.nrec - 1; return 2; }
		recV[0] = db.dbH.nrec + 3 - sh; curY = sh - 3;
		return 2;
	}
	DWORD i; WORD k = 0;
	for (i = db.dbH.nrec; i; i--) {
		if (!Marked(i))continue;
		++k; if (k >= sh - 2) break;
	}
	if (!i || !k) return 0;
	recV[0] = i;
	return 2;
}
//===========================================================================

WORD LOOK::GoFirst(void)
{
	if (!curX) {
		if (!coFirst->Prev()) return 0;
		coFirst = (Column *) coFirst->Head();
		return 2;
	}
	ClearCur(); curX = 0; coCurr = coFirst; Xcur = 2;
	if (!coFirst->Prev()) return 1;
	coFirst = (Column *) coFirst->Head();
	return 2;
}
//===========================================================================

WORD LOOK::GoLeft(void)
{
	if (!curX) {
		if (!coFirst->Prev()) return 0;
		coFirst = (Column *) coFirst->Prev();
		return 2;
	}
	ClearCur(); --curX; coCurr = (Column *) coCurr->Prev(); Xcur -= coCurr->wid;
	return 1;
}
//===========================================================================

WORD LOOK::GoLast(void)
{
	if (!coCurr->Next()) return 0;
	ClearCur(); curX = 0;
	if (!coLast->Next()) {
		Xcur = Yes(LineNums) ? Wrec + 2 : 2; coCurr = coFirst;
		for (; coCurr != coLast; coCurr = (Column *) coCurr->Next()) {
			++curX; Xcur += coCurr->wid;
		}
		if (!coTail) return 1;
		++curX;
		while (coFirst != coLast&&coTail > 0) {
			coTail -= coFirst->wid; --curX;
			coFirst = (Column *) coFirst->Next();
		}
		// UNDONE Broken column navigation thereafter
		return 2;
	}
	ClearCur(); curX = 0;
	coFirst = (Column *) coLast->Tail();
	short x = coFirst->wid + (Yes(LineNums) ? Wrec + 2 : 2); if (x >= sw) return 2;
	while (coFirst->Prev()) {
		auto c = (Column *) coFirst->Prev();
		if (x + c->wid >= sw) break;
		x += c->wid; ++curX; coFirst = c;
	}
	return 2;
}
//===========================================================================

WORD LOOK::GoRight(void)
{
	Column *c = (Column *) coCurr->Next();
	if (!c) return 0;
	ClearCur();
	if (coCurr == coLast) {
		if (c->wid + (Yes(LineNums) ? Wrec + 2 : 2) + coCurr->wid > sw) { coFirst = c; curX = 0; return 2; }
		short x, w = 0;
		for (x = 0, c = coCurr; c->Prev();) {
			w += c->wid; ++x; if (w <= sw) break;
			c = (Column *) c->Prev();
		}
		curX = x; coFirst = c;
		return 2;
	}
	++curX;
	if (c != coLast || (!coTail)) { Xcur += coCurr->wid;  coCurr = c; return 1; }
	while (coFirst != coLast&&coTail > 0) {
		coTail -= coFirst->wid; --curX;
		coFirst = (Column *) coFirst->Next();
	}
	return 2;
}
//===========================================================================

WORD LOOK::GoAsk(void)
{
	short L;
	char mask[16];
	DWORD rn;
	FarDialogItem di[3];
	ZeroMemory(di, sizeof(di));
	for (L = 0; L < Wrec; L++)mask[L + 1] = '9';
	mask[0] = 'X'; mask[L + 1] = 0;

	di[1].Type = DI_TEXT; lstrcpy(di[1].Data, GetMsg(mGoTo));
	L = lstrlen(di[1].Data);
	di[1].X1 = 4; di[1].Y1 = di[1].Y2 = 2;

	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 2; di[0].X2 = L + Wrec + 6;
	di[0].Y1 = 1; di[0].Y2 = 3; di[0].Data[0] = 0;

	di[2].Type = DI_FIXEDIT; di[2].X1 = L + 5; di[2].X2 = di[2].X1 + Wrec - 1; di[2].Y1 = 2;
	di[2].Focus = true; di[2].Mask = mask; di[2].Flags = DIF_MASKEDIT;
	di[2].DefaultButton = 1; di[2].Data[0] = 0;

	L = Info.Dialog(Info.ModuleNumber, -1, -1, L + Wrec + 9, 5, "Contents", di, 3);
	if (L != 2) return 1;
	L = 0;
	if (di[2].Data[0] == '+') L = 1;
	if (di[2].Data[0] == '-') L = -1;
	if (!L) { //--------------------- Jump to absolute record number
		if (di[2].Data[0] < '0') return 1;
		if (di[2].Data[0] > '9') return 1;
		rn = a_i64(di[2].Data, 0);
		if (!rn || rn > db.dbH.nrec) return 1;
	}
	else { //-------- Jump N records down (up) from current record
		__int64 Rrn = recV[curY] + L*a_i64(di[2].Data + 1, 0);
		if (Rrn <= 0) Rrn = 1;
		if (Rrn > db.dbH.nrec) Rrn = db.dbH.nrec;
		rn = Rrn;
	}
	ClearCur();
	for (curY = 0; curY <= botY; curY++) if (recV[curY] == rn) break;
	if (curY > botY) {
		recV[0] = rn; curY = 0; if (MarkOnly && !Marked(rn)) MarkOnly = 0;
		ShowPage(); return 0;
	}
	ShowCur();
	return 0;
}
//===========================================================================

WORD LOOK::GoFind(short ma)
{
	DWORD rn;
	short fn, CurOnly;
	// ma -->  1 - Unmarked, 2 - Marked, 3 - All
	rn = Yes(CondSearch) ? CondNext() : FindNext(&fn, ma);
	if (!rn) { if (No(FindReplace))ShowFindMsg(0); return 1; }
	ClearCur(); CurOnly = 1;
	for (curY = 0; curY <= botY; curY++) if (recV[curY] == rn) break;
	if (curY > botY) { recV[0] = rn; curY = 0; CurOnly = 0; }
	if (Yes(CondSearch)) goto FINISH;
	if (Yes(FindAllFields)) {
		short x, w;
		Column *c, *fc;
		fc = (Column *) coFirst->Head();
		w = Yes(LineNums) ? Wrec + 2 : 2;
		for (x = 0, c = fc; c->finum != fn; c = (Column *) c->Next()) {
			w += c->wid; ++x;
			while (w > sw) { w -= fc->wid; --x; fc = (Column *) fc->Next(); }
		}
		w += c->wid;
		while (w > sw && c != fc) { w -= fc->wid; --x; fc = (Column *) fc->Next(); }
		if (coFirst != fc)CurOnly = 0;
		coFirst = fc; curX = x; coCurr = c; Xcur = w - c->wid;
	}
FINISH:
	if (CurOnly) ShowCur();
	else { ShowPage(); db.Read(rn); db.FiNum(fn); }
	return 0;
}
//===========================================================================

WORD LOOK::GoField(int id)
{
	short x, w, CurOnly;
	Column *c, *fc;
	if (coCurr->finum == id) return 1;
	ClearCur(); CurOnly = 1;
	fc = (Column *) coFirst->Head();
	w = Yes(LineNums) ? Wrec + 2 : 2;
	for (x = 0, c = fc; c && c->finum != id; c = (Column *) c->Next()) {
		w += c->wid; ++x;
		while (w > sw) { w -= fc->wid; --x; fc = (Column *) fc->Next(); }
	}
	if (!c) goto HIDDEN;
	w += c->wid;
	while (w > sw && c != fc) { w -= fc->wid; --x; fc = (Column *) fc->Next(); }
	if (coFirst != fc)CurOnly = 0;
	coFirst = fc; curX = x; coCurr = c; Xcur = w - c->wid;
	if (CurOnly) ShowCur(); else ShowPage();
	return 0;
HIDDEN:
	if (!Hid) return 1; // There is no column for insertion
	if (!Hid->Prev()) { // Last hidden column insert
		coCurr->Before(Hid); Hid = NULL;
		HidNum = 0; ShowStatus(4); return 0;
	}
	for (c = Hid; c->finum != id; c = (Column *) c->Prev())if (!c) return 1;
	c->Extract();
	if (!c->Next()) Hid = (Column *) c->Prev();
	coCurr->Before(c);
	if (coCurr == coFirst) { coFirst = coCurr = c; curX = 0; Xcur = 2; }
	--HidNum; ShowStatus(4); ShowPage();
	return 0;
}
//===========================================================================

WORD LOOK::GoMouse(MOUSE_EVENT_RECORD *m)
{
	WORD r, i, f5, x, mY;
	Column *c = coFirst;
	f5 = 0; x = Yes(LineNums) ? Wrec + 2 : 2;
	for (i = 0; c != coLast; c = (Column *) c->Next()) {
		if (x + c->wid >= m->dwMousePosition.X) break;
		++i; x += c->wid;
	}
	mY = (m->dwMousePosition.Y > botY) ? botY : m->dwMousePosition.Y;
	if (m->dwButtonState == RIGHTMOST_BUTTON_PRESSED) {
		if (MarkAlloc()) return 1;
		MarkInvert(recV[mY]); f5 = 1;
	}
	if (mY == curY && i == curX) {
		if (!curY) {
			if (m->dwEventFlags != DOUBLE_CLICK) r = GoUp(1); else r = GoUp(sh - 3);
			goto SHOW;
		}
		if (curY == botY) {
			if (m->dwEventFlags != DOUBLE_CLICK) r = GoDn(1); else r = GoDn(sh - 3);
			goto SHOW;
		}
		if (!curX) { r = GoLeft(); goto SHOW; }
		if (c == coLast) { r = GoRight(); goto SHOW; }
		r = 0; goto SHOW;
	}
	ClearCur(); curY = mY; r = 1;
	if (i == curX) goto SHOW;
	curX = i; coCurr = c; Xcur = x;
	if (c != coLast || !coTail || !i) goto SHOW;
	--curX; coCurr = (Column *) c->Prev(); Xcur -= coCurr->wid;
	r = GoRight();
SHOW:
	if (f5) { if (!r)ClearCur(); ShowF5(); return 0; }
	switch (r) {
	case 0: return 1;
	case 1: ShowCur(); break;
	default: ShowPage();
	}
	return 0;
}
//===========================================================================

WORD LOOK::TopMouse(int Line, MOUSE_EVENT_RECORD *m)
{
	WORD r;
	if (Line) { // Field headers line. Horizontal Home/End
		if (m->dwMousePosition.X < sw / 2) r = GoFirst();
		else r = GoLast();
	}
	else { // Status Line. Vertical Top/Bottom
		if (m->dwMousePosition.X + Wrec + 1 >= sw) r = GoBot();
		else if (m->dwMousePosition.X + Wrec + Wrec + 1 >= sw) r = GoTop();
		else r = 0;
	}
	switch (r) {
	case 0: return 1;
	case 1: ShowCur();  break;
	default: ShowPage();
	}
	return 0;
}
//===========================================================================

WORD LOOK::ClmnDelete(void)
{
	if (!coFirst->Next()) return 1; // Cannot delete last column
	coCurr->Extract();
	if (!coCurr->Prev() || coCurr == coFirst) coFirst = (Column *) coCurr->Next();
	if (!coCurr->Next()) curX--;
	Hid = (Column *) Hid->Add(coCurr);
	++HidNum; ShowStatus(4);
	return 0;
}
//===========================================================================

WORD LOOK::ClmnInsert(void)
{
	if (!Hid) return 1; // There is no column for insertion
	if (!Hid->Prev()) { // Last hidden column insert
		coCurr->Before(Hid); Hid = NULL;
		HidNum = 0; ShowStatus(4); return 0;
	}
	Column *h;
	int i, NumItem;
	for (NumItem = 0, h = Hid; h; h = (Column *) h->Prev())NumItem++;
	FarMenuItem *fm = new FarMenuItem[NumItem]; if (!fm) { ShowError(4); return 1; }
	fm[0].Selected = 1;
	for (i = 0, h = Hid; h; i++, h = (Column *) h->Prev()) lstrcpy(fm[i].Text, h->name);
	i = Info.Menu(Info.ModuleNumber, -1, -1, sh - 6, FMENU_WRAPMODE, GetMsg(mColIns),
				  NULL, "Functions", NULL, NULL, fm, NumItem);
	delete[] fm; if (i < 0) return 1;
	for (h = Hid; i; i--, h = (Column *) h->Prev());
	h->Extract(); if (!h->Next()) Hid = (Column *) h->Prev();
	--HidNum; ShowStatus(4); coCurr->Before(h);
	if (coCurr == coFirst) { coFirst = h; curX = 0; coCurr = h; Xcur = 2; }
	return 0;
}
//===========================================================================

WORD LOOK::ClmnMove(char d)
{
	if (d == 'L' && !curX) return 1;  // Cannot move first column left
	if (!coFirst->Next()) return 1; // Cannot move alone column
	Column *h;
	if (d == 'R' && coCurr == coLast) return 1;  // Cannot move last column right
	coCurr->Extract();
	if (d == 'L') {
		h = (Column *) coCurr->Prev(); h->Before(coCurr);
		if (curX == 1)coFirst = coCurr; if (coCurr == coLast) coLast = h;
		--curX; return 0;
	}
	h = (Column *) coCurr->Next(); h->After(coCurr);
	if (!curX)coFirst = h; if (h == coLast) coLast = coCurr;
	++curX; return 0;
}
//===========================================================================

WORD LOOK::ClmnNarrow(void)
{
	if (coCurr->wid < 3) return 1; // Cannot narrow minimum width column
	--coCurr->wid; db.FiNum(coCurr->finum);
	if (coCurr->wid > db.FiWidth()) return 0;
	if (db.Numeric())coCurr->pos++;
	return 0;
}
//===========================================================================

WORD LOOK::ClmnEnlarge(void)
{
	if (coCurr->wid + 5 > sw) return 1; // Cannot enlarge maximum width column
	if (Xcur + coCurr->wid + 3 > sw) return 1;
	++coCurr->wid; if (!coCurr->pos) return 0;
	coCurr->pos--;
	return 0;
}
//===========================================================================

WORD LOOK::ClmnRename(void)
{
	int i;
	FarDialogItem di[5];
	ZeroMemory(di, sizeof(di));
	di[0].Type = DI_DOUBLEBOX; di[0].X1 = 3; di[0].X2 = 31; di[0].Y1 = 1; di[0].Y2 = 5;
	lstrcpy(di[0].Data, GetMsg(mColName));
	di[1].Type = DI_FIXEDIT; di[1].X1 = 5; di[1].X2 = 29; di[1].Y1 = 2; di[1].Focus = true;
	if (Yes(WinCode)) ToOem(coCurr->name, di[1].Data);
	else lstrcpy(di[1].Data, coCurr->name);
	di[2].Type = DI_TEXT; di[2].Y1 = 3; di[2].Flags = DIF_SEPARATOR;
	di[3].Type = DI_BUTTON; di[3].Flags = DIF_CENTERGROUP; di[3].Y1 = 4;
	di[3].DefaultButton = 1; lstrcpy(di[3].Data, GetMsg(mButOK));
	di[4].Type = DI_BUTTON; di[4].Flags = DIF_CENTERGROUP; di[4].Y1 = 4;
	lstrcpy(di[4].Data, GetMsg(mButCancel));
	i = Info.Dialog(Info.ModuleNumber, -1, -1, 35, 7, "Functions", di, 5);
	if (i != 3)return 1;
	di[1].Data[25] = 0;
	if (Yes(WinCode))ToAlt(di[1].Data, coCurr->name);
	else lstrcpy(coCurr->name, di[1].Data);
	return 0;
}
//===========================================================================

WORD LOOK::ClmnScroll(short d)
{
	db.FiNum(coCurr->finum);
	if (coCurr->wid > db.FiWidth()) return 1;  // Cannot scroll if width is enough
	if (!d) {                       // Scroll to the left end
		if (!coCurr->pos) return 1;
		coCurr->pos = 0;
		return 0;
	}
	if (d > 0) {
		if (db.FiWidth() - coCurr->pos <= coCurr->wid - 1) return 1;
		if (d == coCurr->wid) coCurr->pos = db.FiWidth() - coCurr->wid + 1; // To the right end
		else coCurr->pos++;                 // Scroll right by one step
		return 0;
	}
	if (!coCurr->pos) return 1;
	coCurr->pos--;
	return 0;
}
//===========================================================================

extern char *RootKey;

void LOOK::GetConfig(bool inrun)
{
	char u[256];
	HKEY hKey;
	DWORD val, sz;
	LONG rz;

	RegCreateKeyEx(HKEY_CURRENT_USER, RootKey, 0, NULL,
				   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &sz);
	if (sz == REG_CREATED_NEW_KEY) {
		val = Yes(WinCode) ? 1 : 0; sz = 4;    //-----> Кодировка по умолчанию
		RegSetValueEx(hKey, "CodeDefault", 0, REG_DWORD, (BYTE*) (&val), sz);
		val = Yes(LineNums) ? 1 : 0; sz = 4;    //-----> Показывать номера записей
		RegSetValueEx(hKey, "ShowRecNum", 0, REG_DWORD, (BYTE*) (&val), sz);
		val = Yes(FullMemo) ? 1 : 0; sz = 4;    //-----> Показывать мемо на весь экран
		RegSetValueEx(hKey, "ShowMemoFull", 0, REG_DWORD, (BYTE*) (&val), sz);
		val = 0; sz = 4;         //-----> Автоматически сохранять шаблон
		RegSetValueEx(hKey, "AutoSave", 0, REG_DWORD, (BYTE*) (&val), sz);
		u[0] = Find.Mask[0]; u[1] = Find.Mask[1]; //-----> Маскирующие символы
		u[2] = 0; sz = 3;
		if (sz)RegSetValueEx(hKey, "MaskChar", 0, REG_SZ, (BYTE*) u, sz);
		u[0] = 0; sz = 1;
		if (sz)RegSetValueEx(hKey, "Colors", 0, REG_SZ, (BYTE*) u, sz);
		val = 0; sz = 4;    //-----> Таблица по умолчанию
		RegSetValueEx(hKey, "CodeTable", 0, REG_DWORD, (BYTE*) (&val), sz);
		sz = lstrlen(DefMemExt) + 1;  //-----> Расширения мемо-файла
		RegSetValueEx(hKey, "MemExt", 0, REG_SZ, (BYTE*) DefMemExt, sz);
		db.OpenMemo(FileName, DefMemExt);
		sz = lstrlen(DefD) + 1;  //-----> Формат даты
		RegSetValueEx(hKey, "D_fmt", 0, REG_SZ, (BYTE*) DefD, sz);
		RegSetValueEx(hKey, "D_fmt_E", 0, REG_SZ, (BYTE*) DefD, sz);
		RegSetValueEx(hKey, "D_fmt_V", 0, REG_SZ, (BYTE*) DefD, sz);
		sz = lstrlen(DefT) + 1;  //-----> Формат даты-времени
		RegSetValueEx(hKey, "T_fmt", 0, REG_SZ, (BYTE*) DefT, sz);
		RegSetValueEx(hKey, "T_fmt_E", 0, REG_SZ, (BYTE*) DefT, sz);
		RegSetValueEx(hKey, "T_fmt_V", 0, REG_SZ, (BYTE*) DefT, sz);
		DTs2f(DefD, fmtD); DTs2f(DefT, fmtT);
		DTs2f(DefD, fmtDE); DTs2f(DefT, fmtTE);
		DTs2f(DefD, fmtDV); DTs2f(DefT, fmtTV);
	}
	else {
		if (inrun || Yes(WinCode)) {
			sz = sizeof(val);
			rz = RegQueryValueEx(hKey, "CodeDefault", NULL, NULL, (BYTE*) (&val), &sz);
			if (rz == ERROR_SUCCESS) { if (val == 1) Set(WinCode); else Clear(WinCode); }
			else {
				val = Yes(WinCode) ? 1 : 0; sz = 4;    //-----> Кодировка по умолчанию
				RegSetValueEx(hKey, "CodeDefault", 0, REG_DWORD, (BYTE*) (&val), sz);
			}
		}
		if (inrun || No(LineNums)) {
			sz = sizeof(val);
			rz = RegQueryValueEx(hKey, "ShowRecNum", NULL, NULL, (BYTE*) (&val), &sz);
			if (rz == ERROR_SUCCESS) { if (val) Set(LineNums); else Clear(LineNums); }
			else {
				val = Yes(LineNums) ? 1 : 0; sz = 4;    //-----> Показывать номера записей
				RegSetValueEx(hKey, "ShowRecNum", 0, REG_DWORD, (BYTE*) (&val), sz);
			}
		}
		if (inrun || No(FullMemo)) {
			sz = sizeof(val);
			rz = RegQueryValueEx(hKey, "ShowMemoFull", NULL, NULL, (BYTE*) (&val), &sz);
			if (rz == ERROR_SUCCESS) { if (val) Set(FullMemo); else Clear(FullMemo); }
			else {
				val = Yes(FullMemo) ? 1 : 0; sz = 4;    //-----> Показывать мемо на весь экран
				RegSetValueEx(hKey, "ShowMemoFull", 0, REG_DWORD, (BYTE*) (&val), sz);
			}
		}
		sz = 64; u[0] = 0; Exp.Sep[0] = Exp.Sep[1] = 0;
		rz = RegQueryValueEx(hKey, "ExpSep", NULL, NULL, (BYTE*) u, &sz);
		if (rz == ERROR_SUCCESS) { Exp.Sep[0] = u[0]; if (sz > 1)Exp.Sep[1] = u[1]; }
		if (!Exp.Sep[0]) {
			Exp.Sep[0] = DefExpSep; Exp.Sep[1] = 0; sz = 2;   //-----> Разделитель экспорта
			if (sz)RegSetValueEx(hKey, "ExpSep", 0, REG_SZ, (BYTE*) Exp.Sep, sz);
		}
		sz = sizeof(val);
		rz = RegQueryValueEx(hKey, "AutoSave", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS) { if (val) Set(AutoSave); else Clear(AutoSave); }
		else {
			val = 0; sz = 4;         //-----> Автоматически сохранять шаблон
			RegSetValueEx(hKey, "AutoSave", 0, REG_DWORD, (BYTE*) (&val), sz);
			Clear(AutoSave);
		}
		sz = 64; u[0] = 0; Find.Mask[0] = Find.Mask[1] = 0;
		rz = RegQueryValueEx(hKey, "MaskChar", NULL, NULL, (BYTE*) u, &sz);
		if (rz == ERROR_SUCCESS) {
			Find.Mask[0] = u[0];
			Find.Mask[1] = u[1];
		}
		if (sz < 2) {            //-----> Маскирующие символы
			Find.Mask[0] = '?'; Find.Mask[1] = '%'; sz = 3;
			RegSetValueEx(hKey, "MaskChar", 0, REG_SZ, (BYTE*) "?%", sz);
		}
		sz = 256; u[0] = 0;       //-----> Пользовательские цвета
		rz = RegQueryValueEx(hKey, "Colors", NULL, NULL, (BYTE*) u, &sz);
		if (rz == ERROR_SUCCESS) u[sz] = 0;
		else u[0] = 0;
		DefColors(u);
		sz = sizeof(val);       //-----> Кодовая таблица
		rz = RegQueryValueEx(hKey, "CodeTable", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS) TableSet(val);
		else {
			val = 0; sz = 4;
			RegSetValueEx(hKey, "CodeTable", 0, REG_DWORD, (BYTE*) (&val), sz);
		}

		sz = 256; u[0] = 0;       //-----> Расширения мемо-файлов
		rz = RegQueryValueEx(hKey, "MemExt", NULL, NULL, (BYTE*) u, &sz);
		if (rz == ERROR_SUCCESS) { u[sz] = 0; FSF.LUpperBuf(u, sz); }
		else {
			lstrcpy(u, DefMemExt);
			sz = lstrlen(u + 1);
			RegSetValueEx(hKey, "MemExt", 0, REG_SZ, (BYTE*) u, sz);
		}
		db.OpenMemo(FileName, u);

		sz = DTFmtL - 1; u[0] = 0;   //-----> Ввод даты
		RegQueryValueEx(hKey, "D_fmt", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtD);
		else {
			sz = lstrlen(DefD) + 1;
			RegSetValueEx(hKey, "D_fmt", 0, REG_SZ, (BYTE*) DefD, sz);
			DTs2f(DefD, fmtD);
		}
		sz = DTFmtL - 1; u[0] = 0;   //-----> Экспорт даты
		RegQueryValueEx(hKey, "D_fmt_E", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtDE);
		else {
			sz = lstrlen(DefD) + 1;
			RegSetValueEx(hKey, "D_fmt_E", 0, REG_SZ, (BYTE*) DefD, sz);
			DTs2f(DefD, fmtDE);
		}
		sz = DTFmtL - 1; u[0] = 0;   //-----> Показ даты
		RegQueryValueEx(hKey, "D_fmt_V", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtDV);
		else {
			sz = lstrlen(DefD) + 1;
			RegSetValueEx(hKey, "D_fmt_V", 0, REG_SZ, (BYTE*) DefD, sz);
			DTs2f(DefD, fmtDE);
		}
		sz = DTFmtL - 1; u[0] = 0;   //-----> Ввод даты-времени
		RegQueryValueEx(hKey, "T_fmt", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtT);
		else {
			sz = lstrlen(DefT) + 1;
			RegSetValueEx(hKey, "T_fmt", 0, REG_SZ, (BYTE*) DefT, sz);
			DTs2f(DefT, fmtT);
		}
		sz = DTFmtL - 1; u[0] = 0;   //-----> Экспорт даты-времени
		RegQueryValueEx(hKey, "T_fmt_E", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtTE);
		else {
			sz = lstrlen(DefT) + 1;
			RegSetValueEx(hKey, "T_fmt_E", 0, REG_SZ, (BYTE*) DefT, sz);
			DTs2f(DefT, fmtTE);
		}
		sz = DTFmtL - 1; u[0] = 0;   //-----> Показ даты-времени
		RegQueryValueEx(hKey, "T_fmt_V", NULL, NULL, (BYTE*) u, &sz);
		if (sz > DTFmtL - 1)sz = DTFmtL - 1; u[sz] = 0;
		if (u[0])DTs2f(u, fmtTV);
		else {
			sz = lstrlen(DefT) + 1;  //-----> Формат экспорта даты-времени
			RegSetValueEx(hKey, "T_fmt_V", 0, REG_SZ, (BYTE*) DefT, sz);
			DTs2f(DefT, fmtTV);
		}
	}
	RegCloseKey(hKey);
	DTf29(fmtD, (char*) D_Mask); DTf29(fmtT, (char*) T_Mask);
	db.fmtD = fmtDV; db.fmtT = fmtTV;
}
//===========================================================================

void LOOK::NameTemplate(char *nm)
{
	int i;
	//------- Construct template file name
	lstrcpy(nm, FileName); i = lstrlen(nm) - 1;
	while (i) {  // Remove extension, if it exist
		if (nm[i] == '\\') break;
		if (nm[i] == '.') { nm[i] = 0; break; }
		--i;
	}
	lstrcat(nm, ".tmp");
}
//===========================================================================

void LOOK::DelTemplate(void)
{
	char TemName[MAX_PATH];
	NameTemplate(TemName);
	DeleteFile(TemName);
}
//===========================================================================

void LOOK::GetTemplate(void)
{
	bool ChangeCol;
	int i;
	__int64 val;
	RecHead rh;
	HANDLE ef;
	char TemName[MAX_PATH];
	Column *c, cu;
	NameTemplate(TemName);
	ef = CreateFile(TemName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ef == INVALID_HANDLE_VALUE) return;
	ChangeCol = false;
	while (!MyRead(ef, &rh, sizeof(RecHead))) {
		switch (rh.Kind) {
		case 0: //------ Number of fields
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			if (val != __int64(db.nfil)) goto BAD_READ;
			continue;
		case 1: //------ Start Column Change
			ChangeCol = true; for (i = 0; i < db.nfil; i++)C[i].Clear();
			Hid = coLast = coCurr = coFirst = NULL; continue;
		case 2: //------ Start Visible Columns
			if (MyRead(ef, &cu, sizeof(Column))) goto BAD_READ;
			if (coFirst) c = (Column *) (c->Add(C + cu.finum));
			else c = coFirst = C + cu.finum;
			c->Put(&cu);
			continue;
		case 3: //------ Start Hidden Columns
			if (MyRead(ef, &cu, sizeof(Column))) goto BAD_READ;
			Hid = (Column *) (Hid->Add(C + cu.finum));
			Hid->Put(&cu);
			continue;
		case 4: //------ Set curX
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			curX = val;
			continue;
		case 5: //------ Set recV[0]
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			recV[0] = val;
			continue;
		case 6: //------ Set curY
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			curY = val;
			continue;
		case 7: //------ Set FindData
			if (MyRead(ef, &Find, sizeof(FindData))) goto BAD_READ;
			continue;
		case 8: //------ Set CondData
			if (MyRead(ef, &Find, sizeof(CondData))) goto BAD_READ;
			continue;
		case 9: //------ Set Flags
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			Flags = val;
			continue;
		case 10: //------ Set edit date format
			if (MyRead(ef, fmtD, DTFmtL)) goto BAD_READ;
			DTf29(fmtD, (char*) D_Mask);
			continue;
		case 11: //------ Set edit time format
			if (MyRead(ef, fmtT, DTFmtL)) goto BAD_READ;
			DTf29(fmtT, (char*) T_Mask);
			continue;
		case 12: //------ Set export date format
			if (MyRead(ef, fmtDE, DTFmtL)) goto BAD_READ;
			continue;
		case 13: //------ Set export time format
			if (MyRead(ef, fmtTE, DTFmtL)) goto BAD_READ;
			continue;
		case 14: //------ Set view date format
			if (MyRead(ef, fmtDV, DTFmtL)) goto BAD_READ;
			continue;
		case 15: //------ Set view time format
			if (MyRead(ef, fmtTV, DTFmtL)) goto BAD_READ;
			continue;
		case 16: //------ Code Table
			if (MyRead(ef, &val, 8)) goto BAD_READ;
			TableSet(val);
			continue;
		case 9999: //------ End of restoring
			goto STOP_READ;
		default: //------ Unknown record skip
			SetFilePointer(ef, rh.Len, NULL, FILE_CURRENT);
			continue;
		}
	}
BAD_READ:
	CloseHandle(ef);
	DeleteFile(TemName);
	if (ChangeCol) { InitColumns(); recV[0] = 1; curX = curY = 0; }
	Flags = FlagsEntry;
	return;
STOP_READ:
	CloseHandle(ef);
}
//===========================================================================

void LOOK::PutTemplate(bool msg)
{
	RecHead rh;
	HANDLE ef;
	__int64 val;
	char TemName[MAX_PATH];
	Column *c, cu;
	NameTemplate(TemName);
	ef = CreateFile(TemName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (ef == INVALID_HANDLE_VALUE) { if (msg)ShowError(12); return; }
	//------ Number of fields
	rh.Kind = 0; rh.Len = 8;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	val = db.nfil;
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ Start Columns Data
	rh.Kind = 1; rh.Len = 0;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	//------ Start Visible Columns
	rh.Kind = 2; rh.Len = sizeof(Column);
	for (c = coFirst; c; c = (Column *) c->Next()) {
		if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
		if (MyWrite(ef, c, sizeof(Column))) goto BAD_WRITE;
	}
	//------ Start Hidden Columns
	rh.Kind = 3;
	for (c = Hid; c; c = (Column *) c->Prev()) {
		if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
		if (MyWrite(ef, c, sizeof(Column))) goto BAD_WRITE;
	}
	//------ Store curX
	rh.Kind = 4; rh.Len = 8;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	val = curX;
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ Store recV[0]
	rh.Kind = 5; rh.Len = 8;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	val = recV[0];
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ Store curY
	rh.Kind = 6; rh.Len = 8;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	val = curY;
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ Store FindData
	rh.Kind = 7; rh.Len = sizeof(FindData);
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, &Find, sizeof(FindData))) goto BAD_WRITE;
	//------ Store CondData
	rh.Kind = 8; rh.Len = sizeof(CondData);
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, &Cond, sizeof(CondData))) goto BAD_WRITE;
	//------ Store Flags
	rh.Kind = 9; rh.Len = 8; val = Flags;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ Store edit date format
	rh.Kind = 10; rh.Len = DTFmtL;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtD, DTFmtL)) goto BAD_WRITE;
	//------ Store edit time format
	rh.Kind = 11;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtT, DTFmtL)) goto BAD_WRITE;
	//------ Store export date format
	rh.Kind = 12;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtDE, DTFmtL)) goto BAD_WRITE;
	//------ Store export time format
	rh.Kind = 13;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtTE, DTFmtL)) goto BAD_WRITE;
	//------ Store view date format
	rh.Kind = 14;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtDV, DTFmtL)) goto BAD_WRITE;
	//------ Store export time format
	rh.Kind = 15;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	if (MyWrite(ef, fmtTV, DTFmtL)) goto BAD_WRITE;
	//------ Code Table Number
	rh.Kind = 16; rh.Len = 8;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	val = ctsNum;
	if (MyWrite(ef, &val, 8)) goto BAD_WRITE;
	//------ End of template storing
	rh.Kind = 9999; rh.Len = 0;
	if (MyWrite(ef, &rh, sizeof(RecHead))) goto BAD_WRITE;
	CloseHandle(ef);
	if (!msg)return;
	const TCHAR *MsgItems[] = { Title, GetMsg(mTemplate), TEXT("\x01") };
	Info.Message(Info.ModuleNumber, FMSG_MB_OK, TEXT("Config"), MsgItems, ARRAYSIZE(MsgItems), 0);
	return;
BAD_WRITE:
	CloseHandle(ef);
	if (msg)ShowError(12);
}
//===========================================================================

static int KeyNum(MOUSE_EVENT_RECORD *m)
{
	return m->dwMousePosition.X / 8 + 1;
}
//===========================================================================

static LONG_PTR WINAPI DiProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	short i;
	WORD Q;
	DWORD k, r_save;
	switch (Msg) {
	case DN_INITDIALOG:
		Info.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
		return 1;
	case DN_RESIZECONSOLE:
	{
		FarDialogItem *di = (FarDialogItem *) Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
		COORD *coo = (COORD *) Param2;
		Q = coo->Y - 1; if (data->sh == Q && data->sw == coo->X) return 1;
		i = data->sh - Q;
		if (i > 0 && data->curY > Q - 3) { data->recV[0] = data->recV[i]; data->curY -= i; }
		data->sh = Q;
		data->sw = coo->X;  data->Xcur += data->coCurr->wid;
		while (data->Xcur > data->sw && data->coCurr != data->coFirst) {
			data->Xcur -= data->coFirst->wid; data->curX--;
			data->coFirst = (Column *) (data->coFirst->Next());
		}
		r_save = data->recV[0];
		if (data->BuildBuffers(true)) {
			data->ShowError(4);
			Info.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
			return 1;
		}
		data->recV[0] = r_save;
		di[0].X2 = data->sw - 1; di[0].VBuf = data->VBuf;
		data->ShowStatus(0);
		di[1].X2 = data->sw - 1; di[1].VBuf = data->VBuf + data->sw;
		data->AttrLine(0, data->sw, 1, data->at[2]);
		di[2].X2 = data->sw - 1; di[2].Y2 = data->sh - 1;
		di[2].VBuf = data->VBuf + data->sw * 2;
		data->AttrRect(0, 2, data->sw, data->sh - 2, data->at[3]);
		data->ShowPage();
		di[3].X2 = data->sw - 1; di[3].Y1 = data->sh; di[3].Y2 = data->sh;
		di[3].VBuf = data->VBuf + data->sw*data->sh; data->KeyShow();
		for (i = 0; i < 4; i++) Info.SendDlgMessage(hDlg, DM_SETDLGITEM, i, (LONG_PTR) (di + i));
		coo->X = coo->Y = 0;
		Info.SendDlgMessage(hDlg, DM_MOVEDIALOG, true, (LONG_PTR) coo);
		coo->X = data->sw; coo->Y = data->sh + 1;
		Info.SendDlgMessage(hDlg, DM_RESIZEDIALOG, 0, (LONG_PTR) coo);
		Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
		return 1;
	}
	case DN_MOUSECLICK:
		switch (Param1) {
		case 0: case 1: // Special Navigation
			if (data->TopMouse(Param1, (MOUSE_EVENT_RECORD *) Param2)) return 1;
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		case 2:  // Navigation
			if (data->GoMouse((MOUSE_EVENT_RECORD *) Param2)) return 1;
			Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
			return 1;
		case 3:  // KeyBar Click
			i = KeyNum((MOUSE_EVENT_RECORD *) Param2);
			Info.SendDlgMessage(hDlg, DM_LOOKDBF, i, 0);
			return 1;
		default: return 1;
		}
	case DM_LOOKDBF:
		switch (Param1) {
		case 1:
			Info.ShowHelp(Info.ModuleName, "Functions", FHELP_SELFHELP);
			return 1;
		case 2:
			data->ShowFields();
			return 1;
		case 3:
			if (!data->Find.Len) return 1;
			if (data->GoFind()) return 1;
			break;
		case 4:
			if (data->EditRecord())  return 1;
			data->ShowPage();
			break;
		case 5:         // Change mode. Show all <-> marked only
			if (!data->MarkNum) return 1;
			data->MarkOnly = 1 - data->MarkOnly;
			data->MarkFindFirst(); data->ShowF5();
			break;
		case 6:  // F6 Show sums of marked records
			data->ShowSum(); return 1;
		case 7:
			i = data->FindAskSample();
			switch (i) {
			case 0: return 1;
			case 1: if (data->GoFind()) return 1;
				break;
			case 2: if (data->FindMark(1)) data->ShowF5();
				break;
			case 3: if (data->FindMark(0)) data->ShowF5();
			}
			break;
		case 8:
			data->ChangeCode(); break;
		case 9:         // Export
			if (data->Export()) return 1;
			k = GetCurrentTime();
			switch (data->Exp.Form) {
			case 0: if (data->ExpTxt()) return 1; break;
			case 1: if (data->ExpHtm()) return 1; break;
			case 2: if (data->ExpDbf()) return 1;
			}
			data->ShowExpMsg(GetCurrentTime() - k);
			return 1;
		case 10:
			Info.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
			return 1;
		default: return 1;
		}
		Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
		return 1;

	case DN_KEY:
		switch (Param2) {
		case KEY_UP:
			Q = data->GoUp(1); goto Q_EXEC;
		case KEY_MSWHEEL_UP:
			Q = data->GoUp(data->sh >> 2); goto Q_EXEC;
		case KEY_PGUP:
			Q = data->GoUp(data->sh - 3); goto Q_EXEC;
		case KEY_CTRLPGUP:
			Q = data->GoTop(); goto Q_EXEC;
		case KEY_DOWN:
			Q = data->GoDn(1); goto Q_EXEC;
		case KEY_MSWHEEL_DOWN:
			Q = data->GoDn(data->sh >> 2); goto Q_EXEC;
		case KEY_PGDN:
			Q = data->GoDn(data->sh - 3); goto Q_EXEC;
		case KEY_CTRLPGDN:
			Q = data->GoBot(); goto Q_EXEC;
		case KEY_CTRLEND:
			Q = data->GoBot() | data->GoLast(); goto Q_EXEC;
		case KEY_LEFT:
			Q = data->GoLeft(); goto Q_EXEC;
		case KEY_HOME:
			Q = data->GoFirst(); goto Q_EXEC;
		case KEY_CTRLHOME:
			Q = data->GoTop() | data->GoFirst(); goto Q_EXEC;
		case KEY_RIGHT:
			Q = data->GoRight(); goto Q_EXEC;
		case KEY_END:
			Q = data->GoLast();  goto Q_EXEC;
		case KEY_TAB:
			Q = data->GoRight(); if (Q) goto Q_EXEC;
			Q = data->GoDn(1);   if (!Q) return 1;
			Q |= data->GoFirst(); goto Q_EXEC;
		case KEY_SHIFTTAB:
			Q = data->GoLeft(); if (Q) goto Q_EXEC;
			Q = data->GoUp(1);  if (!Q) return 1;
			Q |= data->GoLast();

Q_EXEC: //---------------- Execute Navigation Result-----------
			if (!Q) return 1;
			if (Q - 1)data->ShowPage(); else data->ShowCur();
			break;

			//-------------------------------------------------- Operations with Marks
		case KEY_INS: case KEY_SPACE: case KEY_NUMPAD0:   // Mark/Unmark record
			if (data->MarkAlloc()) return 1;
			data->MarkInvert(data->recV[data->curY]);
			if (!data->MarkOnly) data->GoDn(1);
			data->ShowF5();
			break;
		case KEY_SHIFTDOWN:   // Mark record
			if (data->MarkAlloc()) return 1;
			data->MarkSet(data->recV[data->curY]);
			data->GoDn(1); data->ShowF5();
			break;
		case KEY_SHIFTUP:   // Mark record
			if (data->MarkAlloc()) return 1;
			data->MarkSet(data->recV[data->curY]);
			data->GoUp(1); data->ShowF5();
			break;
		case KEY_ADD:       // "Gray +" Mark all records
			if (data->MarkAlloc()) return 1;
			data->MarkSetAll(); data->ShowF5(); break;
		case KEY_SUBTRACT:       // "Gray -" Unmark all records
			if (data->MarkAlloc()) return 1;
			data->MarkClearAll(); data->ShowF5(); break;
		case KEY_MULTIPLY:       // "Gray *" Invert mark of all records
			if (data->MarkAlloc()) return 1;
			data->MarkInvertAll(); data->ShowF5(); break;
		case KEY_CTRLMULTIPLY:  // Ctrl+"Gray *" Mark of nonactual records
			if (data->MarkAlloc()) return 1;
			data->MarkNonActual(); data->MarkFindFirst();
			data->ShowF5(); break;
		case KEY_CTRLDOWN:    // Go to next marked record
			Q = data->GoNextMark(); goto Q_EXEC;
		case KEY_CTRLUP:      // Go to previous marked record
			Q = data->GoPrevMark(); goto Q_EXEC;
		case KEY_F6:       // F6 Show sums of marked records
			data->ShowSum(); return 1;

			//------------------------------------------------- Column Manipulations
		case KEY_SHIFTDEL:  // Hide column
			if (data->ClmnDelete()) return 1;
			data->ShowPage(); break;
		case KEY_SHIFTINS:  // Show column
			if (data->ClmnInsert()) return 1;
			data->ShowPage(); break;
		case KEY_CTRLLEFT:  // Move left
			if (data->ClmnMove('L')) return 1;
			data->ShowPage(); break;
		case KEY_CTRLRIGHT:  // Move right
			if (data->ClmnMove('R')) return 1;
			data->ShowPage(); break;
		case KEY_SHIFTRIGHT:  // Enlarge column
			if (data->ClmnEnlarge()) return 1;
			data->ShowPage(); break;
		case KEY_SHIFTLEFT:  // Narrow column
			if (data->ClmnNarrow()) return 1;
			data->ShowPage(); break;
		case KEY_ALTLEFT:  // Scroll column left
			if (data->ClmnScroll(-1)) return 1;
			data->ShowPage(); break;
		case KEY_ALTRIGHT:  // Scroll column right
			if (data->ClmnScroll(1)) return 1;
			data->ShowPage(); break;
		case KEY_ALTHOME:  // Scroll to the left end
			if (data->ClmnScroll(0)) return 1;
			data->ShowPage(); break;
		case KEY_ALTEND:  // Scroll to the right end
			if (data->ClmnScroll(data->coCurr->wid)) return 1;
			data->ShowPage(); break;
		case KEY_SHIFTENTER:  // Rename column
			if (data->ClmnRename()) return 1;
			data->ShowPage(); break;

			//------------------------------------------------- Other operations
		case KEY_CTRLINS: case KEY_CTRLC:      // Copy to clipboard
			data->Clipboard(); return 1;

		case KEY_DEL:          // Change record actuality
			if (data->LookOnly) { data->ShowError(8); return 1; }
			if (data->ActualRecord()) return 1;
			break;

		case KEY_CTRLDEL:      // Set current field to Null
			if (data->LookOnly) return 1;
			if (data->EditField(1))  return 1;
			data->ShowPage();
			break;

		case KEY_CTRLD:          // Make selected records non-actual
			if (data->LookOnly) { data->ShowError(8); return 1; }
			if (!data->MarkNum) { data->ShowError(5); return 1; }
			if (data->ActualSelected('*')) return 1;
			break;

		case KEY_CTRLI:          // Make selected records actual
			if (data->LookOnly) { data->ShowError(8); return 1; }
			if (!data->MarkNum) { data->ShowError(5); return 1; }
			if (data->ActualSelected(' ')) return 1;
			break;

		case KEY_ALTF1:          // Configure
			Configure(-1); data->GetConfig(true);
			data->KeyShow(); data->ShowStatus(-1);
			data->ShowPage();
			break;

		case KEY_F2:          // Show structure
			i = data->ShowFields(); if (i < 0) return 1;
			if (data->GoField(i)) return 1;
			break;

		case KEY_SHIFTF2:     // Edit Header
			if (data->LookOnly) { data->ShowError(8); return 1; }
			data->EditHeader();
			return 1;

		case KEY_CTRLF2:         // Pack DBF
			if (data->LookOnly) { data->ShowError(8); return 1; }
			i = data->PackDbf();
			if (!i) break;
			if (i == 2) Info.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
			return 1;


		case KEY_F4: // Edit Record. If current field is Memo, show it by editor.
			if (data->CurType() == 'M') { data->ShowMemo(1); break; }
			if (data->EditRecord())  return 1;
			data->ShowPage();
			break;

		case KEY_ENTER:   // Edit current field
			if (data->LookOnly) return 1;
			if (data->EditField())  return 1;
			data->ShowPage();
			break;

		case KEY_F5:         // Change mode. Show all <-> marked only
			if (!data->MarkNum) return 1;
			data->MarkOnly = 1 - data->MarkOnly;
			data->MarkFindFirst(); data->ShowF5();
			break;

		case KEY_SHIFTF6:         // Template store
			data->PutTemplate(true);
			return 1;

		case KEY_CTRLF6:         // Template delete
			data->DelTemplate();

		case KEY_ALTF6:         // Template release
			data->InitColumns();
			data->Flags = data->FlagsEntry;
			data->GetConfig();
			data->recV[0] = 1;
			data->ShowPage();
			break;

		case KEY_CTRLF7:  // Search and replace
			if (data->LookOnly) { data->ShowError(8); return 1; }
			i = data->ReplaceDlg();
			if (!i) return 1;
			k = 0;
			if (data->Yes(ConfReplace)) {
				data->GoTop();
				for (;;) {
					if (data->GoFind(i)) { data->ShowReplMsg(k); break; }
					Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
					switch (data->ReplaceAsk()) {
					case 1: if (data->Replace())break;            // Yes
						data->ShowPage();  ++k;
						continue;
					case 2: data->Find.Step = lstrlen(data->Find.FU); // No
						continue;
					case 3: data->ReplaceAll(i, k);               // All
					}
					break;
				}
			}
			else data->ReplaceAll(i, 0);
			data->ShowPage();
			data->Find.Step = data->Find.Pos = 0;
			data->Clear(FindReplace);
			break;

		case KEY_ALTF7:     // Search using conditions
			switch (data->CondAsk()) {
			case 0: return 1;
			case 1: if (data->GoFind()) return 1;
				break;
			case 2: if (data->FindMark(1)) data->ShowF5();
				break;
			case 3: if (data->FindMark(0)) data->ShowF5();
			}
			break;

		case KEY_F7: case KEY_CTRLF:    // Search using sample
			switch (data->FindAskSample()) {
			case 0: return 1;
			case 1: if (data->GoFind()) return 1;
				break;
			case 2: if (data->FindMark(1)) data->ShowF5();
				break;
			case 3: if (data->FindMark(0)) data->ShowF5();
			}
			break;

		case KEY_F3:   // If current field is Memo, show it by viewer.
			if (data->CurType() == 'M') { data->ShowMemo(0); break; }

		case KEY_SHIFTF7:  // Continue Searching
			if (!data->Find.Len && data->No(CondSearch)) return 1;
			if (data->GoFind()) return 1;
			break;

		case KEY_F8:         // Change code table
			data->ChangeCode(); break;

		case KEY_SHIFTF8:    // Select code table
			if (data->TableSelect()) return 1;
			break;

		case KEY_ALTF8:      // Goto given line number
			if (data->GoAsk()) return 1;
			break;

		case KEY_F9:         // Export
			if (data->Export()) return 1;
			k = GetTickCount();
			switch (data->Exp.Form) {
			case 0: if (data->ExpTxt()) return 1; break;
			case 1: if (data->ExpHtm()) return 1; break;
			case 2: if (data->ExpDbf()) return 1;
			}
			data->ShowExpMsg(GetTickCount() - k);
			return 1;

		case KEY_SHIFTF9:         // Import
			if (data->Import()) return 1;
			break;

		case KEY_SHIFTF10:         // Template store and exit
			if (data->No(AutoSave))data->PutTemplate();
			Info.SendDlgMessage(hDlg, DM_CLOSE, 0, 0);
			return 1;

		default: return 0;
		}
		Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
		return 1;
	}
	return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}
//===========================================================================

void LOOK::ShowDBF(void)
{
	int i;
	char u[64];

	i = db.Open(FileName, LookOnly);
	if (i & 0x0f) { ShowError(i); db.Close(); return; }
	if (i)LookOnly = 1;
	Find.Clear();
	Exp.Sep[0] = DefExpSep; Exp.Sep[1] = 0;
	aw = " Win";
	GetScreenSize(); GetConfig();
	if (!db.dbH.nrec) {
		if (LookOnly) { ShowFields(); db.Close(); return; }
		if (ShowFields() > -2) { db.Close(); return; }
		if (db.Append()) { ShowError(2); db.Close(); return; }
	}
	if (BuildBuffers()) { ShowError(4); db.Close(); return; }
	Set(ExpHeads); Set(ExpActual); Set(ExpSpaces); Set(ExpSeparator);
	FSF.sprintf(u, "%02X", db.dbH.type);
	Exp.Type[0] = u[0]; Exp.Type[1] = u[1];
	lstrcpy(Exp.File, FileName); i = lstrlen(Exp.File) - 1;
	while (i) {  // Remove extension, if it exist
		if (Exp.File[i] == '\\') break;
		if (Exp.File[i] == '.') { Exp.File[i] = 0; break; }
		--i;
	}
	FarDialogItem di[4];  ZeroMemory(di, sizeof(di));
	Exp.fi[0] = Exp.fi[1] = Exp.fi[2] = -1;
	Exp.BufLim = 256; if (Exp.BufLim > db.dbH.nrec)Exp.BufLim = db.dbH.nrec;
	coFirst = C; curX = 0; curY = 0; recV[0] = 1;
	FlagsEntry = Flags;
	GetTemplate();
	if (recV[0] > db.dbH.nrec) recV[0] = 1;
	if (curY >= db.dbH.nrec) curY = 0;

	ClearRect(0, 0, sw, sh + 1);

	di[0].Type = DI_USERCONTROL;
	di[0].X1 = 0; di[0].X2 = sw - 1; di[0].Y1 = 0; di[0].Y2 = 0;
	di[0].VBuf = VBuf; ShowStatus(0);

	di[1].Type = DI_USERCONTROL;
	di[1].X1 = 0; di[1].X2 = sw - 1; di[1].Y1 = 1; di[1].Y2 = 1;
	di[1].VBuf = VBuf + sw; AttrLine(0, sw, 1, at[2]);

	di[2].Type = DI_USERCONTROL;
	di[2].X1 = 0; di[2].X2 = sw - 1; di[2].Y1 = 2; di[2].Y2 = sh - 1;
	di[2].Focus = TRUE;
	di[2].VBuf = VBuf + sw * 2;
	AttrRect(0, 2, sw, sh - 2, at[3]);
	ShowPage();

	di[3].Type = DI_USERCONTROL;
	di[3].X1 = 0; di[3].X2 = sw - 1; di[3].Y1 = sh; di[3].Y2 = sh;
	di[3].VBuf = VBuf + sw*sh; KeyShow();

	Info.DialogEx(Info.ModuleNumber, 0, 0, sw - 1, sh, "Functions", di, 4, 0,
				  FDLG_SMALLDIALOG | FDLG_NODRAWPANEL, DiProc, (LONG_PTR) di);
	if (Yes(AutoSave))PutTemplate();
	if (VBuf) { delete VBuf; VBuf = NULL; }
	if (C) { delete[] C; C = NULL; }
	db.Close();
}
//===========================================================================

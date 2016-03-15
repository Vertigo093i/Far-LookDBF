#include "LookDBF.h"

PluginStartupInfo Info;
FarStandardFunctions FSF;
HANDLE LookHeap;
const char *Title = "LookDBF";
char *RootKey;

LOOK *data;

extern const char *DefD;
extern const char *DefT;
extern const char *DefMemExt;
//===========================================================================

/*
Функция GetMsg возвращает строку сообщения из языкового файла.
А это надстройка над Info.GetMsg для сокращения кода :-)
*/
const char *GetMsg(int MsgId)
{
	return Info.GetMsg(Info.ModuleNumber, MsgId);
}
//===========================================================================

int WINAPI _export GetMinFarVersion(void)
{
	return MAKEFARVERSION(1, 75, 2634); //FARMANAGERVERSION
}

/*
Функция SetStartupInfo вызывается один раз, перед всеми
другими функциями. Она передает плагину информацию,
необходимую для дальнейшей работы.
*/
void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	::FSF = *Info->FSF;
	::Info.FSF = &::FSF;

	RootKey = (TCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(Info->RootKey) + 8) * sizeof(TCHAR) + 1);
	lstrcpy(RootKey, Info->RootKey);
	FSF.AddEndSlash(RootKey);
	lstrcat(RootKey, TEXT("LookDBF"));
}
//===========================================================================

/*
Функция GetPluginInfo вызывается для получения основной
(general) информации о плагине
*/
void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
	static const char *PluginMenuStrings[1];
	pi->StructSize = sizeof(struct PluginInfo);
	PluginMenuStrings[0] = Title;
	pi->PluginMenuStrings = PluginMenuStrings;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = PluginMenuStrings;
	pi->PluginConfigStringsNumber = 1;
	pi->CommandPrefix = PluginMenuStrings[0];
}
//===========================================================================

int WINAPI _export Configure(int ItemNumber)
{
	int i, n;
	LONG rz;
	HKEY hKey;
	DWORD val, sz;
	CharTableSet cts;
	FarDialogItem di[34];
	FarList flc, flt;
	FarListItem fli[2];
	ZeroMemory(di, sizeof(di));
	ZeroMemory(fli, sizeof(fli));
	flc.Items = fli; flc.ItemsNumber = 2;
	lstrcpy(fli[0].Text, "DOS"); lstrcpy(fli[1].Text, "Win");
	if (ItemNumber >= 0) {
		LookHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
		if (!LookHeap)return false;
	}
	n = 1;
	while (Info.CharTable(n - 1, (char*) (&cts), sizeof(CharTableSet)) >= 0)n++;
	flt.Items = new FarListItem[n];
	flt.ItemsNumber = n;
	lstrcpy(flt.Items[0].Text, "Current Windows Code Table");
	for (i = 1; i < n; i++) {
		Info.CharTable(i - 1, (char*) (&cts), sizeof(CharTableSet));
		lstrcpy(flt.Items[i].Text, cts.TableName);
	}
	fli[1].Flags = LIF_SELECTED; //  Кодировка по умолчанию (Win)
	di[3].Selected = 0;          //  Показывать номера записей (нет)
	di[4].Selected = 0;          //  Показывать мемо на весь экран (нет)
	di[5].Data[0] = 0xb3;        //  Разделитель эксопрта
	di[7].Selected = 0;          //  Автоматически сохранять шаблон (нет)
	lstrcpy(di[8].Data, "?");   // маскирующий символ
	lstrcpy(di[14].Data, DefD); // Показ даты
	lstrcpy(di[15].Data, DefT); // Показ даты-времени
	lstrcpy(di[18].Data, DefD); // Ввод даты
	lstrcpy(di[19].Data, DefT); // Ввод даты-времени
	lstrcpy(di[22].Data, DefD); // Экспорт даты
	lstrcpy(di[23].Data, DefT); // Экспорт даты-времени
	di[26].Data[0] = 0;          // Цвета пользователя
	flt.Items[0].Flags = LIF_SELECTED; // Таблица по умолчанию - текущая Windows
	RegCreateKeyEx(HKEY_CURRENT_USER, RootKey, 0,
				   NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &sz);
	if (sz != REG_CREATED_NEW_KEY) {
		sz = sizeof(val);   //---------  Кодировка по умолчанию (Win)
		rz = RegQueryValueEx(hKey, "CodeDefault", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS) {
			if (!val) { fli[0].Flags = LIF_SELECTED; fli[1].Flags = 0; }
		}
		sz = sizeof(val);  //---------  Показывать номера записей (нет)
		rz = RegQueryValueEx(hKey, "ShowRecNum", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS) di[3].Selected = val;
		sz = sizeof(val);  //----------  Показывать мемо на весь экран (нет)
		rz = RegQueryValueEx(hKey, "ShowMemoFull", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS)di[4].Selected = val;
		sz = 3;            //----------- Разделитель экспорта
		rz = RegQueryValueEx(hKey, "ExpSep", NULL, NULL, (BYTE*) di[5].Data, &sz);
		if (rz != ERROR_SUCCESS) { di[5].Data[0] = 0xb3; di[5].Data[1] = 0; }
		sz = sizeof(val);  //-----------  Автоматически сохранять шаблон (нет)
		rz = RegQueryValueEx(hKey, "AutoSave", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS)di[7].Selected = val;
		sz = 3;            //----------- Маскирующие символы
		rz = RegQueryValueEx(hKey, "MaskChar", NULL, NULL, (BYTE*) di[6].Data, &sz);
		if (rz != ERROR_SUCCESS) { di[8].Data[0] = '?'; di[8].Data[1] = '%'; }
		if (!di[8].Data[1])di[8].Data[1] = '%'; di[8].Data[2] = 0;
		sz = 256;       //----------- Пользовательские цвета
		rz = RegQueryValueEx(hKey, "Colors", NULL, NULL, (BYTE*) di[26].Data, &sz);
		if (rz != ERROR_SUCCESS) di[26].Data[0] = 0;
		else di[26].Data[sz] = 0;
		sz = 256;       //----------- Расширения мемо-файла
		rz = RegQueryValueEx(hKey, "MemExt", NULL, NULL, (BYTE*) di[30].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[30].Data, DefMemExt);
		else di[30].Data[sz] = 0;
		sz = DTFmtL;       //----------- Показ даты
		rz = RegQueryValueEx(hKey, "D_fmt_V", NULL, NULL, (BYTE*) di[14].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[14].Data, DefD);
		else di[14].Data[sz] = 0;
		sz = DTFmtL;           //----------- Показ даты-времени
		rz = RegQueryValueEx(hKey, "T_fmt_V", NULL, NULL, (BYTE*) di[15].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[15].Data, DefT);
		else di[15].Data[sz] = 0;
		sz = DTFmtL;       //----------- Ввод даты
		rz = RegQueryValueEx(hKey, "D_fmt", NULL, NULL, (BYTE*) di[18].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[18].Data, DefD);
		else di[18].Data[sz] = 0;
		sz = DTFmtL;           //----------- Ввод даты-времени
		rz = RegQueryValueEx(hKey, "T_fmt", NULL, NULL, (BYTE*) di[19].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[19].Data, DefT);
		else di[19].Data[sz] = 0;
		sz = DTFmtL;       //----------- Экспорт даты
		rz = RegQueryValueEx(hKey, "D_fmt_E", NULL, NULL, (BYTE*) di[22].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[22].Data, DefD);
		else di[22].Data[sz] = 0;
		sz = DTFmtL;           //----------- Экспорт даты-времени
		rz = RegQueryValueEx(hKey, "T_fmt_E", NULL, NULL, (BYTE*) di[23].Data, &sz);
		if (rz != ERROR_SUCCESS) lstrcpy(di[23].Data, DefT);
		else di[23].Data[sz] = 0;
		sz = sizeof(val);   //---------  Таблица по умолчанию  Current Win
		rz = RegQueryValueEx(hKey, "CodeTable", NULL, NULL, (BYTE*) (&val), &sz);
		if (rz == ERROR_SUCCESS && val < n) {
			flt.Items[0].Flags = 0;
			flt.Items[val].Flags = LIF_SELECTED;
			if (val)lstrcpy(fli[0].Text, "Alt");
		}
	}
	i = 0; //0
	di[i].Type = DI_DOUBLEBOX; di[i].X1 = 3; di[i].X2 = 50;
	di[i].Y1 = 1; di[i].Y2 = 22;
	lstrcpy(di[i].Data, Title);
	++i; //1
	di[i].Type = DI_COMBOBOX; di[i].X1 = 5; di[i].Y1 = 2; di[i].X2 = 7;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &flc;
	++i; //2
	di[i].Type = DI_TEXT; di[i].X1 = 9; di[i].Y1 = 2;
	lstrcpy(di[i].Data, GetMsg(mCfgCodeDef));
	++i; //3
	di[i].Type = DI_CHECKBOX; di[i].X1 = 5; di[i].Y1 = 3;
	lstrcpy(di[i].Data, GetMsg(mCfgRecNum));
	++i; //4
	di[i].Type = DI_CHECKBOX; di[i].X1 = 5; di[i].Y1 = 4;
	lstrcpy(di[i].Data, GetMsg(mCfgMemo));
	++i; //5
	di[i].Type = DI_FIXEDIT; di[i].X1 = 5; di[i].X2 = 7; di[i].Y1 = 5;
	++i; //6
	di[i].Type = DI_TEXT; di[i].X1 = 9; di[i].Y1 = 5;
	lstrcpy(di[i].Data, GetMsg(mCfgSep));
	++i; //7
	di[i].Type = DI_CHECKBOX; di[i].X1 = 5; di[i].Y1 = 6;
	lstrcpy(di[i].Data, GetMsg(mCfgAutoSave));
	++i; //8
	di[i].Type = DI_FIXEDIT; di[i].X1 = 5; di[i].X2 = 7; di[i].Y1 = 7;
	++i; //9
	di[i].Type = DI_TEXT; di[i].X1 = 9; di[i].Y1 = 7;
	lstrcpy(di[i].Data, GetMsg(mReplMask));
	++i; //10
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 9;
	lstrcpy(di[i].Data, GetMsg(mExFormat)); lstrcat(di[i].Data, ":");
	++i; //11
	di[i].Type = DI_TEXT; di[i].X1 = 14; di[i].Y1 = 9;
	lstrcpy(di[i].Data, "D (Date)        T (DateTime)");

	++i; //12
	di[i].Type = DI_TEXT; di[i].Y1 = 10; di[i].Flags = DIF_SEPARATOR;
	++i; //13
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 11;
	lstrcpy(di[i].Data, GetMsg(mCfgShow));
	++i; //14
	di[i].Type = DI_EDIT; di[i].X1 = 13; di[i].X2 = di[i].X1 + 10; di[i].Y1 = 11;
	++i; //15
	di[i].Type = DI_EDIT; di[i].X1 = 25; di[i].X2 = di[i].X1 + 23; di[i].Y1 = 11;

	++i; //16
	di[i].Type = DI_TEXT; di[i].Y1 = 12; di[i].Flags = DIF_SEPARATOR;
	++i; //17
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 13;
	lstrcpy(di[i].Data, GetMsg(mCfgEdit));
	++i; //18
	di[i].Type = DI_EDIT; di[i].X1 = 13; di[i].X2 = di[i].X1 + 10; di[i].Y1 = 13;
	++i; //19
	di[i].Type = DI_EDIT; di[i].X1 = 25; di[i].X2 = di[i].X1 + 23; di[i].Y1 = 13;

	++i; //20
	di[i].Type = DI_TEXT; di[i].Y1 = 14; di[i].Flags = DIF_SEPARATOR;
	++i; //21
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 15;
	lstrcpy(di[i].Data, GetMsg(mExTitle));
	++i; //22
	di[i].Type = DI_EDIT; di[i].X1 = 13; di[i].X2 = di[i].X1 + 10; di[i].Y1 = 15;
	++i; //23
	di[i].Type = DI_EDIT; di[i].X1 = 25; di[i].X2 = di[i].X1 + 23; di[i].Y1 = 15;
	++i; //24
	di[i].Type = DI_TEXT; di[i].Y1 = 16; di[i].Flags = DIF_SEPARATOR;

	++i; //25
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 17;
	lstrcpy(di[i].Data, GetMsg(mColors));
	++i; //26
	di[i].Type = DI_EDIT; di[i].X1 = 13; di[i].X2 = di[i].X1 + 35; di[i].Y1 = 17;
	++i; //27
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 18;
	lstrcpy(di[i].Data, GetMsg(mCfgCodeTable));
	++i; //28
	di[i].Type = DI_COMBOBOX; di[i].X1 = 13; di[i].Y1 = 18; di[i].X2 = di[i].X1 + 35;
	di[i].Flags = DIF_DROPDOWNLIST; di[i].ListItems = &flt;
	++i; //29
	di[i].Type = DI_TEXT; di[i].X1 = 5; di[i].Y1 = 19;
	lstrcpy(di[i].Data, GetMsg(mCfgMemExt));
	++i; //30
	di[i].Type = DI_EDIT; di[i].X1 = 13; di[i].X2 = di[i].X1 + 35; di[i].Y1 = 19;
	++i; //31
	di[i].Type = DI_TEXT; di[i].Y1 = 20; di[i].Flags = DIF_SEPARATOR;

	++i; //32
	di[i].Type = DI_BUTTON; lstrcpy(di[i].Data, GetMsg(mButSave));
	di[i].X1 = 0; di[i].Y1 = 21; di[i].Flags = DIF_CENTERGROUP;
	di[i].DefaultButton = 1;
	++i; //33
	di[i].Type = DI_BUTTON; lstrcpy(di[i].Data, GetMsg(mButCancel));
	di[i].X1 = 0; di[i].Y1 = 21; di[i].Flags = DIF_CENTERGROUP;
	++i; //34
	i = Info.Dialog(Info.ModuleNumber, -1, -1, 54, 24, "Config", di, i);
	if (i == 32) {
		val = di[1].ListPos; sz = 4;
		RegSetValueEx(hKey, "CodeDefault", 0, REG_DWORD, (BYTE*) (&val), sz);
		val = di[3].Selected; sz = 4;    //-----> Показывать номера записей
		RegSetValueEx(hKey, "ShowRecNum", 0, REG_DWORD, (BYTE*) (&val), sz);
		val = di[4].Selected; sz = 4;    //-----> Показывать мемо на весь экран
		RegSetValueEx(hKey, "ShowMemoFull", 0, REG_DWORD, (BYTE*) (&val), sz);
		sz = lstrlen(di[5].Data) + 1;    //-----> Разделитель экспорта
		if (sz < 2 || ((di[5].Data[0] == 'B' || di[5].Data[0] == 'b') && di[5].Data[1] == '3')) {
			di[5].Data[0] = 0xb3; di[5].Data[1] = 0; sz = 2;
		}
		if (sz > 3) { di[5].Data[2] = 0; sz = 3; }
		if (sz)RegSetValueEx(hKey, "ExpSep", 0, REG_SZ, (BYTE*) di[5].Data, sz);
		val = di[7].Selected; sz = 4;    //-----> Автоматически сохранять шаблон
		RegSetValueEx(hKey, "AutoSave", 0, REG_DWORD, (BYTE*) (&val), sz);
		di[8].Data[2] = 0;             //-----> Маскирующие символы
		if (!di[8].Data[0]) { di[8].Data[0] = '?'; di[8].Data[1] = '%'; }
		if (!di[8].Data[1]) di[8].Data[1] = '%';
		sz = lstrlen(di[8].Data) + 1;
		RegSetValueEx(hKey, "MaskChar", 0, REG_SZ, (BYTE*) di[8].Data, sz);
		sz = lstrlen(di[26].Data) + 1;   //-----------> Пользовательские цвета
		if (sz > 256)sz = 256;
		RegSetValueEx(hKey, "Colors", 0, REG_SZ, (BYTE*) di[26].Data, sz);
		val = di[28].ListPos; sz = 4;    //----------> Кодовая таблица
		RegSetValueEx(hKey, "CodeTable", 0, REG_DWORD, (BYTE*) (&val), sz);
		sz = lstrlen(di[30].Data) + 1;   //----------> Расширения мемо-файла
		if (sz > 256) { sz = 256; di[30].Data[sz] = 0; }
		RegSetValueEx(hKey, "MemExt", 0, REG_SZ, (BYTE*) di[30].Data, sz);
		di[14].Data[DTFmtL - 1] = 0;            //-----> Показ даты
		sz = lstrlen(di[14].Data) + 1;
		RegSetValueEx(hKey, "D_fmt_V", 0, REG_SZ, (BYTE*) di[14].Data, sz);
		di[15].Data[DTFmtL - 1] = 0;            //-----> Показ даты-времени
		sz = lstrlen(di[15].Data) + 1;
		RegSetValueEx(hKey, "T_fmt_V", 0, REG_SZ, (BYTE*) di[15].Data, sz);
		di[18].Data[DTFmtL - 1] = 0;            //-----> Ввод даты
		sz = lstrlen(di[18].Data) + 1;
		RegSetValueEx(hKey, "D_fmt", 0, REG_SZ, (BYTE*) di[18].Data, sz);
		di[19].Data[DTFmtL - 1] = 0;            //-----> Ввод даты-времени
		sz = lstrlen(di[19].Data) + 1;
		RegSetValueEx(hKey, "T_fmt", 0, REG_SZ, (BYTE*) di[19].Data, sz);
		di[22].Data[DTFmtL - 1] = 0;            //-----> Экспорт даты
		sz = lstrlen(di[22].Data) + 1;
		RegSetValueEx(hKey, "D_fmt_E", 0, REG_SZ, (BYTE*) di[22].Data, sz);
		di[23].Data[DTFmtL - 1] = 0;            //-----> Экспорт даты-времени
		sz = lstrlen(di[23].Data) + 1;
		RegSetValueEx(hKey, "T_fmt_E", 0, REG_SZ, (BYTE*) di[23].Data, sz);
	}
	RegCloseKey(hKey);
	delete flt.Items;
	if (ItemNumber >= 0) { HeapDestroy(LookHeap); LookHeap = 0; }
	return false;
}

/*
Функция OpenPlugin вызывается при создании новой копии плагина.
*/
HANDLE WINAPI _export OpenPlugin(int OpenFrom, INT_PTR Item)
{
	LookHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
	if (!LookHeap)return INVALID_HANDLE_VALUE;
	char *a, b[MAX_PATH];
	struct PanelInfo pi;
	data = new LOOK;
	if (!data)goto PLUG_EXIT;
	if (OpenFrom == OPEN_COMMANDLINE) {
		lstrcpy(b, (const char *) Item); FSF.Unquote(b);
		data->Set(WinCode); data->LookOnly = 1;
		for (a = b;; a++) {
			if (*a == ' ') continue;
			if (*a != '/') break;
			switch (*(++a)) {
			case 'd': case 'D':  data->Clear(WinCode); break;
			case 'e': case 'E':  data->LookOnly = 0; break;
			case 'n': case 'N':  data->Set(LineNums); break;
			case 'm': case 'M':  data->Set(FullMemo); break;
			}
		}
		lstrcpy(data->FileName, a);
		data->ShowDBF();
	}
	else if (OpenFrom == OPEN_PLUGINSMENU) {
		Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, (void *) &pi);
		if (pi.Plugin || !pi.ItemsNumber) goto PLUG_EXIT;
		lstrcpy(data->FileName, pi.PanelItems[pi.CurrentItem].FindData.cFileName);
		data->ShowDBF();
	}
PLUG_EXIT:
	if (LookHeap) HeapDestroy(LookHeap);
	return  INVALID_HANDLE_VALUE;
}
//===========================================================================

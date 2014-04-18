#include <Windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include "detours/detours.h"
#include "./od_pdk/plugin.h"
#include "resource.h"

#pragma comment(lib,"./od_pdk/ollydbg.lib")

HINSTANCE	g_instThis = NULL;
WNDPROC		g_wndProc = NULL;
t_dump		*g_pdOllyCpu = NULL;
t_table		*g_ptabCpuDisasm = NULL;
DWORD		g_dwHookDrawFuncAddr = 0;

DWORD		g_dwEnableOllight = 0;
DWORD		g_dwOllightColor = 0;
BOOL		g_AllowFind = FALSE;
POINT		g_CurMousePos = {0, 0};
BYTE		g_Color[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
UINT		g_nTextAlign = 0;
POINT		g_dwDCOrg = {0, 0};
POINT		g_CurPos = {0, 0};
BOOL		g_bRecAllRect = FALSE;
RECT		g_rcTotalRect = {0, 0, 0, 0,};
TEXTMETRIC	g_tm = {0};

wchar_t g_HighLigthWord[1024] = {0};

#define HAS_CURMOUSEWORD  1
#define NO_CURMOUSEWORD   0

#define PLUGINNAME			L"Ollight"
#define VERSION				L"0.0.1"
#define SETCOLOR			L"Ollight Setting"
#define OD_VIEW_CPU			2206

BOOL (WINAPI* OrgExtTextOutW)( HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCWSTR lpString,  UINT c, CONST INT * lpDx) = ExtTextOutW;

BOOL IsParentOrSelf(HWND hParent, HWND hChild)
{
	HWND hTemp = hChild;
	HWND hDesktop;

	if (hParent == NULL || hChild == NULL)
	{
		return FALSE;
	}

	hDesktop = GetDesktopWindow();
	while (hTemp != NULL && hTemp != hDesktop)
	{
		if (hTemp == hParent)
		{
			return TRUE;
		}
		hTemp = GetParent(hTemp);
	}
	return FALSE;
}

void GetStringRectW(HDC hDC, LPCWSTR lpWideCharStr, UINT cbWideChars, int x, int y, RECT* lpStringRect, CONST INT *lpDx)
{
	SIZE   StringSize;
	POINT  WndPos;
	UINT i;

	if (cbWideChars < 0)
	{
		lpStringRect->top    = 0;
		lpStringRect->bottom = 0;
		lpStringRect->left   = 0;
		lpStringRect->right  = 0;
		return;
	}

	WndPos.x = g_dwDCOrg.x;
	WndPos.y = g_dwDCOrg.y;

	GetTextExtentPoint32W(hDC, lpWideCharStr, cbWideChars, &StringSize);

	if (lpDx != NULL)
	{
		StringSize.cx = 0;
		for (i = 0; i < cbWideChars; i++)
		{
			StringSize.cx += lpDx[i];
		}
	}

	if (TA_UPDATECP & g_nTextAlign)
	{
		x = g_CurPos.x;
		y = g_CurPos.y;
	}

	switch ((TA_LEFT | TA_CENTER | TA_RIGHT)&g_nTextAlign)
	{
	case TA_RIGHT:
		if (g_bRecAllRect == FALSE)
		{
			lpStringRect->right = x;
			lpStringRect->left  = x - StringSize.cx;
		}
		else
		{
			lpStringRect->left = g_rcTotalRect.left;
			lpStringRect->right= g_rcTotalRect.left + StringSize.cx;
		}
		break;
	case TA_CENTER:
		if (g_bRecAllRect == FALSE)
		{
			lpStringRect->right = x + StringSize.cx / 2;
			lpStringRect->left  = x - StringSize.cx / 2;
		}
		else
		{
			lpStringRect->left = g_rcTotalRect.left;
			lpStringRect->right= g_rcTotalRect.left + StringSize.cx;
		}
		break;
	case TA_LEFT:
	default:

		lpStringRect->left  = x ;
		lpStringRect->right = x + StringSize.cx;
		break;
	}

	lpStringRect->top    = y;
	lpStringRect->bottom = y + StringSize.cy;

	LPtoDP(hDC, (LPPOINT)lpStringRect, 2);

	lpStringRect->top    = lpStringRect->top    + WndPos.y;
	lpStringRect->bottom = lpStringRect->bottom + WndPos.y;
	lpStringRect->left   = lpStringRect->left   + WndPos.x;
	lpStringRect->right  = lpStringRect->right  + WndPos.x;
}

#define  AsmCount 152

wchar_t g_AsmComand[AsmCount+1][25] = {
	L"MOV",L"MOVSX",L"MOVZX",L"PUSH",L"POP",L"PUSHA",L"POPA",L"PUSHAD",L"POPAD",L"EAX",L"EBX",L"ECX",L"EDX",L"ESP",L"EBP",L"ESI",L"EDI",L"ST",L"XMM"
	L"BSWAP",L"XCHG",L"CMPXCHG",L"XADD",L"XLAT",L"BX",L"IN",L"OUT",L"LEA",L"LDS",L"LES",L"LFS",L"LGS",L"LSS",L"LAHF",L"SAHF",L"PUSHF",L"POPF",L"PUSHD",
	L"POPD",L"ADD",L"ADC",L"INC",L"AAA",L"DAA",L"SUB",L"SBB",L"DEC",L"NEC",L"CMP",L"AAS",L"DAS",L"MUL",L"IMUL",L"AAM",L"DIV",L"IDIV",L"AAD",L"CBW",
	L"CWD",L"CWDE",L"CDQ",L"AND",L"OR",L"XOR",L"NOT",L"TEST",L"SHL",L"SAL",L"SHR",L"SAR",L"ROL",L"ROR",L"RCL",L"RCR",L"SHL",L"DS:SI",L"ES:DI",L"CX",
	L"AL",L"AX",L"MOVS",L"SCAS",L"LODS",L"STOS",L"REP",L"REPE",L"REPZ",L"REPNE",L"REPNZ",L"REPC",L"REPNC",L"JMP",L"CALL",L"RET",L"RETF",L"JAE",L"JA",
	L"JNB",L"JB",L"JNAE",L"JBE",L"JNA",L"JG",L"JNLE",L"JGE",L"JNL",L"JL",L"JNGE",L"JLE",L"JNG",L"JE",L"JZ",L"JNE",L"JNZ",L"JC",L"JNC",L"JNO",L"JNP",
	L"JPO",L"JNS",L"JO",L"JP",L"JPE",L"JS",L"LOOP",L"LOOPE",L"LOOPZ",L"LOOPNE",L"LOOPNZ",L"JCXZ",L"JECXZ",L"INT",L"IRET",L"HLT",L"WAIT",
	L"ESC",L"LOCK",L"NOP",L"STC",L"CLC",L"CMC",L"STD",L"CLD",L"STI",L"CLI",L"DW",L"PROC",L"ENDP",L"SEGMENT",L"ASSUME",L"ENDS",L"END",L"NEG"
};

BOOL IsAsmInstruction(wchar_t *pwContent, int iContentSize)
{
	wchar_t wContentCopy[256] = {0};
	_tcsncpy_s(wContentCopy, 256, pwContent, iContentSize);
	ZeroMemory(wContentCopy + iContentSize, 256 - iContentSize);
	for(int i = 0;i < AsmCount ; i++)
	{
		if (_tcsstr(wContentCopy, g_AsmComand[i]) != NULL)
		{
			return TRUE;
		}
	}
	return FALSE;
}

#define CHAR_TYPE_ASCII 0
#define CHAR_TYPE_HZ    1
#define CHAR_TYPE_OTHER 2

DWORD CheckMouseInCurWordW(HDC hDC, LPCWSTR lpWideCharStr, UINT cbWideChars, int x, int y, CONST INT *lpDx, int *lpLeft, int nBegin, int nEnd, int nCharType, bool flags = true)
{
	RECT  StringRect;
	wchar_t wTemp[256] = {0};
	_tcsncpy_s(wTemp, 256, lpWideCharStr, nEnd+1);
	GetStringRectW(hDC, wTemp, nEnd+1, x, y, &StringRect, lpDx);

	if (flags == true)
	{
		StringRect.left = *lpLeft;
		*lpLeft = StringRect.right;
	}

	if (g_CurMousePos.x >= StringRect.left && 
		g_CurMousePos.x <= StringRect.right && 
		g_CurMousePos.y >= StringRect.top && 
		g_CurMousePos.y <= StringRect.bottom)
	{
		g_AllowFind = false; 			
		return HAS_CURMOUSEWORD;
	}
	return NO_CURMOUSEWORD;   
}

BOOL IsParterOfWord(wchar_t wch)
{
	if (((wch >= L'a')&&(wch <= L'z'))||
		((wch >= L'A')&&(wch <= L'Z'))||
		((wch >= L'0')&&(wch <= L'9'))||
		((wch == L'-')||(wch == L'+')||
			(wch == L'.')||(wch == L'*')||
			(wch == L'[')||(wch == L']')||(wch == L':'))

		)
	{
		return TRUE;
	}
	return FALSE;
}

DWORD GetCurMousePosWordW(HDC hDC, LPCWSTR lpWideCharStr, INT cbWideChars, int x, int y, CONST INT *lpDx)
{
	int   nCurrentWord, nPrevWord;
	RECT  StringRect ={0};
	int   nLeft;

	DWORD dwResult = NO_CURMOUSEWORD;

	//计算当前绘制文字的矩形区域
	GetStringRectW(hDC, lpWideCharStr, cbWideChars, x, y, &StringRect, lpDx);

	//当前绘制文件的最底部坐标高于鼠标所在行，跳过
	if (StringRect.bottom < g_CurMousePos.y)
	{
		return dwResult;
	}

	//绘制的文字已经越过了鼠标所在行，鼠标点击了空白区域会出现此种情况，停止搜寻并取消高亮
	if (g_CurMousePos.y < y)
	{
		_tcsncpy_s(g_HighLigthWord, 1024, L"", 1);
		g_AllowFind = FALSE;
		return dwResult;
	}

	//已经确定鼠标所指向的文字行，逐字计算所指向的是哪个字
	nPrevWord = -1;
	nCurrentWord = 0;
	for(nCurrentWord = 0; nCurrentWord < cbWideChars; nCurrentWord++)
	{
		dwResult = CheckMouseInCurWordW(hDC, lpWideCharStr, cbWideChars, x, y, lpDx, &nLeft, nPrevWord +1, nCurrentWord, 0);
		if(HAS_CURMOUSEWORD == dwResult)
		{
			int iLeftEdge = 0;
			int iRightEdge = 0;

			//寻找词的左边界
			for(iLeftEdge = nCurrentWord; iLeftEdge >=0; iLeftEdge--)
			{
				if (!IsParterOfWord(*(lpWideCharStr + iLeftEdge)))
				{
					iLeftEdge++;
					break;
				}
			}

			//寻找词的右边界
			for (iRightEdge = nCurrentWord; iRightEdge <= cbWideChars; iRightEdge++)
			{
				if (!IsParterOfWord(*(lpWideCharStr + iRightEdge)))
				{
					break;
				}
			}
			if (iLeftEdge < 0)
			{
				iLeftEdge = 0;
			}
			_tcsncpy_s(g_HighLigthWord, 1024, lpWideCharStr + iLeftEdge, iRightEdge - iLeftEdge);
			g_AllowFind = FALSE;
			// wchar_t OutputStrTest[1024] = L"";
			// wsprintf(OutputStrTest,L"FIND:%s - L:%d, R:%d, X:%d, Y:%d, MX:%d, MY:%d\n",g_HighLigthWord,iLeftEdge,iRightEdge,x,y,g_CurMousePos.x,g_CurMousePos.y);
			// OutputDebugString(OutputStrTest);	
			break;
		}
	}
	return dwResult;	
}

BOOL WINAPI NewExtTextOutW( HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCWSTR lpString,  UINT c, CONST INT * lpDx)
{
	POINT pt;
	HWND  hWDC;
	HWND  hWPT;
	DWORD dwThreadIdWithPoint = 0;
	DWORD dwThreadIdCurr = 0;

	pt.x = g_CurMousePos.x;
	pt.y = g_CurMousePos.y;
	
	hWDC = WindowFromDC(hdc);

	hWPT = WindowFromPoint(pt);

	if (!(hWDC == NULL || hWPT == hWDC
		|| IsParentOrSelf(hWPT, hWDC)
		|| IsParentOrSelf(hWDC, hWPT)))
	{
		return OrgExtTextOutW(hdc, x, y, options, lprect, lpString, c, lpDx);
	}

	//从上一栈帧获取真实的Y坐标
	DWORD dwRealY = 0;
	__asm
	{
		PUSH EAX
		MOV EAX, DWORD PTR SS:[EBP+0x58]
		MOV dwRealY,EAX
		POP EAX
	}

	if (g_AllowFind &&
		c > 1 && 
		IsAsmInstruction((wchar_t *)lpString, c))
	{
		g_nTextAlign = GetTextAlign(hdc);
		GetCurrentPositionEx(hdc, &g_CurPos);
		GetTextMetrics(hdc, &g_tm);
		g_dwDCOrg.x = 0;
		g_dwDCOrg.y = 0;
		g_bRecAllRect = FALSE;
		GetStringRectW(hdc, lpString, c, x, dwRealY,&g_rcTotalRect, lpDx);
		g_bRecAllRect = TRUE;
		GetDCOrgEx(hdc, &g_dwDCOrg);
		GetCurMousePosWordW(hdc, lpString, _tcslen(lpString), x, dwRealY, lpDx);
	}
	return OrgExtTextOutW(hdc, x, y, options, lprect, lpString, c, lpDx);
}

VOID HookTextOut()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)OrgExtTextOutW,NewExtTextOutW);
	DetourTransactionCommit();
}

VOID UnHookTextOut()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)OrgExtTextOutW,NewExtTextOutW);
	DetourTransactionCommit();
}

int SetHighlightColor(t_table *pt, wchar_t *name, ulong index, int mode);

static t_menu g_mMenu[] = 
{
	{ SETCOLOR, L"Set Highlight Color",K_NONE, SetHighlightColor, NULL, 0 },
	{ NULL, NULL, K_NONE, NULL, NULL, 0 }
};

extc int __cdecl ODBG2_Pluginquery(int ollydbgversion, ulong *features, wchar_t pluginname[SHORTNAME], wchar_t pluginversion[SHORTNAME]) 
{
	wcscpy_s(pluginname, SHORTNAME, PLUGINNAME);
	wcscpy_s(pluginversion, SHORTNAME, VERSION);
	if (ollydbgversion != 201)
	{
		Message(0, L"Ollight only for OllyDbg 2.01.");
		return 0;
	}
	return PLUGIN_VERSION;
};

void HookDrawFunc(DWORD TargetProc, DWORD NewProc)
{
	BYTE JMP = 0xE9;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)TargetProc, &JMP, sizeof(JMP), NULL);
	DWORD offset = NewProc - TargetProc - 5;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)(TargetProc + 1), &offset, sizeof(offset), NULL);
};

void UnHookDrawFunc(DWORD TargetProc)
{
	CHAR OrgMachineCode[5] = {0};

	OrgMachineCode[0] = 0x5B;
	OrgMachineCode[1] = 0x8B;
	OrgMachineCode[2] = 0xE5;
	OrgMachineCode[3] = 0x5D;
	OrgMachineCode[4] = 0xC3;

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)TargetProc, OrgMachineCode, 5, NULL);
};

void __cdecl DrawColor(char *pbColor, wchar_t *pwCode)
{
	int iStart = 0;
	int iEnd = 0;
	int iCodeLen = _tcslen(pwCode);
	int iHighLigthLen = _tcslen(g_HighLigthWord);
	wchar_t *pwcStart = _tcsstr(pwCode, g_HighLigthWord);
	if (iCodeLen < 1 || pwcStart == NULL || iHighLigthLen == 0)
	{
		return;
	}
	//是否包含要高亮的字符串
	for (;pwcStart != NULL; 
		 pwcStart = _tcsstr(pwcStart, g_HighLigthWord))
	{
		iStart = pwcStart - pwCode;
		iEnd = iStart + iHighLigthLen;

		//所匹配字符的前一个字符与末尾后一个字符都应该不属于词组的一部分，否则高亮 AL 寄存器，会该把 CALL中的AL也高亮
		if (iStart > 0 && IsParterOfWord(*(pwCode + (iStart - 1))))
		{
			pwcStart += iHighLigthLen;
			continue;
		}

		//右边界不属于词组
		if (iEnd < iCodeLen && IsParterOfWord(*(pwCode + iEnd)))
		{
			pwcStart += iHighLigthLen;
			continue;
		}

		//修改着色
		for (; iStart < iEnd; ++iStart)
		{
			pbColor[iStart] = g_Color[g_dwOllightColor];
		}
		pwcStart += iHighLigthLen;
	}
}

_declspec(naked) void __cdecl ExtendDrawFunc()
{
	__asm{
		PUSHAD
		PUSHFD
		CMP BYTE PTR SS:[EBP+0x1C],02
		JNE NOCOLOR
		MOV ECX,DWORD PTR SS:[EBP+0x14]
		MOV EAX,DWORD PTR DS:[ECX+0x10]
		CMP EAX,0x006D0073			//判断当前窗口标题是否为 CPU Disasm，只比较了sm
		JNE NOCOLOR
		MOV ECX,DWORD PTR SS:[EBP+0x8] //Code
		PUSH ECX
		MOV ECX,DWORD PTR SS:[EBP+0xC] //Color
		PUSH ECX
		CALL DrawColor
		ADD ESP,0x8
NOCOLOR:
		POPFD
		POPAD
		POP EBX
		MOV ESP,EBP
		POP EBP
		RETN
	}
};

BOOL SetXy(HWND hWnd,int x,int y)
{
	g_CurMousePos.x = x;
	g_CurMousePos.y = y;
	g_AllowFind = TRUE;
	Updatetable(&g_pdOllyCpu->table,TRUE);
	Updatetable(&g_pdOllyCpu->table,TRUE);
	return TRUE;
}

LRESULT CALLBACK OllightSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hEditColor = GetDlgItem(hDlg, EDIT_OLLIGHT_COLOR);
	HWND hEnableOllight = GetDlgItem(hDlg, BTN_ENABLE_OLLIGHT);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			wchar_t ColorStrTemp[10] = {0};
			wsprintf(ColorStrTemp, L"%d", g_dwOllightColor);
			
			SetWindowText(hEditColor, ColorStrTemp);
			EnableWindow(hEditColor, g_dwEnableOllight);
			SetWindowText(hEnableOllight, g_dwEnableOllight ? L"Disable Ollight" : L"Enable Ollight");
			
			RECT rcOllyRect;
			GetWindowRect(hwollymain, &rcOllyRect);
			int iOllyWidth = rcOllyRect.right - rcOllyRect.left;
			int iOllyHeigth = rcOllyRect.bottom - rcOllyRect.top;
			int iSetWndLeft = iOllyWidth / 2 - 182;
			int iSetWndTop = iOllyHeigth / 2 - 150;
			MoveWindow(hDlg, iSetWndLeft + rcOllyRect.left, iSetWndTop + rcOllyRect.top, 365, 295, true);

		}break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BTN_ENABLE_OLLIGHT:
			{
				g_dwEnableOllight ^= 1;
				if (g_dwEnableOllight)
				{
					HookDrawFunc((DWORD)g_dwHookDrawFuncAddr, (DWORD)ExtendDrawFunc);
					HookTextOut();
				}
				else
				{
					UnHookTextOut();
					UnHookDrawFunc(g_dwHookDrawFuncAddr);
				}
				EnableWindow(hEditColor, g_dwEnableOllight);
				SetWindowText(hEnableOllight, g_dwEnableOllight ? L"Disable Ollight" : L"Enable Ollight");
				return TRUE;
			}break;
		case BTN_OK:
			{
				DWORD dwColorTextLen = GetWindowTextLength(hEditColor);
				wchar_t ColorStr[10] = {0};
				DWORD dwColorTemp = 0;
				if (dwColorTextLen != 1)
				{
					MessageBox(hDlg, L"Invalid Color.", PLUGINNAME, 0);
					return TRUE;
				}
				else
				{
					GetWindowText(hEditColor, ColorStr, 10);
					dwColorTemp = _wtoi(ColorStr);
					if (dwColorTemp < 0 || dwColorTemp > 9)
					{
						MessageBox(hDlg, L"Invalid Color.", PLUGINNAME, 0);
						return TRUE;
					}
					else
					{
						g_dwOllightColor = dwColorTemp;
						Writetoini(NULL, PLUGINNAME, L"Ollight Color", L"%d", g_dwOllightColor);
						Writetoini(NULL, PLUGINNAME, L"Enable Ollight", L"%d", g_dwEnableOllight);
					}
				}
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}break;
		}
		if(LOWORD(wParam) == IDCANCEL)
		{
			Writetoini(NULL, PLUGINNAME, L"Enable Ollight", L"%d", g_dwEnableOllight);
			EndDialog(hDlg, LOWORD(wParam));
		}
		break;
	}
	return false;
}

int SetHighlightColor(t_table *pt, wchar_t *name, ulong index, int mode) 
{
	if (mode == MENU_VERIFY)
	{
		return MENU_NORMAL;
	}
	else if (mode == MENU_EXECUTE)
	{
		DialogBox(g_instThis, (LPCTSTR)IDD_OLLIGHT_SETTING, hwollymain, (DLGPROC)OllightSettingProc);

		return MENU_NOREDRAW;
	}
	return MENU_ABSENT;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch( fdwReason ) 
	{ 
	case DLL_PROCESS_ATTACH:
		{
			g_instThis = hinstDLL;
		}break;

	case DLL_THREAD_ATTACH:
		{
		}
		break;

	case DLL_THREAD_DETACH:
		{
		}
		break;

	case DLL_PROCESS_DETACH:
		{
		}
		break;
	}
	return TRUE;
};

HRESULT CALLBACK NewODWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	POINT pt;
	if (uMsg == 0x210)
	{
		GetCursorPos(&pt);
		ScreenToClient(g_pdOllyCpu->table.hparent,&pt);
		SetXy(g_pdOllyCpu->table.hparent,pt.x,pt.y);
	}
	return CallWindowProc(g_wndProc, hwnd, uMsg, wParam, lParam);
}

extc int __cdecl ODBG2_Plugininit(void) 
{
	DRAWFUNC *pfunCpuDraw = NULL;

	g_wndProc = (WNDPROC)SetWindowLong (hwollymain, GWL_WNDPROC, (LONG)NewODWndProc);
	CallWindowProc(g_wndProc, hwollymain, WM_COMMAND, OD_VIEW_CPU, 0);
	g_pdOllyCpu = Getcpudisasmdump();
	g_ptabCpuDisasm = Getcpudisasmtable();
	pfunCpuDraw = g_ptabCpuDisasm->drawfunc;
	if (pfunCpuDraw == NULL)
	{
		Message(0,L"Can't get cpu disasm table and drawfunc pointer.");
		return -1;
	}
	char *pbMachineCode = (char *)pfunCpuDraw + 0x419B;
	if (pbMachineCode[0] == 0x5B &&
		pbMachineCode[1] == 0x8B &&
		pbMachineCode[2] == 0xE5 &&
		pbMachineCode[3] == 0x5D &&
		pbMachineCode[4] == 0xC3 )
	{
		g_dwHookDrawFuncAddr = (DWORD)pfunCpuDraw + 0x419B;

		DWORD dwFirstRun = 0;
		Getfromini(NULL, PLUGINNAME, L"First Run Ollight", L"%d", &dwFirstRun);
		if (dwFirstRun != 6080)
		{
			Writetoini(NULL, PLUGINNAME, L"First Run Ollight", L"%d", 6080);
			Writetoini(NULL, PLUGINNAME, L"Enable Ollight", L"%d", 1);
			Writetoini(NULL, PLUGINNAME, L"Ollight Color", L"%d", 4);
			HookDrawFunc((DWORD)pbMachineCode, (DWORD)ExtendDrawFunc);
			HookTextOut();
			g_dwEnableOllight = 1;
			g_dwOllightColor = 4;
		}
		else
		{
			Getfromini(NULL, PLUGINNAME, L"Enable Ollight", L"%d", &g_dwEnableOllight);
			Getfromini(NULL, PLUGINNAME, L"Ollight Color", L"%d", &g_dwOllightColor);
			if (g_dwEnableOllight)
			{
				HookDrawFunc((DWORD)pbMachineCode, (DWORD)ExtendDrawFunc);
				HookTextOut();
			}
		}
	}
	else
	{
		Message(0,L"Can't Hook drawfunc.");
		return -1;
	}
	return 0;
};

extc void __cdecl ODBG2_Pluginreset(void) 
{

};

extc t_menu * __cdecl ODBG2_Pluginmenu(wchar_t *type) 
{
	if (wcscmp(type, PWM_MAIN) == 0)
	{
		return g_mMenu;
	}
	return NULL;
};

extc int __cdecl ODBG2_Pluginclose(void) 
{
	return 0;
};

extc void __cdecl ODBG2_Plugindestroy(void) 
{

};

extc void __cdecl ODBG2_Pluginnotify(int code,void *data,ulong parm1,ulong parm2) 
{

};

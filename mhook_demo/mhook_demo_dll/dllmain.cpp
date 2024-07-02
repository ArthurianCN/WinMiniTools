// dllmain.cpp : 定义 DLL 应用程序的入口点。


#include "stdafx.h"

#include <Shlwapi.h>
#include <tchar.h>
#include <unordered_set>
#include <string>

#include "..\mhook-lib\mhook.h"
#include "..\Public\DebugMsg.h"

TCHAR szApp[MAX_PATH] = { 0 };
TCHAR szAppFullPath[MAX_PATH] = { 0 };
#ifndef _countof
#define _countof(P) sizeof(P)/sizeof(P[0])
#endif

#pragma comment(lib, "Shlwapi.lib")

typedef int (WINAPI *Fun_StartDocA)( HDC hdc, CONST DOCINFOA *lpdi ); 
typedef int (WINAPI *Fun_StartDocW)( HDC hdc, CONST DOCINFOW *lpdi ); 

Fun_StartDocA g_pfnStartDocA = StartDocA;
Fun_StartDocW g_pfnStartDocW = StartDocW; 

#ifdef UNICODE
#define TString		std::wstring
#else
#define TString		std::string
#endif

std::unordered_set<TString> hashSetUnhookProc;

bool DllInit()
{
	GetModuleFileName(NULL, szAppFullPath, _countof(szAppFullPath));
	LPTSTR lpApp = PathFindFileName(szAppFullPath);
	_tcsncpy_s(szApp, _countof(szAppFullPath), lpApp, _TRUNCATE);

	hashSetUnhookProc.insert(_T("DebugView++.exe"));
	hashSetUnhookProc.insert(_T("explorer.exe"));
	hashSetUnhookProc.insert(_T("mhook_demo.exe"));
	return true;
}


bool IsNeedToHook()
{
	if(hashSetUnhookProc.end() != hashSetUnhookProc.find(szApp))
	{
		DP1("Not to hook [%s]", szApp);
		return false;
	}

	/*if(NULL != _tcsstr(szAppFullPath, _T("C:\\Windows\\")))
	{
	DP1("Not to hook [%s]", szAppFullPath);
	return false;
	}*/

	return true;
}


bool IsPermitPrint()
{
	HWND htForegroundWnd = GetForegroundWindow();
	TCHAR szWndText[MAX_PATH] = { 0 };
	GetWindowText(htForegroundWnd, szWndText, _countof(szWndText));
	DP1("IsPermitPrint szWndText[%s]", szWndText);
	return false;
}

int WINAPI GefStartDocA( HDC hdc, CONST DOCINFOA *lpdi )
{
	
	
	DP0("GefStartDocA");
	return IsPermitPrint() ? g_pfnStartDocA(hdc, lpdi) : 0;
}


int WINAPI GefStartDocW( HDC hdc, CONST DOCINFOW *lpdi )
{
	DP0("GefStartDocW");
	return IsPermitPrint() ? g_pfnStartDocW(hdc, lpdi) : 0;
}

bool bHooked = false;
bool HookApi();

void UnHookApi();



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if(false == DllInit())
		{
			return FALSE;
		}
		DP2("DLL_PROCESS_ATTACH szApp: %s bHooked: %d", szApp, bHooked);
		
		bHooked = bHooked ? bHooked : HookApi();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DP0("DLL_PROCESS_ATTACH ");
		UnHookApi();
		break;
	default:
		break;
	}
	return TRUE;
}

bool HookApi()
{
	if(bHooked)
	{
		DP0("Already Hooked");
		return true;
	}

	if(false == IsNeedToHook())
	{
		return false;
	}
	
	DP0("Hook begin");

	DP2("StartDocA original: %p, hooked: %p", g_pfnStartDocA, GefStartDocA);
	Mhook_SetHook((PVOID *)&g_pfnStartDocA, GefStartDocA);

	DP2("StartDocW original: %p, hooked: %p", g_pfnStartDocW, GefStartDocW);
	Mhook_SetHook((PVOID *)&g_pfnStartDocW, GefStartDocW);

	DP0("Hook end");

	return true;
}



void UnHookApi()
{
	Mhook_Unhook((PVOID *)&g_pfnStartDocA);
	Mhook_Unhook((PVOID *)&g_pfnStartDocW);

	return;
}
// mhook_demo_dll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mhook_demo_dll.h"
#include "..\Public\DebugMsg.h"

// 这是导出变量的一个示例
MHOOK_DEMO_DLL_API int nmhook_demo_dll=0;

// 这是导出函数的一个示例。
MHOOK_DEMO_DLL_API int fnmhook_demo_dll(void)
{
	return 42;
}

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 mhook_demo_dll.h
Cmhook_demo_dll::Cmhook_demo_dll()
{
	return;
}
extern bool bHooked;

bool HookApi();
// 全局钩子 
MHOOK_DEMO_DLL_API LRESULT CALLBACK GefApiCBTProc( int	nCode, 
	WPARAM	wParam,
	LPARAM	lParam 
	)
{

	if ((HCBT_CREATEWND != nCode) && (HCBT_ACTIVATE != nCode)) 
	{
		return CallNextHookEx( NULL, nCode, wParam, lParam ); 
	}

	DP1("GefApiCBTProc bHooked : %d", bHooked);

	bHooked = bHooked ? bHooked : HookApi();

	return CallNextHookEx( NULL, nCode, wParam, lParam ); 
}
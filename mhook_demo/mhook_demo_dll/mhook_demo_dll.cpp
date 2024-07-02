// mhook_demo_dll.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "mhook_demo_dll.h"
#include "..\Public\DebugMsg.h"

// ���ǵ���������һ��ʾ��
MHOOK_DEMO_DLL_API int nmhook_demo_dll=0;

// ���ǵ���������һ��ʾ����
MHOOK_DEMO_DLL_API int fnmhook_demo_dll(void)
{
	return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� mhook_demo_dll.h
Cmhook_demo_dll::Cmhook_demo_dll()
{
	return;
}
extern bool bHooked;

bool HookApi();
// ȫ�ֹ��� 
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
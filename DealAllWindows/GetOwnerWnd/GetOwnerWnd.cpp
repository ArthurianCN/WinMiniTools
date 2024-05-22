#include <Windows.h>

#include <iostream>
#include "..\comm\DbgLogWrite.hpp"

using namespace std;



int nCnt = 0;
bool DealTheWndHandle(HWND hWnd)
{
	DWORD dwProcId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcId);

	char szProcessName[MAX_PATH] = { 0 };
	GetProcessName(szProcessName, dwProcId);

	HWND hOwnerWnd = GetWindow(hWnd, GW_OWNER);

	++nCnt;
	//cout << oct << nCnt << "：" << hex << hWnd << endl;
	LogWriteEx("[%d] 0x%08x [ProcID: %d, ProcName: %s] hOwnerWnd: 0x%08x", nCnt, hWnd, dwProcId, szProcessName, hOwnerWnd);
	return true;
}

BOOL CALLBACK EnumWindowProc(HWND hWnd,LPARAM lParam);
BOOL CALLBACK EnumChildWindowProc(HWND hWnd,LPARAM lParam);
int main()
{

	int nTimes = 0;
	while (10 > nTimes)
	{
		++nTimes;
		LogWriteEx("Times: %d", nTimes);
		::EnumWindows(EnumWindowProc, 0);//枚举窗口
		
		Sleep(1 * 1000);
	}
	
	//Sleep(30*1000);
	return 0;
}




BOOL CALLBACK EnumWindowProc(HWND hWnd,LPARAM lParam)//枚举窗口的回调函数
{
	if((NULL == hWnd) || (FALSE == IsWindow(hWnd)))
	{
		return TRUE;
	}

	DealTheWndHandle(hWnd);

	//EnumChildWindows(hWnd, EnumChildWindowProc, 0);

	return true;
}
BOOL CALLBACK EnumChildWindowProc(HWND hWnd,LPARAM lParam)
{
	if((NULL == hWnd) || (FALSE == IsWindow(hWnd)))
	{
		return TRUE;
	}

	DealTheWndHandle(hWnd);

	EnumChildWindows(hWnd, EnumChildWindowProc, 0);

	return true;
}

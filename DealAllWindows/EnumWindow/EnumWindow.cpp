#include <Windows.h>

#include <iostream>
#include "..\comm\DbgLogWrite.hpp"

using namespace std;



int nCnt = 0;
bool DealTheWndHandle(HWND hWnd)
{
	DWORD dwProcId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcId);
	++nCnt;
	//cout << oct << nCnt << "：" << hex << hWnd << endl;
	LogWriteEx("[%d] 0x%08x [ProcID: %d]", nCnt, hWnd, dwProcId);
	return true;
}

BOOL CALLBACK EnumWindowProc(HWND hWnd,LPARAM lParam);
BOOL CALLBACK EnumChildWindowProc(HWND hWnd,LPARAM lParam);
int main()
{
	::EnumWindows(EnumWindowProc, 0);//枚举窗口
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

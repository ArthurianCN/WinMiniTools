/**/
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
	//cout << oct << nCnt << "£º" << hex << hWnd << endl;
	LogWriteEx("[%d] 0x%08x [ProcID: %d]", nCnt, hWnd, dwProcId);
	return true;
}

int main()
{
	LogWriteEx("CurPid [%d]", GetCurrentProcessId());
	Sleep(10*1000);

	HWND hWndPrev = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
	while(::IsWindow(hWndPrev))
	{
		DealTheWndHandle(hWndPrev);

		// ÕÒÏÂÒ»¸öhWnd
		hWndPrev = ::GetWindow( hWndPrev, GW_HWNDNEXT );
	}

	return 0;
}

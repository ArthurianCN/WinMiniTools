#pragma once
#include <windows.h>
#include <stdio.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#pragma comment(lib, "shlwapi.lib")
#include <Tlhelp32.h>



#define LOG_DIRECTORY_EX		"GFA"
#define LOG_FILE_EX				"SockAgent_LOG.log"
#define LOG_MAX_SIZE			(1 *1024 * 1024 * 1024)

void LogWriteEx(LPCSTR szFormat, ...)
{
	char szLogPath[MAX_PATH] = {"C:\\log"};

	va_list args;

	// retrieve the variable arguments
	va_start(args, szFormat);
	// 输出文件的名字 
	FILE* pLogFile = NULL;
#ifdef VS_VERSION_6 
	pLogFile = fopen(LOG_FILE_A, "a+b");
	if (NULL != pLogFile)
#else 
	errno_t err = fopen_s(&pLogFile, szLogPath, "a+b");
	if ((0 == err) && (NULL != pLogFile))
#endif 	
	{
		if(0 == vfprintf(pLogFile, szFormat, args))
		{

		}
#ifdef VS_VERSION_6 
		fprintf(pLogFile, "\r\n");
#else 
		if(0 > fprintf_s(pLogFile, "\r\n"))
		{

		}
#endif 
		fclose(pLogFile);
	}
	va_end(args);/*lint -e438*/

	return;
}

bool GetProcessName(LPSTR lpProcess, DWORD dwProcessID)
{
	HANDLE 	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		LogWriteEx("[%s %d %s] CreateToolhelp32Snapshot failed. LastError: 0x%08x", __FILE__, __LINE__, __FUNCTION__, GetLastError());
		return false;
	}

	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(pe32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		LogWriteEx("[%s %d %s] Process32First Failed. LastError: 0x%08x", __FILE__, __LINE__, __FUNCTION__, GetLastError());
		CloseHandle(hProcessSnap);
		return false;
	}

	while (TRUE == Process32Next(hProcessSnap, &pe32))
	{
		if (dwProcessID == pe32.th32ProcessID)
		{
			strcpy(lpProcess, pe32.szExeFile);
			CloseHandle(hProcessSnap);
			return true;
		}
	}

	CloseHandle(hProcessSnap);
	//My_CloseHandle(hProcessSnap);
	return false;
}
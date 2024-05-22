/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#include "stdafx.h"
#include "Formatting.h"
#include "Utils.h"

// --------------------------------------------------------------------------------
// Get error text from error code
std::wstring SysErrorMessage(DWORD dwErrCode /*= GetLastError()*/)
{
	LPWSTR pszErrMsg = NULL;
	DWORD flags =
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_FROM_SYSTEM ;

	if ( FormatMessageW(
		flags,
		NULL, 
		dwErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPWSTR) &pszErrMsg,
		0,
		NULL ) )
	{
		std::wstring retval = pszErrMsg;
		LocalFree(pszErrMsg);
		return retval;
	}
	else
	{
		std::wstringstream sRetval;
		sRetval << L"Error # " << dwErrCode << L" (" << HEX(dwErrCode) << L")";
		return sRetval.str();
	}
}

std::wstring SysErrorMessageWithCode(DWORD dwErrCode /*= GetLastError()*/)
{
	LPWSTR pszErrMsg = NULL;
	wstringstream sRetval;
	DWORD flags =
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_FROM_SYSTEM ;

	if ( FormatMessageW(
		flags,
		NULL, 
		dwErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPWSTR) &pszErrMsg,
		0,
		NULL ) )
	{
		sRetval << pszErrMsg << endl << L"(Error # " << dwErrCode << L" = " << HEX(dwErrCode) << L")";
		LocalFree(pszErrMsg);
	}
	else
	{
		sRetval << L"Error # " << dwErrCode << L" (" << HEX(dwErrCode) << L")";
	}
	return sRetval.str();
}

// ----------------------------------------------------------------------------------------------------

// Reboot the computer, setting the reason as "Operating System: Reconfiguration (Planned)"
BOOL RebootComputer()
{
	BOOL ret;
	DWORD dwLastErr;
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		//dwLastErr = GetLastError();
		//sResults << L"OpenProcessToken failed; error " << dwLastErr << endl;
		return FALSE;
	}

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL);
	dwLastErr = GetLastError();
	CloseHandle(hToken);
	if (ERROR_SUCCESS != dwLastErr)
	{
		//sResults << L"AdjustTokenPrivileges failed; error " << dwLastErr << endl;
		return FALSE; 
	}

	// "Operating System: Reconfiguration (Planned)"
	const DWORD dwShutdownCode = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED;
	const DWORD dwTimeout = 0;

	ret = InitiateSystemShutdownExW(NULL, NULL, dwTimeout, FALSE, TRUE, dwShutdownCode);
	if ( !ret )
	{
		//dwLastErr = GetLastError();
		//sResults << L"InitiateSystemShutdownExW failed; error " << dwLastErr << endl;
		return FALSE;
	}

	//ret = ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
	//if ( !ret )
	//{
	//	dwLastErr = GetLastError();
	//	sResults << L"ExitWindowsEx failed; error " << dwLastErr << endl;
	//	return FALSE;
	//}

	return TRUE;
}

// ----------------------------------------------------------------------------------------------------

//TODO:  This should return encoding - not just UNICODE vs. not-UNICODE (e.g., report ANSI, UTF-8, UTF-16, etc.)
//References:  http://www.unicode.org/faq/utf_bom.html#BOM , http://msdn.microsoft.com/en-us/library/ms776429.aspx
bool IsUnicodeFile(const wstring & file)
{
	HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if ( INVALID_HANDLE_VALUE == hFile )
		return false;

	DWORD numread = 0;
	wchar_t bom;
	BOOL rfRet = ReadFile(hFile, &bom, sizeof(bom), &numread, NULL);
	//DWORD dwLastErr = GetLastError();
	CloseHandle(hFile);
	if (!rfRet || sizeof(bom) != numread )
		return false;

	return (0xFEFF == bom);
}


// ----------------------------------------------------------------------------------------------------
// If running on 64-bit, allow temporarily turning off file system redirection.
// Need to load the APIs dynamically - not present on XP x86

Wow64FsRedirection::Wow64FsRedirection()
: m_OldValue(NULL)
{
	// No need to free this hLib
	HMODULE hLib = LoadLibraryW(L"kernel32.dll");
	m_pDisableWowFsRedir = (pfnWow64DisableWow64FsRedirection_t)GetProcAddress(hLib, "Wow64DisableWow64FsRedirection");
	m_pRevertWowFsRedir =  (pfnWow64RevertWow64FsRedirection_t) GetProcAddress(hLib, "Wow64RevertWow64FsRedirection");
}

Wow64FsRedirection::~Wow64FsRedirection()
{
}

BOOL Wow64FsRedirection::Disable()
{
	if (m_pDisableWowFsRedir)
		return m_pDisableWowFsRedir(&m_OldValue);
	else
		return FALSE;
}

BOOL Wow64FsRedirection::Revert()
{
	if (m_pRevertWowFsRedir)
		return m_pRevertWowFsRedir(m_OldValue);
	else
		return FALSE;
}

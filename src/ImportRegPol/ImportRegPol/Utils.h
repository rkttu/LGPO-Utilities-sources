/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

// Get error text from error code
std::wstring SysErrorMessage(DWORD dwErrCode = GetLastError());
std::wstring SysErrorMessageWithCode(DWORD dwErrCode = GetLastError());

// Reboot the computer, setting the reason as "Operating System: Reconfiguration (Planned)"
BOOL RebootComputer();

//TODO:  This should return encoding - not just UNICODE vs. not-UNICODE (e.g., report ANSI, UTF-8, UTF-16)
bool IsUnicodeFile(const wstring & file);

// If running on 64-bit, allow temporarily turning off file system redirection.
// Need to load the APIs dynamically - not present on XP x86
class Wow64FsRedirection
{
public:
	Wow64FsRedirection();
	~Wow64FsRedirection();

	BOOL Disable();
	BOOL Revert();

private:
	typedef BOOL (WINAPI *pfnWow64DisableWow64FsRedirection_t)(PVOID *OldValue);
	typedef BOOL (WINAPI *pfnWow64RevertWow64FsRedirection_t)(PVOID OldValue);
	pfnWow64DisableWow64FsRedirection_t m_pDisableWowFsRedir;
	pfnWow64RevertWow64FsRedirection_t  m_pRevertWowFsRedir;
	PVOID m_OldValue;

private:
	Wow64FsRedirection(const Wow64FsRedirection &);
	Wow64FsRedirection & operator = (const Wow64FsRedirection &);

};

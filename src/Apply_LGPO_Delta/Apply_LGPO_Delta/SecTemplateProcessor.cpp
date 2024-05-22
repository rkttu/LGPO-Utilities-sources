/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#include "StdAfx.h"
#include "SecTemplateProcessor.h"

SecTemplateProcessor::SecTemplateProcessor(Logs & logs)
: m_logs(logs)
{
}

SecTemplateProcessor::~SecTemplateProcessor()
{
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

static const wchar_t * szResType = L"GptTmpl.inf";

int SecTemplateProcessor::ProcessSecurityTemplates(const filelist_t & sectempfiles)
{
	m_retval = 0;
	for ( filelistIter_t secInf = sectempfiles.begin(); secInf != sectempfiles.end(); secInf++ )
	{
		ProcessSecurityTemplate(*secInf);
	}
	return m_retval;
}


// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------


int SecTemplateProcessor::ProcessSecurityTemplate(const wstring & sSecTemplateInf)
{
	int retval = 0;

	// Create temp file names for the security template content, for the sdb file that
	// secedit.exe will create and apply to the system, and for the log file that secedit.exe will write to.
	wchar_t 
		temppath[MAX_PATH + 1],
		tempSecTemplateSdb[MAX_PATH + 1], 
		tempSecTemplateLog[MAX_PATH + 1];
	DWORD dwLastErr = 0;
	SecureZeroMemory(temppath, sizeof(temppath));
	SecureZeroMemory(tempSecTemplateSdb, sizeof(tempSecTemplateSdb));
	SecureZeroMemory(tempSecTemplateLog, sizeof(tempSecTemplateLog));
	if ( 
		(0 == GetTempPathW(MAX_PATH, temppath)) ||
		(0 == GetTempFileNameW(temppath, L"GPT", 0, tempSecTemplateSdb)) ||
		(0 == GetTempFileNameW(temppath, L"GPT", 0, tempSecTemplateLog)))
	{
		dwLastErr = GetLastError();
		m_logs.ErrOut() << L"Unable to create temporary files; " << SysErrorMessageWithCode(dwLastErr) << endl;
		return -1;
	}

	m_logs.Output() << Logs::szLine() << endl;
	m_logs.Output() << L"PROCESSING SECURITY TEMPLATE:  " << sSecTemplateInf << endl << endl;

	// Invoke secedit with the three temp files, the first of which contains the security template we want to use.
	// The temporary log file will be read and appended to the output log.
	retval = InvokeSecedit(sSecTemplateInf.c_str(), tempSecTemplateSdb, tempSecTemplateLog);

	// Delete the temp files.
	DeleteFileW(tempSecTemplateSdb);
	DeleteFileW(tempSecTemplateLog);

	return retval;
}


int SecTemplateProcessor::InvokeSecedit(const wchar_t *szSecTemplateInf, const wchar_t *tempSecTemplateSdb, const wchar_t *tempSecTemplateLog)
{
	// Invoke a secedit.exe command line, capturing both its stdout and stderr.
	HANDLE hPipeStdoutWr = NULL, hPipeStdoutRd = NULL, hPipeStderrWr = NULL, hPipeStderrRd = NULL;
	HANDLE hPipeStdinWr = NULL, hPipeStdinRd = NULL;
	DWORD dwLastErr;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// For redirecting stdout and stderr (and stdin)
	if ( !CreatePipe(&hPipeStdoutRd, &hPipeStdoutWr, &sa, 0) || !CreatePipe(&hPipeStderrRd, &hPipeStderrWr, &sa, 0) || !CreatePipe(&hPipeStdinRd, &hPipeStdinWr, &sa, 0) )
	{
		dwLastErr = GetLastError();
		m_logs.ErrOut() << L"Error building connection to read data from secedit.exe; " << SysErrorMessageWithCode(dwLastErr) << endl;
		return -1;
	}

	// The "read" ends of the output pipes shouldn't be inherited, nor the "write" end of the input pipe.
	SetHandleInformation(hPipeStdoutRd, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hPipeStderrRd, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hPipeStdinWr,  HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	SecureZeroMemory(&si, sizeof(si));
	SecureZeroMemory(&pi, sizeof(pi));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdError = hPipeStderrWr;
	si.hStdOutput = hPipeStdoutWr;
	si.hStdInput = hPipeStdinRd;
	si.wShowWindow = SW_HIDE;

	// Build the command line, which needs to be in writable memory.
	//TODO:  Should also get the Windows/system32 folder and specify the full path to secedit.exe.
	wstringstream streamCmdLine;
	wstring stringCmdLine;
	size_t nCmdLineLength, cbCmdLineLength;
	// Build it
	streamCmdLine 
		<< L"secedit.exe /configure "
		<< L"/db \"" << tempSecTemplateSdb << L"\" "
		<< L"/cfg \"" << szSecTemplateInf << L"\" "
		<< L"/log \"" << tempSecTemplateLog << L"\" "
		<< L"/overwrite "; // /quiet";
	stringCmdLine = streamCmdLine.str();
	m_logs.Output() << stringCmdLine << endl << endl;
	nCmdLineLength = stringCmdLine.length();
	cbCmdLineLength = (nCmdLineLength+1) * sizeof(wchar_t);
	// Allocate a writable data structure and copy it in.  (Could have just casted away the constness of stringCmdLine.c_str(), but that's inviting trouble.
	wchar_t * szWritableCmdLine = (wchar_t*)_alloca(cbCmdLineLength);
	SecureZeroMemory(szWritableCmdLine, cbCmdLineLength);
#pragma warning ( suppress: 4996 )
	stringCmdLine.copy(szWritableCmdLine, nCmdLineLength);

	// Run the secedit.exe command line, without displaying a console window.
	BOOL cpRet = CreateProcessW(NULL, szWritableCmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	dwLastErr = GetLastError();
	// Close the "write" ends of the output pipes and the "read" end of the input pipe -- the target process should now have them. (Verify this.)
	CloseHandle(hPipeStdoutWr);
	CloseHandle(hPipeStderrWr);
	CloseHandle(hPipeStdinRd);
	if ( !cpRet )
	{
		m_logs.ErrOut() << L"Unable to start SECEDIT.EXE; " << SysErrorMessageWithCode(dwLastErr) << endl;
	}
	else
	{
		// Write a confirming "Yes" to secedit.exe's stdin
		//TODO:  THIS IS ENGLISH-LANGUAGE SPECIFIC.
		DWORD numWritten = 0;
		const char * yes = "y\r\n";
		WriteFile(hPipeStdinWr, yes, 3, &numWritten, NULL);
		CloseHandle(hPipeStdinWr);

		// Wait up to 60 seconds for the secedit.exe process to complete.  Note that this utility will not terminate
		// secedit.exe if it runs longer than 60 seconds -- it just won't wait around for it any longer than that.
		//TODO:  Consider increasing the wait time for secedit.exe to finish.
		DWORD dwWFSO = WaitForSingleObject(pi.hProcess, 60 * 1000);
		if ( WAIT_TIMEOUT == dwWFSO )
			m_logs.ErrOut() << L"Warning:  SECEDIT.EXE still running after 60 seconds." << endl;

		//TODO:  Verify that piped data is ANSI and not Unicode.
		bool bContinue = true, bStdoutWritten = false, bStderrWritten = false;
		// Read secedit.exe's stdout and write it to the output log.
		while (bContinue)
		{
			//TODO:  Convert CR to CR+LF
			char buf[10240];
			DWORD dwRead = 0;
			SecureZeroMemory(buf, sizeof(buf));
			if ( !ReadFile(hPipeStdoutRd, buf, sizeof(buf) - 8, &dwRead, NULL) || 0 == dwRead )
				bContinue = false;
			else
			{
				m_logs.Output() << buf;
				bStdoutWritten = true;
			}
		}
		bContinue = true;
		// Read secedit.exe's stderr and write it to the error log.
		while (bContinue)
		{
			//TODO:  Convert CR to CR+LF
			char buf[10240];
			DWORD dwRead = 0;
			SecureZeroMemory(buf, sizeof(buf));
			if ( !ReadFile(hPipeStderrRd, buf, sizeof(buf) - 8, &dwRead, NULL) || 0 == dwRead )
				bContinue = false;
			else
			{
				m_logs.ErrOut() << buf;
				bStderrWritten = true;
			}
		}
		// If any bytes written to these logs, append and EOL.
		if ( bStdoutWritten )
			m_logs.Output() << endl;
		if ( bStderrWritten )
			m_logs.ErrOut() << endl;
		// Now read the log file and append it to the output log
		if ( m_logs.Verbose() )
		{
			m_logs.Output() << endl << L"[[[ Security template log file output follows:  " << tempSecTemplateLog << L" ]]]" << endl;
			FILE * pFile = NULL;
			errno_t wfoRet = _wfopen_s(&pFile, tempSecTemplateLog, L"r, ccs=UNICODE");
			if ( 0 == wfoRet )
			{
				wchar_t linebuf[10240];
				while ( fgetws(linebuf, sizeof(linebuf)/sizeof(linebuf[0]) - 1, pFile) )
				{
					wchar_t * pEOL = wcschr(linebuf, L'\r');
					if ( pEOL ) *pEOL = L'\0';
					m_logs.Output() << linebuf << endl;
				}
				fclose(pFile);
			}
		}
		// If secedit.exe completed execution, write its exit code to the log.
		DWORD dwExitCode;
		if ( GetExitCodeProcess(pi.hProcess, &dwExitCode) && STILL_ACTIVE != dwExitCode )
		{
			m_logs.Output() 
				<< L"SECEDIT.EXE exited with exit code " << dwExitCode << endl;
			m_logs.Output() << Logs::szLine() << endl;
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	CloseHandle(hPipeStdoutRd);
	CloseHandle(hPipeStderrRd);

	return 0;
}

/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/


// Apply_LGPO_Delta.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Apply_LGPO_Delta.h"
#include "RegFileProcessor.h"
#include "SecTemplateProcessor.h"



const wchar_t * const szTitle =
	L"Apply Local Group Policies Deltas";

const wchar_t * const szSyntax =
	L"Command line syntax:\r\n"
	L"\r\n"
	L"Apply_LGPO_Delta.exe inputfile[...] [/log LogFile] [/error ErrorLogFile]  [/boot]\r\n"
	L"\r\n"
	L"(See the readme for more information)"
	;


// Display usage information, with optional text prepended.
inline void Usage(const wchar_t * szLeadInfo = NULL)
{
	wstringstream sInfo;
	if ( szLeadInfo && *szLeadInfo )
		sInfo << szLeadInfo << endl << endl;
	sInfo << szSyntax;
	MessageBoxW(NULL, sInfo.str().c_str(), szTitle, MB_ICONEXCLAMATION);
}


// Verify that we're running on a supported OS platform.
bool SupportabilityCheck()
{
	// No need for OS version check.
	//
	//// Supported-platform check.
	//wstring sErrMsg;
	//if ( !gb_PlatformCheck.IsSupported(sErrMsg) )
	//{
	//	MessageBoxW(NULL, sErrMsg.c_str(), szTitle, MB_ICONSTOP);
	//	return false;
	//}

	// Admin check
	// Verify that the user is running as admin.  (Keep the manifest "asInvoker", do the check in here.)
	if ( !IsUserAnAdmin() )
	{
		MessageBoxW(NULL, L"Error:  This utility requires administrative privileges.", szTitle, MB_ICONSTOP);
		return false;
	}

	return true;
}



// Validate and determine command line options.
// Return "true" if command line is valid, "false" otherwise.
bool ProcessCommandLine(filelist_t & regfiles, filelist_t & sectempfiles, bool & bDoReboot, Logs & logs )
{
	bDoReboot = false;
	regfiles.clear();
	sectempfiles.clear();
	try
	{
		for ( int argc = 1; argc < __argc; ++argc )
		{
			const wchar_t * argv = __wargv[argc];
			if ( argv && *argv )
			{
				if (0 == _wcsicmp(argv, L"/log") )
				{
					if ( ++argc < __argc )
					{
						logs.SetOutputFile(__wargv[argc]);
					}
					else
					{
						Usage(L"\"/log\" requires file name");
						return false;
					}
				}
				else
				if (0 == _wcsicmp(argv, L"/error") )
				{
					if ( ++argc < __argc )
					{
						logs.SetErrorFile(__wargv[argc]);
					}
					else
					{
						Usage(L"\"/error\" requires file name");
						return false;
					}
				}
				else
				if (0 == _wcsicmp(argv, L"/boot") )
				{
					bDoReboot = true;
				}
				else // arg specifies the name of a file.
				{
					// Open the file; determine whether it is a security template; if not, assume it's a registry policy file (custom format)
					HANDLE hFile = CreateFileW(argv, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					if ( INVALID_HANDLE_VALUE == hFile )
					{
						wstring sError = SysErrorMessageWithCode();
						wstringstream sInfo;
						sInfo << L"Cannot open input file \"" << argv << L"\":\r\n" << sError;
						Usage(sInfo.str().c_str());
						return false;
					}
					DWORD numread = 0;
					wchar_t buf[2];
					BOOL rfRet = ReadFile(hFile, buf, sizeof(buf), &numread, NULL);
					//DWORD dwLastErr = GetLastError();
					CloseHandle(hFile);
					if (!rfRet || sizeof(buf) != numread )
					{
						wstringstream sInfo;
						sInfo << L"Invalid file:  " << argv ;
						Usage(sInfo.str().c_str());
						return false;
					}
					// Security template can be ANSI or Unicode.  
					// Assume that security template is an ANSI or Unicode file that starts with '['.  Anything else, assume it's a registry-policy input file.
					// If Unicode, it will begin with Unicode Byte Order Mark (BOM) followed by Unicode '[' character.
					// If ANSI, it will just begin with ANSI '[':
					if ( 
						/* Unicode test */ ( 0xFEFF == buf[0] && L'[' == buf[1] ) ||
						/* ANSI test    */ ( '[' == *(const char *)(&buf[0]) )
						)
						sectempfiles.push_back(argv);
					else
						regfiles.push_back(argv);
				}
			}
		}
	}
	catch( const wchar_t * pszException )
	{
		Usage(pszException);
		return false;
	}
	catch( const wstring & sException )
	{
		Usage(sException.c_str());
		return false;
	}
	catch(...)
	{
		Usage(L"Unidentified error processing command line");
		return false;
	}

	if ( 0 == sectempfiles.size() && 0 == regfiles.size() )
	{
		Usage(L"At least one input file must be specified");
		return false;
	}

	return true;
}



int APIENTRY wWinMain(HINSTANCE , //hInstance,
                     HINSTANCE , //hPrevInstance,
                     LPWSTR    , //lpCmdLine,
                     int       ) //nCmdShow)
{
	if ( !SupportabilityCheck() )
		return -1;

	// bDoReboot:  whether to reboot when done
	bool bDoReboot = false;

	// logs:  structure to handle logging output
	Logs logs;
	int retval = 0;
	filelist_t regfiles, sectempfiles;

	if ( !ProcessCommandLine(regfiles, sectempfiles, bDoReboot, logs) )
		return -1;

	if ( logs.Verbose() )
	{
		logs.Output() << Logs::szLine() << endl;
		if ( !regfiles.empty() )
		{
			logs.Output() << L"INPUT FILES FOR REGISTRY-BASED POLICY:" << endl;
			for ( filelistIter_t iter = regfiles.begin(); iter != regfiles.end(); iter++ )
				logs.Output() << L"\t" << *iter << endl;
		}
		if ( !sectempfiles.empty() )
		{
			logs.Output() << L"SECURITY TEMPLATES:" << endl;
			for ( filelistIter_t iter = sectempfiles.begin(); iter != sectempfiles.end(); iter++ )
				logs.Output() << L"\t" << *iter << endl;
		}
		logs.Output() << Logs::szLine() << endl;
	}

	if ( !regfiles.empty() )
	{
		LocalGPO gpo;
		RegFilesProcessor regfilesproc(logs);
		regfilesproc.ProcessRegistryFiles(regfiles);
	}

	if ( !sectempfiles.empty() )
	{
		SecTemplateProcessor sectempProc(logs);
		sectempProc.ProcessSecurityTemplates(sectempfiles);
	}

	//TODO:  Don't reboot if any errors happen?  Badness:  rebooting with an error message displaying on the screen.  Error message not retained anywhere.
	if ( bDoReboot )
	{
		//TODO:  Capture and log any errors that occur here.
		RebootComputer();
	}

	// If error logging wasn't redirected to a file and any error information was logged, display it
	// in a message box dialog.
	const wstring & sErrOut = logs.ErrString();
	if ( sErrOut.length() > 0 )
		MessageBoxW(NULL, sErrOut.c_str(), szTitle, MB_ICONSTOP);

	return retval;
}


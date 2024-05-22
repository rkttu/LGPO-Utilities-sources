/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007-2009.  Microsoft Corporation.  All rights reserved.
*/

// Set_FDCC_LGPO.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RegPolProcessor.h"
#include "SecTemplateProcessor.h"


const wchar_t * const szTitle =
	L"Set FDCC Local Group Policies (Q1 2009)";

const wchar_t * const szSyntax =
	L"Command line syntax:\r\n"
	L"\r\n"
	L"Set_FDCC_LGPO.exe  [/Sec] [/log LogFile] [/error ErrorLogFile]  [/boot]\r\n"
	L"\r\n"
	L"/Sec - Sets security policy settings in addition to registry-based (registry.pol) settings.\r\n"
	L"\r\n"
	L"/log LogFile - Writes detailed results to a log file.  If this option is not specified, output is not logged nor displayed.\r\n"
	L"\r\n"
	L"/error ErrorLogFile - Writes error information to a log file.  If this option is not specified, error information is displayed in a message box.\r\n"
	L"\r\n"
	L"/boot - Reboot computer when completed."
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
	// Supported-platform check.
	wstring sErrMsg;
	if ( !gb_PlatformCheck.IsSupported(sErrMsg) )
	{
		MessageBoxW(NULL, sErrMsg.c_str(), szTitle, MB_ICONSTOP);
		return false;
	}

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
bool ProcessCommandLine(bool & bDoSecurity, bool & bDoReboot, Logs & logs )
{
	bDoSecurity = false;
	bDoReboot = false;
	try
	{
		for ( int argc = 1; argc < __argc; ++argc )
		{
			const wchar_t * argv = __wargv[argc];
			if ( argv && *argv )
			{
				if (0 == _wcsicmp(argv, L"/Sec") )
				{
					bDoSecurity = true;
				}
				else
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
				else
				{
					wstringstream sInfo;
					sInfo << L"Invalid command line:  " << GetCommandLineW();
					Usage(sInfo.str().c_str());
					return false;
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
	return true;
}

int APIENTRY wWinMain(HINSTANCE , //hInstance,
                     HINSTANCE , //hPrevInstance,
                     LPWSTR    , //lpCmdLine,
                     int       ) //nCmdShow)
{
	if ( !SupportabilityCheck() )
		return -1;

	// bDoSecurity:  whether to process security templates
	bool bDoSecurity = false;
	// bDoReboot:  whether to reboot when done
	bool bDoReboot = false;
	// logs:  structure to handle logging output
	Logs logs;
	int retval;

	if ( !ProcessCommandLine(bDoSecurity, bDoReboot, logs) )
		return -1;

	// Process the embedded registry.pol files:
	RegPolProcessor regPol(logs);
	retval = regPol.ProcessEmbeddedRegistryPolFiles();

	if ( bDoSecurity )
	{
		// Process the embedded security template files
		SecTemplateProcessor secProc(logs);
		int ret2 = secProc.ProcessEmbeddedSecurityTemplates();
		if ( 0 != ret2 )
			retval = ret2;
	}

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


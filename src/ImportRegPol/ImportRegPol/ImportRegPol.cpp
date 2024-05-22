/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2009.  Microsoft Corporation.  All rights reserved.
*/


// ImportRegPol.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ImportRegPol.h"
#include "RegPolProcessor.h"


const wchar_t * const szTitle =
	L"Import Registry.Pol Content";

const wchar_t * const szSyntax =
	L"Command line syntax:\r\n"
	L"\r\n"
	L"ImportRegPol.exe -m|-u path\\registry.pol [/parseOnly] [/log LogFile] [/error ErrorFile] [/boot]\r\n"
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


enum eConfig_t
{
	eConfig_NotSet,
	eConfig_Machine,
	eConfig_User
};

// Validate and determine command line options.
// Return "true" if command line is valid, "false" otherwise.
bool ProcessCommandLine(eConfig_t & eConfig, wstring & regpol, bool & bParseOnly, bool & bDoReboot, Logs & logs )
{
	eConfig = eConfig_NotSet;
	regpol.clear();
	bParseOnly = false;
	bDoReboot = false;
	try
	{
		for ( int argc = 1; argc < __argc; ++argc )
		{
			const wchar_t * argv = __wargv[argc];
			if ( argv && *argv )
			{
				// Setting machine/user config and specifying the file name
				if (0 == _wcsicmp(argv, L"-m") || 0 == _wcsicmp(argv, L"/m") || 0 == _wcsicmp(argv, L"-u") || 0 == _wcsicmp(argv, L"/u"))
				{
					// Verify that it hasn't been set already
					if ( eConfig_NotSet != eConfig )
					{
						Usage(L"-m or -u can be specified only one time.");
						return false;
					}

					// Specify machine or user config
					if (0 == _wcsicmp(argv, L"-m") || 0 == _wcsicmp(argv, L"/m"))
					{
						eConfig = eConfig_Machine;
					}
					else if (0 == _wcsicmp(argv, L"-u") || 0 == _wcsicmp(argv, L"/u"))
					{
						eConfig = eConfig_User;
					}

					// Now pick up the file name
					if ( ++argc < __argc )
					{
						regpol = __wargv[argc];
						// Verify that the next parameter is a file
						const DWORD dwInvalidAttrs = 
							FILE_ATTRIBUTE_DEVICE |
							FILE_ATTRIBUTE_DIRECTORY |
							FILE_ATTRIBUTE_OFFLINE;
						DWORD dwFileAttr = GetFileAttributesW(regpol.c_str());
						if ( INVALID_FILE_ATTRIBUTES == dwFileAttr || ( 0 != (dwInvalidAttrs & dwFileAttr) ) )
						{
							wstringstream sInfo;
							sInfo << L"Cannot open input file \"" << regpol << L"\"";
							Usage(sInfo.str().c_str());
							return false;
						}
					}
					else
					{
						Usage(L"-m or -u must be followed by file name");
						return false;
					}
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
				if (0 == _wcsicmp(argv, L"/parseOnly" ))
				{
					bParseOnly = true;
				}
				else // unrecognized/invalid command line option
				{
					wstringstream sInfo;
					sInfo << L"Unrecognized command line option:  " << argv;
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

	if ( eConfig_NotSet == eConfig )
	{
		Usage(L"Configuration and file not specified.");
		return false;
	}

	return true;
}


int APIENTRY wWinMain(HINSTANCE , //hInstance,
                     HINSTANCE , //hPrevInstance,
                     LPWSTR    , //lpCmdLine,
                     int       ) //nCmdShow)
{
	// No check for admin rights here -- parseOnly doesn't require admin.
	// If admin rights are required, the utility will report the failure when it occurs.
	
	eConfig_t eConfig = eConfig_NotSet;
	wstring sRegPol;
	bool bParseOnly = false, bDoReboot = false;
	Logs logs;

	// Turn off Wow64 FS redirection while processing the command line
	Wow64FsRedirection fsRedir;
	fsRedir.Disable();
	if ( !ProcessCommandLine(eConfig, sRegPol, bParseOnly, bDoReboot, logs) )
		return -1;
	fsRedir.Revert();

	RegPolProcessor regPolProc(logs, bParseOnly);
	int retval = regPolProc.ProcessRegistryPolFile((eConfig_User == eConfig), sRegPol.c_str());

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


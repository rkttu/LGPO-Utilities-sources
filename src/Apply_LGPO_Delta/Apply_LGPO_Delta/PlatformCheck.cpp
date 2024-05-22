/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#include "StdAfx.h"
#include "PlatformCheck.h"

// Create a global instance of this utility class.
PlatformCheck gb_PlatformCheck;

// Do all the checks one time when this class is instantiated and save results in member variables.
PlatformCheck::PlatformCheck()
: m_bIsXP(false), m_bIsVista(false)
{
	OSVERSIONINFOEXW osver;
	osver.dwOSVersionInfoSize = sizeof(osver);
	if ( !GetVersionEx((LPOSVERSIONINFOW)&osver) )
	{
		//TODO:  Handle this strange failure
	}
	else
	{
		if (
			// Windows XP, Service Pack 2 or higher
			5 == osver.dwMajorVersion &&
			1 == osver.dwMinorVersion &&
			2 <=  osver.wServicePackMajor
			)
		{
			m_bIsXP = true;
		}
		else
		if (
			// Windows Vista, RTM or higher
			6 == osver.dwMajorVersion &&
			0 == osver.dwMinorVersion )
		{
			m_bIsVista = true;
		}
		else
		{
			// Unsupported platform.  Write information to m_sErrorMsg
			wstringstream sErrText;
			sErrText
				<< L"This program requires Windows XP (SP2 or higher) or Windows Vista (RTM or higher)." << endl
				<< endl
				<< L"(Detected Windows Version " << osver.dwMajorVersion << L"." << osver.dwMinorVersion << L", Service Pack " << osver.wServicePackMajor << L").";
			m_sErrorMsg = sErrText.str();
		}
	}
}

PlatformCheck::~PlatformCheck()
{
}


bool PlatformCheck::IsSupported(wstring & sErrorMsg)
{
	if ( m_bIsXP || m_bIsVista )
		return true;
	else
	{
		sErrorMsg = m_sErrorMsg;
		return false;
	}
}

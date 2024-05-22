/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#include "StdAfx.h"
#include "Logs.h"

// By default, output/error not redirected.
Logs::Logs()
: m_psOutput(&m_strDummy),
  m_psError(&m_strErrorLog),
  m_bVerbose(false)
{
}

Logs::~Logs()
{
	if ( m_fOutput.is_open() )
		m_fOutput.close();
	if ( m_fError.is_open() )
		m_fError.close();
}


// Redirect output to a file; throw an error if an invalid file is specified.
void Logs::SetOutputFile(const wchar_t * szOutputFile)
{
	if ( m_fOutput.is_open() )
	{
		m_fOutput.close();
		m_bVerbose = false;
	}
	m_fOutput.open(szOutputFile, ios_base::out | ios_base::app);
	m_psOutput = &m_fOutput;
	ValidateFile(szOutputFile);
	m_bVerbose = true;
	//TODO:  Insert a timestamp into the file
}

// Redirect error output to a file; throw an error if an invalid file is specified.
void Logs::SetErrorFile(const wchar_t * szErrorFile)
{
	if ( m_fError.is_open() )
		m_fError.close();
	m_fError.open(szErrorFile, ios_base::out | ios_base::app);
	m_psError = &m_fError;
	ValidateFile(szErrorFile);
	//TODO:  Insert a timestamp into the file
}

// File validation (not a complete check)
void Logs::ValidateFile(const wchar_t * szFile)
{
	const DWORD dwInvalidAttrs = 
		FILE_ATTRIBUTE_DEVICE |
		FILE_ATTRIBUTE_DIRECTORY |
		FILE_ATTRIBUTE_OFFLINE;
	DWORD dwFileAttr = GetFileAttributesW(szFile);
	if ( INVALID_FILE_ATTRIBUTES == dwFileAttr || ( 0 != (dwInvalidAttrs & dwFileAttr) ) )
	{
		wstring sError(L"Invalid file:  ");
		sError += szFile;
		throw sError;
	}
}

// Un-redirect error output.
void Logs::SetErrorToMsgBox()
{
	if ( m_fError.is_open() )
		m_fError.close();
	m_psError = &m_strErrorLog;
}



/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

// Class to encapsulate informational and error logging; logs can
// be redirected to files.
class Logs
{
public:
	Logs();
	~Logs();

	// Redirects output to a file
	void SetOutputFile(const wchar_t * szOutputFile);
	// Redirects error output to a file
	void SetErrorFile(const wchar_t * szErrorFile);
	// Resets error information not to go to a file.
	void SetErrorToMsgBox();

	// Get the output stream (may be redirected)
	wostream & Output()
	{ return *m_psOutput; }
	// Get the error output stream (may be redirected)
	wostream & ErrOut()
	{ return *m_psError; }
	// Get the non-redirected error output (if any)
	wstring ErrString()
	{ return m_strErrorLog.str(); }

	// "Verbose" flag gets set if output is redirected to a file.
	bool Verbose() const
	{ return m_bVerbose; }

	static const wchar_t * szLine() { return L"----------------------------------------------------------------------"; };

private:
	// Implementation
	void ValidateFile(const wchar_t * szFile);

private:
	// Members
	// Un-redirected error output - captured as a string stream
	wstringstream m_strErrorLog;
	// Un-redirected output - ultimately ignored
	wstringstream m_strDummy;
	// Points to current output/error streams
	wostream * m_psOutput;
	wostream * m_psError;
	// File output streams (if redirected)
	wofstream m_fOutput, m_fError;
	// Flag gets set if output is redirected to a file.
	bool m_bVerbose;

private:
	// Not implemented
	Logs(const Logs &);
	Logs & operator = (const Logs &);
};

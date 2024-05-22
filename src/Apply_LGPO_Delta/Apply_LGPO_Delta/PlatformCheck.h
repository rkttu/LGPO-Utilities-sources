/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

// Utility class to validate platform that the program is running on.
class PlatformCheck
{
public:
	PlatformCheck();
	~PlatformCheck();

	// Returns true/false; if false, returns error information in sErrorMsg
	bool IsSupported(wstring & sErrorMsg);

	bool IsXP() const { return m_bIsXP; }
	bool IsVista() const { return m_bIsVista; }

private:
	bool m_bIsXP, m_bIsVista;
	wstring m_sErrorMsg;

private:
	// Not implemented
	PlatformCheck(const PlatformCheck &);
	PlatformCheck & operator = (const PlatformCheck &);
};

extern PlatformCheck gb_PlatformCheck;

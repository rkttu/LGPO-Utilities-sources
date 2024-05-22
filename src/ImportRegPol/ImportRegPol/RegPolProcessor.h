/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

#include "Logs.h"
#include "LocalGPO.h"


// Process embedded registry.pol files.
class RegPolProcessor
{
public:
	RegPolProcessor(Logs & logs, bool bParseOnly);
	~RegPolProcessor();

	int ProcessRegistryPolFile(bool bUserConfig, const wchar_t * szRegPol);

private:
	// Implementation
	int ProcessPolicyFile(bool bUser, const LPBYTE pMappedView, const LPBYTE pMaxAddr);
	int ProcessPolicyFileImpl(bool bUser, const LPBYTE pMappedView, const LPBYTE pMaxAddr);
	int ProcessPolicyFileData(bool bUser, LPBYTE & pCurr, const LPBYTE pMaxAddr, LocalGPO & gpo);
	int ProcessPolicyCommand(bool bUser, LocalGPO & gpo, const wstring & sKey, const wstring & sValue, const DWORD dwRegType, const DWORD dwRegSize, const LPBYTE pData, bool bParseOnly);

private:
	// Member vars
	Logs & m_logs;
	bool m_bParseOnly;
	int m_retval;

private:
	// Not implemented
	RegPolProcessor(const RegPolProcessor &);
	RegPolProcessor & operator = (const RegPolProcessor &);

};

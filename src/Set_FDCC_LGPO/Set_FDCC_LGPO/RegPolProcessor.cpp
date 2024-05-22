/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

#include "StdAfx.h"
#include "RegPolProcessor.h"

RegPolProcessor::RegPolProcessor(Logs & logs)
: m_logs(logs)
{
}

RegPolProcessor::~RegPolProcessor()
{
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

static const wchar_t * szResType = L"registry.pol";

int RegPolProcessor::ProcessEmbeddedRegistryPolFiles()
{
	m_retval = 0;
	// Enumerate embedded resources matching the type name specified by szResType.
	EnumResourceNames(NULL, szResType, stEnumResNameProc, (LONG_PTR)this);
	return m_retval;
}


// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

// static method -- last parameter is a pointer to the class instance to use as "this"
// This function will be called once for each matching resource.
BOOL CALLBACK RegPolProcessor::stEnumResNameProc(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR pThis)
{
	// hModule will be NULL
	// lpszType should be "registry.pol"
	// pThis is a pointer to the class instance to use.  Cast it and call the member function.
	return ((RegPolProcessor*)pThis)->ProcessResource(lpszType, lpszName);
}

// Called once for each embedded resource of the "registry.pol" resource type.
BOOL RegPolProcessor::ProcessResource(LPCTSTR lpszType, LPTSTR lpszName)
{
	// Validate the lpszType parameter (should never fail, but check anyway)
	if ( 0 != _wcsicmp(szResType, lpszType) )
	{
		m_logs.ErrOut() << L"Unexpected resource type:  " << lpszType << endl;
		return TRUE;
	}

	// Determine whether we want this resource, based on its name.
	// Name will be of the form:  {OS}.{SECTION}.n
	// Where OS = {BOTH|VISTA|XP}, SECTION = {MACHINE|USER}, and "n" is an arbitrary integer.
	bool bGetIt =
		(                               ( 0 == _wcsnicmp(lpszName, L"BOTH.",  5) ) ) ||
		( gb_PlatformCheck.IsXP() &&    ( 0 == _wcsnicmp(lpszName, L"XP.",    3) ) ) ||
		( gb_PlatformCheck.IsVista() && ( 0 == _wcsnicmp(lpszName, L"VISTA.", 6) ) );
	if ( !bGetIt )
		return TRUE;

	const wchar_t * pDot = wcschr(lpszName, L'.') + 1;

	// Determine whether this is User or Machine policy.
	bool bUser = (0 == _wcsnicmp(pDot, L"USER.", 5));

	m_logs.Output() 
		<< L"; " << Logs::szLine() << endl
		<< L"; PROCESSING " << (bUser ? L"USER" : L"COMPUTER") << " POLICY:  " << endl << endl;

	HRSRC hRsrc = NULL;
	HGLOBAL hLoadedResource = NULL;
	LPBYTE pbResource = NULL;
	DWORD dwResSize = 0;

	// Find and load the resource into memory.
	hRsrc = FindResourceW(NULL, lpszName, lpszType);
	if ( hRsrc )
		hLoadedResource = LoadResource( NULL, hRsrc );
	if ( hLoadedResource )
		pbResource = (LPBYTE)LockResource(hLoadedResource);
	if ( hRsrc )
		dwResSize = SizeofResource(NULL, hRsrc);
	if ( pbResource && dwResSize )
	{
		// Do the real work here:
		int retval = ProcessPolicyFile(bUser, pbResource, pbResource + dwResSize);
		if ( 0 != retval )
			m_retval = retval;
	}
	else
	{
		DWORD dwLastErr = GetLastError();
		m_logs.ErrOut() << L"Error loading resource " << lpszName << L"; " << SysErrorMessageWithCode(dwLastErr) << endl;
	}
	if ( hLoadedResource )
		FreeResource(hLoadedResource);

	return TRUE;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// Wrapper function with SEH around use of mapped view.
int RegPolProcessor::ProcessPolicyFile(bool bUser, const LPBYTE pMappedView, const LPBYTE pMaxAddr)
{
	int retval = 0;
	__try
	{
		retval = ProcessPolicyFileImpl(bUser, pMappedView, pMaxAddr);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		m_logs.ErrOut() << L"An exception occurred while processing the policy file." << endl;
		retval = -5;
	}
	return retval;
}



// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

// Macros suck.  (But a "return" statement in an inline function won't exit from the calling function, which is what we want.)
#define CHECKPOINTER(p)  { if ( ((LPBYTE)(p)) > pMaxAddr ) { m_logs.ErrOut() << L"Parser error.  Unexpected end of file." << endl; return -2; } }

int RegPolProcessor::ProcessPolicyFileImpl(bool bUser, const LPBYTE pMappedView, const LPBYTE pMaxAddr)
{
	// Instantiate wrapper object for local group policy.
	LocalGPO gpo;

	// Current pointer into the mapped view of the registry.pol file.
	LPBYTE pCurr = pMappedView;

	// Validate file header stuff in the registry.pol.
	const DWORD dwRegFileSignature = 0x67655250;
	const DWORD dwRegFileVersion = 1;

	DWORD * pdw = (DWORD *)pCurr;
	if ( dwRegFileSignature != pdw[0] || dwRegFileVersion != pdw[1] )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected headers not found." << endl;
		return -2;
	}
	pCurr = (LPBYTE)(&pdw[2]);
	CHECKPOINTER(pCurr);

	// Process the body (in ProcessPolicyFileData)
	int retval;
	do
	{
		// If at end of file, save and exit.
		if ( pMaxAddr == pCurr )
		{
			HRESULT hr = gpo.Save();
			if ( SUCCEEDED(hr) )
			{
				m_logs.Output() 
					<< L"; " << (bUser ? L"USER" : L"COMPUTER") << L" POLICY SAVED." << endl
					<< L"; " << Logs::szLine() << endl << endl ;
				return 0;
			}
			else
			{
				// Write to output log and error log
				m_logs.Output() << L";;; ERROR:  " << (bUser ? L"User" : L"Computer") << L" policy save failed." << endl
					<< SysErrorMessageWithCode(hr) << endl;
				m_logs.ErrOut() << (bUser ? L"User" : L"Computer") << L" policy save failed." << endl
					<< SysErrorMessageWithCode(hr) << endl;
				return -7;
			}
		}
	}
	while ( 0 == (retval = ProcessPolicyFileData(bUser, pCurr, pMaxAddr, gpo)) );

	return retval;
}



// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

int RegPolProcessor::ProcessPolicyFileData(bool bUser, LPBYTE & pCurr, const LPBYTE pMaxAddr, LocalGPO & gpo)
{
	CHECKPOINTER(pCurr);

	wstring sKey, sValue;
	DWORD dwRegType, dwRegSize;
	sKey.reserve(128);
	sValue.reserve(128);

	/*
		Registry Policy File Format is documented here:  http://msdn2.microsoft.com/en-us/library/aa374407(VS.85).aspx

		The body consists of registry values in the following format.
		[key;value;type;size;data] 
		If value, type, size, or data are missing or zero, only the registry key is created.

		"Missing" in the above is inaccurate.
		Key/Value are NUL-terminated Unicode strings.  Search for NUL-terminator -- semicolon can be part of key/value name.
		type and size are always DWORD values.
		data is always the number of bytes specified by "size".
	*/

	// Opening [ separator
	const wchar_t * pCh = (const wchar_t *)pCurr;
	if ( L'[' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected '[', found character " << HEX(*pCh) << endl;
		return -2;
	}
	
	// Key
	while ( ++pCh < (const wchar_t *)pMaxAddr && L'\0' != *pCh )
		sKey += *pCh;
	CHECKPOINTER(pCh);

	// semicolon ;
	++pCh;
	CHECKPOINTER(pCh);
	if ( L';' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected ';', found character " << HEX(*pCh) << endl;
		return -2;
	}

	// Value
	while ( ++pCh < (const wchar_t *)pMaxAddr && L'\0' != *pCh )
		sValue += *pCh;
	CHECKPOINTER(pCh);

	// semicolon ;
	++pCh;
	CHECKPOINTER(pCh);
	if ( L';' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected ';', found character " << HEX(*pCh) << endl;
		return -2;
	}

	// Type
	const DWORD * pdw = (const DWORD *)++pCh;
	CHECKPOINTER(pdw);
	dwRegType = *pdw;

	// semicolon ;
	pCh = (const wchar_t *)++pdw;
	CHECKPOINTER(pCh);
	if ( L';' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected ';', found character " << HEX(*pCh) << endl;
		return -2;
	}

	// Size
	pdw = (const DWORD *)++pCh;
	CHECKPOINTER(pdw);
	dwRegSize = *pdw;

	// semicolon ;
	pCh = (const wchar_t *)++pdw;
	CHECKPOINTER(pCh);
	if ( L';' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected ';', found character " << HEX(*pCh) << endl;
		return -2;
	}

	// Data - dependent on Type and Size.  Validate that it's still valid memory out there.
	pCurr = (LPBYTE)++pCh;
	CHECKPOINTER(pCurr + dwRegSize);
	// Set a reasonable limit on a reg value data item:  1MB
	if ( dwRegSize > (1024 * 1024) )
	{
		m_logs.ErrOut() << L"Specified registry data size exceeds normal expectations - too large:  " << dwRegSize << endl;
		return -3;
	}
	LPBYTE pData = (LPBYTE)_alloca(dwRegSize);
	if ( !pData )
	{
		m_logs.ErrOut() << L"Memory allocation failed." << endl;
		return -4;
	}
	memcpy(pData, pCurr, dwRegSize);
	pCurr += dwRegSize;

	// Separator
	pCh = (const wchar_t *)pCurr;
	CHECKPOINTER(pCurr);
	if ( L']' != *pCh )
	{
		m_logs.ErrOut() << L"Invalid file format.  Expected ']', found character " << HEX(*pCh) << endl;
		return -2;
	}

	pCurr = (LPBYTE)++pCh;
	CHECKPOINTER(pCurr);

	return ProcessPolicyCommand(bUser, gpo, sKey, sValue, dwRegType, dwRegSize, pData);
}



// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

// Constant string definitions
const wstring csDeleteValues  = L"**DeleteValues";	// A semicolon-delimited list of values to delete. Use as a value of the associated key. 
const wstring csDel           = L"**Del.";			// Del.valuename Deletes a single value. Use as a value of the associated key. 
const wstring csDelVals       = L"**DelVals";		// Deletes all values in a key. Use as a value of the associated key. 
const wstring csDeleteKeys    = L"**DeleteKeys";	// A semicolon-delimited list of keys to delete. Example: **DeleteKeys NoRun;NoFind
const wstring csSecureKey     = L"**SecureKey";		// **SecureKey=1 secures the key, giving administrators and the system full control, and giving users read-only access. **SecureKey=0 resets access to the key to whatever is set on the root. For more information, see Access Rights and Access Masks. 



// ----------------------------------------------------------------------------------------------------

int RegPolProcessor::ProcessPolicyCommand(bool bUser, LocalGPO & gpo, const wstring & sKey, const wstring & sValue, const DWORD dwRegType, const DWORD dwRegSize, const LPBYTE pData)
{
	const bool bParseOnly = false; // Function body copied over from ImportRegPol, which can do parse-only.

	if ( !bParseOnly )
	{
		HKEY hBaseKey = NULL, hSubKey = NULL;
		LONG rv;
		// Create key for User or Machine policy
		hBaseKey = bUser ? gpo.UserKey() : gpo.ComputerKey();
		hSubKey = NULL;
		rv = RegCreateKeyExW(hBaseKey, sKey.c_str(), 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hSubKey, NULL);
		if ( NO_ERROR != rv )
		{
			m_logs.ErrOut() 
				<< L"Error:  Unable to create policy key " << sKey << endl
				<< SysErrorMessageWithCode(rv) << endl;
			return -6;
		}

		// Write the content from the input registry.pol directly into the local policy.
		// Special commands will be processed by group policy.
		rv = RegSetValueExW(hSubKey, sValue.c_str(), 0, dwRegType, pData, dwRegSize);
		RegCloseKey(hSubKey);
		if ( NO_ERROR != rv )
		{
			m_logs.ErrOut() 
				<< L"Error:  Unable to set policy value \"" << sValue << L"\" in key " << sKey << endl
				<< SysErrorMessageWithCode(rv) << endl;
			return -6;
		}
	}

	// If not writing to a log, we're done here.
	if ( !m_logs.Verbose() )
		return 0;

	// Log is written in a way that supports use of Apply_LGPO_Delta.  Comment out log entries that Apply_LGPO_Delta can't support.
	bool bSupportedByApplyLGPODelta = false;
	wstring sValueName;
	wstringstream sAction;
	bool bDebugParams = true;

	// --------------------------------------------------------------------------------------------------------------
	// Special case:  create key, no value specified.  Not in the documentation, but plenty of examples in 
	// valid registry.pol files.
	if ( sValue.length() == 0 && REG_NONE == dwRegType )
	{
		// Create the key, no values
		bSupportedByApplyLGPODelta = true;
		sAction << L"CREATEKEY";
		sValueName = L"*";
	}
	else
	// --------------------------------------------------------------------------------------------------------------
	/*
		Special cases -- command starting with "**"
		Case-insensitive comparison:
			**DeleteValues A semicolon-delimited list of values to delete. Use as a value of the associated key. 
			**Del.valuename Deletes a single value. Use as a value of the associated key. 
			**DelVals Deletes all values in a key. Use as a value of the associated key. 
			**DeleteKeys A semicolon-delimited list of keys to delete. Example: **DeleteKeys NoRun;NoFind
			**SecureKey **SecureKey=1 secures the key, giving administrators and the system full control, and giving users read-only access. **SecureKey=0 resets access to the key to whatever is set on the root. For more information, see Access Rights and Access Masks. 
	*/
	if ( 0 == _wcsnicmp(sValue.c_str(), csDeleteValues.c_str(), csDeleteValues.length()) )
	{
		// A semicolon-delimited list of values to delete. Use as a value of the associated key.
		sAction << L"[Delete specified values:  " << sValue << L"]";
	}
	else
	if ( 0 == _wcsnicmp(sValue.c_str(), csDel.c_str(), csDel.length()) )
	{
		// Del.valuename Deletes a single value. Use as a value of the associated key.
		// Apply_LGPO_Delta can handle this command
		bSupportedByApplyLGPODelta = true;
		const wstring & sValToDelete = sValue.substr(csDel.length());
		sValueName = sValToDelete;
		sAction << L"DELETE";
	}
	else
	if ( 0 == _wcsnicmp(sValue.c_str(), csDelVals.c_str(), csDelVals.length()) )
	{
		bSupportedByApplyLGPODelta = true;
		sAction << L"DELETEALLVALUES";
		sValueName = L"*";
	}
	else
	if ( 0 == _wcsnicmp(sValue.c_str(), csDeleteKeys.c_str(), csDeleteKeys.length()) )
	{
		// A semicolon-delimited list of keys to delete. Example: **DeleteKeys NoRun;NoFind
		sAction << L"[Delete specified subkeys:  " << sValue << L"]";
	}
	else
	if ( 0 == _wcsnicmp(sValue.c_str(), csSecureKey.c_str(), csSecureKey.length()) )
	{
		// **SecureKey=1 secures the key, giving administrators and the system full control, and giving users read-only access. **SecureKey=0 resets access to the key to whatever is set on the root. For more information, see Access Rights and Access Masks.
		sAction << L"[Set security on the key:  " << sValue << L"]";
	}
	else
	// --------------------------------------------------------------------------------------------------------------
	// Remaining cases:  create the named registry value
	{
		bDebugParams = false;
		sValueName = sValue;
		switch(dwRegType)
		{
		case REG_SZ:
			bSupportedByApplyLGPODelta = true;
			sAction << L"SZ:" << (const wchar_t*)pData;
			break;
		case REG_EXPAND_SZ:
			bSupportedByApplyLGPODelta = true;
			sAction << L"EXSZ:" << (const wchar_t*)pData;
			break;
		case REG_DWORD:
			bSupportedByApplyLGPODelta = true;
			sAction << L"DWORD:" << *(const DWORD*)pData;
			break;
		case REG_BINARY:
			sAction << L"REG_BINARY (data not shown; size = " << dwRegSize << L")";
			break;
		case REG_MULTI_SZ:
			sAction << L"REG_MULTI_SZ (data not shown; size = " << dwRegSize << L")";
			break;
		default:
			sAction << L"Data type " << HEX(dwRegType) << L" (data not shown; size = " << dwRegSize << L")";
			break;
		}
	}

	// Write results to the log
	if ( bSupportedByApplyLGPODelta )
	{
		m_logs.Output() 
			<< (bUser ? L"User": L"Computer") << endl
			<< sKey << endl
			<< sValueName << endl
			<< sAction.str() << endl
			<< endl;
	}
	else
	{
		// This item can't be used as input to Apply_LGPO_Delta, so comment it out.
		m_logs.Output() 
			<< L";;; ---- Commented out because Apply_LGPO_Delta cannot process this command" << endl
			<< L"; " << (bUser ? L"User": L"Computer") << endl
			<< L"; " << sKey << endl;
		if ( sValue.length() > 0 )
			m_logs.Output() << L"; " << sValue << endl;
		m_logs.Output()
			<< L"; " << sAction.str() << endl
			<< endl;
	}

	//if ( bDebugParams )
	//{ // const wstring & sKey, const wstring & sValue, const DWORD dwRegType, const DWORD dwRegSize, const LPBYTE pData
	//	m_logs.Output() 
	//		<< L";;; ---- PARAMETERS" << endl
	//		<< L"; Key = " << sKey << endl
	//		<< L"; Value = " << sValue << endl
	//		<< L"; Type = " << dwRegType << endl
	//		<< L"; Size = " << dwRegSize << endl
	//		<< L"; Data =";
	//	for ( DWORD ix = 0; ix < dwRegSize; ++ix )
	//		m_logs.Output() << L" " << HEX(pData[ix], 2);
	//	m_logs.Output() << endl << endl;
	//}

	return 0;
}


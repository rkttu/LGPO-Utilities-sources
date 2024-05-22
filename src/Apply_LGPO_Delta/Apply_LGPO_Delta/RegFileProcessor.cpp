/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/


#include "StdAfx.h"
#include "RegFileProcessor.h"

RegFilesProcessor::RegFilesProcessor(Logs & logs)
: m_logs(logs)
{
}

RegFilesProcessor::~RegFilesProcessor()
{
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

int RegFilesProcessor::ProcessRegistryFiles(const filelist_t & regfiles)
{
	// Input is a list of file names.  Process each in turn.
	for ( filelistIter_t iter = regfiles.begin(); iter != regfiles.end(); iter++ )
	{
		ProcessRegistryFile(*iter);
	}
	return 0;
}

bool RegFilesProcessor::ProcessRegistryFile(const wstring & regfile)
{
	LocalGPO gpo;
	FILE * fRegfile = NULL;
	// Open the file as Unicode or ANSI (assuming ANSI if not Unicode)
	bool bUnicode = IsUnicodeFile(regfile);
	const wchar_t * szMode = (bUnicode ? L"rt, ccs=UNICODE" : L"rt");
	errno_t errNum = _wfopen_s(&fRegfile, regfile.c_str(), szMode);
	if ( 0 != errNum )
	{
		m_logs.ErrOut()
			<< L"Unable to open input file:  " << regfile << endl;
		if ( ENOENT == errNum )
			m_logs.ErrOut() << L"File not found." << endl;
		else
			m_logs.ErrOut() << L"(errno=" << errNum << L")" << endl;
		return false;
	}

	m_logs.Output() << L"PROCESSING INPUT FILE FOR REGISTRY-BASED POLICY:  " << regfile << endl << endl;

	// Process each entry found in the file
	prfeRet_t prfeRet;
	do
	{
		prfeRet = ProcessRegistryFileEntry(fRegfile, gpo);
	}
	while ( prfeRet_EntryProcessed == prfeRet );

	fclose(fRegfile);

	// "No more entries" is the normal terminating condition.  Save and apply the policy entries processed so far.
	if ( prfeRet_NoMoreEntries == prfeRet )
	{
		HRESULT hr = gpo.Save();
		if ( SUCCEEDED(hr) )
		{
			m_logs.Output() 
				<< L"POLICY SAVED." << endl
				<< Logs::szLine() << endl << endl ;
			return true;
		}
		else
		{
			m_logs.Output() << L"ERROR:  Policy save failed." << endl
				<< SysErrorMessageWithCode(hr) << endl;
			m_logs.ErrOut() << L"ERROR:  Policy save failed." << endl
				<< SysErrorMessageWithCode(hr) << endl;
			return false;
		}
	}
	else
	{
		// Abnormal terminating conditions - policy for this file will not be saved.
		wchar_t * szErrorText = L"(Unknown cause)";
		switch(prfeRet)
		{
		case prfeRet_FormatError:
			szErrorText = L"file format error";
			break;
		case prfeRet_FileReadError:
			szErrorText = L"file read error";
			break;
		case prfeRet_RegAccessError:
			szErrorText = L"Registry access error";
			break;
		}
		m_logs.ErrOut() << L"Policy processing aborted due to " << szErrorText << endl;
		return false;
	}
}

/*
	RegFilesProcessor::ProcessRegistryFileEntry finds the next policy entry in the file and processes it.

	Format for each entry:
		First line must be "Computer" or "User"
		Second line is the name of a registry key
		Third line is the name of a registry value (can be a "dummy" value such as "*" for the DELETEALLVALUES or CREATEKEY actions).
		Fourth line is an action:
			* "DELETE" to delete the registry value (sets policy to "Not configured")
			* "DWORD:n" to set the registry value to REG_DWORD value "n" (where "n" is a valid REG_DWORD value)
			* "SZ:text" to set the registry value to REG_SZ value "text" (where "text" is arbitrary text)
			* "EXSZ:text" to set the registry value to REG_EXPAND_SZ value "text" (where "text" is arbitrary text)
			* "DELETEALLVALUES" to delete all values from the key
			* "CREATEKEY" to create the key but create no values.

		Lines between entries can be empty or start with ";" to indicate a comment.
*/
RegFilesProcessor::prfeRet_t RegFilesProcessor::ProcessRegistryFileEntry(FILE*fp, LocalGPO & gpo)
{
	gnlRet_t gnlRet;
	wstring sConfig, sKeyName, sValueName, sAction;

	// Get to the next entry or end of the file.
	// Next entry starts with the configuration:  Computer or User
	bool bHaveConfigLine = false, bComputerConfig = false;
	while ( !bHaveConfigLine )
	{
		// Process empty lines and/or comment lines until Configuration found or end-of-file.
		gnlRet = GetNextLine(fp, sConfig);
		if ( gnlRet_EOF == gnlRet )
			// End of the file -- no more entries
			return prfeRet_NoMoreEntries;
		else if ( gnlRet_LineRead == gnlRet )
		{
			// Look for "Computer" or "User", case insensitive; no whitespace allowed
			if ( 0 == _wcsicmp(sConfig.c_str(), L"Computer") )
			{
				bComputerConfig = true;
				bHaveConfigLine = true;
			}
			else if ( 0 == _wcsicmp(sConfig.c_str(), L"User") )
			{
				bComputerConfig = false;
				bHaveConfigLine = true;
			}
			// Otherwise, line must be empty or start with ";"
			else if ( sConfig.length() != 0 && sConfig[0] != L';' )
			{
				m_logs.ErrOut()
					<< L"Format error:  expected \"Computer\" or \"User\"; found \""
					<< sConfig
					<< L"\"" << endl;
				return prfeRet_FormatError;
			}
		}
		else
		{
			// file read error or other unexpected return
			m_logs.ErrOut()
				<< L"File read error" << endl;
			return prfeRet_FileReadError;
		}
	}

	// Next line must be the registry key
	gnlRet = GetNextLine(fp, sKeyName);
	if ( gnlRet_LineRead != gnlRet || sKeyName.length() == 0 )
	{
		m_logs.ErrOut()
			<< L"Format error:  no registry key name specified" << endl;
		return prfeRet_FormatError;
	}

	// Next line must be the value name
	gnlRet = GetNextLine(fp, sValueName);
	if ( gnlRet_LineRead != gnlRet )
	{
		m_logs.ErrOut()
			<< L"Format error:  no registry value name specified" << endl;
		return prfeRet_FormatError;
	}
	// Allow "(Default)" as a special case
	if ( sValueName == L"(Default)" )
		sValueName.clear();

	// Next line is the Action:  Action must be DELETE or start with DWORD: or SZ:
	// (Here, check only if Action is missing.  Further validation performed in ProcessRegistryFileCommand.)
	gnlRet = GetNextLine(fp, sAction);
	if ( gnlRet_LineRead != gnlRet )
	{
		m_logs.ErrOut()
			<< L"Format error:  no action specified." << endl;
		return prfeRet_FormatError;
	}

	switch( ProcessRegistryFileCommand(gpo, bComputerConfig, sKeyName, sValueName, sAction) )
	{
	case prfcRet_EntryProcessed:
		return prfeRet_EntryProcessed;
	case prfcRet_FormatError:
		return prfeRet_FormatError;
	case prfcRet_RegAccessError:
		return prfeRet_RegAccessError;
	default:
		return prfeRet_OtherError;
	}
}

RegFilesProcessor::prfcRet_t RegFilesProcessor::ProcessRegistryFileCommand(LocalGPO & gpo, bool bComputerConfig, const wstring & sKeyName, const wstring & sValueName, const wstring & sAction)
{
	HKEY hBaseKey = bComputerConfig ? gpo.ComputerKey() : gpo.UserKey();
	wstring sValueNameCmd = sValueName;
	wstring sDelValueCmd = L"**del." + sValueName;
	wstring sValueData;
	DWORD dwRegType = REG_NONE;
	DWORD dwVal = 0;
	const wchar_t szSpace[] = L" \0";
	const BYTE * pData = (const BYTE *)&dwVal; // point to empty value to start
	DWORD cbData = 0;
	bool bDeleteValue = false;
	bool bSetValue = false;

	if ( L"DELETE" == sAction )
	{
		// This command will add a "**del.valuename" command; we also need to delete any set-value command if present
		bDeleteValue = true;

		sValueNameCmd = sDelValueCmd;
		dwRegType = REG_SZ;
		// Samples I've seen show data pointing to a single space char.
		cbData = 4;
		pData = (const BYTE *)szSpace;
	}
	else
	if ( L"DELETEALLVALUES" == sAction )
	{
		// Need to delete any set-value commands now, as well as insert a command to clear values later.
		// Accomplish this by deleting the entire key - it will get recreated below.
		// Ignore return value (e.g., the key doesn't exist yet)
		RegDeleteKey(hBaseKey, sKeyName.c_str());

		sValueNameCmd = L"**delvals.";
		dwRegType = REG_SZ;
		// Samples I've seen show data pointing to a single space char.
		cbData = 4;
		pData = (const BYTE *)szSpace;
	}
	else
	if ( L"CREATEKEY" == sAction )
	{
		sValueNameCmd.clear();
		// value data remains empty
	}
	else
	if ( 0 == wcsncmp(L"DWORD:", sAction.c_str(), 6) )
	{
		bSetValue = true;
		wstring sNum = sAction.c_str() + 6;
		const wchar_t * szFmt = L"%d";
		if ( 0 == _wcsnicmp(L"0x", sNum.c_str(), 2) )
			szFmt = L"0x%x";
		if ( 1 != swscanf_s(sNum.c_str(), szFmt, &dwVal) )
		{
			m_logs.ErrOut()
				<< L"Format error - invalid DWORD value:  " << sNum << endl;
			return prfcRet_FormatError;
		}
		dwRegType = REG_DWORD;
		cbData = sizeof(DWORD);
	}
	else
	if ( 0 == wcsncmp(L"SZ:", sAction.c_str(), 3) )
	{
		bSetValue = true;
		sValueData = sAction.substr(3);
		dwRegType = REG_SZ;
		cbData = (DWORD)(sValueData.length() + 1) * sizeof(wchar_t);
		pData = (const BYTE *)sValueData.c_str();
	}
	else
	if ( 0 == wcsncmp(L"EXSZ:", sAction.c_str(), 5) )
	{
		bSetValue = true;
		sValueData = sAction.substr(5);
		dwRegType = REG_EXPAND_SZ;
		cbData = (DWORD)(sValueData.length() + 1) * sizeof(wchar_t);
		pData = (const BYTE *)sValueData.c_str();
	}
	else
	{
		m_logs.ErrOut()
			<< L"Format error - invalid action:  " << sAction << endl;
		return prfcRet_FormatError;
	}

	// Create key for User or Machine policy
	HKEY hSubKey = NULL;
	DWORD dwDisposition = 0;
	LONG rv = RegCreateKeyExW(hBaseKey, sKeyName.c_str(), 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hSubKey, &dwDisposition);
	if ( NO_ERROR != rv )
	{
		m_logs.ErrOut() 
			<< L"Error:  Unable to create policy key " << sKeyName << endl
			<< SysErrorMessageWithCode(rv) << endl;
		return prfcRet_RegAccessError;
	}

	// If it's a delete-value command, remove any existing set-value commands for that value.
	// If it's a set-value command, remove any existing delete-value commands for that value.
	// Ignore return value from these Delete commands (e.g., the value doesn't exist)
	if ( bDeleteValue )
	{
		RegDeleteValue(hSubKey, sValueName.c_str());
	}
	else if ( bSetValue )
	{
		RegDeleteValue(hSubKey, sDelValueCmd.c_str());
	}

	rv = RegSetValueEx(hSubKey, sValueNameCmd.c_str(), 0, dwRegType, pData, cbData);
	RegCloseKey(hSubKey);
	if ( ERROR_SUCCESS == rv )
	{
		m_logs.Output()
			<< ( bComputerConfig ? L"Computer Config:" : L"User Config:" ) << endl
			<< L"\t" << sKeyName << endl
			<< L"\t" << ( sValueNameCmd.length() > 0 ? sValueNameCmd.c_str() : L"(Default)");
		switch( dwRegType )
		{
		case REG_SZ:
			m_logs.Output() << L" (REG_SZ) = " << sValueData;
			break;
		case REG_EXPAND_SZ:
			m_logs.Output() << L" (REG_EXPAND_SZ) = " << sValueData;
			break;
		case REG_DWORD:
			m_logs.Output() << L" (REG_DWORD) = " << dwVal;
			break;
		default:
			m_logs.Output() << L" (RegType " << dwRegType << L"; size " << cbData << L")";
			break;
		}
		m_logs.Output() << endl << endl;
		return prfcRet_EntryProcessed;
	}
	else
	{
		m_logs.ErrOut() 
			<< L"Error:  Unable to set policy value \"" << sValueNameCmd << L"\" in key " << sKeyName << endl
			<< SysErrorMessageWithCode(rv) << endl;
		return prfcRet_RegAccessError;
	}
}


RegFilesProcessor::gnlRet_t RegFilesProcessor::GetNextLine(FILE*fp, wstring & sLine)
{
	const size_t bufsize = 2048;
	wchar_t * fgetwsRet = NULL, *pLF = NULL;
	wchar_t buf[bufsize];

	sLine.clear();
	
	fgetwsRet = fgetws(buf, bufsize, fp);
	if ( NULL == fgetwsRet )
	{
		if ( feof(fp) )
			return gnlRet_EOF;
		else
		{
			return gnlRet_FileReadError;
		}
	}
	// Replace EOL with NUL
	pLF = wcschr(buf, L'\n');
	if ( pLF )
		*pLF = L'\0';

	sLine = buf;
	return gnlRet_LineRead;
}
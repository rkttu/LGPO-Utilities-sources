/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

#include "Logs.h"
#include "LocalGPO.h"



// Process embedded registry.pol files.
class RegFilesProcessor
{
public:
	RegFilesProcessor(Logs & logs);
	~RegFilesProcessor();

	int ProcessRegistryFiles(const filelist_t & regfiles);

private:
	// Implementation
	enum prfeRet_t
	{
		prfeRet_EntryProcessed,
		prfeRet_NoMoreEntries,
		prfeRet_FormatError,
		prfeRet_FileReadError,
		prfeRet_RegAccessError,
		prfeRet_OtherError,
	};
	enum prfcRet_t
	{
		prfcRet_EntryProcessed,
		prfcRet_FormatError,
		prfcRet_RegAccessError,
		prfcRet_OperationalError
	};
	enum gnlRet_t
	{
		gnlRet_LineRead,
		gnlRet_EOF,
		gnlRet_FileReadError
	};
	bool ProcessRegistryFile(const wstring & regfile);
	prfeRet_t ProcessRegistryFileEntry(FILE*fp, LocalGPO & gpo);
	prfcRet_t ProcessRegistryFileCommand(LocalGPO & gpo, bool bComputerConfig, const wstring & sKey, const wstring & sValue, const wstring & sAction);
	gnlRet_t GetNextLine(FILE*fp, wstring & sLine);

private:
	// Member vars
	Logs & m_logs;
	int m_retval;

private:
	// Not implemented
	RegFilesProcessor(const RegFilesProcessor &);
	RegFilesProcessor & operator = (const RegFilesProcessor &);

};

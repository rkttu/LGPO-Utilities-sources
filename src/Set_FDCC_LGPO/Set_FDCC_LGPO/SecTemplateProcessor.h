/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

#pragma once


// Process embedded security template files
class SecTemplateProcessor
{
public:
	SecTemplateProcessor(Logs & logs);
	~SecTemplateProcessor();

	int ProcessEmbeddedSecurityTemplates();

private:
	// Implementation
	static BOOL CALLBACK stEnumResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR pThis);
	BOOL ProcessResource(LPCTSTR lpszType, LPTSTR lpszName);
	int ProcessEmbeddedFile(const LPBYTE pMappedView, DWORD dwFileSize);
	int ProcessEmbeddedFileImpl(const LPBYTE pMappedView, DWORD dwFileSize);
	int InvokeSecedit(const wchar_t *tempSecTemplateInf, const wchar_t *tempSecTemplateSdb, const wchar_t *tempSecTemplateLog);

private:
	Logs & m_logs;
	int m_retval;

private:
	// Not implemented
	SecTemplateProcessor(const SecTemplateProcessor &);
	SecTemplateProcessor & operator = (const SecTemplateProcessor &);

};

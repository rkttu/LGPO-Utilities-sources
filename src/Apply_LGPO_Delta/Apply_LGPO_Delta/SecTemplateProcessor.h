/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once


// Process security template files
class SecTemplateProcessor
{
public:
	SecTemplateProcessor(Logs & logs);
	~SecTemplateProcessor();

	int ProcessSecurityTemplates(const filelist_t & sectempfiles);

private:
	// Implementation
	int ProcessSecurityTemplate(const wstring & sSecTemplateInf);
	int InvokeSecedit(const wchar_t *tempSecTemplateInf, const wchar_t *tempSecTemplateSdb, const wchar_t *tempSecTemplateLog);

private:
	Logs & m_logs;
	int m_retval;

private:
	// Not implemented
	SecTemplateProcessor(const SecTemplateProcessor &);
	SecTemplateProcessor & operator = (const SecTemplateProcessor &);

};

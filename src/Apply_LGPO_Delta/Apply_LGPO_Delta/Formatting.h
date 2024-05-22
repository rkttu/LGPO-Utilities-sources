/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2008.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

// Structure and operator to insert a zero-filled hex-formatted number into a stream.
struct HEX
{
	HEX(unsigned long num, unsigned long fieldwidth = 8, bool bUpcase = false)
		: m_num(num), m_width(fieldwidth), m_upcase(bUpcase)
		{}

	unsigned long m_num;
	unsigned long m_width;
	bool m_upcase;
};

inline ostream& operator << ( ostream& os, const HEX & h )
{
	int fmt = os.flags();
	char fillchar = os.fill('0');
	os << "0x" << hex << (h.m_upcase ? uppercase : nouppercase) << setw(h.m_width) << h.m_num ;
	os.fill(fillchar);
	os.flags(fmt);
	return os;
}

inline wostream& operator << ( wostream& os, const HEX & h )
{
	int fmt = os.flags();
	wchar_t fillchar = os.fill(L'0');
	os << L"0x" << hex << (h.m_upcase ? uppercase : nouppercase) << setw(h.m_width) << h.m_num ;
	os.fill(fillchar);
	os.flags(fmt);
	return os;
}


/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>
#include <initguid.h>
#include <gpedit.h>
#include <shlobj.h>  // for IsUserAnAdmin

#include <malloc.h>  // for alloca()
#include <stdio.h>


#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
using namespace std;

#include "Formatting.h"
#include "Utils.h"
#include "Logs.h"
#include "PlatformCheck.h"
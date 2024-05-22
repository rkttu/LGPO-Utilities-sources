/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

#include "StdAfx.h"
#include "LocalGPO.h"

// Class to encapsulate group policy processing.

// Constructor
LocalGPO::LocalGPO()
: m_UserKey(NULL), m_ComputerKey(NULL), m_pLGPO(NULL)
{
	// Raise an exception on any error.

	// Initialize COM
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if ( FAILED(hr) )
	{
		//wcerr << L"CoInitializeEx failed; hr = " << HEX(hr) << endl;
		throw hr;
	}

	// Create an instance of IGroupPolicyObject:  http://msdn2.microsoft.com/en-us/library/aa374235.aspx
	hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (void**)&m_pLGPO);
	if ( FAILED(hr) )
	{
		//wcerr << L"CoCreateInstance failed; hr = " << HEX(hr) << endl;
		throw hr;
	}

	// Access local group policy on this computer
	hr = m_pLGPO->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
	if ( FAILED(hr) )
	{
		//wcerr << L"OpenLocalMachineGPO failed; hr = " << HEX(hr) << endl;
		throw hr;
	}

	// Get references to machine and user policy keys.

	hr = m_pLGPO->GetRegistryKey(GPO_SECTION_MACHINE, &m_ComputerKey);
	if ( FAILED(hr) )
	{
		//wcerr << L"GetRegistryKey(GPO_SECTION_MACHINE) failed; hr = " << HEX(hr) << endl;
		throw hr;
	}

	hr = m_pLGPO->GetRegistryKey(GPO_SECTION_USER, &m_UserKey);
	if ( FAILED(hr) )
	{
		//wcerr << L"GetRegistryKey(GPO_SECTION_USER) failed; hr = " << HEX(hr) << endl;
		throw hr;
	}
}


LocalGPO::~LocalGPO()
{
	if ( m_UserKey )
		RegCloseKey(m_UserKey);
	if ( m_ComputerKey )
		RegCloseKey(m_ComputerKey);
	if ( m_pLGPO )
		m_pLGPO->Release();
	CoUninitialize();
}



// Save policy changes
HRESULT LocalGPO::Save()
{
	if ( !m_pLGPO )
		return E_POINTER;

	GUID RegistryId = REGISTRY_EXTENSION_GUID;
	GUID ThisAdminToolGuid = 
		// {DF3DC19F-F72C-4030-940E-4C2A65A6B612}
		{ 0xdf3dc19f, 0xf72c, 0x4030, { 0x94, 0xe, 0x4c, 0x2a, 0x65, 0xa6, 0xb6, 0x12 } };

	// On sharing violation, retry every half second for up to 10 seconds.
	//TODO:  Rearchitect the program so that it does one save for Machine and one save for User,
	// no matter how many registry.pol files are processed.
	const HRESULT hrSharingViolation = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
	const int retries = 20;
	const DWORD retryDelay_ms = 500;

	HRESULT hrComputer = m_pLGPO->Save(TRUE, TRUE, &RegistryId, &ThisAdminToolGuid);
	if ( hrSharingViolation == hrComputer )
	{
		for (int retry = 0; retry < retries; ++retry)
		{
			Sleep(retryDelay_ms);
			hrComputer = m_pLGPO->Save(TRUE, TRUE, &RegistryId, &ThisAdminToolGuid);
			if ( hrSharingViolation != hrComputer )
				break;
		}
	}

	HRESULT hrUser = m_pLGPO->Save(FALSE, TRUE, &RegistryId, &ThisAdminToolGuid);
	if ( hrSharingViolation == hrUser )
	{
		for (int retry = 0; retry < retries; ++retry)
		{
			Sleep(retryDelay_ms);
			hrUser = m_pLGPO->Save(TRUE, TRUE, &RegistryId, &ThisAdminToolGuid);
			if ( hrSharingViolation != hrUser )
				break;
		}
	}

	if ( FAILED(hrComputer) )
		return hrComputer;
	else
		return hrUser;
}

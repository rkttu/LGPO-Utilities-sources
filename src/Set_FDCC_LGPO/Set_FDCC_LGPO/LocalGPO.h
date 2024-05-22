/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2007.  Microsoft Corporation.  All rights reserved.
*/

#pragma once

// Class to encapsulate group policy processing.
class LocalGPO
{
public:
	LocalGPO();
	~LocalGPO();

	// Get registry keys corresponding to user and machine policy.
	HKEY UserKey() const { return m_UserKey; }
	HKEY ComputerKey() const { return m_ComputerKey; }

	// Save policy changes
	HRESULT Save();

private:
	HKEY m_UserKey, m_ComputerKey;
	IGroupPolicyObject* m_pLGPO;

private:
	// Not implemented
	LocalGPO(const LocalGPO &);
	LocalGPO & operator = (const LocalGPO &);
};

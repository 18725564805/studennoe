
// RemotaClient.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CRemotaClientApp:
// See RemotaClient.cpp for the implementation of this class
//

class CRemotaClientApp : public CWinApp
{
public:
	CRemotaClientApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CRemotaClientApp theApp;

// ControlTrackerApp.h : main header file for the MULTIRECTTRACKER application
//

#ifndef _CONTROLTRACKERAPP_H
#define _CONTROLTRACKERAPP_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CControlTrackerApp:
// See ControlTrackerApp.cpp for the implementation of this class
//

class CControlTrackerApp : public CWinApp
{
public:
	CControlTrackerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CControlTrackerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	//{{AFX_MSG(CControlTrackerApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _CONTROLTRACKERAPP_H

// NTSyslogCtrl.h : main header file for the NTSYSLOGCTRL application
//

#if !defined(AFX_NTSYSLOGCTRL_H__9FB33EE5_E0E8_11D5_B306_0040055338AF__INCLUDED_)
#define AFX_NTSYSLOGCTRL_H__9FB33EE5_E0E8_11D5_B306_0040055338AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#define CHECK_NOT_ENABLED		0
#define CHECK_INFORMATION		1
#define CHECK_SUCCESS			32  //In windows EVENTLOG_SUCCESS is zero, but that works out bad for filtering purposes. Bastard solution.
#define CHECK_WARNING			2
#define CHECK_ERROR				4
#define CHECK_AUDIT_SUCCESS		8
#define CHECK_AUDIT_FAILURE		16

#define DEFAULT_CHECKS (CHECK_WARNING + CHECK_ERROR + CHECK_AUDIT_FAILURE)
#define ALL_CHECKS (CHECK_INFORMATION + CHECK_WARNING + CHECK_SUCCESS + CHECK_ERROR \
						+ CHECK_AUDIT_SUCCESS + CHECK_AUDIT_FAILURE)

#define DEFAULT_PRIORITY		9

//erno Many defines in registrySettings.h!


//void __cdecl initRegistry(char * SyslogAddress);
extern "C" {
	void __cdecl initRegistry(char * SyslogAddress);
}


/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlApp:
// See NTSyslogCtrl.cpp for the implementation of this class
//

class CNTSyslogCtrlApp : public CWinApp
{
public:
	CNTSyslogCtrlApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNTSyslogCtrlApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNTSyslogCtrlApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NTSYSLOGCTRL_H__9FB33EE5_E0E8_11D5_B306_0040055338AF__INCLUDED_)

// NTSyslogCtrl.cpp : Defines the class behaviors for the application.
//

#include "..\Syslogserver\common_stdafx.h"
//#include "stdafx.h"
#include "NTSyslogCtrl.h"
#include "NTSyslogCtrlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlApp

BEGIN_MESSAGE_MAP(CNTSyslogCtrlApp, CWinApp)
	//{{AFX_MSG_MAP(CNTSyslogCtrlApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlApp construction

CNTSyslogCtrlApp::CNTSyslogCtrlApp(){
	// Add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNTSyslogCtrlApp object

CNTSyslogCtrlApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlApp initialization

BOOL CNTSyslogCtrlApp::InitInstance(){
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

//ifdef _AFXDLL
//	Enable3dControls();			// Call this when using MFC in a shared DLL
//else
//	Enable3dControlsStatic();	// Call this when linking to MFC statically
//endif

	CNTSyslogCtrlDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK){
		// Place code here to handle when the dialog is dismissed with OK
	}
	else if (nResponse == IDCANCEL){
		// Place code here to handle when the dialog is dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

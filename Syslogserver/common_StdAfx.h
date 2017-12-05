// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//


#if !defined(AFX_STDAFX_H__48C23943_84C7_11D5_898D_0008C725AC74__INCLUDED_)
#define AFX_STDAFX_H__48C23943_84C7_11D5_898D_0008C725AC74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						



#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>

// For implementing NT Services
#include <winsvc.h>
#include <lmcons.h>

//For CSingleLock
#include <afxmt.h>

// TODO: reference additional headers your program requires here
//#include <stdio.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <tlhelp32.h>
#include <eh.h>


#import ".\msado27.tlb" no_namespace rename( "EOF", "adoEOF" )
#import ".\MSJRO.DLL" no_namespace


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__48C23943_84C7_11D5_898D_0008C725AC74__INCLUDED_)

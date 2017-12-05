// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//









//                   NOT  USED !!!!!









#if !defined(AFX_STDAFX_H__9FB33EE9_E0E8_11D5_B306_0040055338AF__INCLUDED_)
#define AFX_STDAFX_H__9FB33EE9_E0E8_11D5_B306_0040055338AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
//#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#include <afxmt.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// Microsoft's STL dosen't compile clean at high warning levels,
// under VC6.  This lets it be sloppy, but keeps our code at the higher
// warning level.
//#pragma warning(push, 3)
//#include <iostream>
//#pragma warning(pop)
// Reference additional headers your program requires here

// For implementing NT Services
#include <winsvc.h>
#include <lmcons.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9FB33EE9_E0E8_11D5_B306_0040055338AF__INCLUDED_)

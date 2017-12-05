#if !defined(AFX_CONFIGLOGGING_H__F78FB47C_8919_44F7_94EC_BE51A0D702AD__INCLUDED_)
#define AFX_CONFIGLOGGING_H__F78FB47C_8919_44F7_94EC_BE51A0D702AD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigLogging.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigLogging dialog

class CConfigLogging : public CDialog
{
// Construction
public:
	CConfigLogging(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigLogging)
	enum { IDD = IDD_CONFIG_LOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	void SetupDialog(CString selection,
			LPCTSTR lpstrComputer = NULL, UINT defaultChecks = DEFAULT_CHECKS);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigLogging)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_csComputer;
	CString m_idTitle;
	CString m_idRegpath;
	UINT m_uDefaultChecks;

	UINT m_uCurrentState;
	UINT m_uInfoPriority;
	UINT m_uSuccessPriority;
	UINT m_uWarningPriority;
	UINT m_uErrorPriority;
	UINT m_uAuditSuccessPriority;
	UINT m_uAuditFailurePriority;

	BOOL IsModified();
	UINT PriorityFromDialog(int facilityId, int severityId);
	void SetDialogFromPriority(int facilityId, int severityId, UINT priority);

	// Generated message map functions
	//{{AFX_MSG(CConfigLogging)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDefaults();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGLOGGING_H__F78FB47C_8919_44F7_94EC_BE51A0D702AD__INCLUDED_)

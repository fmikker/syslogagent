#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CConfAppl dialog

class CConfAppl : public CDialog
{
	DECLARE_DYNAMIC(CConfAppl)

public:
	CConfAppl(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConfAppl();

// Dialog Data
	enum { IDD = IDD_CONFAPPL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CEdit m_ApplicationName;
	CEdit m_Path;
	CEdit m_File_Extension;
	CButton ParseDate;
	CButton ParseHost;
	CButton ParseSeverity;
	CComboBox SeverityLevel;
	CButton ParseProcess;
	CEdit ProcessName;
	CComboBox Facility;
	CButton ignorePrefixLines;
	CEdit prefix;
	CButton ignoreFirstLines;
	CEdit NbrIgnoreLines;
	CString m_initString; //Used to send the application name to this instance 
	afx_msg void OnCbnSelchangeTest();
	int SetupDialog(CString ApplicationName);
private:
	int ReadSettings(void);
	void Browse(CEdit*);
public:
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedUsePrefix();
	afx_msg void OnBnClickedUseIgnoreLines();
	afx_msg void OnBnClickedParseProcess();
	afx_msg void OnBnClickedParseSeverity();
	afx_msg void OnBnClickedRadioDir();
	afx_msg void OnBnClickedRadioFile();
	afx_msg void OnBnClickedBrowse2();
	CButton radioButtonDir;
	CEdit m_fileName;
	CButton radioButtonFile;
	int m_radio;
	afx_msg void OnBnClickedBrowseRotate1();
	afx_msg void OnBnClickedBrowseRotate2();
	CEdit m_rotate_file;
	CEdit m_rotated_file;
	afx_msg void OnBnClickedRadioRotateFile();
	afx_msg void OnBnClickedSuggestsettings();
	CButton Unicode;
};

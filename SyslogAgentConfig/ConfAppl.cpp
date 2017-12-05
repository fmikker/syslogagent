// ConfAppl.cpp : implementation file
//

#include "..\Syslogserver\common_stdafx.h"
//#include "stdafx.h"
#include "NTSyslogCtrl.h"
#include "ConfAppl.h"
#include <shlobj.h>
#include "../Syslogagent/appParse.h"

//Needed to satisfy dependencies to the parse engine (include of appwatch.h)
#include <queue>
using namespace std ;

//    int _convert; (_convert); UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; (_acp); LPCWSTR _lpw; (_lpw); LPCSTR _lpa; (_lpa);
extern int _convert; 
//(_convert); 
extern UINT _acp; 
//(_acp); 
extern LPCWSTR _lpw; 
//(_lpw); 
extern LPCSTR _lpa; 
//(_lpa);


char ernogetc(FILE *fp,bool unicode);
int parseMessage(unsigned char* input,unsigned char* output,applSettings *loggerInformation);

// CConfAppl dialog
IMPLEMENT_DYNAMIC(CConfAppl, CDialog)
CConfAppl::CConfAppl(CWnd* pParent /*=NULL*/)
: CDialog(CConfAppl::IDD, pParent)
//erno 	, radioDir(0)
{
}

CConfAppl::~CConfAppl(){
}

BOOL CConfAppl::OnInitDialog(){
	//erno This function was not created automatically! wtf. Added it myself, including the much needed cDialog-initiation.
	CDialog::OnInitDialog();

	//Read or set settings
	if (m_initString!="") { //Editing, not a new one
		ReadSettings();
	} else {
		SeverityLevel.SetCurSel(6); //Information
		Facility.SetCurSel(23);
		ProcessName.SetWindowText("Process Name");
		CheckDlgButton(IDC_RADIO_DIR,1);
		OnBnClickedRadioDir();
		m_File_Extension.SetWindowText("log");
	}

	//Disable non-valid fields
	OnBnClickedUsePrefix(); //Updates status
	OnBnClickedUseIgnoreLines();
	OnBnClickedParseProcess();
	OnBnClickedParseSeverity();

	if (m_initString=="") { //Editing, not a new one
		GetDlgItem( IDC_PARSE_DATE)->EnableWindow( FALSE);
		GetDlgItem( IDC_PARSE_HOST)->EnableWindow( FALSE);
		GetDlgItem( IDC_PARSE_SEVERITY)->EnableWindow( FALSE);
		GetDlgItem( IDC_USE_PREFIX)->EnableWindow( FALSE);
		GetDlgItem( IDC_PREFIX)->EnableWindow( FALSE);
		GetDlgItem( IDC_USE_IGNORE_LINES)->EnableWindow( FALSE);
		GetDlgItem( IDC_LINES_TO_IGNORE)->EnableWindow( FALSE);
		GetDlgItem( IDC_SEVERITY_LEVEL)->EnableWindow( FALSE);
		GetDlgItem( IDC_PROCESS_NAME)->EnableWindow( FALSE);
		GetDlgItem( IDC_PARSE_PROCESS)->EnableWindow( FALSE);
		GetDlgItem( IDC_FACILITY)->EnableWindow( FALSE);
	}

	return true;

}

void CConfAppl::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_APPNAME, m_ApplicationName);
	DDX_Control(pDX, IDC_APP_PATH, m_Path);
	DDX_Control(pDX, IDC_FILE_EXT, m_File_Extension);
	DDX_Control(pDX, IDC_PARSE_DATE, ParseDate);
	DDX_Control(pDX, IDC_PARSE_HOST, ParseHost);
	DDX_Control(pDX, IDC_PARSE_SEVERITY, ParseSeverity);
	DDX_Control(pDX, IDC_SEVERITY_LEVEL, SeverityLevel);
	DDX_Control(pDX, IDC_PARSE_PROCESS, ParseProcess);
	DDX_Control(pDX, IDC_PROCESS_NAME, ProcessName);
	DDX_Control(pDX, IDC_FACILITY, Facility);
	DDX_Control(pDX, IDC_USE_PREFIX, ignorePrefixLines);
	DDX_Control(pDX, IDC_PREFIX, prefix);
	DDX_Control(pDX, IDC_USE_IGNORE_LINES, ignoreFirstLines);
	DDX_Control(pDX, IDC_LINES_TO_IGNORE, NbrIgnoreLines);
	DDX_Control(pDX, IDCANCEL, radioButtonFile);
	DDX_Control(pDX, IDC_APP_ROTATE_FILE1, m_rotate_file);
	DDX_Control(pDX, IDC_APP_FILEPATH, m_fileName);
	DDX_Control(pDX, IDC_APP_ROTATE_FILE2, m_rotated_file);
	DDX_Control(pDX, IDC_UNICODE, Unicode);
}


BEGIN_MESSAGE_MAP(CConfAppl, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_USE_PREFIX, OnBnClickedUsePrefix)
	ON_BN_CLICKED(IDC_USE_IGNORE_LINES, OnBnClickedUseIgnoreLines)
	ON_BN_CLICKED(IDC_PARSE_PROCESS, OnBnClickedParseProcess)
	ON_BN_CLICKED(IDC_PARSE_SEVERITY, OnBnClickedParseSeverity)
	ON_BN_CLICKED(IDC_RADIO_DIR, OnBnClickedRadioDir)
	ON_BN_CLICKED(IDC_RADIO_FILE, OnBnClickedRadioFile)
	ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse2)
	ON_BN_CLICKED(IDC_BROWSE_ROTATE1, OnBnClickedBrowseRotate1)
	ON_BN_CLICKED(IDC_BROWSE_ROTATE2, OnBnClickedBrowseRotate2)
	ON_BN_CLICKED(IDC_RADIO_ROTATE_FILE, OnBnClickedRadioRotateFile)
	ON_BN_CLICKED(SUGGESTSETTINGS, OnBnClickedSuggestsettings)
END_MESSAGE_MAP()


// CConfAppl message handlers

void CConfAppl::OnBnClickedOk(){
	CString text;
	bool value;
	int intvalue;

	//Verify settings are somewhat valid
	if (m_ApplicationName.GetWindowTextLength()==0) {
		AfxMessageBox( _T( "An application name must be set."), MB_ICONSTOP);
		return;
	}

	switch (m_radio) {

		case 1:
			if (m_Path.GetWindowTextLength()==0) {
				AfxMessageBox( _T( "Path must be set for using this type of application logging."), MB_ICONSTOP);
			return;
			}
			break;
		case 2:
			if (m_fileName.GetWindowTextLength()==0) {
				AfxMessageBox( _T( "The explicit file name be set for using this type of application logging."), MB_ICONSTOP);
			return;
			}
			break;

		case 3:
			if ((m_rotate_file.GetWindowTextLength()==0)|(m_rotated_file.GetWindowTextLength()==0)) {
				AfxMessageBox( _T( "Both current file and rotated file name must be set for using this type of application logging."), MB_ICONSTOP);
			return;
			}
			break;
		default:
			break;
	}

	//Write to registry
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	GoToRegKey(APPLICATIONLOGS);
	m_ApplicationName.GetWindowText(text);
	GoToRegKey(text);

	if (m_radio==1) {
		m_File_Extension.GetWindowText(text);
		WriteRegKey(&text,APP_EXT);
		m_Path.GetWindowText(text);
		WriteRegKey(&text,APP_PATH);
		text="";
		WriteRegKey(&text,APP_FILENAME);
		WriteRegKey(&text,APP_ROTATEFILE);
		WriteRegKey(&text,APP_ROTATEDFILE);
	} else if(m_radio==2) {
		m_fileName.GetWindowText(text);
		WriteRegKey(&text,APP_FILENAME);
		text="";
		WriteRegKey(&text,APP_EXT);
		WriteRegKey(&text,APP_PATH);
		WriteRegKey(&text,APP_ROTATEFILE);
		WriteRegKey(&text,APP_ROTATEDFILE);
	} else {
		m_rotate_file.GetWindowText(text);
		WriteRegKey(&text,APP_ROTATEFILE);
		m_rotated_file.GetWindowText(text);
		WriteRegKey(&text,APP_ROTATEDFILE);

		text="";
		WriteRegKey(&text,APP_FILENAME);
		WriteRegKey(&text,APP_EXT);
		WriteRegKey(&text,APP_PATH);

	}

	value=(ParseDate.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_PARSE_DATE);
	value=(ParseHost.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_PARSE_HOST);
	value=(ParseSeverity.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_PARSE_SEVERITY);

	value=(Unicode.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_UNICODE);

	intvalue=SeverityLevel.GetCurSel();
	WriteRegKey((unsigned int*)&intvalue,APP_SEVERITY);

	value=(ParseProcess.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_PARSE_PROCESS);

	ProcessName.GetWindowText(text);
	WriteRegKey(&text,APP_PROCESS_NAME);

	intvalue=Facility.GetCurSel();
	WriteRegKey((unsigned int*)&intvalue,APP_FACILITY);

	value=(ignorePrefixLines.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_IGNORE_PREFIX_LINES);

	prefix.GetWindowText(text);
	WriteRegKey(&text,APP_PREFIX);

	value=(ignoreFirstLines.GetCheck()==BST_CHECKED);
	WriteRegKey(&value,APP_IGNORE_FIRST_LINES);

	NbrIgnoreLines.GetWindowText(text);
	intvalue=atoi(text);
	WriteRegKey((unsigned int*)&intvalue,APP_NBR_IGNORE_LINES);

	CloseRegistry();
	OnOK();
}

void CConfAppl::OnCbnSelchangeTest(){
}

int CConfAppl::SetupDialog(CString ApplicationName){

	m_initString=ApplicationName;
	return 0;
}

int CConfAppl::ReadSettings(void){
	CString text;
	bool boolvalue;
	int intvalue;

	m_ApplicationName.SetWindowText((LPCTSTR)m_initString);  //Sent via setupDialog


	//Load from registry
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	GoToRegKey(APPLICATIONLOGS);
	if(m_initString=="") {
		AfxMessageBox("Error. Trying to read non-existant settings.", MB_ICONSTOP);
		return 0;
	};
	GoToRegKey(_T(m_initString));

	//	CheckDlgButton( IDC_AUDIT_FAILURE_CHECK, (m_uCurrentState & CHECK_AUDIT_FAILURE));

	//	m_File_Extension.GetWindowText(text);
	ReadRegKey(&text,"",APP_EXT);
	m_File_Extension.SetWindowText(text);
	ReadRegKey(&text,"",APP_PATH);
	m_Path.SetWindowText(text);
	if (text!="") {
		CheckDlgButton(IDC_RADIO_DIR,1);
		OnBnClickedRadioDir();
	}

	ReadRegKey(&text,"",APP_FILENAME);
	m_fileName.SetWindowText(text);
	if (text!="") {
		CheckDlgButton(IDC_RADIO_FILE,1);
		OnBnClickedRadioFile();
	}

	ReadRegKey(&text,"",APP_ROTATEFILE);
	m_rotate_file.SetWindowText(text);
	ReadRegKey(&text,"",APP_ROTATEDFILE);
	m_rotated_file.SetWindowText(text);
	if (text!="") {
		CheckDlgButton(IDC_RADIO_ROTATE_FILE,1);
		OnBnClickedRadioRotateFile();
	}

	ReadRegKey(&boolvalue,false,APP_PARSE_DATE);
	ParseDate.SetCheck(boolvalue);

	ReadRegKey(&boolvalue,false,APP_PARSE_HOST);
	ParseHost.SetCheck(boolvalue);

	ReadRegKey(&boolvalue,false,APP_PARSE_SEVERITY);
	ParseSeverity.SetCheck(boolvalue);

	ReadRegKey((unsigned int*)&intvalue,6,APP_SEVERITY);
	SeverityLevel.SetCurSel(intvalue);

	ReadRegKey(&boolvalue,false,APP_PARSE_PROCESS);
	ParseProcess.SetCheck(boolvalue);

	ReadRegKey(&boolvalue,false,APP_UNICODE);
	Unicode.SetCheck(boolvalue);

	ReadRegKey(&text,"Process Name",APP_PROCESS_NAME);
	ProcessName.SetWindowText(text);

	ReadRegKey((unsigned int*)&intvalue,22,APP_FACILITY);
	Facility.SetCurSel(intvalue);

	ReadRegKey(&boolvalue,false,APP_IGNORE_PREFIX_LINES);
	ignorePrefixLines.SetCheck(boolvalue);

	ReadRegKey(&text,"",APP_PREFIX);
	prefix.SetWindowText(text);

	ReadRegKey(&boolvalue,false,APP_IGNORE_FIRST_LINES);
	ignoreFirstLines.SetCheck(boolvalue);

	ReadRegKey((unsigned int*)&intvalue,0,APP_NBR_IGNORE_LINES);
	text.Format("%d",intvalue);
	NbrIgnoreLines.SetWindowText(text);

	CloseRegistry();
	return 0;
}

void CConfAppl::OnBnClickedBrowse(){
	Browse(&m_Path);
	OnBnClickedSuggestsettings();
}

void CConfAppl::Browse(CEdit *a){
	LPMALLOC	pMalloc;	/* Gets the Shell's default allocator */ 

	if	(	::SHGetMalloc	(	&pMalloc) ==	NOERROR) 
	{ 
		BROWSEINFO		bi;	
		char			pszBuffer [	MAX_PATH]; 
		LPITEMIDLIST	pidl; 

		// Get help on BROWSEINFO struct - it's got all the bit settings. 
		bi.hwndOwner		=	NULL; 
		bi.pidlRoot			=	NULL; //"c:\\temp";	
		bi.pszDisplayName	=	pszBuffer; 
		bi.lpszTitle		=	_T("Select a target Directory"); 
		bi.ulFlags			=	BIF_RETURNFSANCESTORS |	BIF_RETURNONLYFSDIRS; 
		bi.lpfn				=	NULL; 
		bi.lParam			=	0; 

		// This next call issues the dialog box. 
		if	(	pidl =	::SHBrowseForFolder (	&bi)) 
		{ 
			if (	::SHGetPathFromIDList (	pidl, pszBuffer)) 
			{ 
				// At this point pszBuffer contains the selected path */. 
				//m_Path =	(LPCTSTR)pszBuffer; 
				(*a).SetWindowText(pszBuffer);
				GetDlgItem( IDC_PARSE_DATE)->EnableWindow( TRUE);
				GetDlgItem( IDC_PARSE_HOST)->EnableWindow( TRUE);
				GetDlgItem( IDC_PARSE_SEVERITY)->EnableWindow( TRUE);
				GetDlgItem( IDC_PARSE_PROCESS)->EnableWindow( TRUE);
				GetDlgItem( IDC_USE_PREFIX)->EnableWindow( TRUE);
				GetDlgItem( IDC_PREFIX)->EnableWindow( TRUE);
				GetDlgItem( IDC_USE_IGNORE_LINES)->EnableWindow( TRUE);
				GetDlgItem( IDC_LINES_TO_IGNORE)->EnableWindow( TRUE);
				GetDlgItem( IDC_SEVERITY_LEVEL)->EnableWindow( TRUE);
				GetDlgItem( IDC_PROCESS_NAME)->EnableWindow( TRUE);
				GetDlgItem( IDC_FACILITY)->EnableWindow( TRUE);

			} 
			// Free the PIDL allocated by SHBrowseForFolder. 
			pMalloc->Free	(	pidl); 
		} 

		// Release the shell's allocator. 	  
		pMalloc->Release();		
	}	 
}

void CConfAppl::OnBnClickedUsePrefix(){

	if (ignorePrefixLines.GetCheck()) {
		GetDlgItem(IDC_PREFIX)->EnableWindow(true);
	} else {
		GetDlgItem(IDC_PREFIX)->EnableWindow(false);
	}
}

void CConfAppl::OnBnClickedUseIgnoreLines(){
	if (ignoreFirstLines.GetCheck()) {
		GetDlgItem(IDC_LINES_TO_IGNORE)->EnableWindow(true);
	} else {
		GetDlgItem(IDC_LINES_TO_IGNORE)->EnableWindow(false);
	}
}

void CConfAppl::OnBnClickedParseProcess(){
	//m_raw_mode.SetCheck(false);
	if (ParseProcess.GetCheck()) {
		GetDlgItem(IDC_PROCESS_NAME)->EnableWindow(false);
	} else {
		GetDlgItem(IDC_PROCESS_NAME)->EnableWindow(true);
	}
}

void CConfAppl::OnBnClickedParseSeverity(){
	//m_raw_mode.SetCheck(false);
	if (ParseSeverity.GetCheck()) {
		GetDlgItem(IDC_SEVERITY_LEVEL)->EnableWindow(false);
	} else {
		GetDlgItem(IDC_SEVERITY_LEVEL)->EnableWindow(true);
	}
}

void CConfAppl::OnBnClickedRadioDir(){

	m_radio=1;

	GetDlgItem(IDC_APP_FILEPATH)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE_ROTATE1)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE_ROTATE2)->EnableWindow(false);
	GetDlgItem(IDC_APP_ROTATE_FILE1)->EnableWindow(false);
	GetDlgItem(IDC_APP_ROTATE_FILE2)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE2)->EnableWindow(false);
	GetDlgItem(IDC_FILE_EXT)->EnableWindow(true);
	GetDlgItem(IDC_BROWSE)->EnableWindow(true);
	GetDlgItem(IDC_APP_PATH)->EnableWindow(true);
	
	m_rotated_file.SetWindowText("");
	m_rotate_file.SetWindowText("");
	m_fileName.SetWindowText("");
	

}

void CConfAppl::OnBnClickedRadioFile(){

	m_radio=2;
	GetDlgItem(IDC_APP_FILEPATH)->EnableWindow(true);
	GetDlgItem(IDC_BROWSE_ROTATE1)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE_ROTATE2)->EnableWindow(false);
	GetDlgItem(IDC_APP_ROTATE_FILE1)->EnableWindow(false);
	GetDlgItem(IDC_APP_ROTATE_FILE2)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE2)->EnableWindow(true);
	GetDlgItem(IDC_FILE_EXT)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE)->EnableWindow(false);
	GetDlgItem(IDC_APP_PATH)->EnableWindow(false);

	m_rotated_file.SetWindowText("");
	m_rotate_file.SetWindowText("");
	m_Path.SetWindowText("");
	
}

void CConfAppl::OnBnClickedRadioRotateFile(){



	m_radio=3;

	GetDlgItem(IDC_APP_ROTATE_FILE1)->EnableWindow(true);
	GetDlgItem(IDC_APP_ROTATE_FILE2)->EnableWindow(true);
	GetDlgItem(IDC_BROWSE_ROTATE1)->EnableWindow(true);
	GetDlgItem(IDC_BROWSE_ROTATE2)->EnableWindow(true);
	GetDlgItem(IDC_APP_FILEPATH)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE2)->EnableWindow(false);
	GetDlgItem(IDC_FILE_EXT)->EnableWindow(false);
	GetDlgItem(IDC_BROWSE)->EnableWindow(false);
	GetDlgItem(IDC_APP_PATH)->EnableWindow(false);

	m_Path.SetWindowText("");
	m_fileName.SetWindowText("");

}


void CConfAppl::OnBnClickedBrowse2(){

	CFileDialog fileDlg( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, "All Files (*.*)|*.*|Log Files (*.log)|*.log|||", this);
	fileDlg.m_ofn.lpstrTitle = "Choose static log file";

	if ( fileDlg.DoModal() == IDOK){
		m_fileName .SetWindowText(fileDlg.GetPathName()); 
		//AfxMessageBox("Your file name is :" +m_fileName  );
		GetDlgItem( IDC_PARSE_DATE)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_HOST)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_SEVERITY)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_PROCESS)->EnableWindow( TRUE);
		GetDlgItem( IDC_USE_PREFIX)->EnableWindow( TRUE);
		GetDlgItem( IDC_PREFIX)->EnableWindow( TRUE);
		GetDlgItem( IDC_USE_IGNORE_LINES)->EnableWindow( TRUE);
		GetDlgItem( IDC_LINES_TO_IGNORE)->EnableWindow( TRUE);
		GetDlgItem( IDC_SEVERITY_LEVEL)->EnableWindow( TRUE);
		GetDlgItem( IDC_PROCESS_NAME)->EnableWindow( TRUE);
		GetDlgItem( IDC_FACILITY)->EnableWindow( TRUE);

		OnBnClickedSuggestsettings();
	}
}

void CConfAppl::OnBnClickedBrowseRotate1(){
	CFileDialog fileDlg( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, "All Files (*.*)|*.*|Log Files (*.log)|*.log|||", this);
	fileDlg.m_ofn.lpstrTitle = "Choose current log file";

	if ( fileDlg.DoModal() == IDOK){
		m_rotate_file.SetWindowText(fileDlg.GetPathName()); 
		//AfxMessageBox("Your file name is :" +m_fileName  );
	}
}

void CConfAppl::OnBnClickedBrowseRotate2(){
	CFileDialog fileDlg( TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, "All Files (*.*)|*.*|Log Files (*.log)|*.log|||", this);
	fileDlg.m_ofn.lpstrTitle = "Choose rotated log file name";

	if ( fileDlg.DoModal() == IDOK){
		m_rotated_file.SetWindowText(fileDlg.GetPathName()); 
		//AfxMessageBox("Your file name is :" +m_fileName  );
		GetDlgItem( IDC_PARSE_DATE)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_HOST)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_SEVERITY)->EnableWindow( TRUE);
		GetDlgItem( IDC_PARSE_PROCESS)->EnableWindow( TRUE);
		GetDlgItem( IDC_USE_PREFIX)->EnableWindow( TRUE);
		GetDlgItem( IDC_PREFIX)->EnableWindow( TRUE);
		GetDlgItem( IDC_USE_IGNORE_LINES)->EnableWindow( TRUE);
		GetDlgItem( IDC_LINES_TO_IGNORE)->EnableWindow( TRUE);
		GetDlgItem( IDC_SEVERITY_LEVEL)->EnableWindow( TRUE);
		GetDlgItem( IDC_PROCESS_NAME)->EnableWindow( TRUE);
		GetDlgItem( IDC_FACILITY)->EnableWindow( TRUE);

		OnBnClickedSuggestsettings();
	}
}


void CConfAppl::OnBnClickedSuggestsettings(){

	//Used to con parseMessage 
	applSettings appS;
	FILE *ThreadData_fp;
	_int64 ThreadData_filePosition;
	//_threadData td;
	CString tempA,tempB,tempC,fileNameReply;
	char buf[2048],output[2048],DaFileName[256];
	int counter,lastFieldsPos=0,argCounter=0;
	CString mess;

	appS.ParseDate=appS.ParseHost=appS.ParseProcess=appS.ParseSeverity=true;
	appS.ignoreFirstLines=appS.ignorePrefixLines=false;
	ThreadData_filePosition=0;
	appS.failedToParseDate=appS.failedToParseTime=appS.failedToParseHost=appS.failedToParseProcess=appS.failedToParseSeverity=false;

	if ((m_Path.GetWindowTextLength()==0)&(m_fileName.GetWindowTextLength()==0)&(m_rotate_file.GetWindowTextLength()==0)) {
		AfxMessageBox("First specify file or directory and file extension!");
		return;
	} else {
		if (AfxMessageBox( _T( "Inspect log file(s) and suggest settings now?"),MB_YESNO|MB_ICONQUESTION) == IDNO) {
			//choose no
			return;
		}
	}

	//find a file, open
	m_Path.GetWindowText(tempA);
	m_File_Extension.GetWindowText(tempB);
	m_fileName.GetWindowText(tempC);
	if (tempC=="") //find correct file
		m_rotate_file.GetWindowText(tempC);

	fileNameReply=getLatestLogFileName(tempA,tempB,tempC);
	if (fileNameReply=="") {
		AfxMessageBox("No file found!");
		return;
	}

	//is file unicode?
	strncpy(DaFileName,fileNameReply,255);
	appS.ConfirmedUnicodeFormat=testUnicode(DaFileName);

	//open file
	if (appS.ConfirmedUnicodeFormat) {
		ThreadData_fp= _wfsopen(T2W(fileNameReply),T2W("rb"), _SH_DENYNO);
		if(ThreadData_fp==NULL){
			CString foo;
			foo.Format("Failed to open file! Error: %d.",errno);
			AfxMessageBox(foo.GetBuffer());
			return;
		}
		ernogetc(ThreadData_fp,true); //Read and ignore the FF FE
	} else {
		ThreadData_fp= _fsopen(fileNameReply,"rb", _SH_DENYNO);
		if(ThreadData_fp==NULL){
			CString foo;
			foo.Format("Failed to open file! Error: %d.",errno);
			AfxMessageBox(foo.GetBuffer());
			return;
		}
	}
	//*********** Find prefix, if any****************//
	//***********************************************//
	counter=getLine(ThreadData_fp,appS.ConfirmedUnicodeFormat,buf,sizeof(buf)-1);
	if (counter>0) {
		char firstChar=buf[0];
		if (((firstChar>'0')&(firstChar<'9')) | //is a number
			((firstChar>'A')&(firstChar<'Z')) | // is a capital letter
			((firstChar>'a')&(firstChar<'z')) |  // is a small letter
			(firstChar==' ')   // space
			) {
				appS.prefix=""; //no prefix
				appS.ignorePrefixLines=false;
			} else {
				appS.prefix=firstChar; 
				appS.ignorePrefixLines=true;
			}
	} else {
		AfxMessageBox("Log file empty!");
		fclose(ThreadData_fp);
		return;
	}

	//Go back to just after last Field descriptor, or start of file
	if (fseek(ThreadData_fp, 0, SEEK_SET)!= 0) {
		AfxMessageBox("Failed to set file position.");
		return;
	}
	if (appS.ConfirmedUnicodeFormat) {
		ernogetc(ThreadData_fp,true); //Read and ignore the FF FE
	}
	//************Parse field codes, if any*********************//
	//**********************************************************//
	//parse field codes, if any
	appS.fieldCodes[0][0]='\0'; //reset any existing codes
	while ((counter=getLine(ThreadData_fp,appS.ConfirmedUnicodeFormat,buf,sizeof(buf)-1)) !=0) {
		ThreadData_filePosition+=counter;
		if (buf==strstr(buf,"#Fields:")) { //Field codes!
			parseFieldCodes(&appS,&(buf[0]));
			lastFieldsPos=ThreadData_filePosition-counter;
		}
	}
	//Go back to just after last Field descriptor, or start of file
	if (fseek(ThreadData_fp, lastFieldsPos, SEEK_SET)!= 0) {
		AfxMessageBox("Failed to set file position.");
		return;
	}
	if (appS.ConfirmedUnicodeFormat) {
		ernogetc(ThreadData_fp,true); //Read and ignore the FF FE
	}

	ThreadData_filePosition=lastFieldsPos;
	ThreadData_filePosition+=getLine(ThreadData_fp,appS.ConfirmedUnicodeFormat,buf,sizeof(buf)-1);
		

	//***************Test log entries*************************//
	//********************************************************//
	if (buf==strstr(buf,"#Fields:")) { //read past the field codes
		ThreadData_filePosition+=getLine(ThreadData_fp,appS.ConfirmedUnicodeFormat, buf,sizeof(buf)-1); 
	}


	counter=getLine(ThreadData_fp,appS.ConfirmedUnicodeFormat,buf,sizeof(buf)-1);
	if (counter< 1) {
		AfxMessageBox("Log file empty after last description of field. Either repeast this\nprocess when entries are available, or specify an older log file with\nthe same settings during the identification period.");
		return;
	}
	cleanInput((unsigned char*)&(buf[0]),(unsigned char*)&(output[0]),sizeof(buf)-1); //fills output
	GetOwnIP(); //So that static variables needed in parseMessage is prepared
	parseMessage((unsigned char*)&(output[0]),(unsigned char*)&(buf[0]),&appS); //fills buf

	mess="\nResults of parse test:\n\n";

	//unicode

	if (appS.ConfirmedUnicodeFormat) {
		mess+="File format is confirmed unicode.\n\n";
		CheckDlgButton(IDC_UNICODE,BST_CHECKED);
	}else {
		mess+="Standard character format identified (not unicode).\n\n";
	}

	//date and time
	if ((appS.failedToParseDate==false)&(appS.failedToParseTime==false)) { 
		CheckDlgButton(IDC_PARSE_DATE,BST_CHECKED);
		mess+="Date and Time information found.\n\n";
	} else if (((appS.failedToParseDate|appS.failedToParseTime)==true)&((appS.failedToParseDate&appS.failedToParseTime)==false)) {
		CheckDlgButton(IDC_PARSE_DATE,BST_CHECKED);
		if (appS.failedToParseDate==true) 
			mess+="Time information found, but Date information NOT found! SyslogAgent adds Date information.\n\n";
		else
			mess+="Date information found, but Time information NOT found! SyslogAgent adds Time information.\n\n";
	} else {
		mess+="No time or Date information found.\n\n";
		CheckDlgButton(IDC_PARSE_DATE,BST_UNCHECKED);
	}

	//Host
	if (appS.failedToParseHost==false) { 
		mess+="Host information found.\n\n";
		CheckDlgButton(IDC_PARSE_HOST,BST_CHECKED);
	} else {
		mess+="No Host information found. SyslogAgent will add host information.\n\n";
		CheckDlgButton(IDC_PARSE_HOST,BST_UNCHECKED);
	}

	//Severity
	if (appS.failedToParseSeverity==false) { 
		mess+="Severity information found.\n\n";
		CheckDlgButton(IDC_PARSE_SEVERITY,BST_CHECKED);
	} else {
		mess+="No Severity information found. Set manually.\n\n";
		CheckDlgButton(IDC_PARSE_SEVERITY,BST_UNCHECKED);
	}

	//Process
	char Process[256];
	argCounter=sscanf_s(buf,"%*s %*s %*s %*s %[^[]",&Process[0],sizeof(Process));
	if (argCounter!=1) {
		appS.failedToParseProcess=true;
	}
	if (appS.failedToParseProcess==false) { 
		mess+="Process name ";
		mess+=Process;
		mess+=" suggested. Set manually if wrong or if another name is desired.\n\n";
		CheckDlgButton(IDC_PARSE_PROCESS,BST_CHECKED);
	} else {
		mess+="No Process information found. Set manually.\n\n";
		CheckDlgButton(IDC_PARSE_PROCESS,BST_UNCHECKED);
	}

	if (appS.prefix!="") {
		mess+="Found comment prefix character <";
		mess+=appS.prefix;
		mess+=">. Change manually if wrong or if comment lines should be sent to Syslogserver.\n\n";
		CheckDlgButton(IDC_USE_PREFIX,1);
		prefix.SetWindowText(appS.prefix);
	} else {
		prefix.SetWindowText("");
		CheckDlgButton(IDC_USE_PREFIX,BST_UNCHECKED);
	}

	mess+="Choose desired facility - default Local6 set.\n\n";
	Facility.SetCurSel(22); //local6

	OnBnClickedUsePrefix(); //Updates status
	OnBnClickedUseIgnoreLines();
	OnBnClickedParseProcess();
	OnBnClickedParseSeverity();
	
	Sleep(300);
	AfxMessageBox(mess);

}

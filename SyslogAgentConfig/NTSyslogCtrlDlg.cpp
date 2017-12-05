// NTSyslogCtrlDlg.cpp : implementation file
//

#include "..\Syslogserver\common_stdafx.h"
//#include "stdafx.h"
#include "NTSyslogCtrl.h"
#include "NTService.h"
#include "NTSyslogCtrlDlg.h"
#include "ConfAppl.h"

#include "ConfigLogging.h"
#include "process.h"
#include <shlobj.h>

#include "..\Syslogagent\RegistrySettings.h"
#include "..\Syslogserver\common_registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void logger(int severity,char *text,...) {
	va_list args;
	va_start(args,text);
	char outText[1024];
	_vsnprintf_s(outText,1024,text,args);
	AfxMessageBox(text, MB_ICONSTOP);
}

//Dummy, since this is not used in the gui application. For the syslog agent, however, it is used.
extern "C" {
	void CreateMiniDump( EXCEPTION_POINTERS* pep ){
	}
}



/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//MAYBE HAVE TO COMMENT THIS OUT AND CALL IT
//THE WAY THE OTHER ONE IS CALLED
void CNTSyslogCtrlDlg::OnAppAbout()
{
	CAboutDlg aDlg;
	aDlg.DoModal();
}

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//CDialog(IDD_ABOUTBOX).DoModal();
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Add extra initialization here
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlDlg dialog

CNTSyslogCtrlDlg::CNTSyslogCtrlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNTSyslogCtrlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNTSyslogCtrlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//***MUST*** be called ***AFTER*** setting the m_csComputer property for
//CNTSyslogCtrlDlg objects
void CNTSyslogCtrlDlg::ReadMachineEventLogSettings(int good_query)
{
	HKEY			hKeyRemote,
					hKey;

	//Sometimes connecting to a remote computer takes a while
	//Especially if that computer doesn't exist on the network
	//or is off-line
	CWaitCursor		cWait;

	//reset the EventlogSelect property & free memory to avoid memory leaks
	m_csaEventlogSelect.RemoveAll();
	m_csaEventlogSelect.FreeExtra();

	//set the EventlogSelect size to the default
	m_csaEventlogSelectSize = 1;

	//Check if query service status was successful
	//if it wasn't skip attempting to connect to the registry
	//Attempting to connect if the computer is unreachable
	//adds about 15-30 seconds on loading time each time
	//an unreachable computer is selected
	if(good_query == TRUE)
	{
		// Connect to the selected computer's registry on HKLM
		if (RegConnectRegistry( (char*)(LPCTSTR)m_csComputer, HKEY_LOCAL_MACHINE, &hKeyRemote) == ERROR_SUCCESS)
		{
			CStringArray __tmp;
			//Open the key to where Windows stores EventLog info
			if (RegOpenKeyEx( hKeyRemote, EVENTLOG_REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				//Read the subkeys in HKLM\System\CurrentControlSet\Services\Eventlog
				DWORD no_subkeys, subkey_max_len;
				if(::RegQueryInfoKey(hKey,NULL,NULL,NULL,&no_subkeys,&subkey_max_len,NULL,NULL,NULL,NULL,NULL,NULL) == ERROR_SUCCESS )
				{
					subkey_max_len++;
					m_csaEventlogSelectSize = no_subkeys;

					//loop until done reading all subkeys
					for(DWORD index = 0; index < no_subkeys; index++ )
					{
						CString buffer;
						DWORD buffer_size = subkey_max_len;

						LONG retval = RegEnumKeyEx( hKey, index, buffer.GetBuffer(buffer_size), &buffer_size, NULL, NULL, NULL, NULL);
						if(retval == ERROR_SUCCESS && retval != ERROR_NO_MORE_ITEMS)
						{
							__tmp.Add((LPCSTR)buffer);
						}//end if(retval == ERROR_SUCCESS && retval != ERROR_NO_MORE_ITEMS)
					}//end for(DWORD index = 0; index < no_subkeys; index++ )
				}//end if(ReqQueryInfoKey(hKeyRemote,NULL,NULL,NULL,&no_subkeys,&subkey_max_len,NULL,NULL,NUL.NULL,NULL) == ERROR_SUCCESS)
				//don't need the handles to the Registry anymore
				RegCloseKey(hKey);
				RegCloseKey(hKeyRemote);

				//populate the m_csaEventlogSelect CString array
				//no apparent need to sort this as RegEnumKeyEx appears
				//to be returning the registry key names in alphabetical order
				for(DWORD i = 0; i < no_subkeys; i++)
				{
					m_csaEventlogSelect.Add(__tmp.GetAt(i));
				}//end for(DWORD i = 0; i < no_subkeys; i++)
			}//end if (RegOpenKeyEx( hKeyRemote, EVENTLOG_REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		}//end if (RegConnectRegistry( (char*)((LPCTSTR)m_csComputer), HKEY_LOCAL_MACHINE, &hKeyRemote) != ERROR_SUCCESS)
	}//end if(good_query == TRUE)
	else
	{
		//Add extra notification to the soon to be disabled combobox
//		m_csaEventlogSelect.Add("Error Connecting to Registry");
		m_csaEventlogSelect.Add(" ");
	}//end else clause of if(good_query == TRUE)

	//Now need to populate the combo box with data
	// Get pointer to the combo boxe.
	CComboBox *peType = (CComboBox *)GetDlgItem(IDC_EVENTLOG_SELECT);
	peType->ResetContent();

	// If it doesn't exist stop rather than crashing.  Should never happen.
	if(peType == NULL)	{
		AfxMessageBox("Program error: Dialog now missing controls.  This "
				"message should never appear.", MB_ICONSTOP);
		CDialog::OnCancel();
	}

	for(DWORD i = 0; i < m_csaEventlogSelectSize; i++)	{
		peType->AddString( m_csaEventlogSelect.GetAt(i));
	}

	//add something here to set the combobox to the first value in the list
	peType->SelectString(0,m_csaEventlogSelect.GetAt(0));

}//end NYSyslogCtrlDlg::ReadMachineEventLogSettings()

void CNTSyslogCtrlDlg::DoDataExchange(CDataExchange* pDX){
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNTSyslogCtrlDlg)
	DDX_Control(pDX, IDC_STATUS_LIGHT, m_StatusIcon);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_APPLICATION_LIST, m_ApplicationList);
}

BEGIN_MESSAGE_MAP(CNTSyslogCtrlDlg, CDialog)
	//{{AFX_MSG_MAP(CNTSyslogCtrlDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//erno	ON_BN_CLICKED(IDC_SELECT_COMPUTER, OnSelectComputer)
	ON_WM_TIMER()
	//ON_BN_CLICKED(IDC_SYSLOGD, OnSyslogd)
	ON_BN_CLICKED(IDC_EVENTLOG, OnEventLog)
	ON_BN_CLICKED(IDC_START, OnStartService)
	ON_BN_CLICKED(IDC_STOP, OnStopService)
	ON_BN_CLICKED(IDC_ABOUTBOX, OnAppAbout)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_INSTALL, OnBnClickedInstall)
	ON_BN_CLICKED(IDC_HELPBOX, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_FORWARD_EVENTS, OnBnClickedForwardEvents)
	ON_BN_CLICKED(IDC_FORWARD_APPLICATION, OnBnClickedForwardApplication)
	ON_BN_CLICKED(IDC_ADD_APPLICATION, OnBnClickedAddApplication)
	ON_BN_CLICKED(IDC_USEPING, OnBnClickedUseping)
	ON_LBN_SELCHANGE(IDC_APPLICATION_LIST, OnLbnSelchangeApplicationList)
	ON_BN_CLICKED(IDC_REMOVEAPPLICATION, OnBnClickedRemoveapplication)
	ON_BN_CLICKED(IDC_EDIT_APPLICATION, OnBnClickedEditApplication)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_MESSAGE(WM_HELP, OnHelpCommand)
	ON_EN_CHANGE(IDC_FILTERARRAY, OnEnChangeFilterarray)
	ON_BN_CLICKED(IDC_USE_MIRROR, OnBnClickedUseMirror)
	ON_BN_CLICKED(IDC_RADIO_UDP, OnBnClickedRadioUdp)
	ON_BN_CLICKED(IDC_RADIO_UDP_PING, OnBnClickedRadioUdpPing)
	ON_BN_CLICKED(IDC_RADIO_TCP, OnBnClickedRadioTcp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNTSyslogCtrlDlg message handlers

BOOL CNTSyslogCtrlDlg::OnInitDialog(){
	CString keyName;
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	//Disable tcp support - it's not tested and a new version needed to be published.
	GetDlgItem(	IDC_RADIO_TCP)->ShowWindow(false);
	
	// Load last computer from the registry
	m_csComputer = AfxGetApp()->GetProfileString( COMPUTERS_SECTION, LAST_COMPUTER_ENTRY);

	SetComputerName();
	int queryserviceresults = ( QueryServiceStatus() ) ? 1 : 0;
	SetMainDialogControls(queryserviceresults);
	ReadMachineEventLogSettings(queryserviceresults);

	//Read syslog server reg info
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	ReadRegKey(&reg_primary,_T("0.0.0.0"),PRIMARY_SYSLOGD_ENTRY);
	ReadRegKey(&reg_secondary,_T("0.0.0.0"),BACKUP_SYSLOGD_ENTRY);
	ReadRegKey(&m_usePing,false,USE_PING_ENTRY);
	ReadRegKey(&m_deliveryMode,false,TCP_DELIVERY);

	if (m_usePing|m_deliveryMode)
		GetDlgItem(	IDC_USE_MIRROR)->EnableWindow( FALSE);

	bool ForwardApplication,ForwardEvents, ForwardToMirror;
	ReadRegKey(&ForwardToMirror,false,FORWARDTOMIRROR);

	if (!ForwardToMirror) {
		GetDlgItem(	IDC_BACKUP_SYSLOGD)->EnableWindow( FALSE);
		GetDlgItem(	IDC_PORT2)->EnableWindow( FALSE);
	}

	ReadRegKey(&ForwardApplication,false,FORWARDAPPLICATIONLOGS);
	ReadRegKey(&ForwardEvents,true,FORWARDEVENTLOGS);
	
	ReadRegKey((unsigned int*)&reg_Port,514,PORT_ENTRY);
	ReadRegKey((unsigned int*)&reg_Port2,514,PORT_BACKUP_ENTRY);
	ReadRegKey(&reg_FilterArray,"",EVENTIDFILTERARRAY);

	CloseRegistry();

	//fill application log listBox
	UpdateApplicationList();

	//Set syslog server info in window
	SetDlgItemText( IDC_PRIMARY_SYSLOGD, (LPCTSTR)reg_primary);
	SetDlgItemText( IDC_BACKUP_SYSLOGD, (LPCTSTR)reg_secondary);
	CheckDlgButton(IDC_FORWARD_APPLICATION, ForwardApplication);
	CheckDlgButton(IDC_FORWARD_EVENTS, ForwardEvents);
	CheckDlgButton(IDC_USE_MIRROR, ForwardToMirror);
	SetDlgItemInt(IDC_PORT,reg_Port,TRUE);
	SetDlgItemInt(IDC_PORT2,reg_Port2,TRUE);

	//Set radio button
	if (m_deliveryMode==1) { //TCP
		CheckDlgButton(IDC_RADIO_TCP, true);
	} else {
		if (m_usePing==0) 
			CheckDlgButton(IDC_RADIO_UDP, true);
		else
			CheckDlgButton(IDC_RADIO_UDP_PING, true);
	}

	
	CheckDlgButton(IDC_USEPING, m_usePing);
	

	SetDlgItemText( IDC_FILTERARRAY, (LPCTSTR)reg_FilterArray);
	

//	KillTimer(2); //Otherwise popup 'change settings' after initialization
	SetTimer( 1, 1000, NULL);
	SetTimer( 2, 1000, NULL);

	m_RestartServiceQuestionPlanned=false;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNTSyslogCtrlDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CNTSyslogCtrlDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CNTSyslogCtrlDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CNTSyslogCtrlDlg::OnSelectComputer() 
{

	//Erno This code can not be reached now. Button 'Select computer', that supported remote configuration, was removed.
	// Remote installation is instead performed via commandLine/A.D./Logon scripts/SMS/...
	// The original only supported service configuration, not installation (=reason not to support it, lots of work)
/*	CSelectServerDlg	cDlg;

	cDlg.SetCurrentComputer( m_csComputer);
	if (cDlg.DoModal() == IDCANCEL)
		return;
	//Set a wait cursor -- PERHAPS THIS IS CONFUSING THE ISSUE
	CWaitCursor				 cWait;
	m_csComputer = cDlg.GetNewComputer();
	// Write computer name to the registry
	AfxGetApp()->WriteProfileString( COMPUTERS_SECTION, LAST_COMPUTER_ENTRY, m_csComputer);
	SetComputerName(); 
	KillTimer( 1);
	//Check for service on remote machine and set dialog buttons
	int queryserviceresults = (QueryServiceStatus()) ? 1 : 0;
	SetMainDialogControls(queryserviceresults);
	//Setup the dialog with the new machine settings, if any
	ReadMachineEventLogSettings(queryserviceresults);
	*/
}

//This function either enables or disables the contained buttons
//from the main dialog.  Use 1 to enable, 0 to disable.  Any other
//buttons added later which require enabling or disabling should
//be placed here
void CNTSyslogCtrlDlg::SetMainDialogControls(int _i_)
{

	GetDlgItem( IDC_EDIT_APPLICATION)->EnableWindow( _i_);
	GetDlgItem( IDC_ADD_APPLICATION)->EnableWindow( _i_);
	GetDlgItem( IDC_EVENTLOG)->EnableWindow( _i_);
	GetDlgItem( IDC_EVENTLOG_SELECT)->EnableWindow( _i_);
}

void CNTSyslogCtrlDlg::OnTimer(UINT_PTR nIDEvent) {
	// Add your message handler code here and/or call default
	CString csPrimary,csBackup,charPort,csFilterArray;
	int Port,Port2;


	CDialog::OnTimer(nIDEvent);

	if (nIDEvent==1) {
		if (QueryServiceStatus())
			SetTimer( 1, 1000, NULL);
	}

	if (nIDEvent==2) { //user changing ip or port settings
		
		//Read status
		GetDlgItemText(IDC_PORT,charPort);
		Port=atoi(charPort);
		GetDlgItemText(IDC_PORT2,charPort);
		Port2=atoi(charPort);
		GetDlgItemText( IDC_PRIMARY_SYSLOGD, csPrimary);
		GetDlgItemText( IDC_BACKUP_SYSLOGD,csBackup);

		GetDlgItemText( IDC_FILTERARRAY,csFilterArray);

		//if different from registry, save changes and prepare to ask for service restart (via timer)
		if ((reg_primary!=csPrimary)||(reg_secondary!=csBackup)||(reg_Port!=Port)||(reg_Port2!=Port2)||(reg_FilterArray!=csFilterArray)){
			OpenRegistry(NTSYSLOG_SYSLOG_KEY);
			WriteRegKey(&csPrimary,PRIMARY_SYSLOGD_ENTRY);

			WriteRegKey(&csBackup,BACKUP_SYSLOGD_ENTRY);
			WriteRegKey((unsigned int*)&Port,PORT_ENTRY);
			WriteRegKey((unsigned int*)&Port2,PORT_BACKUP_ENTRY);
			WriteRegKey(&csFilterArray,EVENTIDFILTERARRAY);
			
			CloseRegistry();

			reg_primary=csPrimary;
			reg_secondary=csBackup;
			reg_Port=Port;
			reg_Port2=Port2;
			reg_FilterArray=csFilterArray;
			
			KillTimer(daTimer);
			daTimer=SetTimer( 3, 2000, NULL);
			m_RestartServiceQuestionPlanned=true;
		}
		SetTimer( 2, 1000, NULL);
	}
	if (nIDEvent==3) { //Time to ask to restart service
		KillTimer(daTimer);
		m_RestartServiceQuestionPlanned=false;
		CheckRestartService();
	}
}

//Check the status of the NTSyslog service on the selected computer
BOOL CNTSyslogCtrlDlg::QueryServiceStatus()
{
	CNTScmService			 myService;
	CNTServiceControlManager mySCM;
	SERVICE_STATUS			 myServiceStatus;
	CString					 csComputer;
	TCHAR					 InstallStatus[32];

	if (!m_csComputer.IsEmpty())
		// Set computer in \\computer_name format
		csComputer.Format( _T( "\\\\%s"), m_csComputer);
	else
		csComputer.Empty();
	if (!mySCM.Open( csComputer, SC_MANAGER_ALL_ACCESS|GENERIC_READ))
	{
		int apa=GetLastError();
		if (apa==5) {
			AfxMessageBox( _T( "Permission denied to read service status. Please execute program as administrator."), MB_ICONSTOP);
			exit(0);
		}
		DisplayStatus( IDI_ERROR_ICON);
		SetMainDialogControls(false);
		return FALSE;
	}
	if (!mySCM.OpenService( NTSYSLOG_SERVICE_NAME, SERVICE_QUERY_STATUS, myService)){
		//AfxMessageBox( _T( "Error OpenService"), MB_ICONSTOP);
		DisplayStatus( IDI_ERROR_ICON);
		mySCM.Close();
		SetMainDialogControls(false);
		return FALSE;
	}
	if (!myService.QueryStatus( &myServiceStatus))
	{
		AfxMessageBox( _T( "Error Query"), MB_ICONSTOP);
		DisplayStatus( IDI_ERROR_ICON);
		SetMainDialogControls(false);
		myService.Close();
		mySCM.Close();
		return FALSE;
	}

	//We're only here is status is good
	GetDlgItemText(IDC_INSTALL,&(InstallStatus[0]),32);
	if ((strcmp(InstallStatus,"Uninstall"))!=0) {
		SetDlgItemText( IDC_INSTALL, _T( "Uninstall"));
	}
	SetMainDialogControls(true);

	myService.Close();
	mySCM.Close();
	switch (myServiceStatus.dwCurrentState)
	{
	case SERVICE_START_PENDING:
		DisplayStatus( IDI_YELLOW_ICON, myServiceStatus.dwCurrentState);
		break;
	case SERVICE_RUNNING:
		DisplayStatus( IDI_GREEN_ICON);
		break;
	case SERVICE_STOP_PENDING:
		DisplayStatus( IDI_YELLOW_ICON, myServiceStatus.dwCurrentState);
		break;
	case SERVICE_STOPPED:
		DisplayStatus( IDI_RED_ICON);
		break;
	case SERVICE_CONTINUE_PENDING:
		DisplayStatus( IDI_YELLOW_ICON, myServiceStatus.dwCurrentState);
		break;
	case SERVICE_PAUSE_PENDING:
		DisplayStatus( IDI_YELLOW_ICON, myServiceStatus.dwCurrentState);
		break;
	case SERVICE_PAUSED:
		DisplayStatus( IDI_YELLOW_ICON, myServiceStatus.dwCurrentState);
		break;
	default:
		DisplayStatus( IDI_ERROR_ICON);
		break;
	}
	return TRUE;
}

BOOL CNTSyslogCtrlDlg::DisplayStatus(UINT nIconID, DWORD dwServiceState)
{
	HICON	hIcon;
	TCHAR InstallStatus[32];

	if ((hIcon = AfxGetApp()->LoadIcon( nIconID)) == NULL)
		return FALSE;
	m_StatusIcon.SetIcon( hIcon);
	switch (nIconID)
	{
	case IDI_GREEN_ICON: // Service started
		SetDlgItemText( IDC_STATUS, _T( "Service is running."));
		GetDlgItem( IDC_START)->EnableWindow( FALSE);
		GetDlgItem( IDC_STOP)->EnableWindow( TRUE);
		GetDlgItem( IDC_INSTALL)->EnableWindow( FALSE);
		break;
	case IDI_YELLOW_ICON: // Service pending
		switch (dwServiceState)
		{
		case SERVICE_START_PENDING:
			SetDlgItemText( IDC_STATUS, _T( "Service is starting."));
			break;
		case SERVICE_STOP_PENDING:
			SetDlgItemText( IDC_STATUS, _T( "Service is stopping."));
			break;
		case SERVICE_CONTINUE_PENDING:
			SetDlgItemText( IDC_STATUS, _T( "Service is leaving pause."));
			break;
		case SERVICE_PAUSE_PENDING:
			SetDlgItemText( IDC_STATUS, _T( "Service is entering in pause."));
			break;
		case SERVICE_PAUSED:
			SetDlgItemText( IDC_STATUS, _T( "Service is paused."));
			break;
		default:
			SetDlgItemText( IDC_STATUS, _T( "Unknown state!"));
			break;
		}
		GetDlgItem( IDC_START)->EnableWindow( FALSE);
		GetDlgItem( IDC_STOP)->EnableWindow( FALSE);
		break;
	case IDI_RED_ICON: // Service stopped
		SetDlgItemText( IDC_STATUS, _T( "Service is stopped."));
		GetDlgItem( IDC_START)->EnableWindow( TRUE);
		GetDlgItem( IDC_STOP)->EnableWindow( FALSE);
		GetDlgItem( IDC_INSTALL)->EnableWindow( TRUE);
		break;
	case IDI_ERROR_ICON: // Error
	default:

		GetDlgItemText(IDC_INSTALL,&(InstallStatus[0]),32);
		if ((strcmp(InstallStatus,"Install"))!=0) {
			SetDlgItemText( IDC_INSTALL, _T( "Install"));
		}
		SetDlgItemText( IDC_STATUS, _T( "Service is not installed."));
		GetDlgItem( IDC_START)->EnableWindow( FALSE);
		GetDlgItem( IDC_STOP)->EnableWindow( FALSE);
		
		break;
	}
	return TRUE;
}

void CNTSyslogCtrlDlg::OnEventLog()
{

	CConfigLogging cDlg;

	// Get pointers to the combo boxes.
	CComboBox *peType = (CComboBox *)GetDlgItem(IDC_EVENTLOG_SELECT);

	// If doesn't exist, stop rather than crashing.  Should never happen.
	if(peType == NULL)
	{
		AfxMessageBox("Program error: Dialog now missing controls.  This "
				"message should never appear.", MB_ICONSTOP);
		CDialog::OnCancel();
	}

	// Get the selection.  Depends on the dialog having the right items in it.
	int cur_index = peType->GetCurSel();
	
	// Again, make sure we got values back.
	if(cur_index == CB_ERR)
	{
		AfxMessageBox("Program error: Dialog now returns errors.  This "
				"message should never appear.", MB_ICONSTOP);
		CDialog::OnCancel();
	}

	cDlg.SetupDialog(m_csaEventlogSelect.GetAt(cur_index), m_csComputer);

	if (cDlg.DoModal()!=IDCANCEL)

		CheckRestartService();

}

void CNTSyslogCtrlDlg::SetComputerName()
{
	CString	csMessage;

//erno csMessage.Format( _T( "Service status on computer <%s>..."),
//erno					 (m_csComputer.IsEmpty() ? _T( "Local Machine") : m_csComputer));
	csMessage.Format( _T( "Service status"));

	SetDlgItemText( IDC_COMPUTER, csMessage);
}

void CNTSyslogCtrlDlg::OnStartService() {
	// Add your control notification handler code here
	CNTScmService			 myService;
	CNTServiceControlManager mySCM;
	CString					 csComputer;
	CWaitCursor				 cWait;
	TCHAR					 szBuffer[255];
	HKEY					 hKeyRemote,hKeySoftware;
	DWORD					 dwValue,dwSize,dwType;


	//Make sure any very recent changes are considered
	KillTimer(2); //Otherwise popup 'change settings' after initialization
	OnTimer(2);
	KillTimer(3); //dont set restart question flag
	m_RestartServiceQuestionPlanned=false;

	// Connect to the registry on HKLM
	if (RegConnectRegistry( (char*)((LPCTSTR)""), HKEY_LOCAL_MACHINE, &hKeyRemote) != ERROR_SUCCESS)
	{
		AfxMessageBox( _T( "Error while connecting to the registry!\n\nPlease retry."), MB_ICONSTOP);
		return;
	}
	// Create the SOFTWARE\SaberNet key or open it if it exists
	if (RegCreateKeyEx( hKeyRemote, NTSYSLOG_SOFTWARE_KEY, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_WRITE|KEY_READ, NULL, &hKeySoftware, &dwValue) != ERROR_SUCCESS)
	{
		AfxMessageBox( _T( "Error writing new parameters!\n\nCheck permissions to the registry(local machine\\software\\\ndatagram\\syslogagent)\n Please retry."), MB_ICONSTOP);
		RegCloseKey (hKeyRemote);
		return;
	}
	// Read the primary syslogd server
	//dwSize = szBuffer.GetLength();
	dwSize = 255*sizeof( TCHAR);
	memset( szBuffer, 0, 255*sizeof( TCHAR));
	if (RegQueryValueEx( hKeySoftware, PRIMARY_SYSLOGD_ENTRY, 0, &dwType, (LPBYTE) szBuffer, &dwSize) != ERROR_SUCCESS)
	{
		AfxMessageBox( _T( "Please enter primary IP address before starting service."), MB_ICONSTOP);
		RegCloseKey (hKeySoftware);
		RegCloseKey( hKeyRemote);
		return;
	}

	RegCloseKey (hKeySoftware);
	RegCloseKey( hKeyRemote);

	if ((strcmp(szBuffer,"")==0)||(strcmp(szBuffer,"0.0.0.0")==0)) {
		AfxMessageBox( _T( "No Syslog Server address has been entered!\n\nPlease enter address before starting service."), MB_ICONSTOP);
		return;
	}

	if (!m_csComputer.IsEmpty())
		// Set computer in \\computer_name format
		csComputer.Format( _T( "\\\\%s"), m_csComputer);
	else
		csComputer.Empty();
	if (!mySCM.Open( csComputer, SC_MANAGER_ALL_ACCESS|GENERIC_READ))
	{
		AfxMessageBox( _T( "Unable to contact Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	if (!mySCM.OpenService( NTSYSLOG_SERVICE_NAME, SERVICE_START, myService))
	{
		mySCM.Close();
		AfxMessageBox( _T( "Unable to send command to Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	if (!myService.Start( 0, NULL))
	{
		myService.Close();
		mySCM.Close();
		AfxMessageBox( _T( "Error while sending command to Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	myService.Close();
	mySCM.Close();

	GetDlgItem( IDC_INSTALL)->EnableWindow( FALSE);

	QueryServiceStatus();
}

void CNTSyslogCtrlDlg::OnStopService() {
	// Add your control notification handler code here
	CNTScmService			 myService;
	CNTServiceControlManager mySCM;
	CString					 csComputer;
	CWaitCursor				 cWait;

	if (!m_csComputer.IsEmpty())
		// Set computer in \\computer_name format
		csComputer.Format( _T( "\\\\%s"), m_csComputer);
	else
		csComputer.Empty();
	if (!mySCM.Open( csComputer, SC_MANAGER_ALL_ACCESS|GENERIC_READ))
	{
		AfxMessageBox( _T( "Unable to contact Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	if (!mySCM.OpenService( NTSYSLOG_SERVICE_NAME, SERVICE_STOP, myService))
	{
		mySCM.Close();
		AfxMessageBox( _T( "Unable to send command to Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	if (!myService.Stop())
	{
		myService.Close();
		mySCM.Close();
		AfxMessageBox( _T( "Error while sending command to Service Control Manager!"), MB_ICONSTOP);
		return;
	}
	myService.Close();
	mySCM.Close();
	QueryServiceStatus();
}

void CNTSyslogCtrlDlg::OnBnClickedInstall(){
	// Add your control notification handler code here
	CNTScmService			 myService;
	CNTServiceControlManager mySCM;
	CString					 csComputer;
	CWaitCursor				 cWait;
	TCHAR szModulePathname[260];
	TCHAR InstallStatus[32];

	szModulePathname[0]=_T('"');
	LPCTSTR Dapointer=&szModulePathname[0];

	DWORD dwLength=GetModuleFileName(NULL, &szModulePathname[1], _MAX_PATH);

	if( dwLength ) {
		while( dwLength && szModulePathname[ dwLength ] != _T('\\') ) {
			dwLength--;
		}
		if( dwLength )
			szModulePathname[ dwLength + 1 ] = _T('\000');
	}
	_tcscat_s(szModulePathname,SYSLOG_AGENT_NAME);
	_tcscat_s(szModulePathname,"\"");

	GetDlgItemText(IDC_INSTALL,&(InstallStatus[0]),32);
	if ((strcmp(InstallStatus,"Install"))==0) {

		CNTService myCNTService(Dapointer,"Syslog Agent","Syslog Agent",(DWORD)0xFF,"Forwards Event logs to Syslog Server");

		myCNTService.Install();

		initRegistry(NULL);

		OnInitDialog();
		
		ReadMachineEventLogSettings(true);

	} else if ((strcmp(InstallStatus,"Uninstall"))==0) {
		m_csComputer.IsEmpty();
//		if (!mySCM.Open( csComputer, SC_MANAGER_ALL_ACCESS|GENERIC_READ))
		if (!mySCM.Open( csComputer, SC_MANAGER_ALL_ACCESS))
		{
			DisplayStatus( IDI_ERROR_ICON);
			return ;
		}
		if (!mySCM.OpenService( NTSYSLOG_SERVICE_NAME, SERVICE_ALL_ACCESS, myService))
		{
			DisplayStatus( IDI_ERROR_ICON);
			mySCM.Close();
			return;
		}
		if (!myService.Delete()){
			DisplayStatus( IDI_ERROR_ICON);
			AfxMessageBox( _T( "Failed to delete service!"), MB_ICONSTOP);
			myService.Close();
			mySCM.Close();
			return;
		}

		myService.Close();
		mySCM.Close();

	} else {
		AfxMessageBox( _T( "String compare failed!"), MB_ICONSTOP);
	}
	QueryServiceStatus();
}

void CNTSyslogCtrlDlg::OnBnClickedHelp() //well, actually helpbutton..
{
		char temp[256]="",hlpfilepath[512];

	// Get full pathname of the exe-files
	DWORD dwLength=GetModuleFileName(NULL, &temp[0], (sizeof(temp) / sizeof(temp[0])));
	if( dwLength ) {
		while( dwLength && temp[ dwLength ] != _T('\\') ) {
			dwLength--;
		}
		if( dwLength )
			temp[ dwLength + 1 ] = _T('\000');
	}

	sprintf_s(hlpfilepath,"%s%s",temp,"Datagram SyslogAgent manual.pdf");
	_spawnlp( _P_NOWAIT ,"hh.exe","hh.exe",hlpfilepath,NULL );

}

void CNTSyslogCtrlDlg::OnBnClickedForwardEvents(){
	bool theBool;

	//Set and store now value
	if (IsDlgButtonChecked(IDC_FORWARD_EVENTS))
		theBool=true;
	else
		theBool=false;

	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&theBool,FORWARDEVENTLOGS);
	CloseRegistry();

	CheckRestartService();

}

void CNTSyslogCtrlDlg::OnBnClickedForwardApplication(){
	bool theBool;

	if (IsDlgButtonChecked(IDC_FORWARD_APPLICATION))
		theBool=true;
	else
		theBool=false;

	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&theBool,FORWARDAPPLICATIONLOGS);
	CloseRegistry();

	//If any application logging configured, ask for restart of service
	if (m_ApplicationList.GetCount()>0)
		CheckRestartService();
}

void CNTSyslogCtrlDlg::OnBnClickedUseping(){
	bool aBool;
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	if (IsDlgButtonChecked(IDC_USEPING)){
		aBool=true;
		AfxMessageBox( _T( "Please note that this option only affects Event logging, not application\nlogging, as SyslogAgent does not control the application logs."),MB_OK);
	} else {
		aBool=false;
	}
	WriteRegKey(&aBool,USE_PING_ENTRY);
	CloseRegistry();

	CheckRestartService();
}

void CNTSyslogCtrlDlg::OnLbnSelchangeApplicationList(){
	// Add your control notification handler code here
}

void CNTSyslogCtrlDlg::UpdateApplicationList(void){  //fill application log listBox
	CString keyName;
	
	m_ApplicationList.ResetContent();

	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	GoToRegKey(APPLICATIONLOGS);

	while (GetNextKey(&keyName)) {
		m_ApplicationList.AddString(keyName);
	}
	CloseRegistry();
}

void CNTSyslogCtrlDlg::OnBnClickedAddApplication(){
	CConfAppl cDlg;

	//cDlg.SetCurrentComputer( m_csComputer);
	if (cDlg.DoModal() == IDCANCEL)
		return;

	UpdateApplicationList();

	CheckRestartService();

}

void CNTSyslogCtrlDlg::OnBnClickedRemoveapplication(){
	int index;
	CString name;
	index=m_ApplicationList.GetCurSel();
	m_ApplicationList.GetText(index,name);
	m_ApplicationList.DeleteString(index);

	if (name=="") {
		return; //nothing chosen
	}
	//Remove from registry
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	GoToRegKey(APPLICATIONLOGS);
	DeleteKey(name);
	CloseRegistry();

	CheckRestartService();

}

void CNTSyslogCtrlDlg::OnBnClickedEditApplication()
{
	int index;
	CString name;
	CConfAppl cDlg;

	//Ge info om vilken application som ska editeras
	index=m_ApplicationList.GetCurSel();
	if (index==-1) { //Nothing selected
		return;
	}
	m_ApplicationList.GetText(index,name);

	cDlg.SetupDialog(name);

	//cDlg.SetCurrentComputer( m_csComputer);
	if (cDlg.DoModal() == IDCANCEL)
		return;

    UpdateApplicationList();

	CheckRestartService();
	
}

void CNTSyslogCtrlDlg::CheckRestartService(void){
	CString text;

	//Service not stopped?

	GetDlgItemText(IDC_STATUS,text);
    if (text!="Service is running.") {
		return;
	}

	//Restart?
	if (AfxMessageBox( _T( "Service must be restarted for any new settings to take effect. Restart service now?"),MB_YESNO|MB_ICONQUESTION) == IDYES) {
		OnStopService();

		GetDlgItemText(IDC_STATUS,text);
		while(text!="Service is stopped.") {
			QueryServiceStatus();
			GetDlgItemText(IDC_STATUS,text);
    		Sleep(300);
		}
		Sleep(200);
		OnStartService();
	}
}

void CNTSyslogCtrlDlg::OnBnClickedCancel(){ // ehh, quit actually...
	CString csPrimary,csBackup,charPort,csFilterArray;
	int Port,Port2;
	GetDlgItemText( IDC_PRIMARY_SYSLOGD, csPrimary);
	if (csPrimary=="0.0.0.0") {
		if (AfxMessageBox( _T( "No Syslog Server address has been entered!\n\nNo entries will be sent. Quit anyway?"),MB_YESNO|MB_ICONQUESTION) == IDNO) {
		return;
		}
	}
	if (m_RestartServiceQuestionPlanned) { //user makes change, then exits quickly...
		KillTimer(daTimer);
		m_RestartServiceQuestionPlanned=false;
		CheckRestartService();
	}  else {//check if user has changed stuff
	//Read status
		GetDlgItemText(IDC_PORT,charPort);
		Port=atoi(charPort);
		GetDlgItemText(IDC_PORT2,charPort);
		Port2=atoi(charPort);
		GetDlgItemText( IDC_PRIMARY_SYSLOGD, csPrimary);
		GetDlgItemText( IDC_BACKUP_SYSLOGD,csBackup);
		GetDlgItemText( IDC_FILTERARRAY,csFilterArray);

		//if different from registry, save changes and prepare to ask for service restart (via timer)
		if ((reg_primary!=csPrimary)||(reg_secondary!=csBackup)||(reg_Port!=Port)||(reg_Port2!=Port2)||(reg_FilterArray!=csFilterArray)){
			OpenRegistry(NTSYSLOG_SYSLOG_KEY);
			WriteRegKey(&csPrimary,PRIMARY_SYSLOGD_ENTRY);

			WriteRegKey(&csBackup,BACKUP_SYSLOGD_ENTRY);
			WriteRegKey((unsigned int*)&Port,PORT_ENTRY);
			WriteRegKey((unsigned int*)&Port2,PORT_BACKUP_ENTRY);
			WriteRegKey(&csFilterArray,EVENTIDFILTERARRAY);
			
			CloseRegistry();

			reg_primary=csPrimary;
			reg_secondary=csBackup;
			reg_Port=Port;
			reg_Port2=Port2;
			reg_FilterArray=csFilterArray;

			CheckRestartService();
			Sleep(1000);
		}	
	}
	OnCancel();
}

LRESULT CNTSyslogCtrlDlg::OnHelpCommand(WPARAM wParam, LRESULT lParam) {
#ifdef SecLog
		return 0; //No helpfile available
#endif
	OnBnClickedHelp();
	return 0;

}
void CNTSyslogCtrlDlg::OnEnChangeFilterarray(){
	// If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// Add your control notification handler code here
}

void CNTSyslogCtrlDlg::OnBnClickedUseMirror(){
	bool theBool;

	//Set and store now value
	if (IsDlgButtonChecked(IDC_USE_MIRROR)) {
		theBool=true;
		GetDlgItem(	IDC_BACKUP_SYSLOGD)->EnableWindow( TRUE);
		GetDlgItem(	IDC_PORT2)->EnableWindow( TRUE);
	} else {
		theBool=false;
		GetDlgItem(	IDC_BACKUP_SYSLOGD)->EnableWindow( FALSE);
		GetDlgItem(	IDC_PORT2)->EnableWindow( FALSE);

	}

	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&theBool,FORWARDTOMIRROR);
	CloseRegistry();

	CheckRestartService();
}

void CNTSyslogCtrlDlg::OnBnClickedRadioUdp(){
	m_deliveryMode=0;
	m_usePing=0;
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&m_deliveryMode,TCP_DELIVERY);
	WriteRegKey(&m_usePing,USE_PING_ENTRY);
	CloseRegistry();

	GetDlgItem(	IDC_USE_MIRROR)->EnableWindow( TRUE);
	CheckRestartService();
}

void CNTSyslogCtrlDlg::OnBnClickedRadioUdpPing(){
	m_deliveryMode=0;
	m_usePing=1;
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&m_deliveryMode,TCP_DELIVERY);
	WriteRegKey(&m_usePing,USE_PING_ENTRY);
	CloseRegistry();

	if (IsDlgButtonChecked(IDC_USE_MIRROR)) {
		AfxMessageBox( _T( "Mirror delivery is only available with standard UDP delivery. Deactivating setting."),MB_ICONINFORMATION);
		CheckDlgButton(IDC_USE_MIRROR, false);
	};
	OnBnClickedUseMirror();
	GetDlgItem(	IDC_USE_MIRROR)->EnableWindow( FALSE);

	CheckRestartService();
}

void CNTSyslogCtrlDlg::OnBnClickedRadioTcp(){
	m_deliveryMode=1;
	m_usePing=0;
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	WriteRegKey(&m_deliveryMode,TCP_DELIVERY);
	WriteRegKey(&m_usePing,USE_PING_ENTRY);
	CloseRegistry();

	if (IsDlgButtonChecked(IDC_USE_MIRROR)) {
		AfxMessageBox( _T( "Mirror delivery is only available with standard UDP delivery. Deactivating setting."),MB_ICONINFORMATION);
		CheckDlgButton(IDC_USE_MIRROR, false);
	};
	OnBnClickedUseMirror();
	GetDlgItem(	IDC_USE_MIRROR)->EnableWindow( FALSE);

	CheckRestartService();
}



extern "C" {
	void DEBUGSERVICE(int indentLevel,char *a,...) {
	//A stub, really. The agent uses this, but the config program does not.
	}
	void DEBUGAPPLPARSE(int indentLevel,char *a,...) {
	//A stub, really. The agent uses this, but the config program does not.
	}
}
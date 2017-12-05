/*
Module : NTSERV.CPP
Purpose: Implementation for a number of MFC classes which 
         encapsulate the whole area of services in NT. 
Created: PJN / 14-07-1997
History: PJN / 17-05-1999 1. Fixed a warning when compiled with VC 6.
                          2. Fixed a bug in CNTEventLogSource::Report  
         PJN / 05-09-1999 1. Addition of more ASSERT's statements to aid in debugging.
         PJN / 02-10-1999 1. Addition of GetProfileStringArray, WriteProfileStringArray,
                             GetProfileBinary and WriteProfileBinary methods to the 
                             CNTService class.
                          2. Renamed some module names
         PJN / 10-10-1999 1. Added support for the description field which services can
                          have on Windows 2000.
                          2. Added accessor functions for the service name, friendly name
                          and the description text.
         

Copyright (c) 1998 by PJ Naughter.  
All rights reserved.

*/

/////////////////////////////////  Includes  //////////////////////////////////
#include "..\Syslogserver\common_stdafx.h"
//#include "stdafx.h"
#include "NTService.h"
#include "NTService_msg.h"
#include "strsafe.h"


/////////////////////////////////  Macros /////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////// CNTService implementation ///////////////////////////
CNTService* CNTService::sm_lpService = NULL;

CNTService::CNTService(LPCTSTR szModulePathname,LPCTSTR lpszServiceName, LPCTSTR lpszDisplayName, DWORD dwControlsAccepted, LPCTSTR lpszDescription)
{
	// Validate our parameters
	ASSERT(lpszServiceName);
	ASSERT(lpszDisplayName);

	CSingleLock l(&m_CritSect, TRUE); // synchronise access to the variables

	m_sServiceName = lpszServiceName;
	m_sDisplayName = lpszDisplayName;
	m_path=szModulePathname;
	m_hStatus = 0;
	m_dwCurrentState = SERVICE_STOPPED;
	m_dwControlsAccepted = dwControlsAccepted;
	if (lpszDescription)
		m_sDescription = lpszDescription;

	// Copy the address of the current object so we can access it from
	// the static member callback functions.
	// WARNING: This limits the application to only one CNTService object. 
	sm_lpService = this;	//hive away the this pointer;

	// Register ourselves as a source for the event log
	m_EventLogSource.Register(NULL, m_sDisplayName);
}

CNTService::~CNTService()
{
	CSingleLock l(&m_CritSect, TRUE); // synchronise access to the variables

	sm_lpService = NULL;
}

BOOL CNTService::ReportStatusToSCM(DWORD dwCurrentState, DWORD dwWin32ExitCode, 
                                   DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	CSingleLock l(&m_CritSect, TRUE); // synchronise access to the variables

	m_dwCurrentState = dwCurrentState;
	SERVICE_STATUS ServiceStatus;
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	// Disable control requests until the service is started
	if (dwCurrentState == SERVICE_START_PENDING)
		ServiceStatus.dwControlsAccepted = 0;    
	else
		ServiceStatus.dwControlsAccepted = m_dwControlsAccepted;

	// May need to thread protect this
	ServiceStatus.dwCurrentState = dwCurrentState;
	ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ServiceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	ServiceStatus.dwCheckPoint = dwCheckPoint;
	ServiceStatus.dwWaitHint = dwWaitHint;

	BOOL bSuccess = ::SetServiceStatus(m_hStatus, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to SetServiceStatus in ReportStatusToSCM, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTService::ReportStatusToSCM()
{
	CSingleLock l(&m_CritSect, TRUE); // synchronise access to the variables

	SERVICE_STATUS ServiceStatus;
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	// Disable control requests until the service is started
	if (m_dwCurrentState == SERVICE_START_PENDING)
		ServiceStatus.dwControlsAccepted = 0;    
	else
		ServiceStatus.dwControlsAccepted = m_dwControlsAccepted;

	ServiceStatus.dwCurrentState = m_dwCurrentState;
	ServiceStatus.dwWin32ExitCode = NO_ERROR;
	ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	BOOL bSuccess = ::SetServiceStatus(m_hStatus, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to SetServiceStatus in ReportStatusToSCM, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

void CNTService::OnStop()
{
	// Derived classes are required to implement
	// their own code to stop a service, all we do is
	// report that we were succesfully stopped

	// Add an Event log entry to say the service was stopped
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_STOPPED, m_sDisplayName);

	ASSERT(FALSE);
}

void CNTService::OnPause()
{
	// Add an Event log entry to say the service was stopped
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_PAUSED, m_sDisplayName);

	// Derived classes are required to implement
	// their own code to pause a service
	ASSERT(FALSE);
}

void CNTService::OnContinue()
{
	// Add an Event log entry to say the service was stopped
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_CONTINUED, m_sDisplayName);

	// Derived classes are required to implement
	// their own code to continue a service
	ASSERT(FALSE);
}

void CNTService::OnInterrogate()
{
	// Default implementation returns the current status
	// as stored in m_ServiceStatus
	ReportStatusToSCM();
}

void CNTService::OnShutdown()
{
	// Add an Event log entry to say the service was stopped
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_SHUTDOWN, m_sDisplayName);

	// Derived classes are required to implement
	// their own code to shutdown a service
	ASSERT(FALSE);
}

void CNTService::OnUserDefinedRequest(DWORD /*dwControl*/)
{
	TRACE(_T("CNTService::OnUserDefinedRequest was called\n"));

	// Default implementation is do nothing
}

void CNTService::ServiceCtrlHandler(DWORD dwControl)
{
	// Just switch on the control code sent to 
	// us and call the relavent virtual function
 	switch (dwControl)
	{
		case SERVICE_CONTROL_STOP: 
		{
			OnStop();
			break;
		}
		case SERVICE_CONTROL_PAUSE:
		{
			OnPause();
		  break;
		}
		case SERVICE_CONTROL_CONTINUE:
		{
			OnContinue();
		  break;
		}
		case SERVICE_CONTROL_INTERROGATE:
		{
			OnInterrogate();
			break;
		}
		case SERVICE_CONTROL_SHUTDOWN:
		{
			OnShutdown();
			break;
		}
		default:
		{
		  OnUserDefinedRequest(dwControl);
			break;
		}
	}

	// Any request from the SCM will be acked by this service
	ReportStatusToSCM();
}

BOOL CNTService::RegisterCtrlHandler()
{
	CSingleLock l(&m_CritSect, TRUE); // synchronise access to the variables

	m_hStatus = ::RegisterServiceCtrlHandler(m_sServiceName, _ServiceCtrlHandler);
	if (m_hStatus == 0)
		TRACE(_T("Failed in call to RegisterServiceCtrlHandler in RegisterCtrlHandler, GetLastError:%d\n"), ::GetLastError());

	return (m_hStatus != 0);
}

void CNTService::ServiceMain(DWORD /*dwArgc*/, LPTSTR* /*lpszArgv*/)
{
	// Default implementation does nothing but asserts, your version should
	// call RegisterCtrlHandler plus implement its own service specific code
	ASSERT(FALSE); 
}

void CNTService::_ServiceCtrlHandler(DWORD dwControl)
{
	// Convert from the SDK world to the C++ world. In this
	// implementation we just use a single static, In the 
	// future we could use a map just like MFC does for HWND
	// to CWnd conversions
	ASSERT(sm_lpService != NULL);
	sm_lpService->ServiceCtrlHandler(dwControl);
}

void CNTService::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Convert from the SDK world to the C++ world. In this
	// implementation we just use a single static, In the 
	// future we could use a map just like MFC does for HWND
	// to CWnds conversions
	ASSERT(sm_lpService != NULL);
	sm_lpService->ServiceMain(dwArgc, lpszArgv);
}

BOOL CNTService::WriteProfileString( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue)
{
	LONG lResult;
	if (lpszEntry == NULL) // delete whole section
	{
		HKEY hAppKey = GetServiceRegistryKey( hRemoteKey, sServiceName);
		if (hAppKey == NULL)
			return FALSE;
		lResult = ::RegDeleteKey(hAppKey, lpszSection);
		RegCloseKey(hAppKey);
	}
	else if (lpszValue == NULL)
	{
		HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
		if (hSecKey == NULL)
			return FALSE;
		// necessary to cast away const below
		lResult = ::RegDeleteValue(hSecKey, (LPTSTR)lpszEntry);
		RegCloseKey(hSecKey);
	}
	else
	{
		HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
		if (hSecKey == NULL)
			return FALSE;
		lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_SZ,
			(LPBYTE)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
		RegCloseKey(hSecKey);
	}
	return lResult == ERROR_SUCCESS;
}

BOOL CNTService::WriteProfileInt( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue)
{
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	LONG lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_DWORD,
		(LPBYTE)&nValue, sizeof(nValue));
	RegCloseKey(hSecKey);
	return lResult == ERROR_SUCCESS;
}

BOOL CNTService::WriteProfileStringArray( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, const CStringArray& array)
{
	ASSERT(lpszSection != NULL);
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	BOOL bSuccess = CNTEventLogSource::SetStringArrayIntoRegistry(hSecKey, lpszEntry, array);
	RegCloseKey(hSecKey);
	return bSuccess;
}

BOOL CNTService::WriteProfileBinary( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes)
{
	ASSERT(lpszSection != NULL);
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	LONG lResult = RegSetValueEx(hSecKey, lpszEntry, NULL, REG_BINARY, pData, nBytes);
	RegCloseKey(hSecKey);
	return lResult == ERROR_SUCCESS;
}

CString CNTService::GetProfileString( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return lpszDefault;
	CString strValue;
	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
		NULL, &dwCount);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
			(LPBYTE)strValue.GetBuffer(dwCount/sizeof(TCHAR)), &dwCount);
		strValue.ReleaseBuffer();
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_SZ);
		return strValue;
	}
	return lpszDefault;
}

UINT CNTService::GetProfileInt( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault)
{
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return nDefault;
	DWORD dwValue;
	DWORD dwType;
	DWORD dwCount = sizeof(DWORD);
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType, (LPBYTE)&dwValue, &dwCount);
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_DWORD);
		ASSERT(dwCount == sizeof(dwValue));
		return (UINT)dwValue;
	}
	return nDefault;
}

BOOL CNTService::GetProfileStringArray( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArray& array)
{
	ASSERT(lpszSection != NULL);
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return FALSE;
	BOOL bSuccess = CNTEventLogSource::GetStringArrayFromRegistry(hSecKey, lpszEntry, array);
	RegCloseKey(hSecKey);
	return bSuccess;
}

BOOL CNTService::GetProfileBinary( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes)
{
	ASSERT(lpszSection != NULL);
	ASSERT(lpszEntry != NULL);
	ASSERT(ppData != NULL);
	ASSERT(pBytes != NULL);
	*ppData = NULL;
	*pBytes = 0;
	HKEY hSecKey = GetSectionKey( hRemoteKey, sServiceName, lpszSection);
	if (hSecKey == NULL)
		return FALSE;

	DWORD dwType, dwCount;
	LONG lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
		NULL, &dwCount);
	*pBytes = dwCount;
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_BINARY);
		*ppData = new BYTE[*pBytes];
		lResult = RegQueryValueEx(hSecKey, (LPTSTR)lpszEntry, NULL, &dwType,
			*ppData, &dwCount);
	}
	RegCloseKey(hSecKey);
	if (lResult == ERROR_SUCCESS)
	{
		ASSERT(dwType == REG_BINARY);
		return TRUE;
	}
	else
	{
		delete [] *ppData;
		*ppData = NULL;
	}
	return FALSE;
}

// returns key for:
//      HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ServiceName\Parameters\lpszSection.
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CNTService::GetSectionKey( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection)
{
	ASSERT(lpszSection != NULL);

	HKEY hSectionKey = NULL;
	HKEY hAppKey = GetServiceRegistryKey( hRemoteKey, sServiceName);
	if (hAppKey == NULL)
		return NULL;

	DWORD dw;
	RegCreateKeyEx(hAppKey, lpszSection, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hSectionKey, &dw);
	RegCloseKey(hAppKey);
	return hSectionKey;
}

// returns key for:
//      HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ServiceName\Parameters
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
HKEY CNTService::GetServiceRegistryKey( HKEY hRemoteKey, LPCTSTR sServiceName)
{
	HKEY hServicesKey = NULL;
	HKEY hParametersKey = NULL;
	HKEY hAppKey = NULL;
	if (RegOpenKeyEx(hRemoteKey, _T("SYSTEM\\CurrentControlSet\\Services"), 0, KEY_WRITE|KEY_READ,
		&hServicesKey) == ERROR_SUCCESS)
	{
		DWORD dw;
		if (RegCreateKeyEx(hServicesKey, sServiceName, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hAppKey, &dw) == ERROR_SUCCESS)
		{
			RegCreateKeyEx(hAppKey, _T("Parameters"), 0, REG_NONE,
				REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
				&hParametersKey, &dw);
		}
	}
	if (hServicesKey != NULL)
		RegCloseKey(hServicesKey);
	if (hAppKey != NULL)
		RegCloseKey(hAppKey);

	return hParametersKey;
}

BOOL CNTService::Run()
{
	//Uncomment the DebugBreak function below when you want to debug your service
	//DebugBreak();

	//Set up the SERVICE table array
	SERVICE_TABLE_ENTRY ServiceTable[2];
	TCHAR pszServiceName[256];
	_tcscpy_s(pszServiceName, 256,m_sServiceName);
	ServiceTable[0].lpServiceName = pszServiceName;
	ServiceTable[0].lpServiceProc = _ServiceMain;
	ServiceTable[1].lpServiceName = 0;
	ServiceTable[1].lpServiceProc = 0;

	//Notify the SCM of our service
	BOOL bSuccess = ::StartServiceCtrlDispatcher(ServiceTable);
	if (!bSuccess)
		TRACE(_T("Failed in call to StartServiceCtrlDispatcher in Run, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTService::Install()
{
	CString csMessage;

	LPCTSTR lpDependencies = __TEXT("EventLog\0");
	// Get this exes full pathname
//	TCHAR szAppPath[_MAX_PATH];
//	GetModuleFileName(NULL, szAppPath, _MAX_PATH);

	//Open up the SCM requesting Creation rights
	CNTServiceControlManager manager;
	if (!manager.Open(NULL, SC_MANAGER_CREATE_SERVICE))
	{
		csMessage.Format(_T("Failed in call to open Service Control Manager in Install, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		return FALSE;
	}

	//Create the new service entry in the SCM database
	CNTScmService service;
	if (!service.Create(manager, m_sServiceName, m_sDisplayName, SERVICE_ALL_ACCESS,
						SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, 
						SERVICE_ERROR_NORMAL, m_path, NULL, 
						NULL, lpDependencies, NULL, NULL))
	{
		csMessage.Format(_T("Failed in call to CreateService in Install, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		return FALSE;
	}

	//Setup this service as an event log source (using the friendly name)
	if (!m_EventLogSource.Install(m_sDisplayName, m_path, EVENTLOG_ERROR_TYPE | 
								  EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE))
	{
		csMessage.Format(_T("Failed in call to install service as an Event log source, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		return FALSE;
	}

	//Add the description text to the registry if need be
	int nDescriptionLen = m_sDescription.GetLength();
	if (nDescriptionLen)
	{
		CString sKey;
		sKey.Format(_T("SYSTEM\\CurrentControlSet\\Services\\%s"), m_sServiceName);
		HKEY hService;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sKey, 0, KEY_WRITE|KEY_READ, &hService) == ERROR_SUCCESS)
		{
			TCHAR* pszDescription = m_sDescription.GetBuffer(nDescriptionLen);
			if (RegSetValueEx(hService, _T("Description"), NULL, REG_SZ, (LPBYTE)pszDescription, nDescriptionLen*sizeof(TCHAR)) != ERROR_SUCCESS)
				TRACE(_T("Failed in call to set service description text, GetLastError:%d\n"), ::GetLastError());

			RegCloseKey(hService);
		}
	}

	//Add an Event log entry to say the service was successfully installed
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_INSTALLED, m_sDisplayName);

	//erno csMessage.Format( _T("Service %s was succesfully installed.\n"), m_sServiceName);
	//erno AfxMessageBox( csMessage, MB_ICONINFORMATION);

	return TRUE;
}

BOOL CNTService::Uninstall()
{
	CString csMessage;

	// Open up the SCM requesting connect rights
	CNTServiceControlManager manager;
	if (!manager.Open(NULL, SC_MANAGER_CONNECT))
	{
		csMessage.Format(_T("Failed in call to open Service Control Manager in Uninstall, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		m_EventLogSource.Report(EVENTLOG_ERROR_TYPE, CNTS_MSG_SERVICE_FAIL_CONNECT_SCM, csMessage);
		return FALSE;
	}

	// Open up the existing service requesting deletion rights
	CNTScmService service;
	if (!manager.OpenService(m_sServiceName, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS, service))
	{
		csMessage.Format(_T("Failed in call to OpenService in Uninstall, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		m_EventLogSource.Report(EVENTLOG_ERROR_TYPE, CNTS_MSG_SERVICE_FAIL_OPEN_SERVICE, csMessage);
		return FALSE;
	}

	// Delete the service from the SCM database
	if (!service.Delete())
	{
		csMessage.Format(_T("Failed in call to DeleteService in Uninstall, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		m_EventLogSource.Report(EVENTLOG_ERROR_TYPE, CNTS_MSG_SERVICE_FAIL_DELETE_SERVICE, csMessage);
		return FALSE;
	}

	//Add an Event log entry to say the service was successfully installed
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_UNINSTALLED, m_sDisplayName);

	//Remove this service as an event log source
	if (!m_EventLogSource.Uninstall(m_sDisplayName))
	{
		csMessage.Format(_T("Failed in call to delete service as an Event log source, GetLastError:%d\n"), ::GetLastError());
		AfxMessageBox( csMessage, MB_ICONSTOP);
		return FALSE;
	}

	csMessage.Format( _T("Service %s was succesfully uninstalled.\n\nNB: you must restart the computer before trying to reinstall it."), m_sServiceName);
	AfxMessageBox( csMessage, MB_ICONINFORMATION);

	return TRUE;
}

void CNTService::Debug()
{
	//Runing as EXE not as service, just execute the services
	//SeviceMain function
	ServiceMain(0, NULL);
}

void CNTService::ShowHelp()
{
	//Default behaviour is to do nothing. In your
	//service application, you should override
	//this function to either display something
	//helpful to the console or if the service
	//is running in the GUI subsystem, to 
	//display a messagebox or dialog to provide
	//info about your service.
}

//Based upon the function of the same name in CWinApp
void CNTService::ParseCommandLine(CNTServiceCommandLineInfo& rCmdInfo)
{
	for (int i = 1; i < __argc; i++)
	{
#ifdef _UNICODE
		LPCTSTR pszParam = __wargv[i];
#else
		LPCTSTR pszParam = __argv[i];
#endif
		BOOL bFlag = FALSE;
		BOOL bLast = ((i + 1) == __argc);
		if (pszParam[0] == _T('-') || pszParam[0] == _T('/'))
		{
			// remove flag specifier
			bFlag = TRUE;
			++pszParam;
		}
		rCmdInfo.ParseParam(pszParam, bFlag, bLast);
	}
}

// Based upon the function of the same name in CWinApp
BOOL CNTService::ProcessShellCommand(CNTServiceCommandLineInfo& rCmdInfo)
{
	BOOL bResult = TRUE;
	switch (rCmdInfo.m_nShellCommand)
	{
		case CNTServiceCommandLineInfo::RunAsService:
			bResult = Run();
			break;

		case CNTServiceCommandLineInfo::InstallService:
		  bResult = Install();
			break;

		case CNTServiceCommandLineInfo::UninstallService:
		  bResult = Uninstall();
			break;

		case CNTServiceCommandLineInfo::DebugService:
		  Debug();
			bResult = TRUE;
			break;

		case CNTServiceCommandLineInfo::ShowServiceHelp:
		  ShowHelp();
			bResult = TRUE;
			break;
	}
	return bResult;
}








/////////////// CNTServiceCommandLineInfo implementation //////////////////////
CNTServiceCommandLineInfo::CNTServiceCommandLineInfo()
{
	m_nShellCommand = RunAsService;
}

CNTServiceCommandLineInfo::~CNTServiceCommandLineInfo()
{
}

void CNTServiceCommandLineInfo::ParseParam(LPCTSTR pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag)
	{
		if (lstrcmpi(pszParam, _T("install")) == 0)
			m_nShellCommand = InstallService;
		else if ( (lstrcmpi(pszParam, _T("remove")) == 0) ||
              (lstrcmpi(pszParam, _T("uninstall")) == 0) )
			m_nShellCommand = UninstallService;
		else if (lstrcmpi(pszParam, _T("debug")) == 0)
		  m_nShellCommand = DebugService;
		else if ( (lstrcmpi(pszParam, _T("help")) == 0) ||
						  (lstrcmpi(pszParam, _T("?")) == 0) )
		  m_nShellCommand = ShowServiceHelp;
	}
	else
	{
		// Currently don't support parsing anything from
		// the command line except flags
	}

	if (bLast)
	{
		// Again the we don't support anything for the
		// last parameter
	}
}






///////////////////////// CNTScmService implementation ////////////////////////
CNTScmService::CNTScmService()
{
	m_hService = NULL;
}

CNTScmService::~CNTScmService()
{
	Close();
}

void CNTScmService::Close()
{
	if (m_hService)
	{
		CloseServiceHandle(m_hService);
		m_hService = NULL;
	}
}

CNTScmService::operator SC_HANDLE() const
{
	return m_hService;
}

BOOL CNTScmService::Attach(SC_HANDLE hService)
{
	if (m_hService != hService)
		Close();

	m_hService = hService;
	return TRUE;
}

SC_HANDLE CNTScmService::Detach()
{
	SC_HANDLE hReturn = m_hService;
	m_hService = NULL;
	return hReturn;
}

BOOL CNTScmService::ChangeConfig( DWORD dwServiceType, DWORD dwStartType,
					              DWORD dwErrorControl, LPCTSTR lpBinaryPathName,
								  LPCTSTR lpLoadOrderGroup, LPDWORD lpdwTagId,
                                  LPCTSTR lpDependencies, LPCTSTR lpServiceStartName,
 								  LPCTSTR lpPassword, LPCTSTR lpDisplayName) const
{
	ASSERT(m_hService != NULL);
	BOOL bSuccess = ::ChangeServiceConfig( m_hService, dwServiceType, dwStartType,
 				        				 dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId,
										 lpDependencies, lpServiceStartName, lpPassword, lpDisplayName);
	if (!bSuccess)
		TRACE(_T("Failed in call to ChangeServiceConfig in ChangeConfig, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Control(DWORD dwControl)
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = ::ControlService(m_hService, dwControl, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Control, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Stop() const
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = ::ControlService(m_hService, SERVICE_CONTROL_STOP, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Stop, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Pause() const
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = ::ControlService(m_hService, SERVICE_CONTROL_PAUSE, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Pause, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Continue() const
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = ::ControlService(m_hService, SERVICE_CONTROL_CONTINUE, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Continue, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Interrogate() const
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = ::ControlService(m_hService, SERVICE_CONTROL_INTERROGATE, &ServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Interrogate, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Start(DWORD dwNumServiceArgs, LPCTSTR* lpServiceArgVectors) const
{
	ASSERT(m_hService != NULL);
	BOOL bSuccess = ::StartService(m_hService, dwNumServiceArgs, lpServiceArgVectors);
	if (!bSuccess)
		TRACE(_T("Failed in call to ControlService in Start, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::AcceptStop(BOOL& bStop)
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = QueryStatus(&ServiceStatus);
	if (bSuccess)
		bStop = ((ServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP) != 0);
	else
		TRACE(_T("Failed in call to QueryStatus in AcceptStop, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTScmService::AcceptPauseContinue(BOOL& bPauseContinue)
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = QueryStatus(&ServiceStatus);
	if (bSuccess)
		bPauseContinue = ((ServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) != 0);
	else
		TRACE(_T("Failed in call to QueryStatus in AcceptPauseContinue, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTScmService::AcceptShutdown(BOOL& bShutdown)
{
	ASSERT(m_hService != NULL);
	SERVICE_STATUS ServiceStatus;
	BOOL bSuccess = QueryStatus(&ServiceStatus);
	if (bSuccess)
		bShutdown = ((ServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) != 0);
	else
		TRACE(_T("Failed in call to QueryStatus in AcceptShutdown, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTScmService::QueryStatus(LPSERVICE_STATUS lpServiceStatus) const
{
	ASSERT(m_hService != NULL);
	BOOL bSuccess = ::QueryServiceStatus(m_hService, lpServiceStatus);
	if (!bSuccess)
		TRACE(_T("Failed in call to QueryServiceStatus in QueryStatus, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::QueryConfig(LPQUERY_SERVICE_CONFIG& lpServiceConfig) const
{
	ASSERT(m_hService != NULL);
	ASSERT(lpServiceConfig == NULL); // To prevent double overwrites, this function
								   // asserts if you do not send in a NULL pointer

	DWORD dwBytesNeeded;
	BOOL bSuccess = ::QueryServiceConfig(m_hService, NULL, 0, &dwBytesNeeded);
	if (!bSuccess && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		lpServiceConfig = (LPQUERY_SERVICE_CONFIG) new BYTE[dwBytesNeeded];
		DWORD dwSize;
		bSuccess = ::QueryServiceConfig(m_hService, lpServiceConfig, dwBytesNeeded, &dwSize);
	}

	if (!bSuccess)
		TRACE(_T("Failed in call to QueryServiceConfig in QueryConfig, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::Create(CNTServiceControlManager& Manager, LPCTSTR lpServiceName, LPCTSTR lpDisplayName,	  
			               DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl,	    
			               LPCTSTR lpBinaryPathName, LPCTSTR lpLoadOrderGroup, LPDWORD lpdwTagId,	        
			               LPCTSTR lpDependencies, LPCTSTR lpServiceStartName, LPCTSTR lpPassword)
{
	Close();
	m_hService = ::CreateService(Manager, lpServiceName, lpDisplayName, dwDesiredAccess, dwServiceType, 
							   dwStartType, dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, 
							   lpdwTagId, lpDependencies, lpServiceStartName, lpPassword);
	if (m_hService == NULL)
		TRACE(_T("Failed in call to CreateService in Create, GetLastError:%d\n"), ::GetLastError());

	return (m_hService != NULL);
}

BOOL CNTScmService::Delete() const
{
	ASSERT(m_hService != NULL);
	BOOL bSuccess = ::DeleteService(m_hService);
	if (!bSuccess)
		TRACE(_T("Failed in call to DeleteService in Delete, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::SetObjectSecurity(SECURITY_INFORMATION dwSecurityInformation,
                                      PSECURITY_DESCRIPTOR lpSecurityDescriptor) const

{
	ASSERT(m_hService != NULL);
	BOOL bSuccess = ::SetServiceObjectSecurity(m_hService, dwSecurityInformation, lpSecurityDescriptor);
	if (!bSuccess)
		TRACE(_T("Failed in call to SetServiceObjectSecurity in SetObjectSecurity, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTScmService::QueryObjectSecurity(SECURITY_INFORMATION dwSecurityInformation,
                                        PSECURITY_DESCRIPTOR& lpSecurityDescriptor) const
{
	ASSERT(m_hService != NULL);
	ASSERT(lpSecurityDescriptor == NULL); // To prevent double overwrites, this function
										// asserts if you do not send in a NULL pointer

	DWORD dwBytesNeeded;
	BOOL bSuccess = ::QueryServiceObjectSecurity(m_hService, dwSecurityInformation, NULL, 0, &dwBytesNeeded);
	if (!bSuccess && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		lpSecurityDescriptor = (PSECURITY_DESCRIPTOR) new BYTE[dwBytesNeeded];
		DWORD dwSize;
		bSuccess = ::QueryServiceObjectSecurity(m_hService, dwSecurityInformation, lpSecurityDescriptor, dwBytesNeeded, &dwSize);
	}

	if (!bSuccess)
		TRACE(_T("Failed in call to QueryServiceObjectSecurity in QueryObjectSecurity, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTScmService::EnumDependents(DWORD dwServiceState, DWORD dwUserData, ENUM_SERVICES_PROC lpEnumServicesFunc) const
{
	ASSERT(m_hService != NULL);

	DWORD dwBytesNeeded;
	DWORD dwServices;
	BOOL bSuccess = ::EnumDependentServices(m_hService, dwServiceState, NULL, 0, &dwBytesNeeded, &dwServices);
	if (!bSuccess && ::GetLastError() == ERROR_MORE_DATA)
	{
		BYTE* lpServices = new BYTE[dwBytesNeeded];
		DWORD dwSize;
		bSuccess = ::EnumDependentServices(m_hService, dwServiceState, (LPENUM_SERVICE_STATUS) lpServices, dwBytesNeeded, &dwSize, &dwServices);
		if (bSuccess)
		{
			BOOL bContinue = TRUE;
			for (DWORD i=0; i<dwServices; i++)
				bContinue = lpEnumServicesFunc(dwUserData, *(LPENUM_SERVICE_STATUS)(lpServices + i*sizeof(ENUM_SERVICE_STATUS)));
		}
		delete [] lpServices;
	}

	if (!bSuccess)
		TRACE(_T("Failed in call to EnumDependentServices in EnumDependents, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}






///////////////////////// CNTServiceControlManager implementation /////////////
CNTServiceControlManager::CNTServiceControlManager()
{
	m_hSCM = NULL;
	m_hLock = NULL;
}

CNTServiceControlManager::~CNTServiceControlManager()
{
	Unlock();
	Close();
}

CNTServiceControlManager::operator SC_HANDLE() const
{
	return m_hSCM;
}

BOOL CNTServiceControlManager::Attach(SC_HANDLE hSCM)
{
	if (m_hSCM != hSCM)
		Close();

	m_hSCM = hSCM;
	return TRUE;
}

SC_HANDLE CNTServiceControlManager::Detach()
{
	SC_HANDLE hReturn = m_hSCM;
	m_hSCM = NULL;
	return hReturn;
}

BOOL CNTServiceControlManager::Open(LPCTSTR pszMachineName, DWORD dwDesiredAccess)
{
	Close();
	m_hSCM = ::OpenSCManager(pszMachineName, SERVICES_ACTIVE_DATABASE, dwDesiredAccess);

	if (m_hSCM == NULL)
		TRACE(_T("Failed in call to OpenSCManager in Open, GetLastError:%d\n"), ::GetLastError());
	return (m_hSCM != NULL);
}

void CNTServiceControlManager::Close()
{
	if (m_hSCM)
	{
		::CloseServiceHandle(m_hSCM);
		m_hSCM = NULL;
	}
}

BOOL CNTServiceControlManager::QueryLockStatus(LPQUERY_SERVICE_LOCK_STATUS& lpLockStatus) const
{
	ASSERT(m_hSCM != NULL);
	ASSERT(lpLockStatus == NULL); // To prevent double overwrites, this function
								// asserts if you do not send in a NULL pointer

	DWORD dwBytesNeeded;
	BOOL bSuccess = ::QueryServiceLockStatus(m_hSCM, NULL, 0, &dwBytesNeeded);
	if (!bSuccess && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		lpLockStatus = (LPQUERY_SERVICE_LOCK_STATUS) new BYTE[dwBytesNeeded];
		DWORD dwSize;
		bSuccess = ::QueryServiceLockStatus(m_hSCM, lpLockStatus, dwBytesNeeded, &dwSize);
	}

	if (!bSuccess)
		TRACE(_T("Failed in call to QueryServiceLockStatus in QueryLockStatus, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTServiceControlManager::EnumServices(DWORD dwServiceType, DWORD dwServiceState, DWORD dwUserData, ENUM_SERVICES_PROC lpEnumServicesFunc) const
{
	ASSERT(m_hSCM != NULL);

	DWORD dwBytesNeeded;
	DWORD dwServices;
	DWORD dwResumeHandle = 0;
	BOOL bSuccess = ::EnumServicesStatus(m_hSCM, dwServiceType, dwServiceState, NULL, 0, &dwBytesNeeded, &dwServices, &dwResumeHandle);
	if (!bSuccess && ::GetLastError() == ERROR_MORE_DATA)
	{
		BYTE* lpServices = new BYTE[dwBytesNeeded];
		DWORD dwSize;
		bSuccess = ::EnumServicesStatus(m_hSCM, dwServiceType, dwServiceState, (LPENUM_SERVICE_STATUS) lpServices, dwBytesNeeded, &dwSize, &dwServices, &dwResumeHandle);
		if (bSuccess)
		{
			BOOL bContinue = TRUE;
			for (DWORD i=0; i<dwServices; i++)
				bContinue = lpEnumServicesFunc(dwUserData, *(LPENUM_SERVICE_STATUS)(lpServices + i*sizeof(ENUM_SERVICE_STATUS)));
		}
		delete [] lpServices;
	}

	if (!bSuccess)
		TRACE(_T("Failed in call to EnumServicesStatus in EnumServices, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTServiceControlManager::OpenService(LPCTSTR lpServiceName, DWORD dwDesiredAccess, CNTScmService& service) const
{
	ASSERT(m_hSCM != NULL);

	SC_HANDLE hService = ::OpenService(m_hSCM, lpServiceName, dwDesiredAccess);
	if (hService != NULL)
		service.Attach(hService);
	else
		TRACE(_T("Failed in call to OpenService in OpenService, GetLastError:%d\n"), ::GetLastError());

	return (hService != NULL);
}

BOOL CNTServiceControlManager::Lock()
{
	ASSERT(m_hSCM != NULL);

	m_hLock = LockServiceDatabase(m_hSCM);
	if (m_hLock == NULL)
		TRACE(_T("Failed in call to LockServiceDatabase in Lock, GetLastError:%d\n"), ::GetLastError());
	return (m_hLock != NULL);
}

BOOL CNTServiceControlManager::Unlock()
{
	BOOL bSuccess = TRUE;
	if (m_hLock)
	{
		bSuccess = ::UnlockServiceDatabase(m_hLock);
		if (!bSuccess)
			TRACE(_T("Failed in call to UnlockServiceDatabase in Unlock, GetLastError:%d\n"), ::GetLastError());
		m_hLock = NULL;
	}

	return bSuccess;
}






///////////////////////// CNTEventLog implementation //////////////////////////
CNTEventLog::CNTEventLog()
{
	m_hEventLog = NULL;
}

CNTEventLog::~CNTEventLog()
{
	Close();
}

CNTEventLog::operator HANDLE() const
{
	return m_hEventLog;
}

BOOL CNTEventLog::Attach(HANDLE hEventLog)
{
	if (m_hEventLog != hEventLog)
		Close();

	m_hEventLog = hEventLog;
	return TRUE;
}

HANDLE CNTEventLog::Detach()
{
	HANDLE hReturn = m_hEventLog;
	m_hEventLog = NULL;
	return hReturn;
}

BOOL CNTEventLog::Open(LPCTSTR lpUNCServerName, LPCTSTR lpSourceName)
{
	Close();
	m_hEventLog = ::OpenEventLog(lpUNCServerName, lpSourceName);
	if (m_hEventLog == NULL)
		TRACE(_T("Failed in call to OpenEventLog in Open, Server:%s, Source:%s, GetLastError:%d\n"), lpUNCServerName, lpSourceName, ::GetLastError());
	return (m_hEventLog != NULL);
}

BOOL CNTEventLog::OpenApplication(LPCTSTR lpUNCServerName)
{
	return Open(lpUNCServerName, _T("Application"));
}

BOOL CNTEventLog::OpenSystem(LPCTSTR lpUNCServerName)
{
	return Open(lpUNCServerName, _T("System"));
}

BOOL CNTEventLog::OpenSecurity(LPCTSTR lpUNCServerName)
{
	return Open(lpUNCServerName, _T("Security"));
}

BOOL CNTEventLog::OpenBackup(LPCTSTR lpUNCServerName, LPCTSTR lpFileName)
{
	Close();
	m_hEventLog = ::OpenBackupEventLog(lpUNCServerName, lpFileName);
	if (m_hEventLog == NULL)
		TRACE(_T("Failed in call to OpenBackupEventLog, Server:%s, Filename:%s, GetLastError:%d\n"), lpUNCServerName, lpFileName, ::GetLastError());
	return (m_hEventLog != NULL);
}

BOOL CNTEventLog::Close()
{
	BOOL bSuccess = TRUE;
	if (m_hEventLog != NULL)
	{
		bSuccess = ::CloseEventLog(m_hEventLog);
		if (!bSuccess)
			TRACE(_T("Failed in call to CloseEventLog in Close, GetLastError:%d\n"), ::GetLastError());
		m_hEventLog = NULL;
	}

	return bSuccess;
}

BOOL CNTEventLog::Backup(LPCTSTR lpBackupFileName) const
{
	ASSERT(m_hEventLog != NULL);
	BOOL bSuccess = ::BackupEventLog(m_hEventLog, lpBackupFileName);
	if (!bSuccess)
		TRACE(_T("Failed in call to BackupEventLog in Backup, Filename:%s, GetLastError:%d\n"), lpBackupFileName, ::GetLastError());
	return bSuccess;
}

BOOL CNTEventLog::Clear(LPCTSTR lpBackupFileName) const
{
	ASSERT(m_hEventLog != NULL);
	BOOL bSuccess = ::ClearEventLog(m_hEventLog, lpBackupFileName);
	if (!bSuccess)
		TRACE(_T("Failed in call to ClearEventLog in Clear, Filename:%s, GetLastError:%d\n"), lpBackupFileName, ::GetLastError());
	return bSuccess;
}

BOOL CNTEventLog::GetNumberOfRecords(DWORD& dwNumberOfRecords) const
{
	ASSERT(m_hEventLog != NULL);
	BOOL bSuccess = ::GetNumberOfEventLogRecords(m_hEventLog, &dwNumberOfRecords);
	if (!bSuccess)
		TRACE(_T("Failed in call to GetNumberOfEventLogRecords in GetNumberOfRecords, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTEventLog::GetOldestRecord(DWORD& dwOldestRecord) const
{
	ASSERT(m_hEventLog != NULL);
	BOOL bSuccess = ::GetOldestEventLogRecord(m_hEventLog, &dwOldestRecord);
	if (!bSuccess)
		TRACE(_T("Failed in call to GetOldestEventLogRecord in GetOldestRecord, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTEventLog::NotifyChange(HANDLE hEvent) const
{
	ASSERT(m_hEventLog != NULL);
	BOOL bSuccess = ::NotifyChangeEventLog(m_hEventLog, hEvent);
	if (!bSuccess)
		TRACE(_T("Failed in call to NotifyChangeEventLog in NotifyChange, GetLastError:%d\n"), ::GetLastError());
	return bSuccess;
}

BOOL CNTEventLog::ReadNext(CEventLogRecord& record, LPCTSTR lpstrLogType) const
{
	ASSERT(m_hEventLog != NULL);

	DWORD dwBytesRead;
	DWORD dwBytesNeeded;
	EVENTLOGRECORD el;
	BOOL bSuccess = ::ReadEventLog(m_hEventLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, &el, sizeof(EVENTLOGRECORD), &dwBytesRead, &dwBytesNeeded);
	if (bSuccess)
		record = CEventLogRecord(&el, lpstrLogType);
	else if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		// buffer was too small allocate a new one and call again
		BYTE* lpBuffer = new BYTE[dwBytesNeeded];
		bSuccess = ::ReadEventLog(m_hEventLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, lpBuffer, dwBytesNeeded, &dwBytesRead, &dwBytesNeeded);
		if (bSuccess)
			record = CEventLogRecord((EVENTLOGRECORD*) lpBuffer, lpstrLogType);
		delete [] lpBuffer;
	}
	else
		TRACE(_T("Failed in call to ReadEventLog in ReadNext, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTEventLog::ReadPrev(CEventLogRecord& record, LPCTSTR lpstrLogType) const
{
	ASSERT(m_hEventLog != NULL);

	DWORD dwBytesRead;
	DWORD dwBytesNeeded;
	EVENTLOGRECORD el;
	BOOL bSuccess = ::ReadEventLog(m_hEventLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 0, &el, sizeof(EVENTLOGRECORD), &dwBytesRead, &dwBytesNeeded);
	if (bSuccess)
		record = CEventLogRecord(&el, lpstrLogType);
	else if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		// buffer was too small allocate a new one and call again
		BYTE* lpBuffer = new BYTE[dwBytesNeeded];
		bSuccess = ::ReadEventLog(m_hEventLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 0, lpBuffer, dwBytesNeeded, &dwBytesRead, &dwBytesNeeded);
		if (bSuccess)
			record = CEventLogRecord((EVENTLOGRECORD*) lpBuffer, lpstrLogType);
		delete [] lpBuffer;
	}
	else
		TRACE(_T("Failed in call to ReadEventLog in ReadPrev, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}






///////////////////////// CEventLogRecord implementation //////////////////////
CEventLogRecord::CEventLogRecord()
{
	m_dwRecordNumber = 0;
	m_TimeGenerated = CTime(0);
	m_TimeWritten = CTime(0);
	m_dwEventID = 0;
	m_wEventType = 0;
	m_wEventCategory = 0;
}

CEventLogRecord::~CEventLogRecord()
{
}

CEventLogRecord::CEventLogRecord(const CEventLogRecord& record)
{
	*this = record;
}

CEventLogRecord::CEventLogRecord(const EVENTLOGRECORD* pRecord, LPCTSTR lpstrLogType)
{
	ASSERT(pRecord);

	// First the easy ones
	m_dwRecordNumber = pRecord->RecordNumber;
	m_TimeGenerated = pRecord->TimeGenerated;
	m_TimeWritten = pRecord->TimeWritten;
	m_dwEventID = pRecord->EventID;
	m_wEventType = pRecord->EventType;
	m_wEventCategory = pRecord->EventCategory;

	// Map the User SID to the User Name
	if (!GetEventUserName( pRecord, m_sUserID))
		m_sUserID = _T( "N/A");
	// Copy over the Binary data
	DWORD i = 0;
	BYTE* pBeginRecord = (BYTE*) pRecord;
	DWORD dwCurOffset = pRecord->DataOffset;
	while (i < pRecord->DataLength)
	{
		m_Data.Add(pBeginRecord[dwCurOffset]);
		dwCurOffset++;
		i++;
	}

	// Copy over the SourceName
	TCHAR* pszBeginRecord = (TCHAR*) pRecord;
	dwCurOffset = sizeof(EVENTLOGRECORD);
	while (pszBeginRecord[dwCurOffset])
	{
		m_sSourceName += TCHAR(pBeginRecord[dwCurOffset]) ;
		dwCurOffset++;
	}

	// Skip over the NULL 
	while (pszBeginRecord[dwCurOffset] == 0 )
		dwCurOffset++;

	// Copy over the ComputerName
	while (pszBeginRecord[dwCurOffset])
	{
		m_sComputerName += TCHAR(pBeginRecord[dwCurOffset]);
		dwCurOffset++;
	}

	// Copy over the strings array
	int nStringsRead = 0;
	dwCurOffset = pRecord->StringOffset;
	while (nStringsRead < pRecord->NumStrings)
	{
		// Find the next string
		CString sText;
		while (pszBeginRecord[dwCurOffset])
		{
			sText += TCHAR(pBeginRecord[dwCurOffset]);
			dwCurOffset++;
		}
		// Add it to the array
		m_Strings.Add(sText);

		// Increment the number of strings read
		nStringsRead++;

		// Skip over the NULL (if more strings to parse)
		if (nStringsRead < pRecord->NumStrings)
		{
			while (pszBeginRecord[dwCurOffset] == 0)
				dwCurOffset++;
		}
	}
	m_sLogType = lpstrLogType;
	GetEventMessage( pRecord);
}

CEventLogRecord& CEventLogRecord::operator=(const CEventLogRecord& record)
{
	m_dwRecordNumber = record.m_dwRecordNumber;
	m_TimeGenerated = record.m_TimeGenerated;
	m_TimeWritten = record.m_TimeWritten;
	m_dwEventID = record.m_dwEventID;
	m_wEventType = record.m_wEventType;
	m_wEventCategory = record.m_wEventCategory;
	m_sUserID = record.m_sUserID;
	m_Strings.Copy(record.m_Strings);
	m_Data.Copy(record.m_Data);
	m_sSourceName = record.m_sSourceName;
	m_sComputerName = record.m_sComputerName;
	m_sMessage = record.m_sMessage;
	m_sLogType = record.m_sLogType;

	return *this;
}

BOOL CEventLogRecord::GetEventUserName(const EVENTLOGRECORD *pelr, CString &csUser)
{
    PSID lpSid;
    TCHAR szName[256];
    TCHAR szDomain[256];
    SID_NAME_USE snu;
    DWORD cbName = 256;
    DWORD cbDomain = 256;

    // Point to the SID. 
    lpSid = (PSID)((LPBYTE) pelr + pelr->UserSidOffset); 

    if (LookupAccountSid(NULL, lpSid, szName, &cbName, szDomain,
         &cbDomain, &snu))
    {
        // Set the user and domain
		csUser.Format( _T( "%s\\%s"), szDomain, szName);
    }
    else
    {
        // Use the error status from LookupAccountSid.
        return FALSE;
	}

    SetLastError(0);
    return TRUE;
}

BOOL CEventLogRecord::GetEventMessage(const EVENTLOGRECORD *pRecord)
{
	LPCTSTR szStringArray[100];
	TCHAR	szBuffer[2048],
			szDll[2048];
	ULONG	uBufferSize = 2048;
	LONG	lLength   = 0;
	DWORD	dwRegType = 0;
	HKEY	hKey;
	HMODULE	hLib;
	int		i;

	// Check paramaters
	if (pRecord == NULL)
	{
		m_sMessage = _T( "N/A (Empty record)");
		return FALSE;
	}

	for (i=0; (i<100) && (i<m_Strings.GetSize()); i++)
		szStringArray[i] = LPCTSTR( m_Strings.GetAt( i));

	// Build registry path to obtain which Dll to load for retreiving message
	StringCchPrintfA( szBuffer,2048, _T( "SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s"), m_sLogType,
					 m_sSourceName);

	// Load message text
	if (RegOpenKey( HKEY_LOCAL_MACHINE, szBuffer, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx( hKey, "EventMessageFile", 0, &dwRegType, 
				(unsigned char*)szBuffer, &uBufferSize) == ERROR_SUCCESS)
		{
			if (ExpandEnvironmentStrings( szBuffer, szDll, 2048) > 0)
			{
				if ((hLib = LoadLibraryEx( szDll, NULL, DONT_RESOLVE_DLL_REFERENCES)) != NULL)
				{
					LPVOID pMsg  = NULL;
					
					lLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
						 					FORMAT_MESSAGE_FROM_HMODULE |
											FORMAT_MESSAGE_ARGUMENT_ARRAY, hLib, m_dwEventID, 
											0, (LPTSTR) &pMsg, 128, (LPTSTR *) szStringArray);
	
					if (lLength > 0)
					{
						m_sMessage = (LPCTSTR) pMsg;
						LocalFree( (HANDLE) pMsg);
					}
					else
						m_sMessage.Format( _T( "N/A (GetLastError %lu)"), GetLastError());
					FreeLibrary( hLib);
				}
				else
					m_sMessage.Format( _T( "N/A (GetLastError %lu)"), GetLastError());
			}
			else
				m_sMessage.Format( _T( "N/A (GetLastError %lu)"), GetLastError());
		}
		else
			m_sMessage.Format( _T( "N/A (GetLastError %lu)"), GetLastError());
		RegCloseKey( hKey);
	}
	else
		m_sMessage.Format( _T( "N/A (GetLastError %lu)"), GetLastError());
	return TRUE;
}




///////////////////////// CNTEventLogSource implementation ////////////////////
CNTEventLogSource::CNTEventLogSource()
{
	m_hEventSource = NULL;
}

CNTEventLogSource::~CNTEventLogSource()
{
	Deregister();
}

CNTEventLogSource::operator HANDLE() const
{
	return m_hEventSource;
}

BOOL CNTEventLogSource::Attach(HANDLE hEventSource)
{
	if (m_hEventSource != hEventSource)
		Deregister();

	m_hEventSource = hEventSource;
	return TRUE;
}

HANDLE CNTEventLogSource::Detach()
{
	HANDLE hReturn = m_hEventSource;
	m_hEventSource = NULL;
	return hReturn;
}

BOOL CNTEventLogSource::Register(LPCTSTR lpUNCServerName, LPCTSTR lpSourceName)
{
	Deregister();
	m_hEventSource = ::RegisterEventSource(lpUNCServerName, lpSourceName);
	if (m_hEventSource == NULL)
		TRACE(_T("Failed in call to RegisterEventSource in Register, GetLastError:%d\n"), ::GetLastError());
	return (m_hEventSource != NULL);
}

BOOL CNTEventLogSource::Report(WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid,
                               WORD wNumStrings, DWORD dwDataSize, LPCTSTR* lpStrings, LPVOID lpRawData) const
{
	ASSERT(m_hEventSource != NULL);

	// If we were send a NULL SID, then automatically create
	// one prior to calling the SDK ReportEvent. This really should 
	// have been done internally in ReportEvent.

	// First get the user name
	DWORD dwUserNameSize = UNLEN + 1;
	TCHAR szUserName[UNLEN + 1];
	::GetUserName(szUserName, &dwUserNameSize);

	DWORD dwDomainNameSize = UNLEN + 1;
	TCHAR szDomainName[UNLEN + 1];

	SID_NAME_USE accountType; 
	DWORD dwSidSize = 0;

	PSID lpSid = lpUserSid;
	BYTE* lpBuffer = NULL;
	if (::LookupAccountName(NULL, szUserName, NULL, &dwSidSize, szDomainName, 
						  &dwDomainNameSize, &accountType) == FALSE) 
	{
		lpBuffer = new BYTE[dwSidSize];
		if (::LookupAccountName(NULL, szUserName, lpBuffer, &dwSidSize, szDomainName, 
								&dwDomainNameSize, &accountType)) 
		lpSid = lpBuffer;
	}


	// Finally call the SDK version of the function
	BOOL bSuccess = ::ReportEvent(m_hEventSource, wType, wCategory, dwEventID, lpSid,
						wNumStrings, dwDataSize, lpStrings, lpRawData);
	if (!bSuccess)
		TRACE(_T("Failed in call to ReportEvent in Report, GetLastError:%d\n"), ::GetLastError());

	// Delete any memory we may have used
	if (lpBuffer)
		delete [] lpBuffer;

	return bSuccess;
}

BOOL CNTEventLogSource::Report(WORD wType, DWORD dwEventID, LPCTSTR lpszString) const
{
	ASSERT(lpszString);
	return Report(wType, 0, dwEventID, NULL, 1, 0, &lpszString, NULL);
}

BOOL CNTEventLogSource::Report(WORD wType, DWORD dwEventID, LPCTSTR lpszString1, LPCTSTR lpszString2) const
{
	ASSERT(lpszString1);
	ASSERT(lpszString2);
	LPCTSTR lpStrings[2];
	lpStrings[0] = lpszString1;
	lpStrings[1] = lpszString2;
	return Report(wType, 0, dwEventID, NULL, 2, 0, lpStrings, NULL);
}

BOOL CNTEventLogSource::Report(WORD wType, DWORD dwEventID, DWORD dwCode) const
{
	CString sError;
	sError.Format(_T("%d"), dwCode);
	return Report(wType, dwEventID, sError);
}

BOOL CNTEventLogSource::Deregister()
{
	BOOL bSuccess = TRUE;
	if (m_hEventSource != NULL)
	{
		bSuccess = ::DeregisterEventSource(m_hEventSource);
		if (!bSuccess)
			TRACE(_T("Failed in call to DeregisterEventSource in Deregister, GetLastError:%d\n"), ::GetLastError());
		m_hEventSource = NULL;
	}

	return bSuccess;
}

BOOL CNTEventLogSource::Install(LPCTSTR lpSourceName, LPCTSTR lpEventMessageFile, DWORD dwTypesSupported)
{
	// Validate our parameters
	ASSERT(lpSourceName);
	ASSERT(lpEventMessageFile);

	// Make the necessary updates to the registry
	BOOL bSuccess = FALSE;
	HKEY hAppKey;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"), 0, 
						 KEY_WRITE|KEY_READ, &hAppKey) == ERROR_SUCCESS)
	{
		DWORD dw;
		HKEY hSourceKey;
		if (RegCreateKeyEx(hAppKey, lpSourceName, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
							 &hSourceKey, &dw) == ERROR_SUCCESS)
		{
			// Write the Message file string
			bSuccess = (RegSetValueEx(hSourceKey, _T("EventMessageFile"), NULL, REG_SZ, (LPBYTE)lpEventMessageFile, 
									(lstrlen(lpEventMessageFile)+1)*sizeof(TCHAR)) == ERROR_SUCCESS);
			if (!bSuccess)
			TRACE(_T("Failed in call to RegSetValueEx in Install, GetLastError:%d\n"), ::GetLastError());


			// Write the Types supported dword
			bSuccess = bSuccess && (RegSetValueEx(hSourceKey, _T("TypesSupported"), NULL, REG_DWORD,
													(LPBYTE)&dwTypesSupported, sizeof(dwTypesSupported)) == ERROR_SUCCESS);
			if (!bSuccess)
				TRACE(_T("Failed in call to RegSetValueEx in Install, GetLastError:%d\n"), ::GetLastError());

			// Close the registry key we opened
			::RegCloseKey(hSourceKey);

			// Update the sources registry key so that the event viewer can filter 
			// on the events which we write to the event log
			CStringArray sources;
			if (GetStringArrayFromRegistry(hAppKey, _T("Sources"), sources))
			{
				// If our name is not in the array then add it
				BOOL bFoundMyself = FALSE;
				for (int i=0; i<sources.GetSize() && !bFoundMyself; i++)
				  bFoundMyself = (sources.GetAt(i) == lpSourceName);
				if (!bFoundMyself)
				{
					sources.Add(lpSourceName);
					SetStringArrayIntoRegistry(hAppKey, _T("Sources"), sources);
				}
			}
		}
		else
			TRACE(_T("Failed in call to RegCreateKeyEx in Install, GetLastError:%d\n"), ::GetLastError());

		// Close the registry key we opened
		::RegCloseKey(hAppKey);
	}
	else
		TRACE(_T("Failed in call to RegOpenKeyEx in Install, GetLastError:%d\n"), ::GetLastError());

	return bSuccess;
}

BOOL CNTEventLogSource::Uninstall(LPCTSTR lpSourceName)
{
	// Validate our parameters
	ASSERT(lpSourceName);

	// Remove the settings from the registry
	CString sSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"));
	sSubKey += lpSourceName;
	BOOL bSuccess = (RegDeleteKey(HKEY_LOCAL_MACHINE, sSubKey) == ERROR_SUCCESS);
	if (!bSuccess)
		TRACE(_T("Failed in call to RegDeleteKey in Uninstall, GetLastError:%d\n"), ::GetLastError());

	// Remove ourself from the "Sources" registry key
	HKEY hAppKey;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"), 0, 
						 KEY_WRITE|KEY_READ, &hAppKey) == ERROR_SUCCESS)
	{
		CStringArray sources;
		if (GetStringArrayFromRegistry(hAppKey, _T("Sources"), sources))
		{
			// If our name is in the array then remove it
			BOOL bFoundMyself = FALSE;
			for (int i=0; i<sources.GetSize() && !bFoundMyself; i++)
			{
				bFoundMyself = (sources.GetAt(i) == lpSourceName);
				if (bFoundMyself)
					sources.RemoveAt(i);
			}
			if (bFoundMyself)
				SetStringArrayIntoRegistry(hAppKey, _T("Sources"), sources);
		}

		// Close the registry key we opened
		::RegCloseKey(hAppKey);
	}

	return bSuccess;
}

BOOL CNTEventLogSource::GetStringArrayFromRegistry(HKEY hKey, const CString& sEntry, CStringArray& array)
{
	// Validate our input parameters
	ASSERT(hKey);
	ASSERT(sEntry);

	// First find out the size of the key
	DWORD dwLongestValueDataLength;
	DWORD dwError = ::RegQueryInfoKey(hKey, NULL, NULL, (LPDWORD) NULL, NULL,
									NULL, NULL, NULL, NULL, &dwLongestValueDataLength,
									NULL, NULL);
	if (dwError != ERROR_SUCCESS)
		return FALSE;


	// Allocate some memory to retrieve the data back into
	BYTE* lpBuffer = new BYTE[dwLongestValueDataLength];

	DWORD dwDataType;
	DWORD dwDataSize = dwLongestValueDataLength;
	dwError = ::RegQueryValueEx(hKey, (LPTSTR) (LPCTSTR) sEntry,
							  NULL, &dwDataType, lpBuffer, &dwDataSize);


	if ((dwError != ERROR_SUCCESS) || (dwDataType != REG_MULTI_SZ))
	{
		delete [] lpBuffer;
		return FALSE;
	}

	LPTSTR lpszStrings = (LPTSTR) lpBuffer;
	array.RemoveAll();
	while (lpszStrings[0] != 0)
	{
		array.Add((LPCTSTR) lpszStrings);
		lpszStrings += (_tcslen(lpszStrings ) + 1);
	}

	delete [] lpBuffer;

	return TRUE;
}

BOOL CNTEventLogSource::SetStringArrayIntoRegistry(HKEY hKey, const CString& sEntry, const CStringArray& array)
{   
	// Validate our input parameters
	ASSERT(hKey);
	ASSERT(sEntry);
	int i;

	// Work out the size of the buffer we will need
	DWORD dwSize = 0;
	int nStrings = array.GetSize();
	for (i=0; i<nStrings; i++)
		dwSize += array.GetAt(i).GetLength() + 1; // 1 extra for each NULL terminator

	// Need one second NULL for the double NULL at the end
	dwSize++;

	// Allocate the memory we want
	BYTE* lpBuffer = new BYTE[dwSize];
	::ZeroMemory(lpBuffer, dwSize);

	// Now copy the strings into the buffer
	int nCurOffset = 0;
	LPTSTR lpszString = (LPTSTR) lpBuffer;
	for (i=0; i<nStrings; i++)
	{
		CString sText = array.GetAt(i);
		_tcscpy_s(&lpszString[nCurOffset],dwSize, sText);
		nCurOffset += sText.GetLength();
		nCurOffset++;
	}

	// Finally write it into the registry
	BOOL bSuccess = (::RegSetValueEx(hKey, sEntry, NULL, REG_MULTI_SZ, lpBuffer, dwSize) == ERROR_SUCCESS);

	// free up the memory we used
	delete [] lpBuffer;

	return bSuccess;
}


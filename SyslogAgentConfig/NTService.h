/*
Module : SERV.H
Purpose: Defines the interface for a number of MFC classes which 
         encapsulate the whole area of services in NT. 
Created: PJN / 14-07-1998
History: None

Copyright (c) 1998 by PJ Naughter.  
All rights reserved.
				 


Classes provided include:

CNTService: An C++ encapsulation of an actual running service.
To develop your own service you should derive a class from it and
override the necessary functions to implement your own specific 
behaviour of your service.

CNTScmService: A C++ wrapper for the SC_HANDLE type which represents
a service as returned from the Service Control Manager APIs. You would
use the class in conjuction with the SCM class to install, reconfigure
and uninstall your services.

CNTServiceControlManager: A C++ wrapper for the SC_HANDLE type which
represents the Service Control Manager itself.

CEventLogRecord: A simple C++ class which adds some helpful functions
to the underlying EVENTLOGRECORD structure.

CNTEventLog: A C++ wrapper for the HANDLE for the Event Log APIs. 
This class corresponds almost exactly to the code which internally the 
"Event Viewer" application in NT would use. You could think of this 
as the client side to the NT Event Log API.

CNTEventLogSource: A C++ wrapper for the "Server" side of the Event Log
API's. You would normally use this class in the code when installing
,uninstalling your service or when reporting to the event log while
your service is running.

*/



#ifndef __SERV_H__
#define __SERV_H__




//  The CNTServiceCommandLineInfo class aids in parsing the 
//  command line at application startup of an NT Service. The 
//  structure is styled upon the MFC class CCommandLineInfo
class CNTServiceCommandLineInfo
{
public:
//   Constructors / Destructors
	CNTServiceCommandLineInfo();
	~CNTServiceCommandLineInfo();

//  Methods
	virtual void ParseParam(LPCTSTR pszParam, BOOL bFlag, BOOL bLast);

//  Data
	enum 
	{ 
		RunAsService,
	  InstallService, 
		UninstallService,
		DebugService, 
		ShowServiceHelp 
	} m_nShellCommand;
};






//  An encapsulation of the APIs used to register, unregister,
//  write, install and uninstall Event log entries i.e. the
//  server side to the Event log APIs
class CNTEventLogSource
{
public:
//  Constructors / Destructors
	CNTEventLogSource();
	~CNTEventLogSource();

//  Methods
	operator HANDLE() const;
	BOOL Attach(HANDLE hEventSource);
	HANDLE Detach();
	BOOL Register(LPCTSTR lpUNCServerName, //   server name for source 
                LPCTSTR lpSourceName 	   //   source name for registered handle  
                );
	BOOL Report(WORD wType,	        // event type to log 
				WORD wCategory,	    //   event category 
				DWORD dwEventID,	//   event identifier 
				PSID lpUserSid,	    //   user security identifier (optional) 
				WORD wNumStrings,	//   number of strings to merge with message  
				DWORD dwDataSize,	//   size of binary data, in bytes
				LPCTSTR* lpStrings,	//   array of strings to merge with message 
				LPVOID lpRawData 	//  address of binary data 
 			  ) const;
	BOOL Report(WORD wType, DWORD dwEventID, LPCTSTR lpszString) const;
	BOOL Report(WORD wType, DWORD dwEventID, LPCTSTR lpszString1, LPCTSTR lpszString2) const;
	BOOL Report(WORD wType, DWORD dwEventID, DWORD dwCode) const;
	BOOL Deregister();

	static BOOL Install(LPCTSTR lpSourceName, LPCTSTR lpEventMessageFile, DWORD dwTypesSupported);
  static BOOL Uninstall(LPCTSTR lpSourceName);
  
protected:
  static BOOL GetStringArrayFromRegistry(HKEY hKey, const CString& sEntry, CStringArray& array);
  static BOOL SetStringArrayIntoRegistry(HKEY hKey, const CString& sEntry, const CStringArray& array);

  HANDLE m_hEventSource;
  friend class CNTService;
};






// An MFC framework encapsulation of an NT service 
// You are meant to derive your own class from this and
// override its functions to implement your own 
// service specific functionality.
class CNTService
{
public:
// Constructors / Destructors
	CNTService(LPCTSTR szModulePathname,LPCTSTR lpszServiceName, LPCTSTR lpszDisplayName, DWORD dwControlsAccepted, LPCTSTR lpszDescription = NULL); 
	~CNTService();

// Accessors / Mutators
	CString GetServiceName() const { return m_sServiceName; };
	CString GetDisplayName() const { return m_sDisplayName; };
	CString GetDescription() const { return m_sDescription; };

// Persistance support
	// Allows saving and restoring of a services settings to the 
	// "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ServiceName\Parameters"
	// location in the registry
	static BOOL    WriteProfileString( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue);
	static BOOL    WriteProfileInt( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
	static BOOL    WriteProfileStringArray( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, const CStringArray& array);
	static BOOL    WriteProfileBinary( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes);

	static CString GetProfileString( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL);
	static UINT    GetProfileInt( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
	static BOOL    GetProfileStringArray( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArray& array); 
	static BOOL    GetProfileBinary( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes);

	BOOL    WriteProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue) { return CNTService::WriteProfileString( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, lpszValue); };
	BOOL    WriteProfileInt( LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue) { return CNTService::WriteProfileInt( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, nValue); };
	BOOL    WriteProfileStringArray( LPCTSTR lpszSection, LPCTSTR lpszEntry, const CStringArray& array) { return CNTService::WriteProfileStringArray( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, array); };
	BOOL    WriteProfileBinary( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes) { return CNTService::WriteProfileBinary( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, pData, nBytes); };

	CString GetProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL) { return CNTService::GetProfileString( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, lpszDefault); };
	UINT    GetProfileInt( LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault) { return CNTService::GetProfileInt( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, nDefault); };
	BOOL    GetProfileStringArray( LPCTSTR lpszSection, LPCTSTR lpszEntry, CStringArray& array) { return CNTService::GetProfileStringArray( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, array); }; 
	BOOL    GetProfileBinary( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes) { return CNTService::GetProfileBinary( HKEY_LOCAL_MACHINE, m_sServiceName, lpszSection, lpszEntry, ppData, pBytes); };



// Other Methods
  // Helpful functions to parse the command line and execute the results
	void ParseCommandLine(CNTServiceCommandLineInfo& rCmdInfo);
	BOOL ProcessShellCommand(CNTServiceCommandLineInfo& rCmdInfo);

	// Reports the status of this service back to the SCM
	BOOL ReportStatusToSCM();
	BOOL ReportStatusToSCM(DWORD dwCurrentState, DWORD dwWin32ExitCode, 
	                       DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint, DWORD dwWaitHint);

	// Installs the callback funtion by calling RegisterServiceCtrlHandler
	BOOL RegisterCtrlHandler();

	// Member function which does the job of responding to SCM requests
	virtual void WINAPI ServiceCtrlHandler(DWORD dwControl);						

	// The ServiceMain function for this service
	virtual void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);

	// Called in reponse to a shutdown request
	virtual void OnStop();

	// Called in reponse to a pause request
	virtual void OnPause();

	// Called in reponse to a continue request
	virtual void OnContinue();

	// Called in reponse to a Interrogate request
	virtual void OnInterrogate();

	// Called in reponse to a Shutdown request
	virtual void OnShutdown();

	// Called in reponse to a user defined request
	virtual void OnUserDefinedRequest(DWORD dwControl);

	// Kicks off the Service. You would normally call this
	// some where in your main/wmain or InitInstance
	// a standard process rather than as a service. If you are
	// using the CNTServiceCommandLineInfo class, then internally
	// it will call this function for you.
	virtual BOOL Run();

	// Installs the service
	virtual BOOL Install();

	// Uninstalls the service
	virtual BOOL Uninstall();

	// Runs the service as a normal function as opposed
	// to a service
	virtual void Debug();

	// Displays help for this service
	virtual void ShowHelp();

protected:
// Methods
	// These two static functions are used internally to
	// go from the SDK functions to the C++ member functions
	static void WINAPI _ServiceCtrlHandler(DWORD dwControl);						
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);

	// Used internally by the persistance functions
	static HKEY GetSectionKey( HKEY hRemoteKey, LPCTSTR sServiceName, LPCTSTR lpszSection);
	static HKEY GetServiceRegistryKey( HKEY hRemoteKey, LPCTSTR sServiceName);

// Data
	SERVICE_STATUS_HANDLE m_hStatus;
	DWORD                 m_dwControlsAccepted;   //  What Control requests will this service repond to
	DWORD                 m_dwCurrentState;       //  Current Status of the service
	CString               m_sServiceName;         //  Name of the service
	CString               m_sDisplayName;         //  Display name for the service
	CString               m_sDescription;         //  The description text for the service
	CString				  m_path; //erno added
	CNTEventLogSource     m_EventLogSource;       //  For reporting to the event log
	static CNTService*    sm_lpService;			  //  Static which contains the this pointer
	CCriticalSection      m_CritSect;             //  Protects changes to any member variables from multiple threads
};






// // //  Forward declaration
class CNTServiceControlManager;


typedef BOOL (CALLBACK* ENUM_SERVICES_PROC)(DWORD dwData, ENUM_SERVICE_STATUS& Service);


// An encapsulation of a service as returned from querying the SCM (i.e. an SC_HANDLE)
class CNTScmService
{
public:
// Constructors / Destructors
	CNTScmService();
	~CNTScmService();

// Methods
	// Releases the underlying SC_HANDLE
	void Close();

	// Allows access to the underlying SC_HANDLE representing the service
	operator SC_HANDLE() const;

	// Attach / Detach support from an SDK SC_HANDLE
	BOOL Attach(SC_HANDLE hService);
	SC_HANDLE Detach();

	// Changes the configuration of this service
	BOOL ChangeConfig(DWORD dwServiceType,	      //  type of service 
					  DWORD dwStartType,	      //  when to start service 
 					  DWORD dwErrorControl,	      //  severity if service fails to start 
 					  LPCTSTR lpBinaryPathName,	  //  pointer to service binary file name 
					  LPCTSTR lpLoadOrderGroup,	  //  pointer to load ordering group name 
					  LPDWORD lpdwTagId,	      //  pointer to variable to get tag identifier 
 					  LPCTSTR lpDependencies,	  //  pointer to array of dependency names 
 					  LPCTSTR lpServiceStartName, //  pointer to account name of service 
 					  LPCTSTR lpPassword,	      //  pointer to password for service account  
 					  LPCTSTR lpDisplayName 	  //  pointer to display name 
                    ) const;

	// Send a defined control code to the service
	BOOL Control(DWORD dwControl);

	// These functions call Control() with the 
	// standard predefined control codes
	BOOL Stop() const;		 // Ask the service to stop
	BOOL Pause() const;		 // Ask the service to pause
	BOOL Continue() const;	 // Ask the service to continue
	BOOL Interrogate() const;// Ask the service to update its status to the SCM

	// Start the execution of the service
	BOOL Start(DWORD dwNumServiceArgs,	      //  number of arguments 
			   LPCTSTR* lpServiceArgVectors 	//  address of array of argument string pointers  
		      ) const;	

	// Determines what Control codes this service supports
	BOOL AcceptStop(BOOL& bStop);                   // Ask the service can it stop
	BOOL AcceptPauseContinue(BOOL& bPauseContinue);	// Ask the service can it pause continue
	BOOL AcceptShutdown(BOOL& bShutdown);           // Ask the service if it is notified of shutdowns

	// Get the most return status of the service reported to the SCM by this service
	BOOL QueryStatus(LPSERVICE_STATUS lpServiceStatus) const;

	// Get the configuration parameters of this service from the SCM
	BOOL QueryConfig(LPQUERY_SERVICE_CONFIG& lpServiceConfig) const;

	// Add a new service to the SCM database
	BOOL Create(CNTServiceControlManager& Manager,		//  handle to service control manager database  
 						  LPCTSTR lpServiceName,	    //  pointer to name of service to start 
						  LPCTSTR lpDisplayName,	    //  pointer to display name 
						  DWORD dwDesiredAccess,	    //  type of access to service 
						  DWORD dwServiceType,	        //  type of service 
						  DWORD dwStartType,	        //  when to start service 
						  DWORD dwErrorControl,	        //  severity if service fails to start 
						  LPCTSTR lpBinaryPathName,	    //  pointer to name of binary file 
						  LPCTSTR lpLoadOrderGroup,	    //  pointer to name of load ordering group 
						  LPDWORD lpdwTagId,	        //  pointer to variable to get tag identifier 
						  LPCTSTR lpDependencies,	    //  pointer to array of dependency names 
						  LPCTSTR lpServiceStartName,	//  pointer to account name of service 
						  LPCTSTR lpPassword 	        //  pointer to password for service account 
              );

	// Mark this service as to be deleted from the SCM.
	BOOL Delete() const;

	// Enumerate the services that this service depends upon
	BOOL EnumDependents(DWORD dwServiceState,				  //  state of services to enumerate 
                        DWORD dwUserData,                     //  User defined data
 						ENUM_SERVICES_PROC lpEnumServicesFunc //  The callback function to use
                      ) const;

	// Get the security information associated with this service
	BOOL QueryObjectSecurity(SECURITY_INFORMATION dwSecurityInformation,  //  type of security information requested  
			                 PSECURITY_DESCRIPTOR& lpSecurityDescriptor	  //  address of security descriptor 
						   	) const;

	// Set the security descriptor associated with this service
	BOOL SetObjectSecurity(SECURITY_INFORMATION dwSecurityInformation,	//  type of security information requested  
			               PSECURITY_DESCRIPTOR lpSecurityDescriptor 	//  address of security descriptor 
		                  ) const;

protected:
	SC_HANDLE m_hService;
};






// An encapsulation of the NT Service Control Manager
class CNTServiceControlManager
{
public:
// Constructors / Destructors
	CNTServiceControlManager();
	~CNTServiceControlManager();

// Methods
	// Allows access to the underlying SC_HANDLE representing the SCM
	operator SC_HANDLE() const;

	// Attach / Detach support from an SDK SC_HANDLE
	BOOL Attach(SC_HANDLE hSCM);
	SC_HANDLE Detach();

	// Opens a connection to the SCM
	BOOL Open(LPCTSTR pszMachineName, DWORD dwDesiredAccess);
	
	// Close the connection to the SCM
	void Close();																							 

	// Get the SCM Status
	BOOL QueryLockStatus(LPQUERY_SERVICE_LOCK_STATUS& lpLockStatus) const; 

	// Enumerates the specified services
	BOOL EnumServices(DWORD dwServiceType, DWORD dwServiceState, DWORD dwUserData, ENUM_SERVICES_PROC lpEnumServicesFunc) const;

	// Opens the specified service
	BOOL OpenService(LPCTSTR lpServiceName,	DWORD dwDesiredAccess, CNTScmService& service) const;

	// Lock the SCM database
	BOOL Lock();

	// Unlocks the SCM database
	BOOL Unlock();

protected:
	SC_HANDLE m_hSCM;	 // Handle to the SCM
	SC_LOCK		m_hLock; // Handle of any lock on the Database
};





// A friendlier way of handling EVENTLOGRECORD structures.
class CEventLogRecord
{
public: 
// Constructors / Destructors
	CEventLogRecord();
	CEventLogRecord(const CEventLogRecord& record);
	CEventLogRecord(const EVENTLOGRECORD* pRecord, LPCTSTR lpstrLogType);
	~CEventLogRecord();

// Methods
	CEventLogRecord& operator=(const CEventLogRecord& record);

// Data
	DWORD        m_dwRecordNumber;
	CTime        m_TimeGenerated;
	CTime        m_TimeWritten;
	DWORD        m_dwEventID;
	WORD         m_wEventType;
	WORD         m_wEventCategory;
	CString		 m_sUserID;
	CStringArray m_Strings;
	CByteArray   m_Data;
	CString      m_sSourceName;
	CString      m_sComputerName;
	CString		 m_sLogType;
	CString		 m_sMessage;
protected:
	BOOL GetEventMessage( const EVENTLOGRECORD *pRecord);
	BOOL GetEventUserName(const EVENTLOGRECORD *pelr, CString &csUser);
};





// An encapsulation of the client side to the 
// NT event log APIs
#define	APPLICATION_EVENTLOG	_T( "Application")
#define	SECURITY_EVENTLOG		_T( "Security")
#define SYSTEM_EVENTLOG			_T( "System")

class CNTEventLog
{
public:
// Constructors / Destructors
	CNTEventLog();
	~CNTEventLog();

// Methods
	operator HANDLE() const;
	BOOL     Attach(HANDLE hEventLog);
	HANDLE   Detach();
	BOOL     Open(LPCTSTR lpUNCServerName, LPCTSTR lpSourceName);
	BOOL     OpenBackup(LPCTSTR lpUNCServerName, LPCTSTR lpFileName);
	BOOL     OpenApplication(LPCTSTR lpUNCServerName);
	BOOL     OpenSystem(LPCTSTR lpUNCServerName);
	BOOL     OpenSecurity(LPCTSTR lpUNCServerName);
	BOOL     Close();
	BOOL     Backup(LPCTSTR lpBackupFileName) const;
	BOOL     Clear(LPCTSTR lpBackupFileName) const;
	BOOL     GetNumberOfRecords(DWORD& dwNumberOfRecords) const;
	BOOL     GetOldestRecord(DWORD& dwOldestRecord) const;
	BOOL     NotifyChange(HANDLE hEvent) const;
	BOOL     ReadNext(CEventLogRecord& record, LPCTSTR lpstrLogType) const;
	BOOL     ReadPrev(CEventLogRecord& record, LPCTSTR lpstrLogType) const;

protected:
	HANDLE m_hEventLog;
};






#endif // __SERV_H__
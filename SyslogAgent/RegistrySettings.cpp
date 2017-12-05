// This file NOT extern C, even though its part of ntsyslog

#include "..\Syslogserver\common_stdafx.h"
#include "RegistrySettings.h"
#include "..\Syslogserver\common_registry.h"
#include "..\Syslogserver\common_registry_permissions.h"
#include "winsock2.h"
extern "C" {
	#include "service.h"
}



/*****************************************************************************
*initEventLogTypeEntry  -  Register the different event log types (security, DNS server...) and thereby activate SyslogAgents handling of them...
********************************************************************************/

void initEventLogTypeEntry(HKEY hKey,char *Name,int facility) {

	HKEY 		hKeySection;
	DWORD		dwSize, dwValue;

	// Create the appropriate subkey or open it if it exists
	if (RegCreateKeyEx( hKey, Name, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_WRITE|KEY_READ|WRITE_DAC, NULL, &hKeySection, &dwValue) != ERROR_SUCCESS)	{
		return;
	}

	// Write that we have to forward Information events
	dwValue = 1;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, INFORMATION_ENTRY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	// Write Information event priority
	dwValue = (facility*8)+SEVERITY_INFORMATION;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, INFORMATION_PRIORITY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}

	// Write that we have to forward Warning events
	dwValue = 1;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, WARNING_ENTRY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	// Write Warning event priority
	dwValue = (facility*8)+SEVERITY_WARNING;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, WARNING_PRIORITY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	
	// Write that we to forward Error events
	dwValue = 1;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, ERROR_ENTRY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS){
		return;
	}
	// Write Error event priority
	dwValue = (facility*8)+mySEVERITY_ERROR;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, ERROR_PRIORITY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}

	// Write that we have to forward Audit Success events
	dwValue = 1;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, AUDIT_SUCCESS_ENTRY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	// Write Audit Success event priority
	dwValue = (facility*8)+SEVERITY_INFORMATION;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, AUDIT_SUCCESS_PRIORITY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}

	// Write  we  have to forward Audit Failure events
	dwValue = 1;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, AUDIT_FAILURE_ENTRY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	// Write Audit Failure event priority
	dwValue = (facility*8)+SEVERITY_NOTICE;
	dwSize = sizeof (DWORD);
	if (RegSetValueEx( hKeySection, AUDIT_FAILURE_PRIORITY, 0, REG_DWORD, (LPBYTE ) &dwValue, dwSize) != ERROR_SUCCESS)	{
		return;
	}
	RegCloseKey( hKeySection);
}
/*****************************************************************************
*initRegistry
*
* Called upon installation of service *AND* upon start of service. Checks if entry exists before overwriting. 
* SyslogAddress can be NULL, indicating it is not an installation
********************************************************************************/
void __cdecl initRegistry(char *SyslogAddress) {
	//,char *csRegPath, int facility) {
	
	// Save the changes to the appropriate registry.
	HKEY		hKeyRemote,hKeySoftware,hReg,hRegTest;
	DWORD		dwSize,dwValue;
	bool		usePing=0;  //i.e. default
	bool		forwardEvents=1;
	bool		forwardApplications=0;
	int			port=514;
	int			status;
	int			EventLogPollInterval=2; //SyslogAgent default.
	CString		regPermSet;


	// Connect to the registry on HKLM
	if (RegConnectRegistry( (char*)((LPCTSTR)""), HKEY_LOCAL_MACHINE, &hKeyRemote) != ERROR_SUCCESS){
		DEBUGSERVICE(Message,"Failed to open HKEY_LOCAL_MACHINE in registry - settings not read");
		return;
	}
	// Create the SOFTWARE\Datagram key or open it if it exists
	if (RegCreateKeyEx( hKeyRemote, NTSYSLOG_SOFTWARE_KEY, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_WRITE|KEY_READ|WRITE_DAC, NULL, &hKeySoftware, &dwValue) != ERROR_SUCCESS){
		DEBUGSERVICE(Message,"Failed to open or create SyslogAgent key in registry - settings not read");
		RegCloseKey( hKeyRemote);
		return;
	}
	
	if (SyslogAddress!=NULL) {
		// Write the primary syslogd server
		dwSize = strlen(SyslogAddress);
		if (RegSetValueEx( hKeySoftware, PRIMARY_SYSLOGD_ENTRY, 0, REG_SZ, (LPBYTE ) (LPCTSTR)( SyslogAddress), dwSize) != ERROR_SUCCESS){
			DEBUGSERVICE(Message,"Failed to write SyslogIPAdress key to registry.");
			RegCloseKey (hKeySoftware);
			RegCloseKey( hKeyRemote);
			return;
		}
	}

	// Write the usePing bool
	dwValue=REG_BINARY;
	dwSize=sizeof(bool);
	if (RegQueryValueEx( hKeySoftware, USE_PING_ENTRY, 0,&dwValue , (LPBYTE ) &usePing, &dwSize) != ERROR_SUCCESS){
		dwSize = sizeof(bool);
		if (RegSetValueEx( hKeySoftware, USE_PING_ENTRY, 0, REG_BINARY, (LPBYTE ) &usePing, dwSize) != ERROR_SUCCESS){
			RegCloseKey (hKeySoftware);
			RegCloseKey( hKeyRemote);
			return;
		}
	}

	// Write port
	dwSize = sizeof (DWORD);
	dwValue=REG_DWORD;
	if (RegQueryValueEx( hKeySoftware, PORT_ENTRY, 0, &dwValue, (LPBYTE ) &port, &dwSize) != ERROR_SUCCESS)	{
		dwSize = sizeof (DWORD);
		if (RegSetValueEx( hKeySoftware, PORT_ENTRY, 0, REG_DWORD, (LPBYTE ) &port, dwSize) != ERROR_SUCCESS)	{
			return;
		}
	}

	// Write backup port
	dwSize = sizeof (DWORD);
	dwValue=REG_DWORD;
	if (RegQueryValueEx( hKeySoftware, PORT_BACKUP_ENTRY, 0, &dwValue, (LPBYTE ) &port, &dwSize) != ERROR_SUCCESS)	{
		dwSize = sizeof (DWORD);
		if (RegSetValueEx( hKeySoftware, PORT_BACKUP_ENTRY, 0, REG_DWORD, (LPBYTE ) &port, dwSize) != ERROR_SUCCESS)	{
			return;
		}
	}

	// Write the forwardEvents bool
	dwSize = sizeof(bool);
	dwValue=REG_BINARY;
	if (RegQueryValueEx( hKeySoftware, FORWARDEVENTLOGS, 0, &dwValue, (LPBYTE ) &forwardEvents, &dwSize) != ERROR_SUCCESS){
		dwSize = sizeof(bool);
		if (RegSetValueEx( hKeySoftware, FORWARDEVENTLOGS, 0, REG_BINARY, (LPBYTE ) &forwardEvents, dwSize) != ERROR_SUCCESS){
			RegCloseKey (hKeySoftware);
			RegCloseKey( hKeyRemote);
			return;
		}
	}
	// Write the forwardApplication bool
	dwSize = sizeof(bool);
	dwValue=REG_BINARY;
	if (RegQueryValueEx( hKeySoftware, FORWARDAPPLICATIONLOGS, 0, &dwValue, (LPBYTE ) &forwardApplications, &dwSize) != ERROR_SUCCESS){
		dwSize = sizeof(bool);
		if (RegSetValueEx( hKeySoftware, FORWARDAPPLICATIONLOGS, 0, REG_BINARY, (LPBYTE ) &forwardApplications, dwSize) != ERROR_SUCCESS){
			RegCloseKey (hKeySoftware);
			RegCloseKey( hKeyRemote);
			return;
		}
	}

	// Write EventLogPollInterval
	dwSize = sizeof (DWORD);
	dwValue=REG_DWORD;
	if (RegQueryValueEx( hKeySoftware, EVENTLOG_POLL_INTERVAL, 0, &dwValue, (LPBYTE ) &EventLogPollInterval, &dwSize) != ERROR_SUCCESS)	{
		dwSize = sizeof (DWORD);
		if (RegSetValueEx( hKeySoftware, EVENTLOG_POLL_INTERVAL, 0, REG_DWORD, (LPBYTE ) &EventLogPollInterval, dwSize) != ERROR_SUCCESS)	{
			return;
		}
	}

	DEBUGSERVICE(Message,"Registry init phase 1 done.");

	if (RegOpenKeyEx(hKeySoftware,APPLICATION_SECTION,0,KEY_WRITE|KEY_READ|WRITE_DAC,&hRegTest)!= ERROR_SUCCESS) {
		initEventLogTypeEntry(hKeySoftware,APPLICATION_SECTION, FACILITY_LOCAL7);
		regPermSet.Format("%s\\%s",NTSYSLOG_SOFTWARE_KEY,APPLICATION_SECTION);
		AddPermissions(regPermSet);
	} else
		RegCloseKey(hRegTest);

	if (RegOpenKeyEx(hKeySoftware,SECURITY_SECTION,0,KEY_WRITE|KEY_READ|WRITE_DAC,&hRegTest)!= ERROR_SUCCESS) {
		initEventLogTypeEntry(hKeySoftware,SECURITY_SECTION, FACILITY_SECURITYAUTH);
		regPermSet.Format("HKEY_LOCAL_MACHINE\\%s\\%s",NTSYSLOG_SOFTWARE_KEY,SECURITY_SECTION);
		AddPermissions(regPermSet);
	} else
		RegCloseKey(hRegTest);
	
	if (RegOpenKeyEx(hKeySoftware,SYSTEM_SECTION,0,KEY_WRITE|KEY_READ|WRITE_DAC,&hRegTest)!= ERROR_SUCCESS) {
		initEventLogTypeEntry(hKeySoftware,SYSTEM_SECTION, FACILITY_SYSTEM);
		regPermSet.Format("HKEY_LOCAL_MACHINE\\%s\\%s",NTSYSLOG_SOFTWARE_KEY,SYSTEM_SECTION);
		AddPermissions(regPermSet);
	} else 
		RegCloseKey(hRegTest);

	DEBUGSERVICE(Message,"Registry init phase 2 done.");

	//Create and fix settings for applicationlogs
	if (RegCreateKeyEx( hKeySoftware, APPLICATIONLOGS, 0, REG_NONE, REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ|WRITE_DAC, NULL, &hRegTest, &dwValue) == ERROR_SUCCESS){
		regPermSet.Format("HKEY_LOCAL_MACHINE\\%s\\%s",NTSYSLOG_SOFTWARE_KEY,APPLICATIONLOGS);
		AddPermissions(regPermSet);
	} else
		RegCloseKey(hRegTest);

	DEBUGSERVICE(Message,"Registry init phase 3 done.");

	status=AddPermissions("SYSTEM\\CurrentControlSet\\Services\\EventLog");
	if (status) {
		DEBUGSERVICE(Message,"Failed to write permissions to registry, with error %d",status);
	}

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\EventLog", 0, KEY_READ, &hReg);
   	if (status == ERROR_SUCCESS){
		int 	i = 0;
		char buffer[256];

		while (RegEnumKey(hReg, i++, buffer, sizeof(buffer)) == ERROR_SUCCESS)	{
			if ((_stricmp(buffer,"System")==0)|(_stricmp(buffer,"Application")==0)|(_stricmp(buffer,"Security")==0))
				continue; //three main already initialized
	
			regPermSet.Format("SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s",buffer);
			AddPermissions(regPermSet);

			if (RegOpenKeyEx(hKeySoftware,buffer,0,KEY_READ,&hRegTest)!= ERROR_SUCCESS) {
				initEventLogTypeEntry(hKeySoftware,buffer, FACILITY_SYSTEM);
			} else
				RegCloseKey(hRegTest);
		}
		RegCloseKey(hReg);
	}

	RegCloseKey( hKeySoftware);
	RegCloseKey( hKeyRemote);
	}
/*****************************************************************************
*
********************************************************************************/
void __cdecl ReadSettings(int *_syslogaPort, int *_syslogBackupPort, bool *forwardEvents,int *EventLogPollInterval) {
	HKEY		hKeyRemote,hKeySoftware;
	DWORD		dwSize,dwValue,dwType;

	bool tempForwardEvents=true;

	// Connect to the registry on HKLM
	if (RegConnectRegistry( (char*)((LPCTSTR)""), HKEY_LOCAL_MACHINE, &hKeyRemote) != ERROR_SUCCESS){
		DEBUGSERVICE(Message,"Failed to open HKEY_LOCAL_MACHINE in registry - settings not read");
		return;
	}
	// Create the SOFTWARE\Datagram\SyslogAgent key or open it if it exists
	if (RegCreateKeyEx( hKeyRemote, NTSYSLOG_SOFTWARE_KEY, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_WRITE|KEY_READ, NULL, &hKeySoftware, &dwValue) != ERROR_SUCCESS){
		RegCloseKey( hKeyRemote);
		DEBUGSERVICE(Message,"Failed to open SyslogAgent key in registry - settings not read");
		return;
	}

	// Read the forwardEvents info
	dwSize =sizeof( bool);
	if (RegQueryValueEx( hKeySoftware, FORWARDEVENTLOGS, 0, &dwType, (LPBYTE) &tempForwardEvents, &dwSize) != ERROR_SUCCESS)	{
		RegCloseKey (hKeySoftware);
		RegCloseKey( hKeyRemote);
		return;
	}
	*forwardEvents=tempForwardEvents;

	//port info
	dwSize =sizeof( DWORD);
	if ((RegQueryValueEx( hKeySoftware, PORT_ENTRY, 0, &dwType, (LPBYTE) &dwValue, &dwSize) != ERROR_SUCCESS)) {
		RegCloseKey (hKeySoftware);
		RegCloseKey( hKeyRemote);
		return;
	}
	*_syslogaPort=dwValue;

	//backup port info
	dwSize =sizeof( DWORD);
	if ((RegQueryValueEx( hKeySoftware, PORT_BACKUP_ENTRY, 0, &dwType, (LPBYTE) &dwValue, &dwSize) != ERROR_SUCCESS)) {
		RegCloseKey (hKeySoftware);
		RegCloseKey( hKeyRemote);
		return;
	}
	*_syslogBackupPort=dwValue;

	//Event Log Poll Interval, in seconds
	dwSize =sizeof( DWORD);
	if ((RegQueryValueEx( hKeySoftware, EVENTLOG_POLL_INTERVAL, 0, &dwType, (LPBYTE) &dwValue, &dwSize) != ERROR_SUCCESS)) {
		RegCloseKey (hKeySoftware);
		RegCloseKey( hKeyRemote);
		return;
	}
	*EventLogPollInterval=dwValue;
	
	RegCloseKey( hKeySoftware);
	RegCloseKey( hKeyRemote);

	}

/****************************************************************************
* initFilterEventArray - read registry event ids to filter out

****************************************************************************/
void initFilterEventArray( DWORD *A) {
	CString filterArray;
	char theList[256];
	char *Ptr=&(theList[0]);
	DWORD anEventId;
	int status,counter=0;
	
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	ReadRegKey(&filterArray,"",EVENTIDFILTERARRAY);
	CloseRegistry();

	strcpy_s(theList,filterArray.GetBuffer());

	do {
		status=sscanf_s(Ptr,"%d",&anEventId);
		if (status==-1) {
			*A=-1;
			break;
		}
		if (status==0) {
			*A=-1;
			logger(Error,"EventIDFilterList in registry contained invalid characters. Only numbers, comma and spaces are allowed. Complete filter list possibly not read.");
			break;
		}
		*A=anEventId;
		A++;
		counter++;

		Ptr=strstr(Ptr,",");
		while((Ptr!=NULL)&&((*Ptr==' ')||(*Ptr==','))) //move past ',' and space
			Ptr++;

	} while ((Ptr!=NULL)&(counter<MAXEVENTIDFILTERNUMBER));
	
}

/*****************************************************************************
*
*
********************************************************************************/
int __cdecl SyslogHostInRegistryOK() {
	HKEY		hKeyRemote,hKeySoftware;
	DWORD		dwSize,dwValue,dwType;
	TCHAR		szBuffer[256];
	int			status;
//	char		ownIP[32];

	// Connect to the registry on HKLM
	if (RegConnectRegistry( (char*)((LPCTSTR)""), HKEY_LOCAL_MACHINE, &hKeyRemote) != ERROR_SUCCESS){
		DEBUGSERVICE(Message,"Failed to open HKEY_LOCAL_MACHINE in registry - settings not read");
		return 0;
	}
	// Create the SOFTWARE\Datagram key or open it if it exists
	if (RegCreateKeyEx( hKeyRemote, NTSYSLOG_SOFTWARE_KEY, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_WRITE|KEY_READ, NULL, &hKeySoftware, &dwValue) != ERROR_SUCCESS)
	{
		DEBUGSERVICE(Message,"Failed to open SyslogAgent key in registry - settings not read");
		RegCloseKey( hKeyRemote);
		return 0;
	}

	// Read the primary syslogd server
	dwSize = 255*sizeof( TCHAR);
	memset( szBuffer, 0, 255+sizeof( TCHAR));
	status=RegQueryValueEx( hKeySoftware, PRIMARY_SYSLOGD_ENTRY, 0, &dwType, (LPBYTE) szBuffer, &dwSize);
	
	if ((status!= ERROR_SUCCESS)||(strcmp(szBuffer,"")==0)||(strcmp(szBuffer,"0.0.0.0")==0))	{
	RegCloseKey (hKeySoftware);
	RegCloseKey( hKeyRemote);
        return 0;
	}
	RegCloseKey (hKeySoftware);
	RegCloseKey( hKeyRemote);
	return 1;
}



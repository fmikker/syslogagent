#include "..\Syslogserver\common_stdafx.h" 
#include <winsock2.h> 
#include "..\Syslogserver\common_SyslogProject.h"
#include "..\Syslogserver\common_registry.h"
#include "winerror.h"
#include "assert.h"
//-----LeakWatcher--------------------
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//------------------------------------
HKEY hKeySoftware=NULL,hKey=NULL,hKeyTemp=NULL;
int enumerator = 0;

/*******************************************************
*	WriteRegKey - int
********************************************************/
int WriteRegKey(UINT *value, CString Name){

	if (hKey==NULL) {
		return 0;
	}

	if (RegSetValueEx( hKey, Name, 0, REG_DWORD, (LPBYTE ) value, sizeof( DWORD)) != ERROR_SUCCESS) {
		return 0;
	}
	return 1;
}
/*******************************************************
*	WriteRegKey - bool
********************************************************/
int  WriteRegKey(bool *value, CString Name){
	if (hKey==NULL) {
		return 0;
	}
	if (RegSetValueEx( hKey, Name, 0, REG_BINARY, (LPBYTE ) value, sizeof(bool)) != ERROR_SUCCESS) {
		return 0;
	}
	return 1;
}
/*******************************************************
*	WriteRegKey - string
********************************************************/
int WriteRegKey(CString *value, CString Name){
	if (hKey==NULL) {
		return 0;
	}
	if (RegSetValueEx( hKey, Name, 0, REG_SZ, (LPBYTE) ((*value).GetString()), (*value).GetLength()) != ERROR_SUCCESS) {
		return 0;
	}
	return 1;
}
/*******************************************************
*	ReadRegKey - int
********************************************************/
void ReadRegKey(UINT *value, UINT DefaultValue, CString Name){
	DWORD dwSize,dwType,dwValue=REG_DWORD;
	dwSize = sizeof( DWORD);

	if (hKey==NULL) {
		return;
	}

	if ((RegQueryValueEx( hKey, Name, 0, &dwType, (LPBYTE) &dwValue, &dwSize) == ERROR_SUCCESS)) {
		*value = dwValue;
	} else {  //key not found
		*value=DefaultValue;
	}
}
/*******************************************************
*	ReadRegKey - bool
********************************************************/
void ReadRegKey(bool *value, bool DefaultValue, CString Name){
	DWORD dwSize,dwType=REG_BINARY;
	bool dwValue;
	dwSize = sizeof( bool);
	if (hKey==NULL) {
		return;
	}

	if ((RegQueryValueEx( hKey, Name, 0, &dwType, (LPBYTE) &dwValue, &dwSize) == ERROR_SUCCESS)) {
		*value = dwValue;
	} else {  //key not found
		*value=DefaultValue;
	}
}
/*******************************************************
*	ReadRegKey - string
********************************************************/
void ReadRegKey(CString *value, CString DefaultValue, CString Name){
	DWORD dwSize,dwType=REG_SZ;
	char temp[256]="";
	dwSize = sizeof( temp);
	if (hKey==NULL) {
		return;
	}

	if ((RegQueryValueEx( hKey, Name, 0, &dwType, (LPBYTE) &(temp[0]), &dwSize) == ERROR_SUCCESS)) {
		*value = temp;
	} else {  //key not found
		*value=DefaultValue;
	}
}


/*******************************************************
*	DeleteKey - delete entire key
********************************************************/
void DeleteKey(CString name) {
	RegDeleteKey(hKey,name);
}

/*******************************************************
*	GetNextKey - cycle through existing keys
********************************************************/
int GetNextKey(CString *keyName) {
	char buffer[256];

	if (RegEnumKey(hKey, enumerator++, buffer, sizeof(buffer)) == ERROR_SUCCESS) {
		*keyName=buffer;
		return 1;
	}
	*keyName="";
	return 0;
}

/*******************************************************
*	GoToRegKey - create if not existant
********************************************************/
int GoToRegKey(CString name){
	long i;
	DWORD dwValue;
	CString m_csComputer,csKeyName;
	assert(hKey!=NULL);

	// Open the sub key
	csKeyName.Format( _T( "%s\\"),name);
	i=RegCreateKeyEx( hKey, csKeyName, 0, REG_NONE, REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ|WRITE_DAC, NULL, &hKeyTemp, &dwValue);
	if (i != ERROR_SUCCESS) {
		logger(Error,"Failed to go to new sub registry key.");
		hKeyTemp=NULL;
		return -1;
	}

	//make the new sub key the default
	RegCloseKey(hKey);
	hKey=hKeyTemp;
	hKeyTemp=NULL;
	return 0;

}
/********************************************************
SystemDSNtype - reply type of db engine
0 - means not found or can not open
1 - not fullt identified -> use standard SQL
2 - Mysql
3 - Access
4 - Postgres
********************************************************/
int SystemDSNtype() {
	CString m_csComputer,csKeyName,csRegPath,dbName,dbDriver;
	int i,type;
	HKEY hKeySoftware,hKeySyslogODBC;
	DWORD dwSize,dwType=REG_SZ;
	char temp[256]="";
	dwSize = sizeof( temp);


	m_csComputer.Empty();
	if (RegConnectRegistry( (char*)((LPCTSTR)m_csComputer), HKEY_LOCAL_MACHINE, &hKeySoftware) != ERROR_SUCCESS) {
		logger(Error,"Error while connecting to the registry!");
		RegCloseKey(hKeySoftware);
		return 0;
	}

	// Open the SYSLOG ODBC
	csKeyName="SOFTWARE\\ODBC\\ODBC.INI\\Syslog\\";
	i=RegOpenKeyEx( hKeySoftware, csKeyName, 0, KEY_READ, &hKeySyslogODBC);
	if (i != ERROR_SUCCESS) {
		RegCloseKey(hKeySyslogODBC);
		RegCloseKey(hKeySoftware);
		return 0; //error
	}

	//Read driver details
	if ((RegQueryValueEx( hKeySyslogODBC, "Driver", 0, &dwType, (LPBYTE) &(temp[0]), &dwSize) == ERROR_SUCCESS)) {
		dbDriver=temp;
	} else {  //key not found
		dbDriver="void";
	}
	if ((RegQueryValueEx( hKeySyslogODBC, "Database", 0, &dwType, (LPBYTE) &(temp[0]), &dwSize) == ERROR_SUCCESS)) {
		dbName=temp;
	} else {  //key not found
		dbName="void";
	}

	if ((dbName.CompareNoCase("Syslog")!=0)&(strstr(dbDriver,"odbcjt")==NULL)) { //not access, no db specified
		//Wrong in registry. We do not have permission to write...
		//		logger("Error in configuration. The ODBC connection Syslog does not have database 'Syslog' specified. Syslogserver not allowed to correct error.",Error);
		//		return 0;
	}

	if (strstr(dbDriver,"myodbc")) {
		type=2;
	} else if (strstr(dbDriver,"odbcjt")) {  //Access
		type=3;
	} else if (strstr(dbDriver,"psqlodbc")) {  //Postgres
		type=4;
	} else {
		type=1;
	}

	RegCloseKey(hKeySyslogODBC);
	RegCloseKey(hKeySoftware);
	return type;  //ok
}
/********************************************************
ReadPathFromRegistry
********************************************************/
void ReadPathFromRegistry(char *connStr,CString ownPath){
	DWORD		dwSize;
	CString m_csComputer,csKeyName,DefaultPath;
	// Forward logpath info
	dwSize = 256;
	CString DaPath;
	char apa[256];
	int pos=0;
	char *connStrpos;

	//backup
	strcpy_s(connStr,256,"driver={Microsoft Access Driver (*.mdb)};Dbq=c:\\syslog.mdb;UID=;PWD=");

	if (!OpenRegistry((CString)SYSLOG_SYSLOG_KEY)) {
		return;	//failed
	}

	ReadRegKey(&DaPath,ownPath,LOGPATH);
	strcpy_s(apa,DaPath);
	strcpy_s(connStr,256,"driver={Microsoft Access Driver (*.mdb)};Dbq=");
	connStrpos=&connStr[0]+strlen(connStr);
	while(apa[pos]!='\0') {
		*connStrpos++=apa[pos++];
		if (apa[pos-1]=='\\') *connStrpos++=apa[pos-1];
	}
	*connStrpos++=apa[pos];
	strcat_s(connStr,256,"\\\\syslog.mdb;UID=;PWD=");
	CloseRegistry();

}

/********************************************************
OpenRegistry
********************************************************/
int OpenRegistry(CString key) {
	CString csKeyName;
	DWORD dwValue;
	int i;

	enumerator=0;

	if (RegConnectRegistry( "", HKEY_LOCAL_MACHINE, &hKeySoftware) != ERROR_SUCCESS) {
		logger(Error,"Error while connecting to the registry (HKEY_LOCAL_MACHINE)!");
		RegCloseKey(hKeySoftware);
		return 0;
	}

	// Open the appropriate Syslog key
	csKeyName.Format( _T( "%s\\%s\\"), SYSLOG_SOFTWARE_KEY, key);
	i=RegCreateKeyEx( hKeySoftware, csKeyName, 0, REG_NONE, REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ, NULL, &hKey, &dwValue);

	if (i != ERROR_SUCCESS) {
		logger(Error,"Failed to open registry key %s.",csKeyName);
		RegCloseKey(hKeySoftware);
		hKey=NULL;
		hKeySoftware=NULL;
		return 0;
		}
	return 1;
}
/********************************************************
CloseRegistry
********************************************************/
void CloseRegistry() {
	if (hKey!=NULL) {
		RegCloseKey(hKey);
	}
	if (hKeySoftware!=NULL) {
		RegCloseKey(hKeySoftware);
	}
	hKey=NULL;
	hKeySoftware=NULL;
	hKeyTemp=NULL;

}

/********************************************************
writeServiceDescription
********************************************************/
void writeServiceDescription() {
	HKEY hKeyRemote=NULL,hKey=NULL;
	long DaCode;
	if (RegConnectRegistry( "", HKEY_LOCAL_MACHINE, &hKeyRemote) == ERROR_SUCCESS) {
		//Open the key to where Windows stores service info
		if (RegOpenKeyEx( hKeyRemote, SERVICE_REG_PATH, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
			DaCode=RegSetValueEx( hKey, SERVICESTRING, 0, REG_SZ, (LPBYTE)"SyslogServer by Datagram receives and stores syslog entries sent via the network.", (DWORD)81);
		RegCloseKey(hKey);

	}
	RegCloseKey(hKeyRemote);
}

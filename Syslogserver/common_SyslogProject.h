
#define parseSyslogProjectheaderonce 1

#include "..\Syslogserver\common_registry.h"
#define dbname   "Syslog"
#define dbtable  "syslog"
#define dbtable_backup  "syslog_backup"
#define dbtable_analyze  "syslog_analyze"
#define dbtableAlarmDefinitions  "alarmdefinitions"
#define dbtableTriggered_Alarms "Triggered_Alarms"
#define Service_Name "Syslogserver"


#define SYSLOG_SERVICE_NAME	_T("Syslogserver")
#define SYSLOG_EXE_NAME	_T("Syslogserver.exe")
#define SYSLOG_DISPLAY_NAME	_T("Syslogserver")


#define SyslogreceiverProcessName "Syslogreceiver.exe"
#define SyslogWatchdogProcessName "Syslogserver.exe"

#define idleTime 250L   //Milliseconds. Idle time after having successfully read from file
						//Affects both NamedPipe-watchdog and numberOfIdleTimes calculation is changed
						//Inspect both when changing value!
#define TIMEOUTINMILLISECONDS 900;

#define VERSIONSTRING "Version 2.2.2"  //First word must be a a-zA-Z word if license interpretation is to work (common_license.cpp)

#define MaxLogEntriesInAlarm 30  //Max number of entires in alarm emails 

#define DefaultBatchInsertNbr 100  //Max number of entires in alarm emails 

 // SQL variable lengths in AlarmDefinitions
#define Definition_name_length 128
#define SQL_query_length 4096
#define Border_value_length 16
#define Email_addresses_length 512
#define MAXNAME 2048  //length of sql row in alarmdefinitions

#define SyslogreceiverPipeName "SyslogreceiverPipe"

#define AccessCurrentlyLocked -1102

#define MAXEVENTIDFILTERNUMBER 30 //also defined in engine.c - the c code cannot import this file!

//Erno 06, introduced strcpy_s et al
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1

#define true 1
#define false 0
#define informerror -1
#define KillAll 1

#define DISPLAY 1
#define NODISPLAY 0

//Function return values 
#define OK 0
#define NOK 1  
#define NOK_NoDbSyslogExist 911  //same as sql error message
#define NOK_CantConnectToSyslog 4060

//Debug indentation
#define EndHeader 1
#define Header 2
#define Title 3
#define Message 4


//Optimize schedule
#define AwaitDoneAlarmThread 5
#define AlarmThreadInProgress 4
#define AlarmAndBackupThreadDone 3
#define OptimizeInProgress 2
#define OptimizePerformed 1
#define RegularMode 0



typedef struct {
	int dbId; //alarm id, not syslog id - no need for more than 2^32 alarm definitions.
	unsigned char Definition_name[Definition_name_length];
	char SQL_query[SQL_query_length];
	char Border_value[Border_value_length];
	int Larm_interval;
	unsigned char Next_run_time[32];
	char Email_addresses[Email_addresses_length];
	} AlarmDef;

typedef struct {
	HANDLE ThreadId;  //NULL = no thread running
	bool TimeToDie;
	bool AlarmTimeWarningReported;
	bool UpdateDate;
	} ThreadStatus;

typedef struct {
	HANDLE ThreadId;
	bool running;
	bool backlogMode;
	int watchdog;
	bool terminate;
	bool performSwitch;
	bool optimize;
} StatusType;
	
typedef struct {
__int64 dbId;
int facility;
int severity;
char syslog_time[32];
char syslog_host[16];
char header_time[20];
char header_host[255];
char msg_tag[32];
char msg_content[1024];
} SyslogDef;


typedef struct {
	int line;
	char filename[255];
} throwData;

typedef struct {
	bool Debug_Alarm;
	int DebugAlarmIndentation;
	bool Debug_Backup; //same as optimize
	int DebugBackupIndentation;
	bool Debug_Service;
	int DebugServiceIndentation;
	bool Debug_IO;
	int DebugIOIndentation;
} DebugFlagsDef;


typedef struct {
	unsigned int maxFileSize;
	unsigned int DaysToKeepInDb;
	unsigned int DaysToDeleteDb;
	unsigned int DaysToDeleteBackupDb;
	unsigned int AlarmLoopTimer;
	unsigned int DNSPurgeIntervalInMinutes;
	unsigned int AdminReportingLevel;
	bool deleteFileWhenDone; //=false
	bool PerformBackup;//=false
	bool PerformDeleteDb;//=false
	bool PerformDeleteBackupDb;//=false;
	bool lookUpNames;
	CString cataloguename;
	CString Sender;
	CString Administrator;
	CString Gateway;
	char GatewayIP[16];
	CString MyPort;
	CString MyPortTCP;
	CString FileLocation; //härleds från cataloguename
	CString FileLocation_with_quotes; //härleds från cataloguename
	CString dbUser;
	CString dbPass;
	CString regDate;
} MyMemstruct;

extern MyMemstruct reg;


//Common to all propjects - instead of a common_misc.h file

//void logger(char *text, int severity);
//void logger(CString, int severity);

void logger(int severity,char *text,...);
void logToLogFile(char *text,int severity);
void logToSyslog(char *text,int severity);
void misc_init();
void misc_port_init(int Port,int PortTCP);
void* createDatabaseInstance(CString dbUser, CString dbPass);
void cleanStr(unsigned char *dest,unsigned char *orig);  //used to clear licenseString from manipulation, and more
extern CString LogFilePathName;
extern CString ownIP;
extern CString hostname;
extern CString ownPath;


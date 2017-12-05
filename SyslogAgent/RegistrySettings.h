//extern "C" by erno aug04
extern "C" {

#if !defined(ReadRegistrySettingsDone)
#define ReadRegistrySettingsDone


// define EVENTLOG_REG_PATH		 "System\\CurrentControlSet\\Services\\EventLog"

#define MAXEVENTIDFILTERNUMBER 30


void __cdecl initRegistry(char * SyslogAddress);
int SyslogHostInRegistryOK();
void ReadSettings(int *port, int *backupport, bool *forwardEvents,int *EventLogPollInterval);

#endif

} //end extern "C" by erno aug04


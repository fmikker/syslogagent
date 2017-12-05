#include "..\Syslogserver\common_stdafx.h"
#include "appWatch.h" 

#include "ipexport.h"
#include "icmpapi.h"
#include "winsock2.h"
#include "RegistrySettings.h"
#include "afxtempl.h"
#include "output.h"
#include <queue>
#include "errorHandling.h"


extern "C" {
#include "service.h"
#include "ntsl.h"
	volatile bool ReadMoreFromRegistry;  //Tell poll engine to go ahead - last transmit ok.
	extern int eventlog_write_lastrun(DWORD *lastrun);
}

using namespace std ;

//Erno 06, introduced strcpy_s et al
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1

queue<aMess> messQueue;
int ErrorCount;
CTime lastTCPsendTime; //Set when a message is sent via tcp to the server. When 1 minute of inactivity has passed, the connection is closed.
HANDLE SyslogQueueMutex;

extern char applHostName[128];
extern char applHostIP[16];
volatile int LastTransmitOK; //could not link with bool and external references. Why?
extern DWORD thisrun;

/********************************************************
********************************************************/
void loggerPrivate(char *text,int severity) {
	int FacSev=24+severity;
	CString	texten;
	char tempDate[64];
	_threadData	sockContainer;

	//send to queue
	aMess data;

	CTime DaTime= CTime::GetCurrentTime();
	strcpy_s(&(tempDate[0]),sizeof(tempDate),DaTime.Format("%b %d %H:%M:%S"));
	if (tempDate[4] == '0') // Unix style formatting
		tempDate[4] = ' ';

	texten.Format("<%d>%s ",FacSev,tempDate);
	texten+=applHostName;
	texten+=" SyslogAgent: ";
	texten+=text;

	strncpy_s(data.text,sizeof(data.text),texten,MAXBUFLEN);

	LoggerInsertIntoOutputQueue(&data);

}

/****************************************************************************
* logger -- 
*
****************************************************************************/
void logger(int severity,char *text,...) {
	va_list args;
	va_start(args,text);
	char outText[1024];

#ifdef _DEBUG
	ErrorCount=0; //Disable reduction of errors when debugging
#endif
	if (ErrorCount>4) //Ignore
		return;

	if (ErrorCount==4){ //Write that we stop producing errors
		ErrorCount++;
		strcpy_s(outText,sizeof(outText),"Error reporting halted until network connection restored.");
		loggerPrivate((char*)outText,Notice);
		return;
	}

	ErrorCount++;
	_vsnprintf_s((char*)outText,1024,_TRUNCATE,text,args);
	DEBUGLOGGER(Message,outText);
	loggerPrivate((char*)outText,severity);
}

/*--------------------------[ ping_syslog_server ]--------------------------
*
*	Returns:
*		success		1
*		failure		0 
*----------------------------------------------------------------------------*/
bool ping_syslog_server(CString destHost) {
	HANDLE icmphandle ;
	unsigned long ip;
	char reply[sizeof(struct icmp_echo_reply)+8];
	struct icmp_echo_reply* iep=(struct icmp_echo_reply*)&reply[0];
	iep->RoundTripTime = 0xffffffff;

	ip =(DWORD)inet_addr(destHost);

	icmphandle = IcmpCreateFile();
	if (icmphandle !=INVALID_HANDLE_VALUE) {
		IcmpSendEcho(icmphandle,ip,0,0,NULL,reply,sizeof(struct icmp_echo_reply)+8,ippingtimeout);
		IcmpCloseHandle(icmphandle);
	} else {
		logger(Error, "Error. Failed to create icmp handle. Ping cannot be performed. Currently not sending messages. Will retry.");
		Sleep(5000);
		//error. Shuting down?
		return false;
	}

	switch( iep->Status )
	{
	case IP_SUCCESS:		//Error free       
		break;
	case IP_REQ_TIMED_OUT:  	//Timeout
	default:               		//Error
		//Error handling
		iep->RoundTripTime = 0xffffffff;
		break;
	}

	if (iep->RoundTripTime != 0xffffffff) {
		return true;
	} else 
		return false;
}

/********************************************************
GetNetworkAdresses
1 - ok
0 - not ok
********************************************************/

static bool GetNetworkAdresses(_threadData *threadData,CString host,CString backupHost) {

	char server_name[256],backup_server_name[256];

	strncpy_s(server_name,sizeof(server_name),host.GetBuffer(),255);
	host.ReleaseBuffer();
	strncpy_s(backup_server_name,sizeof(backup_server_name),backupHost.GetBuffer(),255);
	backupHost.ReleaseBuffer();


	memset(&(threadData->server),0, sizeof(threadData->server));
	memset(&(threadData->backupserver), 0, sizeof(threadData->backupserver));

	if (isalpha(server_name[0])) {	 
		struct hostent *hp = gethostbyname(server_name);
		if (hp != NULL)	
			memcpy(&(threadData->server.sin_addr),hp->h_addr,hp->h_length);
	}else  { 
		threadData->server.sin_addr.s_addr = inet_addr(server_name);
	}

	if (threadData->server.sin_addr.s_addr == 0) {
		logger(Error, "Primary syslog server IP not identified in fuction GetNetworkAdresses.");
		return false;	//must have	primary syslog server
	}

	if (backup_server_name[0]!='\0') {

		if (isalpha(backup_server_name[0]))	{	
			struct hostent *hp = gethostbyname(backup_server_name);
			if (hp != NULL)	
				memcpy(&(threadData->backupserver.sin_addr),hp->h_addr,hp->h_length);
		}else  { 
			threadData->backupserver.sin_addr.s_addr = inet_addr(backup_server_name);
		}

	} else {
		threadData->backupserver.sin_addr.s_addr=0;
	}
	return true;
}
/********************************************************
closeTCPconnection - one minut of inactivity has passed --> close connection

********************************************************/
void closeTCPconnection(_threadData *threadData) {

	if (threadData->sock!=0) {
		shutdown(threadData->sock,SD_BOTH);
		Sleep(250); //Allow server to respond to shutdown (although we don't really care, it will result in a cleaner termination of the connection.
		closesocket(threadData->sock);
	}

	if (threadData->backupsock!=0) {
		shutdown(threadData->backupsock,SD_BOTH);
		closesocket(threadData->backupsock);
	}

	threadData->sock=threadData->backupsock=0;
}
/********************************************************
initNetworkSettings

1 - ok
0 - not ok
********************************************************/
static bool initNetworkSettings(_threadData *threadData,int protocol,CString host,int _port,CString backupHost,int _backupport) {
	int port, backupport,status;
	unsigned long foo=1;
	TIMEVAL tv = {NetworkTimeout, 0};  //Timeout period
	TIMEVAL tv_zero = {0, 0};  //non-blocking
	FD_SET fdStruct;

#ifdef _DEBUG
	if (protocol==SOCK_STREAM)
		logger(Informational,"Initiating tcp conn to server.");
	else
		logger(Informational,"Initiating udp conn to server.");
#endif

	port = htons(_port);
	backupport = htons(_backupport);

	/* setup sockaddr_in structure */
	threadData->server.sin_family =	AF_INET;
	threadData->server.sin_port   =	port;

	/* open socket */
	threadData->sock = socket(AF_INET, protocol, 0);
	if (threadData->sock < 0) {
		logger(Error,"Failed to get socket.");
		WSACleanup();
		return false;
	}


	//Set async sockets for the TCP connection (otherwise 30 sec timneout when connecting...
	if (protocol==SOCK_STREAM) {
		ioctlsocket(threadData->sock,FIONBIO,&foo);
	}

	//Connect to syslog host
	connect(threadData->sock, (struct sockaddr*)&(threadData->server), sizeof(threadData->server));

	//Check progress
	FD_ZERO(&fdStruct);
	FD_SET(threadData->sock,&fdStruct);
	if (select(0,NULL,&fdStruct,&fdStruct,&tv)==1) { //Connection successful or error. Blocks for up to 3 secs
		if (select(0,NULL,NULL,&fdStruct,&tv_zero)==1) { //error
			status=WSAGetLastError();
			logger(Error,"Failed to connect to primary Syslog server. WSA error %d.",status);
			closeTCPconnection(threadData);
			WSACleanup();
			return false;
		}
	} else { //timeout, i.e. no host
		status=WSAGetLastError();
		logger(Error,"Timeout connecting to primary Syslog server. WSA error %d.",status);
		closeTCPconnection(threadData);
		WSACleanup();
		return false;
	}
	//We reached this spot = primary connection success.

	DEBUGLOGGER(Message,"Primary conn successful.");

	//Now same with the	backup connection, if so configured
	if (threadData->backupserver.sin_addr.s_addr==0) {	//no backup
		threadData->backupsock=0;
		return true;
	}

	if (service_halting())
		return false;  //do not try to send, since we're terminating...

	/* setup sockaddr_in structure */
	threadData->backupserver.sin_family = AF_INET;
	threadData->backupserver.sin_port = backupport;

	/* open socket */
	threadData->backupsock = socket(AF_INET, protocol, 0);
	if (threadData->backupsock < 0)	{
		logger(Error,"Failed to get backup socket.");
		threadData->backupsock=0;
		return true; //Still return true, since the primary socket was ok
	}

	//Set async sockets for the TCP connection (otherwise 30 sec timneout when connecting...
	if (protocol==SOCK_STREAM)
		ioctlsocket(threadData->backupsock,FIONBIO,&foo);

	/* connect to syslog host */
	connect(threadData->backupsock, (struct sockaddr*)&(threadData->backupserver), sizeof(threadData->backupserver));

	//Check progress
	FD_ZERO(&fdStruct);
	FD_SET(threadData->backupsock,&fdStruct);
	if (select(0,NULL,&fdStruct,&fdStruct,&tv)==1) { //Connection successful or error. Blocks for up to 3 secs
		if (select(0,NULL,NULL,&fdStruct,&tv_zero)==1) { //error
			status=WSAGetLastError();
			logger(Error,"Failed to connect to mirror Syslog server. WSA error %d.",status);
			if (threadData->backupsock!=0)
				closesocket(threadData->backupsock);
			threadData->backupsock=0;
			return true; //Primary sock still ok...
		}
	} else { //timeout, i.e. no host
		status=WSAGetLastError();
		logger(Error,"Timeout connecting to mirror Syslog server. WSA error %d.",status);
		if (threadData->backupsock!=0)
			closesocket(threadData->backupsock);
		threadData->backupsock=0;
		return true;
	}


	return true;
}
/**********************************************************************
TransmitMessage - send syslog message by defined means
**********************************************************************/
static bool TransmitMessage(_threadData *threadData, bool TCPDelivery, aMess *mess) {
	int lastError,LastPos;

	mess->text[1022]='\n';
	mess->text[1023]='\0';
	if (mess->text[1021]=='\n') { //special case
		mess->text[1022]='\0';
	}

	LastPos=strlen(&(mess->text[0]));

	if (TCPDelivery) {

		for (int a=0;a<=LastPos;a++) { //Must especially clean for LF, since that is the end-of-message-char in tcp transport. For udp LF is let through to Syslogserver (as of May 06).
			if (mess->text[a]==(char)10)
				mess->text[a]=' ';
		}

		//Add LF in the end, if not present, so that TCP transport works as intended
		switch(LastPos) {
			case 0:
			case 1:
				mess->text[LastPos]=(char)10;
				mess->text[LastPos+1]=(char)0;
				LastPos++;
				break;
			case MAXBUFLEN:
				mess->text[MAXBUFLEN-2]=(char)10;
				mess->text[MAXBUFLEN-1]=(char)0;
				LastPos=MAXBUFLEN-2;
				break;
			default:
				if (mess->text[LastPos-1]==(char)10) {
					mess->text[LastPos]=(char)0;
					break;
				}
				mess->text[LastPos]=(char)10;
				mess->text[LastPos+1]=(char)0;
				LastPos++;
				break;
		}
	}

	while(send(threadData->sock, &(mess->text[0]), LastPos,0) == SOCKET_ERROR) {
		lastError=WSAGetLastError();
		if (lastError==WSAEWOULDBLOCK) { //Buffer full - just wait a while
			Sleep(50);
			continue;
		}
		if (lastError==WSAETIMEDOUT) { //Not too serious. Allow the Agent to try again before logging error
			closeTCPconnection(threadData);
			return false;
		}

		//Error handling
		logger(Error,"Send message failed on socket level.");
		closeTCPconnection(threadData);
		LastTransmitOK=false;
		WSACleanup();
		threadData->sock=threadData->backupsock=0;
		Sleep(1000); //Slowdown loop
		break;
	}


	if (threadData->backupsock!=0) {
		while(send(threadData->backupsock, &(mess->text[0]), LastPos,0) == SOCKET_ERROR) {
			lastError=WSAGetLastError();
			if (lastError==WSAEWOULDBLOCK) { //Buffer full - just wait a while
				Sleep(20);
				continue;
			}

			//Error handling
			logger(Error,"Send message to mirror syslog server failed on socket level.");
			closeTCPconnection(threadData);
			threadData->sock=threadData->backupsock=0;
			Sleep(1000); //Slowdown loop
			break;
		}
	}

	if ((threadData->sock!=0)&&(TCPDelivery))
		lastTCPsendTime=CTime::GetCurrentTime();


	return (threadData->sock ? true : false);
}
/**********************************************************************
ReadyToSendMessage - Report on preparedness to send message

true - ready
false - not ready - failure
**********************************************************************/
static bool ReadyToSendMessage(_threadData	*threadData,applSettings *SettingsPtr) {

	static int PingOk;  //status to check ping status to syslog host.
	static CTime DaTime;
	static CTime LastSuccPing;
	CTimeSpan foo;

	if (LastTransmitOK==false) {
		WSADATA wsdata;
		WORD wVersionRequested = MAKEWORD( 2, 2 );
		if (WSAStartup(wVersionRequested, &(wsdata)) == SOCKET_ERROR){
			logger(Error,"WSAStartup failed.");
			WSACleanup();
			return false;
		}
	}

	//UDP without ping
	//****************
	if ((SettingsPtr->UsePing==false)&&(SettingsPtr->TCPDelivery==false)) {  
		if (threadData->sock==0) { //Not initialised
			return initNetworkSettings(threadData,SOCK_DGRAM,SettingsPtr->DestionationHost,SettingsPtr->DestinationPort,SettingsPtr->BackupDestionationHost,SettingsPtr->BackupDestinationPort);
		} else 
			return true; //already ready to go
	}

	//UDP with ping
	//*************
	if ((SettingsPtr->UsePing==true)&&(SettingsPtr->TCPDelivery==false)) {  

		//Get time
		DaTime=CTime::GetCurrentTime();

		//If succ ping recent - ready
		foo=DaTime-LastSuccPing;
		if (abs(foo.GetSeconds())>PingInterval)
			if (ping_syslog_server(SettingsPtr->DestionationHost)) {
				LastSuccPing=DaTime;
				if (threadData->sock==0) { //Not initialised
					return initNetworkSettings(threadData,SOCK_DGRAM,SettingsPtr->DestionationHost,SettingsPtr->DestinationPort,SettingsPtr->BackupDestionationHost,SettingsPtr->BackupDestinationPort);
				} else 
					return true; //already ready to go
			} else {
				return false;
			}
	}

	//TCP delivery
	//************
	if (SettingsPtr->TCPDelivery==true) { 
		if (threadData->sock==0) {
			return initNetworkSettings(threadData,SOCK_STREAM,SettingsPtr->DestionationHost,SettingsPtr->DestinationPort,SettingsPtr->BackupDestionationHost,SettingsPtr->BackupDestinationPort);
		}
	}

	return true;
}

/**********************************************************************
Slumber - Sleep until next attempt for network contact
false - slumber done
true - terminate
**********************************************************************/
bool Slumber(int _count) {
	int count=2;

	if (_count>4)
		count=10;
	if (_count>10)
		count=60;

	for (int a=0;a<count;a++) {
		if (service_halting())
			return true; 
		Sleep(1000);
	}

	return false;
}
/**********************************************************************
outputMain - Empty queue on messages. Send them over the net
**********************************************************************/
void outputMain(applSettings *SettingsPtr) {
	CString	errorText, newfileName;
	_threadData	threadData;
	aMess mess;
	applSettings loggerInformation;
	int ReadyToSendAttempts;
	WSADATA wsdata;
	WORD wVersionRequested = MAKEWORD( 2, 2 );

	DEBUGSERVICE(Message,"Output handling:Thread start.");
	_set_se_translator(&trans_func);

	try {
	if (WSAStartup(wVersionRequested, &(wsdata)) == SOCKET_ERROR){
		logger(Error,"WSAStartup failed.");
		WSACleanup();
		service_stop_with_error(SOCKET_ERROR,"WSAStartup failed - no network");
		return;
	}

	GetOwnIP();

	SyslogQueueMutex=CreateMutex(NULL,false,NULL);

	LastTransmitOK=true; //Assume initially that it works
	ReadMoreFromRegistry=true;

	// Give	logger function prerequisites to contact the server to report errors...
	loggerInformation=*SettingsPtr;

	threadData.sock=threadData.backupsock=0;
	ErrorCount=0; //logger only sends message if count<5...
	ReadyToSendAttempts=0; //loops of attempted network contact. Slow sown attempts based on this variable.

	while (!GetNetworkAdresses(&threadData,loggerInformation.DestionationHost,loggerInformation.BackupDestionationHost)) {
		if (service_halting()) {
			Sleep(850); //less than 1 sec to avoid race condition with the 1 sek loop in eventlog/ntsl checking LastTRansmitOK status
			return;
		}
		Sleep(2500); //Slowdown
	}

	DEBUGSERVICE(Message,"Output handling:Init done. Starting main output loop.");


	}
	catch(SE_Exception e) {
		logger(Error,"SEH Exception in OutputMain. Event forwarding not functional. Terminating. Error code %u.",e.getSeNumber());
		CreateMiniDump(NULL);
		service_stop_with_error(e.getSeNumber(),"SEH Exception in OutputMain. Event forwarding no0t functional. Terminating.");
		AfxEndThread(0,true);
	}

	catch(...) {
		logger(Message,"SEH Exception in OutputMain. Event forwarding not functional. Terminating.");
		CreateMiniDump(NULL);
		service_stop_with_error(-1,"Exception in OutputMain. Event forwarding not functional. Terminating.");
		AfxEndThread(0,true);
	}

	//output loop
	//***********
	while(true) {

		try {

			if (!(messQueue.empty())) { //fill if something in queue

				WaitForSingleObject(SyslogQueueMutex,INFINITE);
				mess=messQueue.front();
				ReleaseMutex(SyslogQueueMutex);

				do { //until message successfully sent
					while (!ReadyToSendMessage(&threadData, &loggerInformation)) { //Network failure. Wait for improvement... 

						LastTransmitOK=false;
						WSACleanup();
						ReadyToSendAttempts++;
						DEBUGPARSE(Message,"Output handling: Failed to send message. Will slumber and then retry.");
						if (Slumber(ReadyToSendAttempts))  //returns true if service is halting
							return;
					}
				} while (!TransmitMessage(&threadData,SettingsPtr->TCPDelivery, &mess));

				DEBUGPARSE(Message,"Output handling: Message successfully sent.");
				LastTransmitOK=true;
				ErrorCount=0;
				ReadyToSendAttempts=0;

				WaitForSingleObject(SyslogQueueMutex,INFINITE);
				messQueue.pop();
				ReleaseMutex(SyslogQueueMutex);

			} else { //empty queue - slow down

				if ((ReadMoreFromRegistry==false)&&(messQueue.empty())) { //Eventlog process awaits clearToContinue-info. we also...
					//confirmed that no unfortunate context switch occured.
					eventlog_write_lastrun(&thisrun); //Write lastRun to registry, since we successfully have sent all messages in queue.
					ReadMoreFromRegistry=true;
				}

				//Check tcp connection status
				if ((loggerInformation.TCPDelivery)&&(threadData.sock!=0)) {
					CTimeSpan foo=CTime::GetCurrentTime()-lastTCPsendTime;
					if (abs((int)(foo.GetTotalSeconds()))>TCPResetPeriod)
						closeTCPconnection(&threadData);
				}

				if (service_halting())
					break;
				Sleep(850); //less than 1 sec to avoid race condition with the 1 sek loop in eventlog/ntsl checking LastTRansmitOK status

			}
		}
	catch(SE_Exception e) {
		logger(Error,"SEH Exception in Output loop. Event forwarding not functional. Trying to recover. Error code %u.",e.getSeNumber());
		CreateMiniDump(NULL);
		Sleep(2000);
	}

	catch(...) {
		logger(Message,"SEH Exception in Output loop. Event forwarding not functional. Trying to recover.");
		CreateMiniDump(NULL);
		Sleep(2000);
	}

	}
}

/**********************************************************************
insertIntoOutputQueue - receive message from event.c. Put on queue
**********************************************************************/
void insertIntoOutputQueue(void *mess) {
	int number=messQueue.size();

	aMess *messptr=(aMess*)mess;
	while (number>250) {
		Sleep(min(number-250,200));
		if (service_halting())
			return;

		number=messQueue.size();
	}

	WaitForSingleObject(SyslogQueueMutex,INFINITE);
	messQueue.push(*messptr);
	ReleaseMutex(SyslogQueueMutex);
}

/**********************************************************************
LoggerInsertIntoOutputQueue - received message from own, internal system

Internal messages, genereated from SyslogAgent, cannot be garantied to enter the queue. The reason being deadlock issues if the thread that is supposed to empty the queue also wants to fill it sometimes.
Solution: Allowed to insert upto 260 messages limit (up from 250), but no more. If 260 mesages already exist, ignore internal message.

**********************************************************************/

void LoggerInsertIntoOutputQueue(void *mess) {
	int number=messQueue.size();

	aMess *messptr=(aMess*)mess;
	if (number<260) {
		WaitForSingleObject(SyslogQueueMutex,INFINITE);
		messQueue.push(*messptr);
		ReleaseMutex(SyslogQueueMutex);
	} else {
		//drop!
		return;
	}

}

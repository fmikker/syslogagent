#ifndef parseappWatchOnce
#define parseappWatchOnce



#include "..\Syslogserver\common_stdafx.h"
#include "winsock2.h"
#include "AppParse.h"



//Interface according to C syntax, so it can be called from C
extern "C" {
	void AppWatchMain();
}

//also defined in event.c!
typedef struct {
	char text[MAXBUFLEN];
} aMess;


typedef struct {
	FILE *fp;
	CString filename;
	SOCKET sock, backupsock;
	_int64 filePosition;
	__time64_t createdTime;
	struct sockaddr_in server, backupserver;
} _threadData;

#endif
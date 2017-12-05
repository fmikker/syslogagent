#include "..\Syslogserver\common_stdafx.h" 
/*-----------------------------------------------------------------------------
 *
 *  ntsl.c - NTSysLog main
 *
 *    Copyright (c) 1998, SaberNet.net - All rights reserved
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307
 *
 *  $Id: ntsl.c,v 1.10 2002/07/23 06:36:51 jason Exp $
 *
 *	Options:
 *	  _DEBUG     -  Turns on memory leak detection 
 *    NTSL_STUB  -  Minimal functions for module testing
 *
 *  Revision history:
 *    17-Aug-98  JRR  Module completed
 *
 *----------------------------------------------------------------------------*/

//Interface according to C syntax, so it can be called from C
extern "C" {
	void AppWatchMain();
	extern void GetOwnIP();
}



//extern "C" by erno aug04
extern "C" {

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <direct.h>
#include <process.h>
#include <time.h>
#include "ntsl.h"
#include "service.h"
#include "eventlog.h"
#include "engine.h"

//Erno 06, introduced strcpy_s et al
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1



/*-------------------------------[ static data ]------------------------------*/
#define	MAX_ERROR_LEN		512
static void ntsl_cron(void *arg);

extern int hostIsUpdated;
extern volatile bool ReadMoreFromRegistry;


/*-----------------------------[ ntsl_log_error ]-----------------------------
 * Report error to the event log
 *----------------------------------------------------------------------------*/
void ntsl_log_error(char *format, ...)
{
	va_list     args;
    char        s[MAX_ERROR_LEN];
	
    va_start(args, format);
    _vsnprintf_s(s,MAX_ERROR_LEN, MAX_ERROR_LEN, format, args);
    va_end(args);

	s[MAX_ERROR_LEN - 1] = 0;
	ntsl_log_msg(EVENTLOG_ERROR_TYPE, s);
}

/*------------------------------[ ntsl_log_info ]-----------------------------
 * Write information message to the event log
 *----------------------------------------------------------------------------*/
void ntsl_log_info(char *format, ...)
{
	va_list     args;
    char        s[MAX_ERROR_LEN];
	
    va_start(args, format);
    _vsnprintf_s(s, MAX_ERROR_LEN,MAX_ERROR_LEN, format, args);
    va_end(args);

	s[MAX_ERROR_LEN - 1] = 0;
	ntsl_log_msg(EVENTLOG_INFORMATION_TYPE, s);	
}

/*----------------------------[ ntsl_log_warning ]----------------------------
 * Write information message to the event log
 *----------------------------------------------------------------------------*/
void ntsl_log_warning(char *format, ...)
{
	va_list     args;
    char        s[MAX_ERROR_LEN];
	
    va_start(args, format);
    _vsnprintf_s(s,MAX_ERROR_LEN, MAX_ERROR_LEN, format, args);
    va_end(args);

	s[MAX_ERROR_LEN - 1] = 0;
	ntsl_log_msg(EVENTLOG_WARNING_TYPE, s);
}

/*------------------------------[ ntsl_log_msg ]-----------------------------
 * Write message to the event log
 *----------------------------------------------------------------------------*/
void ntsl_log_msg(uint16 etype, char *format, ...)
{
    va_list     args;
    char        s[MAX_ERROR_LEN];
	
    va_start(args, format);
    _vsnprintf_s(s, MAX_ERROR_LEN, MAX_ERROR_LEN, format, args);
    va_end(args);

	s[MAX_ERROR_LEN - 1] = 0;


	#ifndef _DEBUG
	{
		HANDLE		hEvent = RegisterEventSource(NULL, APP_NAME);
		char		*strings[2] = { s, NULL };

		ReportEvent(hEvent, etype, 0, 0, NULL, 1, 0, (LPCSTR*)strings, NULL);
		DeregisterEventSource(hEvent);
	}
	#else
    fprintf(stderr, "%s\n", s);
	#endif

	DEBUGSERVICE(Message,"ntsl error handler activated, code %u.",etype);
	DEBUGSERVICE(Message,s);
}


/*--------------------------------[ ntsl_die ]--------------------------------
 * Log error and exit.
 *----------------------------------------------------------------------------*/
void ntsl_die(char *format, ...)
{
	va_list		args;
    char        s[MAX_ERROR_LEN];
	
    va_start(args, format);
    _vsnprintf_s(s, MAX_ERROR_LEN,MAX_ERROR_LEN, format, args);
    va_end(args);

	s[MAX_ERROR_LEN - 1] = 0;
	ntsl_log_msg(EVENTLOG_ERROR_TYPE, s);


#ifndef NTSL_STUB
	service_stop();
#else
	exit(1);
#endif
}


//#ifdef _DEBUG
#include <crtdbg.h>
//#endif

/*--------------------------------[ ntsl_nit ]--------------------------------
 * Initialize sybsystems
 *----------------------------------------------------------------------------*/
void ntsl_init()
{
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE   );
	_CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE   );
	_CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE   );
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT );
#endif

	engine_init();
	eventlog_init();	
}


/*--------------------------------[ ntsl_run ]--------------------------------
 * Service event loop
 *----------------------------------------------------------------------------*/
void ntsl_run(bool forwardEvents,int _EventLogPollInterval){
	int i;
	int EventLogPollInterval=_EventLogPollInterval;
	bool TimeToStop=false;
	HANDLE AppWatchThreadId;
	//ntsl_log_info(NTSL_INFO_SERVICE, "started");

	EventLogPollInterval=_EventLogPollInterval;
	if (EventLogPollInterval<1) //safety
		EventLogPollInterval=1;

#ifdef _DEBUG
	EventLogPollInterval=2;
#endif

	SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS );

	DEBUGSERVICE(Message,"Lookup own IP.");
	GetOwnIP();

	//Launch point for application logging
	//The new thread begins with to start the outputThread - regardless of application settings

	DEBUGSERVICE(Message,"Launching application logging thread.");
	AppWatchThreadId=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&AppWatchMain,0,0,NULL);	 //Exists if failure

	Sleep(1000); //Let output process get started

	DEBUGSERVICE(Message,"Entering main event log loop - init done in this thread.");

	do
	{

		if (TimeToStop) break;
		if (forwardEvents) { //The function can be disabled from the gui (and just do application logging)
			DEBUGPARSE(Title,"Scanning event logs...");

			hostIsUpdated=false;
			
			if (ReadMoreFromRegistry) { //Set in output process to signal continue
				eventlog_check_events(); 
			} 
		
			DEBUGPARSE(Title,"Sleeping...\n");
		}
		for (i=0;i<EventLogPollInterval;i++) {
			if (!service_halting())	{
				Sleep(1000);
			} else {
				TimeToStop=true; //we're stopping
				break;
			}
		}
	} while(!TimeToStop);

	ntsl_shutdown();
	//The sleep *might* be needed so that app- and output threads can terminate gently during shutdown.
	Sleep(1000);

	//ntsl_log_info(NTSL_INFO_SERVICE, "shutdown");
}

/*------------------------------[ ntsl_shutdown ]------------------------------
 *  Shutdown subsystems
 *----------------------------------------------------------------------------*/
void ntsl_shutdown()
{
	engine_shutdown();
	eventlog_shutdown();

#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
}


} //end extern "C" by erno aug04


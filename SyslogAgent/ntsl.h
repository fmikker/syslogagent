//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  ntsl.h - Common definitions and types
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
 *  $Id: ntsl.h,v 1.14 2002/09/20 06:12:47 jason Exp jason $
 *
 *----------------------------------------------------------------------------*/
#ifndef _NTSL_H_
#define _NTSL_H_

#include <windows.h>
#include "error.h"
#include "safestr.h"

//Erno 06, introduced strcpy_s et al
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1


#define VERSION_MAJOR	    	"3"
#define VERSION_MINOR			"6"
#define COPYRIGHT   			"Copyright (c) 2011, Datagram Consulting, Sweden. GNU licensed."

#define APP_NAME    			"Syslog Agent"
#define SERVICE_NAME			"Syslog Agent"
#define SERVICE_EXE				"SyslogAgent.exe"

#define DEV_NAME   	 			"Datagram\\SyslogAgent"

#define NTSL_NAME_LEN			32
#define NTSL_DESC_LEN			80
#define NTSL_SYS_LEN			256
#define NTSL_DATE_LEN           16
#define NTSL_EVENT_LEN          1024
#define NTSL_PATH_LEN			1024
#define NTSL_PASSWD_LEN			64
#define NTSL_LOOP_WAIT	        500	    /* milliseconds to wait for shutdown event  */
#define NTSL_BIAS               150000  /* milliseconds to sleep between scans      */ 
#define NTSL_LOG_DIR			"log"	

/*------------------------[ portable type definitions ]-----------------------*/
#ifndef uchar
#define uchar  UCHAR
#endif
#ifndef uint16
#define uint16 WORD
#endif
#ifndef uint32
#define uint32 DWORD
#endif
#ifndef int16
#define int16  SHORT
#endif
#ifndef int32
#define int32  LONG
#endif
#ifndef int64
#define int64  DWORDLONG
#endif 
#ifndef bool
#define bool   BOOL
#endif
#ifndef true
#define true   TRUE
#endif
#ifndef false
#define false  FALSE
#endif

/*---------------------------------[ globals ]--------------------------------*/
void		 ntsl_log_info(char *format, ...);
void		 ntsl_log_warning(char *format, ...);
void		 ntsl_log_error(char *format, ...);
void		 ntsl_log_msg(uint16 etype, char *format, ...);
void		 ntsl_die(char *format, ...);
int			 ntsl_check_dir(char *dir, int relative);
void		 ntsl_init();
void		 ntsl_run(bool forwardEvents,int EventLogPollInterval);
void		 ntsl_shutdown();


/*-------------------------------[ static data ]------------------------------*/
#define REG_BUFFER_LEN				2048
//#define EVENTLOG_BUFFER_LEN			500*1024 // Used to be 511*1024, with allocation every loop, with comment 'Changed to support .NET'. But i experienced continous small cpu spikes with that size static allocation. Customer has reported error , where error message indicated that at least 131k was needed.
#define EVENTLOG_BUFFER_LEN			50*1024 // Used to be 511*1024, with allocation every loop, with comment 'Changed to support .NET'. But i experienced continous small cpu spikes with that size static allocation. 
											// docs say 32k is max, but customer has reported error, where error message indicated that at least 131k was needed.
											//system calls fail at 512k buffer - too big
#define MAX_LOG_NAME_LEN			256
#define MAX_MSG_STRINGS				100	// FormatMessage(): %n = {1..99}
#define LAST_RUN_REG				"LastRun"
#define LOOKUPACCOUNTSID			"LookupAccountSID"
#define CARRIGERETURNREPLACEMENTCHARINASCII			"CarrigeReturnReplacementCharInASCII"
#define LINEFEEDRETURNREPLACEMENTCHARINASCII		"LineFeedReplacementCharInASCII"
#define TABREPLACEMENTCHARINASCII					"TabReplacementCharInASCII"

#define EVENTLOG_NO_FLAGS			0x0
#define EVENTLOG_SUCCESS_FLAG		0x20    //Value in Windows as actually zero. But zero does not work well as flag for filtering.
#define EVENTLOG_INFORMATION_FLAG	0x1
#define EVENTLOG_WARNING_FLAG		0x2
#define EVENTLOG_ERROR_FLAG			0x4
#define EVENTLOG_AUDIT_SUCCESS_FLAG	0x8
#define EVENTLOG_AUDIT_FAILURE_FLAG	0x10

#define EVENTLOG_DEFAULT_PRIORITY	9	// Default to user.alert

#define SYSLOG_NAME         "syslog"
#define SYSLOG_REG_KEY_0    "Syslog"
#define SYSLOG_REG_KEY_1    "Syslog1"

//Network settings   (move into registry/gui one day?)
#define ippingtimeout 3000
#define PingInterval 20
#define TCPResetPeriod 60
#define NetworkTimeout 3  //Seconds connect timeout

#define MAXLOADEDLIBRARIES 50
#define MAXLOADEDSIDS 50

//RFC3164 messages
//Not defined in common_Syslogproject since syslogagent is c, not cpp.
#define Emergency 0       
#define Alert 1 
#define Critical 2       
#define Error 3       
#define Warning 4       
#define Notice 5 
#define Informational 6       
#define Debug 7       




#endif

} //end extern "C" by erno aug04


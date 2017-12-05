/*-----------------------------------------------------------------------------
 *
 *  error.h - Application error messages
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
 *  $Id: error.h,v 1.10 2002/07/31 23:09:28 jason Exp $
 *
 *----------------------------------------------------------------------------*/
#define NTSL_ERROR_EVENT_LOG_ACCESS \
			"Unable to access %s event log for host %s"

#define NTSL_ERROR_EVENT_MALLOC \
			"Unable to create event - out of memory"

#define NTSL_ERROR_MKDIR \
			"Unable to create directory: %s"

#define NTSL_ERROR_TIME_DATA_READ \
			"Unable to read data file: %s"

#define NTSL_ERROR_TIME_DATA_WRITE \
			"Unable to write to registry: %s"

#define NTSL_ERROR_MAPI_LOGON \
			"Failed to logon to MAPI subsystem. status: %d  lhSession: %08lx"

#define NTSL_ERROR_MAPI_LOGOFF \
			"Failed to logoff MAPU subsystem. status: %s lhSession: %08lx"

#define NTSL_ERROR_MAPI_DLL_LOAD \
			"Failed to load MAPI library. status: %d"

#define NTSL_ERROR_MAPI_DLL_FUNC \
			"Failed to get function address in MAPI library"

#define NTSL_ERROR_MAPI_SEND \
			"Failed to send MAPI message.  error: %d"

#define NTSL_ERROR_APP_HOME \
			"Failed to locate registry information for %s"

#define NTSL_ERROR_WSASTARTUP \
			"WSAStartup failed with error %d"

#define NTSL_ERROR_SOCKET_INIT \
			"Failed to open syslog socket. error: %d"

#define NTSL_ERROR_SOCKET_SEND \
			"Failed to send data to syslog host. error: %d"

#define NTSL_ERROR_WRITE_LOG \
			"Unable to write to log file: %s"

#define NTSL_ERROR_ENGINE_MUTEX \
			"Failed to create engine mutex. error: %d"

#define NTSL_ERROR_SYSLOG_THREAD \
			"Failed to start syslog service."

#define NTSL_ERROR_SERVICE_DISPATCH \
			"StartServiceCtrlDispatcher failed. error: %d"

#define NTSL_ERROR_SERVICE_STATUS \
			"Failed to report status.  error: %d"

#define NTSL_ERROR_CONFIG_MALLOC \
			"Unable initialize configuration data - out of memory"

#define NTSL_ERROR_CONFIG_OPEN \
			"Unable to open registry: %s"

#define NTSL_ERROR_SYSLOG_CONFIG \
			"A Syslog host has not been configured in the registry"

#define NTSL_ERROR_CONFIG_READ \
			"Unable to read access registry key: %s"

#define NTSL_ERROR_ACTION_MUTEX \
			"Failed to create action mutex. error: %d"

#define NTSL_ERROR_ACTION_QUEUE \
			"Failed to open queue for action: %s"

#define NTSL_ERROR_CRON \
			"Failed to start scheduler. error: %d"

#define NTSL_ERROR_ACTION_EXEC \
			"Failed to execute program for action: %s"

#define NTSL_ERROR_FILTER_REGEX \
			"Invalid expression in filter: %d"

#define NTSL_ERROR_SYSLOG_HOST \
			"Received syslog message from unauthorized host: %s"

#define NTSL_WARNING_LOG_DISK \
			"Log disk running low on space. bytes free: %u"

#define NTSL_INFO_SERVICE \
			"Service %s."

#define NTSL_ERROR_EVENT_READ_BUF \
			"Insufficient read buffer: %d configured; %d required"

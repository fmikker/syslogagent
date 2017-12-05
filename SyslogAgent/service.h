/*-----------------------------------------------------------------------------
 *
 *  service.h - NT Service module
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
 *  $Id: service.h,v 1.9 2002/06/27 21:05:44 jason Exp $
 *
 *----------------------------------------------------------------------------*/

#ifndef _ERNOSRV_H_
#define _ERNOSRV_H_


void service_start(int argc, char **argv);

void service_stop();
void service_stop_with_error(DWORD errorCode, LPTSTR comment);

int  service_halting();


typedef struct {
	char DebugFilePath[256];
	char DebugDumpFilePath[256];
	char DebugOldDumpFilePath[256];
	bool Debugging;
	bool Debug_Service;
	int DebugServiceIndentation;
	bool Debug_Parse;
	int DebugParseIndentation;
	bool Debug_Appl;
	int DebugApplIndentation;
	bool Debug_Logger;
	int DebugLoggerIndentation;
} DebugFlagsDef;

//Debug indentation
#define EndHeader 1
#define Header 2
#define Title 3
#define Message 4

void DEBUGSERVICE(int indentLevel,char *a,...);
void DEBUGPARSE(int indentLevel,char *a,...);
void DEBUGAPPLPARSE(int indentLevel,char *a,...);
void DEBUGLOGGER(int indentLevel,char *a,...);

#endif
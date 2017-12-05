//extern "C" by erno aug04

#include "..\Syslogserver\common_stdafx.h" 


extern void	insertIntoOutputQueue(void*);
extern char applHostIP[16];
#include "output.h" //for logger()
#include "errorHandling.h"


extern "C" {

	/*-----------------------------------------------------------------------------
	*
	*  event.c - Event module 
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
	*  $Id: event.c,v 1.11 2002/09/20 06:12:47 jason Exp jason $
	*
	*  Revision history:
	*    01-Dec-1999  JRR  Removed superfluous gethostbyaddr
	*    28-Sep-1999  JRR  Added support for secondary syslog host
	*    18-Aug-1998  JRR  Module completed
	*
	*----------------------------------------------------------------------------*/
#include <stdio.h>
#include "event.h"
#include <lmerr.h>
#include "service.h"

	//erno
	extern int hostIsUpdated;
	extern char charHostname[128];
	extern int LFReplacementChar;
	int syslogPort, backupSyslogPort;    

	//Erno 06, introduced strcpy_s et al
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1




	/*-------------------------------------------------------------------------
	*  WindowsError
	*  
	*--------------------------------------------------------------------------*/
	int WindowsError(int errorCode, char *errorText){
		HMODULE hModule = NULL; // default to system source
		LPSTR MessageBuffer;
		DWORD dwBufferLength;
		DWORD dwLastError=(DWORD)errorCode;

		DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_IGNORE_INSERTS |FORMAT_MESSAGE_FROM_SYSTEM;

		// If dwLastError is in the network range, load the message source.

		if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
			DEBUGPARSE(Message,"Loading netmsg.dll");
			hModule = LoadLibraryEx(TEXT("netmsg.dll"),NULL,LOAD_LIBRARY_AS_DATAFILE);
			if(hModule != NULL)
				dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
		}

		// Call FormatMessage() to allow for message 
		//  text to be acquired from the system 
		//  or from the supplied module handle.

		DEBUGPARSE(Message,"Precall FormatMessageA.");
		if(dwBufferLength = FormatMessageA(dwFormatFlags,hModule, dwLastError,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR) &MessageBuffer,0,NULL)){
			DEBUGPARSE(Message,"Identified dwBufferLength:%d",dwBufferLength);
			strncpy_s(errorText,250,MessageBuffer,249);
			DEBUGPARSE(Message,"Copy done.");
			//Remove last cr-return
			if (errorText[strlen(errorText)-2]==(char)13) {
				errorText[strlen(errorText)-2]=(char)0;
			}
			DEBUGPARSE(Message,"CRLF Padding cleared.");

			if (dwBufferLength>250) {
				DEBUGPARSE(Message,"Adding dots to long message.");
				strcat_s(errorText,5,"...");
				DEBUGPARSE(Message,"done adding dots to long message.");
			}
			// Free the buffer allocated by the system.
			LocalFree(MessageBuffer);
			DEBUGPARSE(Message,"Free MessageBuffer done.");
			return dwBufferLength;
		} else {
			DEBUGPARSE(Message,"Else: Identified dwBufferLength:%d",dwBufferLength);
		}

		//
		// If we loaded a message source, unload it.
		//
		if(hModule != NULL)
			FreeLibrary(hModule);
		return dwBufferLength;
	}

	/*-------------------------------------------------------------------------
	*  ernoAuditErrorCodes  - replace windows error codes, such as %9005,
	*  with text
	*--------------------------------------------------------------------------*/
	bool ernoAuditErrorCodes(int errorCode, char* errorText) {

		switch (errorCode) {

		case 1537:
			strcpy_s(errorText,256,"DELETE");
			return true;
		case 1538:
			strcpy_s(errorText,256,"READ_CONTROL");
			return true;
		case 1539:
			strcpy_s(errorText,256,"WRITE_DAC");
			return true;
		case 1540:
			strcpy_s(errorText,256,"WRITE_OWNER");
			return true;
		case 1541:
			strcpy_s(errorText,256,"SYNCHRONIZE");
			return true;
		case 4384:
			strcpy_s(errorText,256,"Query event state");
			return true;
		case 4385:
			strcpy_s(errorText,256,"Modify event state");
			return true;
		case 4448:
			strcpy_s(errorText,256,"Query mutant state");
			return true;
		case 4416:
			strcpy_s(errorText,256,"ReadData (or ListDirectory)");
			return true;
		case 4417:
			strcpy_s(errorText,256,"WriteData (or AddFile)");
			return true;
		case 4418:
			strcpy_s(errorText,256,"AppendData (or AddSubdirectory or CreatePipeInstance)");
			return true;
		case 4420:
			strcpy_s(errorText,256,"WriteEA");
			return true;
		case 4424:
			strcpy_s(errorText,256,"WriteAttributes");
			return true;
		case 4432:
			strcpy_s(errorText,256,"Query key value");
			return true;
		case 4433:
			strcpy_s(errorText,256,"Set key value");
			return true;
		case 4435:
			strcpy_s(errorText,256,"Enumerate sub-keys");
			return true;
		case 4436:
			strcpy_s(errorText,256,"Notify about changes to key");
			return true;
		case 4464:
			strcpy_s(errorText,256,"Communicate using port");
			return true;
		case 4528:
			strcpy_s(errorText,256,"Query semaphore state");
			return true;
		case 4529:
			strcpy_s(errorText,256,"Modify semaphore state");
			return true;
		case 5376:
			strcpy_s(errorText,256,"ConnectToServer");
			return true;
		case 5377:
			strcpy_s(errorText,256,"ShutdownServer");
			return true;
		case 5378:
			strcpy_s(errorText,256,"InitializeServer");
			return true;
		case 5379:
			strcpy_s(errorText,256,"CreateDomain");
			return true;
		case 5380:
			strcpy_s(errorText,256,"EnumerateDomains");
			return true;
		case 5381:
			strcpy_s(errorText,256,"LookupDomain");
			return true;
		case 5392:
			strcpy_s(errorText,256,"ReadPasswordParameters");
			return true;
		case 5393:
			strcpy_s(errorText,256,"WritePasswordParameters");
			return true;
		case 5394:
			strcpy_s(errorText,256,"ReadOtherParameters");
			return true;
		case 5395:
			strcpy_s(errorText,256,"WriteOtherParameters");
			return true;
		case 5396:
			strcpy_s(errorText,256,"CreateUser");
			return true;
		case 5398:
			strcpy_s(errorText,256,"CreateLocalGroup");
			return true;
		case 5399:
			strcpy_s(errorText,256,"GetLocalGroupMembership");
			return true;
		case 5400:
			strcpy_s(errorText,256,"ListAccounts");
			return true;
		case 5401:
			strcpy_s(errorText,256,"LookupIDs");
			return true;
		case 5402:
			strcpy_s(errorText,256,"AdministerServer");
			return true;
		case 5444:
			strcpy_s(errorText,256,"ReadAccount");
			return true;
		case 5445:
			strcpy_s(errorText,256,"WriteAccount");
			return true;
		case 5447:
			strcpy_s(errorText,256,"SetPassword (without knowledge of old password)");
			return true;
		case 5448:
			strcpy_s(errorText,256,"ListGroups");
			return true;
		case 5440:
			strcpy_s(errorText,256,"ReadGeneralInformation");
			return true;
		case 5441:
			strcpy_s(errorText,256,"ReadPreferences");
			return true;
		case 5443:
			strcpy_s(errorText,256,"ReadLogon");
			return true;
		case 6656:
			strcpy_s(errorText,256,"Enumerate desktops");
			return true;
		case 6672:
			strcpy_s(errorText,256,"Read objects");
			return true;
		case 6679:
			strcpy_s(errorText,256,"Write objects");
			return true;
		case 7168:
			strcpy_s(errorText,256,"Connect to service controller");
			return true;
		case 7171:
			strcpy_s(errorText,256,"Lock service database for exclusive access");
			return true;
		default:
			return false;
		}
	}
	/*-------------------------------------------------------------------------
	*  ParseWindowsErrorCodes  - replace windows error codes, such as %9005,
	*  with text
	*--------------------------------------------------------------------------*/
	void ParseWindowsErrorCodes(char *msgString, char* facilityName) {
		char *posAfterErrorCode=NULL,*BreakpointInString,*startpos=msgString,*testpos=msgString;
		int errorCode=0,anotherPerCent=0;
		char tempString[2048]="";
		char errorText[256];
		char errorMessage[256];
		bool UseAuditErrorMessages;

			//Audit eventlog messages can't use Windows error functions. Don't ask why - they don't parse them (or parses wrong)
			if (strcmp("Security",facilityName)==0)
				UseAuditErrorMessages=true;
			else 
				UseAuditErrorMessages=false;
		
			/*  CRASH TEST - dump not created  
			__try {
	char test[4];
	strcpy_s(test,4,"This is a long message");
	DEBUGPARSE(Message,"test %s",test);
			}
		__except( CreateMiniDump( GetExceptionInformation() ), EXCEPTION_EXECUTE_HANDLER ) {
			logger(Error,"prutt");
			Sleep(3000);
		}

*/
			while(true) {

				//Search if any error message can be found in the message
				BreakpointInString=strstr(testpos,"%");
				if (BreakpointInString==NULL) break;

				DEBUGPARSE(Message,"Found possible error code in message: %s",testpos);

				//Sometimes, double %% are sent...
				if(*(BreakpointInString+1)=='%') 
					anotherPerCent=1;
				else
					anotherPerCent=0;

				//Ensure found '%' is followed by a number
				//status=sscanf((BreakpointInString+2),"%d",&errorCode);
				errorCode=atoi(BreakpointInString+1+anotherPerCent);
				DEBUGPARSE(Message,"The following error code, if any, was identified: %d",errorCode);
				if ((errorCode==0)&&(*(BreakpointInString+1+anotherPerCent)!='0')) { //not a number after the percent chars
					testpos=BreakpointInString+1+anotherPerCent;
					continue;
				}

				//Go past the numbers
				posAfterErrorCode=BreakpointInString+1+anotherPerCent;
				while (((*posAfterErrorCode)>(char)47)&&((*posAfterErrorCode)<(char)58))
					posAfterErrorCode++;

				//lookup errorCode
				if (UseAuditErrorMessages) {
					DEBUGPARSE(Message,"Looking up audit error number %d",errorCode);
					if (ernoAuditErrorCodes(errorCode,&(errorText[0]))==0) {
						errorText[0]='\0';
					}
				} else {
					DEBUGPARSE(Message,"Looking up Windows error number %d",errorCode);
					if (WindowsError(errorCode,&(errorText[0]))==0) { //obtained no info
						DEBUGPARSE(Message,"Returned from WindowsError 1.");
						errorText[0]='\0';
					} else {
						DEBUGPARSE(Message,"Returned from WindowsError 2.");
					}
				}

				//Fill tempString with replacement text, starting at the %
				if (errorText[0]!=0) {
					DEBUGPARSE(Message,"Adding errorText to message: %s",errorText);
					if (UseAuditErrorMessages) { //For audit events, hide the entire code
						sprintf_s(errorMessage," %s",errorText);
					} else {
						sprintf_s(errorMessage,"%%%d (%s)",errorCode,errorText);
					}
				} else {
					DEBUGPARSE(Message,"Adding errorCode only to message: %d",errorCode);
					sprintf_s(errorMessage,"%%%d",errorCode);
				}

				//copy text upto this error code
				strncat_s(tempString,startpos,(BreakpointInString-startpos));
				strncat_s(tempString,errorMessage,_TRUNCATE);

				startpos=posAfterErrorCode;
				testpos=BreakpointInString+1+anotherPerCent;
				DEBUGPARSE(Message,"Adding one error message done.");
			};

			if (tempString[0]!='\0') {//If anything has been modified, copy new string. Otherwise, leave intact.
				strcat_s(tempString,posAfterErrorCode);
				strncpy_s(msgString,1024,tempString,1023);
			}
	DEBUGPARSE(Message,"Leaving ParseWindowsErrorCodes.");
	}

	/*------------------------------[ event_output ]------------------------------
	*  Output the event
	*
	*  Parameters:
	*		event		event to format
	*		fp			file pointer
	*
	*  Return value:
	*		success		(0)
	*		failure		(-1)
	*
	*----------------------------------------------------------------------------*/
	int _event_output(ntsl_event *event){
		aMess mess; //from appWatch.h
		char LFstring[2];

		LFstring[0]=(char)LFReplacementChar;LFstring[1]='\0';

		if ( (event == NULL) )
			return(-1);

		//erno added own IP information. Update charHostname every time in this loop (currently every 20secs) *if* new data has arrived.
		if (!hostIsUpdated) {
			if( gethostname(charHostname, 128) != 0 ) {  //bad karma
				charHostname[0]='\0';
			}
			hostIsUpdated=true;
		}

		//erno: parse any windows message. 
		ParseWindowsErrorCodes(event->msg,event->facilityName);

		DEBUGPARSE(Message,"Preparing final string to send.");
		_snprintf_s(mess.text,sizeof(mess.text),_TRUNCATE, "<%d>%s %s %s%s %d%s%s%s%s%s %s", 
			event->priority, 
			event->date,
			charHostname,
			event->source, 
			event->etype, 
			event->id, 
			(event->user[0] == 0 ? "" : " "),
			(event->domain[0] == 0 ? "" : event->domain),
			(event->domain[0] == 0 ? "" : "\\"),

			(event->user[0] == 0 ? "" : event->user),
			//raden nedan adderad för att infoga LF, som är vår kod för radbryt om anv finns
			(event->user[0] == 0 ? "" : LFstring),

			//erno	(event->msg[0] == ':' ? "" : " "), 
			event->msg);

		DEBUGPARSE(Message,"Inserting message into output queue: %s", mess.text);
		insertIntoOutputQueue((void*)&mess);

		return(0);
	}

} //end extern "C" by erno aug04


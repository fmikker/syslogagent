//extern "C" by erno aug04
#include "..\Syslogserver\common_stdafx.h" 


#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ntsl.h"
#include "engine.h"
#include "errorHandling.h"

extern void initFilterEventArray( DWORD *A);
extern void logger(int severity,char *text,...);

//Location counter in registry. Global since only output process can write it to registry
uint32 thisrun;

extern "C" {

	/*-----------------------------------------------------------------------------
	*
	*  eventlog.c - Windows NT eventlog module
	*
	*    Copyright (c) 1998-2002, SaberNet.net - All rights reserved
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
	*  $Id: eventlog.c,v 1.13 2002/09/20 06:12:47 jason Exp $
	*
	*  Revision history:
	*    22-Jul-2002  JRR  Added support for user defined event logs
	*    14-Jun-2002  LWE  Fixed compiler warning.
	*    22-Apr-2002  FK   Fixed language independence and multiple message files
	*    15-Oct-1999  JRR  Fixed handling of events w/ missing descriptions
	*    06-Jul-1998  JRR  Module completed
	*
	*----------------------------------------------------------------------------*/
#include "windows.h"
#include "eventlog.h"
#include "Registrysettings.h"
#include "service.h"
#include "list.h"

	static _LoadedLibrary LoadedLibrary[MAXLOADEDLIBRARIES];
	static uint32 filterArray[MAXEVENTIDFILTERNUMBER];
	static _LoadedSIDS LoadedSIDS[MAXLOADEDSIDS];


	static bool LookupAccountSID;
	int CRReplacementChar; //which char (in ascii) to replace CR with in input.
	int LFReplacementChar; //which char (in ascii) to replace LF with in input.
	int TABReplacementChar; //which char (in ascii) to replace tab with in input. Added since sql inserts fail with tabs that are escaped
	static uint32 lastunload;
	char *eventBuffer;
	int eventBufferSize; //Yes, stupid. But sizeof(eventBuffer) does not work.
	EVENTLOGRECORD *record;

#define addoffset(p,o) ( (byte *) (p) + o )



	/*---------------------------[ private structures ]---------------------------*/
	typedef struct eventlog_data_st
	{
		int 	captureFlags;

		int 	successPriority;
		int 	informationPriority;
		int 	warningPriority;
		int 	errorPriority;
		int 	auditSuccessPriority;
		int 	auditFailurePriority;
		char	name[MAX_LOG_NAME_LEN];
		DWORD	recordOffset; 
		DWORD  verificationTime; //used to detect if a eventlog deletion has occured, *and* new entries up to the current id has already been entered.
	} eventlog_data;

	/*----------------------------[ private functions ]---------------------------*/
	static char  *eventlog_lcase(char *s);
	static int    eventlog_append_data(char *s, int len, char* data, BOOL chop);
	static char **eventlog_strings_to_array(char *strings, int num);
	static int    eventlog_read_events(eventlog_data* eventlog, uint32 *lastrun, uint32 *thisrun);
	static int    eventlog_read_lastrun(uint32 *lastrun);
	static int    eventlog_set_event_type(ntsl_event *event, int id);
	static int    eventlog_set_event_priority(ntsl_event *event, int type, eventlog_data* eventlog);
	static int    eventlog_set_user(ntsl_event *event, PSID sid, DWORD len);
	static eventlog_data* eventlog_new_log(const char* name);

	static int ernoReadEventLog(HANDLE hLog,DWORD flags,DWORD offset,DWORD *BytesRead,DWORD *BytesNeeded);

	//No longer static - output process uses it
	int    eventlog_write_lastrun(uint32 *lastrun);



	/*-------------------------------[ static data ]------------------------------*/
	static List* eventlog_list = NULL;

	//erno
	int hostIsUpdated=false;
	char charHostname[128];

	extern volatile bool ReadMoreFromRegistry;

	/*-----------------------------[ eventlog_lcase ]-----------------------------
	* Convert string to lowercase
	*----------------------------------------------------------------------------*/
	static char *eventlog_lcase(char *s)
	{
		char *sp = s;

		while(*sp) { *sp = tolower(*sp); sp++; }
		return(s);
	} 


	//--------------------------[ ernoException ]--------------------------
	//erno 08-01 - no longer used. Replaced by win API CreateMiniDump
	/*
	int ernoException(unsigned int code, struct _EXCEPTION_POINTERS *ep) {

	char text[256];

	DEBUGSERVICE(Message,"Entered own exception handler.");

	switch(code) {
	case STATUS_ACCESS_VIOLATION:
	strcpy_s(text,"EXCEPTION_ACCESS_VIOLATION");
	break;
	case STATUS_DATATYPE_MISALIGNMENT:
	strcpy_s(text,"EXCEPTION_DATATYPE_MISALIGNMENT");
	break;
	case STATUS_BREAKPOINT:
	strcpy_s(text,"EXCEPTION_BREAKPOINT");
	break;
	case STATUS_SINGLE_STEP:
	strcpy_s(text,"EXCEPTION_SINGLE_STEP");
	break;
	case STATUS_ARRAY_BOUNDS_EXCEEDED:
	strcpy_s(text,"EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
	break;
	case STATUS_FLOAT_DENORMAL_OPERAND:
	strcpy_s(text,"EXCEPTION_FLT_DENORMAL_OPERAND");
	break;
	case STATUS_FLOAT_DIVIDE_BY_ZERO:
	strcpy_s(text,"EXCEPTION_FLT_DIVIDE_BY_ZERO");
	break;
	case STATUS_FLOAT_INEXACT_RESULT:
	strcpy_s(text,"EXCEPTION_FLT_INEXACT_RESULT");
	break;
	case STATUS_FLOAT_INVALID_OPERATION:
	strcpy_s(text,"EXCEPTION_FLT_INVALID_OPERATION");
	break;
	case STATUS_FLOAT_OVERFLOW:
	strcpy_s(text,"EXCEPTION_FLT_OVERFLOW");
	break;
	case STATUS_FLOAT_STACK_CHECK:
	strcpy_s(text,"EXCEPTION_FLT_STACK_CHECK");
	break;
	case STATUS_FLOAT_UNDERFLOW:
	strcpy_s(text,"EXCEPTION_FLT_UNDERFLOW");
	break;
	case STATUS_INTEGER_DIVIDE_BY_ZERO:
	strcpy_s(text,"EXCEPTION_INT_DIVIDE_BY_ZERO");
	break;
	case STATUS_INTEGER_OVERFLOW:
	strcpy_s(text,"EXCEPTION_INT_OVERFLOW");
	break;
	case STATUS_PRIVILEGED_INSTRUCTION:
	strcpy_s(text,"EXCEPTION_PRIV_INSTRUCTION");
	break;
	case STATUS_IN_PAGE_ERROR:
	strcpy_s(text,"EXCEPTION_IN_PAGE_ERROR");
	break;
	case STATUS_ILLEGAL_INSTRUCTION:
	strcpy_s(text,"EXCEPTION_ILLEGAL_INSTRUCTION");
	break;
	case STATUS_NONCONTINUABLE_EXCEPTION:
	strcpy_s(text,"EXCEPTION_NONCONTINUABLE_EXCEPTION");
	break;
	case STATUS_STACK_OVERFLOW:
	strcpy_s(text,"EXCEPTION_STACK_OVERFLOW");
	break;
	case STATUS_INVALID_DISPOSITION:
	strcpy_s(text,"EXCEPTION_INVALID_DISPOSITION");
	break;
	case STATUS_GUARD_PAGE_VIOLATION:
	strcpy_s(text,"EXCEPTION_GUARD_PAGE");
	break;
	case STATUS_INVALID_HANDLE:
	strcpy_s(text,"EXCEPTION_INVALID_HANDLE");
	break;
	//		case STATUS_POSSIBLE_DEADLOCK:
	//			strcpy_s(text,"EXCEPTION_POSSIBLE_DEADLOCK");
	//			break;
	default:
	strcpy_s(text,"Unknown error");
	return EXCEPTION_CONTINUE_SEARCH;

	}


	sprintf_s(text,"%s. Address %ul",text,ep->ExceptionRecord->ExceptionAddress);
	DEBUGSERVICE(Message,text);

	logger(Error,text);

	return EXCEPTION_EXECUTE_HANDLER;

	}
	*/
	/*--------------------------[ eventlog_append_data ]--------------------------
	* Appends up to n bytes of data to the end of the buffer. (Null terminating) 
	*
	*	Returns:
	*		success		0
	*		failure		-1 
	*----------------------------------------------------------------------------*/
	static int eventlog_append_data(char *buffer, int n, char *data, BOOL chop)
	{
		int	rc = -1;

		if ( (buffer != NULL) && (data != NULL) && (*data != NULL)){

			DEBUGPARSE(Message,"Input to event message: %s",data);

			int i  = strlen(buffer);

			if (chop)
				while(iswspace(*data)) data++;

			while (*data){
				//Test for CR
				if (*data == (char)13) {
					if (CRReplacementChar!=0)
						*data = (char)CRReplacementChar;  //used to be 127 static
					else {
						data++;
						continue;
					}
				}
				//Test for LF
				if (*data == (char)10) {
					if (LFReplacementChar!=0)
						*data = (char)LFReplacementChar;  
					else {
						data++;
						continue;
					}
				}
				//Test for tab
				if (*data == (char)9) {
					if (TABReplacementChar!=0)
						*data = (char)TABReplacementChar;  
					else {
						data++;
						continue;
					}
				}

				//Ordinary char...
				if (i < (n - 1))	
					buffer[i++] = *data;
				data++;
			}
			buffer[i] = (char)0;
			rc = 0;
		}

		return(rc);
	}

	/*------------------------[ eventlog_strings_to_array ]------------------------
	* Converts a concatenation of null terminated strings to an array of strings.
	* (Null terminating)
	*----------------------------------------------------------------------------*/
	static char **eventlog_strings_to_array(char *strings, int num)
	{
		static char *array[MAX_MSG_STRINGS];
		int			 i;

		if (strings == NULL)
		{
			array[0] = NULL;
			return(array);
		}

		if (num > MAX_MSG_STRINGS)
			num = MAX_MSG_STRINGS;

		for(i=0; i<num; i++)
		{
			array[i] = strings;
			strings += strlen(strings) + 1;
		}
		array[i] = NULL;

		return(array);
	}

	/*--------------------------[ eventlog_check_event ]--------------------------
	* Returns non-zero value if interested in the event; otherwise returns 0

	erno 2008-02: Note that eventlog_success type was identified, and it has value zero. sigh. This caused changes.
	*----------------------------------------------------------------------------*/
	static int eventlog_check_event(eventlog_data* eventlog, int id)
	{
		int rc = 1;

		switch(id){
		case EVENTLOG_ERROR_TYPE:		
			rc = (eventlog->captureFlags & EVENTLOG_ERROR_FLAG);
			break;
		case EVENTLOG_WARNING_TYPE:     
			rc = (eventlog->captureFlags & EVENTLOG_WARNING_FLAG);  
			break;
		case EVENTLOG_AUDIT_FAILURE:    
			rc = (eventlog->captureFlags & EVENTLOG_AUDIT_FAILURE_FLAG);
			break;
		case EVENTLOG_AUDIT_SUCCESS:    
			rc = (eventlog->captureFlags & EVENTLOG_AUDIT_SUCCESS_FLAG);     
			break;
		case EVENTLOG_INFORMATION_TYPE: 
			rc = (eventlog->captureFlags & EVENTLOG_INFORMATION_FLAG);     
			break;
		case EVENTLOG_SUCCESS:  //added later by Windows, it seems.
			rc = (eventlog->captureFlags & EVENTLOG_SUCCESS_FLAG);     
			break;
		default:
			DEBUGPARSE(Warning, "Unknown event log type %d. Event forwarded.",id);
			rc=id;
		}
		return(rc);
	}

	/*-------------------------[ eventlog_set_event_type ]-------------------------
	* Sets the type for the given event.
	*----------------------------------------------------------------------------*/
	static int eventlog_set_event_type(ntsl_event *event, int id)
	{
		int   rc    = -1;
		char *eType = NULL;

		if (NULL != event)
		{
			switch (id)
			{
				
			case EVENTLOG_SUCCESS:			eType = NTSL_EVENT_SUCCESS;	     break;  //separate auditsuccess from success?
			case EVENTLOG_ERROR_TYPE:		eType = NTSL_EVENT_ERROR;	     break;
			case EVENTLOG_WARNING_TYPE:     eType = NTSL_EVENT_WARNING;      break;
			case EVENTLOG_INFORMATION_TYPE: eType = NTSL_EVENT_INFORMATION;  break;
			case EVENTLOG_AUDIT_SUCCESS:    eType = NTSL_EVENT_SUCCESS;      break;
			case EVENTLOG_AUDIT_FAILURE:    eType = NTSL_EVENT_FAILURE;      break;
			};

			if (eType != NULL)
			{
				strcpy_s(event->etype,  eType);	
				rc = 0;
			}
		}

		return(rc);
	}


	/*-----------------------[ eventlog_set_event_priority ]-----------------------
	* Sets the priority for the given event.
	*----------------------------------------------------------------------------*/
	static int eventlog_set_event_priority(ntsl_event *event, int type, eventlog_data* eventlog)
	{
		int   rc    = -1;

		if (NULL != event)
		{
			// Set default.
			switch (type)
			{
			case EVENTLOG_ERROR_TYPE:
				event->priority = eventlog->errorPriority;
				break;

			case EVENTLOG_WARNING_TYPE:
				event->priority = eventlog->warningPriority;
				break;

			case EVENTLOG_SUCCESS:
				event->priority = eventlog->successPriority;
				break;

			case EVENTLOG_INFORMATION_TYPE:
				event->priority = eventlog->informationPriority;
				break;

			case EVENTLOG_AUDIT_SUCCESS:
				event->priority = eventlog->auditSuccessPriority;
				break;

			case EVENTLOG_AUDIT_FAILURE:
				event->priority = eventlog->auditFailurePriority;
				break;

			default:
				event->priority = NTSL_DEFAULT_PRIORITY;
				break;
			}		

			rc = 0;
		}

		return(rc);
	}

	/*-------------------------[ ernoUnloadLoadSIDS ]-------------------------
	* Unload all SIDS
	*
	*----------------------------------------------------------------------------*/
	void ernoUnloadLoadSIDS() {
		int i=0;
		while ((i<MAXLOADEDSIDS)&&(LoadedSIDS[i].valid!=false)) {
			LoadedSIDS[i].valid=false;
			i++;
		}
	}
	/*-------------------------[ ernoUnloadLoadLibrary ]-------------------------
	* Unload all libraries
	*
	*----------------------------------------------------------------------------*/
	void ernoUnloadLoadLibrary() {
		int i;

		for(i=0;i<MAXLOADEDLIBRARIES;i++) {

			if (LoadedLibrary[i].name[0]!='\0') {
				FreeLibrary((HMODULE)LoadedLibrary[i].hlib);
				LoadedLibrary[i].name[0]='\0';
			} else {
				break;
			}
		}
	}
	/*-------------------------[ ernoLoadLibrary ]-------------------------
	* Retrieves the event message from the appropriate DLL.
	*
	*	Returns:
	*		success		0
	*		failure		-1 
	*----------------------------------------------------------------------------*/
	HMODULE ernoLoadLibrary(char *singeldll) {
		int i=0;
		DWORD DaError;

		if (singeldll==NULL) return NULL;

		//Already opened?
		while ((i<MAXLOADEDLIBRARIES)&&(LoadedLibrary[i].name[0]!='\0')) {
			if (strstr(singeldll,LoadedLibrary[i].name)!=NULL) {
				return LoadedLibrary[i].hlib;
			}
			i++;
		}

		if (i==(MAXLOADEDLIBRARIES-1)) { //loaded full
			ernoUnloadLoadLibrary(); //unload all
			i=0;
		}

		//open library
		LoadedLibrary[i].hlib= LoadLibraryEx(singeldll, NULL, LOAD_LIBRARY_AS_DATAFILE);

		if (LoadedLibrary[i].hlib!=NULL) {
			strncpy_s(LoadedLibrary[i].name,sizeof(LoadedLibrary[i].name),singeldll,sizeof(LoadedLibrary[0].name));
			return LoadedLibrary[i].hlib;
		} else {
			DaError=GetLastError();
		}
		return 0;
	}

	/*-------------------------[ ScanDLLManuallyForMessage ]-------------------------
	* Taken from http://win32.mvps.org/misc/fm.cpp, without which this would not be possible
	*
	*----------------------------------------------------------------------------*/
	int ScanDLLManuallyForMessage(HINSTANCE hlib, DWORD id, LPTSTR *msg) {


		MESSAGE_RESOURCE_ENTRY *pMRE;
		DWORD first, last, count, i, j;
		DWORD from = 0, to = 0xffff, idTrim=0xffff;
		MESSAGE_RESOURCE_DATA *pMRD;

		HRSRC hRes = FindResource( hlib, MAKEINTRESOURCE( 1 ),MAKEINTRESOURCE( RT_MESSAGETABLE ) );
		if ( hRes == NULL )	{ //Can happen...
			//fwprintf( stderr, L"FindResource( 1, RT_MESSAGETABLE ): gle = %lu\n", GetLastError() );
			return 0;
		}

		HGLOBAL hgRes = LoadResource( hlib, hRes );
		if ( hgRes == NULL ){
			logger(Warning, "LoadResource failed with error %d.",GetLastError());
			return 0;
		}

		pMRD = (MESSAGE_RESOURCE_DATA *) LockResource( hgRes );
		if ( pMRD == NULL )	{
			logger(Warning, "LockResource failed with error %d.",GetLastError());
			return 0;
		}

		// look for string table, extract first/last entries

		if ( pMRD->NumberOfBlocks == 0 ){
			return 0;
		}

		first = last = pMRD->Blocks[0].LowId & idTrim;
		count = 0;

		for ( i = 0; i < pMRD->NumberOfBlocks; ++ i )
		{
			count += (pMRD->Blocks[i].HighId  & idTrim) - (pMRD->Blocks[i].LowId  & idTrim) + 1;
			if ( first > (pMRD->Blocks[i].LowId  & idTrim))
				first = (pMRD->Blocks[i].LowId & idTrim);
			if ( last < (pMRD->Blocks[i].HighId  & idTrim))
				last =( pMRD->Blocks[i].HighId & idTrim);
		}

		printf( " %lu strings, range %lu-%lu, in %lu blocks\n", count, first, last, pMRD->NumberOfBlocks );

		if ( from < first )
		{
			fwprintf( stderr, L"Adjusting \"from\": %lu -> %lu\n", from, first );
			from = first;
		}

		if ( to > last )
		{
			fwprintf( stderr, L"Adjusting \"to\": %lu -> %lu\n", to, last );
			to = last;
		}

		// now display the strings

		for ( unsigned int i = 0; i < pMRD->NumberOfBlocks; ++ i ){
			for ( pMRE = (MESSAGE_RESOURCE_ENTRY *) addoffset( pMRD, pMRD->Blocks[i].OffsetToEntries ), j = pMRD->Blocks[i].LowId & idTrim; 
				j <= (pMRD->Blocks[i].HighId & idTrim);	
				pMRE = (MESSAGE_RESOURCE_ENTRY *) addoffset( pMRE, pMRE->Length ), ++ j )
			{
				wprintf(L"%lu:  %s",j,pMRE->Text );
				if (id==j) {
					msg=(LPTSTR*)malloc(sizeof(TCHAR)*1024);
					strncpy_s((char *)msg,sizeof(TCHAR)*1024,(char *)(pMRE->Text),1024);
					return strlen((char*)msg);
				}

			}
		}

		return 0;
	}
	/*-------------------------[ eventlog_set_event_msg ]-------------------------
	* Retrieves the event message from the appropriate DLL.
	*
	*	Returns:
	*		success		0
	*		failure		-1 
	*----------------------------------------------------------------------------*/
	static int eventlog_set_event_msg(ntsl_event *event, char *logType,	uint32 id, char *strings, int numStrings){
		char	**array; 
		char	buffer[REG_BUFFER_LEN];
		char	regSource[REG_BUFFER_LEN];
		char	dll[REG_BUFFER_LEN];
		uint32	bufsize = REG_BUFFER_LEN;
		uint32	bytes   = 0;
		uint32	regtype = 0;
		HINSTANCE	hlib;
		HKEY	hkey;
		uint32  sequencenumber = 0;
		char    singeldll[REG_BUFFER_LEN];
		BOOL    handled = false;
		int FormatMessageLastError;
		char *msg  = NULL;
		//LPTSTR msg  = NULL;

		buffer[0]='\0';

		/* check paramaters */
		if ( (event == NULL) || (logType == NULL) || (strings == NULL) )
			return(-1);

		array = eventlog_strings_to_array(strings, numStrings);
		event->id = id;
		event->id &= 0x0000FFFF;  //ok. Does not gimp real id used for lookup, but only presentation code.


		/* check strings array */
		if ( (numStrings) && (array[0] == NULL) )
			return(-1);

		/* build registry path */
		sprintf_s(regSource, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s", logType,event->source);

		/* load message text */
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regSource, 0, KEY_READ,&hkey) == ERROR_SUCCESS){

			if (RegQueryValueEx(hkey, "EventMessageFile", 0, &regtype, (unsigned char*)buffer, &bufsize) == ERROR_SUCCESS)	{

			} else {
				if (RegQueryValueEx(hkey, "ProviderGuid", 0, &regtype, (unsigned char*)buffer, &bufsize) == ERROR_SUCCESS)	{
					//Ok, at least we have a providerGuid, which we can use in another part of the registry.
					RegCloseKey(hkey);
					sprintf_s(regSource, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Publishers\\%s",buffer);
					buffer[0]='\0';
					if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regSource, 0, KEY_READ,&hkey) == ERROR_SUCCESS){
						RegQueryValueEx(hkey, "MessageFileName", 0, &regtype, (unsigned char*)buffer, &bufsize);
					}
				}
			}
			RegCloseKey(hkey);
		}

		if (strlen(buffer)>0) { //success to get a reference to a dll
			if (ExpandEnvironmentStrings(buffer, dll, REG_BUFFER_LEN) > 0){
				/* Parse into different ones if needed */
				while(!eventlog_parse_libs(dll,singeldll,sequencenumber)){

					// erno2005 - replaced as LoadLibraryEx uses 30+% of CPU during full load
					//					if ((hlib = LoadLibraryEx(singeldll, NULL, DONT_RESOLVE_DLL_REFERENCES)) != NULL)
					if ((hlib = ernoLoadLibrary(singeldll)) != NULL){

						msg  = NULL;

						bytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |FORMAT_MESSAGE_ARGUMENT_ARRAY | 60, 
							hlib, 
							id, 
							0, 
							(LPTSTR)&msg, 
							NTSL_EVENT_LEN, 
							array);
						FormatMessageLastError=GetLastError();
						if (bytes == 0){
							bytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY | 60, 
								NULL, 
								id, 
								0, 
								(LPTSTR)&msg, 
								NTSL_EVENT_LEN, 
								array);
						}
						FormatMessageLastError=GetLastError();
						if (bytes > 0){
							handled = true;
							event->msg[0] = (char)0;
							eventlog_append_data(event->msg, NTSL_EVENT_LEN, (char *)msg, TRUE);
							LocalFree((HANDLE)msg);
							break; //found the message - no need to continue.
						} else {
							FormatMessageLastError=GetLastError();
							if ((FormatMessageLastError!=ERROR_MR_MID_NOT_FOUND)&(FormatMessageLastError!=ERROR_RESOURCE_TYPE_NOT_FOUND)) {
								logger(4,"SyslogAgent failed to find error text for a message (FormatMessage error). error: %d dll: %s message: %s, eventid: %d date: %s",FormatMessageLastError,buffer,event->msg,event->id,event->date);
							}
						}

						//FreeLibrary((HMODULE)hlib);
					}
					sequencenumber++;
				} //end while more dll's exist...
			}

		}

		if (handled){
			; // handled above
		}
		else if (numStrings > 0){
			int i;
			char tempBuff[2048]="";

			for (i=0; i<numStrings; i++){
				if (array[i][0]!='\0') {
					strncat_s(tempBuff,sizeof(tempBuff),array[i],_TRUNCATE);
					strncat_s(tempBuff,sizeof(tempBuff)," ",_TRUNCATE);
				}
			}
			int b=strlen(tempBuff)-1; //It's actually needed! The max function messes with data types, or similar. Max (strlen(buff)-1,0) returned -1!!!!!
			tempBuff[max(b,0)]='\0'; //Clear last space
			eventlog_append_data(event->msg, NTSL_EVENT_LEN, tempBuff, FALSE);
		}
		else{
			eventlog_append_data(event->msg, NTSL_EVENT_LEN, "No description available", FALSE);
		}

		return(0);
	}

	/*--------------------------[ thisEventIsFilteredOut ]--------------------------
	* erno used to filter out events
	*
	*----------------------------------------------------------------------------*/
	bool thisEventIsFilteredOut(uint32 id) {

		int i=0;
		id= id & 0xffff; //want the generic event id, not source-specific

		while((i<MAXEVENTIDFILTERNUMBER)&&(filterArray[i]!=-1)) {
			if (id==filterArray[i])
				return true;
			i++;
		}
		return false;
	}

	/*--------------------------[ FindFirstReadPos ]--------------------------
	* Find approx start position in the event log to start finding entries from - dont want to start sequentially from the beginning
	*
	*	Returns:
	*		0		empty event log, or error ; just go from the beginnning
	*		value	record offset
	*----------------------------------------------------------------------------*/
	DWORD FindFirstReadPos(char *name,uint32 lastrun) {

		DWORD minPos=0;
		DWORD maxPos;
		DWORD probe;
		int status;
		//char                buffer[2048];
		uint32              bytes;
		uint32              next;
		//EVENTLOGRECORD      *record = (EVENTLOGRECORD *)&buffer;
		HANDLE hLog;

		if ((hLog = OpenEventLog(NULL, name)) == NULL) 
			return 0;

		//Find min
		status=ernoReadEventLog(hLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,0, &bytes, &next);
		if (status==0) {
			CloseEventLog(hLog); 
			return 0;
		}
		minPos=record->RecordNumber;

		CloseEventLog(hLog); 
		if ((hLog = OpenEventLog(NULL, name)) == NULL) 
			return 0;

		//Find min
		status=ernoReadEventLog(hLog, EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,0, &bytes, &next);
		if (status==0) {
			CloseEventLog(hLog); 
			return 0;
		}
		maxPos=record->RecordNumber;
		CloseEventLog(hLog); 

		probe=(DWORD)(minPos+0.9*(maxPos-minPos));

		while(true) {


			if ((maxPos-minPos)<200) {
				return minPos;
			}

			if ((hLog = OpenEventLog(NULL, name)) == NULL) 
				return 0;

			status=ernoReadEventLog(hLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEEK_READ,probe, &bytes, &next);
			CloseEventLog(hLog); 

			if (status==0) {
				return minPos;
			}

			if ( (record->TimeWritten >= lastrun)) {
				maxPos=record->RecordNumber;
			} else {
				minPos=record->RecordNumber;
			}
			probe=(DWORD)((minPos+maxPos)/2);

		}
		return 0;

	}
	/*--------------------------[ FreeBigBuffer ]--------------------------
	* Reduce size of buffer, if bigger than default size. 
	*
	*
	*----------------------------------------------------------------------------*/
	void FreeBigBuffer(){
		if (eventBufferSize>EVENTLOG_BUFFER_LEN) {
			free(eventBuffer);
			record=NULL;
			eventBuffer=NULL;
			eventBufferSize=0;
		}
	}
	/*--------------------------[ ernoReadEventLog ]--------------------------
	* Wrapper to ReadEventLogm which increases buffer size if needed 
	*
	* After this function call, record points to allocated structure. This function will handle any free operation needed, as record struct is static
	*
	*	Returns:
	*		error		0 - use GetLastError
	*		success		bytes read
	*----------------------------------------------------------------------------*/
	int ernoReadEventLog(HANDLE hLog,DWORD flags,DWORD offset, DWORD *BytesRead,DWORD *BytesNeeded) {
		int Status, lastError;
		unsigned int multiple;

		//Allocate standard buffer if needed
		if (eventBuffer==NULL) {
			eventBuffer=(char*)malloc(EVENTLOG_BUFFER_LEN); //Standard size
			if (eventBuffer==NULL) {
				logger(Error,"Failed to allocate standard buffer of %d bytes of memory.",EVENTLOG_BUFFER_LEN);
				return 0; //Error
			}
			eventBufferSize=EVENTLOG_BUFFER_LEN;
		}

		record=(EVENTLOGRECORD *)eventBuffer;

		//Allocate bigger buffer if needed
		//
		//Executes until big enough memory has been allocated and used, or function fails.
		while (true){
			Status=ReadEventLog(hLog, flags,offset, record, eventBufferSize, BytesRead, BytesNeeded);
			if (Status!=0)
				return Status; //We're done

			//Potential problem here
			lastError=GetLastError();
			switch(lastError) {
			case ERROR_INSUFFICIENT_BUFFER:
				free(eventBuffer);
				record=NULL;
				eventBuffer=NULL;
				eventBufferSize=0;
				multiple=2;
				while(EVENTLOG_BUFFER_LEN*multiple<(*BytesNeeded)) {
					multiple=multiple*2;
					if(EVENTLOG_BUFFER_LEN*multiple>3000000) {
						logger(Error,"SyslogAgent rejected suggested needed memory allocation of %d bytes from system to process message. Possible permanent error",*BytesNeeded);
						return 0;
					}
				}
				eventBuffer=(char*)malloc(EVENTLOG_BUFFER_LEN*multiple);
				if (eventBuffer==NULL) {
					logger(Error,"Failed to allocate %d bytes of memory.",EVENTLOG_BUFFER_LEN*multiple);
					return 0; //Error
				}
				record=(EVENTLOGRECORD *)eventBuffer;
				eventBufferSize=EVENTLOG_BUFFER_LEN*multiple;
				continue;
			case ERROR_INVALID_PARAMETER: //read past the end - no more to read
			case ERROR_HANDLE_EOF: //read empty event log - no problems
				*BytesRead=0;
				return 0;
			default:

				DEBUGSERVICE(Message,"ReadEventLog failed with error code %d.",lastError);

				return 0; //Do not want to spam recipient. A better error reporting is needed first...
			}
		}
		return Status;
	}
	/*--------------------------[ eventlog_read_events ]--------------------------
	* Read messages from the event log 
	*
	*	Returns:
	*		success		(0)
	*		error		(-1)	
	*----------------------------------------------------------------------------*/
	int eventlog_read_events(eventlog_data* eventlog, uint32 *lastrun,uint32 *thisrun){
		//char                buffer[EVENTLOG_BUFFER_LEN];
		uint32              bytes;
		uint32              next;
		//    EVENTLOGRECORD      *record = (EVENTLOGRECORD *)eventBuffer;
		HANDLE              hLog;
		int					status;
		DWORD				recordCounter=1; //used to indicate where to read next from eventlog. It does *not* neccessarily mean that post is to be added;eventlog->Recordoffset takes care of that.
		DWORD				startPos;
		ntsl_event	*event;
		char		*source;
		char		*computer;
		char		*strings;
		struct tm   _time,*time;
		bool ThisIsInitialLoop=true;

		__try {

			time=&_time;

LabelForResettedEventLog:

			if ( (lastrun == NULL) || (thisrun == NULL) || (service_halting()) )
				return(-1);

			if ((hLog = OpenEventLog(NULL, eventlog->name)) == NULL) {
				//ntsl_log_error(NTSL_ERROR_EVENT_LOG_ACCESS, eventlog->name);
				DEBUGSERVICE(Message,"Failed to open event log %s. Error: %s", eventlog->name,GetLastError());
				return(-1);
			}

			DEBUGPARSE(Header,"Reading %s event log starting at offset %u.", eventlog->name,eventlog->recordOffset);

			//find start position
			if (eventlog->recordOffset==0) {

				startPos=FindFirstReadPos(eventlog->name,*lastrun);
				if (startPos==0) { //no help
					status=ernoReadEventLog(hLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,0, &bytes, &next);
				} else {
					status=ernoReadEventLog(hLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEEK_READ,startPos, &bytes, &next);
				}
				if ((status==0)&& (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) { //totally empty eventlog, or error; eject
					CloseEventLog(hLog); 
					return(0);
				}
				eventlog->recordOffset=record->RecordNumber;
				eventlog->verificationTime=record->TimeGenerated;
				recordCounter=eventlog->recordOffset;
			} else {
				recordCounter=max(eventlog->recordOffset,0); //-1 so that a valid entry is read unless the enventlog has been wiped.
			}

			//big loop
			while ((status=ernoReadEventLog(hLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEEK_READ,recordCounter, &bytes, &next))){


				DEBUGPARSE(Message,"Read %u bytes at position %u with return status %d.",bytes,recordCounter, status);

				if ((ThisIsInitialLoop)&&(eventlog->verificationTime!=record->TimeGenerated)) { //same id as before, but new time! Start from scratch since eventlog has been rotated/deleted

					DEBUGPARSE(Message,"Verification time mismatch! EventLog cleared? starting from the beginning.");

					eventlog->recordOffset=0;
					ThisIsInitialLoop=false;
					CloseEventLog(hLog); 
					goto LabelForResettedEventLog; //erno only solution to a complete rewrite...(?)
				}

				ThisIsInitialLoop=false; //Read successful, hence set to false for the future

				if (service_halting())
					return(-1);

				while (bytes > 0) { 

					/* If this event happened between the last time we ran, and the current
					time, and it is one we are interested in, then fill in an ntsl_event
					structure, and pass it to the engine to log. */
					//If match with registry list of filtered event id's, ignore.

					//record->EventID &=0x0000FFFF; Dum, dum idé. Utan unika, högre delarena blir matchningen mellan id och dll'ens id inte unika--> problem.

					if ( (record->TimeWritten >=  (uint32)*lastrun) && 
						(record->TimeWritten < (uint32)*thisrun) &&
						(eventlog_check_event(eventlog, record->EventType)) &&
						(thisEventIsFilteredOut(record->EventID)==false)) {

							source   = (LPSTR) ((LPBYTE) record + sizeof(EVENTLOGRECORD));
							computer = source + strlen(source) + 1;
							strings  = (LPSTR) ((LPBYTE) record + record->StringOffset);
							__time64_t _TimeGenerated=record->TimeGenerated;

							localtime_s(time,&_TimeGenerated);		

							//DEBUGPARSE(Message,"Nasta startposition %u. Record %u adderades. lastrun: %u thisrun: %u TimeWritten: %u",eventlog->recordOffset,record->RecordNumber,(uint32)*lastrun,(uint32)*thisrun,record->TimeWritten);

							if ( (event = (ntsl_event*)malloc0(sizeof(ntsl_event))) == NULL){
								ntsl_log_error(NTSL_ERROR_EVENT_MALLOC);
								return(-1);
							}	

							event->msg[0] = 0;
							//erno2005
							strcpy_s(event->facilityName,sizeof(event->facilityName),eventlog->name);
							event->time1970format=record->TimeGenerated;

							strftime(event->date, NTSL_DATE_LEN, "%b %d %H:%M:%S", time);
							if (event->date[4] == '0') // Unix style formatting
								event->date[4] = ' ';
							strcpy_s(event->host, eventlog_lcase(computer));
							strcpy_s(event->source, eventlog_lcase(source));
							eventlog_set_event_type(event, record->EventType);
							eventlog_set_event_priority(event, record->EventType, eventlog);
							eventlog_set_event_msg(event, eventlog->name, record->EventID, strings, record->NumStrings);
							eventlog_set_user(event, ((LPBYTE) record + record->UserSidOffset), record->UserSidLength);


							DEBUGPARSE(Message,"Sending parsed record %u to output: %s",record->RecordNumber, event->msg);

							engine_process_event(event);
							event = NULL;
					} else {
						//DEBUGPARSE(Message,"Next start position %u. Record %u ignored. lastrun: %u thisrun: %u TimeWritten: %u",eventlog->recordOffset,record->RecordNumber,(uint32)*lastrun,(uint32)*thisrun,record->TimeWritten);
					}

					bytes -= record->Length; 

					//DEBUGPARSE(Message,"Bytes remaining in buffer to read is %u.",bytes);

					if (record->TimeWritten < (uint32)*thisrun) {
						eventlog->recordOffset=record->RecordNumber;
						eventlog->verificationTime=record->TimeGenerated;
					}
					record = (EVENTLOGRECORD *) ((LPBYTE) record + record->Length); 
					recordCounter++;
				} //End loop parse received messages in buffer

				DEBUGPARSE(EndHeader,"");
				//record = (EVENTLOGRECORD *)eventBuffer; 
			}  //end big while loop

			if ((status==0)&&(ThisIsInitialLoop)) { //this only happens if an entry has been removed, i.e. deleted from eventLog. Start from scratch.

				DEBUGPARSE(Message,"Id not found! EventLog cleared? starting from the beginning.");
				eventlog->recordOffset=0;
				ThisIsInitialLoop=false;
				CloseEventLog(hLog); 
				goto LabelForResettedEventLog; //erno only solution to a complete rewrite...(?)
			}


		}

		__except( CreateMiniDump( GetExceptionInformation() ), EXCEPTION_EXECUTE_HANDLER ) {
			logger(Error,"Exception when reading events from %s! Will reattempt reading. Current or previous event text is:%s", eventlog->name, event->msg);
			Sleep(3000);
		}

		FreeBigBuffer();


		CloseEventLog(hLog); 
		return(0);
	}


	/*-------------------------[ eventlog_read_lastrun ]---------------------------
	*  Reads last run times
	*
	*	Returns:
	*		success		(0)
	*		error		(-1)	
	*----------------------------------------------------------------------------*/
	static int eventlog_read_lastrun(uint32 *lastrun)
	{
		char        buffer[REG_BUFFER_LEN];
		HKEY        hReg;
		int32		size = sizeof(*lastrun);
		int32       rc, rv;

		if (lastrun == NULL)
			return(-1);

		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
		if (rc != ERROR_SUCCESS)
			return(-1);

		rv  = RegQueryValueEx(hReg, LAST_RUN_REG, NULL, NULL, (unsigned char*)lastrun, (LPDWORD)&size);
		RegCloseKey(hReg);

		if ( (rv != ERROR_SUCCESS) || (*lastrun <0) )
		{
			*lastrun = 0;
		}	

		return(0);
	}


	/*--------------------------[ eventlog_write_lastrun ]-----------------------
	*  Writes last run time
	*
	*	Returns:
	*		success		(0)
	*		error		(-1)	
	*----------------------------------------------------------------------------*/
	int eventlog_write_lastrun(uint32 *lastrun){
		char        buffer[REG_BUFFER_LEN];
		HKEY        hReg;
		int32		size = sizeof(*lastrun);
		int32       rc, rv;

		if (lastrun == NULL)
			return(-1);

		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_SET_VALUE, &hReg);
		if (rc != ERROR_SUCCESS)
			return(-1);

		rv  = RegSetValueEx(hReg, LAST_RUN_REG, 0, REG_DWORD, (unsigned char*)lastrun, size);
		RegCloseKey(hReg);

		if (rv != ERROR_SUCCESS)
			ntsl_die(NTSL_ERROR_TIME_DATA_WRITE, LAST_RUN_REG);

		return(0);
	}


	/*--------------------------[ eventlog_check_events ]--------------------------
	* Locate recent event entries to be processed
	*----------------------------------------------------------------------------*/
	void eventlog_check_events(){
		uint32	lastrun;
		__int64 _thisrun;

		time((time_t*)&_thisrun);
		thisrun=(uint32)_thisrun;

		if (lastunload+20<thisrun) {//every 20 seconds, unload all libraries (used to be *every* entry)
			lastunload=thisrun;
			ernoUnloadLoadLibrary();
		}

		if (eventlog_read_lastrun(&lastrun) != 0)	return; //failed to read registry
		if (lastrun > 0){
			ListIterator* i = list_iterator(eventlog_list);
			if (i){
				eventlog_data* log = (eventlog_data*)list_first(i);
				DEBUGPARSE(Header,"");
				while(log != NULL) {
					if (log->captureFlags != EVENTLOG_NO_FLAGS)
						eventlog_read_events(log, &lastrun, &thisrun);
					//spara recordOffset
					log = (eventlog_data*)list_next(i);
				}
				DEBUGPARSE(EndHeader,"");
				i = list_iterator_delete(i);
			}
		}

		ReadMoreFromRegistry=false; //await output process to clear
		//	if ((PingOk)|(lastrun==0))  //erno added ping condition
		//		eventlog_write_lastrun(&thisrun);

	}
	/*----------------------------[ eventlog_read_reg ]----------------------------
	* Read eventlog registry settings 
	*----------------------------------------------------------------------------*/
	static void eventlog_read_reg(eventlog_data* eventlog)
	{
		char        buffer[REG_BUFFER_LEN];
		int			val;
		HKEY        hReg;
		int32		size = sizeof(val);
		int32       rc, rv;

		if (eventlog){
			// Set up the structure with clean defaults.
			eventlog->captureFlags = EVENTLOG_NO_FLAGS;       

			eventlog->successPriority = EVENTLOG_DEFAULT_PRIORITY;
			eventlog->informationPriority = EVENTLOG_DEFAULT_PRIORITY;
			eventlog->warningPriority = EVENTLOG_DEFAULT_PRIORITY;
			eventlog->errorPriority = EVENTLOG_DEFAULT_PRIORITY;
			eventlog->auditSuccessPriority = EVENTLOG_DEFAULT_PRIORITY;
			eventlog->auditFailurePriority = EVENTLOG_DEFAULT_PRIORITY;

			sprintf_s(buffer, "SOFTWARE\\%s\\%s", DEV_NAME, eventlog->name);
			rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
			if (rc == ERROR_SUCCESS)
			{
				// Read "Information" flag from registry.
				rv  = RegQueryValueEx(hReg, "Information", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val) )
					eventlog->captureFlags |= EVENTLOG_INFORMATION_FLAG;

				// Read "Information Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Information Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val > 0) )
					eventlog->informationPriority = val;

				// Read "Success" flag from registry.
				rv  = RegQueryValueEx(hReg, "Success", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if (rv == ERROR_SUCCESS) {
					if (val)
						eventlog->captureFlags |= EVENTLOG_SUCCESS_FLAG;
				} else //if no info in registry - since it was added first 2008-02, add it.
					eventlog->captureFlags |= EVENTLOG_SUCCESS_FLAG;

				// Read "Success Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Success Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if (rv == ERROR_SUCCESS) {
					if (val > 0)
						eventlog->successPriority = val;
				} else
						eventlog->successPriority = 190; //information, dafault

				// Read "Warning" flag from registry.
				rv  = RegQueryValueEx(hReg, "Warning", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val) )
					eventlog->captureFlags |= EVENTLOG_WARNING_FLAG;

				// Read "Warning Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Warning Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val > 0) )
					eventlog->warningPriority = val;

				// Read "Error" flag from registry.
				rv  = RegQueryValueEx(hReg, "Error", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val) )
					eventlog->captureFlags |= EVENTLOG_ERROR_FLAG;

				// Read "Error Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Error Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val > 0) )
					eventlog->errorPriority = val;

				// Read "Audit Success" flag from registry.
				rv  = RegQueryValueEx(hReg, "Audit Success", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val) )
					eventlog->captureFlags |= EVENTLOG_AUDIT_SUCCESS_FLAG;

				// Read "Audit Success Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Audit Success Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val > 0) )
					eventlog->auditSuccessPriority = val;

				// Read "Audit Failure" flag from registry.
				rv  = RegQueryValueEx(hReg, "Audit Failure", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val) )
					eventlog->captureFlags |= EVENTLOG_AUDIT_FAILURE_FLAG;

				// Read "Audit Failure Priority" from registry.
				rv  = RegQueryValueEx(hReg, "Audit Failure Priority", NULL, NULL, (unsigned char*)&val, (LPDWORD)&size);
				if ( (rv == ERROR_SUCCESS) && (val > 0) )
					eventlog->auditFailurePriority = val;

				RegCloseKey(hReg);
			}
			else
			{
				ntsl_die(NTSL_ERROR_CONFIG_READ, buffer);
			}
		}
	}

	//*******************************************************************
	int eventlog_init(){
		int i;
		char        buffer[REG_BUFFER_LEN];
		HKEY        hReg;
		int32		size = sizeof(LookupAccountSID);
		int32		sizeReplacementChar = sizeof(CRReplacementChar);
		int32       rc, rv;

		//erno init filter
		for(i=0;i<MAXEVENTIDFILTERNUMBER;i++) {
			filterArray[i]=-1; //well, it's unsigned, but it works...
		}
		initFilterEventArray((uint32*)&(filterArray[0]));


		//eventBuffer=(char*)malloc(EVENTLOG_BUFFER_LEN);
		record=NULL;
		eventBuffer=NULL;

		lastunload=0; //10 sec timer to regularly unload libraries

		for(i=0;i<MAXLOADEDLIBRARIES;i++) {
			LoadedLibrary[i].name[0]='\0';
		}

		for(i=0;i<MAXLOADEDSIDS;i++) {
			LoadedSIDS[i].valid=false;
		}

		//Read LookupAccountSID
		LookupAccountSID=true;
		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
		if (rc == ERROR_SUCCESS) {
			rv  = RegQueryValueEx(hReg, LOOKUPACCOUNTSID, NULL, NULL, (unsigned char*)&LookupAccountSID, (LPDWORD)&size);
			RegCloseKey(hReg);
		}

		//Read CarrigeReturnReplacementCharInASCII
		CRReplacementChar=127; //default
		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
		if (rc == ERROR_SUCCESS) {
			rv  = RegQueryValueEx(hReg, CARRIGERETURNREPLACEMENTCHARINASCII, NULL, NULL, (unsigned char*)&CRReplacementChar, (LPDWORD)&sizeReplacementChar);
			RegCloseKey(hReg);
		}

		//Read LineFeedReplacementCharInASCII
		LFReplacementChar=0; //default
		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
		if (rc == ERROR_SUCCESS) {
			rv  = RegQueryValueEx(hReg, LINEFEEDRETURNREPLACEMENTCHARINASCII, NULL, NULL, (unsigned char*)&LFReplacementChar, (LPDWORD)&sizeReplacementChar);
			RegCloseKey(hReg);
		}

		//Read TabReplacementCharInASCII
		TABReplacementChar=0; //default
		sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
		if (rc == ERROR_SUCCESS) {
			rv  = RegQueryValueEx(hReg, TABREPLACEMENTCHARINASCII, NULL, NULL, (unsigned char*)&TABReplacementChar, (LPDWORD)&sizeReplacementChar);
			RegCloseKey(hReg);
		}


		//Prepare list
		eventlog_list = list_new();

		if (eventlog_list != NULL)
		{
			char        buffer[REG_BUFFER_LEN];
			HKEY        hReg;
			int32       rc;

			sprintf_s(buffer, "SOFTWARE\\%s", DEV_NAME);
			rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hReg);
			if (rc == ERROR_SUCCESS)
			{
				int32		i = 0;

				while (RegEnumKey(hReg, i++, buffer, REG_BUFFER_LEN) == ERROR_SUCCESS){
					if (strstr(buffer,"ApplicationLogs")==NULL) {
						list_add(eventlog_list, eventlog_new_log(buffer));
					}
				}

				RegCloseKey(hReg);
			}

		}

		return(eventlog_list != NULL);
	}

	void eventlog_shutdown()
	{
		eventlog_list = list_delete(eventlog_list);	// free list contents
	}	

	/*--------------------------[ eventlog_parse_libs ]--------------------------
	* Parse different libraries from string 
	*
	*	Returns:
	*		success		(0)
	*		error		(-1)	
	*----------------------------------------------------------------------------*/
	int eventlog_parse_libs(char *dllIn, char *dllOut, uint32 sequence)
	{
		int start = 0;
		int target = 0;

		while (sequence > 0 && dllIn[start] != '\0')
		{
			if (dllIn[start] == ';')
				sequence--;
			start++;
		}

		if (dllIn[start] == '\0')
			return -1;

		while (dllIn[start] != '\0' && dllIn[start] != ';')
		{
			dllOut[target] = dllIn[start];
			target++;
			start++;
		}
		dllOut[target] = '\0';
		return 0;
	}

	/*----------------------------[ eventlog_new_log ]----------------------------
	* Obtains the user name associated with a event 
	*
	*	Returns:
	*		success		(0)
	*		error		(-1)	
	*----------------------------------------------------------------------------*/
	static int eventlog_set_user(ntsl_event *event, PSID sid, DWORD len){
		DWORD			us = NTSL_SYS_LEN - 1;
		DWORD			ds = NTSL_SYS_LEN - 1;
		SID_NAME_USE	pe;
		int i=0;

		if (event == NULL) return -1;
		if (len<1) return -1;

		event->user[0] = 0;
		event->domain[0] = 0;

		if (!LookupAccountSID) //no lookup at all
			return 0;

		//See if it is already in list
		while ((i<MAXLOADEDSIDS)&&(LoadedSIDS[i].valid==true)) {
			if (EqualSid(sid,(PSID)(LoadedSIDS[i].SID))!=0) { //hit
				strncpy_s(event->user,sizeof(event->user),LoadedSIDS[i].user,sizeof(LoadedSIDS[i].user));
				strncpy_s(event->domain,sizeof(event->domain),LoadedSIDS[i].domain,sizeof(LoadedSIDS[i].domain));
				return 0;		
			}
			i++;
		}

		if (i==MAXLOADEDSIDS) { //full list, and no match
			ernoUnloadLoadSIDS();
			i=0;
		}

		//Not found
		if (!(LookupAccountSid(NULL, sid, LoadedSIDS[i].user, &us, LoadedSIDS[i].domain, &ds, &pe)))
			return -1; //fail to lookup
		LoadedSIDS[i].valid=true;
		CopySid(sizeof(LoadedSIDS[0].SID),(PSID)(LoadedSIDS[i].SID),sid);
		strncpy_s(event->user,sizeof(event->user),LoadedSIDS[i].user,sizeof(LoadedSIDS[i].user));
		strncpy_s(event->domain,sizeof(event->domain),LoadedSIDS[i].domain,sizeof(LoadedSIDS[i].domain));		
		return 0;
	}

	/*----------------------------[ eventlog_new_log ]----------------------------
	* Mallocs new eventlog entry 
	*
	*	Returns:
	*		success		(!NULL)
	*		error		(NULL)	
	*----------------------------------------------------------------------------*/
	static eventlog_data* eventlog_new_log(const char* name)
	{
		eventlog_data*	data = NULL;

		if (name != NULL)
		{
			data = (eventlog_data*)malloc(sizeof(eventlog_data));

			if (data != NULL)
			{
				strcpy_s(data->name, name);
				data->recordOffset=0; //erno
				data->verificationTime=0; //erno
				eventlog_read_reg(data);

			}
		}

		return data;
	}


} //end extern "C" by erno aug04


#include "..\Syslogserver\common_stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "..\Syslogserver\common_registry.h"
#include "errorHandling.h"
extern "C" {
	#include "..\Syslogagent\service.h"

extern DebugFlagsDef DebugFlags;

}


#pragma comment ( lib, "dbghelp.lib" )

//Global variable
bool MemoryCallbackCalled = false; 

extern void logger(int severity,char *text,...);


long WINAPI top_level_exception_filter(_EXCEPTION_POINTERS *exceptioninfo) {

	CreateMiniDump(exceptioninfo);
	logger(Error,"Exception! Top level exception handler called. Writing dump file and rethrowing exception!");
	return EXCEPTION_CONTINUE_SEARCH;  //We did not handle the problem.

	//return EXCEPTION_CONTINUE_EXECUTION means we handled the problem.
}

void CreateMiniDump( EXCEPTION_POINTERS* pep ) 
{
	// Open the file 

	HANDLE hFile = CreateFile( _T(DebugFlags.DebugDumpFilePath), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL ); //Namn förekommer på flera ställen!

	if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) ) 
	{
		// Create the minidump 

		MINIDUMP_EXCEPTION_INFORMATION mdei; 

		mdei.ThreadId           = GetCurrentThreadId(); 
		mdei.ExceptionPointers  = pep; 
		mdei.ClientPointers     = FALSE; 

		MINIDUMP_CALLBACK_INFORMATION mci; 

		mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback; 
		mci.CallbackParam       = 0; 

		//original MINIDUMP_TYPE mdt       = MiniDumpNormal; 
		MINIDUMP_TYPE mdt       = MiniDumpWithIndirectlyReferencedMemory; 
		
		BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), 
			hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci ); 

		if( !rv ) 
			logger(Error,"MiniDumpWriteDump failed. Error: %u \n", GetLastError() ); 
			//_tprintf( _T("MiniDumpWriteDump failed. Error: %u \n"), GetLastError() ); 
		else 
			logger(Informational,"Minidump file SyslogAgentCrashInfo.dmp created in the syslogAgent directory. Start service with parameters -DLOGGER -DSERVICE using administrative tools->services->SyslogAgent interface, and inspect debug file in the SyslogAgent directory. -DPARSE and -DAPPLPARSE for event and application log parsing is also available, but very verbose. Please send debug file and SyslogAgentCrashInfo.dmp to info@syslogserver.com."); 

		// Close the file 

		CloseHandle( hFile ); 

	}
	else 
	{
		logger(Informational,"Dump file already exists."); 
	}

}


///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback 
//

BOOL CALLBACK MyMiniDumpCallback(
	PVOID                            pParam, 
	const PMINIDUMP_CALLBACK_INPUT   pInput, 
	PMINIDUMP_CALLBACK_OUTPUT        pOutput 
) 
{
	BOOL bRet = FALSE; 


	// Check parameters 

	if( pInput == 0 ) 
		return FALSE; 

	if( pOutput == 0 ) 
		return FALSE; 


	// Process callbacks 

	switch( pInput->CallbackType ) 
	{
		case IncludeModuleCallback: 
		{
			// Include the module into the dump 
			_tprintf( _T("IncludeModuleCallback (module: %08I64x) \n"), 
				pInput->IncludeModule.BaseOfImage); 
			bRet = TRUE; 
		}
		break; 

		case IncludeThreadCallback: 
		{
			// Include the thread into the dump 
			_tprintf( _T("IncludeThreadCallback (thread: %x) \n"), 
				pInput->IncludeThread.ThreadId); 
			bRet = TRUE; 
		}
		break; 

		case ModuleCallback: 
		{
			// Include all available information 
			wprintf( L"ModuleCallback (module: %s) \n", pInput->Module.FullPath ); 
			bRet = TRUE; 
		}
		break; 

		case ThreadCallback: 
		{
			// Include all available information 
			_tprintf( _T("ThreadCallback (thread: %x) \n"), pInput->Thread.ThreadId ); 
			bRet = TRUE;  
		}
		break; 

		case ThreadExCallback: 
		{
			// Include all available information 
			_tprintf( _T("ThreadExCallback (thread: %x) \n"), pInput->ThreadEx.ThreadId ); 
			bRet = TRUE;  
		}
		break; 

		case MemoryCallback: 
		{
			// Let CancelCallback know where to stop 
			MemoryCallbackCalled = true; 

			// We do not include any information here -> return FALSE 
			_tprintf( _T("MemoryCallback\n") ); 
			bRet = FALSE; 
		}
		break; 

		/* no longer valid option? erno 08-01
		case CancelCallback: 
		{
			_tprintf( _T("CancelCallback\n") ); 

			if( !MemoryCallbackCalled) 
			{
				// Continue receiving CancelCallback callbacks 
				pOutput->Cancel       = FALSE; 
				pOutput->CheckCancel  = TRUE; 
			}
			else 
			{
				// No cancel callbacks anymore 
				pOutput->Cancel       = FALSE; 
				pOutput->CheckCancel  = FALSE; 
			}
			bRet = TRUE; 
		}
		break; 
		*/
	}


	return bRet; 

}



//Called by _set_se_translator
void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
    logger(Error, "SEH Exception, caught in trans_func by _set_se_translator. writing dump file and rethrowing exception.");
	CreateMiniDump(pExp);
    throw SE_Exception(u);
}

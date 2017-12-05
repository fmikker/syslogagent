#pragma once


// http://www.debuginfo.com/articles/effminidumps.html
#include <dbghelp.h>
#include <crtdbg.h>

//erno 08-01
extern "C" {

void CreateMiniDump( EXCEPTION_POINTERS* pep ); 
long WINAPI top_level_exception_filter(_EXCEPTION_POINTERS *exceptioninfo);

BOOL CALLBACK MyMiniDumpCallback(
	PVOID                            pParam, 
	const PMINIDUMP_CALLBACK_INPUT   pInput, 
	PMINIDUMP_CALLBACK_OUTPUT        pOutput 
); 

}

void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp );


//Yes, code in a header file...
class SE_Exception
{
private:
    unsigned int nSE;
public:
    SE_Exception() {}
    SE_Exception( unsigned int n ) : nSE( n ) {}
    ~SE_Exception() {}
    unsigned int getSeNumber() { return nSE; }
};

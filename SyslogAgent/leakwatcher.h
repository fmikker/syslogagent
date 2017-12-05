#ifndef IMWATCHINGYOULEAK
#define IMWATCHINGYOULEAK

#include <crtdbg.h>

#ifdef _DEBUG
//void* operator new(size_t nSize, const char * lpszFileName, int nLine)
//{
//   return ::operator new(nSize, 1, lpszFileName, nLine);
//}
extern void* operator new(size_t nSize, const char * lpszFileName, int nLine);
#define DEBUG_NEW new(THIS_FILE, __LINE__)

#define MALLOC_DBG(x) _malloc_dbg(x, 1, THIS_FILE, __LINE__);
#define malloc(x) MALLOC_DBG(x)

#endif // _DEBUG

#endif // #include guard

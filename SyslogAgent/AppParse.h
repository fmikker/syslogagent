#pragma once

#include "..\Syslogserver\common_registry.h"

extern "C" {
	void GetOwnIP();
}


void parseFieldCodes(applSettings *,char *);
int getLine(FILE *fp,bool unicode,char *buf,int maxNbr);
int cleanInput(unsigned	char *buf,unsigned char *buf2,int numbytes);
CString getLatestLogFileName(CString logPath,CString fileExtension, CString SpecificFile);
int parseMessage(unsigned char*,unsigned char*,applSettings*);
bool testUnicode(char *filename);
char ernogetc(FILE *fp,bool unicode);


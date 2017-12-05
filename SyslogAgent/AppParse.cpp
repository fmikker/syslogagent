#include "..\Syslogserver\common_stdafx.h"
#include "winsock2.h"
#include "..\Syslogserver\common_registry.h"
#include "AppParse.h"
#include "ErrorHandling.h"
extern "C" {
	#include "..\Syslogagent\service.h"
}

char applHostName[128];
char applHostIP[128];

extern int GMTimeDiffInHours;


//for unicode macros
//    int _convert; (_convert); UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; (_acp); LPCWSTR _lpw; (_lpw); LPCSTR _lpa; (_lpa);
int _convert = 0; 
//(_convert); 
UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; 
//(_acp); 
LPCWSTR _lpw = NULL; 
//(_lpw); 
LPCSTR _lpa = NULL; 
//(_lpa);

/***********************************************************************
*
************************************************************************/
char* facilityName(int a) {
	switch (a) {
		case 0:
			return "Kernel";
			break;
		case 1:
			return "User";
			break;
		case 2:
			return "Mail";
			break;
		case 3:
			return "System";
			break;
		case 4:
			return "Security";
			break;
		case 5:
			return "Syslogd";
			break;
		case 6:
			return "Printer";
			break;
		case 7:
			return "Network";
			break;
		case 8:
			return "UUCP";
			break;
		case 9:
			return "Clock";
			break;
		case 10:
			return "Security";
			break;
		case 11:
			return "FTP";
			break;
		case 12:
			return "NTP";
			break;
		case 13:
			return "Log audit";
			break;
		case 14:
			return "Log alert";
			break;
		case 15:
			return "Clock";
			break;
		case 16:
			return "Local0";
			break;
		case 17:
			return "Local1";
			break;
		case 18:
			return "Local2";
			break;
		case 19:
			return "Local3";
			break;
		case 20:
			return "Local4";
			break;
		case 21:
			return "Local5";
			break;
		case 22:
			return "Local6";
			break;
		case 23:
			return "Local7";
			break;
		default:
			return "facility";
			break;
	}
}

/********************************************************
********************************************************/
bool testUnicode(char *filename) {
	char a,b;
	FILE *test=NULL;

	try {
		test= _wfsopen(T2W(filename),T2W("rb"), _SH_DENYNO);
		if(test==NULL ){
			logger(Error,"Failed to open input file %s to inspect unicode status. Error code %d. Assuming non unicode. Problem with log file access?",filename, errno);
			return false;
		}

		a=getc(test);  //NOT getwc!
		b=getc(test);

		// Empty file produces -1 and -1 (EOF twice)
		if ((a==-1)&(b==-2)) {  //actually 0xFF 0xFE
			//is unicode
			fclose(test);
			test=0;
			DEBUGAPPLPARSE(Informational,"Log file %s is in unicode format.",filename);
			return true;
		} else {
			fclose(test);
			test=0;
			DEBUGAPPLPARSE(Informational,"Log file %s is not unicode format.",filename);
			return false;
		}
	}
	catch(SE_Exception e) {
		logger(Error,"SEH Exception in testUnicode for log file %s. Assuming non unicode log file. Error code %u.",filename,e.getSeNumber());
	}
	catch(...) {
		CreateMiniDump(NULL);
		logger(Error, "Exception when detecting unicode status on log file %s. Dump file written.",filename);
		if (test!=NULL)
		fclose(test);
	}
	return false;

}
/********************************************************
ernogetc --	wrapper for widechar and char getc
********************************************************/
char ernogetc(FILE *fp,bool unicode) {
	char a;

	try{
		if (unicode) {
			a=getwc(fp);
			//since a is not wide, it's automatically cleaned
		} else {
			a=getc(fp);
		}
		return a;
	}
	catch(...) { //is it a wide char file?
	}

}
/********************************************************
getLine  --	getline from FILE *
********************************************************/
int getLine(FILE *fp,bool unicode,char *buf,int maxNbr) {
	char *startptr=buf,*endPtr=buf+maxNbr-1;

	while ((*buf) = ernogetc(fp,unicode))  {

		if (*buf==(char)-1) { //EOF
			break;
		}

		if (buf==endPtr) {
			*buf='\0';
			break;
		}
		if ((*buf)==(char)13) {
			*buf='\0';
		} else if (((*buf)==(char)10)||((*buf)=='\0')) {
			*buf='\0';
			buf++; //so that end calculation is right.
			break;
		}
		buf++;
	}
	return (buf-startptr);
}
/****************************************************************************
* cleanInput --	replace	unwanted chars
*
****************************************************************************/
int cleanInput(unsigned	char *buf,unsigned char *buf2,int numbytes)	{
	int numbytes2=0;

	buf[numbytes] =	'\0';

	//Eat final cr/lf. calculates new numbytes too.

	while (((numbytes>1)&&((buf[numbytes-1]==(char)10)||(buf[numbytes-1]==(char)13)))) 
		buf[--numbytes]='\0';

	while(*buf!='\0') {

		if (*buf>'%') {	
			*buf2=*buf;
		} else {  //buf	=< %
			if (*buf=='%') { 
				*buf2++='%';
				*buf2=*buf;
				numbytes2++;
			} else {
				if (*buf>(unsigned char)31)	{ 
					*buf2=*buf;
				} else {
					if (*buf==(unsigned char)9)	{ 
						*buf2=' ';
					} else {
						*buf2='#';
					}
				}
			}
		}
		buf++;buf2++;
	}

	*buf2='\0';
	return numbytes2+numbytes; //return total chars, i.e. original+addendum due	to esc chars
}
/********************************************************
getLatestLogFileName --	find file with right extension and latest *creation* time stamp
// Last written timestamp not good enough, since depending on OS(!) and handle handling, write timestamp may not be updated until closing of file...
********************************************************/
CString getLatestLogFileName(CString logPath,CString fileExtension, CString SpecificFile) {

	CString fileName;
	CFileFind finder;
	CString candidate;
	CTime candidateCreationTime,fileNameCreationTime;
	char singleChar;

	//Check if a specific file is configured.If so, no work to be done
	if (SpecificFile!="") {
		return SpecificFile;
	}

	//	build a string with wildcards
	CString	strWildcard(logPath);
	strWildcard	+= _T("\\*");

	if (fileExtension!="") {
		singleChar=fileExtension.GetAt(0);
		if (singleChar!='.')	{
			strWildcard += _T(".");
		}
		strWildcard	+= _T(fileExtension);
	}

	// start searching for files
	BOOL bWorking = finder.FindFile(strWildcard);

	while (bWorking) {

		bWorking = finder.FindNextFile(); //must be called according to spec before finding any file..

		// skip .	and	.. files; otherwise, we'd recur infinitely!
		if (finder.IsDots())
			continue;

		//Get file info
		candidate=finder.GetFileName();
		finder.GetCreationTime(candidateCreationTime);

		if ((fileName=="")||(candidateCreationTime>fileNameCreationTime)) {
			fileName=candidate;
			fileNameCreationTime=candidateCreationTime;
		}
	}

	if (fileName=="") return fileName; //If no valid file at all.
	fileName=logPath + "\\" + fileName;
	return fileName;
}
/****************************************************************************
* GetOwnIP
*
****************************************************************************/
void GetOwnIP() {
	int j;
	size_t aSize;
	struct hostent * pHost;
	WSADATA wsdata;
	WORD wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup( wVersionRequested, &wsdata );
	char aName[256];
	char *namePtr=&aName[0];
	getenv_s(&aSize,namePtr,256,"COMPUTERNAME");
	CString temp=namePtr;
	temp.MakeLower();
	strcpy_s(applHostName,temp);

	pHost = gethostbyname(applHostName);
	if((pHost!=NULL)&&(pHost->h_addr_list[0]!=NULL)) {
		//original went through al aliases  for( i = 0; pHost!= NULL && pHost->h_addr_list[i]!= NULL; i++ )	{
		CString str,addr;
		for( j = 0; j < pHost->h_length; j++ ){
			if( j > 0 )
				str += ".";
			addr.Format("%u", (unsigned int)((unsigned char*)pHost->h_addr_list[0])[j]);
			str += addr;
		}
		strcpy_s(applHostIP,str);  //Set global variable ownIP
	} else {
		strcpy_s(applHostIP,"127.0.0.1");  //What other option do we have..
		logger(Error,"Failed to obtain Host IP address. Using 127.0.0.1.");
	}

	WSACleanup();

}
/***********************************************************************
*
************************************************************************/
bool identifyDate(unsigned char* input,CString *date) {
	int year,month,day;
	char temp[3];
	bool status=false;

	do {
		if (sscanf_s((char*)input,"%2d-%2d-%2d",&year,&month,&day)==3) {
			status=true;
			break;
		}
		if (sscanf_s((char*)input,"20%2d-%2d-%2d",&year,&month,&day)==3) {
			status=true;
			break;
		}
	} while(false);

	if(!status) return false;

	switch(month) {
		case 1: *date="Jan ";
			break;
		case 2: *date="Feb ";
			break;
		case 3: *date="Mar ";
			break;
		case 4: *date="Apr ";
			break;
		case 5: *date="May ";
			break;
		case 6: *date="Jun ";
			break;
		case 7: *date="Jul ";
			break;
		case 8: *date="Aug ";
			break;
		case 9: *date="Sep ";
			break;
		case 10: *date="Oct ";
			break;
		case 11: *date="Nov ";
			break;
		case 12: *date="Dec ";
			break;
		default:
			return false;
	}

	_snprintf_s(temp,3,"%d",day);	
	(*date)+=temp;
	return true;
}

/***********************************************************************
*
************************************************************************/
bool identifyTime(unsigned char* input,CString *time) {
	int hour,minute,second;
	char temp[9];
	int DaLength=strlen((char*)input);

	//hmmm, tid kan vara :  2004-10-15	14:14:24+0200	etc

	if((DaLength<8)|(DaLength>13)) { // must be xx:xx:xx+xxxx
		return false;
	}

	if (sscanf_s((char*)input,"%2d:%2d:%2d",&hour,&minute,&second)==3) {
		sprintf_s(temp,"%02d:%02d:%02d",hour,minute,second);
		*time=temp;
		return true;
	}

	return false;
}
/***********************************************************************
*
************************************************************************/
bool identifyHost(unsigned char* input,CString *host) {
	char *ptr;

	ptr=strstr((char*)input,applHostName);
	if(ptr!=NULL) {
		if ((strlen((char*)input)-strlen(applHostName))<5) { //accept some extra chars, such as :[] etc, but not many..
			*host=applHostName;
			return true;
		}
	}

	ptr=strstr((char*)input,applHostIP);
	if(ptr!=NULL) {
		if ((strlen((char*)input)-strlen(applHostIP))<5) { //accept some extra chars, such as :[] etc, but not many..
			*host=applHostName;
			return true;
		}
	}
	return false;
}
/***********************************************************************
*
************************************************************************/
bool identifyProcess(unsigned char* input,CString *process) {
	char temp[64];
	if (sscanf_s((char*)input,"%63[a-zA-Z0-9-_]",&(temp[0]),sizeof(temp))==1) {
		if(strlen(temp)>3) {
			*process=temp;
			return true;
		}
	}
	return false;
}
/***********************************************************************
* getmonthnumerical -- transform Jan to 1, Feb to 2 etc
*
************************************************************************/
int getmonthnumerical(char *month) {
	if(strcmp(month,"Jan")==0) {
		return 1;
	} else if(strcmp(month,"Feb")==0) {
		return 2;
	} else if(strcmp(month,"Mar")==0) {
		return 3;
	} else if(strcmp(month,"Apr")==0) {
		return 4;
	} else if(strcmp(month,"May")==0) {
		return 5;
	} else if(strcmp(month,"Jun")==0) {
		return 6;
	} else if(strcmp(month,"Jul")==0) {
		return 7;
	} else if(strcmp(month,"Aug")==0) {
		return 8;
	} else if(strcmp(month,"Sep")==0) {
		return 9;
	} else if(strcmp(month,"Oct")==0) {
		return 10;
	} else if(strcmp(month,"Nov")==0) {
		return 11;
	} else if(strcmp(month,"Dec")==0) {
		return 12;
	} else {
		return 0;
	}
}
/********************************************************
parseFieldCodes -- parse IIS and others field codes 

#Fields: time c-ip cs-method cs-uri-stem sc-status 

********************************************************/
void parseFieldCodes(applSettings *SettingsPtr,char *buf) {
	char *ptr=buf;
	int i,status;

	try {
		//Ignore first '#Field'
		status=sscanf_s(ptr,"%s",&(SettingsPtr->fieldCodes[0][0]),sizeof(SettingsPtr->fieldCodes[0]));
		if (status!=1) { //failed
			SettingsPtr->fieldCodes[0][0]='\0';
			return;
		}
		ptr+=strlen(SettingsPtr->fieldCodes[0])+1;

		for(i=0;i<32;i++) {
			status=sscanf_s(ptr,"%s",&(SettingsPtr->fieldCodes[i][0]),sizeof(SettingsPtr->fieldCodes[i]));
			if (status!=1) {
				SettingsPtr->fieldCodes[i][0]='\0';
				break;
			}
			if (SettingsPtr->fieldCodes[i]=="s-sitename")
				SettingsPtr->fieldCodeProcessIsPresent=true;
			ptr+=strlen(SettingsPtr->fieldCodes[i])+1;
		}
		if (i>0) {
			DEBUGAPPLPARSE(Informational,"Identified %d field codes in application log for %s.",i,SettingsPtr->ApplicationName);
		}
	}
	catch (...) {
		logger(Error,"Exception in Application parsings parseFieldCodes. Application logging of %s cannot continue due to exception. The line being parsed:%s",SettingsPtr->ApplicationName,buf);
		CreateMiniDump(NULL);
		AfxEndThread(0,true);
	}

}
/***********************************************************************
* parseMessage 
*
* return 1 for success, 0 for fail
************************************************************************/
int parseMessage(unsigned char* input,unsigned char* output,applSettings *loggerInformation) {

	// vanligt indata:    2004-09-03 02:48:27.25 spid3     Server name is '2000SERVER'.  [SQL server]
	// vanligt indata:	  #Fields: date time c-ip cs-username s-ip s-port cs-method cs-uri-stem cs-uri-query sc-status cs(User-Agent) 
	// vanligt indata:	  2004-08-25 23:14:14 192.168.0.69 - 192.168.0.70 80 HEAD /iuident.cab 0408260814 404 Industry+Update+Control
	// vanligt indata:	  23:14:14 2004-08-25 192.168.0.69 - 192.168.0.70 80 HEAD /iuident.cab 0408260814 404 Industry+Update+Control

	//   facit:           <82>Sep 18 13:57:42 192.168.0.12 SyslogGen This is a test

	CString date,time,host,UserID,facilityString,SeverityString="I",ServerIP,Process,nonMatchingFields,ClientIP,SeverityFullString;
	bool dateFound,timeFound,FoundSeverity,ClientIPFound,FoundServerName,FoundServerIP,FoundUserID,FoundProcess,FoundWin32Severity,hostFound;
	unsigned char field[32][1024],tempStr[1024];
	int charsParsed,i,nbrOfFields=0,status,EventId=0;
	CTime DaTime= CTime::GetCurrentTime();
	int facilityAndSeverityValue=loggerInformation->Facility*8+6; //information
	output[0]='\0';
	unsigned char* startpos=input, *endpos=input+strlen((char*)input);
	dateFound=timeFound=FoundSeverity=ClientIPFound=hostFound=FoundServerIP=FoundServerName=FoundUserID=FoundProcess=FoundWin32Severity=false;

	try {

		DEBUGAPPLPARSE(Informational,"Appl line to parse:[%s]",input);
		//prepare host
		if (!loggerInformation->ParseHost) {
			hostFound=true;
			host=applHostName;
			FoundServerIP=true;
			FoundServerName=true;
		} else {
			hostFound=false;
		}

		//prepare severity
		if (!loggerInformation->ParseSeverity) {
			FoundSeverity=true;
			facilityAndSeverityValue=loggerInformation->Facility*8+loggerInformation->SeverityLevel;

			//severity
			SeverityString="I";
			if (loggerInformation->SeverityLevel<6)
				SeverityString="W";
			if (loggerInformation->SeverityLevel<4)
				SeverityString="E";
		} else {
			FoundSeverity=false;
		}

		//prepare process
		if (!loggerInformation->ParseProcess) {
			FoundProcess=true;
			Process=loggerInformation->ProcessName;
		} else {
			FoundProcess=false;
		}

		//parse Date, time
		if (loggerInformation->ParseDate==false) {
			date=DaTime.Format("%b %d");		
			time=DaTime.Format("%H:%M:%S");	
			dateFound=true;
			timeFound=true;
		}


		//Read line into field[]
		do {
			status=sscanf_s((char*)input, "%1023s",(char*)&tempStr,sizeof(tempStr));     //misses multiple spaces... 
			if (status<1)
				break;
			strncpy_s((char*)&field[nbrOfFields],sizeof(field[nbrOfFields]),(char*)&tempStr,sizeof(field[0]));
			input+=strlen((char*)&field[nbrOfFields])+1;
			while ((unsigned int)*input==32) {
				strncat_s((char*)&field[nbrOfFields],sizeof(field[nbrOfFields]),(char*)" ",sizeof(" "));
				input++;
			}
			nbrOfFields++;

		} while((input<endpos)&(nbrOfFields<31));

		if ((*startpos)=='#') { //Headerline in file. If so, skip field identification and assignment.
			i=nbrOfFields;
			nonMatchingFields.Append((char*)startpos);
		} else {
			i=0;
		}

		//Not used
		ClientIPFound=FoundUserID=true;

		//Go through all fieldCodes - fill matches with data
		charsParsed=0;
		for (i;i<nbrOfFields;i++) {


			if (loggerInformation->fieldCodes[i][0]!='\0') { //Fields has been identified

				if((!dateFound)&&(strcmp("date",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					identifyDate(&(field[i][0]),&date);
					dateFound=true;
					continue;
				} 
				else if((!timeFound)&&(strcmp("time",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					identifyTime(&(field[i][0]),&time);
					timeFound=true;
					continue;
				}
				else if((!ClientIPFound)&&(strcmp("c-ip",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					ClientIP=field[i];
					ClientIPFound=true;
					continue;
				}
				else if((!FoundServerName)&&(strcmp("s-computername",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					FoundServerName=true;
					if (loggerInformation->ParseHost) { //Syslog use. Ignores if hostFounf was true! Trust the s-computername
						hostFound=true;
						host=field[i];
					}

					continue;
				}
				else if((!FoundServerIP)&&(strcmp("s-ip",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					ServerIP=field[i]; 
					FoundServerIP=true;
					if ((loggerInformation->ParseHost)&(hostFound==false)) { //Syslog use
						hostFound=true;
						host=field[i];
					}

					continue;
				}
				else if((!FoundUserID)&&(strcmp("cs-username",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					UserID=field[i];
					FoundUserID=true;
					continue;
				}
				else if((!FoundProcess)&&(strcmp("s-sitename",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {
					Process=field[i];
					FoundProcess=true;
					continue;
				}
				//For severity, two fields are interesting!
				else if((!FoundWin32Severity)&&(loggerInformation->ParseSeverity)&&(strcmp("sc-win32-status",(char*)&(loggerInformation->fieldCodes[i][0]))==0)) {

					sscanf_s((char*)&(field[i][0]),"%d",&status);
					if (status!=0) {
						SeverityString="E";
						facilityAndSeverityValue=loggerInformation->Facility*8+3; //Error
					}
					FoundWin32Severity=true;
					nonMatchingFields.Append((char*)&(field[i][0]));
					nonMatchingFields.Append(" ");
					continue;
				}
				else if(strcmp("sc-status",(char*)&(loggerInformation->fieldCodes[i][0]))==0) {
					//sscanf(field[i],"%d",&status);
					EventId=atoi((char*)&(field[i][0]));
					if ((!FoundSeverity)&(loggerInformation->ParseSeverity))
						if ((field[i][0]>'3')) {
							SeverityString="E";
							facilityAndSeverityValue=loggerInformation->Facility*8+3; //Error
						}
						FoundSeverity=true;
						nonMatchingFields.Append((char*)&(field[i][0]));
						nonMatchingFields.Append(" ");
						continue;
				}
			} //end if fieldcodes

			if ((!dateFound)&&(identifyDate(&(field[i][0]),&date))) {
				dateFound=true;
				continue;
			}
			if ((!timeFound)&&(identifyTime(&(field[i][0]),&time))) {
				timeFound=true;
				continue;
			}
			if ((!hostFound)&&(identifyHost(&(field[i][0]),&host))) {
				hostFound=true;
				continue;
			}
			//fieldCodeProcessIsPresent: shows if a valid process info will show up - don't parse first best name
			if ((!FoundProcess)&&(!loggerInformation->fieldCodeProcessIsPresent)&&(identifyProcess(&(field[i][0]),&Process))) {
				FoundProcess=true;
				continue;
			}

			//no match
			nonMatchingFields.Append((char*)&(field[i][0]));
			nonMatchingFields.Append(" ");
		}

		//Should parse, but nothing found
		if (dateFound==false) {
			date=DaTime.Format("%b %d");
			loggerInformation->failedToParseDate=true;
		}
		if (timeFound==false) {
			time=DaTime.Format("%H:%M:%S");	
			loggerInformation->failedToParseTime=true;
		}
		if (FoundSeverity==false) {
			facilityAndSeverityValue=loggerInformation->Facility*8+6; //Information
			loggerInformation->failedToParseSeverity=true;
		}

		if (SeverityString=="E")
			SeverityFullString="error";
		else if (SeverityString=="W")
			SeverityFullString="warning";
		else 
			SeverityFullString="info";

		if (host=="") {
			loggerInformation->failedToParseHost=true;
			host=applHostName;
		}
		if (FoundProcess==false) {
			loggerInformation->failedToParseProcess=true;
			Process="Unknown";
		}

		//just stoppat in [info] under, för att få parsern i Syslogservern att förstå mellanslag i processnamnet.... Bättre lösning?
		//sprintf((char*)output,"<%d>%s %s %s %s[%s] %s%s",facilityAndSeverityValue,date.GetBuffer(),time.GetBuffer(),host.GetBuffer(),process.GetBuffer(),SeverityFullString.GetBuffer(),nonMatchingFields.GetBuffer(),(char*)(input+charsParsed));
		_snprintf_s((char*)output,1024,_TRUNCATE,"<%d>%s %s %s %s[%s] %s",facilityAndSeverityValue,date.GetBuffer(),time.GetBuffer(),host.GetBuffer(),Process.GetBuffer(),SeverityFullString.GetBuffer(),nonMatchingFields.GetBuffer());
		output[1022]=(char)10; //Force cr if full buffert was filled
		output[1023]=(char)0;
		if (output[1021]==(char)10) { //Special case
			output[1023]=(char)0;
		}

		DEBUGAPPLPARSE(Informational,"Appl parse done:[%s]",output);
	}
		catch(SE_Exception e) {
		logger(Informational,"SEH Exception in parsing application log. Error code %u. The line was not inserted. Application logging continues. The line not inserted:%s",e.getSeNumber(),(char*)input);
		CreateMiniDump(NULL);
		Sleep(1000);
		return 0;
	}

	catch (...) {
		logger(Informational,"Exception in parsing application log. The line was not inserted. Application logging continues. The line not inserted:%s",(char*)input);
		CreateMiniDump(NULL);
		Sleep(1000);
		return 0;
	}
	return 1;

}
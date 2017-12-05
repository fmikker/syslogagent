//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  event.h - Event type definition
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
 *  $Id: event.h,v 1.12 2002/09/20 06:12:47 jason Exp $
 *
 *----------------------------------------------------------------------------*/
#ifndef _EVENT_H_
#define _EVENT_H_

#include "ntsl.h"
#include <time.h>


/*-------------------------------[ static data ]------------------------------*/
#define NTSL_EVENT_ERROR		 "[error]"
#define NTSL_EVENT_WARNING  	 "[warning]"
#define NTSL_EVENT_INFORMATION   "[info]"
#define NTSL_EVENT_SUCCESS       "[success]"
#define NTSL_EVENT_FAILURE       "[failure]"
#define NTSL_EVENT_FORMAT_LEN    NTSL_EVENT_LEN
#define NTSL_DEFAULT_PRIORITY	9

/*-------------------------------[ ntsl_event ]-------------------------------*/
typedef struct 
{
    char    date[NTSL_DATE_LEN];
	//erno2005
	char    facilityName[256];
	DWORD   time1970format;

    char    host[NTSL_SYS_LEN];
	char	source[NTSL_SYS_LEN];
	char	etype[NTSL_SYS_LEN];	
    char    msg[NTSL_EVENT_LEN];
    int     priority;
	uint32	id;
	char	user[NTSL_SYS_LEN];
	char	domain[NTSL_SYS_LEN];
} ntsl_event;


//typedef struct {
//	char text[1200];  //MAXBUFLEN not used due to c/c++ problems if including appwatch.h
//} aMess;

//erno2005
int _event_output(ntsl_event *event);

//void ping_syslog_server();

#endif

} //end extern "C" by erno aug04


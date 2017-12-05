//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  eventlog.h - Windows NT eventlog module
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
 *  $Id: eventlog.h,v 1.10 2002/07/23 06:36:51 jason Exp $
 *
 *----------------------------------------------------------------------------*/
#ifndef _EVENTLOG_H_
#define _EVENTLOG_H_

#include "event.h"


typedef struct LoadedLibraryStruct {
	char name[1024];
	HINSTANCE hlib;
} _LoadedLibrary;

typedef struct LoadedSIDStruct {
	bool valid;
	char SID[64];
	char user[256];
	char domain[256];
} _LoadedSIDS;

int  eventlog_init();
void eventlog_shutdown();
void eventlog_check_events();
int eventlog_parse_libs(char*,char*,uint32);
#endif

} //end extern "C" by erno aug04


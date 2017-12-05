//extern "C" by erno aug04

#include "..\Syslogserver\common_stdafx.h" 
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ntsl.h"
#include "engine.h"

extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  engine.c - Event processing engine
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
 *  $Id: engine.c,v 1.9 2002/06/27 21:05:44 jason Exp $
 *
 *  Revision history:
 *    17-Aug-98  JRR  Module completed
 *
 *----------------------------------------------------------------------------*/


/*------------------------------[ private data ]------------------------------*/
static ntsl_event   *engine_last_event  = NULL;
static HANDLE        engine_event_mutex = NULL;


/*-------------------------------[ engine_init ]-------------------------------
 * Create mutex object and open logfile
 *----------------------------------------------------------------------------*/
void engine_init(){

	if ((engine_event_mutex = CreateMutex(NULL, FALSE, NULL)) == 0)    
		ntsl_die(NTSL_ERROR_ENGINE_MUTEX, GetLastError());

}
/*-----------------------------[ engine_shutdown ]-----------------------------
 * Force execution of shutdown functions.
 *----------------------------------------------------------------------------*/
void engine_shutdown()
{
	CloseHandle(engine_event_mutex);
	free(engine_last_event);
}


/*--------------------------[ engine_process_event ]--------------------------
 * Top level event handler.  
 *
 *	Returns:
 *		success		0
 *		failure		-1 
 *
 * NOTE: This module is resposible for freeing event objects.
 *----------------------------------------------------------------------------*/
int engine_process_event(ntsl_event *event)
{
	int				   rc    = -1;

	if (NULL != event){
		
		_event_output(event);

		free(engine_last_event);
		engine_last_event = event;			

		rc = 1;
	}

	return(rc);
}



} //end extern "C" by erno aug04


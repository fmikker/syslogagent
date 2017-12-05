#include "..\Syslogserver\common_stdafx.h" 
//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  safestr.c - Safe string functions
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
 *  $Id: safestr.c,v 1.9 2002/06/27 21:05:44 jason Exp $
 *
 *  Revision history:
 *		11-Sep-98  JRR  Module completed
 *
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

/*-------------------------------[ _snprintf0 ]-------------------------------
 * Write formatted data to a string.  A null character is always appended
 * to the end of the buffer.
 *
 *	Returns
 *		success		number of characters written 
 *		failure		negative value
 *----------------------------------------------------------------------------*/
/*int _snprintf0(char *buffer, size_t count, const char *format, ...)
{
	va_list     args;
	size_t		rc;

	if ( (buffer == NULL) || (format == NULL) || (count < 0) )
		return(-1);
	
    va_start(args, format);
    rc = _vsnprintf_s(buffer, count,count, format, args);
    va_end(args);

	buffer[count - 1] = 0;
	if (rc == count)
		rc--;

	return(rc);
}
*/

/*--------------------------------[ strncpy0 ]--------------------------------
 * Copy characters of one string to another.  A null character is always 
 * appended to the copied string.
 *
 *	Returns
 *		success		pointer to copied string
 *		failure		null 	
 *----------------------------------------------------------------------------*/
/*char *strncpy0(char *strDest, const char *strSource, size_t count)
{
	if ( (strDest == NULL) || (strSource == NULL) || (count < 0) )
		return(NULL);

	strncpy_s(strDest, count,strSource, count);
	strDest[count - 1] = 0;	

	return(strDest);
}
*/

/*---------------------------------[ malloc0 ]---------------------------------
 *  Allocate memory blocks - initialize to 0
 *
 *  Parameters:
 *		size		number of bytes to allocate
 *
 *  Return value:
 *		success		pointer to memory block
 *		failure		null pointer
 *----------------------------------------------------------------------------*/
void *malloc0(size_t size)
{
	void *rc = NULL;

	if (size > 0)
	{
		rc = malloc(size);
		if (rc != NULL)
			memset(rc, 0, size);
	}

	return(rc);
}

} //end extern "C" by erno aug04


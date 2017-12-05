//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  safestr.h - Safe string functions
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
 *  $Id: safestr.h,v 1.9 2002/06/27 21:05:44 jason Exp $
 *
 *----------------------------------------------------------------------------*/

//not needed with _s functions  int _snprintf0(char *buffer, size_t count, const char *format, ...);

//not needed with _s functions   char *strncpy0(char *strDest, const char *strSource, size_t count);

void *malloc0(size_t size);

} //end extern "C" by erno aug04


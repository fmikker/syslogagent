#include "..\Syslogserver\common_stdafx.h" 
#include <winsock2.h> 
#include "..\Syslogserver\common_registry.h"
#include "winerror.h"
#include "assert.h"
//-----LeakWatcher--------------------
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//------------------------------------


PSECURITY_DESCRIPTOR pRegKeySD;
CString pszRegKeyName;
//erno. From Microsofts site, articles Q106387 and http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dnsecure/html/msdn_ntprog.asp called windows nt security (from 94!)
/*----------------------------------------------------------------
| Name: GetRegKeySecurity
| Desc: gets security registry sec. descriptor
|   pRegKeySD is global of type PSECURITY_DESCRIPTOR; you must free
|   the memory alloc'ed for this when done with the reg key
-----------------------------------------------------------------*/
int GetRegKeySecurity ( CString szRegKey){

 HKEY  hRegKey;   // handle for register key
 LONG  lError = 0L;  // reg errors 
        // (GetLastError won't work with registry calls)


 CHAR  szClassName[MAX_PATH] = ""; // Buffer for class name.
 DWORD dwcClassLen = MAX_PATH;  // Length of class string.
 DWORD dwcSubKeys;     // Number of sub keys.
 DWORD dwcMaxSubKey;    // Longest sub key size.
 DWORD dwcMaxClass;    // Longest class string.
 DWORD dwcValues;     // Number of values for this key.
 DWORD dwcMaxValueName;   // Longest Value name.
 DWORD dwcMaxValueData;   // Longest Value data.
 DWORD dwcSDLength;    // Security descriptor length
 FILETIME ftLastWriteTime;   // Last write time.


 // open the security key
 if ( ( lError = RegOpenKey ( HKEY_LOCAL_MACHINE, szRegKey, &hRegKey) ) ) {
  return ( lError);
 }

 // get length of security descriptor
 if ( ( lError = RegQueryInfoKey ( hRegKey, szClassName, &dwcClassLen, 
      NULL, &dwcSubKeys, &dwcMaxSubKey, &dwcMaxClass, 
      &dwcValues, &dwcMaxValueName, &dwcMaxValueData, 
           &dwcSDLength, &ftLastWriteTime) ) )
 {
	RegCloseKey ( hRegKey);
	return lError;
 }

 // get SD memory
 pRegKeySD = ( PSECURITY_DESCRIPTOR) LocalAlloc ( LPTR, ( UINT)dwcSDLength);

 // now get SD
 if ( ( lError = RegGetKeySecurity ( hRegKey, 
      (SECURITY_INFORMATION)( OWNER_SECURITY_INFORMATION
            | GROUP_SECURITY_INFORMATION
            | DACL_SECURITY_INFORMATION),
             pRegKeySD, &dwcSDLength) ) )
 {
	RegCloseKey ( hRegKey);
	return lError;
 }

 // check if SD is good
 if ( ! IsValidSecurityDescriptor ( pRegKeySD)) {
  lError = GetLastError();

	RegCloseKey ( hRegKey);
	return lError;
 }


 return 0;
}
/* eof - GetRegKeySecurity */

//----------------------------------------------------------------------------
/*------------------------------------------------------------------
| Name: AddToRegKeySD
| Desc: adds SID to SD on reg key
|   passed into function:
|    SD in self-relative mode
|    SID from the group or user
|    permission access requested
------------------------------------------------------------------*/
DWORD AddToRegKeySD ( PSECURITY_DESCRIPTOR pRelSD, PSID pGroupSID,
               DWORD dwAccessMask)
{
 PSECURITY_DESCRIPTOR pAbsSD = NULL;

 PACL  pDACL;

 DWORD  dwSDLength = 0;
 DWORD  dwSDRevision;
 DWORD  dwDACLLength = 0;

 SECURITY_DESCRIPTOR_CONTROL sdcSDControl;

 PACL  pNewDACL  = NULL;
 DWORD  dwAddDACLLength = 0;

 BOOL  fHasDACL  = FALSE;
 BOOL  fDACLDefaulted = FALSE; 

 ACCESS_ALLOWED_ACE  *pDACLAce;

 DWORD  dwError = 0;

 DWORD  i;

 // handle for security registry key
 HKEY  hSecurityRegKey = ( HKEY) 0;

 // get SD control bits
 if ( ! GetSecurityDescriptorControl ( pRelSD, 
        ( PSECURITY_DESCRIPTOR_CONTROL) &sdcSDControl,
             ( LPDWORD) &dwSDRevision) )
  return ( GetLastError() );

 // check if DACL is present
 if ( SE_DACL_PRESENT & sdcSDControl)
 {
  // get dacl 
  if ( ! GetSecurityDescriptorDacl ( pRelSD, ( LPBOOL) &fHasDACL,
           ( PACL *) &pDACL, 
           ( LPBOOL) &fDACLDefaulted) )
   return ( GetLastError());

  // get dacl length
  dwDACLLength = pDACL->AclSize;

  // now check if SID's ACE is there
  for ( i = 0; i < pDACL->AceCount; i++)
  {
   if ( ! GetAce ( pDACL, i, ( LPVOID *) &pDACLAce) )
    return ( GetLastError());

   // check if group sid is already there
   if ( EqualSid ( ( PSID) &(pDACLAce->SidStart), pGroupSID) )
    break;
  }

  // exit if found (means already has been set)
  if ( i < pDACL->AceCount)
  {
   dwError = ERROR_GROUP_EXISTS;

   return ( dwError);
  }

  // get length of new DACL
  dwAddDACLLength = sizeof ( ACCESS_ALLOWED_ACE) - 
        sizeof ( DWORD) + GetLengthSid ( pGroupSID);
 }
 else
  // get length of new DACL
  dwAddDACLLength = sizeof ( ACL) + sizeof ( ACCESS_ALLOWED_ACE) - 
        sizeof ( DWORD) + GetLengthSid ( pGroupSID);

 // get memory needed for new DACL
 if ( ! ( pNewDACL = ( PACL) malloc ( dwDACLLength + dwAddDACLLength) ) )
  return ( GetLastError());

 // get the sd length
 dwSDLength = GetSecurityDescriptorLength ( pRelSD);

 // get memory for new SD
 if ( ! ( pAbsSD = ( PSECURITY_DESCRIPTOR) 
       malloc ( dwSDLength + dwAddDACLLength) ) )
 {
  dwError = GetLastError();

  goto ErrorExit;
 }

 // change self-relative SD to absolute by making new SD
 if ( ! InitializeSecurityDescriptor ( pAbsSD, SECURITY_DESCRIPTOR_REVISION) ) {
	dwError = GetLastError();
	goto ErrorExit;
 }

 // init new DACL
 if ( ! InitializeAcl ( pNewDACL, dwDACLLength + dwAddDACLLength, ACL_REVISION) ) {
	dwError = GetLastError();
	goto ErrorExit;
 }

 // now add in all of the ACEs into the new DACL (if org DACL is there)
 if ( SE_DACL_PRESENT & sdcSDControl)
 { 
  for ( i = 0; i < pDACL->AceCount; i++)
  {
   // get ace from original dacl
   if ( ! GetAce ( pDACL, i, ( LPVOID *) &pDACLAce) )
   {
    dwError = GetLastError();

    goto ErrorExit;
   }

   // now add ace to new dacl
   if ( ! AddAccessAllowedAce ( pNewDACL, 
           ACL_REVISION, 
           pDACLAce->Mask,
           ( PSID) &(pDACLAce->SidStart) ) )
   {
    dwError = GetLastError();

    goto ErrorExit;
   }
  }
 }

 // now add new ACE to new DACL
 if ( ! AddAccessAllowedAce ( pNewDACL, ACL_REVISION, dwAccessMask,
               pGroupSID) )
 {
  dwError = GetLastError();

  goto ErrorExit;
 }

 // check if everything went ok
 if ( ! IsValidAcl ( pNewDACL) ){
  dwError = GetLastError();
  goto ErrorExit;
 }

 // now set security descriptor DACL
 if ( ! SetSecurityDescriptorDacl ( pAbsSD, TRUE, pNewDACL, fDACLDefaulted) ) {
  dwError = GetLastError();
  goto ErrorExit;
 }

 // check if everything went ok
 if ( ! IsValidSecurityDescriptor ( pAbsSD) ) {
  dwError = GetLastError();
  goto ErrorExit;
 }

 // now open reg key to set security 
 // note: pzsRegKeyName is a global
//erno if ( ( dwError = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, pszRegKeyName, 0,KEY_ALL_ACCESS, &hSecurityRegKey) ) )
 if ( ( dwError = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, pszRegKeyName, 0,WRITE_DAC, &hSecurityRegKey) ) )
	 
  goto ErrorExit;


 // now set the reg key security (this will overwrite any existing security)
 dwError = RegSetKeySecurity ( 
      hSecurityRegKey, 
      (SECURITY_INFORMATION)( DACL_SECURITY_INFORMATION),
      pAbsSD);

 // close reg key
 RegCloseKey ( hSecurityRegKey);


ErrorExit:

 // free memory
 if ( pAbsSD)
  free ( ( VOID *) pAbsSD);
 if ( pNewDACL)
  free ( ( VOID *) pNewDACL);

 return ( dwError);
}
/* eof - AddToRegKeySD */

/*-----------------------------------------------------------------------------
*
* Add Authenticated group to permissions
*
-----------------------------------------------------------------------------*/
int AddPermissions(CString keyName){

	PSID   pAuthenticatedUsersSID = NULL;
	SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;

	int status=GetRegKeySecurity ( keyName);
	//fixes global PSECURITY_DESCRIPTOR pRegKeySD;

	if (status) //failed
		return status;

	pszRegKeyName=keyName;

   // obtain a sid for the Authenticated Users Group
      if (!AllocateAndInitializeSid(&siaNT, 1,SECURITY_AUTHENTICATED_USER_RID, 0, 0, 0, 0, 0, 0, 0,&pAuthenticatedUsersSID)) {
         return GetLastError();
	  }

	status=AddToRegKeySD ( pRegKeySD, pAuthenticatedUsersSID,KEY_ALL_ACCESS);
	if (status) //failed
		return status;

return 0;
}


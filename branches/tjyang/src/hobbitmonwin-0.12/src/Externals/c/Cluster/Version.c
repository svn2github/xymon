/*
this file is part of BBWin
Copyright (C)2006 Etienne GRIGNON  ( etienne.grignon@gmail.com )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Version.c

    Check Windows version for Windows Clustering

*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "cluster.h"

// Only use these versions :
// Windows Server 2003 
// Windows 2000 Advanced Server and Windows 2000 Datacenter Server 
// Windows NT Server 4.0 Enterprise Edition SP3 and later 
//
int 					checkVersion()
{
	OSVERSIONINFOEX 	osvi;
	BOOL 				bOsVersionInfoEx;
	int					flag;
	
	flag = TRUE;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if( !(bOsVersionInfoEx = GetVersionExW ((OSVERSIONINFO *) &osvi)) ) {
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionExW ( (OSVERSIONINFO *) &osvi) ) {
			wprintf (L"GetVersionExW failed on this system. Error %i\n", GetLastError());
			return FALSE;
		}
	}
	wprintf (L"Microsoft Windows Version %i.%i\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
	switch (osvi.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
		// Test for the specific product.
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			wprintf (L"Microsoft Windows Server 2003, ");
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			wprintf (L"Microsoft Windows XP ");
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			wprintf (L"Microsoft Windows 2000 ");
		if ( osvi.dwMajorVersion <= 4 )
			wprintf(L"Microsoft Windows NT ");
		if( bOsVersionInfoEx )
		{
			// Test for the workstation type.
			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				if( osvi.dwMajorVersion == 4 )
					wprintf ( L"Workstation 4.0 " );
				else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					wprintf ( L"Home Edition " );
				else 
					wprintf ( L"Professional " );
				flag = FALSE;
			}
			// Test for the server type.
			else if ( osvi.wProductType == VER_NT_SERVER || 
					   osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
			{
				if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
				{
				   if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						wprintf ( L"Datacenter Edition " );
				   else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						wprintf ( L"Enterprise Edition " );
				   else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
						wprintf ( L"Web Edition " );
				   else 
						wprintf ( L"Standard Edition " );
				}
				else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						wprintf ( L"Datacenter Server " );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						wprintf ( L"Advanced Server " );
					else 
					{
						wprintf ( L"Server " );
						flag = FALSE;
					}
				}
            else  // Windows NT 4.0 
            {
				if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					wprintf (L"Server 4.0, Enterprise Edition " );
				else 
					wprintf ( L"Server 4.0 " );
				flag = FALSE;
            }
         }
      }
      // Test for specific product on Windows NT 4.0 SP5 and earlier
      else  
      {
         HKEY hKey;
         TCHAR	 szProductType[BUFFER_SIZE];
         DWORD dwBufLen=BUFFER_SIZE;
         LONG lRet;

         lRet = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet != ERROR_SUCCESS )
            return FALSE;

         lRet = RegQueryValueExW( hKey, L"ProductType", NULL, NULL,
            (LPBYTE) szProductType, &dwBufLen);
         if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFFER_SIZE) )
            return FALSE;

         RegCloseKey( hKey );

         if ( lstrcmpiW( L"WINNT", szProductType) == 0 )
            wprintf( L"Workstation " );
         if ( lstrcmpiW( L"LANMANNT", szProductType) == 0 )
            wprintf( L"Server " );
         if ( lstrcmpiW( L"SERVERNT", szProductType) == 0 )
            wprintf( L"Advanced Server " );
         wprintf( L"%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
		 flag = FALSE;
      }
      if( osvi.dwMajorVersion == 4 && 
          lstrcmpiW( osvi.szCSDVersion, L"Service Pack 6" ) == 0 )
      { 
         HKEY hKey;
         LONG lRet;

         // Test for SP6 versus SP6a.
         lRet = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet == ERROR_SUCCESS )
            wprintf( L"Service Pack 6a (Build %d)\n", 
            osvi.dwBuildNumber & 0xFFFF );         
         else // Windows NT 4.0 prior to SP6a
         {
            wprintf( L"%s (Build %d)\n",
               osvi.szCSDVersion,
               osvi.dwBuildNumber & 0xFFFF);
         }
         RegCloseKey( hKey );
		 flag = FALSE;
      }
      else // not Windows NT 4.0 
      {
        wprintf( L"%s (Build %d)\n",
            osvi.szCSDVersion,
            osvi.dwBuildNumber & 0xFFFF);
      }
      break;
      // Test for the Windows Me/98/95.
      case VER_PLATFORM_WIN32_WINDOWS:
		//"Microsoft Windows 95 "
		//"Microsoft Windows 98 "
		//"Microsoft Windows Millennium Edition"
		wprintf (L"Microsoft Windows Me/98/95\n");
		flag = FALSE;
      break;

      case VER_PLATFORM_WIN32s:
		wprintf (L"Microsoft Win32s\n");
		flag = FALSE;
      break;
   }
   return flag; 
}
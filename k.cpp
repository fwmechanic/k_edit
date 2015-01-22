//
// Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
//
// This file is part of K.
//
// K is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// K is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along
// with K.  If not, see <http://www.gnu.org/licenses/>.
//

//
//  CONVERTING THIS TO COMPILE WITH C-language to make k.exe smaller DOES NOT
//  WORK.  Don't bother (again).
//

#include <windows.h>
#include <stdio.h>

const char entrypoint[] =
#if defined(__GNUC__)

#ifdef __x86_64
                           "Main" ;
#else
                           "Main@12" ;
#endif

int _CRT_glob = 0; // suppress default MinGW argv globbing   http://cygwin.com/ml/cygwin/1999-11/msg00052.html

#else
                           "_Main@12";
#endif

#if defined(_WIN32)
   #define CDECL__ __cdecl
#else
   #define CDECL__
#endif

int CDECL__ main( int argc, char *argv[], char *envp[] ) {
   char  buf[ MAX_PATH+1 ];
   const auto len( GetModuleFileName( nullptr, buf, sizeof buf ) );
   if( len >= sizeof(buf) ) {
      fprintf( stderr, "GetModuleFileName rv (%lu) >= sizeof(buf) (%" PR_SIZET "u)\n", len, sizeof(buf) );
      return 1;
      }

   for( auto pExt = buf + strlen( buf ) - 1; pExt > buf; --pExt ) {
      // fprintf( stderr, "'%c' ?\n" );
      if( *pExt == '.' ) {
 #if 0
         pExt[1] = 'd';   // load K.DLL
         pExt[2] = 'l';
         pExt[3] = 'l';
         pExt[4] =  0 ;
 #elif 1
         pExt[0] = 'x';   // load KX.DLL
         pExt[1] = '.';
         pExt[2] = 'd';
         pExt[3] = 'l';
         pExt[4] = 'l';
         pExt[5] =  0 ;
 #else
 #   error
 #endif

         // fprintf( stderr, "MFN '%s'\n", buf );

         auto hDLL = LoadLibrary( buf );
         if( !hDLL ) {
            fprintf( stderr, "LoadLibrary( '%s' ) failed; Error: %lu\n", buf, GetLastError() );
            return 1;
            }

         typedef void (*FxnMain)( int argc, char *argv[], char *envp[] );

         const auto editor_main( FxnMain( GetProcAddress( hDLL, entrypoint ) ) ); // NB: cannot use static_cast<FxnMain> to replace C-style cast here
         if( !editor_main ) {
            FreeLibrary( hDLL );
            fprintf( stderr, "GetProcAddress(%s) failed\n", argv[0] );
            return 1;
            }

         editor_main( argc, argv, envp ); // never returns
         return 1;  // just in case ...
         }
      }
   return 1;  // should not get here
   }

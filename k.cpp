//
//  Copyright 1991 - 2012 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
//  CONVERTING THIS TO COMPILE WITH C-language to make k.exe smaller DOES NOT
//  WORK.  Don't bother (again).
//

#include <windows.h>
#include <stdio.h>

const char entrypoint[] =
#ifdef __GNUC__

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
      fprintf( stderr, "GetModuleFileName rv (%lu) >= sizeof(buf) (%Iu)\n", len, sizeof(buf) );
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

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#else
#include "fix_coverity.h"
#include <unistd.h>  // readlink etc
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#endif

#if defined(_WIN32)
#   define WL(ww,ll)  ww
#else
#   define WL(ww,ll)  ll
#endif

#include <sys/stat.h>
#if defined(_WIN32)
   #define stat  _stat
   #define fstat _fstat
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Must be defined before including lua.h */
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "k_lib.h"

#undef   LUAFUNC_
#undef   LUA_FUNC_I
#define  LUAFUNC_( func )   static int l_klib_##func( lua_State *L )
#define  LUA_FUNC_I( func )   { #func, l_klib_##func },

// copy of liolib.c:pushresult

static int push_errno_result (lua_State *L, int fOk, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (fOk) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      lua_pushfstring(L, "%s", strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}


#define  K_BIN_DEF  1
#if      K_BIN_DEF

unsigned GetUint_Hack( lua_State *L, int n )
   {
   // I_(n) macro does not handle corner case where int value would have MSBit
   // set.  For those cases where it is assumed that the "integer" value is
   // actually unsigned, you should use U_ which calls this function.
   //
   // note also that if a numeric RETURN value could be > MAX_INT, you should use R_num instead of R_int
   //
   // this behavior seen with Lua 5.1.2 and 5.1.3
   //
   // NB: if this code is condensed, it may stop working...
   //
   // 20080505 kgoodwin discovered/wrote

   unsigned rv;
   const lua_Number n1 = N_(n) ;
   if( n1 > ((lua_Number)0xFFFFFFFF) )  luaL_error( L, "GetUint range error: N > 0xFFFFFFFF" );
   if( n1 < ((lua_Number)0)          )  luaL_error( L, "GetUint range error: N < 0" );

   rv = n1;
   return rv;
   }

#include <math.h>

LUAFUNC_(bitand)  { R_num( U_(1) &  U_(2) ); }
LUAFUNC_(bitor)   { R_num( U_(1) |  U_(2) ); }
LUAFUNC_(bitxor)  { R_num( U_(1) ^  U_(2) ); }
LUAFUNC_(shiftl)  { R_num( U_(1) << U_(2) ); }
LUAFUNC_(shiftr)  { R_num( U_(1) >> U_(2) ); }
LUAFUNC_(bit )    { R_num( pow( ((lua_Number)2), ((lua_Number)U_(1)) ) ); }
LUAFUNC_(bits)    { R_num( pow( ((lua_Number)2), ((lua_Number)U_(1)) ) - 1 ); }

LUAFUNC_(replace)   // in S1, replace S2 with S3, return replaced string, # of replacements
   {
   size_t inBytes, srchBytes, rplcBytes, bytesLeft;

   const char *pIn   = Bytes_( 1, inBytes   );
   const char *pSrch = Bytes_( 2, srchBytes );
   const char *pRplc = Bytes_( 3, rplcBytes );

#if 1
   if( srchBytes == rplcBytes )
      {
      //
      // Since srchBytes == rplcBytes we can (when the buffers involved are very
      // large: circa 10MB) reap a MAJOR performance improvement by avoiding any
      // intermediate Lua buffer allocs (that arise from the use of luaL_Buffer
      // and friends) because we can take advantage of the fact that we know at
      // the start exactly how big the destination buffer needs to be, and
      // manually allocate it "out of band" via malloc.
      //
      // NB: The apparent Lua memory usage may actually INCREASE due to this
      // optzn (my explanation is that gc activity is throttled by # of allocs,
      // and this optzn leads to fewer, but MUCH larger Lua allocs).  Thus it is
      // likely that you'll need to call collectgarbage("step",N) in Lua code
      // following calls to this function to counteract the increase in overall
      // Lua heap usage which this change may lead to.
      //
      // Even so, the net performance improvement is huge.
      //
      // 20071101 kgoodwin  added, doc'd this optimization
      //
      int rplcCount = 0;
      char *pOutbuf, *pC;
      pC = pOutbuf = malloc( inBytes );
      if( !pOutbuf ) return luaL_error( L, "replace1: failed to alloc %d bytes for buffer", inBytes );
      bytesLeft = inBytes;
      while( bytesLeft >= srchBytes )
         {
         const char *pStart = pIn + (inBytes - bytesLeft);
         const char *pMatch = memchr( pStart, *pSrch, bytesLeft );
         // fprintf( stderr, "%d# 0x%02X @%d\n", rplcCount, pStart - pIn );
         if( pMatch )
            {
            const size_t numBytes = (pMatch - pStart);
            bytesLeft -= numBytes;
            memcpy( pC, pStart, numBytes );
            pC += numBytes;
            // fprintf( stderr, "%d# @%X C  @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, pMatch - pStart );

            if( (0 == memcmp( pMatch, pSrch, srchBytes )) )
               {
               bytesLeft -= srchBytes;
               // fprintf( stderr, "%d# @%X -> @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, rplcBytes );
               memcpy( pC, pRplc, rplcBytes );
               pC += rplcBytes;
               ++rplcCount;
               }
            else
               {
               *pC++ = *pMatch;
               --bytesLeft;
               }
            }
         else
            {
            break;
            }
         }

      memcpy( pC, pIn + (inBytes - bytesLeft), bytesLeft );
      lua_pushlstring(L, pOutbuf, inBytes);
      free( pOutbuf );
      lua_pushinteger(L, rplcCount);  /* number of substitutions */
      return 2;
      }
   else
#endif
      {
      int rplcCount = 0;
      luaL_Buffer buf;
      luaL_buffinit(L, &buf);

      bytesLeft = inBytes;
      while( bytesLeft >= srchBytes )
         {
         const char *pStart = pIn + (inBytes - bytesLeft);
         const char *pMatch = memchr( pStart, *pSrch, bytesLeft );
         // fprintf( stderr, "%d# 0x%02X @%d\n", rplcCount, pStart - pIn );
         if( pMatch )
            {
            bytesLeft -= (pMatch - pStart);
            luaL_addlstring( &buf, pStart, pMatch - pStart );
            // fprintf( stderr, "%d# @%X C  @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, pMatch - pStart );

            if( (0 == memcmp( pMatch, pSrch, srchBytes )) )
               {
               bytesLeft -= srchBytes;
               // fprintf( stderr, "%d# @%X -> @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, rplcBytes );
               luaL_addlstring( &buf, pRplc , rplcBytes       );
               ++rplcCount;
               }
            else
               {
               luaL_addlstring( &buf, pMatch, 1 );
               --bytesLeft;
               }
            }
         else
            {
            break;
            }
         }

      luaL_addlstring( &buf, pIn + (inBytes - bytesLeft), bytesLeft );
      luaL_pushresult( &buf );
      lua_pushinteger(L, rplcCount);  /* number of substitutions */
      return 2;
      }
   }


void register_k_bin( lua_State *L )
   {
   static const luaL_reg myLuaFuncs[] =
      {
      LUA_FUNC_I( bitand  )  // leading "bit" as these conflict with build-in operators of the same name
      LUA_FUNC_I( bitor   )  // leading "bit" as these conflict with build-in operators of the same name
      LUA_FUNC_I( bitxor  )  // leading "bit" as these conflict with build-in operators of the same name
      LUA_FUNC_I( shiftl  )
      LUA_FUNC_I( shiftr  )
      LUA_FUNC_I( bit     )
      LUA_FUNC_I( bits    )
      LUA_FUNC_I( replace )
      { 0, 0 }
      };

   luaL_register(L, "_bin", myLuaFuncs);
   }

#endif//K_BIN_DEF


//=========================================================================

#if defined(_WIN32)
#define  U_DIR_DEF  1
#else
#define  U_DIR_DEF  1
#endif

#if      U_DIR_DEF

#if defined(_WIN32)

//
// THE (one and only) rule for how we decide which pathsep to use: if any '/' is
// found in the parameter string S_(1), then all pathseps will be '/', else all
// pathseps will be '\'
//
static int pathsep_of_path( const char *path )  { return strchr( path, '/' ) ? '/' : '\\'; }

// 20061212 kgoodwin I HATE that Win32 API's insist on capitalizing the drive letter!
#define  FIX_DRIVE_LETTER( buf )  do { if( buf[0] && buf[1] == ':' )  buf[0] = tolower( buf[0] ); } while( 0 )

static void tr1( char *buf, int from, int to )
   {
   int ix;
   for( ix=0 ; buf[ix] ; ++ix )
      if( buf[ix] == from )
          buf[ix] =  to;
   }

static void tr_pathsep( char *buf, int pathsep )
   {
   tr1( buf, (pathsep == '/') ? '\\' : '/', pathsep );
   }

#endif

LUAFUNC_(chdir)
   {
   const char *dirnm = S_(1);
   int ok = !( WL( _chdir, chdir )( dirnm ) == -1);
   return push_errno_result(L, ok, dirnm);
   }

LUAFUNC_(create)
   {
   const char *dirnm = S_(1);
   int ok = (WL( _mkdir( dirnm ), mkdir( dirnm, 0777 ) ) != -1);
   return push_errno_result(L, ok, dirnm);
   }

//
// Functions not implemented (or removed) because the following functions exist
// in the standard Lua libraries:
//
// os.rename: Renames file or directory named oldname to newname.  If this
// function fails, it returns nil, plus a string describing the error.
//
// os.remove: Deletes the file or directory with the given name.  Directories
// must be empty to be removed.  If this function fails, it returns nil, plus a
// string describing the error.
//

// note that we appear to IGNORE the parts of the string that are not
// relative paths (IOW, those parts are not filtered out for duplicate
// sequential pathsep chars, nor for incorrectly-cased names

LUAFUNC_(fullpath)
   {
   const char *pnm = S_(1);
#if defined(_WIN32)
   char buf[ _MAX_PATH+1 ];
   char pathsep = pathsep_of_path( pnm );
   if( !_fullpath( buf, pnm, sizeof buf ) )
      {
      R_nil();
      }

   FIX_DRIVE_LETTER( buf );
   tr_pathsep( buf, pathsep );
   R_str( buf );
#else
   char actualpath [PATH_MAX+1];
   char *rp = realpath( pnm, actualpath );
   if( !rp ) {
      return luaL_error( L, "realpath %s returned null", pnm );
      }
   R_str( rp );
#endif
   }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct
   {
   lua_State *L;
   int        ix;
#if defined(_WIN32)
   char       pathsep;
#endif
   int        fDirsOnly;
   } scand_shvars;

#if defined(_WIN32)

static void scandir_( const char *dirname, int recurse, scand_shvars *pSdsv )
   {

#define  A_PUSH( str )                           \
      {                                          \
      lua_pushstring( pSdsv->L, str );           \
      lua_rawseti(    pSdsv->L, -2, pSdsv->ix ); \
      ++pSdsv->ix;                               \
      }

   HANDLE          hList;
   char            wcStr[ _MAX_PATH+1 ];
   WIN32_FIND_DATA fd;

   int dnLen = strlen( dirname );
   if( dnLen && (dirname[dnLen-1] == '\\' || dirname[dnLen-1] == '/') )
      {
      --dnLen;
      _snprintf( wcStr, sizeof wcStr, "%s*", dirname);
      }
   else
      _snprintf( wcStr, sizeof wcStr, "%s%c*", dirname, pSdsv->pathsep);

   tr_pathsep( wcStr, pSdsv->pathsep );

   if( pSdsv->fDirsOnly )
      // 20081005 kgoodwin FindExSearchLimitToDirectories appears to have NO EFFECT, so using FindFirstFileEx is same as using FindFirstFile
      hList = FindFirstFileEx( wcStr, FindExInfoStandard, &fd, FindExSearchLimitToDirectories, NULL, 0 );  // Get the first file
   else
      hList = FindFirstFile(wcStr, &fd);  // Get the first file

   if( hList == INVALID_HANDLE_VALUE ) // No dirs or files found?
      return;                          // return emptyhanded

   do { // Traverse through the directory structure
      char buf[ _MAX_PATH+1 ];
      _snprintf( buf, sizeof buf, "%*.*s%c%s", dnLen, dnLen, wcStr, pSdsv->pathsep, fd.cFileName);
      if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
         {
         if( (strcmp(fd.cFileName, ".") != 0) && (strcmp(fd.cFileName, "..") != 0) )
            {
            char buf2[ _MAX_PATH+1 ];
            _snprintf( buf2, sizeof buf2, "%s%c", buf, pSdsv->pathsep );
            A_PUSH( buf2 );
            if( recurse )  // Get the full path for sub directory
               scandir_( buf, 1, pSdsv );
            }
         }
      else // it's a FILE
         {
         if( !pSdsv->fDirsOnly )
            {
            A_PUSH( buf );
            }
         }
      } while( !(FindNextFile(hList, &fd) == 0 && GetLastError() == ERROR_NO_MORE_FILES) );
                 // failure                    && failure

   FindClose(hList);
   }

#else

static void scandir_( const char *dirname, int recurse, scand_shvars *pSdsv )
   {
#if 1
#define  A_PUSH( str )                           \
      {                                          \
      lua_pushstring( pSdsv->L, str );           \
      lua_rawseti(    pSdsv->L, -2, pSdsv->ix ); \
      ++pSdsv->ix;                               \
      }
   if( 1 ) {
      int dnLen = strlen( dirname );
      if( dnLen && dirname[dnLen-1] == '/' ) {
         --dnLen;
         }

      DIR *d = opendir( dirname );
      if( d ) {
         struct dirent *dir;
         while ((dir = readdir(d)) != NULL) { // Traverse through the directory structure (depth first)
            if( (strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0) )
               continue;

            char buf[ PATH_MAX+1 ];
            snprintf( buf, sizeof buf, "%*.*s/%s", dnLen, dnLen, dirname, dir->d_name );
            struct stat sbuf;
            if( stat( buf, &sbuf ) == 0 ) {
               if( sbuf.st_mode & S_IFDIR ) {
                  int len = strlen( buf );
                  if( len+1 > sizeof buf ) {
                     A_PUSH( "** TRUNCATED **" );
                     }
                  else {
                     buf[len] = '/';
                     buf[len+1] = '\0';
                     A_PUSH( buf );
                     buf[len] = '\0';
                     if( recurse )  // Get the full path for sub directory
                        scandir_( buf, recurse, pSdsv );
                     }
                  }
               else // it's a FILE
                  {
                  if( !pSdsv->fDirsOnly ) {
                     A_PUSH( buf );
                     }
                  }
               }
            }
         closedir(d);
         }
      }
#endif
   }

#endif

//
// Return an array of names of all entries (except '.' and '..') contained in
// the directory named S_(1), optionally (if I_(2)) recursively.
static int read_names_( lua_State *L, int fDirsOnly )
   {
   const char  *pnm = S_(1);
   scand_shvars sv = { L, 1 };
#if defined(_WIN32)
   sv.pathsep = pathsep_of_path( pnm );
#endif
   sv.fDirsOnly = fDirsOnly;
   lua_newtable(L);  // result
   scandir_( pnm, I_(2), &sv );
   return 1;
   }

LUAFUNC_(read_names)
   {
   return read_names_( L, 0 );
   }

LUAFUNC_(read_dirnames)
   {
   return read_names_( L, 1 );
   }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <sys/types.h>
#include <sys/stat.h>

static int is_name_stat( lua_State *L, const char *nm, int statflag ) {
   struct WL(_stat,stat) statbuf;
   if( WL(_stat,stat)( nm, &statbuf ) || 0==(statbuf.st_mode & statflag) ) { R_nil(); }
   R_str( nm );
   }

LUAFUNC_(install) {
#if defined(_WIN32)
   char buf[ _MAX_PATH+1+1 ];
   const char *psep = So0_(1);
   const char sep = (psep && pathsep_of_path( psep ) == '/') ? '/' : '\\';
   const DWORD gmlen = GetModuleFileName( NULL, buf, sizeof(buf) );
   if( 0==gmlen )  { R_nil(); }

   FIX_DRIVE_LETTER( buf );
   tr_pathsep( buf, sep );

   { // convention in _dir module is that returned directories have trailing sep
   int ix;
   for( ix = gmlen-1; ix>0; --ix ) {
      if( sep == buf[ix] ) {
         buf[ix+1] = 0;
         break;
         }
      }
   R_str( buf );
   }
#else
   // Linux-only magic
   static const char s_link_nm[] = "/proc/self/exe";
   if( 0 ) {
      // this is pointless since there is a race condition: between (this)
      // lstat and the readlink call, the link content could be changed
      struct stat sb;
      if( lstat( s_link_nm, &sb ) == -1 ) {
         return luaL_error( L, "could not lstat %s", s_link_nm );
         }
      }
   size_t bufbytes = PATH_MAX;
   for(;;) {
      char *linkname = malloc( bufbytes );
      if( !linkname ) {
         return luaL_error( L, "could not alloc %u bytes", bufbytes );
         }
      ssize_t r = readlink( s_link_nm, linkname, bufbytes );
      if( r < 0 ) {
         free( linkname );
         R_nil();
         }
      if( r < bufbytes ) {
         linkname[r] = '\0';
         P_str( linkname );
         free( linkname );
         return 1;
         }
      free( linkname );
      bufbytes *= 2;
      }
#endif
   }

LUAFUNC_(current) {
#if defined(_WIN32)
   char buf[ _MAX_PATH+1+1 ];
   const char *psep = So0_(1);
   const char sep = (psep && pathsep_of_path( psep ) == '/') ? '/' : '\\';
   if( !_getcwd( buf, sizeof(buf)-1 ) )  { R_nil(); }
   FIX_DRIVE_LETTER( buf );
   tr_pathsep( buf, sep );

   // convention in _dir module is that returned directories have trailing sep
   const int len = strlen( buf );
   if( len > sizeof( buf )-2 )  { R_nil(); }
   buf[len  ] = sep;
   buf[len+1] = 0  ;
   R_str( buf );
#else
   char *mallocd_cwd = getcwd( NULL, 0 );
   luaL_Buffer buf;
   luaL_buffinit(L, &buf);
   // convention in _dir module is that returned directories have trailing sep
   luaL_addstring( &buf, mallocd_cwd );
   free( mallocd_cwd );
   luaL_addchar( &buf, '/' ); // no choice about dirsep in linux
   luaL_pushresult( &buf );
   return 1;
#endif
   }

LUAFUNC_(dirsep_class)     { R_str( WL( "[\\/]", "[/]" ) ); }
LUAFUNC_(dirsep_os)        { R_str( WL( "\\", "/" ) ); }
LUAFUNC_(dirsep_preferred) { R_str(           "/"   ); }
LUAFUNC_(pathsep_os)       { R_str( WL( ";", ":" ) ); }

LUAFUNC_(name_isfile) {
   const char *nm = S_(1);
   return is_name_stat( L, nm, WL( _S_IFREG, (S_IFREG | S_IFLNK) ) );
   }

LUAFUNC_(name_isdir) {
   int rv;
   char *nm = strdup( S_(1) );
   const size_t len = strlen( nm );
   if( len > 0 && (nm[len-1]=='\\' || nm[len-1]=='/') ) {
      nm[len-1] = 0;
      }
   rv = is_name_stat( L, nm, WL( _S_IFDIR, S_IFDIR ) );
   free( nm );
   return rv;
   }

LUAFUNC_(filesize) {
   struct WL(_stat,stat) statbuf;
   if( WL(_stat,stat)( S_(1), &statbuf ) )        { R_nil(); }
   if( (statbuf.st_mode & WL( _S_IFMT, S_IFMT )) != WL( _S_IFREG, (S_IFREG | S_IFLNK) ) )  { R_nil(); }
   R_int( statbuf.st_size );
   }

LUAFUNC_(rmdir) {
   const char *dirnm = S_(1);
   const int ok = ( WL( _rmdir, rmdir )( dirnm ) == 0);
   return push_errno_result(L, ok, dirnm);
   }


static void register__dir( lua_State *L ) {
   static const luaL_reg myLuaFuncs[] = {
      LUA_FUNC_I( chdir         )
      LUA_FUNC_I( current       )
      LUA_FUNC_I( create        )
      LUA_FUNC_I( read_names    )
      LUA_FUNC_I( read_dirnames )
      LUA_FUNC_I( filesize      )
      LUA_FUNC_I( fullpath      )
      LUA_FUNC_I( install       )
      LUA_FUNC_I( name_isfile   )
      LUA_FUNC_I( name_isdir    )
      LUA_FUNC_I( dirsep_class     )
      LUA_FUNC_I( dirsep_os        )
      LUA_FUNC_I( dirsep_preferred )
      LUA_FUNC_I( pathsep_os    )
      LUA_FUNC_I( rmdir         ) // os.remove will not remove directories on Win32; this does!
      { 0, 0 }
      };

   luaL_register(L, "_dir", myLuaFuncs);
   }

#endif//U_DIR_DEF


#if defined(_WIN32)

static LARGE_INTEGER s_PcFreq;

LUAFUNC_(NowSeconds)
   {
   double rv;
   LARGE_INTEGER now;
   QueryPerformanceCounter( &now );
   rv = ((double)now.QuadPart) / ((double)s_PcFreq.QuadPart);
   R_num(rv);
   }

void register__windows( lua_State *L )
   {
   static const luaL_reg myLuaFuncs[] =
      {
      LUA_FUNC_I( NowSeconds )
      { 0, 0 }
      };

   luaL_register(L, "_win", myLuaFuncs);

   QueryPerformanceFrequency( &s_PcFreq );
   }

#endif


#define  USE_SHA1  1
#if USE_SHA1  //20081014 kgoodwin

// from
// sha1.c http://www.koders.com/c/fid1A3BFA49A2F9E1FFB3147B7238E287C22E7ED0A3.aspx?s=mdef%3ainsert
// and
// sha1.h http://www.koders.com/c/fid8EC445E3AD2C75D307514001A4DDE9DBC6612C8C.aspx?s=mdef%3ainsert

//-----------------------------------------------------------------------------------  SHA1
/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2003-2006  Christophe Devine
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License, version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */
/*
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <string.h>
#include <stdio.h>


#ifdef LUA_LIB

#ifndef _STD_TYPES
#define _STD_TYPES

#define uchar   unsigned char
#define uint    unsigned int
#define ulong   unsigned long int

#endif

typedef struct
{
    ulong total[2];
    ulong state[5];
    uchar buffer[64];
}
sha1_context;

#else
#include "sha1.h"
#endif


/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                    \
{                                               \
    (n) = ( (ulong) (b)[(i)    ] << 24 )        \
        | ( (ulong) (b)[(i) + 1] << 16 )        \
        | ( (ulong) (b)[(i) + 2] <<  8 )        \
        | ( (ulong) (b)[(i) + 3]       );       \
}
#endif
#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                         \
{                                                    \
    (b)[(i)    ] = ((uchar) (0xFF & ( (n) >> 24 ))); \
    (b)[(i) + 1] = ((uchar) (0xFF & ( (n) >> 16 ))); \
    (b)[(i) + 2] = ((uchar) (0xFF & ( (n) >>  8 ))); \
    (b)[(i) + 3] = ((uchar) (0xFF & ( (n)       ))); \
}
#endif

/*
 * Core SHA-1 functions
 */
void sha1_starts( sha1_context *ctx )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
}

void sha1_process( sha1_context *ctx, uchar data[64] )
{
    ulong temp, W[16], A, B, C, D, E;

    GET_UINT32_BE( W[0],  data,  0 );
    GET_UINT32_BE( W[1],  data,  4 );
    GET_UINT32_BE( W[2],  data,  8 );
    GET_UINT32_BE( W[3],  data, 12 );
    GET_UINT32_BE( W[4],  data, 16 );
    GET_UINT32_BE( W[5],  data, 20 );
    GET_UINT32_BE( W[6],  data, 24 );
    GET_UINT32_BE( W[7],  data, 28 );
    GET_UINT32_BE( W[8],  data, 32 );
    GET_UINT32_BE( W[9],  data, 36 );
    GET_UINT32_BE( W[10], data, 40 );
    GET_UINT32_BE( W[11], data, 44 );
    GET_UINT32_BE( W[12], data, 48 );
    GET_UINT32_BE( W[13], data, 52 );
    GET_UINT32_BE( W[14], data, 56 );
    GET_UINT32_BE( W[15], data, 60 );

    #define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

    #define R(t)                                            \
    (                                                       \
        temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
               W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
        ( W[t & 0x0F] = S(temp,1) )                         \
    )

    #define P(a,b,c,d,e,x)                                  \
    {                                                       \
        e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];

    #define F(x,y,z) (z ^ (x & (y ^ z)))
    #define K 0x5A827999

    P( A, B, C, D, E, W[0]  );
    P( E, A, B, C, D, W[1]  );
    P( D, E, A, B, C, W[2]  );
    P( C, D, E, A, B, W[3]  );
    P( B, C, D, E, A, W[4]  );
    P( A, B, C, D, E, W[5]  );
    P( E, A, B, C, D, W[6]  );
    P( D, E, A, B, C, W[7]  );
    P( C, D, E, A, B, W[8]  );
    P( B, C, D, E, A, W[9]  );
    P( A, B, C, D, E, W[10] );
    P( E, A, B, C, D, W[11] );
    P( D, E, A, B, C, W[12] );
    P( C, D, E, A, B, W[13] );
    P( B, C, D, E, A, W[14] );
    P( A, B, C, D, E, W[15] );
    P( E, A, B, C, D, R(16) );
    P( D, E, A, B, C, R(17) );
    P( C, D, E, A, B, R(18) );
    P( B, C, D, E, A, R(19) );

    #undef K
    #undef F

    #define F(x,y,z) (x ^ y ^ z)
    #define K 0x6ED9EBA1

    P( A, B, C, D, E, R(20) );
    P( E, A, B, C, D, R(21) );
    P( D, E, A, B, C, R(22) );
    P( C, D, E, A, B, R(23) );
    P( B, C, D, E, A, R(24) );
    P( A, B, C, D, E, R(25) );
    P( E, A, B, C, D, R(26) );
    P( D, E, A, B, C, R(27) );
    P( C, D, E, A, B, R(28) );
    P( B, C, D, E, A, R(29) );
    P( A, B, C, D, E, R(30) );
    P( E, A, B, C, D, R(31) );
    P( D, E, A, B, C, R(32) );
    P( C, D, E, A, B, R(33) );
    P( B, C, D, E, A, R(34) );
    P( A, B, C, D, E, R(35) );
    P( E, A, B, C, D, R(36) );
    P( D, E, A, B, C, R(37) );
    P( C, D, E, A, B, R(38) );
    P( B, C, D, E, A, R(39) );

    #undef K
    #undef F

    #define F(x,y,z) ((x & y) | (z & (x | y)))
    #define K 0x8F1BBCDC

    P( A, B, C, D, E, R(40) );
    P( E, A, B, C, D, R(41) );
    P( D, E, A, B, C, R(42) );
    P( C, D, E, A, B, R(43) );
    P( B, C, D, E, A, R(44) );
    P( A, B, C, D, E, R(45) );
    P( E, A, B, C, D, R(46) );
    P( D, E, A, B, C, R(47) );
    P( C, D, E, A, B, R(48) );
    P( B, C, D, E, A, R(49) );
    P( A, B, C, D, E, R(50) );
    P( E, A, B, C, D, R(51) );
    P( D, E, A, B, C, R(52) );
    P( C, D, E, A, B, R(53) );
    P( B, C, D, E, A, R(54) );
    P( A, B, C, D, E, R(55) );
    P( E, A, B, C, D, R(56) );
    P( D, E, A, B, C, R(57) );
    P( C, D, E, A, B, R(58) );
    P( B, C, D, E, A, R(59) );

    #undef K
    #undef F

    #define F(x,y,z) (x ^ y ^ z)
    #define K 0xCA62C1D6

    P( A, B, C, D, E, R(60) );
    P( E, A, B, C, D, R(61) );
    P( D, E, A, B, C, R(62) );
    P( C, D, E, A, B, R(63) );
    P( B, C, D, E, A, R(64) );
    P( A, B, C, D, E, R(65) );
    P( E, A, B, C, D, R(66) );
    P( D, E, A, B, C, R(67) );
    P( C, D, E, A, B, R(68) );
    P( B, C, D, E, A, R(69) );
    P( A, B, C, D, E, R(70) );
    P( E, A, B, C, D, R(71) );
    P( D, E, A, B, C, R(72) );
    P( C, D, E, A, B, R(73) );
    P( B, C, D, E, A, R(74) );
    P( A, B, C, D, E, R(75) );
    P( E, A, B, C, D, R(76) );
    P( D, E, A, B, C, R(77) );
    P( C, D, E, A, B, R(78) );
    P( B, C, D, E, A, R(79) );

    #undef K
    #undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
}

void sha1_update( sha1_context *ctx, uchar *input, uint length )
{
    ulong left, fill;

    if( ! length ) return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += length;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < length )
        ctx->total[1]++;

    if( left && length >= fill )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, fill );
        sha1_process( ctx, ctx->buffer );
        length -= fill;
        input  += fill;
        left = 0;
    }

    while( length >= 64 )
    {
        sha1_process( ctx, input );
        length -= 64;
        input  += 64;
    }

    if( length )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, length );
    }
}

static uchar sha1_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sha1_finish( sha1_context *ctx, uchar digest[20] )
{
    ulong last, padn;
    ulong high, low;
    uchar msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, msglen, 0 );
    PUT_UINT32_BE( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sha1_update( ctx, sha1_padding, padn );
    sha1_update( ctx, msglen, 8 );

    PUT_UINT32_BE( ctx->state[0], digest,  0 );
    PUT_UINT32_BE( ctx->state[1], digest,  4 );
    PUT_UINT32_BE( ctx->state[2], digest,  8 );
    PUT_UINT32_BE( ctx->state[3], digest, 12 );
    PUT_UINT32_BE( ctx->state[4], digest, 16 );
}

/*
 * Output SHA-1(file contents), returns 0 if successful.
 */
#ifdef LUA_LIB
LUAFUNC_(sha1_file)
{
    ulong size;
    const char *filename = S_(1);
    uchar digest[20];

#else
int sha1_file( char *filename, uchar digest[20] )
{
#endif
    FILE *f;
    size_t n;
    sha1_context ctx;
    uchar buf[1024];

    if( ( f = fopen( filename, "rb" ) ) == NULL )
#ifdef LUA_LIB
        RZ;
#else
        return( 1 );
#endif

    sha1_starts( &ctx );

    while( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        sha1_update( &ctx, buf, (uint) n );

    sha1_finish( &ctx, digest );

#ifdef LUA_LIB
    size = 0;
    {
    struct WL(_stat,stat) statbuf;
    if( 0 == WL(_fstat,fstat)( fileno(f), &statbuf ) )
       size = statbuf.st_size;
    }
#endif

    fclose( f );

#ifdef LUA_LIB
    P_lstr( (char *)digest, sizeof digest );  // retval[1]
    P_num( size );                            // retval[2]
    return 2;
#else
    return( 0 );
#endif
}

/*
 * Output SHA-1(buf)
 */
void sha1( uchar *buf, uint buflen, uchar digest[20] )
{
    sha1_context  ctx;
    sha1_starts( &ctx );
    sha1_update( &ctx, buf, buflen );
    sha1_finish( &ctx, digest );
}

void register_sha1( lua_State *L )
   {
   static const luaL_reg myLuaFuncs[] =
      {
      LUA_FUNC_I( sha1_file )
      { 0, 0 }
      };

   luaL_register(L, "sha1", myLuaFuncs);
   }

#ifndef LUA_LIB

/*
 * Output HMAC-SHA-1(key,buf)
 */
void sha1_hmac( uchar *key, uint keylen, uchar *buf, uint buflen,
                uchar digest[20] )
{
    uint i;
    sha1_context ctx;
    uchar k_ipad[64];
    uchar k_opad[64];
    uchar tmpbuf[20];

    memset( k_ipad, 0x36, 64 );
    memset( k_opad, 0x5C, 64 );

    for( i = 0; i < keylen; i++ )
    {
        if( i >= 64 ) break;

        k_ipad[i] ^= key[i];
        k_opad[i] ^= key[i];
    }

    sha1_starts( &ctx );
    sha1_update( &ctx, k_ipad, 64 );
    sha1_update( &ctx, buf, buflen );
    sha1_finish( &ctx, tmpbuf );

    sha1_starts( &ctx );
    sha1_update( &ctx, k_opad, 64 );
    sha1_update( &ctx, tmpbuf, 20 );
    sha1_finish( &ctx, digest );

    memset( k_ipad, 0, 64 );
    memset( k_opad, 0, 64 );
    memset( tmpbuf, 0, 20 );
    memset( &ctx, 0, sizeof( sha1_context ) );
}

#endif

#ifdef SELF_TEST
/*
 * FIPS-180-1 test vectors
 */
static char *sha1_test_str[3] =
{
    "abc",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    NULL
};

static uchar sha1_test_sum[3][20] =
{
    { 0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
      0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D },
    { 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
      0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1 },
    { 0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E,
      0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F }
};

/*
 * Checkup routine
 */
int sha1_self_test( void )
{
    int i, j;
    uchar buf[1000];
    uchar sha1sum[20];
    sha1_context ctx;

    for( i = 0; i < 3; i++ )
    {
        printf( "  SHA-1 test #%d: ", i + 1 );

        sha1_starts( &ctx );

        if( i < 2 )
            sha1_update( &ctx, (uchar *) sha1_test_str[i],
                         strlen( sha1_test_str[i] ) );
        else
        {
            memset( buf, 'a', 1000 );
            for( j = 0; j < 1000; j++ )
                sha1_update( &ctx, (uchar *) buf, 1000 );
        }

        sha1_finish( &ctx, sha1sum );

        if( memcmp( sha1sum, sha1_test_sum[i], 20 ) != 0 )
        {
            printf( "failed\n" );
            return( 1 );
        }

        printf( "passed\n" );
    }

    printf( "\n" );
    return( 0 );
}
#else
int sha1_self_test( void )
{
    printf( "SHA-1 self-test not available\n\n" );
    return( 1 );
}
#endif

//-----------------------------------------------------------------------------------  SHA1

#endif


LUALIB_API int luaopen_klib (lua_State *L)
   {
#if K_BIN_DEF
   register_k_bin( L );
#endif

#if U_DIR_DEF
   register__dir( L );
#endif

#if defined(_WIN32)
   register__windows( L );
#endif

#if USE_SHA1  //20081014 kgoodwin
   register_sha1( L );
#endif

   return 1;
   }

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

unsigned GetUint_Hack( lua_State *L, int n ) {
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

LUAFUNC_(replace) {  // in S1, replace S2 with S3, return replaced string, # of replacements
   size_t inBytes, srchBytes, rplcBytes, bytesLeft;

   const char *pIn   = Bytes_( 1, inBytes   );
   const char *pSrch = Bytes_( 2, srchBytes );
   const char *pRplc = Bytes_( 3, rplcBytes );

#if 1
   if( srchBytes == rplcBytes ) {
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
      while( bytesLeft >= srchBytes ) {
         const char *pStart = pIn + (inBytes - bytesLeft);
         const char *pMatch = memchr( pStart, *pSrch, bytesLeft );
         // fprintf( stderr, "%d# 0x%02X @%d\n", rplcCount, pStart - pIn );
         if( pMatch ) {
            const size_t numBytes = (pMatch - pStart);
            bytesLeft -= numBytes;
            memcpy( pC, pStart, numBytes );
            pC += numBytes;
            // fprintf( stderr, "%d# @%X C  @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, pMatch - pStart );

            if( (0 == memcmp( pMatch, pSrch, srchBytes )) ) {
               bytesLeft -= srchBytes;
               // fprintf( stderr, "%d# @%X -> @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, rplcBytes );
               memcpy( pC, pRplc, rplcBytes );
               pC += rplcBytes;
               ++rplcCount;
               }
            else {
               *pC++ = *pMatch;
               --bytesLeft;
               }
            }
         else {
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
      while( bytesLeft >= srchBytes ) {
         const char *pStart = pIn + (inBytes - bytesLeft);
         const char *pMatch = memchr( pStart, *pSrch, bytesLeft );
         // fprintf( stderr, "%d# 0x%02X @%d\n", rplcCount, pStart - pIn );
         if( pMatch ) {
            bytesLeft -= (pMatch - pStart);
            luaL_addlstring( &buf, pStart, pMatch - pStart );
            // fprintf( stderr, "%d# @%X C  @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, pMatch - pStart );

            if( (0 == memcmp( pMatch, pSrch, srchBytes )) ) {
               bytesLeft -= srchBytes;
               // fprintf( stderr, "%d# @%X -> @%X cpy %X\n", rplcCount, pStart - pIn, pMatch - pIn, rplcBytes );
               luaL_addlstring( &buf, pRplc , rplcBytes       );
               ++rplcCount;
               }
            else {
               luaL_addlstring( &buf, pMatch, 1 );
               --bytesLeft;
               }
            }
         else {
            break;
            }
         }

      luaL_addlstring( &buf, pIn + (inBytes - bytesLeft), bytesLeft );
      luaL_pushresult( &buf );
      lua_pushinteger(L, rplcCount);  /* number of substitutions */
      return 2;
      }
   }


void register_k_bin( lua_State *L ) {
   static const luaL_reg myLuaFuncs[] = {
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

static void tr1( char *buf, int from, int to ) {
   int ix;
   for( ix=0 ; buf[ix] ; ++ix )
      if( buf[ix] == from )
          buf[ix] =  to;
   }

static void tr_pathsep( char *buf, int pathsep ) {
   tr1( buf, (pathsep == '/') ? '\\' : '/', pathsep );
   }

#endif

LUAFUNC_(chdir) {
   const char *dirnm = S_(1);
   const int ok = !( WL( _chdir, chdir )( dirnm ) == -1);
   return push_errno_result(L, ok, dirnm);
   }

LUAFUNC_(create) {
   const char *dirnm = S_(1);
   const int ok = (WL( _mkdir( dirnm ), mkdir( dirnm, 0777 ) ) != -1);
   return push_errno_result(L, ok, dirnm);
   }

// 20061212 kgoodwin I HATE that Win32 API's insist on capitalizing the drive letter!
#define  FIX_DRIVE_LETTER( buf )  do { if( buf[0] && buf[1] == ':' )  buf[0] = tolower( buf[0] ); } while( 0 )

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

LUAFUNC_(fullpath) {
   const char *pnm = S_(1);
#if defined(_WIN32)
   char buf[ _MAX_PATH+1 ];
   char pathsep = pathsep_of_path( pnm );
   if( !_fullpath( buf, pnm, sizeof buf ) ) {
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

typedef struct {
   lua_State *L;
   int        ix;
#if defined(_WIN32)
   char       pathsep;
#endif
   int        fDirsOnly;
   } scand_shvars;

#if defined(_WIN32)

static void scandir_( const char *dirname, int recurse, scand_shvars *pSdsv ) {

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
   if( dnLen && (dirname[dnLen-1] == '\\' || dirname[dnLen-1] == '/') ) {
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
      if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
         if( (strcmp(fd.cFileName, ".") != 0) && (strcmp(fd.cFileName, "..") != 0) ) {
            char buf2[ _MAX_PATH+1 ];
            _snprintf( buf2, sizeof buf2, "%s%c", buf, pSdsv->pathsep );
            A_PUSH( buf2 );
            if( recurse )  // Get the full path for sub directory
               scandir_( buf, 1, pSdsv );
            }
         }
      else {  // it's a FILE
         if( !pSdsv->fDirsOnly ) {
            A_PUSH( buf );
            }
         }
      } while( !(FindNextFile(hList, &fd) == 0 && GetLastError() == ERROR_NO_MORE_FILES) );
                 // failure                    && failure

   FindClose(hList);
   }

#else

static void scandir_( const char *dirname, int recurse, scand_shvars *pSdsv ) {
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
                  if( len+1 > sizeof( buf )-1 ) {
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
               else { // it's a FILE
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
static int read_names_( lua_State *L, int fDirsOnly ) {
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

LUAFUNC_(read_names)    { return read_names_( L, 0 ); }
LUAFUNC_(read_dirnames) { return read_names_( L, 1 ); }

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
   if( len+1 > sizeof( buf )-1 )  { R_nil(); }
   buf[len  ] = sep;
   buf[len+1] = 0  ;
   R_str( buf );
#else
   char *mallocd_cwd = getcwd( NULL, 0 );
   // This code is relying on GNU-extension-specific behavior of getcwd, which
   // specifies that a NULL first parameter will cause the function to return a
   // pointer to a mallocd block which the caller must free.
   if( !mallocd_cwd ) { R_nil(); }  // CID186315 protect against non-GNU behavior
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
      LUA_FUNC_I( pathsep_os       )
      LUA_FUNC_I( rmdir         ) // os.remove will not remove directories on Win32; this does!
      { 0, 0 }
      };

   luaL_register(L, "_dir", myLuaFuncs);
   }

#endif//U_DIR_DEF


#if defined(_WIN32)

static LARGE_INTEGER s_PcFreq;

LUAFUNC_(NowSeconds) {
   double rv;
   LARGE_INTEGER now;
   QueryPerformanceCounter( &now );
   rv = ((double)now.QuadPart) / ((double)s_PcFreq.QuadPart);
   R_num(rv);
   }

void register__windows( lua_State *L ) {
   static const luaL_reg myLuaFuncs[] = {
      LUA_FUNC_I( NowSeconds )
      { 0, 0 }
      };

   luaL_register(L, "_win", myLuaFuncs);

   QueryPerformanceFrequency( &s_PcFreq );
   }

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

   return 1;
   }

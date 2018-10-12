//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include "lua_intf_common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

//####  lh_ = my Lua Helper functions

STATIC_FXN int lh_rtnStrOrNil( lua_State *L, PCChar src ) {
   if( src ) { lua_pushstring( L, src ); }
   else      { lua_pushnil( L );         }
   return 1;
   }

STATIC_FXN int lh_rtnStr( lua_State *L, PCChar src ) {
   lua_pushstring( L, src ? src : "" );
   return 1;
   }

namespace LExFx { // exported functions
   STATIC_FXN int HideCursor( lua_State *L )   { ::HideCursor()  ; RZ; }
   STATIC_FXN int UnhideCursor( lua_State *L ) { ::UnhideCursor(); RZ; }
   STATIC_FXN int DispRefreshWholeScreenNow( lua_State *L ) { ::DispRefreshWholeScreenNow(); RZ; }
   STATIC_FXN int GetKey( lua_State *L )       {
      std::string keyNm;
      while(1) {
         const PCCMD pCmd( CmdFromKbdForInfo( keyNm ) );
         if( pCmd && !keyNm.empty() ) {
            R_str( keyNm.c_str() );
            }
         }
      }

   STATIC_FXN int StartShellExecuteProcess( lua_State *L ) {  ::StartShellExecuteProcess( S_(1), So0_(2) ); RZ; }
   STATIC_FXN int StartConProcess( lua_State *L )    { R_int( ::StartConProcess( S_(1) ) ); }
   STATIC_FXN int StartGuiProcess( lua_State *L )    { R_int( ::StartGuiProcess( S_(1) ) ); }
   STATIC_FXN int OsErrStr( lua_State *L )           { linebuf lb; ::OsErrStr( BSOB(lb) ); R_str( lb ); }
   STATIC_FXN int OsName( lua_State *L )             { R_str( ::OsName() ); }
   STATIC_FXN int OsVer( lua_State *L )              { R_str( ::OsVerStr() ); }
   STATIC_FXN int rmargin( lua_State *L )            { R_int( g_iRmargin ); }

   STATIC_FXN int RsrcFilename( lua_State *L )       { R_str( ::RsrcFilename ( S_(1) ).c_str() ); }
   STATIC_FXN int StateFilename( lua_State *L )      { R_str( ::StateFilename( S_(1) ).c_str() ); }

   STATIC_FXN int Bell( lua_State *L )               { ConOut::Bell(); RZ; }
   STATIC_FXN int ScreenLines( lua_State *L )        { R_int( ::ScreenLines() ); }
   STATIC_FXN int ScreenCols( lua_State *L )         { R_int( ::ScreenCols () ); }
   STATIC_FXN int DirectVidClear( lua_State *L )     { ::DirectVidClear(); RZ; }
   STATIC_FXN int DirectVidWrStrColorFlush( lua_State *L ) { 0 && DBG("%s: %d, %d", __func__, I_(1), I_(2) );
      ::DirectVidWrStrColorFlush( I_(1), I_(2), S_(3), I_(4) );
      RZ;
      }
   STATIC_FXN int Path_CommonPrefixLen( lua_State *L ) { R_int( Path::CommonPrefixLen( S_(1), S_(2) ) ); }

   STATIC_FXN int AddToSearchLog( lua_State *L )     { ::AddToSearchLog( S_(1) ); RZ; }
   STATIC_FXN int l_AssignStrOk( lua_State *L )      { R_bool( ::AssignStrOk( S_(1) ) ); }
   STATIC_FXN int PushVariableMacro( lua_State *L )  { R_bool( ::PushVariableMacro( S_(1) ) ); }
   STATIC_FXN int CmdIdxAddLuaFunc( lua_State *L )   { ::CmdIdxAddLuaFunc( S_(1), fn_runLua(), I_(2)  _AHELP( S_(3) ) ); RZ; }
   STATIC_FXN int BindKeyToCMD( lua_State *L )       { R_bool( ::BindKeyToCMD( S_(1), S_(2) ) ); }
   STATIC_FXN int fExecute( lua_State *L )           { R_bool( ::fExecute( S_(1) ) ); }
   STATIC_FXN int fChangeFile( lua_State *L )        { R_bool( ::fChangeFile( S_(1), true ) ); }
   STATIC_FXN int DBG( lua_State *L )                { ::DBG( "%s", S_(1) ); RZ; }
   STATIC_FXN int Msg( lua_State *L )         {
      const auto p1( S_(1) );
      s_pFbufLuaLog->FmtLastLine( "*** %s", p1 );
      ::Msg( "%s", p1 );
      RZ;
      }

   STATIC_FXN void push_mac_def( lua_State *L, PCChar macroName ) {
      std::string val( std::string(macroName) + ":=" + DupTextMacroValue( macroName ) );
      lua_pushlstring( L, val.data(), val.length() );
      }

   STATIC_FXN int GetDynMacros( lua_State *L ) {
      lua_newtable(L);  // result
      auto tblIdx( 0u );
      push_mac_def( L, "curfile"     );                      lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "curfileext"  );                      lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "curfilename" );                      lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "curfilepath" );                      lua_rawseti( L, -2, ++tblIdx );
      lua_pushstring( L, "------------------------------" ); lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "mffile"  );                          lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "mfspec"  );                          lua_rawseti( L, -2, ++tblIdx );
      push_mac_def( L, "mfspec_" );                          lua_rawseti( L, -2, ++tblIdx );
      std::string val( "<mfspec>" );
      const auto fb( FindFBufByName( val.c_str() ) );
      if( fb ) {
         if( FBOP::IsBlank( fb ) ) { val.append( " exists but empty" ); }
         else                      { val.append( FmtStr<31>( " %d lines", fb->LineCount() ) ); }
         }
      else                         { val.append( " does not exist" ); }
      lua_pushlstring( L, val.data(), val.length() );        lua_rawseti( L, -2, ++tblIdx );
      return 1;  // return the table
      }

   STATIC_FXN int GetCwd( lua_State *L )     { R_str( Path::GetCwd_ps().c_str() ); }
   STATIC_FXN int Path_Dirnm( lua_State *L ) { const auto rv( Path::RefDirnm(    S_(1) ) ); R_lstr( rv.data(), rv.length() ); }
   STATIC_FXN int Path_Fnm( lua_State *L )   { const auto rv( Path::RefFnm(      S_(1) ) ); R_lstr( rv.data(), rv.length() ); }
   STATIC_FXN int Path_Ext( lua_State *L )   { const auto rv( Path::RefExt(      S_(1) ) ); R_lstr( rv.data(), rv.length() ); }
   STATIC_FXN int Path_FnameExt( lua_State *L ) { const auto rv( Path::RefFnameExt( S_(1) ) ); R_lstr( rv.data(), rv.length() ); }
   STATIC_FXN int GetChildDirs( lua_State *L ) {
      lua_newtable(L);  // result
      DirListGenerator dlg( __PRETTY_FUNCTION__ );
      Path::str_t xb;
      for( int tblIdx=1 ; dlg.VGetNextName( xb ) ; ++tblIdx ) {
         lua_pushstring( L, xb.c_str() );
         lua_rawseti( L, -2, tblIdx );
         }
      return 1;  // return the table
      }

   STATIC_FXN int Get_SearchCaseSensitive( lua_State *L )      { R_bool( g_fCase ); }
   STATIC_FXN int Getenv( lua_State *L )      { return lh_rtnStr(      L, getenv( S_(1) ) ); }
   STATIC_FXN int GetenvOrNil( lua_State *L ) { return lh_rtnStrOrNil( L, getenv( S_(1) ) ); }
   STATIC_FXN int Putenv( lua_State *L )      {
      const auto    param1( S_(1) );
      const auto    param2( So0_(2) );
      if( param2 )  PutEnvOk( param1, param2 );
      else          PutEnvOk( param1 );
      RZ;
      }
   STATIC_FXN int Clipboard_PutText( lua_State *L )     { ::Clipboard_PutText( S_(1) ); RZ; }
   STATIC_FXN int MarkDefineAtCurPos( lua_State *L )    { ::MarkDefineAtCurPos( S_(1) ); RZ; }
   STATIC_FXN int MarkGoto( lua_State *L )              { R_bool( ::MarkGoto( S_(1) ) ); }
   STATIC_FXN int ModifyTimeOfDiskFile( lua_State *L )  {
      const auto fstat_( GetFileStat( S_(1) ) );
      R_lstr( PCChar(&fstat_), sizeof(fstat_) );
      }

   STATIC_FXN int valueof( lua_State *L )     {  0 && ::DBG( "%s: '%s'", __func__, S_(1) );
      lua_getglobal( L, S_(1) ); // returns Nil if nosuch variable (how to discriminate vs. undefined variable?  I think we can't)
      return 1;
      }

   #define  ROUNDUP_TO_NEXT_POWER2( val, pow2 )  ((val + ((pow2)-1)) & ~((pow2)-1))

   STATIC_FXN int IsFile( lua_State *L )  { R_bool( ::IsFile( S_(1) ) ); }
   STATIC_FXN int IsDir( lua_State *L )   { R_bool( ::IsDir(  S_(1) ) ); }

   STATIC_FXN int SleepMs( lua_State *L )   { ::SleepMs( U_(1) ); RZ; }

   } // namespace LExFx

//------------------------------------------------------------------------------
typedef PCChar (*TLuaSplitFxn)( PCChar pszToSearch, PCChar pszToSearchFor, int *matchLen );

STATIC_FXN PCChar StrToNextOrEos_( PCChar pszToSearch, PCChar pszToSearchFor, int *matchLen ) {
   *matchLen = 1;
   return StrToNextOrEos( pszToSearch, pszToSearchFor );
   }

STATIC_FXN PCChar FindStr_( PCChar pszToSearch, PCChar pszToSearchFor, int *matchLen ) { 0 && DBG( "'%s' -> '%s'", pszToSearchFor, pszToSearch );
   const PCChar match( strstr( pszToSearch, pszToSearchFor ) );
   if( match ) {
      *matchLen = Strlen( pszToSearchFor );
      return match;
      }
   return Eos( pszToSearch );
   }

// why these functions exist: the ONLY Lua std string library search function
// that takes a non-pattern search key string is string.find, and that only if a
// trailing bool param is passed as true. string.find is quite clumsy (basically,
// character-indexed operations, same as we're doing below), leading to creation
// of intermediate tables in many cases, so we just do the simple stuff here in
// C with maximum efficiency, and plain-string search key strings lead to more
// readable Lua code.
//
// NB: lua_split_rtn_mult can return a potentially unlimited number of values
//     on the lua_State stack which is a fixed size which we can overflow!!!

STATIC_FXN int lua_split_rtn_mult( lua_State *L, TLuaSplitFxn sf ) {
   auto strToSplit = S_(1);
   auto sep = S_(2);
   auto ix  = 1;
   // repeat for each separator
   PCChar pastEnd;
   int sepLen(0);
   while( *(pastEnd=sf( strToSplit, sep, &sepLen )) != '\0' ) {
      lua_pushlstring( L, strToSplit, pastEnd - strToSplit );  // push substring
      ++ix;
      strToSplit = pastEnd + sepLen;  // skip separator
      }
   // push last substring
   lua_pushstring( L, strToSplit );
   0 && DBG( "%s rtns %d", __func__, ix );
   return ix;  // return number of strings pushed
   }

STATIC_FXN int lua_split_rtn_tbl( lua_State *L, TLuaSplitFxn sf ) {
   auto strToSplit = S_(1);
   auto sep = S_(2);
   auto ix  = 1;
   lua_newtable(L);  // result
   // repeat for each separator
   PCChar pastEnd;
   int sepLen(0);
   while( *(pastEnd=sf( strToSplit, sep, &sepLen )) != '\0' ) {
      lua_pushlstring( L, strToSplit, pastEnd - strToSplit );  // push substring
      lua_rawseti( L, -2, ix );
      ++ix;
      strToSplit = pastEnd + sepLen;  // skip separator
      }
   // push last substring
   lua_pushstring( L, strToSplit );
   lua_rawseti( L, -2, ix );
   return 1;  // return the table
   }

namespace LExFx {
   STATIC_FXN int split_ch( lua_State *L )      { return lua_split_rtn_mult( L, StrToNextOrEos_ ); }
   STATIC_FXN int split_ch_tbl( lua_State *L )  { return lua_split_rtn_tbl(  L, StrToNextOrEos_ ); }
   STATIC_FXN int split_str( lua_State *L )     { return lua_split_rtn_mult( L, FindStr_ );        }
   STATIC_FXN int split_str_tbl( lua_State *L ) { return lua_split_rtn_tbl(  L, FindStr_ );        }

   STATIC_FXN PChar nib2bitstr_( PChar p5, int nib ) {
      p5[4] = '\0';
      for( int ix=3; ix>=0; --ix ) {
         p5[ix] = (nib & 1) ? '1' : '0';
         nib >>= 1;
         }
      return p5;
      }

   STATIC_FXN int hexstr2bitstr( lua_State *L ) {
      auto inst = S_(1);
      if( inst[0] == '0' && tolower(inst[1]) == 'x' ) { inst += 2; } // skip redundant "hex prefix"
      const int xdigits( consec_xdigits( inst ) );
      if( 0 == xdigits ) { luaL_error(L, "string '%s' contains no leading hexits", inst); }
      luaL_Buffer lb; luaL_buffinit(L, &lb);
      for( int ix=0; ix < xdigits; ++ix ) {
         const uint8_t ch( tolower( inst[ix] ) );
         const int val( ch - (ch < 'a' ? '0' : 'a' - 10) );  0 && ::DBG( ":: %X", val );
         char buf5[5];
         luaL_addstring( &lb, nib2bitstr_( buf5, val ) );    0 && ::DBG( ":: %s L %d", buf5, Strlen(buf5) );
         }
      luaL_pushresult( &lb );
      return 1;
      }

   STATIC_CONST char b2x[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

   STATIC_FXN int bitstr2hexstr( lua_State *L ) {
      auto inst = S_(1);
      luaL_Buffer b; luaL_buffinit(L, &b);
      if( inst[0] == '0' && tolower(inst[1]) == 'b' ) { inst += 2; } // skip redundant "bin prefix"
      const auto bdigits( consec_bdigits( inst ) );
      const auto outchars( (bdigits / 4) + ((bdigits % 4) != 0) );
      const PChar outbuf = PChar( alloca( outchars+1 ) );
      outbuf[outchars] = '\0';
      auto pb = outbuf + outchars;
      auto pc = inst + bdigits;
      auto bits = 0;
      auto bbuf = 0;
      for( auto ix(0); ix<bdigits; ++ix ) {
         auto val( *(--pc) - '0' ); 0 && ::DBG( "val = %X", val );
         bbuf |= (val <<= bits);
         if( ++bits == 4 ) {
            *(--pb) = b2x[bbuf];
            bits = bbuf = 0;
            }
         }
      if( bits > 0 ) {
         *(--pb) = b2x[bbuf];
         }
      Assert( pb >= outbuf );
      R_str( outbuf );
      }

   STATIC_FXN int enqueue_compile_jobs( lua_State *L ) {
      luaL_checktype(L, 1, LUA_TTABLE); // 1st argument must be a table (t)
      StringList sl;
      const auto jobs( luaL_getn(L, 1) );
      for( auto ix(1) ; ix <= jobs ; ++ix ) {
         lua_rawgeti(L, 1, ix);  // push t[ix]
         PCChar pCmd = luaL_checkstring(L, -1);
         0 && ::DBG( "%s", pCmd );
         sl.push_front( pCmd );
         }
      CompilePty_CmdsAsyncExec( sl, true );
      RZ;
      }
   } // namespace LExFx

STATIC_CONST char KevinsMetatable_FBUF[] = "KevinsMetatable.FBUF";
struct lua_FBUF { PFBUF pFBuf; };

STATIC_FXN int l_FBUF_function_getmetatable( lua_State *L ) {
   luaL_getmetatable( L, KevinsMetatable_FBUF );
   return 1;
   }

int l_construct_FBUF( lua_State *L, PFBUF pFBuf ) {
   if( !pFBuf ) { R_nil(); }
   const auto ud( static_cast<lua_FBUF *>( lua_newuserdata( L, sizeof( lua_FBUF ) ) ) );
   ud->pFBuf = pFBuf;
   l_FBUF_function_getmetatable( L );
   lua_setmetatable( L, -2 );
   return 1;
   }

STATIC_FXN int l_FBUF_function_new( lua_State *L ) {
   const auto filename( So0_(1) );
   return l_construct_FBUF( L, filename ? OpenFileNotDir_NoCreate( filename ) : g_CurFBuf() );
   }

STATIC_FXN int l_FBUF_function_new_may_create( lua_State *L ) {
   const auto filename( So0_(1) );
   return l_construct_FBUF( L, OpenFileNotDir_CreateSilently( filename ) );
   }

STATIC_FXN int l_FBUF_function_curview( lua_State *L ) {
   int    rv = l_construct_FBUF( L, g_CurFBuf() );
   return rv + l_construct_View( L, g_CurView() );
   }

#if FBUF_TREE
STATIC_FXN int l_FBUF_function_first( lua_State *L ) {
           RbNode *pNd( rb_first( g_FBufIdx ) );
           return (pNd != rb_nil(g_FBufIdx)) ? l_construct_FBUF( L, static_cast<PFBUF>(rb_val( pNd )) ) : 0;
           }
#else
STATIC_FXN int l_FBUF_function_first( lua_State *L )     { return l_construct_FBUF( L, g_FBufHead.front() ); }
#endif

STATIC_FXN int l_FBUF_function_getlog( lua_State *L )    { return l_construct_FBUF( L, s_pFbufLuaLog      ); }

STATIC_FXN PFBUF l_Get_FBUF( lua_State *L, int stacklvl=1 ) {
   const auto rv( static_cast<lua_FBUF *>( luaL_checkudata( L, stacklvl, KevinsMetatable_FBUF ) ) );
   luaL_argcheck( L, rv, stacklvl, "'PFBUF' expected" );
   return rv->pFBuf;
   }

#define  thisPF()   l_Get_FBUF( L )
#define  PFBUF_(n)  l_Get_FBUF( L, n )

namespace LFBUF {
   STATIC_FXN int __tostring( lua_State *L )               { R_strf( "FBUF(\"%s\")", thisPF()->Name() ); }
   STATIC_FXN int Next( lua_State *L )                     { return l_construct_FBUF( L, thisPF()->Next()     ); }

   STATIC_FXN int ClearUndo( lua_State *L )                { thisPF()->ClearUndo()                                              ; RZ; }
   STATIC_FXN int ClrNoEdit( lua_State *L )                { thisPF()->ClrNoEdit()                                              ; RZ; }
   STATIC_FXN int CopyLines( lua_State *L )                { FBOP::CopyLines( thisPF(), I_(2)-1, PFBUF_(3), I_(4)-1, I_(5)-1 )  ; RZ; }
   STATIC_FXN int DiscardTrailSpcs( lua_State *L )         { thisPF()->DiscardTrailSpcs()                                       ; RZ; }
   STATIC_FXN int InsBlankLinesBefore( lua_State *L )      { thisPF()->InsBlankLinesBefore( I_(2)-1, Io_( 3, 1 ) )              ; RZ; }
   STATIC_FXN int InsLine( lua_State *L )                  { std::string tmp; thisPF()->InsLineEntab( I_(2)-1, S_(3), tmp )     ; RZ; }
   STATIC_FXN int InsLineSortedAscending( lua_State *L )   { const PCChar st = S_(2);
                                                             FBOP::InsLineSortedAscending( thisPF(), Io_(3,1)-1, st ); RZ;
                                                           }
   STATIC_FXN int InsLineSortedDescending( lua_State *L )  { const PCChar st = S_(2);
                                                             FBOP::InsLineSortedDescending( thisPF(), Io_(3,1)-1, st ); RZ;
                                                           }
   STATIC_FXN int IsGrepBuf( lua_State *L )                { Path::str_t searchedFnm; int metaLines;
                                                             if( FBOP::IsGrepBuf( searchedFnm, &metaLines, thisPF() ) ) {
                                                                P_str(searchedFnm.c_str()) ; P_int(metaLines) ; return 2;
                                                                }
                                                             RZ;
                                                           }
   STATIC_FXN int KeepTrailSpcs( lua_State *L )            { thisPF()->KeepTrailSpcs()                                    ; RZ; }
   STATIC_FXN int MakeEmpty( lua_State *L )                { thisPF()->MakeEmpty()                                        ; RZ; }
   STATIC_FXN int PutLastLine( lua_State *L )              { thisPF()->PutLastMultilineRaw( S_(2) )                       ; RZ; }
   STATIC_FXN int PutLine( lua_State *L )                  { std::string tmp; thisPF()->PutLineEntab( I_(2)-1, S_(3), tmp ); RZ; }
   STATIC_FXN int MoveCursorToBofAllViews( lua_State *L )  { thisPF()->MoveCursorToBofAllViews()                          ; RZ; }
   STATIC_FXN int SetAutoRead( lua_State *L )              { thisPF()->SetAutoRead()                                      ; RZ; }
   STATIC_FXN int SetBackupMode_Bak( lua_State *L )        { thisPF()->SetBackupMode(bkup_BAK)                            ; RZ; }
   STATIC_FXN int SetBackupMode_None( lua_State *L )       { thisPF()->SetBackupMode(bkup_NONE)                           ; RZ; }
   STATIC_FXN int SetBackupMode_Undel( lua_State *L )      { thisPF()->SetBackupMode(bkup_UNDEL)                          ; RZ; }
   STATIC_FXN int SetBlockRsrcLd( lua_State *L )           { thisPF()->SetBlockRsrcLd()                                   ; RZ; }
   STATIC_FXN int SetForgetOnExit( lua_State *L )          { thisPF()->SetForgetOnExit()                                  ; RZ; }
   STATIC_FXN int SetNoEdit( lua_State *L )                { thisPF()->SetNoEdit()                                        ; RZ; }
   STATIC_FXN int SetRememberAfterExit( lua_State *L )     { thisPF()->SetRememberAfterExit()                             ; RZ; }
   #ifdef fn_su
   STATIC_FXN int SetSilentUpdateMode( lua_State *L )      { thisPF()->SetSilentUpdateMode()                              ; RZ; }
   #endif
   STATIC_FXN int ToglNoEdit( lua_State *L )               { thisPF()->ToglNoEdit()                                       ; RZ; }
   STATIC_FXN int UnDirty( lua_State *L )                  { thisPF()->UnDirty()                                          ; RZ; }
   STATIC_FXN int WriteToDisk( lua_State *L )              { thisPF()->WriteToDisk( So0_(2) )                             ; RZ; }
   STATIC_FXN int cat( lua_State *L )                      { thisPF()->cat( S_(2) )                                       ; RZ; }
   STATIC_FXN int SetEntabOk( lua_State *L )               { R_bool(      thisPF()->SetEntabOk( I_(2) )         ); }
   STATIC_FXN int RefreshFailedShowError( lua_State *L )   { R_bool( 0 != thisPF()->RefreshFailedShowError()    ); }
   STATIC_FXN int IsAutoRead( lua_State *L )               { R_bool( 0 != thisPF()->IsAutoRead()                ); }
   STATIC_FXN int RefreshFailed( lua_State *L )            { R_bool( 0 != thisPF()->RefreshFailed()             ); }
   STATIC_FXN int IsDirty( lua_State *L )                  { R_bool( 0 != thisPF()->IsDirty()                   ); }
   #ifdef fn_su
   STATIC_FXN int SilentUpdateMode( lua_State *L )         { R_bool( 0 != thisPF()->SilentUpdateMode()          ); }
   #endif
   #if defined(_WIN32)
   STATIC_FXN int IsDiskRO( lua_State *L )                 { R_bool( 0 != thisPF()->IsDiskRO()                  ); }
   #else
   STATIC_FXN int IsDiskRO( lua_State *L )                 { R_bool( false ); }
   #endif
   STATIC_FXN int ToForgetOnExit( lua_State *L )           { R_bool( 0 != thisPF()->ToForgetOnExit()            ); }
   STATIC_FXN int IsPseudo( lua_State *L )                 { R_bool( 0 != thisPF()->FnmIsPseudo()               ); }
   STATIC_FXN int IsSysPseudo( lua_State *L )              { R_bool( 0 != thisPF()->IsSysPseudo()               ); }
   STATIC_FXN int IsRsrcLdBlocked( lua_State *L )          { R_bool( 0 != thisPF()->IsRsrcLdBlocked()           ); }
   STATIC_FXN int IsNoEdit( lua_State *L )                 { R_bool( 0 != thisPF()->IsNoEdit()                  ); }
   #if defined(_WIN32)
   STATIC_FXN int MakeDiskFileWritable( lua_State *L )     { R_bool( 0 != thisPF()->MakeDiskFileWritable()      ); }
   #else
   STATIC_FXN int MakeDiskFileWritable( lua_State *L )     { R_bool( false ); }
   #endif
   STATIC_FXN int TrailSpcsKept( lua_State *L )            { R_bool( 0 != thisPF()->TrailSpcsKept()             ); }
   STATIC_FXN int CantModify( lua_State *L )               { R_bool( 0 != thisPF()->CantModify()                ); }
   STATIC_FXN int LineCount( lua_State *L )                { R_int(       thisPF()->LineCount()                 ); }
   STATIC_FXN int LastLine( lua_State *L )                 { R_int(       thisPF()->LastLine()+1                ); }
   STATIC_FXN int TabWidth( lua_State *L )                 { R_int(       thisPF()->TabWidth()                  ); }
   STATIC_FXN int ModifyTimeOfDiskFile( lua_State *L )     {
                                     const auto fstat_( thisPF()->GetLastFileStat() );
                                     R_lstr( PCChar(&fstat_), sizeof(fstat_) );
                                   }
   STATIC_FXN int Name( lua_State *L )                     { R_str(       thisPF()->Name()                      ); }
   STATIC_FXN int DelLine( lua_State *L )                  { const LINE L1( I_(2)-1 ); thisPF()->DelLines( L1, Io_( 3, L1+1)-1 ); RZ; }

   STIL bool isValidLineNum( PFBUF fb, LINE lnum )  { return lnum >= 0 && lnum <= fb->LastLine(); }

   STATIC_FXN int GetLine( lua_State *L ) {
      const auto pf( thisPF() );
      const auto lnum( I_(2) - 1 );
      if( isValidLineNum( pf, lnum ) ) {
         std::string tmp;
         pf->DupLineLua( tmp, lnum );
         R_lstr( tmp.c_str(), tmp.length() );
         }
      R_nil();
      }

   STATIC_FXN int GetLineRaw( lua_State *L ) {
      const auto pf( thisPF() );
      const int lnum( I_(2) - 1 );
      if( isValidLineNum( pf, lnum ) ) {
         const auto rl( pf->PeekRawLine( lnum ) );
         R_lstr( rl.data(), rl.length() );
         }
      R_nil();
      }

   STATIC_FXN int FTypeEq( lua_State *L ) { R_bool( thisPF()->FTypeEq( S_(2) ) ); }

   STATIC_FXN int GetLineSeg( lua_State *L ) {
      const auto pf( thisPF() );
      const auto lnum( I_(2) - 1 );
      if( isValidLineNum( pf, lnum ) ) {
         const auto xLeftIncl ( I_(3) - 1 );
         const auto xRightIncl( I_(4) - 1 );
         const auto rl( pf->PeekRawLineSeg( lnum, xLeftIncl, xRightIncl ) );
         R_lstr( rl.data(), rl.length() );
         }
      R_nil();
      }

   STATIC_FXN int PutLineSeg( lua_State *L ) { std::string t0,t1; thisPF()->PutLineSeg( I_(2)-1, Sr_(L,3), t0,t1, I_(4)-1, I_(5)-1, LuaBool(6) ); RZ; }

   STATIC_FXN int ExpandWildcardSorted( lua_State *L ) { FBOP::ExpandWildcardSorted  ( thisPF(), S_(2) ); RZ; }
   STATIC_FXN int ExpandWildcardUnsorted( lua_State *L ) { FBOP::ExpandWildcardUnsorted( thisPF(), S_(2) ); RZ; }
   } // namespace LFBUF

//------------------------------------------------------------------------------


STATIC_CONST char KevinsMetatable_Win[] = "KevinsMetatable.Win";
struct lua_Win { PWin pWin; };

STATIC_FXN PWin l_Get_Win( lua_State *L, int argNum=1 ) {
   const auto rv( static_cast<lua_Win *>( luaL_checkudata( L, argNum, KevinsMetatable_Win ) ) );
   luaL_argcheck( L, rv, argNum, "'Win' expected" );
   return rv->pWin;
   }

#define  thisWin()      l_Get_Win( L )

STATIC_FXN int l_Win_function_getmetatable( lua_State *L ) {
   luaL_getmetatable( L, KevinsMetatable_Win );
   return 1;
   }
int l_construct_Win( lua_State *L, PWin pWin ) {
   if( !pWin ) { R_nil(); }
   const auto ud( static_cast<lua_Win *>( lua_newuserdata( L, sizeof( lua_Win ) ) ) );
   ud->pWin = pWin;
   l_Win_function_getmetatable( L );
   lua_setmetatable( L, -2 );
   return 1;
   }

int l_construct_Win( lua_State *L, int winIdx ) {
   return l_construct_Win( L, winIdx >= 0 && winIdx < g_WindowCount() ? g_WinWr( winIdx ) : 0 );
   }
//---------------------------
STATIC_FXN int l_Win_mmethod__eq ( lua_State *L ) { R_bool( l_Get_Win( L, 1 ) == l_Get_Win( L, 2 ) ); }
STATIC_FXN int l_cmp_Win_1_2 ( lua_State *L ) { return cmp_win( l_Get_Win( L, 1 ), l_Get_Win( L, 2 ) ); }
STATIC_FXN int l_Win_mmethod__lt ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) <  0 ); }
STATIC_FXN int l_Win_mmethod__le ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) <= 0 ); }
STATIC_FXN int l_Win_mmethod__gt ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) >  0 ); }
STATIC_FXN int l_Win_mmethod__ge ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) >= 0 ); }
//---------------------------
STATIC_FXN int l_Win_function_cur ( lua_State *L ) { return l_construct_Win( L, g_CurWinWr() ); }
STATIC_FXN int l_Win_function_getn( lua_State *L ) { return l_construct_Win( L, I_(1)-1      ); }
STATIC_FXN int l_Win_function_by_filename( lua_State *L ) { // Beware!  There MAY BE more than one window onto a given file
   auto pFnm = S_(1);
   for( auto ix(0) ; ix < g_WindowCount(); ++ix ) {
      if( g_Win( ix )->ViewHd.front()->FBuf()->NameMatch( pFnm ) ) {
         return l_construct_Win( L, ix );
         }
      }
   R_nil();
   }

namespace LExFx {
   STATIC_FXN int SplitCurWnd( lua_State *L ) {
      const auto fSplitVertical = ToBOOL(I_(1));
            auto splitAt        = I_(2);    // this is a PERCENTAGE
      const auto pWin( g_CurWin() );
      const auto size   = fSplitVertical ? pWin->d_Size.col : pWin->d_Size.lin;
      const int  minwin = fSplitVertical ? MIN_WIN_WIDTH    : MIN_WIN_HEIGHT  ;
      if( size > (2*minwin)+1 ) { // +1 for border
         splitAt = (size * splitAt) / 100;
         NoLessThan( &splitAt, minwin );
         auto rv = ::SplitCurWnd( fSplitVertical, splitAt );
         if( rv ) {
            ::DispRefreshWholeScreenNow();
            return l_construct_Win( L, rv );
            }
         }
      R_nil();
      }
   }

namespace LWin {
   STATIC_FXN int CurFBUF( lua_State *L )    { return l_construct_FBUF( L, thisWin()->ViewHd.front()->FBuf() ); }
   STATIC_FXN int Height( lua_State *L )     { R_int( thisWin()->d_Size.lin ); }
   STATIC_FXN int MakeCurrent( lua_State *L ){ SetWindowSetValidView_( thisWin() ); R_nil(); }
   }

//------------------------------------------------------------------------------

STATIC_CONST char KevinsMetatable_View[] = "KevinsMetatable.View";
struct lua_View { PView pView; };

STATIC_FXN PView l_Get_View( lua_State *L ) {
   const auto rv( static_cast<lua_View *>( luaL_checkudata( L, 1, KevinsMetatable_View ) ) );
   luaL_argcheck( L, rv, 1, "'View' expected" );
   return rv->pView;
   }

#define  thisVw()      l_Get_View( L )

STATIC_FXN int l_View_function_getmetatable( lua_State *L ) {
   luaL_getmetatable( L, KevinsMetatable_View );
   return 1;
   }

int l_construct_View( lua_State *L, PView pView ) {
   if( !pView ) { R_nil(); }
   const auto ud( static_cast<lua_View *>( lua_newuserdata( L, sizeof( lua_View ) ) ) );
   ud->pView = pView;
   l_View_function_getmetatable( L );
   lua_setmetatable( L, -2 );
   return 1;
   }

namespace LWin {
   STATIC_FXN int CurView( lua_State *L )    { return l_construct_View( L, thisWin()->ViewHd.front() ); }  // Note that View ctor is a Win _method_
   }

namespace LFBUF {
   STATIC_FXN int PutFocusOn( lua_State *L ) { return l_construct_View( L, thisPF()->PutFocusOn() ); }     // Note that View ctor is a FBUF _method_
   }

int l_View_function_cur( lua_State *L ) { return l_construct_View( L, g_CurView() ); }
namespace LView {
   STATIC_FXN int Next( lua_State *L )     { return l_construct_View( L, DLINK_NEXT( thisVw(), dlinkViewsOfWindow ) ); }
   STATIC_FXN int __tostring( lua_State *L )  { auto pView = thisVw(); lua_pushfstring( L, "View(%p=\"%s\")", pView, pView->FBuf()->Name() ); return 1; }
   STATIC_FXN int MoveCursor( lua_State *L )  { thisVw()->MoveCursor( I_(2)-1, I_(3)-1 ); RZ; }
   STATIC_FXN int GetCursorYX( lua_State *L ) {
      auto pView = thisVw();
      lua_pushinteger( L, pView->Cursor().lin+1 );
      lua_pushinteger( L, pView->Cursor().col+1 );
      lua_pushinteger( L, pView->Origin().lin+1 );
      lua_pushinteger( L, pView->Origin().col+1 );
      return 4;
      }
   STATIC_FXN int FBuf( lua_State *L )   { return l_construct_FBUF( L, thisVw()->FBuf() ); }
   STATIC_FXN int FName( lua_State *L )  { R_str( thisVw()->FBuf()->Name() ); }
   STATIC_FXN int Get_LineCompile( lua_State *L )  { R_int( thisVw()->Get_LineCompile()+1 );          }
   STATIC_FXN int Set_LineCompile( lua_State *L )  { thisVw()->Set_LineCompile( I_(2)-1 ); RZ; }
   STATIC_FXN int HiliteMatch( lua_State *L ) { // params: int line, int col, int MatchCols
      const auto pv    ( thisVw() );
      const auto yLine ( I_(2)-1 );
      const auto xStart( I_(3)-1 );
      const auto Cols  ( I_(4)   );
      0 && DBG( "%s (%d,%d) L %d", __func__, yLine, xStart, Cols );
      const Point pt( yLine, xStart ); // NB: Lua uses 1-based line and column numbers!!!
      pv->SetMatchHiLite( pt, Cols, false );
      RZ;
      }
   }

#pragma GCC diagnostic pop


// load the list of libs
void l_OpenStdLibs( lua_State *L ) {
   STATIC_CONST luaL_reg lualibs[] = {
      { ""                ,  luaopen_base    },
      { LUA_LOADLIBNAME   ,  luaopen_package },
      { LUA_TABLIBNAME    ,  luaopen_table   },
      { LUA_STRLIBNAME    ,  luaopen_string  },
      { LUA_IOLIBNAME     ,  luaopen_io      },
      { LUA_OSLIBNAME     ,  luaopen_os      },
      { LUA_MATHLIBNAME   ,  luaopen_math    },  // 20060421 klg enabled (at a cost of 30Kbytes!!!) JUST TO GET math.floor for int() !!!
      { LUA_DBLIBNAME     ,  luaopen_debug   },
      { LUA_KLIBNAME      ,  luaopen_klib    },
#if USE_PCRE
      { "pcre"            ,  luaopen_rex_pcre }, // 20080105 kgoodwin apparent 32K cost
#endif
      { "lpeg"            ,  luaopen_lpeg    },  // 20081110 kgoodwin apparent 36K cost
      // add your libraries here
   };

   for( const auto &lib : lualibs ) {
      DBG_LUA && DBG( "%s *** loading %s ***", __func__, lib.name );
      lua_pushcfunction(L, lib.func);
      lua_pushstring(L, lib.name);
      lua_call(L, 1, 0);
      DBG_LUA && DBG( "%s *** loaded  %s ***", __func__, lib.name );
      }
   }

void l_RegisterEditorFuncs( lua_State *L ) {
   extern int FindMatchingTagsLines( lua_State *L );
   STATIC_CONST luaL_reg myLuaFuncs[] = {
      #define  LUA_FUNC_GLOBAL( func )   { #func, func },
      #define  LUA_FUNC_I( func )   { #func, LExFx::func },
#ifdef use_AddEdFxn
       LUA_FUNC_I(AddEdFxn)
#endif
       LUA_FUNC_I(AddToSearchLog)
       { "AssignStrOk"                 , LExFx::l_AssignStrOk },
       LUA_FUNC_I(PushVariableMacro)
       LUA_FUNC_I(Clipboard_PutText)
       LUA_FUNC_I(CmdIdxAddLuaFunc)
       { "SetKeyOk"                    , LExFx::BindKeyToCMD  },
       LUA_FUNC_I(DBG)
       LUA_FUNC_I(Path_CommonPrefixLen)
       LUA_FUNC_I(MarkDefineAtCurPos)
       LUA_FUNC_I(MarkGoto)
       LUA_FUNC_I(GetChildDirs)
       LUA_FUNC_I(GetDynMacros)
       LUA_FUNC_I(GetCwd)
       LUA_FUNC_I(Getenv)
       LUA_FUNC_I(GetenvOrNil)
       LUA_FUNC_I(Get_SearchCaseSensitive)
       LUA_FUNC_I(Putenv)
       LUA_FUNC_I(ModifyTimeOfDiskFile)
       LUA_FUNC_I(Msg)
       LUA_FUNC_I(OsErrStr)
       LUA_FUNC_I(OsName)
       LUA_FUNC_I(OsVer)
       LUA_FUNC_I(Path_Dirnm)
       LUA_FUNC_I(Path_Fnm)
       LUA_FUNC_I(Path_Ext)
       LUA_FUNC_I(Path_FnameExt)
       LUA_FUNC_I(RsrcFilename)
       LUA_FUNC_I(StateFilename)
       LUA_FUNC_I(StartConProcess)
       LUA_FUNC_I(StartGuiProcess)
       LUA_FUNC_I(StartShellExecuteProcess)
       LUA_FUNC_I(Bell)
       LUA_FUNC_I(GetKey)
       LUA_FUNC_I(SplitCurWnd)
       LUA_FUNC_I(ScreenLines)
       LUA_FUNC_I(ScreenCols)
       LUA_FUNC_I(HideCursor)
       LUA_FUNC_I(UnhideCursor)
       LUA_FUNC_I(DispRefreshWholeScreenNow)
       LUA_FUNC_I(DirectVidClear)
       LUA_FUNC_I(DirectVidWrStrColorFlush)
       LUA_FUNC_I(fExecute)
       LUA_FUNC_I(fChangeFile)
       LUA_FUNC_I(IsFile)
       LUA_FUNC_I(IsDir)
       LUA_FUNC_I(SleepMs)
       LUA_FUNC_I(rmargin)
       LUA_FUNC_I(valueof)                      // my Lua language extension
       LUA_FUNC_I(split_ch)                     //    Lua language extension (from PIL)
       LUA_FUNC_I(split_str)                    //    Lua language extension (from PIL)
       LUA_FUNC_I(split_ch_tbl)
       LUA_FUNC_I(split_str_tbl)
       LUA_FUNC_I(enqueue_compile_jobs)
       LUA_FUNC_I(hexstr2bitstr)
       LUA_FUNC_I(bitstr2hexstr)
       LUA_FUNC_GLOBAL(FindMatchingTagsLines)
      #undef   LUA_FUNC_I
      };

   for( const auto &lib : myLuaFuncs ) {
      DBG_LUA && ::DBG( "%s --- registering %s ---", __func__, lib.name );
      lua_register( L, lib.name, lib.func );
      DBG_LUA && ::DBG( "%s --- registered  %s ---", __func__, lib.name );
      }
   }

void l_SetEditorGlobalsInts( lua_State *L ) {
   struct nm2int { PCChar name; int value; };
   STATIC_CONST nm2int intVals[] = {
      { "NOARG"       , NOARG      },
      { "TEXTARG"     , TEXTARG    },
      { "NULLARG"     , NULLARG    },
      {   "NULLEOL"   ,   NULLEOL  },
      {   "NULLEOW"   ,   NULLEOW  },
      {   "NOARGWUC"  ,   NOARGWUC },
      { "LINEARG"     , LINEARG    },
      { "BOXARG"      , BOXARG     },
      { "STREAMARG"   , STREAMARG  },
      { "NUMARG"      , NUMARG     },
      { "MARKARG"     , MARKARG    },
      { "BOXSTR"      , BOXSTR     },
      { "MODIFIES"    , MODIFIES   },
      { "KEEPMETA"    , KEEPMETA   },
      { "WINDOWFUNC"  , WINDOWFUNC },
      { "CURSORFUNC"  , CURSORFUNC },
      { "MACROFUNC"   , MACROFUNC  },
      { "MAXCOL"      , COL_MAX    },
   };
   for( const auto &pE : intVals ) { 0 && DBG_LUA && DBG( "%s *** loading %s ***", __func__, pE.name );
      setglobal( L, pE.name, pE.value );
      }
   }

void l_register_class( lua_State *L
   , PCChar     szObjName
   , PCChar     kszMetatable_magic_string
   , const luaL_Reg *metamethods
   , const luaL_Reg *functions
   , const luaL_Reg *methods
   ) {
   // void luaL_register (lua_State *L, const char *libname, const luaL_Reg *l);
   //
   // When called with libname equal to NULL, simply registers all functions in
   // the list l (see luaL_Reg) into the table on the top of the stack.
   //
   // When called with a non-NULL libname, creates a new table t, sets it as the
   // value of the global variable libname, sets it as the value of
   // package.loaded[libname], and registers on it all functions in the list l.
   // If there is a table in package.loaded[libname] or in variable libname,
   // reuses this table instead of creating a new one.
   //
   // In any case the function leaves the table on the top of the stack.

   luaL_newmetatable( L, kszMetatable_magic_string );
   lua_pushvalue(     L, -1 );  // DUP metatable
   lua_setfield(      L, -2, "__index" );  // metatable.__index = metatable; leaves TOS=metatable
   if( metamethods ) {
      luaL_register( L, 0, metamethods );    // leaves TOS=metatable
      }
   luaL_register( L, 0, methods );           // leaves TOS=metatable
   luaL_register( L, szObjName, functions ); // leaves TOS=metatable
   0 && LDS( __func__, L );
   0 && DBG( "%s: done +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-", __func__ );
   }


STATIC_FXN void l_register_Win_object( lua_State *L ) {
   STATIC_CONST luaL_reg metamethods[] = {
      { "__eq", l_Win_mmethod__eq },
      { "__lt", l_Win_mmethod__lt },
//    { "__le", l_Win_mmethod__le },
//    { "__gt", l_Win_mmethod__gt },
//    { "__ge", l_Win_mmethod__ge },
      { 0 , 0 }
      };
   STATIC_CONST luaL_reg methods[] = {
      #define  LUA_FUNC_I( func )   { #func, LWin::func },
      LUA_FUNC_I(CurFBUF)
      LUA_FUNC_I(CurView)
      LUA_FUNC_I(Height)
      LUA_FUNC_I(MakeCurrent)
      #undef   LUA_FUNC_I
      { 0 , 0 }
      };
   STATIC_CONST luaL_reg functions[] = {
      { "getmetatable" , l_Win_function_getmetatable },
      { "cur"          , l_Win_function_cur          },
      { "getn"         , l_Win_function_getn         },
      { "by_filename"  , l_Win_function_by_filename  },
      { 0 , 0 }
      };
   l_register_class( L, "Win", KevinsMetatable_Win, metamethods, functions, methods );
   }

STATIC_FXN void l_register_View_object( lua_State *L ) {
   STATIC_CONST luaL_reg methods[] = {
      #define  LUA_FUNC_I( func )   { #func, LView::func },
      LUA_FUNC_I(__tostring)
      LUA_FUNC_I(MoveCursor)
      LUA_FUNC_I(GetCursorYX)
      LUA_FUNC_I(Get_LineCompile)
      LUA_FUNC_I(Set_LineCompile)
      LUA_FUNC_I(HiliteMatch)
      LUA_FUNC_I(FBuf)
      LUA_FUNC_I(FName)
      LUA_FUNC_I(Next)
      #undef   LUA_FUNC_I
      { 0 , 0 }
      };
   STATIC_CONST luaL_reg functions[] = {
      { "getmetatable", l_View_function_getmetatable },
      { "cur"         , l_View_function_cur          },
      { 0 , 0 }
      };
   l_register_class( L, "View", KevinsMetatable_View, 0, functions, methods );
   }

STATIC_FXN void l_register_FBUF_object( lua_State *L ) {
   STATIC_CONST luaL_reg methods[] = {
      #define  LUA_FUNC_I( func )   { #func, LFBUF::func },
      LUA_FUNC_I(__tostring)
      LUA_FUNC_I(CantModify)
      LUA_FUNC_I(CopyLines)
      LUA_FUNC_I(ClearUndo)
      LUA_FUNC_I(ClrNoEdit)
      LUA_FUNC_I(DelLine)
      LUA_FUNC_I(DiscardTrailSpcs)
      LUA_FUNC_I(WriteToDisk)
      LUA_FUNC_I(ExpandWildcardSorted  )
      LUA_FUNC_I(ExpandWildcardUnsorted)
      LUA_FUNC_I(FTypeEq)
      LUA_FUNC_I(GetLine)
      LUA_FUNC_I(GetLineRaw)
      LUA_FUNC_I(GetLineSeg)
      LUA_FUNC_I(PutLineSeg)
      LUA_FUNC_I(InsBlankLinesBefore)
      LUA_FUNC_I(InsLine)
      LUA_FUNC_I(InsLineSortedAscending)
      LUA_FUNC_I(InsLineSortedDescending)
      LUA_FUNC_I(IsAutoRead)
      LUA_FUNC_I(IsDirty)
      LUA_FUNC_I(IsDiskRO)
      LUA_FUNC_I(IsGrepBuf)
      LUA_FUNC_I(IsNoEdit)
      LUA_FUNC_I(IsPseudo)
      LUA_FUNC_I(IsSysPseudo)
      LUA_FUNC_I(IsRsrcLdBlocked)
      LUA_FUNC_I(KeepTrailSpcs)
      LUA_FUNC_I(LastLine)
      LUA_FUNC_I(LineCount)
      LUA_FUNC_I(MakeDiskFileWritable)
      LUA_FUNC_I(MakeEmpty)
      LUA_FUNC_I(ModifyTimeOfDiskFile)
      LUA_FUNC_I(MoveCursorToBofAllViews)
      LUA_FUNC_I(Name)
      LUA_FUNC_I(Next)
      LUA_FUNC_I(PutFocusOn)
      LUA_FUNC_I(PutLastLine)
      LUA_FUNC_I(PutLine)
      LUA_FUNC_I(RefreshFailed)
      LUA_FUNC_I(RefreshFailedShowError)
      LUA_FUNC_I(SetAutoRead)
      LUA_FUNC_I(SetBackupMode_Bak)
      LUA_FUNC_I(SetBackupMode_None)
      LUA_FUNC_I(SetBackupMode_Undel)
      LUA_FUNC_I(SetBlockRsrcLd)
      LUA_FUNC_I(SetForgetOnExit)
      LUA_FUNC_I(SetNoEdit)
      LUA_FUNC_I(SetRememberAfterExit)
#ifdef fn_su
      LUA_FUNC_I(SetSilentUpdateMode)
      LUA_FUNC_I(SilentUpdateMode)
#endif
      LUA_FUNC_I(SetEntabOk)
      LUA_FUNC_I(TabWidth)
      LUA_FUNC_I(ToForgetOnExit)
      LUA_FUNC_I(ToglNoEdit)
      LUA_FUNC_I(TrailSpcsKept)
      LUA_FUNC_I(UnDirty)
      LUA_FUNC_I(cat)
      #undef   LUA_FUNC_I
      { 0 , 0 }
      };
   STATIC_CONST luaL_reg functions[] = {
      { "getmetatable"    , l_FBUF_function_getmetatable   },
      { "new"             , l_FBUF_function_new            },
      { "new_may_create"  , l_FBUF_function_new_may_create },
      { "CurView"         , l_FBUF_function_curview        },
      { "first"           , l_FBUF_function_first          },
      { "log"             , l_FBUF_function_getlog         },
      { 0 , 0 }
      };
   l_register_class( L, "FBUF", KevinsMetatable_FBUF, 0, functions, methods );
   }

void l_register_Editor_objects( lua_State *L ) {
   typedef void (* objReg) (lua_State *L);
   STATIC_CONST objReg myLuaObjects[] = {
        l_register_FBUF_object
      , l_register_View_object
      , l_register_Win_object
      };
   for( const auto &mlo : myLuaObjects ) {
      mlo( L );
      }
   }

void l_register_EdLib( lua_State *L ) {
   l_SetEditorGlobalsInts   ( L );
   l_RegisterEditorFuncs    ( L );
   l_register_Editor_objects( L );
   }

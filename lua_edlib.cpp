//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "lua_intf_common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

//####  lh_ = my Lua Helper functions

STATIC_FXN int lh_rtnStrOrNil( lua_State *L, PCChar src ) {
   if( src )  lua_pushstring( L, src );
   else       lua_pushnil( L );
   return 1;
   }

STATIC_FXN int lh_rtnStr( lua_State *L, PCChar src ) {
   lua_pushstring( L, src ? src : "" );
   return 1;
   }

// exported functions

#define  FBUF_(   API_name)  LUAFUNC_(l_FBUF_method_ ## API_name)

LUAFUNC_(HideCursor)   { HideCursor()  ; RZ; }
LUAFUNC_(UnhideCursor) { UnhideCursor(); RZ; }
LUAFUNC_(DispRefreshWholeScreenNow) { DispRefreshWholeScreenNow(); RZ; }
LUAFUNC_(GetKey)     {
   while(1) {
      char keyNm[30];
      const PCCMD pCmd( CmdFromKbdForInfo( BSOB(keyNm) ) );
      if( pCmd && keyNm[0] ) {
         R_str(keyNm);
         }
      }
   }

LUAFUNC_(StartShellExecuteProcess) { StartShellExecuteProcess( S_(1), So0_(2) ); RZ; }
LUAFUNC_(StartConProcess)    {    R_int( StartConProcess( S_(1) ) ); }
LUAFUNC_(StartGuiProcess)    {    R_int( StartGuiProcess( S_(1) ) ); }
LUAFUNC_(OsErrStr)           { linebuf lb; OsErrStr( BSOB(lb) ); R_str( lb ); }
LUAFUNC_(OsVer)              { R_str( OsVerStr() ); }

LUAFUNC_(RsrcFilename)  { pathbuf pb; R_str( RsrcFilename ( BSOB(pb), S_(1) ) ); }
LUAFUNC_(StateFilename) { pathbuf pb; R_str( StateFilename( BSOB(pb), S_(1) ) ); }

LUAFUNC_(Bell) { Bell(); RZ; }
LUAFUNC_(ScreenLines) { R_int( ScreenLines() ); }
LUAFUNC_(ScreenCols ) { R_int( ScreenCols () ); }
LUAFUNC_(VidWrStrColorFlush) { 0 && DBG("%s: %d, %d", __func__, I_(1), I_(2) );
   VidWrStrColorFlush( I_(1), I_(2), S_(3), I_(4), I_(5), LuaBool(6) );
   RZ;
   }
LUAFUNC_(l_CommonStrlenI)    { R_int( CommonStrlenI( S_(1), S_(2) ) ); }
LUAFUNC_(AddToSearchLog)     { AddToSearchLog( S_(1) ); RZ; }
LUAFUNC_(l_AssignStrOk)      { R_bool( AssignStrOk( S_(1) ) ); }
LUAFUNC_(PushVariableMacro)  { R_bool( PushVariableMacro( S_(1) ) ); }
LUAFUNC_(CmdIdxAddLuaFunc)   { CmdIdxAddLuaFunc( S_(1), fn_runLua(), I_(2)  _AHELP( S_(3) ) ); RZ; }
LUAFUNC_(SetKeyOk)           { R_bool( SetKeyOk( S_(1), S_(2) ) ); }
LUAFUNC_(fExecute)           { R_bool( fExecute( S_(1) ) ); }
LUAFUNC_(fChangeFile)        { R_bool( fChangeFile( S_(1), true ) ); }
LUAFUNC_(DBG)                { DBG( "%s", S_(1) ); RZ; }
LUAFUNC_(Msg)         {
   const auto p1( S_(1) );
   s_pFbufLuaLog->FmtLastLine( "*** %s", p1 );
   Msg( "%s", p1 );
   RZ;
   }
LUAFUNC_(GetCwd)      {
   R_str( ( Path::GetCwd() + PATH_SEP_STR ).c_str() );
   }
LUAFUNC_(GetChildDirs) {
   lua_newtable(L);  // result
   DirListGenerator dlg;

   #if USE_STATE_ELB
      auto pXb( get_xb( L ) );
   #else
      Path::str_t xb;
   #endif
   for( int tblIdx=1 ; dlg.VGetNextName( xb ) ; ++tblIdx ) {
      lua_pushstring( L, xb.c_str() );
      lua_rawseti( L, -2, tblIdx );
      }
   return 1;  // return the table
   }
LUAFUNC_(Get_SearchCaseSensitive)      { R_bool( g_fCase ); }
LUAFUNC_(Getenv)      { return lh_rtnStr(      L, getenv( S_(1) ) ); }
LUAFUNC_(GetenvOrNil) { return lh_rtnStrOrNil( L, getenv( S_(1) ) ); }
LUAFUNC_(Putenv)      {
   const auto    param1( S_(1) );
   const auto    param2( So0_(2) );
   if( param2 )  PutEnvOk( param1, param2 );
   else          PutEnvOk( param1 );
   RZ;
   }
LUAFUNC_(MarkDefineAtCurPos)       { MarkDefineAtCurPos( S_(1) ); RZ; }
LUAFUNC_(MarkGoto)                 { R_bool( MarkGoto( S_(1) ) ); }
LUAFUNC_(ModifyTimeOfDiskFile)     {
   const auto fstat_( GetFileStat( S_(1) ) );
   R_lstr( PCChar(&fstat_), sizeof(fstat_) );
   }

LUAFUNC_(valueof)     {  0 && DBG( "%s: '%s'", __func__, S_(1) );
   lua_getglobal( L, S_(1) ); // returns Nil if nosuch variable (how to discriminate vs. undefined variable?  I think we can't)
   return 1;
   }

#define  ROUNDUP_TO_NEXT_POWER2( val, pow2 )  ((val + ((pow2)-1)) & ~((pow2)-1))


LUAFUNC_(IsFile)  { R_bool( IsFile( S_(1) ) ); }
LUAFUNC_(IsDir)   { R_bool( IsDir(  S_(1) ) ); }


LUAFUNC_(SleepMs)   { SleepMs( U_(1) ); RZ; }


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


// BUGBUG - if SPLIT_RTNS_TABLE == 0, lua_split_ could return a potentially
//          unlimited number of values (on the stack), and Lua has a fixed stack
//          size which we can overflow!!!
//
#define  SPLIT_RTNS_TABLE  0

STATIC_FXN int lua_split_rtn_mult( lua_State *L, TLuaSplitFxn sf ) {
   auto strToSplit = S_(1);
   auto sep = S_(2);
   int    i   = 1;

   // repeat for each separator
   PCChar pastEnd;
   int sepLen(0);
   while( *(pastEnd=sf( strToSplit, sep, &sepLen )) != '\0' ) {
      lua_pushlstring( L, strToSplit, pastEnd - strToSplit );  // push substring
      ++i;
      strToSplit = pastEnd + sepLen;  // skip separator
      }

   // push last substring
   lua_pushstring( L, strToSplit );

   0 && DBG( "%s rtns %d", __func__, i );
   return i;  // return number of strings pushed
   }

STATIC_FXN int lua_split_rtn_tbl( lua_State *L, TLuaSplitFxn sf ) {
   auto strToSplit = S_(1);
   auto sep = S_(2);
   auto i   = 1;

   lua_newtable(L);  // result

   // repeat for each separator
   PCChar pastEnd;
   int sepLen(0);
   while( *(pastEnd=sf( strToSplit, sep, &sepLen )) != '\0' ) {
      lua_pushlstring( L, strToSplit, pastEnd - strToSplit );  // push substring
      lua_rawseti( L, -2, i );
      ++i;
      strToSplit = pastEnd + sepLen;  // skip separator
      }

   // push last substring
   lua_pushstring( L, strToSplit );
   lua_rawseti( L, -2, i );

   return 1;  // return the table
   }

LUAFUNC_(split_ch)      { return lua_split_rtn_mult( L, StrToNextOrEos_ ); }
LUAFUNC_(split_ch_tbl)  { return lua_split_rtn_tbl(  L, StrToNextOrEos_ ); }

LUAFUNC_(split_str)     { return lua_split_rtn_mult( L, FindStr_ );        }
LUAFUNC_(split_str_tbl) { return lua_split_rtn_tbl(  L, FindStr_ );        }

STATIC_FXN PChar nib2bitstr_( PChar p5, int nib ) {
   p5[4] = '\0';
   for( int ix=3; ix>=0; --ix ) {
      p5[ix] = (nib & 1) ? '1' : '0';
      nib >>= 1;
      }
   return p5;
   }

LUAFUNC_(hexstr2bitstr) {
   auto inst = S_(1);
   if( inst[0] == '0' && tolower(inst[1]) == 'x' ) inst += 2; // skip redundant "hex prefix"
   const int xdigits( consec_xdigits( inst ) );
   if( 0 == xdigits ) luaL_error(L, "string '%s' contains no leading hexits", inst);
   luaL_Buffer lb; luaL_buffinit(L, &lb);
   for( int ix=0; ix < xdigits; ++ix ) {
      const U8 ch( tolower( inst[ix] ) );
      const int val( ch - (ch < 'a' ? '0' : 'a' - 10) );  0 && DBG( ":: %X", val );
      char buf5[5];
      luaL_addstring( &lb, nib2bitstr_( buf5, val ) );    0 && DBG( ":: %s L %d", buf5, Strlen(buf5) );
      }
   luaL_pushresult( &lb );
   return 1;
   }

STATIC_CONST char b2x[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

LUAFUNC_(bitstr2hexstr) {
   auto inst = S_(1);
   luaL_Buffer b; luaL_buffinit(L, &b);
   if( inst[0] == '0' && tolower(inst[1]) == 'b' ) inst += 2; // skip redundant "bin prefix"
   const auto bdigits( consec_bdigits( inst ) );
   const auto outchars( (bdigits / 4) + ((bdigits % 4) != 0) );

   CPChar outbuf = PChar( alloca( outchars+1 ) );
   outbuf[outchars] = '\0';
   auto pb = outbuf + outchars;
   auto pc = inst + bdigits;
   auto bits = 0;
   auto bbuf = 0;
   for( auto ix(0); ix<bdigits; ++ix ) {
      auto val( *(--pc) - '0' ); 0 && DBG( "val = %X", val );
      bbuf |= (val <<= bits);
      if( ++bits == 4 ) {
         *(--pb) = b2x[bbuf];
         bits = bbuf = 0;
         }
      }
   if( bits > 0 )
      *(--pb) = b2x[bbuf];
   Assert( pb >= outbuf );
   R_str( outbuf );
   }

LUAFUNC_(enqueue_compile_jobs) {
   luaL_checktype(L, 1, LUA_TTABLE); // 1st argument must be a table (t)
   StringList sl;
   const auto jobs( luaL_getn(L, 1) );
   for( auto ix(1) ; ix <= jobs ; ++ix ) {
      lua_rawgeti(L, 1, ix);  // push t[ix]
      PCChar pCmd = luaL_checkstring(L, -1);
      0 && DBG( "%s", pCmd );
      sl.AddStr( pCmd );
      }

   CompilePty_CmdsAsyncExec( sl, true );
   RZ;
   }


STATIC_CONST char KevinsMetatable_FBUF[] = "KevinsMetatable.FBUF";
struct lua_FBUF { PFBUF pFBuf; };

int l_construct_View( lua_State *L, PView pView );
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
           PRbNode pNd( rb_first( g_FBufIdx ) );
           return (pNd != rb_nil(g_FBufIdx)) ? l_construct_FBUF( L, static_cast<PFBUF>(rb_val( pNd )) ) : 0;
           }
#else
STATIC_FXN int l_FBUF_function_first( lua_State *L )     { return l_construct_FBUF( L, g_FBufHead.First() ); }
#endif

STATIC_FXN int l_FBUF_function_getlog( lua_State *L )    { return l_construct_FBUF( L, s_pFbufLuaLog      ); }

STATIC_FXN PFBUF l_Get_FBUF( lua_State *L, int stacklvl=1 ) {
   const auto rv( static_cast<lua_FBUF *>( luaL_checkudata( L, stacklvl, KevinsMetatable_FBUF ) ) );
   luaL_argcheck( L, rv, stacklvl, "'PFBUF' expected" );
   return rv->pFBuf;
   }

#define  thisPF()   l_Get_FBUF( L )
#define  PFBUF_(n)  l_Get_FBUF( L, n )

FBUF_(__tostring)               { R_strf( "FBUF(\"%s\")", thisPF()->Name() ); }
FBUF_(Next)                     { return l_construct_FBUF( L, thisPF()->Next()     ); }

FBUF_(ClearUndo)                { thisPF()->ClearUndo()                                              ; RZ; }
FBUF_(ClrNoEdit)                { thisPF()->ClrNoEdit()                                              ; RZ; }
FBUF_(CopyLines)                { FBOP::CopyLines( thisPF(), I_(2)-1, PFBUF_(3), I_(4)-1, I_(5)-1 )  ; RZ; }
FBUF_(DiscardTrailSpcs)         { thisPF()->DiscardTrailSpcs()                                       ; RZ; }
FBUF_(InsBlankLinesBefore)      { thisPF()->InsBlankLinesBefore( I_(2)-1, Io_( 3, 1 ) )              ; RZ; }
FBUF_(InsLine)                  { thisPF()->InsLine(      I_(2)-1, S_(3) )                           ; RZ; }
FBUF_(InsLineSortedAscending)   {
                                  #if USE_STATE_ELB
                                       auto pXb( get_xb( L ) );
                                  #else
                                       Xbuf xb; auto pXb(&xb);
                                  #endif
                                  const PCChar st = S_(2);
                                  FBOP::InsLineSortedAscending(  thisPF(), pXb, Io_(3,1)-1, st ); RZ;
                                  }
FBUF_(InsLineSortedDescending)  {
                                  #if USE_STATE_ELB
                                       auto pXb( get_xb( L ) );
                                  #else
                                       Xbuf xb; auto pXb(&xb);
                                  #endif
                                  const PCChar st = S_(2);
                                  FBOP::InsLineSortedDescending( thisPF(), pXb, Io_(3,1)-1, st ); RZ;
                                  }
FBUF_(IsGrepBuf)                { int metaLines;  pathbuf searchedFnm;
                                  if( FBOP::IsGrepBuf( thisPF(), BSOB(searchedFnm), &metaLines ) ) {
                                     P_str(searchedFnm) ; P_int(metaLines) ; return 2;
                                     }
                                  RZ;
                                }
FBUF_(KeepTrailSpcs)            { thisPF()->KeepTrailSpcs()                                    ; RZ; }
FBUF_(MakeEmpty)                { thisPF()->MakeEmpty()                                        ; RZ; }
FBUF_(PutLastLine)              { thisPF()->PutLastLine( S_(2) )                               ; RZ; }
FBUF_(PutLine)                  { thisPF()->PutLine(      I_(2)-1, S_(3) )                     ; RZ; }
FBUF_(MoveCursorToBofAllViews)  { thisPF()->MoveCursorToBofAllViews()                          ; RZ; }
FBUF_(SetAutoRead)              { thisPF()->SetAutoRead()                                      ; RZ; }
FBUF_(SetBackupMode_Bak)        { thisPF()->SetBackupMode(bkup_BAK)                            ; RZ; }
FBUF_(SetBackupMode_None)       { thisPF()->SetBackupMode(bkup_NONE)                           ; RZ; }
FBUF_(SetBackupMode_Undel)      { thisPF()->SetBackupMode(bkup_UNDEL)                          ; RZ; }
FBUF_(SetBlockRsrcLd)           { thisPF()->SetBlockRsrcLd()                                   ; RZ; }
FBUF_(SetForgetOnExit)          { thisPF()->SetForgetOnExit()                                  ; RZ; }
FBUF_(SetNoEdit)                { thisPF()->SetNoEdit()                                        ; RZ; }
FBUF_(SetRememberAfterExit)     { thisPF()->SetRememberAfterExit()                             ; RZ; }
#ifdef fn_su
FBUF_(SetSilentUpdateMode)      { thisPF()->SetSilentUpdateMode()                              ; RZ; }
#endif
FBUF_(ToglNoEdit)               { thisPF()->ToglNoEdit()                                       ; RZ; }
FBUF_(UnDirty)                  { thisPF()->UnDirty()                                          ; RZ; }
FBUF_(WriteToDisk)              { thisPF()->WriteToDisk( So0_(2) )                             ; RZ; }
FBUF_(cat)                      { thisPF()->cat( S_(2) )                                       ; RZ; }
FBUF_(SetTabconvOk)             { R_bool(      thisPF()->SetTabconvOk( I_(2) )       ); }
FBUF_(RefreshFailedShowError)   { R_bool( 0 != thisPF()->RefreshFailedShowError()    ); }
FBUF_(IsAutoRead)               { R_bool( 0 != thisPF()->IsAutoRead()                ); }
FBUF_(RefreshFailed)            { R_bool( 0 != thisPF()->RefreshFailed()             ); }
FBUF_(IsDirty)                  { R_bool( 0 != thisPF()->IsDirty()                   ); }
#ifdef fn_su
FBUF_(SilentUpdateMode)         { R_bool( 0 != thisPF()->SilentUpdateMode()          ); }
#endif
FBUF_(IsDiskRO)                 { R_bool( 0 != thisPF()->IsDiskRO()                  ); }
FBUF_(HasGlobalPtr)             { R_bool( 0 != thisPF()->HasGlobalPtr()              ); }
FBUF_(ToForgetOnExit)           { R_bool( 0 != thisPF()->ToForgetOnExit()            ); }
FBUF_(IsPseudo)                 { R_bool( 0 != thisPF()->FnmIsPseudo()               ); }
FBUF_(IsRsrcLdBlocked)          { R_bool( 0 != thisPF()->IsRsrcLdBlocked()           ); }
FBUF_(IsNoEdit)                 { R_bool( 0 != thisPF()->IsNoEdit()                  ); }
FBUF_(MakeDiskFileWritable)     { R_bool( 0 != thisPF()->MakeDiskFileWritable()      ); }
FBUF_(TrailSpcsKept)            { R_bool( 0 != thisPF()->TrailSpcsKept()             ); }
FBUF_(CantModify)               { R_bool( 0 != thisPF()->CantModify()                ); }
FBUF_(LineCount)                { R_int(       thisPF()->LineCount()                 ); }
FBUF_(LastLine)                 { R_int(       thisPF()->LastLine()+1                ); }
FBUF_(TabWidth)                 { R_int(       thisPF()->TabWidth()                  ); }
FBUF_(ModifyTimeOfDiskFile)     {
                                  const auto fstat_( thisPF()->GetLastFileStat() );
                                  R_lstr( PCChar(&fstat_), sizeof(fstat_) );
                                }
FBUF_(Name)                     { R_str(       thisPF()->Name()                      ); }
FBUF_(DelLine)                  { const LINE L1( I_(2)-1 ); thisPF()->DelLines( L1, Io_( 3, L1+1)-1 ); RZ; }

STIL bool isValidLineNum( PFBUF fb, LINE lnum )  { return lnum >= 0 && lnum <= fb->LastLine(); }

FBUF_(GetLine) {
   const auto pf( thisPF() );
   const auto lnum( I_(2) - 1 );
   if( isValidLineNum( pf, lnum ) ) {
 #if USE_STATE_ELB
      auto pXb( get_xb( L ) );
 #else
      Xbuf xb; auto pXb(&xb);
 #endif
      pf->getLineTabx( pXb, lnum );
      R_lstr( pXb->c_str(), pXb->length() );
      }
   R_nil();
   }

FBUF_(GetLineRaw) {
   const auto pf( thisPF() );
   const int lnum( I_(2) - 1 );
   if( isValidLineNum( pf, lnum ) ) {
      PCChar ptr; size_t chars;
      pf->PeekRawLineExists( lnum, &ptr, &chars );
      R_lstr( ptr, chars );
      }
   R_nil();
   }

FBUF_(GetLineSeg) {
   const auto pf( thisPF() );
   const auto lnum( I_(2) - 1 );
   if( isValidLineNum( pf, lnum ) ) {
      const auto xLeftIncl ( I_(3) - 1 );
      const auto xRightIncl( I_(4) - 1 );
 #if USE_STATE_ELB
      auto pXb( get_xb( L ) );
 #else
      Xbuf xb; auto pXb(&xb);
 #endif
      const auto chars( pf->GetLineSeg( *pXb, lnum, xLeftIncl, xRightIncl ) );
      R_lstr( pXb->c_str(), pXb->length() );
      }
   R_nil();
   }

FBUF_(PutLineSeg) { thisPF()->PutLineSeg( I_(2)-1, S_(3), I_(4)-1, I_(5)-1, LuaBool(6) ); RZ; }

FBUF_(ExpandWildcardSorted  ) { FBOP::ExpandWildcardSorted  ( thisPF(), S_(2) ); RZ; }
FBUF_(ExpandWildcardUnsorted) { FBOP::ExpandWildcardUnsorted( thisPF(), S_(2) ); RZ; }

//------------------------------------------------------------------------------

#define  Win_( API_name )  STATIC_FXN int l_Win_method_ ## API_name ( lua_State *L )

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
   return l_construct_Win( L, winIdx >= 0 && winIdx < g_iWindowCount() ? g__.aWindow[ winIdx ] : 0 );
   }


// FBUF_(PutFocusOn)  // Note that Win ctor is a FBUF _method_
//    {
//    return l_construct_Win( L, thisPF()->PutFocusOn() );
//    }

//---------------------------
STATIC_FXN int l_Win_mmethod__eq ( lua_State *L ) { R_bool( l_Get_Win( L, 1 ) == l_Get_Win( L, 2 ) ); }

STATIC_FXN int l_cmp_Win_1_2 ( lua_State *L ) { return cmp_win( l_Get_Win( L, 1 ), l_Get_Win( L, 2 ) ); }

STATIC_FXN int l_Win_mmethod__lt ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) <  0 ); }
STATIC_FXN int l_Win_mmethod__le ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) <= 0 ); }
STATIC_FXN int l_Win_mmethod__gt ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) >  0 ); }
STATIC_FXN int l_Win_mmethod__ge ( lua_State *L ) { R_bool( l_cmp_Win_1_2( L ) >= 0 ); }
//---------------------------


STATIC_FXN int l_Win_function_cur ( lua_State *L ) { return l_construct_Win( L, g_CurWin() ); }
STATIC_FXN int l_Win_function_getn( lua_State *L ) { return l_construct_Win( L, I_(1)-1    ); }

STATIC_FXN int l_Win_function_by_filename( lua_State *L ) { // Beware!  There MAY BE more than one window onto a given file
   auto pFnm = S_(1);
   for( auto ix(0) ; ix < g_iWindowCount(); ++ix )
      if( g_Win( ix )->ViewHd.First()->FBuf()->NameMatch( pFnm ) )
         return l_construct_Win( L, ix );

   R_nil();
   }

LUAFUNC_(SplitCurWnd) {
   const auto fSplitVertical = ToBOOL(I_(1));
         auto splitAt        = I_(2);    // this is a PERCENTAGE
   const auto pWin( g_CurWin() );
   const auto size   = fSplitVertical ? pWin->d_Size.col : pWin->d_Size.lin;
   const int  minwin = fSplitVertical ? MIN_WIN_WIDTH    : MIN_WIN_HEIGHT  ;
   if( size > (2*minwin)+1 ) { // +1 for border
      splitAt = (size * splitAt) / 100;
      NoLessThan( &splitAt, minwin );
      auto rv = SplitCurWnd( fSplitVertical, splitAt );
      if( rv ) {
         DispRefreshWholeScreenNow();
         return l_construct_Win( L, rv );
         }
      }

   R_nil();
   }

Win_(CurFBUF)    { return l_construct_FBUF( L, thisWin()->ViewHd.First()->FBuf() ); }
Win_(Height)     { R_int( thisWin()->d_Size.lin ); }
Win_(MakeCurrent){ SetWindowSetValidView_( thisWin() ); R_nil(); }


//------------------------------------------------------------------------------

#define  View_( API_name )  STATIC_FXN int l_View_method_ ## API_name ( lua_State *L )

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

Win_(CurView)      { return l_construct_View( L, thisWin()->ViewHd.First() ); }  // Note that View ctor is a Win _method_
FBUF_(PutFocusOn)  { return l_construct_View( L, thisPF()->PutFocusOn() ); }     // Note that View ctor is a FBUF _method_

int l_View_function_cur( lua_State *L ) { return l_construct_View( L, g_CurView() ); }
View_(Next)                             { return l_construct_View( L, DLINK_NEXT( thisVw(), dlinkViewsOfWindow ) ); }

View_(__tostring)  { auto pView = thisVw(); lua_pushfstring( L, "View(%p=\"%s\")", pView, pView->FBuf()->Name() ); return 1; }
View_(MoveCursor)  { thisVw()->MoveCursor( I_(2)-1, I_(3)-1 ); RZ; }
View_(GetCursorYX) {
   auto pView = thisVw();
   lua_pushinteger( L, pView->Cursor().lin+1 );
   lua_pushinteger( L, pView->Cursor().col+1 );
   lua_pushinteger( L, pView->Origin().lin+1 );
   lua_pushinteger( L, pView->Origin().col+1 );
   return 4;
   }

View_(FBuf)   { return l_construct_FBUF( L, thisVw()->FBuf() ); }
View_(FName)  { R_str( thisVw()->FBuf()->Name() ); }

View_(Get_LineCompile)  { R_int( thisVw()->Get_LineCompile()+1 );          }
View_(Set_LineCompile)  { thisVw()->Set_LineCompile( I_(2)-1 ); RZ; }

View_(HiliteMatch) { // params: int line, int col, int MatchCols
   const auto pv    ( thisVw() );
   const auto yLine ( I_(2)-1 );
   const auto xStart( I_(3)-1 );
   const auto Cols  ( I_(4)   );
   0 && DBG( "%s (%d,%d) L %d", __func__, yLine, xStart, Cols );
   const Point pt( yLine, xStart ); // NB: Lua uses 1-based line and column numbers!!!
   pv->SetMatchHiLite( pt, Cols, false );
   RZ;
   }

//------------------------------------------------------------------------------

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
   STATIC_CONST luaL_reg myLuaFuncs[] = {
      //#define  LUA_FUNC_I( func )   { #func, l_edfunc_##func },
#ifdef use_AddEdFxn
       { "AddEdFxn"                    , AddEdFxn                    },
#endif
       { "AddToSearchLog"              , AddToSearchLog              },
       { "AssignStrOk"                 , l_AssignStrOk               },
       { "PushVariableMacro"           , PushVariableMacro           },
       { "CmdIdxAddLuaFunc"            , CmdIdxAddLuaFunc            },
       { "SetKeyOk"                    , SetKeyOk                    },
       { "DBG"                         , DBG                         },
       { "CommonStrlenI"               , l_CommonStrlenI             },
       { "MarkDefineAtCurPos"          , MarkDefineAtCurPos          },
       { "MarkGoto"                    , MarkGoto                    },
       { "GetChildDirs"                , GetChildDirs                },
       { "GetCwd"                      , GetCwd                      },
       { "Getenv"                      , Getenv                      },
       { "GetenvOrNil"                 , GetenvOrNil                 },
       { "Get_SearchCaseSensitive"     , Get_SearchCaseSensitive     },
       { "Putenv"                      , Putenv                      },
       { "ModifyTimeOfDiskFile"        , ModifyTimeOfDiskFile        },
       { "Msg"                         , Msg                         },
       { "OsErrStr"                    , OsErrStr                    },
       { "OsVer"                       , OsVer                       },
       { "RsrcFilename"                , RsrcFilename                },
       { "StateFilename"               , StateFilename               },
       { "StartConProcess"             , StartConProcess             },
       { "StartGuiProcess"             , StartGuiProcess             },
       { "StartShellExecuteProcess"    , StartShellExecuteProcess    },
       { "Bell"                        , Bell                        },
       { "GetKey"                      , GetKey                      },
       { "SplitCurWnd"                 , SplitCurWnd                 },
       { "ScreenLines"                 , ScreenLines                 },
       { "ScreenCols"                  , ScreenCols                  },
       { "HideCursor"                  , HideCursor                  },
       { "UnhideCursor"                , UnhideCursor                },
       { "DispRefreshWholeScreenNow"   , DispRefreshWholeScreenNow   },
       { "VidWrStrColorFlush"          , VidWrStrColorFlush          },
       { "fExecute"                    , fExecute                    },
       { "fChangeFile"                 , fChangeFile                 },
       { "IsFile"                      , IsFile                      },
       { "IsDir"                       , IsDir                       },
       { "SleepMs"                     , SleepMs                     },

       { "valueof"                     , valueof                     },  // my Lua language extension

       { "split_ch"                    , split_ch                    }, //    Lua language extension (from PIL)
       { "split_str"                   , split_str                   }, //    Lua language extension (from PIL)
       { "split_ch_tbl"                , split_ch_tbl                },
       { "split_str_tbl"               , split_str_tbl               },
       { "enqueue_compile_jobs"        , enqueue_compile_jobs        },
       { "hexstr2bitstr"               , hexstr2bitstr               },
       { "bitstr2hexstr"               , bitstr2hexstr               },

      #undef   LUA_FUNC_I
      };

   for( const auto &lib : myLuaFuncs ) {
      DBG_LUA && DBG( "%s --- registering %s ---", __func__, lib.name );
      lua_register( L, lib.name, lib.func );
      DBG_LUA && DBG( "%s --- registered  %s ---", __func__, lib.name );
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
   , PCluaL_Reg metamethods
   , PCluaL_Reg functions
   , PCluaL_Reg methods
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

   if( metamethods )
      luaL_register( L, 0, metamethods );    // leaves TOS=metatable

   luaL_register( L, 0, methods );           // leaves TOS=metatable
   luaL_register( L, szObjName, functions ); // leaves TOS=metatable
   0 && LDS( __func__, L );
   0 && DBG( "%s: done +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-", __func__ );
   }


STATIC_FXN void l_register_Win_object( lua_State *L ) {
   STATIC_CONST luaL_reg metamethods[] = {
      { "__eq", l_Win_mmethod__eq },
      { "__lt", l_Win_mmethod__lt },
      { "__le", l_Win_mmethod__le },
      { "__gt", l_Win_mmethod__gt },
      { "__ge", l_Win_mmethod__ge },
      { 0 , 0 }
      };

   STATIC_CONST luaL_reg methods[] = {
      #define  LUA_FUNC_I( func )   { #func, l_Win_method_##func },
      LUA_FUNC_I(CurFBUF)
      LUA_FUNC_I(CurView)
      LUA_FUNC_I(Height)
      LUA_FUNC_I(MakeCurrent)
      #undef   LUA_FUNC_I
      { 0 , 0 }
      };

   STATIC_CONST luaL_reg functions[] = {
      { "getmetatable" , l_Win_function_getmetatable },
      { "cur"          , l_Win_function_cur         },
      { "getn"         , l_Win_function_getn        },
      { "by_filename"  , l_Win_function_by_filename },
      { 0 , 0 }
      };

   l_register_class( L, "Win", KevinsMetatable_Win, metamethods, functions, methods );
   }


STATIC_FXN void l_register_View_object( lua_State *L ) {
   STATIC_CONST luaL_reg methods[] = {
      #define  LUA_FUNC_I( func )   { #func, l_View_method_##func },
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
      #define  LUA_FUNC_I( func )   { #func, l_FBUF_method_##func },
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
      LUA_FUNC_I(GetLine)
      LUA_FUNC_I(GetLineRaw)
      LUA_FUNC_I(GetLineSeg)
      LUA_FUNC_I(PutLineSeg)
      LUA_FUNC_I(HasGlobalPtr)
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
      LUA_FUNC_I(SetTabconvOk)
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
      { "getmetatable"    , l_FBUF_function_getmetatable },
      { "new"             , l_FBUF_function_new     },
      { "new_may_create"  , l_FBUF_function_new_may_create },
      { "CurView"         , l_FBUF_function_curview },
      { "first"           , l_FBUF_function_first   },
      { "log"             , l_FBUF_function_getlog  },
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

//
// Copyright 2015-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

// functions called to write particular "system buffer" FBUFs ("pseudofiles"),
// called by ReadPseudoFileOk

#include "ed_main.h"
#include "ed_search.h"   // for RegexVersion()

STATIC_VAR CPCChar s_InvisibleFilenames[] = {
   kszClipboard  ,
   kszFiles      ,
// kszCompile    ,
   };

STATIC_VAR CPCChar s_UninterestingFilenames[] = {
   kszClipboard  ,
   kszFiles      ,
   kszSearchLog  ,
   kszEnvFile    ,
   kszAsgnFile   ,
   kszUsgFile    ,
   kszMacDefs    ,
   };

STATIC_VAR struct {
   PCChar name;
   int    counter;
   } pseudoBufInfo[] = {
     [to_underlying(ePseudoBufType::GREP)]={"grep"},
     [to_underlying(ePseudoBufType::SEL )]={"sel"},
   };

PFBUF PseudoBuf( ePseudoBufType pseudoBufType, bool fNew ) {
   auto bti( to_underlying(pseudoBufType) );
   auto &selNum( pseudoBufInfo[ bti ].counter );
   if( fNew ) {
      ++selNum;
      }
   return OpenFileNotDir_NoCreate( FmtStr<20>( "<%s.%u>", pseudoBufInfo[ bti ].name, selNum ) );
   }

bool ARG::nextselbuf() {
   ++pseudoBufInfo[ to_underlying(ePseudoBufType::SEL) ].counter;
   return true;
   }

STATIC_FXN bool FileMatchesNameInList( PCFBUF pFBuf, CPCChar *pC, size_t elements ) {
   for( decltype(elements) ix(0) ; ix < elements ; ++ix ) {
      if( pFBuf->NameMatch( pC[ix] ) ) {
         return true;
         }
      }
   return false;
   }

STATIC_FXN int IsWFilesName( stref pszName ) { // pszName matches "<win4>"
   if( !(pszName.starts_with( "<win" ) && pszName.ends_with( ">" )) ) {
      return -1;
      }
   const auto numst( pszName.substr( 4, pszName.length()-4-1 ) );
   for( auto ch : numst ) {
      if( !isdigit( ch ) ) {
         return -1;
         }
      }
   int errno_; uintmax_t wnum; stref txtConvd; UI bs; std::tie( errno_, wnum, txtConvd, bs ) = conv_u( numst, 10 );
   if( errno_ ) {
      return -1;
      }
   if( !(wnum < g_WindowCount()) ) {
      return -2;
      }
   return wnum;
   }

//==========================================================================================

bool FBUF::IsFileInfoFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Namestr() ) == widx ) {
      return true;
      }
   return NameMatch( kszFiles );
   }

bool FBUF::IsInvisibleFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Namestr() ) == widx ) {
      return true;
      }
   return FileMatchesNameInList( this, s_InvisibleFilenames, ELEMENTS( s_InvisibleFilenames ) );
   }

bool FBUF::IsInterestingFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Namestr() ) == widx ) {
      return false;
      }
   return !FileMatchesNameInList( this, s_UninterestingFilenames, ELEMENTS( s_UninterestingFilenames ) );
   }

STATIC_FXN int NextNInterestingFiles( int widx, bool fFromWinViewList, PFBUF pFBufs[], int pFBufsEls ) {
   auto ix( 0 );
   if( fFromWinViewList ) {
      DLINKC_FIRST_TO_LASTA( g_CurViewHd(), d_dlinkViewsOfWindow, pv ) // find
         if( pv->FBuf()->IsInterestingFile( widx ) ) {
            pFBufs[ix++] = pv->FBuf();
            if( !(ix < pFBufsEls) ) {
               break;
               }
            }
      }
   else {
#if FBUF_TREE
      rb_traverse( pNd, g_FBufIdx )
#else
      DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
         {
#if FBUF_TREE
         auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
         if( pFBuf->IsInterestingFile( widx ) ) {
            pFBufs[ix++] = pFBuf;
            if( !(ix < pFBufsEls) ) {
               break;
               }
            }
         }
      }
   return ix;
   }

bool ARG::files() {  // bound to alt+f2
// const auto fWinFiles( (g_WindowCount() > 1) != (d_cArg > 0) );
   const auto fWinFiles( true );
   const auto winIdx( g_CurWindowIdx() );
   if( !g_CurFBuf()->IsFileInfoFile( winIdx ) ) {
      return fChangeFile( fWinFiles ? FmtStr<20>("<win%" PR_SIZET ">", winIdx ) : kszFiles );
      }
   //////////////////////////////////////////////////////////////////////////
   //
   // <files> or <wfiles> WAS current when the cmd was executed:
   //
   // Put the top 2 interesting files in <files> at the VERY top of the <files>
   // stack, so that "setfile" function can can be used to toggle between them
   // directly.
   //
   //////////////////////////////////////////////////////////////////////////
   PFBUF pFBufs[2];
   if( ELEMENTS(pFBufs) != NextNInterestingFiles( winIdx, fWinFiles, pFBufs, ELEMENTS(pFBufs) ) ) {
      return Msg( "not enough files" );
      }
   0 && DBG( "%s: [0]=%s, [1]=%s", __func__, pFBufs[0]->Name(), pFBufs[1]->Name() );
   pFBufs[1]->PutFocusOn();
   pFBufs[0]->PutFocusOn();
   MsgClr();
   return true;
   }

void FBufRead_Assign_SubHd( PFBUF pFBuf, PCChar subhd, int count ) {
   pFBuf->FmtLastLine( "\n#-------------------- %d %s", count, subhd );
   }

stref BoostVersion() {
   STATIC_VAR char s_BoostVer[31] = { '\0', };
   if( '\0' == s_BoostVer[0] ) {
      safeSprintf( BSOB(s_BoostVer), "Boost %d.%d.%d"
                 , BOOST_VERSION / 100000
                 , BOOST_VERSION / 100 % 1000
                 , BOOST_VERSION % 100
         );
      }
   return s_BoostVer;
   }

STATIC_FXN void FBufRead_Assign( PFBUF pFBuf, int ) {
   pFBuf->SetBlockRsrcLd();
   pFBuf->FmtLastLine( "%s, %s, built %s, git clone %s", ProgramVersion(), ExecutableFormat(), kszDtTmOfBuild, kszMasterRepo );
   pFBuf->FmtLastLine( "  %" PR_BSR, BSR(CompilerVersion()) );
   const auto rexinfo( RegexVersion() );
   if( !rexinfo.empty() ) {
      pFBuf->FmtLastLine( "  %" PR_BSR, BSR(rexinfo) );
      }
   pFBuf->FmtLastLine( "  %" PR_BSR, BSR(BoostVersion()) );
   pFBuf->PutLastLineRaw( "" );
   FBufRead_Assign_OsInfo( pFBuf );
   pFBuf->FmtLastLine( " \n#-------------------- %s\npgmdir   %s\nstatedir %s\n", "Metadata", ThisProcessInfo::ExePath(), EditorStateDir() );
   pFBuf->FmtLastLine( " \n#-------------------- %s\nsizeof(FBUF) %" PR_SIZET "\nsizeof(View) %" PR_SIZET "\n", "Internals", sizeof(FBUF), sizeof(View) );
   FBufRead_Assign_Switches( pFBuf );
   // We do most print loops twice: first time to determine the count for the header
   auto macroCmds(0);
   auto luaCmds  (0);
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if(      pCmd->IsRealMacro() ) { ++macroCmds; }
      else if( pCmd->IsLuaFxn()    ) { ++luaCmds  ; }
      else                           { DBG( "%s ???", __func__ ); }
      }
   std::string tmp1, tmp2;
   FBufRead_Assign_SubHd( pFBuf, "Lua functions", luaCmds );
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsLuaFxn() ) { AssignShowKeyAssignment( *pCmd, pFBuf, tmp1, tmp2 ); }
      }
   FBufRead_Assign_intrinsicCmds( pFBuf, tmp1, tmp2 );
   FBufRead_Assign_SubHd( pFBuf, "Macros", macroCmds );
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() ) { AssignShowKeyAssignment( *pCmd, pFBuf, tmp1, tmp2 ); }
      }
   FBufRead_Assign_SubHd( pFBuf, "Available Keys", ShowAllUnassignedKeys( nullptr ) );
   ShowAllUnassignedKeys( pFBuf );
   }

struct maxFileInfos {
   COL        nmLen   ;
   LINE       lineCnt ;
   int        lineCntLog10 ;
   filesize_t imgSize ;
   int        imgSizeLog10 ;
   maxFileInfos()
      : nmLen   (0)
      , lineCnt (0), lineCntLog10(5)
      , imgSize (0), imgSizeLog10(9)
      {}
   void Accum( PCFBUF pFBuf ) {
      if( g_fShowFbufDetails && (pFBuf->HasLines() || pFBuf->FnmIsPseudo()) ) {
         NoLessThan( &imgSize , pFBuf->cbOrigFileImage() );
         NoLessThan( &lineCnt , pFBuf->LineCount()       );
         }
      NoLessThan(    &nmLen   , pFBuf->UserNameLen()     );
      }
   void Calc() {
      NoMoreThan( &nmLen, 120 );
      lineCntLog10 = uint_log_10( lineCnt );
      imgSizeLog10 = uint_log_10( imgSize );
      }
   };

STATIC_FXN void ShowAFilesInfo( PFBUF pFout, PFBUF pFBuf, maxFileInfos const &max ) {
   if( g_fShowFbufDetails && (pFBuf->HasLines() || pFBuf->FnmIsPseudo()) ) {
      char entabStr[] = { 'e', char( '0' + pFBuf->Entab() ), 0 };
      pFout->FmtLastLine(
#if defined(_WIN32)
         "%-*s %c%*d L %*" PR_FILESIZET " %d%s {%s%s%s%s%s%s}"
#else
         "%-*s %c%*d L %*" PR_FILESIZET " %d%s {%s%s%s%s%s}"
#endif
            , max.nmLen, pFBuf->UserName().c_str()
               , pFBuf->IsDirty()         ? '*'       : ' '
                       , max.lineCntLog10, pFBuf->LineCount()
                           , max.imgSizeLog10, pFBuf->cbOrigFileImage()
                             , pFBuf->TabWidth()
                             , pFBuf->Entab()           ? entabStr  : "t "
                             , pFBuf->IsSysPseudo()     ? "S+"      : ""
                             , pFBuf->FnmIsPseudo()     ? "Pseu,"   : ""
      //                     , pFBuf->HasLines()        ? "Lines,"  : ""
                             , pFBuf->ToForgetOnExit()  ? "Txnt,"   : ""
                             , pFBuf->IsNoEdit()        ? "RO,"     : ""
#if defined(_WIN32)
                             , pFBuf->IsDiskRO()        ? "DiskRO," : ""
#endif
                             , pFBuf->EolName()
         );
      }
   else {
      pFout->PutLastLineRaw( pFBuf->UserName() );
      }
   }

STATIC_FXN void FBufRead_Files( PFBUF pFout, int ) { // fxn that fills kszFiles
   //*** preprocessing pass to determine max width of some fields:
   maxFileInfos max;
   {
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      PCFBUF pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( !pFBuf->HasLines() ) {
         continue;
         }
      max.Accum( pFBuf );
      }
   }
   max.Calc();
   //*** content-fill pass:
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( !pFBuf->HasLines() ) {
         continue;
         }
      ShowAFilesInfo( pFout, pFBuf, max );
      }
   }

void FBufRead_WInformation( PFBUF pFout, int widx ) { // fxn that fills "<winN>"
   //*** preprocessing pass to determine max width of some fields:
   maxFileInfos max;
   DLINKC_FIRST_TO_LASTA( g_CurViewHd(), d_dlinkViewsOfWindow, pv ) {
      auto pFBuf( pv->FBuf() );
      if( !pFBuf || pFBuf->IsInvisibleFile( widx ) ) {
         continue;
         }
      max.Accum( pFBuf );
      }
   max.Calc();
   //*** content-fill pass:
   DLINKC_FIRST_TO_LASTA( g_CurViewHd(), d_dlinkViewsOfWindow, pv ) {
      auto pFBuf( pv->FBuf() );
      if( !pFBuf || pFBuf->IsInvisibleFile( widx ) ) {
         continue;
         }
      ShowAFilesInfo( pFout, pFBuf, max );
      }
   }

STATIC_FXN void FBufRead_MacDefs( PFBUF pFBuf, int ) {
   pFBuf->SetBlockRsrcLd();
   auto nmLen(0);
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() ) {
         nmLen = std::max( nmLen, Strlen( pCmd->Name() ) );
         }
      }
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinNil() ; pNd = CmdIdxNext( pNd ) ) { // sorted traversal
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() ) {
         pFBuf->PutLastLineRaw( SprintfBuf( "%-*s %s", nmLen, pCmd->Name(), pCmd->MacroText() ).c_str() );
         }
      }
   }

//============================================================================

STATIC_FXN void FBufRead_Environment( PFBUF pFBuf, int ) {
   for( auto ix(0) ; g_envp[ ix ] ; ++ix ) {
      FBOP::InsLineSortedAscending( pFBuf, 0, g_envp[ix] );
      }
   }

STATIC_FXN int CDECL__ qsort_cmp_env( PCVoid pA, PCVoid pB ) {
   return Stricmp( *static_cast<CPPCChar>(pA), *static_cast<CPPCChar>(pB) );
   }

STATIC_FXN void FBufRead_MyEnvironment( PFBUF pFBuf, int ) {
   auto envEntries(0);
   for( auto ix(0) ; g_envp[ ix ] ; ++ix ) {
      ++envEntries;
      }
   PPCChar lines;  AllocArrayNZ( lines, envEntries );
   for( auto ix(0) ; g_envp[ ix ] ; ++ix ) {
      lines[ix] =  g_envp[ ix ];
      }
   qsort( lines, envEntries, sizeof(*lines), qsort_cmp_env );
   auto bytes(0);
   std::string sbuf;
   for( auto ix(0) ; g_envp[ ix ] ; ++ix ) {
      ALLOCA_STRDUP( envstr, len, lines[ix], Strlen( lines[ix] ) );
      bytes += len + 1;
      PCChar pEos( envstr + len );
      PCChar pFirstSeg( envstr );
      CPCChar pEQ( strchr( envstr, '=' ) );
      const int  indent( pd2Int( pEQ - envstr + 1 ) );
      PCChar pPrevSEMI( pEQ );
      while( PCChar pSEMI = strchr( pPrevSEMI+1, WL( ';',':' ) ) ) {
         const auto pSegStart( pFirstSeg ? pFirstSeg : pPrevSEMI+1 );
         const auto curIndent( pFirstSeg ? 0 : indent );
         const auto slen( pd2Int( pSEMI - pSegStart ) );
         if( slen > 0 ) {
            sbuf.assign( curIndent, ' ' );
            sbuf.append( pSegStart, slen );
            pFBuf->PutLastLineRaw( sbuf );
            }
         pPrevSEMI = pSEMI;
         pFirstSeg = nullptr;
         }
      const auto pSegStart( pFirstSeg ? pFirstSeg : pPrevSEMI+1 );
      const auto curIndent( pFirstSeg ? 0 : indent );
      const auto slen( pd2Int( pEos - pSegStart ) );
      if( slen > 0 ) {
         sbuf.assign( curIndent, ' ' );
         sbuf.append( pSegStart, slen );
         pFBuf->PutLastLineRaw( sbuf );
         }
      }
   Msg( "environment size = %d bytes", bytes );
   Free0( lines );
   }

struct UsageCtxt {
   PFBUF       fbOut;
   COL         maxCmdNmLen;
   uint32_t    maxCallCount;
   std::string dest;
   std::string tmp;
   };

STATIC_FXN void CalledMaxNmLen( PCCMD Cmd, void *pCtxt ) {
   if( Cmd->d_gCallCount ) {
      const auto len( Strlen( Cmd->Name() ) );
      auto uc( static_cast<UsageCtxt*>(pCtxt) );
      NoLessThan( &uc->maxCmdNmLen , len );
      NoLessThan( &uc->maxCallCount, Cmd->d_gCallCount );
      }
   }

STATIC_FXN void ShowCalls( PCCMD Cmd, void *pCtxt ) {
   if( Cmd->d_gCallCount ) {
      auto uc( static_cast<UsageCtxt*>(pCtxt) );
      uc->dest.assign( FmtStr<132>( "%*u  %-*s  ", uc->maxCallCount, Cmd->d_gCallCount, uc->maxCmdNmLen, Cmd->Name() ).c_str() );
      if(   Cmd == pCMD_graphic
         || Cmd == pCMD_unassigned
        ) {
         uc->dest.append( "(many)" );
         }
      else {
         uc->dest.append( KeyNmAssignedToCmd_all( Cmd, "," ) );
         }
      FBOP::InsLineSortedDescending( uc->fbOut, 0, uc->dest.c_str() );
      }
   }

STATIC_FXN int CDECL__ qsort_cmp_fbuf_wrtime( PCVoid pA, PCVoid pB ) {
   const auto pFA( *static_cast<CPPFBUF>(pA) ); const auto tA( pFA->get_tmLastWrToDisk() );
   const auto pFB( *static_cast<CPPFBUF>(pB) ); const auto tB( pFB->get_tmLastWrToDisk() );
   const auto rv( tA==tB ? 0 : tA>tB ? -1 : 1 ); // descending sort: greatest first/top
   0 && DBG( "%s %" PR_TIMET ", %s %" PR_TIMET " = %d", pFA->Name(), tA, pFB->Name(), tB, rv );
   return rv;
   }

bool ARG::wr0() {
   auto count( 0u );
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->get_tmLastWrToDisk() > 0 ) {
         pFBuf->clr_tmLastWrToDisk();
         ++count;
         }
      }
   Msg( "clr_tmLastWrToDisk() on %u files", count );
   return false;
   }

STATIC_FXN void FBufRead_WrToDisk( PFBUF dest, int ) { enum {SD=0}; SD && DBG( "%s", FUNC );
   auto count( 0u );
   {
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->get_tmLastWrToDisk() > 0 ) {
         ++count;
         }
      }
   }
   PPFBUF fbufs;  AllocArrayNZ( fbufs, count );
   auto ix( 0u );
   {
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->get_tmLastWrToDisk() > 0 ) { SD && DBG( "[%u] %s %" PR_TIMET, ix, pFBuf->Name(), pFBuf->get_tmLastWrToDisk() );
         fbufs[ix++] = pFBuf;
         }
      }
   }
   qsort( fbufs, count, sizeof(*fbufs), qsort_cmp_fbuf_wrtime );
   for( ix=0u ; ix < count ; ++ix ) {
      PCFBUF pFBuf( fbufs[ix] );
      dest->PutLastLineRaw( pFBuf->Name() ); SD && DBG( "s[%u] %s %" PR_TIMET, ix, pFBuf->Name(), pFBuf->get_tmLastWrToDisk() );
      }
   Free0( fbufs );
   Msg( "%u files have been written to disk", count );
   }

STATIC_FXN void FBufRead_Usage( PFBUF pFBuf, int ) { 0 && DBG( "%s", FUNC );
   cmdusage_updt();
   UsageCtxt uc;
   uc.fbOut        = nullptr;
   uc.maxCmdNmLen  = 0;
   uc.maxCallCount = 0;
   WalkAllCMDs( static_cast<void *>(&uc), CalledMaxNmLen );
   uc.fbOut        = pFBuf;
   uc.maxCallCount = uint_log_10( uc.maxCallCount );
   WalkAllCMDs( static_cast<void *>(&uc), ShowCalls );
   }

STATIC_FXN void FBufRead_AsciiTbl( PFBUF pFBuf, int ) {
   pFBuf->PutLastLineRaw( "    0123 4567 89AB CDEF" );
   linebuf buf; buf[0] = '\0'; auto pb( buf ); auto sb( sizeof buf );
   for( auto ch(0); ch < 0x100; ++ch ) {
      if( (ch % 0x10) == 0 ) {
         if( buf[0] ) { pFBuf->PutLastLineRaw( buf ); }
         pb = buf, sb = sizeof buf;
         snprintf_full( &pb, &sb, "%02X:", ch );
         }
      snprintf_full( &pb, &sb, "%s%c", (ch%4)==0?" ":"", ch<' '?'.':ch );
      }
   }

typedef void (*FbufReaderFxn)( PFBUF, int );

STATIC_FXN void CallFbufReader( PFBUF pFBuf, FbufReaderFxn readerFxn, int instance ) {
   pFBuf->MakeEmpty();
   readerFxn( pFBuf, instance );
   pFBuf->MoveCursorToBofAllViews();
   pFBuf->Undo_Reinit();
   pFBuf->UnDirty();
   pFBuf->SetAutoRead();
   }

bool ReadPseudoFileOk( PFBUF pFBuf ) { enum {SD=0};  SD && DBG( "%s %s'", FUNC, pFBuf->Name() );
   /* shortcoming  BUGBUG

      This ALWAYS causes *pFBuf to be rewritten, and all associated views'
      cursor positions reset.  This policy only makes sense for those *pFBuf's
      for which the backing data is likely to change each time the *pFBuf gets
      focus; for the others, the loss of cursor position is annoying (and comes
      with no benefit).

      Also, currently these fbufs are all editable, and it can be disconcerting
      for a user to edit one of these, switch away, switch back, and have their
      edits be unrecoverably wiped out

    */
   if( LuaCtxt_Edit::ReadPseudoFileOk( pFBuf ) ) {  SD && DBG( "%s %s' Lua read it", FUNC, pFBuf->Name() );
      return true;
      }
   STATIC_CONST struct {
      PCChar         name;
      FbufReaderFxn  readerFxn;
      } pseudofileReaders[] = {
         { kszFiles     , FBufRead_Files         },
         { kszMacDefs   , FBufRead_MacDefs       },
         { kszEnvFile   , FBufRead_Environment   },
         { kszMyEnvFile , FBufRead_MyEnvironment },
         { kszAsgnFile  , FBufRead_Assign        },
         { kszUsgFile   , FBufRead_Usage         },
         { "<most_recently_written_files>" , FBufRead_WrToDisk      },
         { "<ascii>"   , FBufRead_AsciiTbl      },
      };
                                                     SD && DBG( "%s %s' looping", FUNC, pFBuf->Name() );
   for( const auto &pfR : pseudofileReaders ) {
      if( pFBuf->NameMatch( pfR.name ) ) {           SD && DBG( "%s %s' matches %s", FUNC, pFBuf->Name(), pfR.name );
         CallFbufReader( pFBuf, pfR.readerFxn, 0 );
         return true;
         }
      }
   const auto widx( IsWFilesName( pFBuf->Namestr() ) );
   if( widx >= 0 ) {
      CallFbufReader( pFBuf, FBufRead_WInformation, widx );
      }
   else if( widx == -2 ) {
      return Msg( "that window was deleted" ); // caller should clobber pFBuf!
      }
   return true;
   }

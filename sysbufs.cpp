//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

// functions called to write particular "system buffer" FBUFs ("pseudofiles"),
// called by ReadPseudoFileOk

#include "ed_main.h"

GLOBAL_CONST char szClipboard[] = "<clipboard>";
GLOBAL_CONST char szConsole[] = "<console>";
GLOBAL_CONST char szSearchLog[] = "<search-keys>";
GLOBAL_CONST char szSearchRslts[] = "<search-results>";
GLOBAL_CONST char szStkname  [] = "<stack>";
GLOBAL_CONST char szCwdStk   [] = "<cwd>";
GLOBAL_CONST char szFiles    [] = "<files>";
GLOBAL_CONST char szEnvFile  [] = "<env_>";
GLOBAL_CONST char szMyEnvFile[] = "<env>";
GLOBAL_CONST char szAsnFile  [] = "<CMD-SWI-Keys>";
GLOBAL_CONST char szUsgFile  [] = "<usage>";
GLOBAL_CONST char szMacDefs  [] = "<macdefs>";
GLOBAL_CONST char szNoFile   [] = "*";


STATIC_VAR CPCChar s_InvisibleFilenames[] = {
   szClipboard  ,
   szFiles      ,
// szCompile    ,
   };

STATIC_VAR CPCChar s_UninterestingFilenames[] = {
   szClipboard  ,
   szFiles      ,
   szSearchLog  ,
   szEnvFile    ,
   szAsnFile    ,
   szUsgFile    ,
   szMacDefs    ,
   };

STATIC_FXN bool FileMatchesNameInList( PCFBUF pFBuf, CPCChar *pC, CPCChar *pPastEnd ) {
   for( ; pC < pPastEnd ; ++pC )
      if( pFBuf->NameMatch( *pC ) )
         return true;

   return false;
   }

bool FBUF::IsFileInfoFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Name() ) == widx )
      return true;

   return NameMatch( szFiles );
   }

bool FBUF::IsInvisibleFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Name() ) == widx )
      return true;

   return FileMatchesNameInList( this, s_InvisibleFilenames, PAST_END( s_InvisibleFilenames ) );
   }

bool FBUF::IsInterestingFile( int widx ) const {
   if( widx >= 0 && IsWFilesName( Name() ) == widx )
      return false;

   return !FileMatchesNameInList( this, s_UninterestingFilenames, PAST_END( s_UninterestingFilenames ) );
   }

STATIC_FXN int NextNInterestingFiles( int widx, bool fFromWinViewList, PFBUF pFBufs[], int pFBufsEls ) {
   auto ix( 0 );
   if( fFromWinViewList ) {
      DLINKC_FIRST_TO_LASTA( g_CurViewHd(), dlinkViewsOfWindow, pv ) // find
         if( pv->FBuf()->IsInterestingFile( widx ) ) {
            pFBufs[ix++] = pv->FBuf();
            if( !(ix < pFBufsEls) )
               break;
            }
      }
   else {
#if FBUF_TREE
      RbNode *pNd;
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
            if( !(ix < pFBufsEls) )
               break;
            }
         }
      }

   return ix;
   }


bool ARG::files() {  // bound to alt+f2
// const auto fWinFiles( (g_iWindowCount() > 1) != (d_cArg > 0) );
   const auto fWinFiles( true );
   const auto winIdx( g_CurWindowIdx() );
   if( !g_CurFBuf()->IsFileInfoFile( winIdx ) ) {
      return fChangeFile( fWinFiles ? FmtStr<20>("<win%d>", winIdx ) : szFiles );
      }

   //////////////////////////////////////////////////////////////////////////
   //
   // <files> or <wfiles> WAS current when the cmd was executed:
   //
   // Put the top 2 interesting files in <files> at the VERY top of the <files>
   // stack, so that "setfile" function can can be used to swap between them
   // directly.
   //
   //////////////////////////////////////////////////////////////////////////

   PFBUF pFBufs[2];
   if( ELEMENTS(pFBufs) != NextNInterestingFiles( winIdx, fWinFiles, pFBufs, ELEMENTS(pFBufs) ) )
      return Msg( "not enough files" );

   0 && DBG( "%s: [0]=%s, [1]=%s", __func__, pFBufs[0]->Name(), pFBufs[1]->Name() );

   pFBufs[1]->PutFocusOn();
   pFBufs[0]->PutFocusOn();
   MsgClr();
   return true;
   }


void FBufRead_Assign_SubHd( PFBUF pFBuf, PCChar subhd, int count ) {
   pFBuf->PutLastLine( " " );
   if(true)
      pFBuf->FmtLastLine( "#-------------------- %d %s", count, subhd );
   else {
      CPCChar frags[] = { "#-------------------- ", FmtStr<12>( "%d ", count ), subhd };  // this is overkill (in this case)
      pFBuf->PutLastLine( frags, ELEMENTS(frags) );
      }
   }

STATIC_FXN void FBufRead_Assign( PFBUF pFBuf, int ) {
   pFBuf->SetBlockRsrcLd();
   pFBuf->FmtLastLine( "%s, %s, compiled %s", ProgramVersion(), ExecutableFormat(), kszDtTmOfBuild );
   pFBuf->PutLastLine( " " );
   pFBuf->FmtLastLine( "#-------------------- %s", "Win32 Status" );
   FBufRead_Assign_Win32( pFBuf );

   FBufRead_Assign_Switches( pFBuf );

   auto intrinsicCmds(0);
   for( auto pNd( CmdIdxFirst() ) ; pNd != CmdIdxLast() ; pNd = CmdIdxNext( pNd ) )
      ++intrinsicCmds;

   auto macroCmds(0);
   auto luaCmds  (0);
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if(      pCmd->IsRealMacro() ) ++macroCmds;
      else if( pCmd->IsLuaFxn()    ) ++luaCmds  ;
      else                           DBG( "%s ???", __func__ );
      }

   FBufRead_Assign_SubHd( pFBuf, "Lua functions", luaCmds );
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsLuaFxn() ) PAssignShowKeyAssignment( *pCmd, pFBuf );
      }

   // We do most print loops twice: first time to determine the count for the
   // header
   //
   FBufRead_Assign_SubHd( pFBuf, "Intrinsic Functions", intrinsicCmds );
   for( auto pNd( CmdIdxFirst() ) ; pNd != CmdIdxLast() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if(      pCmd->IsRealMacro() ) ;
      else if( pCmd->IsLuaFxn()    ) ;
      else PAssignShowKeyAssignment( *pCmd, pFBuf );
      }

   FBufRead_Assign_SubHd( pFBuf, "Macros", macroCmds );
   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() ) PAssignShowKeyAssignment( *pCmd, pFBuf );
      }

   FBufRead_Assign_SubHd( pFBuf, "Available Keys", ShowAllUnassignedKeys( nullptr ) );
   ShowAllUnassignedKeys( pFBuf );
   }

GLOBAL_VAR bool g_fShowFbufDetails = false;

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
   pathbuf pb;
   if( g_fShowFbufDetails && (pFBuf->HasLines() || pFBuf->FnmIsPseudo()) ) {
      char entabStr[] = { 'e', char( '0' + pFBuf->TabConv() ), 0 };
      pFout->FmtLastLine(
#if defined(_WIN32)
         "%-*s %c%*d L %*u %d%s {%s%s%s%s%s%s}"
#else
         "%-*s %c%*d L %*u %d%s {%s%s%s%s%s}"
#endif
            , max.nmLen, pFBuf->UserName( BSOB(pb) )
               , pFBuf->IsDirty()         ? '*'       : ' '
                       , max.lineCntLog10, pFBuf->LineCount()
                           , max.imgSizeLog10, pFBuf->cbOrigFileImage()
                             , pFBuf->TabWidth()
                             , pFBuf->TabConv()         ? entabStr  : "t "
                             , pFBuf->HasGlobalPtr()    ? "G,"      : ""
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
   else
      pFout->FmtLastLine( "%s", pFBuf->UserName( BSOB(pb) ) );
   }



STATIC_FXN void FBufRead_Files( PFBUF pFout, int ) { // fxn that fills szFiles
   //*** preprocessing pass to determine max width of some fields:
   maxFileInfos max;

   {
#if FBUF_TREE
   RbNode *pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      PCFBUF pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( !pFBuf->HasLines() )
         continue;

      max.Accum( pFBuf );
      }
   }

   max.Calc();

   //*** content-fill pass:

   {
#if FBUF_TREE
   RbNode *pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      PCFBUF pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( !pFBuf->HasLines() )
         continue;

      ShowAFilesInfo( pFout, pFBuf, max );
      }
   }

   pFout->SetFileType( ftype_UNKNOWN );
   }

void FBufRead_WInformation( PFBUF pFout, int widx ) { // fxn that fills "<winN>"

   //*** preprocessing pass to determine max width of some fields:
   maxFileInfos max;
   DLINKC_FIRST_TO_LASTA( g_CurViewHd(), dlinkViewsOfWindow, pv ) {
      auto pFBuf( pv->FBuf() );
      if( !pFBuf || pFBuf->IsInvisibleFile( widx ) )
         continue;

      max.Accum( pFBuf );
      }
   max.Calc();

   //*** content-fill pass:
   DLINKC_FIRST_TO_LASTA( g_CurViewHd(), dlinkViewsOfWindow, pv ) {
      auto pFBuf( pv->FBuf() );
      if( !pFBuf || pFBuf->IsInvisibleFile( widx ) )
         continue;

      ShowAFilesInfo( pFout, pFBuf, max );
      }

   pFout->SetFileType( ftype_UNKNOWN );
   }

STATIC_FXN void FBufRead_MacDefs( PFBUF pFBuf, int ) {
   pFBuf->SetBlockRsrcLd();
   auto nmLen(0);

   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) ) {
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() ) {
         Max( &nmLen, Strlen( pCmd->Name() ) );
         }
      }

   for( auto pNd( CmdIdxAddinFirst() ) ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) ) { // sorted traversal
      const auto pCmd( CmdIdxToPCMD( pNd ) );
      if( pCmd->IsRealMacro() )
         pFBuf->PutLastLine( SprintfBuf( "%-*s %s", nmLen, pCmd->Name(), pCmd->MacroText() ) );
      }
   }

//============================================================================

STATIC_FXN void FBufRead_Environment( PFBUF pFBuf, int ) {
   std::string tmp;
   for( auto ix(0) ; g_envp[ ix ] ; ++ix )
      FBOP::InsLineSortedAscending( pFBuf, tmp, 0, g_envp[ix] );
   }

STATIC_FXN int CDECL__ qsort_cmp_env( PCVoid pA, PCVoid pB ) {
   return Stricmp( *PPCChar(pA), *PPCChar(pB) );
   }

STATIC_FXN void FBufRead_MyEnvironment( PFBUF pFBuf, int ) {
   auto envEntries(0);
   for( auto ix(0) ; g_envp[ ix ] ; ++ix )
      ++envEntries;

   PPCChar             lines  ;
   AllocArrayNZ(       lines  , envEntries, __func__ );

   for( auto ix(0) ; g_envp[ ix ] ; ++ix )
      lines[ix] =  g_envp[ ix ];

   qsort( lines, envEntries, sizeof(*lines), qsort_cmp_env );

   Xbuf xb;
   auto bytes(0);
   for( auto ix(0) ; g_envp[ ix ] ; ++ix ) {
      ALLOCA_STRDUP( envstr, len, lines[ix], Strlen( lines[ix] ) );
      bytes += len + 1;
      PCChar pEos( envstr + len );
      PCChar pFirstSeg( envstr );
      CPCChar pEQ( strchr( envstr, '=' ) );
      const int  indent( pd2Int( pEQ - envstr + 1 ) );
      PCChar pPrevSEMI( pEQ );
      while( PCChar pSEMI = strchr( pPrevSEMI+1, ';' ) ) {
         const auto pSegStart( pFirstSeg ? pFirstSeg : pPrevSEMI+1 );
         const auto curIndent( pFirstSeg ? 0 : indent );
         const auto slen( pd2Int( pSEMI - pSegStart ) );
         if( slen > 0 ) {
            pFBuf->xFmtLastLine( &xb, "%*.*s%*.*s", curIndent, curIndent, "", slen, slen, pSegStart );
            }

         pPrevSEMI = pSEMI;
         pFirstSeg = nullptr;
         }

      const auto pSegStart( pFirstSeg ? pFirstSeg : pPrevSEMI+1 );
      const auto curIndent( pFirstSeg ? 0 : indent );
      const auto slen( pd2Int( pEos - pSegStart ) );
      if( slen > 0 ) {
         pFBuf->xFmtLastLine( &xb, "%*.*s%*.*s", curIndent, curIndent, "", slen, slen, pSegStart );
         }
      }

   Msg( "environment size = %d bytes", bytes );

   Free0( lines );
   }


struct UsageCtxt {
   PFBUF fbOut;
   COL   maxCmdNmLen;
   U32   maxCallCount;
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
      linebuf lbuf;
      if(   Cmd == pCMD_graphic
         || Cmd == pCMD_unassigned
        ) {
         SafeStrcpy( lbuf, "(many)" );
         }
      else {
         StringOfAllKeyNamesFnIsAssignedTo( BSOB(lbuf), Cmd, "," );
         }
      std::string tmp;
      FBOP::InsLineSortedDescending( uc->fbOut, tmp, 0, SprintfBuf( "%*u  %-*s  %s", uc->maxCallCount, Cmd->d_gCallCount, uc->maxCmdNmLen, Cmd->Name(), lbuf ) );
      }
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
   pFBuf->PutLastLine( "    0123 4567 89AB CDEF" );
   linebuf buf; buf[0] = '\0'; auto pb( buf ); auto sb( sizeof buf );
   for( auto ch(0); ch < 0x100; ++ch ) {
      if( (ch % 0x10) == 0 ) {
         if( buf[0] )  pFBuf->PutLastLine( buf );
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
   pFBuf->ClearUndo();
   pFBuf->UnDirty();
   pFBuf->SetAutoRead();
   }

bool ReadPseudoFileOk( PFBUF pFBuf ) { enum {DB=0};  DB && DBG( "%s %s'", FUNC, pFBuf->Name() );
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

   {
   int luaReadOk( false );
   if( LuaCtxt_Edit::ReadPseudoFileOk( pFBuf, &luaReadOk ) && luaReadOk ) {  DB && DBG( "%s %s' Lua read it", FUNC, pFBuf->Name() );
      return true;
      }
   }
   STATIC_CONST struct {
      PCChar         name;
      FbufReaderFxn  readerFxn;
      } pseudofileReaders[] = {
         { szFiles     , FBufRead_Files         },
         { szMacDefs   , FBufRead_MacDefs       },
         { szEnvFile   , FBufRead_Environment   },
         { szMyEnvFile , FBufRead_MyEnvironment },
         { szAsnFile   , FBufRead_Assign        },
         { szUsgFile   , FBufRead_Usage         },
         { "<ascii>"   , FBufRead_AsciiTbl      },
      };
                                                     DB && DBG( "%s %s' looping", FUNC, pFBuf->Name() );
   for( const auto &pfR : pseudofileReaders ) {
      if( pFBuf->NameMatch( pfR.name ) ) {           DB && DBG( "%s %s' matches %s", FUNC, pFBuf->Name(), pfR.name );
         CallFbufReader( pFBuf, pfR.readerFxn, 0 );
         return true;
         }
      }

   const auto widx( IsWFilesName( pFBuf->Name() ) );

   if( widx >= 0 ) {
      CallFbufReader( pFBuf, FBufRead_WInformation, widx );
      }
   else if( widx == -2 ) {
      return Msg( "that window was deleted" ); // caller should clobber pFBuf!
      }

   return true;
   }

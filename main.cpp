//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "ed_search.h"
#include "my_fio.h"
#include "getopt.h"

//--------------------------------------

GLOBAL_VAR bool  g_fShowMemUseInK     =    true;
GLOBAL_VAR bool  g_fCase                       ;
GLOBAL_VAR bool  g_fEditReadonly               ;
GLOBAL_VAR bool  g_fErrPrompt         =    true;
GLOBAL_VAR bool  g_fSoftCr            =    true;
GLOBAL_VAR bool  g_fTabAlign                   ;
GLOBAL_VAR bool  g_fViewOnly                   ;
GLOBAL_VAR bool  g_fWordwrap                   ;
GLOBAL_VAR bool  g_fFuncRetVal                 ;
GLOBAL_VAR bool  g_fMeta                       ;
GLOBAL_VAR bool  g_fMsgflush                   ;
GLOBAL_VAR bool  g_fViewsActivelyTailOutput = true;
GLOBAL_VAR int   g_ClipboardType               ; // 0 == EMPTY
GLOBAL_VAR int   g_iTabWidth          =       8;
GLOBAL_VAR int   g_iHike              =      25;
GLOBAL_VAR int   g_iHscroll           =      10;
GLOBAL_VAR int   g_iRmargin           =      80;
GLOBAL_VAR int   g_iMaxfilehist       =       0; // Keep info on last 'maxfilehist' files edited (0=never ends)
GLOBAL_VAR int   g_iMaxUndel          =    5000;
GLOBAL_VAR int   g_iMaxUndo           =  100000;
GLOBAL_VAR int   g_iVscroll           =       1;

GLOBAL_VAR bool  g_fSelKeymapEnabled;

GLOBAL_VAR PFBUF g_pFbufClipboard          ;
GLOBAL_VAR PFBUF g_pFBufCmdlineFiles       ;
GLOBAL_VAR PFBUF s_pFbufLog                ;
GLOBAL_VAR PFBUF g_pFBufConsole            ;

#ifdef __GNUC__
//=================================================================
// replace dflt abort() RTL fxn with one that lets us access gdb
EXTERNC void abort() {
   *((char *)nullptr) = 0;  // cause a "segfault" to drop us into the debugger
   while(1) {}  // dflt abort() RTL fxn is apparently prototyped with "__attribute__((noreturn))"; w/o this code I get "warning: 'noreturn' function does return [enabled by default]"
   }
//=================================================================
#endif


STATIC_FXN void AddCmdlineFile( PCChar filename, bool fForgetFile ) { 1 && DBG( "%s(%s)", FUNC, filename );
      g_pFBufCmdlineFiles->FmtLastLine( "%s%s", (fForgetFile ? "|" : ""), filename );
   // g_pFBufCmdlineFiles->PutLastLine( filename );
   std::string st;
   g_pFBufCmdlineFiles->getLineRaw( st, 0 );
   1 && DBG( "%s readback(%s)", FUNC, st.c_str() );
   }

STATIC_FXN int NumberOfCmdlineFilesRemaining() {
   return g_pFBufCmdlineFiles ? g_pFBufCmdlineFiles->LineCount() : 0;
   }

STATIC_FXN bool SwitchToNextCmdlineFile() {
   std::string sb;
   while( FBOP::PopFirstLine( sb, g_pFBufCmdlineFiles ) ) {
      auto pNxtArg( sb.c_str() );  DBG( "%s trying %s", FUNC, pNxtArg );
      const auto fOptT( pNxtArg[0] == '|' );
      const auto pFnm( pNxtArg + (fOptT ? 1 : 0) );
      if( fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( pFnm ) ) {
         if( fOptT )
            g_CurFBuf()->SetForgetOnExit();

         return true;
         }                        DBG( "%s failed %s", FUNC, pNxtArg );
      }

   return false;
   }


STATIC_CONST char kszThisProgVersion[] = "2.0";
STATIC_CONST char kszTmpFBufMagic[] = "FMT0";

PCChar ProgramVersion() {
   STATIC_VAR char szProgramVersion[40];
   if( !szProgramVersion[0] )
      safeSprintf( BSOB(szProgramVersion), "Kevin's Editor %s", kszThisProgVersion );

   return szProgramVersion;
   }

PCChar ExecutableFormat() {
   STATIC_CONST char kszExecutableFormat_[] =
#ifdef __x86_64
        "x64"
      //"pei-x86-64"
#else
        "i386"
      //"pei-i386"
#endif
      ;
   return kszExecutableFormat_;
   }


STATIC_FXN bool fgotline( Xbuf *xb, FILE *f ) {
   if( feof( f ) ) return nullptr;
   auto ofs( 0 ); enum { bump=16 };
   PChar rvbuf( nullptr );
   for(;;) {
      const auto buf( xb->wbuf()+ofs );
      const auto fgrv( fgets( buf, xb->buf_bytes()-ofs, f ) );
      if( fgrv != buf )
         return rvbuf != nullptr;

      if( !rvbuf ) rvbuf = xb->wbuf();

      const auto len( Strlen( buf ) );
      if( len > 0 && buf[len-1] == '\n' ) {
         buf[len-1] = '\0';
         return rvbuf != nullptr;
         }
      xb->kresize( xb->buf_bytes() + bump );
      // if( bump < 2048 )  bump <<= 1;
      }
   }

STATIC_FXN void StrStartOfNext2Tokens( PChar pszStringToSplit, PPChar pchTokenStart, PPChar pchNextTokenStart ) {
   *pchTokenStart = StrPastAnyWhitespace( pszStringToSplit );
   auto pC( StrToNextWhitespaceOrEos( *pchTokenStart ) );
   if( *pC ) {
      *pC++ = '\0';
      pC = StrPastAnyWhitespace( pC );
      }

   *pchNextTokenStart = pC;
   }


STATIC_VAR struct { // kludgy, but alternative is to pass params thru the depths ... not!
   bool fDoIt;
   int  droppedFiles;
   } s_tmpFBufCleanup;

STATIC_VAR PFBUF s_pFForgottenFiles;

STATIC_FXN void InitNewView_File( PChar filename ) {
   const PChar filenameEnd( strchr( filename, '|' ) );
   PChar pNextTokenStart;
   if( filenameEnd ) {
      *filenameEnd = '\0';
      pNextTokenStart = filenameEnd + 1;
      }
   else
      StrStartOfNext2Tokens( filename, &filename, &pNextTokenStart );

   if( s_tmpFBufCleanup.fDoIt && !FileAttribs( filename ).Exists() ) {
      // garbage-discard mode: don't create Views for nonexistent files
      if( !s_pFForgottenFiles ) {
         FBOP::FindOrAddFBuf( "<forgotten-files>", &s_pFForgottenFiles );
         s_pFForgottenFiles->PutFocusOn();
         }

      s_pFForgottenFiles->PutLastLine( filename );
      DispDoPendingRefreshes();
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      ++s_tmpFBufCleanup.droppedFiles;
      0 && DBG( "%s dropping '%s'", FUNC, filename );
      }
   else {
      auto pView( new View( FBOP::FindOrAddFBuf( filename ), g_CurWinWr(), pNextTokenStart ) );
      auto &cvwHd( g_CurViewHd() );
      DLINK_INSERT_LAST( cvwHd, pView, dlinkViewsOfWindow ); // push_back()
      0 && DBG( "%p %s %s", pView, filename, pNextTokenStart );
      }
   }

//*************************************************************************************************

STATIC_FXN void saveProgVer ( FILE *fout ) { fprintf( fout, "%s\n" , ProgramVersion() ); }
STATIC_FXN void saveTsNow ( FILE *fout ) {
   linebuf lbuf;
   auto now( time( nullptr ) );
   strftime( BSOB(lbuf), "%Y/%m/%d %H:%M:%S", localtime( &now ) );
   fprintf( fout, "state at %s\n" , lbuf );
   }

STATIC_FXN void saveCwd     ( FILE *fout ) {
   fprintf( fout, "cwd is %s\n" , Path::GetCwd().c_str() );
   }

STATIC_CONST char kszSCRN[] = "SCRN:";
STATIC_FXN bool recovScrnDims( PCChar lbuf ) {
   auto fFailed( ToBOOL( Strnicmp( lbuf, kszSCRN, KSTRLEN(kszSCRN) ) ) );
   if( !fFailed ) {
      Point newSize;
      fFailed = 2 != sscanf( lbuf+KSTRLEN(kszSCRN), "%dx%d", &newSize.col, &newSize.lin );
      if( !fFailed )
         VideoSwitchModeToXYOk( newSize );
      }
   return fFailed;
   }
STATIC_FXN void saveScrnDims( FILE *fout ) { fprintf( fout, "%s%dx%d\n", kszSCRN, EditScreenCols(), ScreenLines() ); }

STATIC_CONST char kszSRC[] = "SRC:";
STATIC_FXN bool recovSRC( PCChar lbuf ) {
   const auto fFailed( ToBOOL( Strnicmp( lbuf, kszSRC, KSTRLEN(kszSRC) ) ) );
   if( !fFailed )
      g_SnR_szSearch = lbuf+KSTRLEN(kszSRC);
   return fFailed;
   }
STATIC_FXN void saveSRC( FILE *fout ) { fprintf( fout, "%s%s\n", kszSRC, g_SnR_szSearch.c_str() ); }

STATIC_CONST char kszDST[] = "DST:";
STATIC_FXN bool recovDST( PCChar lbuf ) {
   const auto fFailed( ToBOOL( Strnicmp( lbuf, kszDST, KSTRLEN(kszDST) ) ) );
   if( !fFailed )
      g_SnR_szReplacement = lbuf+KSTRLEN(kszDST);
   return fFailed;
   }
STATIC_FXN void saveDST( FILE *fout ) { fprintf( fout, "%s%s\n", kszDST, g_SnR_szReplacement.c_str() ); }

typedef bool (*fxnLineRecov)( PCChar lbuf );
typedef void (*fxnLineSave) ( FILE *fout );
STATIC_CONST struct {
   PCChar        description;
   fxnLineRecov  lpRecov;
   fxnLineSave   lpSave;
   } stateF_lineprocessor[] =
   { // description   lpRecov        lpSave
      { nullptr     , nullptr      , saveProgVer  },
      { nullptr     , nullptr      , saveTsNow    },
      { nullptr     , nullptr      , saveCwd      },
      { "ScreenDims", recovScrnDims, saveScrnDims },
      { "SRC"       , recovSRC     , saveSRC      },
      { "DST"       , recovDST     , saveDST      },
   };

STATIC_FXN void RecoverFromStateFile( FILE *ifh ) { enum { DBG_RECOV = 0 };
   auto fSyncd( false );
   Xbuf xb( BUFBYTES+10 );  // as long as an editor line, plus room for tmpfile line prefixes (e.g. "SRCH:")

   int ix(0);
   for( ; ix < ELEMENTS(stateF_lineprocessor) ; ++ix ) {
      if( stateF_lineprocessor[ ix ].lpRecov )
         break;
      }
   if( !(ix < ELEMENTS(stateF_lineprocessor)) ) {
      DBG( "%s couldn't find first reader in stateF_lineprocessor", FUNC );
      return;
      }

   DBG_RECOV && DBG( "first reader is %s", stateF_lineprocessor[ ix ].description );

   while( 1 ) {
      if( !fgotline( &xb, ifh ) ) {
         DBG( "%s hit EOF (or rd err) while trying to sync at line %d of tmpfile", FUNC, ix+1 );
         return;
         }
      const auto buf( xb.c_str() );
      if( !stateF_lineprocessor[ ix ].lpRecov( buf ) ) {
         DBG_RECOV && DBG( "recover %s syncd on line %d", stateF_lineprocessor[ ix ].description, ix+1 );
         DBG_RECOV && DBG( "recover %s syncd on: '%s'"  , stateF_lineprocessor[ ix ].description, buf );
         break;
         }
      }

   for( ++ix ; ix < ELEMENTS(stateF_lineprocessor) ; ++ix ) {
      if( !fgotline( &xb, ifh ) ) {
         DBG( "%s hit EOF (or rd err) at line %d of tmpfile", FUNC, ix+1 );
         return;
         }
      const auto buf( xb.c_str() );
      if( stateF_lineprocessor[ ix ].lpRecov &&
          stateF_lineprocessor[ ix ].lpRecov( buf )
        ) {
         DBG( "failed to recover %s, line %d", stateF_lineprocessor[ ix ].description, ix+1 );
         DBG( "failed to recover %s: '%s'"   , stateF_lineprocessor[ ix ].description, buf );
         return;
         }
      }

   // move existing Views, later append them to tail
   // works IFF "case '>': InitNewWin" is commented out below!!!
   ViewHead tmpVHd;
   ViewHead &cvh( g_CurViewHd() );
   DLINK_MOVE_HD( tmpVHd, cvh );

   auto winCnt(0);
   while( winCnt <= 1 && fgotline( &xb, ifh ) ) {
      const auto buf( xb.wbuf() );
      DBG_RECOV && DBG( "%s %s", FUNC, buf );
      switch( buf[0] ) {
         default :                             break;  // ignore blank line
         case '.':                             break;  // ignore EoF marker
         case '>': ++winCnt;                           // New window.  Lines following contain that window's View + FBUF list.
                // InitNewWin(       buf+1 );
                                               break;
         case ' ': InitNewView_File( buf+1 );  break;  // another View + FBUF entry
         }
      }

   DLINK_JOIN( cvh, tmpVHd, dlinkViewsOfWindow );
   cvh.First()->PutFocusOn();

   if( s_pFForgottenFiles ) {
       s_pFForgottenFiles->ClearUndo();
       s_pFForgottenFiles->UnDirty();
       }
   DBG_RECOV && DBG( "%s done", FUNC );
   }

PChar RsrcFilename( PChar dest, size_t sizeofDest, PCChar ext ) {
   return safeSprintf( dest, sizeofDest, "%s%s.%s", ThisProcessInfo::ExePath(), ThisProcessInfo::ExeName(), ext );
   }

STATIC_VAR Path::str_t s_stateFileDir;

PChar StateFilename( PChar dest, size_t sizeofDest, PCChar ext ) {
   return safeSprintf( dest, sizeofDest, "%s%s.%s", s_stateFileDir.c_str(), ThisProcessInfo::ExeName(), ext );
   }

STATIC_FXN FILE *fopen_tmpfile( PCChar pModeStr ) {
   pathbuf pbuf;
   StateFilename( BSOB(pbuf), "tmp" );
   auto rv( fopen( pbuf, pModeStr ) );
   DBG( "%s -%c-> '%s'", FUNC, rv ? '-' : 'X', pbuf );
   return rv;
   }

STATIC_FXN void RecoverFromStateFile() {
   const auto ifh( fopen_tmpfile( "rt" ) );
   if( ifh ) {
      RecoverFromStateFile( ifh );
      fclose( ifh );
      }
   }

STATIC_FXN void WriteStateFile( FILE *ofh ) {
   for( const auto &sflp : stateF_lineprocessor )
      if( sflp.lpSave )
          sflp.lpSave( ofh );

   Wins_WriteStateFile( ofh );
   }

void WriteStateFile() {
   auto ofh( fopen_tmpfile( "wt" ) );
   if( ofh ) {
      WriteStateFile( ofh );
      fclose( ofh );
      }
   }

STATIC_FXN void InitFromStateFile() { enum { DD=0 };   DD && DBG( "%s+", FUNC );
   RecoverFromStateFile();
   SetWindow0();                                       DD && DBG( "%s %d windows", FUNC, g_iWindowCount() );
   if( !SwitchToNextCmdlineFile()
       && USER_INTERRUPT == ExecutionHaltRequested()
     ) {
      EditorExit( 1, false );
      }
   for( auto iw(0) ; iw < g_iWindowCount() ; ++iw ) {  DD && DBG( "%s Win[%d]", FUNC, iw );
      SetWindowSetValidView( iw );
      }                                                DD && DBG( "%s back to Win[0]", FUNC );
   SetWindowSetValidView( 0 );  // so global ops work on this window's View list
   DD && DBG( "%s-", FUNC );
   }


//*************************************************************************************************
//*************************************************************************************************
//*************************************************************************************************

void EditorExit( int processExitCode, bool fWriteStateFile ) { enum { DV=1 };
   if( processExitCode != 0 ) {                          DV && DBG("%s invoking SW_BP", __func__ );
      SW_BP;  // sw breakpoint
      }

   Msg( nullptr );

   if( fWriteStateFile ) {                               DV && DBG("%s LuaCtxt_ALL::call_EventHandler( \"EXIT\" );", __func__ );
      LuaCtxt_ALL::call_EventHandler( "EXIT" );          DV && DBG("%s WriteStateFile();", __func__ );
      WriteStateFile();
      }
                                                         DV && DBG("%s LuaClose();", __func__ );
   LuaClose();
                                                         DV && DBG("%s FreeAllMacroDefs();", __func__ );
   FreeAllMacroDefs();                                   DV && DBG("%s CmdIdxClose();", __func__ );
   CmdIdxClose();
                                                         DV && DBG("%s CloseFileExtensionSettings();", __func__ );
   CloseFileExtensionSettings();
                                                         DV && DBG("%s DestroyViewList(%d);", __func__, g_iWindowCount() );
   for( auto &win : g__.aWindow )
      DestroyViewList( &win->ViewHd );
                                                         DV && DBG("%s RemoveFBufOnly();", __func__ );
   while( auto pFb = g_FBufHead.First() )
      pFb->RemoveFBufOnly();

   // finally, garbage-collect the bits and pieces:
                                                         DV && DBG("%s ConIO_Shutdown();", __func__ );
   ConIO_Shutdown();
                                                               DBG("%s calling exit(%d);", __func__, processExitCode );
   exit( processExitCode );
   }


STATIC_FXN bool SaveAllDirtyFilesUserEscaped() {
   enum { rvUSER_ESCAPED = true, rvWILL_EXIT = false };
   BoolOneShot NeedToQueryUser;

#if FBUF_TREE
   PRbNode pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead,dlinkAllFBufs,pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->IsDirty() && pFBuf->FnmIsDiskWritable() ) {
         if( NeedToQueryUser() ) {
            auto numDirtyFiles(0);
#if FBUF_TREE
            PRbNode pNd2;
            rb_traverse( pNd2, g_FBufIdx )
#else
            DLINKC_FIRST_TO_LASTA(g_FBufHead,dlinkAllFBufs,pFBuf2)
#endif
               {
#if FBUF_TREE
               auto pFBuf2( IdxNodeToFBUF( pNd2 ) );
#endif
               if( pFBuf2->IsDirty() && pFBuf2->FnmIsDiskWritable() )
                  ++numDirtyFiles;
               }

            switch( chGetCmdPromptResponse( "yna", -1, -1, "Save ALL %d remaining changed files (Y/N)? ", numDirtyFiles ) ) {
               default:   Assert( 0 );  // chGetCmdPromptResponse bug or params out of sync
               case 'n':                         break;        // will ask user about each file in turn
               case -1:                          return rvUSER_ESCAPED;
               case 'a':  // fall thru
               case 'y':  WriteAllDirtyFBufs();  return rvWILL_EXIT;
               }
            }

         switch( chGetCmdPromptResponse( "yna", -1, -1, "%s has changed!  Save changes (Y/N/A)? ", pFBuf->Name() ) ) {
            default:   Assert( 0 );            // chGetCmdPromptResponse bug or params out of sync
            case -1:                           return rvUSER_ESCAPED;
            case 'a':  WriteAllDirtyFBufs();   return rvWILL_EXIT;
            case 'y':  pFBuf->WriteToDisk();   break; // SAVE
            case 'n':                          break; // NO SAVE
            }
         }
      }

   return rvWILL_EXIT;
   }


GLOBAL_VAR bool g_fAskExit; // global/switchval

bool ARG::exit() {
   DBG( "%s ***************************************************", FUNC );

   bool fToNextFile;
   switch( d_argType )
    {
    default:      return BadArg();

    case NULLARG: DBG( "%s NULLARG", FUNC );
                  {
                  const auto filesRemaining( NumberOfCmdlineFilesRemaining() );
                  fToNextFile =
                     (   filesRemaining
                      && !ConIO::Confirm( "You have %d more file%s to edit.  Are you sure you want to exit? "
                                 , filesRemaining
                                 , Add_s( filesRemaining )
                                 )
                     );
                  }
                  break;

    case NOARG:   DBG( "%s NOARG", FUNC );
                  fToNextFile = true;
                  break;
    }

   if( fToNextFile && SwitchToNextCmdlineFile()     ) { DBG( "%s ********* switching to another file *********", FUNC );  return false; }
   if( !KillAllBkgndProcesses()                     ) { DBG( "%s ********* !KillAllBkgndProcesses() *********" , FUNC );  return false; }
   if( g_fAskExit && !ConIO::Confirm( "Exit the Editor?" ) ) { DBG( "%s ********* user cancelled exit A *********"    , FUNC );  return false; }
   if( SaveAllDirtyFilesUserEscaped()               ) { DBG( "%s ********* user cancelled exit B *********"    , FUNC );  return false; }

   DBG( "%s exiting ===========================================================", FUNC );
   EditorExit( 0, true );
   return false; // silence compiler warning
   }

STATIC_VAR PFBUF s_pFBufRsrc;

STATIC_FXN bool OpenRsrcFileFailed() {
   if( s_pFBufRsrc ) return false;

   STATIC_VAR Path::str_t s_pszRsrcFilename;
   if( s_pszRsrcFilename.empty() ) {
      s_pszRsrcFilename = ThisProcessInfo::ExePath() + static_cast<Path::str_t>(".krsrc");
      SearchEnvDirListForFile( s_pszRsrcFilename );
      }

   0 && DBG( "%s opens Rsrc file '%s'", FUNC, s_pszRsrcFilename.c_str() );

   FBOP::FindOrAddFBuf( s_pszRsrcFilename.c_str(), &s_pFBufRsrc );
   return s_pFBufRsrc->ReadDiskFileNoCreateFailed();
   }


PChar IsolateTagStr( PChar pszText ) {
   const auto pStart( StrPastAnyWhitespace( pszText ) );
   if( *pStart != chLSQ )
      return nullptr;

   const auto pEnd( StrToNextOrEos( pStart, "]" ) );
   if( *pEnd == 0 )
      return nullptr;

   *pEnd = '\0';
   return pStart + 1;
   }


STATIC_FXN LINE FindRsrcTag( PCChar pszSectionName, PFBUF pFBuf, const LINE startLine, bool fHiLiteTag=false ) {
   const auto keyLen( Strlen( pszSectionName ) );
   0 && DBG( "FindRsrcTag: '%s' L %d", pszSectionName, keyLen );
   Xbuf xb;
   for( auto yLine(startLine) ; yLine <= pFBuf->LastLine(); ++yLine ) {
      pFBuf->getLineTabxPerRealtabs( &xb, yLine );
      const auto lbuf( xb.wbuf() );
      if( auto pszTag = IsolateTagStr( lbuf ) ) {
         while( *pszTag ) {
            const auto pTagStart( StrPastAnyWhitespace( pszTag ) );
            pszTag = StrToNextWhitespaceOrEos( pTagStart );
            const auto tagLen( pszTag - pTagStart );
            if( tagLen == keyLen && strnicmp_LenOfFirstStr( pszSectionName, pTagStart, tagLen ) == 0 ) {
               if( fHiLiteTag ) {
                  const auto pView( pFBuf->PutFocusOn() );
                  pView->SetMatchHiLite( Point(yLine,pTagStart-lbuf), tagLen, true );
                  }

               0 && DBG( "%s '%.*s'", FUNC, pd2Int(tagLen), pTagStart );
               return yLine + 1;
               }
            0 && DBG( "FindRsrcTag- '%.*s' L %" PR_PTRDIFFT "d", pd2Int(tagLen), pTagStart, tagLen );
            }
         }
      }
   return -1;
   }


class RsrcSectionWalker {
   LINE    d_lnum;
   Linebuf d_tagbuf;

   public:

   PCChar srchTag() const { return d_tagbuf; }
   RsrcSectionWalker( PCChar pszSectionName );
   bool NextSectionInstance( FBufLocn *fl, bool fHiLiteTag=false );
   PCChar SectionName() const { return d_tagbuf; }
   };

RsrcSectionWalker::RsrcSectionWalker( PCChar pszSectionName ) : d_lnum(0) {
   d_tagbuf.Strcpy( ThisProcessInfo::ExeName() );
   if( pszSectionName && *pszSectionName )
      d_tagbuf.SprintfCat( "-%s", pszSectionName );

   _strlwr( d_tagbuf );
   }

bool RsrcSectionWalker::NextSectionInstance( FBufLocn *fl, bool fHiLiteTag ) {
   if( OpenRsrcFileFailed() )
      return false;

   if( (d_lnum=FindRsrcTag( d_tagbuf, s_pFBufRsrc, d_lnum, fHiLiteTag )) < 0 )
      return false;

   fl->Set( s_pFBufRsrc, Point(d_lnum,0) );

   ++d_lnum; // next call will find a different section tag

   return true;
   }


bool ARG::ext() {
   switch( d_argType ) {
      default:
           return BadArg();

      case NOARG: { // if noarg, go to extension-section of .krsrc assoc w/curfile
           Linebuf lastTag( LastExtTagLoaded() );

           // pass 1: find out how many matching tags there are
           auto count(0);
           {
           RsrcSectionWalker rsw( lastTag );
           FBufLocn fl_dummy;
           while( rsw.NextSectionInstance( &fl_dummy ) )
              ++count;

           if( !count )
              return Msg( "no sections matching '%s'", rsw.SectionName() );
           }

           // pass 2: let the user choose the tag/section he wants
           while(1) {
              RsrcSectionWalker rsw( lastTag );
              FBufLocn fl;
              for( auto ix(1) ; rsw.NextSectionInstance( &fl, true ) ; ++ix ) {
                 fl.ScrollToOk();
                 DispDoPendingRefreshes();
                 if( count == 1 ) {
                    Msg( "only one section matching '%s'", rsw.SectionName() );
                    return true;
                    }

                 if( ConIO::Confirm( "Stop here (%d of %d)? ", ix, count ) )
                    return true;
                 }
              }
           // break; unreachable
           }
      }
   }


bool LoadRsrcSectionFound( PCChar pszSectionName, PInt pAssignCountAccumulator ) {
   RsrcSectionWalker rsw( pszSectionName );
   FmtStr<90> tag( "LoadRsrcSection [%s]", rsw.srchTag() );
   AssignLogTag( tag.k_str() );

   auto fFound(false);
   auto totalAssignsDone(0);
   FBufLocn fl;
   while( rsw.NextSectionInstance( &fl ) ) {
      fFound = true;
      int assignsDone;
      // const auto fErr =
      AssignLineRangeHadError( tag.k_str(), s_pFBufRsrc, fl.Pt().lin, -1, &assignsDone );
      totalAssignsDone += assignsDone;
      }

   if( pAssignCountAccumulator )
      *pAssignCountAccumulator += totalAssignsDone;

   0 && DBG( "%s %+d for [%s]", FUNC, fFound ? totalAssignsDone : -1, pszSectionName );

   return fFound;
   }

STIL PCChar GetVideoName() {
   return
#if   defined(_WIN32)
          "win32console"
#elif defined(X_WINDOWS)
          "x"
#else
          "curses"
#endif
          ;
   }

STIL PCChar GetOsName() { return WL( "win32", "linux" ); }

STATIC_VAR bool s_fLoadRsrcFile = true;

STATIC_FXN int ReinitializeMacros( bool fEraseExistingMacros ) {
   if( fEraseExistingMacros ) {
      UnbindMacrosFromKeys();
      FreeAllMacroDefs();
      }

   AssignStrOk( "curfilename:=" );
   AssignStrOk( "curfile:="     );
   AssignStrOk( "curfileext:="  );
   AssignStrOk( "curfilepath:=" );

   auto assignDone(0);
   if( s_fLoadRsrcFile ) {
      LoadRsrcSectionFound( nullptr       , &assignDone ); // [editorname]
      LoadRsrcSectionFound( GetOsName()   , &assignDone ); // [editorname-osname]
      LoadRsrcSectionFound( OsVerStr()    , &assignDone ); // [editorname-osver]
      LoadRsrcSectionFound( GetVideoName(), &assignDone ); // [editorname-vidname]
      }

   if( g_CurFBuf() )
      FBOP::AssignFromRsrc( g_CurFBuf() );

   DispNeedsRedrawAllLinesAllWindows();

   return assignDone;
   }

//-------------------------------------------------------------------------------------------------------

STATIC_VAR Path::str_t s_szLastFileExtSectionLoaded;

STATIC_FXN bool LdFileExtRsrcSectionFound( PCChar pszSectionName ) {
   auto fDummy(0);
   const auto fSectionExists( LoadRsrcSectionFound( pszSectionName, &fDummy ) );

   if( fSectionExists )
      s_szLastFileExtSectionLoaded = pszSectionName;

   return fSectionExists;
   }

PCChar LastExtTagLoaded() {
   return s_szLastFileExtSectionLoaded.c_str();
   }

bool LoadFileExtRsrcIniSection( PCChar pszSectionName ) { 0 && DBG( "%s [%s]", FUNC, pszSectionName );
   if( !LdFileExtRsrcSectionFound( pszSectionName ) ) {
      LdFileExtRsrcSectionFound( ".." );  // no match in .krsrc: load dflt
      return false;
      }

   0 && DBG( "---- added [%s]", pszSectionName );
   return true;
   }

//
//  Initialize
//
//      Discards all current settings, then reads the statements from the main
//      ([K]) section of .krsrc
//
//  Arg Initialize
//
//      Reads the statements from a tagged section of .krsrc. The tag
//      name is specified by the continuous string of nonblank characters
//      starting at the cursor.
//
//  Arg <textarg> Initialize
//
//      Reads the statements from the .krsrc tagged section specified by
//      <textarg>.
//
//      The section tagged with
//
//           [K-name]
//
//      is initialized by the command
//
//           Arg 'name' Initialize
//
//  TIP: To reload the main section of .krsrc without clearing other
//       settings that you want to remain in effect, label the main section
//       of .krsrc with the tag:
//
//           [k k-main]
//
//       then use Arg 'main' Initialize to recover your main settings
//       instead of using Initialize with no arguments.
//
//  Returns
//
//  True:  Initialized tagged section in .krsrc
//  False: Did not find tagged section in .krsrc
//

bool ARG::initialize() {
   if( OpenRsrcFileFailed() )
      return false;

   auto assignsDone(0);
   switch( d_argType ) {
    default:      return BadArg();

    case TEXTARG: LoadRsrcSectionFound( d_textarg.pText, &assignsDone );
                  break;

    case NOARG:   s_pFBufRsrc->ReadDiskFileNoCreateFailed(); // force reread
                  assignsDone = ReinitializeMacros( true );
                  break;
    }

   Msg( "%d assign%s done", assignsDone, Add_s( assignsDone ) );
   return assignsDone > 0;
   }

//------------------------------------------------

#if defined(_WIN32)

STATIC_FXN bool putenv_ok( PCChar szNameEqualsVal ) {
   const auto ok( _putenv( szNameEqualsVal ) == 0 );
   if( !ok )
      ErrorDialogBeepf( "%s(%s) FAILED: %s", FUNC, szNameEqualsVal, strerror( errno ) );

   return ok;
   }

#endif

bool PutEnvOk( PCChar varName, PCChar varValue ) {
   0 && DBG( "*** %s(%s=%s)", FUNC, varName, varValue );
#if defined(_WIN32)
   auto pBuf( PChar( alloca( Strlen(varName) + Strlen(varValue) + (1+1) ) ) ); // (1+1) = ('=' + '\0')
   sprintf( pBuf, "%s=%s", varName, varValue ); // sprintf is OK here since we have pre-calc'd the buf size to fit
   return putenv_ok( pBuf );
#else
   return 0 == setenv( varName, varValue, 1 );
#endif
   }

bool PutEnvOk( PCChar szNameEqualsVal ) {
   0 && DBG( "*** %s(%s)", FUNC, szNameEqualsVal );
   const auto pEQ( strchr( szNameEqualsVal, '=' ) );
   if( pEQ == szNameEqualsVal ) { // no name?
      return false; // not OK
      }

   if( !pEQ ) {
#if !defined(_WIN32)
      return 0 == unsetenv( szNameEqualsVal );
#else
      return PutEnvOk( szNameEqualsVal, "" );
#endif
      }
   else {
      ALLOCA_STRDUP( nm, nmLen, szNameEqualsVal, pEQ - szNameEqualsVal - 1 );
      return PutEnvOk( nm, pEQ + 1 );
      }
   }

STATIC_FXN bool PutEnvChkOk( PCChar szNameEqualsVal ) {
   if( strchr( szNameEqualsVal, '=' ) ) {
      return PutEnvOk( szNameEqualsVal );
      }
   else {
      DBG( "%s '%s' missing '='", FUNC, szNameEqualsVal );
      return false;
      }
   }


#ifdef fn_setenv

bool ARG::setenv() {
   switch( d_argType ) {
      default:      return BadArg();
      case TEXTARG: return PutEnvChkOk( d_textarg.pText );

      case LINEARG: //lint -fallthrough
      case BOXARG:  for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                       if( aw.GetLine() && !PutEnvChkOk( aw.c_str() ) ) {
                          return false;
                          }
                       }
                    return true;
      }
   }

#endif// fn_setenv

//------------------------------------------------

GLOBAL_CONST char kszTMPDIR[] = "$APPDATA:" PATH_SEP_STR "Kevins Editor";

STATIC_FXN void InitEnvRelatedSettings() { enum { DD=1 };  // c_str()
   PutEnvOk( "K_RUNNING?", "yes"               );
   PutEnvOk( "KINIT"     , ThisProcessInfo::ExePath() );

#if defined(_WIN32)
   #define  HOME_ENVVAR_NM  "APPDATA"
   #define  HOME_SUBDIR_NM  "Kevins Editor"
#else
   #define  HOME_ENVVAR_NM  "HOME"
   #define  HOME_SUBDIR_NM  ".kedit"
#endif
   const PCChar appdataVal( getenv( HOME_ENVVAR_NM ) );
   if( !appdataVal )          { fprintf( stderr, "%%" HOME_ENVVAR_NM "%% is not defined???\n"                      ); exit( 1 ); }
   if( !IsDir( appdataVal ) ) { fprintf( stderr, "%%" HOME_ENVVAR_NM "%% (%s) is not a directory???\n", appdataVal ); exit( 1 ); }
   #undef   HOME_ENVVAR_NM

   s_stateFileDir = appdataVal;                     DD && DBG( "1: %s", s_stateFileDir.c_str() );
   s_stateFileDir += PATH_SEP_STR HOME_SUBDIR_NM;   DD && DBG( "2: %s", s_stateFileDir.c_str() );
   if( !IsDir( s_stateFileDir.c_str() ) ) { mkdirOk( s_stateFileDir.c_str() ); }
   if( !IsDir( s_stateFileDir.c_str() ) ) { fprintf( stderr, "mkdir(%s) FAILED\n", s_stateFileDir.c_str() ); exit( 1 ); }
   s_stateFileDir += PATH_SEP_CH;              DD && DBG( "s_stateFileDir = '%s'", s_stateFileDir.c_str() );
   PutEnvOk( "K_STATEDIR", s_stateFileDir.c_str() );
   }


class kGetopt : public Getopt {
   public:

   void VErrorOut( PCChar msg );

   kGetopt( int argc_, PPCChar argv_, PCChar optset_ ) : Getopt( argc_, argv_, optset_ ) {}
   };


void kGetopt::VErrorOut( PCChar msg ) {
   printf(
"\n%s  Copyright KLG 1998-%4.4s\n%s, built %s with "
#ifdef __GNUC__
                  "GCC %d.%d.%d"
   #ifdef __MINGW32_MAJOR_VERSION
                                 ", MinGW runtime %d.%d"
   #endif
                  "\n\n"
#else
                  "MSVC\n\n"
#endif
"Usage: %s [-acdh?nv] [-e n=v] [-x mac] [-t] filename\n\n"
" -a     write assign-log pseudofile %s\n"
" -c     clean: remove non-existent files from tmpfile at init-time\n"
" -d     don't read .krsrc or the status file at startup\n"
" -h     this help screen\n"
" -?     this help srceen\n"
" -n     force New console\n"
" -v     start in no-edit mode: by dflt you cannot make changes to files\n"
" -e N=V set environment variable N=V within the editor and it's children\n"
" -x mac eXecute a function or macro at startup\n"
" -t fn  edit file fn, which will forgotten by next editor session\n"
      , ProgramVersion()
      , kszDtTmOfBuild
      , ExecutableFormat()
      , kszDtTmOfBuild
#ifdef __GNUC__
   #if !defined(__GNUC_PATCHLEVEL__)
   #define __GNUC_PATCHLEVEL__ 0
   #endif
      , __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
   #ifdef __MINGW32_MAJOR_VERSION
      , __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION // http://sourceforge.net/p/predef/wiki/Compilers/
   #endif
#else
#endif
      , d_pgm.c_str()
      , szAssignLog
      );

   if( msg )
      printf( "\n***>>> %s\n", msg );

   exit( 1 );
   }

STATIC_FXN void CreateStartupPseudofiles() { // construct special files early so they are pushed to bottom of <files> list
   STATIC_CONST struct
      {
      PCChar    name;
      PPFBUF  ppFBufVar;
      int       flags;
         #define  KEEPTRAILSPCS  BIT(0)
      } startupPseudofiles[] = {
      { "<cmdline-args>", &g_pFBufCmdlineFiles                 },
      { szCwdStk        , &g_pFBufCwd                          },
      { szSearchLog     , &g_pFBufSearchLog                    },
      { szSearchRslts   , &g_pFBufSearchRslts                  },
      { "<lua>"         , &s_pFbufLuaLog                       },
      { "<!>"           , &s_pFbufLog                          },
      { szConsole       , &g_pFBufConsole                      },
      { "<textargs>"    , &g_pFBufTextargStack , KEEPTRAILSPCS },
      { szRecord        , &g_pFbufRecord       , KEEPTRAILSPCS },
      { szClipboard     , &g_pFbufClipboard    , KEEPTRAILSPCS }, // aside: <clipboard> is filtered out of <files>
      };

   for( const auto &sPf : startupPseudofiles ) {
      const auto pFbuf( AddFBuf( sPf.name, sPf.ppFBufVar ) );
      if( sPf.flags & KEEPTRAILSPCS )  pFbuf->KeepTrailSpcs();
      }
   }

STATIC_FXN ptrdiff_t memdelta() {
   STATIC_VAR size_t lastmem;
   auto memnow( GetProcessMem() );
   auto delta( memnow - lastmem );
   lastmem = memnow;
   return delta;
   }


// NB: CANNOT use Main function's envp parameter to initialize g_envp: it does
//     not point to the same environment that _environ does (the latter is the
//     one that's modified by _putenv())
//
GLOBAL_VAR char ** &g_envp = WL( _environ, environ );

#ifdef APP_IN_DLL
DLLX void Main( int argc, const char **argv, const char **envp ) // Entrypoint from K.EXE
#else
int CDECL__ main( int argc, const char *argv[], const char *envp[] )
#endif
   {
   ThisProcessInfo::Init();

   enum { DBGFXN=1 };
   DBGFXN && DBG( "### %s @ENTRY mem =%7" PR_PTRDIFFT "d", __func__, memdelta() );

#if defined(_Win32)
   extern void TestMQ();
   TestMQ();
#endif

   0 && DBG( "Got to Main!!!" );

   // for( auto argi(0); argi < argc; ++argi ) { DBG( "argv[%d] = '%s'", argi, argv[argi] ); }

   AssignLogTag( "InitEnvRelatedSettings" );
   InitEnvRelatedSettings();
   CmdIdxInit();
   InitFileExtensionSettings();
#if FBUF_TREE
   FBufIdxInit();
#endif
   CreateStartupPseudofiles();

   auto fForceNewConsole(false);
   PCChar cmdlineMacro(nullptr);
   kGetopt opt( argc, argv, "acde:h?nt:vx:" );
   while( char ch=opt.NextOptCh() ) {  0 && DBG( "opt='%c'", ch );
     switch( ch ) {
       default:  opt.VErrorOut( FmtStr<60>( "internal error: unsupported option -%c\n", ch ) );
                 break;

       case ' ': // NON-OPTION-ARGUMENT
                 AddCmdlineFile( opt.nextarg(), false );
                 DBG( "%u files to be edited", NumberOfCmdlineFilesRemaining() );
                 break;

       case 'a': AddFBuf( szAssignLog, &g_pFBufAssignLog );
                 AssignLogTag( "editor startup" );
                 break;

       case 'c': s_tmpFBufCleanup.fDoIt = true;
                 break;

       case 'd': s_fLoadRsrcFile = false; // don't read .krsrc or the status file.
                 break;

       case 'e': PutEnvChkOk( opt.optarg() );
                 break;

       case '?':
       case 'h': opt.VErrorOut( nullptr );
                 break;

       case 'n': fForceNewConsole = true;
                 break;

       case 'v': g_fViewOnly = true;
                 break;

       case 't': AddCmdlineFile( opt.optarg(), true ); // The following filename is not retained in the tmpfile.
                 break;

       case 'x': cmdlineMacro = opt.optarg();
                 break;
       }
     }

   DBG( "%u files to be edited now", NumberOfCmdlineFilesRemaining() );
   {
                                 DBGFXN && DBG( "### %s t=0 mem+=%7" PR_PTRDIFFT "d", __func__, memdelta() );
   MainThreadPerfCounter pc;


   if( !ConIO_InitOK( fForceNewConsole ) ) { exit( 1 ); }
                                 DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT "d thru ConIO_InitOK"    , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();

   CreateWindow0();
   s_pFbufLog->PutFocusOn();

   {
   pathbuf pb;
   RsrcFilename( BSOB(pb), "luaedit" );
   if( IsFile( pb ) ) {
      AssignLogTag( FmtStr<_MAX_PATH+19>( "compiling+running %s", pb ) );
      MainThreadPerfCounter px;
      LuaCtxt_Edit::InitOk( pb );
                                 DBGFXN && DBG( "### %s t=%6.3f S mem+=%7" PR_PTRDIFFT "d LuaCtxt_Edit::InitOk %s", __func__, px.Capture(), memdelta(), pb );
      }
   }
   {
   pathbuf pb;
   RsrcFilename( BSOB(pb), "luastate" );
   if( IsFile( pb ) ) {
      AssignLogTag( FmtStr<_MAX_PATH+19>( "compiling+running %s", pb ) );
      MainThreadPerfCounter px;
      LuaCtxt_State::InitOk( pb );
                                 DBGFXN && DBG( "### %s t=%6.3f S mem+=%7" PR_PTRDIFFT "d LuaCtxt_Edit::InitOk %s", __func__, px.Capture(), memdelta(), pb );
      }
   }

   register_atexit_search();

   InitFromStateFile();          DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT "d thru ReadStateFile"     , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
   ReinitializeMacros( false );  DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT "d thru ReinitializeMacros", __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
   MsgClr(); // hack this is the earliest that it will actually have the effect of clearing the dialog line on startup (which is REALLY necessary in dialogtop mode)
   InitJobQueues();              DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT "d thru InitJobQueues"     , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
                                 DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT "d done"                   , __func__, pc.Capture(), memdelta() );
   }

   win_fully_on_desktop();

   if( CmdFromName( "autostart" ) ) {
      AssignLogTag( FmtStr<_MAX_PATH+50>( "running 'autostart' macro on FBUF \"%s\"", g_CurFBuf()->Name() ) );
      fExecuteSafe( "autostart" );
      }

   if( cmdlineMacro ) {
      AssignLogTag( FmtStr<_MAX_PATH+50+50>( "running cmdline macro '%s' on FBUF \"%s\"", cmdlineMacro, g_CurFBuf()->Name() ) );
      fExecuteSafe( cmdlineMacro );
      }

   DispDoPendingRefreshes();

   DBG( "%u files to be edited now", NumberOfCmdlineFilesRemaining() );

#if 0 && !defined(_WIN32)
   ConIO_Shutdown();
   exit(0);
#endif

   FetchAndExecuteCMDs( true );  // the mainloop: NEVER RETURNS
   }

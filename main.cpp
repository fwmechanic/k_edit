//
// Copyright 2015-2020 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
GLOBAL_VAR bool  g_fWcShowDotDir               ;
GLOBAL_VAR bool  g_fFuncRetVal                 ;
GLOBAL_VAR bool  g_fMeta                       ;
GLOBAL_VAR bool  g_fMfgrepNoise                ;
GLOBAL_VAR bool  g_fMfgrepRunning              ;
GLOBAL_VAR bool  g_fMsgflush                   ;
GLOBAL_VAR bool  g_fSelKeymapEnabled           ;
GLOBAL_VAR int   g_ClipboardType               ; // 0 == EMPTY
GLOBAL_VAR int   g_iTabWidth          =       8;
GLOBAL_VAR int   g_iHike              =      25;
GLOBAL_VAR int   g_iHscroll           =      10;
GLOBAL_VAR int   g_iRmargin           =      80;
GLOBAL_VAR int   g_iMaxUndo           =  100000;
GLOBAL_VAR int   g_iVscroll           =       1;

GLOBAL_VAR PFBUF g_pFbufClipboard          ;
GLOBAL_VAR PFBUF g_pFBufCmdlineFiles       ;
GLOBAL_VAR PFBUF s_pFbufLog                ;
GLOBAL_VAR PFBUF g_pFBufMsgLog             ;
GLOBAL_VAR PFBUF g_pFBufConsole            ;

#if defined(__GNUC__)
//=================================================================
// replace dflt abort() RTL fxn with one that lets us access gdb
EXTERNC void abort() {
   *((char *)nullptr) = 0;  // cause a "segfault" to drop us into the debugger
   while(1) {}  // dflt abort() RTL fxn is apparently prototyped with "__attribute__((noreturn))"; w/o this code I get "warning: 'noreturn' function does return [enabled by default]"
   }
//=================================================================
#endif

STATIC_FXN void AddCmdlineFile( PCChar filename, bool fForgetFile ) { 0 && DBG( "%s(%s)", FUNC, filename );
   g_pFBufCmdlineFiles->FmtLastLine( "%s%s", (fForgetFile ? "|" : ""), filename );
   0 && DBG( "%s readback(%" PR_BSR  ")", FUNC, BSR( g_pFBufCmdlineFiles->PeekRawLine( 0 ) ) );
   }

STATIC_FXN int NumberOfCmdlineFilesRemaining() {
   return g_pFBufCmdlineFiles ? g_pFBufCmdlineFiles->LineCount() : 0;
   }

STATIC_FXN bool SwitchToNextCmdlineFile() {
   std::string sb;
   while( FBOP::PopFirstLine( sb, g_pFBufCmdlineFiles ) ) {
      auto pNxtArg( sb.c_str() );  0 && DBG( "%s trying %s", FUNC, pNxtArg );
      const auto fOptT( pNxtArg[0] == '|' );
      const auto pFnm( pNxtArg + (fOptT ? 1 : 0) );
      if( fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( pFnm ) ) {
         if( fOptT ) {
            g_CurFBuf()->SetForgetOnExit();
            }
         return true;
         }                         0 && DBG( "%s failed %s", FUNC, pNxtArg );
      }
   return false;
   }

STATIC_CONST char kszThisProgVersion[] = "2.0";
STATIC_CONST char kszTmpFBufMagic[] = "FMT0";

PCChar ProgramVersion() {
   STATIC_VAR char szProgramVersion[40];
   if( !szProgramVersion[0] ) {
      safeSprintf( BSOB(szProgramVersion), "Kevin's Editor %s", kszThisProgVersion );
      }
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
   if( feof( f ) ) { return false; }
   auto ofs( 0 ); enum { bump=16 };
   PChar rvbuf( nullptr );
   for(;;) {
      const auto buf( xb->wbuf()+ofs );
      const auto fgrv( fgets( buf, xb->buf_bytes()-ofs, f ) );
      if( fgrv != buf ) {
         return rvbuf != nullptr;
         }
      if( !rvbuf ) { rvbuf = xb->wbuf(); }
      const auto len( Strlen( buf ) );
      if( len > 0 && buf[len-1] == '\n' ) {
         buf[len-1] = '\0';
         return rvbuf != nullptr;
         }
      xb->wresize( xb->buf_bytes() + bump );
      // if( bump < 2048 ) { bump <<= 1; }
      }
   }

STATIC_VAR struct {
   bool  fDoIt;
   PFBUF logfb;
   } s_ForgetAbsentFiles;  // in lieu of passing params thru the depths

STATIC_FXN void InitNewView_File( PChar viewPersistentText ) {
   // Conditionally construct a View from the contents of viewPersistentText
   // viewPersistentText
   //    points into a WRITABLE buffer containing a record written to the tmp
   //    file by View::Write() (with the single leading ' ' skipped)
   //
   // To implement s_ForgetAbsentFiles mode, we construct the View iff the named
   // file exists.  In order to perform this file-exists test, a ViewPersistent
   // instance (which will be used to construct the View if this is done) is
   // "constructed" from the viewPersistentText buffer, a process that MODIFIES
   // the viewPersistentText buffer.  See ViewPersistentInitOk...
   //
   ViewPersistent vp;
   if( !ViewPersistentInitOk( vp, viewPersistentText ) ) { return; }
   // viewPersistentText HAS BEEN MODIFIED!!!
   stref fnm( vp.filename );
   if( s_ForgetAbsentFiles.fDoIt && !FileAttribs( vp.filename ).Exists() ) {
      // garbage-discard mode: don't create Views for nonexistent files
      if( !s_ForgetAbsentFiles.logfb ) {
         FBOP::FindOrAddFBuf( "<forgotten-files>", &s_ForgetAbsentFiles.logfb );
         s_ForgetAbsentFiles.logfb->PutFocusOn();
         }
      s_ForgetAbsentFiles.logfb->PutLastLineRaw( fnm );
      DispDoPendingRefreshes();
      }
   else {
     #if !defined(_WIN32)
      if( fnm.starts_with( "/tmp/" ) && !FileAttribs( vp.filename ).Exists() ) {
         return;
         }
     #endif
      auto pView( new View( FBOP::FindOrAddFBuf( fnm ), g_CurWinWr(), vp ) );
      auto &cvwHd( g_CurViewHd() );
      DLINK_INSERT_LAST( cvwHd, pView, d_dlinkViewsOfWindow ); // push_back()
      }
   }

//*************************************************************************************************

STATIC_FXN void saveProgVer ( FILE *fout ) { fprintf( fout, "%s\n" , ProgramVersion() ); }
STATIC_FXN void saveTsNow ( FILE *fout ) {
   char lbuf[40];
   auto now( time( nullptr ) );
   struct tm tt;
   const auto fOk(
#if defined(_WIN32)
                   0       == localtime_s( &tt, &now )
#else
                   nullptr != localtime_r( &now, &tt )
#endif
                 );
   if( fOk ) { strftime( BSOB(lbuf), "%Y/%m/%d %H:%M:%S", &tt ); }
   else      { bcpy(          lbuf , "YY/mm/dd HH:MM:SS" ); }
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
      fFailed = (2 != sscanf( lbuf+KSTRLEN(kszSCRN), "%dx%d", &newSize.col, &newSize.lin ));
      if( !fFailed ) {
         VideoSwitchModeToXYOk( newSize );
         }
      }
   return fFailed;
   }
STATIC_FXN void saveScrnDims( FILE *fout ) { fprintf( fout, "%s%dx%d\n", kszSCRN, EditScreenCols(), ScreenLines() ); }

STATIC_CONST char kszSRC[] = "SRC:";
STATIC_FXN bool recovSRC( PCChar lbuf ) {
   const auto fFailed( ToBOOL( Strnicmp( lbuf, kszSRC, KSTRLEN(kszSRC) ) ) );
   if( !fFailed ) {
      g_SnR_stSearch = lbuf+KSTRLEN(kszSRC);
      }
   return fFailed;
   }
STATIC_FXN void saveSRC( FILE *fout ) { fprintf( fout, "%s%s\n", kszSRC, g_SnR_stSearch.c_str() ); }

STATIC_CONST char kszDST[] = "DST:";
STATIC_FXN bool recovDST( PCChar lbuf ) {
   const auto fFailed( ToBOOL( Strnicmp( lbuf, kszDST, KSTRLEN(kszDST) ) ) );
   if( !fFailed ) {
      g_SnR_stReplacement = lbuf+KSTRLEN(kszDST);
      }
   return fFailed;
   }
STATIC_FXN void saveDST( FILE *fout ) { fprintf( fout, "%s%s\n", kszDST, g_SnR_stReplacement.c_str() ); }

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
      if( stateF_lineprocessor[ ix ].lpRecov ) {
         break;
         }
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
   // recover Views from ifh (state file)
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
   DLINK_JOIN( cvh, tmpVHd, d_dlinkViewsOfWindow );
   cvh.front()->PutFocusOn();
   if( s_ForgetAbsentFiles.logfb ) {
       s_ForgetAbsentFiles.logfb->Undo_Reinit();
       s_ForgetAbsentFiles.logfb->UnDirty();
       Msg( "done forgetting %d files", s_ForgetAbsentFiles.logfb->LineCount() );
       }
   DBG_RECOV && DBG( "%s done", FUNC );
   }

Path::str_t RsrcFilename( PCChar ext ) {
   Path::str_t
        rv( ThisProcessInfo::ExePath() );
        rv.append( ThisProcessInfo::ExeName() );
        rv.append( "." );
        rv.append( ext );
   return rv;
   }

STATIC_VAR Path::str_t s_EditorStateDir; // init'd by InitEnvRelatedSettings()

PCChar EditorStateDir() { return s_EditorStateDir.c_str(); }

Path::str_t StateFilename( PCChar ext ) {
   auto rv( s_EditorStateDir );
        rv.append( ThisProcessInfo::ExeName() );
        rv.append( "." );
        rv.append( ext );
   return rv;
   }

STATIC_FXN FILE *fopen_tmpfile( PCChar pModeStr ) {
   auto fnm( StateFilename( "tmp" ) );
   auto rv( fopen( fnm.c_str(), pModeStr ) );
   DBG( "%s -%c-> '%s'", FUNC, rv ? '-' : 'X', fnm.c_str() );
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
   for( const auto &sflp : stateF_lineprocessor ) {
      if( sflp.lpSave ) {
          sflp.lpSave( ofh );
          }
      }
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
   SetWindow0();                                       DD && DBG( "%s %" PR_SIZET " windows", FUNC, g_WindowCount() );
   if( !SwitchToNextCmdlineFile()
       && USER_INTERRUPT == ExecutionHaltRequested()
     ) {
      EditorExit( 1, false );
      }
   for( auto iw(0) ; iw < g_WindowCount() ; ++iw ) {  DD && DBG( "%s Win[%d]", FUNC, iw );
      SetWindowSetValidView( iw );
      }                                               DD && DBG( "%s back to Win[0]", FUNC );
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
      cmdusage_updt();
      }
                                                         DV && DBG("%s LuaClose();", __func__ );
   LuaClose();
                                                         DV && DBG("%s FreeAllMacroDefs();", __func__ );
   FreeAllMacroDefs();                                   DV && DBG("%s CmdIdxClose();", __func__ );
   CmdIdxClose();
                                                         DV && DBG("%s CloseFTypeSettings();", __func__ );
   CloseFTypeSettings();
                                                         DV && DBG("%s DestroyViewList(%" PR_SIZET ");", __func__, g_WindowCount() );
   for( auto &win : g__.aWindow ) {
      DestroyViewList( &win->d_ViewHd );
      }                                                  DV && DBG("%s RemoveFBufOnly();", __func__ );
#if FBUF_TREE

#else
   while( auto pFb = g_FBufHead.front() ) {
      pFb->RemoveFBufOnly();
      }
#endif
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
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead,dlinkAllFBufs,pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->IsDirty() && pFBuf->FnmIsDiskWritable() ) {
         if( NeedToQueryUser ) {
            auto numDirtyFiles(0);
#if FBUF_TREE
            rb_traverse( pNd2, g_FBufIdx )
#else
            DLINKC_FIRST_TO_LASTA(g_FBufHead,dlinkAllFBufs,pFBuf2)
#endif
               {
#if FBUF_TREE
               auto pFBuf2( IdxNodeToFBUF( pNd2 ) );
#endif
               if( pFBuf2->IsDirty() && pFBuf2->FnmIsDiskWritable() ) {
                  ++numDirtyFiles;
                  }
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
            default:   Assert( 0 );            return rvUSER_ESCAPED;  // chGetCmdPromptResponse bug or params out of sync
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
                      && !ConIO::Confirm( Sprintf2xBuf( "You have %d more file%s to edit.  Are you sure you want to exit? "
                                 , filesRemaining
                                 , Add_s( filesRemaining )
                                 ))
                     );
                  }
                  break;
    case NOARG:   DBG( "%s NOARG", FUNC );
                  fToNextFile = true;
                  break;
    }
   if( fToNextFile && SwitchToNextCmdlineFile() ) { DBG( "%s ********* switching to another file *********", FUNC );  return false; }
   if( !KillAllBkgndProcesses()                 ) { DBG( "%s ********* !KillAllBkgndProcesses() *********" , FUNC );  return false; }
   if(    g_fAskExit
       && !ConIO::Confirm( "Exit the Editor?" ) ) { DBG( "%s ********* user cancelled exit A *********"    , FUNC );  return false; }
   if( SaveAllDirtyFilesUserEscaped()           ) { DBG( "%s ********* user cancelled exit B *********"    , FUNC );  return false; }
   DBG( "%s exiting ===========================================================", FUNC );
   EditorExit( 0, true );
   return false; // silence compiler warning
   }

//------------------------------------------------

bool mkdir_failed( PCChar dirname ) {
   const auto err( WL( _mkdir( dirname ), mkdir( dirname, 0777 ) ) == -1 );
   if( !err ) {             0 && fprintf( stderr, "created dir '%s'", dirname );
      return false;
      }
   if( EEXIST == errno ) {  0 && fprintf( stderr, "existing dir '%s'", dirname );
      return false;
      }
   return true;
   }

STATIC_FXN void InitEnvRelatedSettings() { enum { DD=1 };  // c_str()
   PutEnvOk( "K_RUNNING?", "yes" );
   PutEnvOk( "KINIT"     , ThisProcessInfo::ExePath() );
   auto mkdir_stf = [&]() {
      const auto dirname( s_EditorStateDir.c_str() );
      if( mkdir_failed( dirname ) ) {
         fprintf( stderr, "mkdir(%s) failed: %s\n", dirname, strerror( errno ) );
         exit( 1 );
         }
      s_EditorStateDir.append( DIRSEP_STR );  0 && DD && DBG( "%s", s_EditorStateDir.c_str() );
      };
   #define  HOME_SUBDIR_NM  "k_edit"
#if defined(_WIN32)
   #define  HOME_ENVVAR_NM  "APPDATA"
   const PCChar appdataVal( getenv( HOME_ENVVAR_NM ) );
   if( !(appdataVal && appdataVal[0]) )  { fprintf( stderr, "%%" HOME_ENVVAR_NM "%% is not defined???\n"                      ); exit( 1 ); }
   if( !IsDir( appdataVal ) )            { fprintf( stderr, "%%" HOME_ENVVAR_NM "%% (%s) is not a directory???\n", appdataVal ); exit( 1 ); }
   s_EditorStateDir.assign( appdataVal );                    0 && DD && DBG( "1: %s", s_EditorStateDir.c_str() );
   s_EditorStateDir.append( DIRSEP_STR HOME_SUBDIR_NM );     0 && DD && DBG( "2: %s", s_EditorStateDir.c_str() );
   #undef   HOME_ENVVAR_NM
#else
   //
   // ifdef XDG_CACHE_HOME
   //    "$XDG_CACHE_HOME/k_edit"
   // else
   //    "$HOME/.cache/k_edit"
   // endif
   //
   #define  CACHE_ENVVAR_NM       "XDG_CACHE_HOME"
   #define  HOME_ENVVAR_NM        "HOME"
   #define  HOME_ENVVAR_SUFFIXNM  ".cache"
   PCChar baseVarVal( getenv( CACHE_ENVVAR_NM ) );
   if( baseVarVal && baseVarVal[0] ) {
      s_EditorStateDir.assign( baseVarVal );
      mkdir_stf();  // fprintf( stderr, "$" CACHE_ENVVAR_NM " %s\n", s_EditorStateDir.c_str() );
      }
   else {
      baseVarVal = getenv( HOME_ENVVAR_NM );
      if( !(baseVarVal && baseVarVal[0]) ) { fprintf( stderr, "$" HOME_ENVVAR_NM " is not defined???\n" ); exit( 1 ); }
      // fprintf( stderr, "$" HOME_ENVVAR_NM " %s\n", baseVarVal );
      if( !IsDir( baseVarVal ) )           { fprintf( stderr, "dir $" HOME_ENVVAR_NM " %s does not exist???\n", baseVarVal ); exit( 1 ); }
      s_EditorStateDir.assign( baseVarVal );    //       fprintf( stderr, "$" HOME_ENVVAR_NM " %s\n", s_EditorStateDir.c_str() );
      s_EditorStateDir.append( DIRSEP_STR HOME_ENVVAR_SUFFIXNM ); // fprintf( stderr, "$" HOME_ENVVAR_NM "/" HOME_ENVVAR_SUFFIXNM " %s\n", s_EditorStateDir.c_str() );
      mkdir_stf();
      }
   s_EditorStateDir.append( HOME_SUBDIR_NM );   0 && DD && DBG( "2: %s", s_EditorStateDir.c_str() );
   #undef   HOME_ENVVAR_NM
#endif
   #undef   HOME_SUBDIR_NM
   mkdir_stf();
  #if !defined(_WIN32)
   { // in case homedir is an NFS mount: add a level of indirection (hostname) to store editor state per-host
   const auto hostname( ThisProcessInfo::hostname() );
   if( hostname[0] ) {
      s_EditorStateDir.append( hostname );      0 && DD && DBG( "3: %s", s_EditorStateDir.c_str() );
      mkdir_stf();
      // since I am often sudo'd, and since the root-associated edit-fileset often can't be accessed (and root-written state files
      // can't be rewritten) by non-root, track them separately by adding a username level of indirection
      const auto euname( ThisProcessInfo::euname() );
      if( euname[0] ) {
         s_EditorStateDir.append( euname );
         mkdir_stf();
         }
      }
   }
  #endif
   PutEnvOk( "K_STATEDIR", s_EditorStateDir.c_str() );
   }

#if !NO_LOG

STATIC_VAR FILE *ofh_DBG;
STATIC_VAR time_t log_t0;

constexpr auto LOG_STRFTIME = true;
#define        LOG_STRFTIME_FMT  "%y%m%dT%H%M%S"

void DBG_init() {
   if( !ofh_DBG ) {
      log_t0 = time( nullptr );
      struct tm tt;
      const auto fOk(
#if defined(_WIN32)
                      0       == localtime_s( &tt, &log_t0 )
#else
                      nullptr != localtime_r( &log_t0, &tt )
#endif
                    );
      if( !fOk ) {
         perror("localtime_s");
         exit(EXIT_FAILURE);
         }
      char tmstr[100];
      if( strftime( BSOB(tmstr), LOG_STRFTIME_FMT, &tt ) == 0 ) {
         perror( "strftime returned 0" );
         exit(EXIT_FAILURE);
         }
      pathbuf fnmbuf;
      snprintf( BSOB( fnmbuf ), "%s-%s.log", ThisProcessInfo::ExeName(), tmstr );
      Path::str_t
      logfnm( EditorStateDir() );
      logfnm.append( "log"      );
      if( mkdir_failed( logfnm.c_str() ) ) {
         fprintf( stderr, "mkdir(%s) failed: %s\n", logfnm.c_str(), strerror( errno ) );
         exit( 1 );
         }
      logfnm.append( DIRSEP_STR );
      PutEnvOk( "K_LOGDIR", logfnm.c_str() );
      logfnm.append( fnmbuf     );
      ofh_DBG = fopen( logfnm.c_str(), "w" );
      if( ofh_DBG == nullptr ) {
         fprintf( stderr, "DBG_init() fopen(%s): %s", logfnm.c_str(), strerror(errno) );
         exit(EXIT_FAILURE);
         }
      if( LOG_STRFTIME ) {
         fprintf( ofh_DBG, "%s logging to %s\n", tmstr, logfnm.c_str() );
         }
      else {
         fprintf( ofh_DBG, "%" PR_TIMET " %s logging to %s\n", log_t0, tmstr, logfnm.c_str() );
         }
      PutEnvOk( "K_LOGFNM", logfnm.c_str() );
   // fprintf( stderr , "logging to %s\n", logfnm.c_str() );
      }
   }

GLOBAL_VAR bool g_fLogFlush = false;
STATIC_VAR bool fDBGNL_last;
STATIC_FXN void wr_nl_flush( FILE *ofh ) {
   fputc( '\n', ofh );
   if( g_fLogFlush ) {
      fflush( ofh );
      }
   }

void DBGNL() {
   if( fDBGNL_last ) { return; }
   fDBGNL_last = true;
   auto ofh( ofh_DBG ? ofh_DBG : stdout );
   wr_nl_flush( ofh );
   }

#if !defined(_WIN32)

static void strftime_us( char *buf, size_t sizeof_buf ) {  // based on https://stackoverflow.com/a/2409054
   struct timeval tv;
   time_t nowtime;
   gettimeofday( &tv, nullptr );
   nowtime = tv.tv_sec;
   struct tm tt;
   const auto fOk( nullptr != localtime_r( &nowtime, &tt ) );
   if( fOk ) {
      const size_t sftLen( strftime( buf, sizeof_buf, LOG_STRFTIME_FMT, &tt ) );
      snprintf( buf+sftLen, sizeof_buf-sftLen, ".%06ld" " ", tv.tv_usec );
      }
   else {
      const auto sftLen( scpy( buf, sizeof_buf, LOG_STRFTIME_FMT ) );
      snprintf( buf+sftLen, sizeof_buf-sftLen, ".%06ld" " ", 0L );
      }
   }

#endif

int DBG( char const *kszFormat, ...  ) {
   auto ofh( ofh_DBG ? ofh_DBG : stdout );
   if( LOG_STRFTIME ) {
      char tmstr[100];
#if defined(_WIN32)
      const auto tnow( time( nullptr ) );
      struct tm tt;
      const auto fOk( 0 == localtime_s( &tt, &tnow ) );
      if( fOk ) { strftime( BSOB(tmstr), LOG_STRFTIME_FMT " ", &tt ); }
      else      { bcpy(          tmstr , LOG_STRFTIME_FMT " " ); }
#else
      strftime_us( BSOB(tmstr) );
#endif
      fputs( tmstr, ofh );
      }
   else {
      const auto tnow( time( nullptr ) );
      fprintf( ofh, "%" PR_TIMET " ", (tnow - log_t0) );
      }
   va_list args;  va_start(args, kszFormat);
   const auto chWr( vfprintf( ofh, kszFormat, args ) );
   va_end(args);
   if( chWr ) {
      wr_nl_flush( ofh );
      fDBGNL_last = false;
      }
   return 1; // so we can use short-circuit bools like (DBG_X && DBG( "got here" ))
   }

#endif

class kGetopt : public Getopt {
public:
   void VErrorOut( PCChar msg );
   kGetopt( int argc_, PPCChar argv_, PCChar optset_ ) : Getopt( argc_, argv_, optset_ ) {}
   };

void kGetopt::VErrorOut( PCChar msg ) {
   printf(
"\n%s  Copyright KLG 2015-%4.4s\n%s, built %s with "
#if defined(__GNUC__)
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
" -h     this help text\n"
" -?     this help text\n"
" -n     force New console\n"
" -v     start in no-edit mode: by dflt you cannot make changes to files\n"
" -e N=V set environment variable N=V within the editor and it's children\n"
" -x mac eXecute a function or macro at startup\n"
" -t fn  edit file fn, which will forgotten by next editor session\n"
      , ProgramVersion()
      , kszDtTmOfBuild
      , ExecutableFormat()
      , kszDtTmOfBuild
#if defined(__GNUC__)
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
      , kszAssignLog
      );
   if( msg ) {
      printf( "\n***>>> %s\n", msg );
      }
   exit( 1 );
   }

STATIC_FXN void CreateStartupPseudofiles() { // construct special files early so they are pushed to bottom of <files> list
   STATIC_CONST struct
      {
      PCChar  name;
      PFBUF  &pFBufVar;
      bool    fKeepTrailSpcs;
      } startupPseudofiles[] = {              // fKeepTrailSpcs
      { "<cmdline-args>", g_pFBufCmdlineFiles        },
      { "<msg>"         , g_pFBufMsgLog              },
      { kszCwdStk       , g_pFBufCwd                 },
      { kszSearchLog    , g_pFBufSearchLog           },
      { kszSearchRslts  , g_pFBufSearchRslts         },
      { "<lua>"         , s_pFbufLuaLog              },
      { "<!>"           , s_pFbufLog                 },
      { kszConsole      , g_pFBufConsole             },
      { "<textargs>"    , g_pFBufTextargStack , true },
      { kszRecord       , g_pFbufRecord       , true },
      { kszClipboard    , g_pFbufClipboard    , true }, // aside: <clipboard> is filtered out of <files>
      };
   for( const auto &sPf : startupPseudofiles ) {
      const auto pFbuf( AddFBuf( sPf.name, &sPf.pFBufVar ) );
      if( sPf.fKeepTrailSpcs ) { pFbuf->KeepTrailSpcs(); }
      }
   }

STATIC_FXN ptrdiff_t memdelta() {
   STATIC_VAR size_t lastmem;
   auto memnow( GetProcessMem() );
   auto delta( memnow - lastmem );
   lastmem = memnow;
   return delta;
   }

GLOBAL_VAR bool g_CLI_fUseRsrcFile = true;

STATIC_FXN void ConstructStatics() {
   ThisProcessInfo::Init();
   AssignLogTag( "InitEnvRelatedSettings" );
   InitEnvRelatedSettings();
   DBG_init();
#if defined(_Win32)
   extern void TestMQ();
   TestMQ();
#endif
   }


// NB: CANNOT use Main function's envp parameter to initialize g_envp: it does
//     not point to the same environment that _environ does (the latter is the
//     one that's modified by _putenv())
//
GLOBAL_VAR char ** &g_envp = WL( _environ, environ );

#ifndef APP_IN_DLL
int CDECL__ main( int argc, const char *argv[], const char *envp[] )
#else
DLLX void Main( int argc, const char **argv, const char **envp ) // Entrypoint from K.EXE
#endif
   {
   // extern void test_CaptiveIdxOfCol();
   //             test_CaptiveIdxOfCol();
   ConstructStatics();
   enum { DBGFXN=1 };
   DBGFXN && DBG( "### %s @ENTRY mem =%7" PR_PTRDIFFT, __func__, memdelta() );
   // for( auto argi(0); argi < argc; ++argi ) { DBG( "argv[%d] = '%s'", argi, argv[argi] ); }
   CmdIdxInit();
   SwitblInit();
   InitFTypeSettings();
#if FBUF_TREE
   FBufIdxInit();
#endif
   CreateStartupPseudofiles();
   DBGNL();
   auto fForceNewConsole(false);
   PCChar cmdlineMacro(nullptr);
   kGetopt opt( argc, argv, "acde:h?nt:vx:" );
   while( char ch=opt.NextOptCh() ) {  0 && DBG( "opt='%c'", ch );
     switch( ch ) {
       default:  opt.VErrorOut( FmtStr<60>( "internal error: unsupported option -%c\n", ch ) );
                 break;
       case ' ': // NON-OPTION-ARGUMENT
                 AddCmdlineFile( opt.nextarg(), false );
                 break;
       case 'a': AddFBuf( kszAssignLog, &g_pFBufAssignLog );
                 AssignLogTag( "editor startup" );
                 break;
       case 'c': s_ForgetAbsentFiles.fDoIt = true;
                 break;
       case 'd': g_CLI_fUseRsrcFile = false; // don't read .krsrc or the status file.
                 break;
       case 'e': PutEnvChkOk( opt.optarg() );
                 break;
       case '?': ATTR_FALLTHRU;
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
   {                             DBGFXN && DBG( "### %s t=0 mem+=%7" PR_PTRDIFFT, __func__, memdelta() );
   MainThreadPerfCounter pc;
   if( !ConIO_InitOK( fForceNewConsole ) ) { exit( 1 ); }
                                 DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT " thru ConIO_InitOK"    , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
   CreateWindow0();
   s_pFbufLog->PutFocusOn();
   {
   auto pb( RsrcFilename( "luaedit" ) );
   if( IsFile( pb.c_str() ) ) {
      AssignLogTag( FmtStr<_MAX_PATH+19>( "compiling+running %s", pb.c_str() ) );
      MainThreadPerfCounter px;
      LuaCtxt_Edit::InitOk( pb.c_str() );
                                 DBGFXN && DBG( "### %s t=%6.3f S mem+=%7" PR_PTRDIFFT " LuaCtxt_Edit::InitOk %s", __func__, px.Capture(), memdelta(), pb.c_str() );
      }
   }
   {
   auto pb( RsrcFilename( "luastate" ) );
   if( IsFile( pb.c_str() ) ) {
      AssignLogTag( FmtStr<_MAX_PATH+19>( "compiling+running %s", pb.c_str() ) );
      MainThreadPerfCounter px;
      LuaCtxt_State::InitOk( pb.c_str() );
                                 DBGFXN && DBG( "### %s t=%6.3f S mem+=%7" PR_PTRDIFFT " LuaCtxt_Edit::InitOk %s", __func__, px.Capture(), memdelta(), pb.c_str() );
      }
   }
   register_atexit_search();
   ReinitializeMacros( false );  DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT " thru ReinitializeMacros", __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
   InitFromStateFile();          DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT " thru ReadStateFile"     , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
   // MsgClr(); // hack this is the earliest that it will actually have the effect of clearing the dialog line on startup (which is REALLY necessary in dialogtop mode)
   InitJobQueues();              DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT " thru InitJobQueues"     , __func__, pc.Capture(), memdelta() );  CleanupAnyExecutionHaltRequest();
                                 DBGFXN && DBG( "### %s t=%6.3f mem+=%7" PR_PTRDIFFT " done"                   , __func__, pc.Capture(), memdelta() );
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
#if 0 && !defined(_WIN32)
   ConIO_Shutdown();
   exit(0);
#endif
   FetchAndExecuteCMDs( true );  // the mainloop: NEVER RETURNS
   }

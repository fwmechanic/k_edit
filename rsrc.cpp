//
// Copyright 2015-2017 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

STATIC_VAR PFBUF s_pFBufRsrc;

STATIC_FXN bool OpenRsrcFileFailed() {
   if( s_pFBufRsrc ) { return false; }
   STATIC_VAR Path::str_t s_pszRsrcFilename;
   if( s_pszRsrcFilename.empty() ) {
      s_pszRsrcFilename = ThisProcessInfo::ExePath() + static_cast<Path::str_t>(".krsrc");
      SearchEnvDirListForFile( s_pszRsrcFilename );
      }                                                          0 && DBG( "%s opens Rsrc file '%s'", FUNC, s_pszRsrcFilename.c_str() );
   FBOP::FindOrAddFBuf( s_pszRsrcFilename, &s_pFBufRsrc );
   return s_pFBufRsrc->ReadDiskFileNoCreateFailed();
   }

STATIC_FXN stref IsolateTagStr( stref src ) {
   const auto ixLSQ( src.find_first_not_of( SPCTAB ) );
   if( stref::npos==ixLSQ || src[ixLSQ] != chLSQ ) {
      return stref();
      }
   src.remove_prefix( ixLSQ+1 );
   const auto ixRSQ( src.find( chRSQ ) );
   if( stref::npos==ixRSQ ) {
      return stref();
      }
   return src.substr( 0, ixRSQ );
   }

STATIC_FXN LINE FindRsrcTag( stref srKey, PFBUF pFBuf, const LINE startLine, bool fHiLiteTag=false ) { enum { DB=0 };
                                                                 DB && DBG( "FindRsrcTag: '%" PR_BSR "'", BSR(srKey) );
   for( auto yLine(startLine) ; yLine <= pFBuf->LastLine(); ++yLine ) {
      const auto rl( pFBuf->PeekRawLine( yLine ) );
      const stref tag( IsolateTagStr( rl ) );
      if( !tag.empty() ) {                                       DB && DBG( "tag---------------------------=%" PR_BSR "|", BSR(tag) );
         for( sridx ix( 0 ); ix < tag.length() ; ) {
            const auto ix0( FirstNonBlankOrEnd( tag, ix  ) );
            const auto ix1( FirstBlankOrEnd   ( tag, ix0 ) );
            const auto taglen( ix1 - ix0 );
            const auto atag( tag.substr( ix0, taglen ) );        DB && DBG( "%s ? '%" PR_BSR "'", FUNC, BSR(atag) );
            if( 0==cmpi( atag, srKey ) ) {
               if( fHiLiteTag ) {
                  const auto pView( pFBuf->PutFocusOn() );
                  const auto iox0( isri2osri( rl, tag, ix0 ) );  DB && DBG( "%s! tagging y=%d x=%" PR_SIZET " L %" PR_SIZET "u", __func__, yLine, iox0, taglen );
                  pView->SetMatchHiLite( Point(yLine,iox0), taglen, true );
                  }
               const auto rv( yLine + 1 );                       DB && DBG( "%s! %d * '%" PR_BSR "'", __func__, rv, BSR(atag) );
               return rv;
               }
            ix = ix1;
            }
         }
      }
   return -1;
   }

class RsrcSectionWalker {
   LINE    d_lnum;
   stref   d_tagbuf;
public:
   RsrcSectionWalker( stref pszSectionName ) : d_lnum(0), d_tagbuf(pszSectionName) {}
   bool NextSectionInstance( FBufLocn *fl, bool fHiLiteTag=false );
   stref SectionName() const { return d_tagbuf; }
   };

bool RsrcSectionWalker::NextSectionInstance( FBufLocn *fl, bool fHiLiteTag ) {
   if( OpenRsrcFileFailed() ) {
      return false;
      }
   if( (d_lnum=FindRsrcTag( d_tagbuf, s_pFBufRsrc, d_lnum, fHiLiteTag )) < 0 ) {
      return false;
      }
   fl->Set( s_pFBufRsrc, Point(d_lnum,0) );
   ++d_lnum; // next call will find a different section tag
   return true;
   }

bool ARG::ext() {
   switch( d_argType ) {
      default:
           return BadArg();
      case NOARG: { // if noarg, go to extension- or ftype- section of .krsrc assoc w/curfile
           const auto lastTag( LastRsrcFileLdSectionFtypeSectionNm() );
           // pass 1: find out how many matching tags there are
           auto count(0);
           {
           RsrcSectionWalker rsw( lastTag );
           FBufLocn fl_dummy;
           while( rsw.NextSectionInstance( &fl_dummy ) ) {
              ++count;
              }
           if( !count ) {
              return Msg( "no sections matching '%" PR_BSR "'", BSR(rsw.SectionName()) );
              }
           }
           // pass 2: let the user choose the tag/section he wants
           while(1) {
              RsrcSectionWalker rsw( lastTag );
              FBufLocn fl;
              for( auto ix(1) ; rsw.NextSectionInstance( &fl, true ) ; ++ix ) {
                 fl.ScrollToOk();
                 DispDoPendingRefreshes();
                 if( count == 1 ) {
                    Msg( "only one section matching '%" PR_BSR "'", BSR(rsw.SectionName()) );
                    return true;
                    }
                 if( ConIO::Confirm( FmtStr<50>( "Stop here (%d of %d)? ", ix, count ) ) ) {
                    return true;
                    }
                 }
              }
           // break; unreachable
           }
      }
   }

bool RsrcFileLdAllNamedSections( stref srSectionName, int *pAssignCountAccumulator ) {
   if( srSectionName.empty() ) { return false; }
   RsrcSectionWalker rsw( srSectionName );
   FmtStr<90> tag( "LoadRsrcSection [%" PR_BSR "]", BSR(rsw.SectionName()) );
   AssignLogTag( tag.k_str() );
   auto fFound(false);
   auto totalAssignsDone(0);
   FBufLocn fl;
   while( rsw.NextSectionInstance( &fl ) ) {
      fFound = true;
      int assignsDone;
      // const auto fErr =
      RsrcFileLineRangeAssignFailed( tag.k_str(), s_pFBufRsrc, fl.Pt().lin, -1, &assignsDone );
      totalAssignsDone += assignsDone;
      }
   if( pAssignCountAccumulator ) {
      *pAssignCountAccumulator += totalAssignsDone;
      }                                     0 && DBG( "%s %+d for [%" PR_BSR "]", FUNC, fFound ? totalAssignsDone : -1, BSR(srSectionName) );
   return fFound;
   }

STIL PCChar GetDisplayName() {
   return
#if   defined(_WIN32)
          "display.win32console"
#elif defined(X_WINDOWS)
          "display.x"
#else
          "display.curses"
#endif
          ;
   }

int ReinitializeMacros( bool fEraseExistingMacros ) {
   if( fEraseExistingMacros ) {
      UnbindMacrosFromKeys();
      FreeAllMacroDefs();
      }
   DefineMacro( "curfilename", "" );
   DefineMacro( "curfile"    , "" );
   DefineMacro( "curfileext" , "" );
   DefineMacro( "curfilepath", "" );
   auto assignDone(0);
   if( g_CLI_fUseRsrcFile ) {
      RsrcFileLdAllNamedSections( "@startup"      , &assignDone ); // [@startup]
      std::string key( "os." );
                  key.append( OsName() );
      RsrcFileLdAllNamedSections( key   , &assignDone ); // [os.osname]
     #if 0
      const stref verval( OsVerStr() );
      if( !verval.empty() ) {
                  key.append( "." );
                  key.append( verval );
         RsrcFileLdAllNamedSections( key          , &assignDone ); // [os.osname.osver]
         }
      RsrcFileLdAllNamedSections( GetDisplayName(), &assignDone ); // [vidname]
     #endif
      }
   if( g_CurFBuf() ) {
      FBOP::CurFBuf_AssignMacros_RsrcLd();
      }
   DispNeedsRedrawAllLinesAllWindows();
   return assignDone;
   }

//-------------------------------------------------------------------------------------------------------

bool RsrcFileLdAllNamedSections( stref pszSectionName ) {
   auto fDummy(0);
   return RsrcFileLdAllNamedSections( pszSectionName, &fDummy );
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
   if( OpenRsrcFileFailed() ) {
      return false;
      }
   auto assignsDone(0);
   switch( d_argType ) {
    default:      return BadArg();
    case TEXTARG: RsrcFileLdAllNamedSections( d_textarg.pText, &assignsDone );
                  break;
    case NOARG:   s_pFBufRsrc->ReadDiskFileNoCreateFailed(); // force reread
                  assignsDone = ReinitializeMacros( true );
                  break;
    }
   Msg( "%d assign%s done", assignsDone, Add_s( assignsDone ) );
   return assignsDone > 0;
   }

bool RsrcFileLineRangeAssignFailed( PCChar title, PFBUF pFBuf, LINE yStart, LINE yEnd, int *pAssignsDone, Point *pErrorPt ) { enum {DBGEN=0};
                                                              DBGEN && DBG( "%s: {%s} L [%d..%d]", __func__, title, yStart, yEnd );
   if( yEnd < 0 || yEnd > pFBuf->LastLine() ) {
      yEnd = pFBuf->LastLine();
      }
   enum AL2MSS { HAVE_CONTENT, FOUND_TAG, BLANK_LINE, };
   std::string srcAccum;
   auto yCur( yStart );
   // reads lines until an assignable entity is complete (IOW, an EoL w/o continuation char is reached)
   auto AccumAssignableEntity = [&]() -> AL2MSS  {
      for( ; yCur <= yEnd ; ++yCur ) {
         const auto rl( pFBuf->PeekRawLine( yCur ) );
         if( !IsolateTagStr( rl ).empty() ) {                 DBGEN && DBG( "L %d TAG VIOLATION", yCur );
            return FOUND_TAG;
            }
         auto continues( false );
         const auto parsed( ExtractAssignableText( rl, continues ) );
         srcAccum.append( sr2st( parsed ) );                  DBGEN && DBG( "%c+> %" PR_BSR "|", continues?'C':'c', BSR(srcAccum) );
         if( !continues && !srcAccum.empty() ) {              DBGEN && DBG( "RTN HvContent" );
            return HAVE_CONTENT; // we got SOME text in the buffer, and the parser says there is no continuation to the next line
            }
         }                                                    DBGEN && DBG( "RTN ?" ); //
      return srcAccum.empty() ? BLANK_LINE : HAVE_CONTENT;
      };
   auto fContinueScan( true ); auto fAssignError( false ); auto assignsDone( 0 );
   for( ; fContinueScan && yCur <= yEnd ; ++yCur ) {          DBGEN && DBG( "%s L %d", __func__, yCur );
      const auto rslt( AccumAssignableEntity() );             DBGEN && DBG( "%s L %d rslt=%d", __func__, yCur, rslt );
      switch( rslt ) {
       case HAVE_CONTENT :                                    DBGEN && DBG( "assigning --- |%s|", srcAccum.c_str() );
                            if( !AssignStrOk( srcAccum ) ) {  DBGEN && DBG( "%s atom failed '%s'", __func__, srcAccum.c_str() );
                               if( pErrorPt ) {
                                  pErrorPt->Set( yCur, 0 );
                                  }
                               fAssignError = true;
                               }
                            else {                            DBGEN && DBG( "%s atom OKOKOK '%s'", __func__, srcAccum.c_str() );
                               ++assignsDone;
                               }
                            srcAccum.clear();
                            break;
       case BLANK_LINE   :  /* keep going */        break;
       case FOUND_TAG    :  fContinueScan = false;  break;
       default           :  fContinueScan = false;  break;
       }
      }                                                       DBGEN && DBG( "%s: {%s} L [%d..%d/%d] = %d", __func__, title, yStart, yCur, yEnd, assignsDone );
   if( pAssignsDone ) { *pAssignsDone = assignsDone; }
   return fAssignError;
   }

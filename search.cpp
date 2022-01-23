//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
#include "fname_gen.h"

// !0 == VERBOSE_SEARCH logging
#if 0
#  define  VS_( x ) x
#else
#  define  VS_( x )
#endif

//
// NEW FASTER GREP SEARCHER!!!
//
// 20-Jun-2003 klg this is MASSIVELY faster than the old implementation!!!
//
// The improvement is that we search the line data directly, NOT taking the
// time to copy the line data into an automatic/stack buffer.
//
// The BEHAVIORAL DIFFERENCE is that whitespace in search strings will only
// match whatever is literally there in the file (because internal way copies
// line-string and expands tabs, etc., and we achieve a MASSIVE performance
// benefit by scanning the original string DIRECTLY).  In 99.9% of cases, this
// difference is totally irrelevant...
//
// MORE work was involved in getting the case-INsensitive search to within
// 5-10% of the case sensitive search.
//
// Last remaining optimization that I can think of is to flatten the code
// (remove subrtne calls) by making fewer bigger functions...
//

typedef sridx (* pFxn_strstr)( stref haystack, stref needle );
                                                                // HAYSTACK
STATIC_FXN sridx strnstr( stref haystack, stref needle ) {
   return haystack.find( needle );
   }

STATIC_FXN sridx strnstri( stref haystack, stref needle ) {
   const auto pos( std::search( haystack.cbegin(), haystack.cend(), needle.cbegin(), needle.cend(), eqi_ ) );
   return pos == haystack.cend() ? stref::npos : std::distance( haystack.cbegin(), pos );
   }


FBufLocnNow::FBufLocnNow() : FBufLocn( g_CurFBuf(), g_Cursor() ) {}

bool FBufLocn::Moved() const {
   return !(InCurFBuf() && g_Cursor() == d_pt);
   }

bool FBufLocn::ScrollToOk() const {
   if( !IsSet() ) {
      return false;
      }
   d_pFBuf->PutFocusOn();
   g_CurView()->MoveCursor( d_pt.lin, d_pt.col, d_width+1 );
   DispNeedsRedrawAllLinesAllWindows();
   return true;
   }

// this may be a mistake!  English language is unorthogonal in the extreme!
PCChar Add_es( int count ) { return 1 == count ?  "" : "es"; }
PCChar Add_s(  int count ) { return 1 == count ?  "" : "s" ; }

//*****************************************************************************************************

struct SearchScanMode {
   bool d_fSearchForward;
   };

STATIC_CONST SearchScanMode smFwd    = { true  };
STATIC_CONST SearchScanMode smBackwd = { false };

class FileSearchMatchHandler {
   class SearchStats {
      int d_matchCount;  // FOR both SEARCH AND REPLACE, any match is a match.
      int d_actionCount; // FOR SEARCH, an action is a match; FOR REPLACE, it's a replace actually being performed
   public:
      SearchStats() { Reset(); }
      void Reset() {
         d_matchCount  = 0;
         d_actionCount = 0;
         }
      void IncMatch()       { ++d_matchCount ; }
      void IncMatchAction() { ++d_actionCount; }
      int  GetMatches()      const { return d_matchCount ; }
      int  GetMatchActions() const { return d_actionCount; }
      };
protected:
   int         d_lifetimeFileCount;
   int         d_lifetimeFileCountMatch      ;
   int         d_lifetimeFileCountMatchAction;
   SearchStats d_lifetimeStats;
   SearchStats d_curFileStats;
   FBufLocn    d_flToScroll;
   Point       d_scrollPoint;
   MainThreadPerfCounter d_pc;
public:
   FileSearchMatchHandler()
      : d_lifetimeFileCount           (0)
      , d_lifetimeFileCountMatch      (0)
      , d_lifetimeFileCountMatchAction(0)
      { g_CurView()->FreeHiLiteRects(); }
   virtual ~FileSearchMatchHandler() {}
   // External Event Hooks
   virtual void VEnteringFile( PFBUF pFBuf ) { // could add a file skip retval here
                ++d_lifetimeFileCount;
                if( d_curFileStats.GetMatches()      ) { ++d_lifetimeFileCountMatch;       }
                if( d_curFileStats.GetMatchActions() ) { ++d_lifetimeFileCountMatchAction; }
                d_curFileStats.Reset();
                }
           bool FoundMatchContinueSearching( PFBUF pFBuf, const Point &cur, COL MatchCols, RegexMatchCaptures &captures );
           bool VCanForgetCurFile() {
                0 && DBG( "%5d all %5d:%5d cur %5d:%5d"
                        , d_lifetimeFileCount
                        , d_lifetimeStats.GetMatches(), d_lifetimeStats.GetMatchActions()
                        , d_curFileStats .GetMatches(), d_curFileStats .GetMatchActions()
                        );
                return d_curFileStats.GetMatchActions() == 0;
                }
   virtual void VLeavingFile( PFBUF pFBuf ) {}           // could merge this with VCanForgetCurFile() ?
           void ShowResults() {
                ScrollToFLOk();
             // if( !Interpreter::Interpreting() )  // user who invokes this fxn w/in a macro still wants to see the stats
                   VShowResultsNoMacs();
                }
   virtual bool VOverallRetval() { return d_lifetimeStats.GetMatchActions() > 0; } // retval for ARG:ffXxxx
protected:
   // SUBCLASS Event Hooks
      // called by FoundMatchContinueSearching
      virtual bool VMatchWithinColumnBounds( PFBUF pFBuf, const Point &cur, COL MatchCols ) { return true; };
      virtual bool VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) = 0;
      virtual bool VContinueSearching() { return true; }
      // called by ShowResults
      virtual void VShowResultsNoMacs() {}
   // common tools for SUBCLASS Event Hooks to use
   bool ScrollToFLOk() const; // useful in VShowResultsNoMacs()
   int GetLifetimeMatchCount()            const { return d_lifetimeStats.GetMatchActions(); }
   int GetLifetimeFileCount()             const { return d_lifetimeFileCount              ; }
   int GetLifetimeFileCountMatch()        const { return d_lifetimeFileCountMatch         ; }
   int GetLifetimeFileCountMatchAction()  const { return d_lifetimeFileCountMatchAction   ; }
   };

bool FileSearchMatchHandler::FoundMatchContinueSearching( PFBUF pFBuf, const Point &cur, COL MatchCols, RegexMatchCaptures &captures ) {
   if( VMatchWithinColumnBounds( pFBuf, cur, MatchCols ) ) { // it IS a MATCH?
      if( !d_flToScroll.IsSet() ) {
         d_flToScroll.Set( pFBuf, cur, MatchCols );
         }
      d_lifetimeStats.IncMatch();
      d_curFileStats .IncMatch();
      if( VMatchActionTaken( pFBuf, cur, MatchCols, captures ) ) { // Is it also an ACTION?
         d_lifetimeStats.IncMatchAction();
         d_curFileStats .IncMatchAction();
         }
      }
   return VContinueSearching();
   }

bool FileSearchMatchHandler::ScrollToFLOk() const { return d_flToScroll.ScrollToOk(); }

//****************************
//****************************

class FindPrevNextMatchHandler : public FileSearchMatchHandler {
   NO_COPYCTOR(FindPrevNextMatchHandler);
   NO_ASGN_OPR(FindPrevNextMatchHandler);
   const LINE   d_yAtStart = g_CurView()->Cursor().lin;
   const char   d_dirCh;
   const bool   d_fIsRegex;
   const std::string d_SrchDispStr;
   void DrawDialog( PCChar hdr, PCChar trlr );
protected:
   bool VContinueSearching() override { return false; };  // stop at first match
public:
   FindPrevNextMatchHandler( bool fSearchForward, bool fIsRegex, stref srchStr );
   ~FindPrevNextMatchHandler() {}
   bool VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) {
      PCV;
      pcv->SetMatchHiLite( cur, MatchCols, cur.lin != d_yAtStart );
      return true;  // "action" taken!
      }
   void VShowResultsNoMacs() override;
   };

void FindPrevNextMatchHandler::DrawDialog( PCChar hdr, PCChar trlr ) {
   const auto de_color( 0x5f );
   const auto ss_color( g_fCase ? (bgRED|fgWHT) : (bgGRN|fgWHT) );
   STATIC_CONST char s_chRex = '/';
   ColoredStrefs csrs; csrs.reserve( d_fIsRegex ? 5 : 3 );
                      csrs.emplace_back( g_colorInfo , hdr );
   if( d_fIsRegex ) { csrs.emplace_back( de_color    , stref( &s_chRex, sizeof(s_chRex) ) ); }
                      csrs.emplace_back( ss_color    , d_SrchDispStr );
   if( d_fIsRegex ) { csrs.emplace_back( de_color    , stref( &s_chRex, sizeof(s_chRex) ) ); }
                      csrs.emplace_back( g_colorInfo , trlr, ePad::padWSpcsToEol );
   VidWrColoredStrefs( DialogLine(), 0, csrs );
   }

FindPrevNextMatchHandler::FindPrevNextMatchHandler( bool fSearchForward, bool fIsRegex, stref srchStr )
   : d_dirCh(fSearchForward ? '+' : '-')
   , d_fIsRegex(fIsRegex)
   , d_SrchDispStr( FormatExpandedSeg( COL_MAX, srchStr, 0, 1, g_chTabDisp, g_chTrailSpaceDisp ) )
   {
   if( !Interpreter::Interpreting() ) {
      DrawDialog( (('+'==d_dirCh)
                   ? "+Search for "
                   : "-Search for "
                  )
                , ""
                );
      }
   }

void FindPrevNextMatchHandler::VShowResultsNoMacs() {
   if( !VOverallRetval() ) {
      DrawDialog( (('+'==d_dirCh)
                   ? "+"
                   : "-"
                  )
                , " not found"
                );
      }
   }

//****************************

class FileSearcher;

class MFGrepMatchHandler : public FileSearchMatchHandler {
   PFBUF d_pOutputFile;
   std::string  d_sb;
protected:
   bool VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) override;
   void VShowResultsNoMacs() override;
public:
   MFGrepMatchHandler( PFBUF pOutputFile )
      : d_pOutputFile(pOutputFile)
      {
      Msg( "searching cache" );  // rewrite the dialog line so user doesn't think we've hung
      }
   void InitLogFile( const FileSearcher &FSearcher, stref src );
   STATIC_CONST SearchScanMode &sm() { return smFwd; }
   };

bool MFGrepMatchHandler::VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) {
   if( 0 == GetLifetimeMatchCount() ) {
      LuaCtxt_Edit::LocnListInsertCursor(); // do this IFF a match was found
      }
   CapturePrevLineCountAllWindows( d_pOutputFile );
   d_sb.assign( pFBuf->Namestr() );
   d_sb.append( FmtStr<40>( " %d %dL%d: ", cur.lin+1, cur.col+1, MatchCols ).c_str() );
   d_sb.append( pFBuf->PeekRawLine( cur.lin ) );
   d_pOutputFile->PutLastLineRaw( d_sb );
   MoveCursorToEofAllWindows( d_pOutputFile );
   return true;  // action taken!
   }

//****************************
//****************************

class SearchSpecifier {
#if USE_PCRE
   bool   d_fRegex;
#endif
   std::string  d_rawStr;
#if USE_PCRE
   bool   d_fRegexCase;   // state when last (re-)init'd
   CompiledRegex *d_re;
#endif
   bool   d_fNegateMatch; // same as grep's -v option
   bool   d_fCanUseFastSearch;
public:
   SearchSpecifier( stref rawSrc, bool fRegex=false );
   ~SearchSpecifier();
   bool   MatchNegated() const { return d_fNegateMatch; };
#if USE_PCRE
   CompiledRegex *re() const { return d_re; }
   bool   IsRegex()  const { return d_fRegex; }
   bool   HasError() const { return d_fRegex && !d_re; }
   int    MinHaystackLen() const { return IsRegex() ? 1 : d_rawStr.length(); }
#else
   bool   IsRegex()  const { return false; }
   bool   HasError() const { return false; }
   int    MinHaystackLen() const { return d_rawStr.length(); }
#endif
   bool   CanUseFastSearch() const { return d_fCanUseFastSearch && g_fFastsearch; }
   void   CaseUpdt(); // in case case switch has changed since CompiledRegex was compiled
   void   Dbgf( PCChar tag ) const;
   stref  SrchStr()    const { return d_rawStr; }
   };

STATIC_VAR SearchSpecifier *s_searchSpecifier;

#if USE_PCRE
static void CDECL__ atexit_Search() {
   Delete0( s_searchSpecifier );
   }

void register_atexit_search() {
   atexit( atexit_Search );
   }
#endif

void MFGrepMatchHandler::VShowResultsNoMacs() {
   const auto matches( GetLifetimeMatchCount() );
   const auto files  ( GetLifetimeFileCount()  );
   if( matches ) {
      fExecute( "mfgrep_done_nextmsg_FROM_C" ); // NOT a LuaCtxt_Edit::fxn because Lua code herein needs an ARG parameter
      }
   Msg( "%d match%s found%s in %d of %d file%s in %5.3f S"
      , matches
      , Add_es( matches )
      , s_searchSpecifier->CanUseFastSearch() ? " FAST" : ""
      , GetLifetimeFileCountMatch()
      , files
      , Add_s( files )
      , d_pc.Capture()
      );
   }

//****************************

class FileSearcher {
   NO_COPYCTOR(FileSearcher);
   NO_ASGN_OPR(FileSearcher);
protected:
   const SearchScanMode   &d_sm;
   const SearchSpecifier  &d_ss;
public:
   FileSearchMatchHandler &d_mh;
   Point                   d_start;
   Point                   d_end;
   PFBUF                   d_pFBuf;
   RegexMatchCaptures      d_captures;
   FileSearcher( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   class FindStrRslt { // must distinguish match (including 0-length match) and no match
      STATIC_VAR const char chNoMatch;
      const stref d_sr;
   public:
      FindStrRslt( PCChar data, size_t length ) : d_sr( data, length ) {}
      FindStrRslt( stref sr_ ) : d_sr( sr_ ) {}
      FindStrRslt( int ) : d_sr( &chNoMatch, 0 ) {} // chNoMatch
      FindStrRslt() : d_sr() {}
      stref sr() const { return d_sr; }
      bool noMatch() const { return &chNoMatch == d_sr.data(); }
      };
   virtual FindStrRslt VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) = 0; // rv.empty() if no match found
public:
   enum StringSearchVariant {
      fsTABSAFE_STRING ,  // FileSearcherString also bidirectional
      fsFAST_STRING    ,  // FileSearcherFast   about 20% faster than fsTABSAFE_STRING (WHEN SEARCHING CACHED FILES!!!)  (CANNOT SEARCH BACKWARD!)
      };
           void FindMatches();
   virtual void VFindMatches_();
   void SetInputFile( PFBUF pFBuf=g_CurFBuf() );
   void SetBoundsWholeFile();
   void SetBoundsToEnd( Point StartPt );
   void SetBounds( Point StartPt, Point EndPt );
   void SetBounds( const ARG &arg );
   void Dbgf() const;
   stref SrchStr() const { return d_ss.SrchStr(); }
   bool IsRegex() const { return
#if USE_PCRE
      d_ss.IsRegex();
#else
      false;
#endif
      }
   virtual ~FileSearcher();
protected:
   void ResolveDfltBounds();
   };
const char FileSearcher::FindStrRslt::chNoMatch = '\0';

GLOBAL_CONST Point g_PtInvalid = Point( -1, -1 );

void FileSearcher::ResolveDfltBounds() {
   auto &Bof( d_sm.d_fSearchForward ? d_start : d_end   );
   auto &Eof( d_sm.d_fSearchForward ? d_end   : d_start );
   if( g_PtInvalid == Bof ) { Bof = Point( 0, 0 );                         }
   if( g_PtInvalid == Eof ) { Eof = Point( d_pFBuf->LastLine(), COL_MAX ); }
   }

void FileSearcher::FindMatches() {
   ResolveDfltBounds();
   VFindMatches_();
   }

void FileSearcher::SetBoundsWholeFile() {
   d_start = g_PtInvalid;
   d_end   = g_PtInvalid;
   }

void FileSearcher::SetBoundsToEnd( Point StartPt ) {
   // this is the only method that would be used for backward searches (since ARG::msearch is the only backward-searching cmd)
   d_start = StartPt;
   d_end   = g_PtInvalid;
   }

void FileSearcher::SetBounds( Point StartPt, Point EndPt ) {
   d_start = StartPt;
   d_end   = EndPt;
   // Assert( d_sm.d_fSearchForward ? StartPt <= EndPt : StartPt >= EndPt );
   }

void FileSearcher::SetBounds( const ARG &arg ) {
   arg.BeginPt( &d_start );
   arg.EndPt  ( &d_end   );
   }

void FileSearcher::SetInputFile( PFBUF pFBuf ) {
   if( d_pFBuf ) {
      d_mh.VLeavingFile( d_pFBuf );
      }
   d_pFBuf = pFBuf;
   SetBoundsWholeFile();  // init dflt: whole file
   d_mh.VEnteringFile( d_pFBuf );
   }

FileSearcher::~FileSearcher() {
   if( d_pFBuf ) {
      d_mh.VLeavingFile( d_pFBuf );
      }
   }

void MFGrepMatchHandler::InitLogFile( const FileSearcher &FSearcher, stref src ) { // digression!
   LuaCtxt_Edit::nextmsg_setbufnm( kszSearchRslts );
   std::string sbuf;
   sbuf.append( "mfgrep::" );
   sbuf.append( FSearcher.IsRegex() ? "regex " : "str " );
   sbuf.append( FSearcher.SrchStr() );
   sbuf.append( " " );
   sbuf.append( src );                                      DBG( "%s: '%" PR_BSR "'", __func__, BSR(src) );
   LuaCtxt_Edit::nextmsg_newsection_ok( sbuf.c_str() );
   }

STATIC_FXN FileSearcher *NewFileSearcher( FileSearcher::StringSearchVariant type, const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );

class  FileSearcherString : public FileSearcher {
   std::string        d_searchKey;
   const pFxn_strstr  d_pfxStrnstr;
   NO_COPYCTOR(FileSearcherString);
   NO_ASGN_OPR(FileSearcherString);
public:
   FileSearcherString( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   ~FileSearcherString() {}
   FindStrRslt VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) override;
   };

class  FileSearcherFast : public FileSearcher {  // ONLY SEARCHES FORWARD!!!
   std::string        d_searchKey;
   const pFxn_strstr  d_pfxStrnstr;
   std::vector<stref> d_pNeedles;
   NO_COPYCTOR(FileSearcherFast);
   NO_ASGN_OPR(FileSearcherFast);
public:
   FileSearcherFast( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   virtual ~FileSearcherFast() {}
   void   VFindMatches_() override;
   FindStrRslt  VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) override;
   };

#if USE_PCRE

class  FileSearcherRegex : public FileSearcher {
   NO_COPYCTOR(FileSearcherRegex);
   NO_ASGN_OPR(FileSearcherRegex);
public:
   FileSearcherRegex( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   FindStrRslt VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) override;
   };

#endif

//*****************************************************************************************************

Rect::Rect( PFBUF pFBuf ) {
   flMin.col = 0;
   flMin.lin = 0;
   flMax.col = COL_MAX;
   flMax.lin = pFBuf->LastLine();
   }

Rect::Rect( bool fSearchFwd ) {
   flMin.col = 0;
   flMin.lin = fSearchFwd ? g_CursorLine() : 0;
   flMax.col = COL_MAX;
   flMax.lin = g_CurFBuf()->LastLine();
   }

bool FBUF::RefreshFailed() {
   if( HasLines() ) {
      SyncNoWrite();
      return false; // no failure
      }
   return ReadDiskFileNoCreateFailed();
   }

bool FBUF::RefreshFailedShowError() {
   const auto failed( RefreshFailed() );
   if( failed && !ExecutionHaltRequested() ) {
      return ErrorDialogBeepf( "Cannot read '%s'", Name() );
      }
   return failed;
   }

//=============================================================================================

void View::SetStrHiLite( const Point &pt, COL Cols, ColorTblIdx color ) {
   const auto hiliteWidth( Cols > 0 ? Cols : 1 );
   InsHiLite1Line( color, pt.lin, pt.col, pt.col + hiliteWidth - 1 );
   }

void View::SetMatchHiLite( const Point &pt, COL Cols, bool fErrColor ) {
   const auto colorIdx( fErrColor ? ColorTblIdx::ERRM : ColorTblIdx::SEL );
   const auto hiliteWidth( Cols > 0 ? Cols : 1 );
   enum { MWHOSMHL = 0 }; // -> MASK_WUC_HILITES_ON_SEARCH_MATCH_HILIT_LINE
   if( MWHOSMHL && pt.col > 0 ) { InsHiLite1Line( ColorTblIdx::CXY, pt.lin, 0                   , pt.col               - 1 ); }
                                  InsHiLite1Line( colorIdx        , pt.lin, pt.col              , pt.col + hiliteWidth - 1 );
   if( MWHOSMHL               ) { InsHiLite1Line( ColorTblIdx::CXY, pt.lin, pt.col + hiliteWidth, COL_MAX                  ); }
   }

class HiLiteFreer {
   const PView d_View;
public:
   HiLiteFreer() :  d_View( g_CurView() ) {}
   ~HiLiteFreer() { d_View->FreeHiLiteRects(); }
   };

enum CheckNextRetval { STOP_SEARCH, CONTINUE_SEARCH, REREAD_LINE_CONTINUE_SEARCH, DO_REPLACE, /* CONTINUE_SEARCH_NEXT_LINE */ };

class CharWalker_ {
public:
   virtual CheckNextRetval VCheckNext( stref rl, const Point &curPtIx, COL tabWidth ) = 0;
   virtual ~CharWalker_() {};
   };

// CharWalkToEnd is called by PBalFindMatching & PMword
STATIC_FXN bool CharWalkToEnd( bool fWalkFwd, bool fVisibleOnly, PFBUF pFBuf, const Point &start, CharWalker_ &walker ) {
   const LINE yLast( fWalkFwd ? (fVisibleOnly ? g_CurView()->MaxVisibleFbufLine() : g_CurFBuf()->LastLine())
                              : (fVisibleOnly ? g_CurView()->MinVisibleFbufLine() : 0 )
                   );
   auto dbgStep( 0 );
   0 && DBG( "%s: @(y=%d,x=%d) LINEs(%d->%d)", __func__, start.lin, start.col, start.lin, yLast );
   const auto tw( pFBuf->TabWidth() );
   #define CHECK_BREAK                          \
           if( 0==(curPt.lin & 0x1FF) && ExecutionHaltRequested() ) { \
              FlushKeyQueuePrimeScreenRedraw(); \
              return false;                     \
              }
   if( fWalkFwd ) { // -------------------- search FORWARD --------------------
      // convert curPt.col from col to ix
      Point curPt( start.lin, FreeIdxOfCol( tw, pFBuf->PeekRawLine( start.lin ), start.col + 1 ) );
      for( ; curPt.lin <= yLast ; ++curPt.lin, curPt.col = 0 ) {
         CHECK_BREAK;
         const auto rl( pFBuf->PeekRawLine( curPt.lin ) );
         for( ; curPt.col < rl.length() ; ++curPt.col ) {
            if( STOP_SEARCH == walker.VCheckNext( rl, curPt, tw ) ) {
               return true;
               }
            }
         }
      }
   else { // -------------------- search BACKWARD --------------------
      Point curPt( start );                                                                          0 && DBG( "rvs%d: curPt( %d, %d )", ++dbgStep, curPt.lin, curPt.col );
      // convert curPt.col from col to ix
      curPt.col = CaptiveIdxOfCol( tw, pFBuf->PeekRawLine( curPt.lin ), curPt.col );                 0 && DBG( "rvs%d: curPt( %d, %d )", ++dbgStep, curPt.lin, curPt.col );
      // move cursor backward one ix (underflow handled in for)
      curPt.col = ((curPt.col<=0) ? pFBuf->PeekRawLine( --curPt.lin ).length() : curPt.col) - 1;     0 && DBG( "rvs%d: curPt( %d, %d )", ++dbgStep, curPt.lin, curPt.col );
      for( ; curPt.lin >= yLast ; curPt.col = pFBuf->PeekRawLine( --curPt.lin ).length() - 1 ) {
         CHECK_BREAK;                                                                                0 && DBG( "rvs%d: curPt( %d, %d/%" PR_SIZET " )", ++dbgStep, curPt.lin, curPt.col, pFBuf->PeekRawLine( curPt.lin ).length()-1 );
         for( ; curPt.col >= 0 ; --curPt.col ) {                                                     0 && DBG( "rvsX: curPt( %d, %d )", curPt.lin, curPt.col );
            if( STOP_SEARCH == walker.VCheckNext( pFBuf->PeekRawLine( curPt.lin ), curPt, tw ) ) {
               return true;
               }
            }
         }
      }
   return false;
   #undef CHECK_BREAK
   }

STATIC_VAR std::string g_SavedSearchString_Buf ;
GLOBAL_VAR std::string g_SnR_stSearch          ;
GLOBAL_VAR std::string g_SnR_stReplacement     ;

GTS::eRV GTS::addText( stref sr ) {
   if( fInitialStringSelected_ ) {
      stb_.assign( sr );
      xCursor_ = stb_.length();
      }
   else {
      stb_.insert( xCursor_, sr );
      xCursor_ += sr.length();
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::qreplace() {
   return addText( g_SnR_stSearch );
   }

class CharWalkerReplace {
   std::string             d_sbuf; // replace-action getLine... tmp
   std::string             d_stmp; // replace-action PutLine tmp
   const SearchSpecifier  &d_ss;
   RegexMatchCaptures      d_captures;
   class replaceDope_t { // this is instantiated as const (only)
      struct refSelector {
         bool  d_isLit;
         sridx d_contentIx;
         refSelector( bool isLit, sridx idx ) : d_isLit( isLit ), d_contentIx( idx ) {}
         };
      std::vector<stref>       d_Literal;
      std::vector<refSelector> d_replaceRefs;
      void AddLitRef( stref sr ) {
         if( sr.length() > 0 ) {
            d_Literal.emplace_back( sr );
            d_replaceRefs.emplace_back( true, d_Literal.size()-1 );
            }
         }
      void AddBackRef( sridx ix ) {
         d_replaceRefs.emplace_back( false, ix );
         }
   public:
      const std::vector<stref>       &Literal()     const { return d_Literal    ; }
      const std::vector<refSelector> &ReplaceRefs() const { return d_replaceRefs; }
      replaceDope_t( stref srReplace, const SearchSpecifier &ss ) {
         stref srSrc( srReplace ); // srReplace is assumed to persist for lifetime of CharWalkerReplace object!
#if USE_PCRE
         if( ss.IsRegex() ) {
            constexpr char chBR( '\\' );
            for(;;) {
               const auto ix( srSrc.find( chBR ) );
               if( ix == eosr || ix == srSrc.length()-1 ) { break; }
               if( srSrc[ix+1]==chBR ) {
                  AddLitRef( stref( srSrc.data(), ix+1 ) );
                  srSrc.remove_prefix( ix+2 );
                  }
               else {
                  if( isdigit( srSrc[ix+1] ) ) { // we only support SINGLE-DIGIT replacement-string backrefs (\0..\9)
                     const sridx bkrefNum( srSrc[ix+1] - '0' );
                     AddLitRef( stref( srSrc.data(), ix ) );  // _must precede_ the AddBackRef within this block!
                     AddBackRef( bkrefNum );                  // _must follow_  the AddLitRef  within this block!
                     srSrc.remove_prefix( ix+2 );
                     }
                  else { // this is inefficient (leads to multiple LitRefs describing a single srReplace substring), but ...
                     AddLitRef( stref( srSrc.data(), ix+2 ) );
                     srSrc.remove_prefix( ix+2 );
                     }
                  }
               }
            }
#endif
         AddLitRef( srSrc );
         }
      }; // class replaceDope_t
   const replaceDope_t d_replaceDope;
   std::string   d_stReplace;
   ColoredStrefs d_promptCsrs;

   stref GenerateReplacement() {
      /* there is some inefficiency here impacting non-interactive replace
         mode (which includes interactive in "all" mode):
           in the non-regex case (or if regex but replacement string has no
           backrefs) rv and d_promptCsrs are the same for all matches.

           in the non-interactive case d_promptCsrs is superfluous

       */
      d_promptCsrs.reserve( d_replaceDope.ReplaceRefs().size() );
      d_promptCsrs.clear();
      d_promptCsrs.emplace_back( g_colorStatus, "Substitute " );
      constexpr PCChar endq = " (Yes/No/All/Quit)? ";
      if( d_replaceDope.ReplaceRefs().size()==1 ) { // optimize d_stReplace-not-needed case
         constexpr auto ixEnt( 0u ), ixCont( 0u );
         const auto &ent( d_replaceDope.ReplaceRefs()[ixEnt] );
         const auto rv_only( ent.d_isLit ? d_replaceDope.Literal()[ixCont] : (ixCont < d_captures.size() ? d_captures[ixCont].value() : "") );
         d_promptCsrs.emplace_back( ent.d_isLit ? g_colorInfo : g_colorError, rv_only );
         d_promptCsrs.emplace_back( g_colorStatus, endq, ePad::padWSpcsToEol );
         return rv_only;
         }
      d_stReplace.clear();
      for( const auto &ent : d_replaceDope.ReplaceRefs() ) {
         const auto ixCont( ent.d_contentIx );
         const stref sr(  ent.d_isLit
                       ? (ixCont < d_replaceDope.Literal().size() ? d_replaceDope.Literal()[ixCont]         : "")
                       : (ixCont < d_captures             .size() ? d_captures             [ixCont].value() : "")
                       );
         if( sr.length() > 0 ) {
            d_stReplace.append( sr );
            d_promptCsrs.emplace_back( ent.d_isLit ? g_colorInfo : g_colorError, sr );
            }
         }
      d_promptCsrs.emplace_back( g_colorStatus, endq, ePad::padWSpcsToEol );
      return stref( d_stReplace );
      }
   const bool        d_fDoAnyReplaceQueries; // only to support Interactive() method
   bool              d_fDoReplaceQuery;
   const pFxn_strstr d_pfxStrnstr;
   FBufLocn          d_user_refused; // last match to which user replied "no" to a replace query
   LINE              d_lastLineVisited = -1;  // only to calc d_linesVisited
public:
   size_t            d_linesVisited = 0;
   size_t            d_CheckNextCallCount = 0;
   int               d_iReplacementsPoss = 0;
   int               d_iReplacementsMade = 0;
   int               d_iReplacementFiles = 0;
   int               d_iReplacementFileCandidates = 0;
   CharWalkerReplace(
        bool fDoReplaceQuery
      , bool fSearchCase
      , const SearchSpecifier &ss
      )
      : d_ss                ( ss )
      , d_replaceDope       ( g_SnR_stReplacement, ss )
      , d_fDoAnyReplaceQueries( fDoReplaceQuery )
      , d_fDoReplaceQuery   ( fDoReplaceQuery )
      , d_pfxStrnstr        ( fSearchCase ? strnstr : strnstri )
      {}
   bool Interactive() const { return d_fDoAnyReplaceQueries; }
   CheckNextRetval CheckNext(       PFBUF pFBuf, IdxCol_cached &rlc, sridx ixBOL, Point *curPt, COL *colLastPossibleMatchChar, bool fWholeLine );
   CheckNextRetval DoReplace(       PFBUF pFBuf, IdxCol_cached &rlc, sridx ixBOL, Point *curPt, COL *colLastPossibleMatchChar, sridx ixLastPossibleLastMatchChar );
   CheckNextRetval GetUserApproval( PFBUF pFBuf, IdxCol_cached &rlc, Point *curPt, sridx ixMatchMin, sridx ixMatchMax );
   };

CheckNextRetval CharWalkerReplace::GetUserApproval( PFBUF pFBuf, IdxCol_cached &rlc, Point *curPt, sridx ixMatchMin, sridx ixMatchMax ) { enum { DB=0 };
   const auto pView( pFBuf->PutFocusOn() );
   // rlc_post_focus exists (vs rlc) because pFBuf->TabWidth() may have changed (vs rlc.tw()) due to side effects of PutFocusOn()
   IdxCol_cached rlc_post_focus( pFBuf->TabWidth(), rlc.sr() );
   const auto xMatchMin( rlc_post_focus.i2c( ixMatchMin ) );
   const auto xMatchMax( rlc_post_focus.i2c( ixMatchMax ) );   DB && rlc.tw() != rlc_post_focus.tw() && DBG( "%s tw=%d->%d ix[%" PR_SIZET "..%" PR_SIZET "] col[%d..%d]", __func__, rlc.tw(), rlc_post_focus.tw(), ixMatchMin, ixMatchMax, xMatchMin, xMatchMax );
   auto adv_continue = [&]() {
      curPt->col = rlc_post_focus.ColOfNextChar( curPt->col );
      return CONTINUE_SEARCH;
      };
   const auto matchCols( xMatchMax - xMatchMin + 1 );
   Point matchBegin( curPt->lin, xMatchMin );
   FBufLocn thisMatch( pFBuf, matchBegin, matchCols );
   if( d_user_refused == thisMatch ) { // when performing Regex relaces, multiple consecutive search iterations can produce
      return adv_continue();           // THE SAME match; if the user already refused to replace this match, don't ask again!
      }
   ++d_iReplacementsPoss;  //##### it's A REPLACEABLE MATCH
   pView->FreeHiLiteRects();
   DispDoPendingRefreshesIfNotInMacro();
   pView->MoveCursor         ( matchBegin, matchCols );
       // MoveAndCenterCursor
   pView->SetMatchHiLite( matchBegin, matchCols, true );
   DispDoPendingRefreshesIfNotInMacro();
   HiLiteFreer hf;
#if USE_PCRE
   const auto szAllowed( d_ss.IsRegex() ? "ynsaq" : "ynaq" );
#else
   constexpr auto szAllowed( "ynaq" );
#endif
   const auto ch( chGetCmdPromptResponse
      ( szAllowed     //  allowed responses
      , '?'           //  interactive dflt: cause retry by NOT matching any explicit case below
      , 'a'           //  macro dflt:
      , d_promptCsrs  //
      ) );
   switch( ch ) {
      default:  Assert( 0 );        // chGetCmdPromptResponse has bug or params wrong
                return STOP_SEARCH;
      case -1 : ATTR_FALLTHRU;
      case 'q': SetUserChoseEarlyCmdTerminate();
                return STOP_SEARCH;
      case 'n': d_user_refused = thisMatch;
                return adv_continue();
      case 's': curPt->col = xMatchMax;     // advance cursor past entire match (dflt 'n' only advances to next char)
                return CONTINUE_SEARCH;     // mfrplcword "GenericReplace" nl "foobar"
      case 'a': d_fDoReplaceQuery = false;  ATTR_FALLTHRU;
      case 'y': return DO_REPLACE;          // perform replacement (below)
      }
   }

CheckNextRetval CharWalkerReplace::DoReplace( PFBUF pFBuf, IdxCol_cached &rlc, const sridx ixBOL, Point *curPt, COL *colLastPossibleMatchChar, const sridx ixLastPossibleLastMatchChar ) { enum { DB=0 };
   const auto ixMatchMin( d_captures[0].offset() + ixBOL );  // d_captures[0] describes the overall match
   const auto ixMatchMax( ixMatchMin + d_captures[0].value().length() - 1 );
   if( ixMatchMax > ixLastPossibleLastMatchChar ) {  // match lies partially OUTSIDE a BOXARG: skip (isn't this impossible?)
      curPt->col = rlc.ColOfNextChar( curPt->col );
      return CONTINUE_SEARCH;
      }
   stref srReplace( GenerateReplacement() ); // generates d_promptCsrs too!
   if( d_fDoReplaceQuery ) { // interactive-replace (mfreplace/qreplace) ONLY ...
      const auto rv( GetUserApproval( pFBuf, rlc, curPt, ixMatchMin, ixMatchMax ) );
      if( rv != DO_REPLACE ) { return rv; }
      }
   else {
      ++d_iReplacementsPoss;  //##### it's A REPLACEABLE MATCH
      }
   // perform replacement...
   const auto destMatchChars( ixMatchMax - ixMatchMin + 1 );          DB && DBG("DFPoR+ y=%d ixMaxValid=%" PR_PTRDIFFT " ixMaxPossMatch=%" PR_PTRDIFFT " ixMatch[%" PR_PTRDIFFT ",%" PR_PTRDIFFT "] L=%" PR_PTRDIFFT, curPt->lin, rlc.sr().length()-1, ixLastPossibleLastMatchChar, ixMatchMax, ixMatchMin, destMatchChars );
   d_sbuf.assign( rlc.sr() );
   d_sbuf.replace( ixMatchMin, destMatchChars, srReplace );           DB && DBG("DFPoR+ cursor=(%d,%d) LR=%" PR_SIZET " LoSB=%" PR_PTRDIFFT, curPt->lin, curPt->col, srReplace.length(), d_sbuf.length() );
   pFBuf->PutLineEntab( curPt->lin, d_sbuf, d_stmp );  // ... and commit
   ++d_iReplacementsMade;
   // replacement done: adjust starting, limit point for next iteration
   IdxCol_cached conv( pFBuf->TabWidth(), d_sbuf );
   const auto ixCurcol( ixMatchMin                  + srReplace.length() );
   const auto ixLPMC  ( ixLastPossibleLastMatchChar + srReplace.length() - destMatchChars );  DB && DBG("DFPoR: ix: cur=%" PR_PTRDIFFT " ixLPMC=%" PR_PTRDIFFT, ixCurcol, ixLPMC );
   if( ixCurcol < ixLPMC ) {
      curPt->col                = conv.i2c( ixCurcol );
      *colLastPossibleMatchChar = conv.i2c( ixLPMC   );
      }
   else {
      *colLastPossibleMatchChar = conv.i2c( ixLPMC   );
      curPt->col                = conv.i2c( ixCurcol );
      }                                                               DB && DBG("DFPoR- cursor=(%d,%d),%d", curPt->lin, curPt->col, *colLastPossibleMatchChar );
   return REREAD_LINE_CONTINUE_SEARCH;
   }

CheckNextRetval CharWalkerReplace::CheckNext( PFBUF pFBuf, IdxCol_cached &rlc, const sridx ixBOL, Point *curPt, COL *colLastPossibleMatchChar, const bool fWholeLine ) { enum { DB=0 };
   if( d_CheckNextCallCount != SIZE_MAX ) { ++d_CheckNextCallCount; }
   if( d_lastLineVisited != curPt->lin ) {  DB && DBG( "d_lastLineVisited != curPt->lin: %d != %d", d_lastLineVisited, curPt->lin );
       d_lastLineVisited  = curPt->lin;
       ++d_linesVisited;
       }
   const sridx ix_curPt_Col( rlc.c2ci( curPt->col ) );
   const auto srRawSearch( d_ss.SrchStr() );
   const auto ixLastPossibleLastMatchChar( fWholeLine ? rlc.sr().length()-1 : rlc.c2ci( *colLastPossibleMatchChar ) );
   d_captures.clear();                             DB && DBG( "%s ( %d, %d L %" PR_SIZET " ) for '%" PR_BSR "' in raw '%" PR_BSR "'", __PRETTY_FUNCTION__, curPt->lin, curPt->col, srRawSearch.length(), BSR(srRawSearch), BSR(rlc.sr()) );
   const auto haystack( rlc.sr().substr( ixBOL, ixLastPossibleLastMatchChar + 1 - ixBOL ) );
                                                   DB && DBG( "%s ( %d, %d L %" PR_SIZET " ) for '%" PR_BSR "' in hsk '%" PR_BSR "'", __PRETTY_FUNCTION__, curPt->lin, curPt->col, srRawSearch.length(), BSR(srRawSearch), BSR(haystack) );
   const auto ixHaystackCurCol( ix_curPt_Col - ixBOL );  // leading ix_curPt_Col chars will not be searched
#if USE_PCRE
   if( d_ss.IsRegex() ) {
   // const auto pcre_exec_flags( ixHaystackCurCol == 0 ? 0 : PCRE_NOTBOL ); // !/PCRE_NOTBOL describes haystack[0], not haystack[ixHaystackCurCol] (in Regex_Match call), so
      const auto pcre_exec_flags(                         0               ); DB && DBG( "%s (%d,%d) for '%" PR_BSR "' in '%" PR_BSR "[%" PR_BSR "'", __PRETTY_FUNCTION__, curPt->lin, curPt->col, BSR(srRawSearch), BSR(haystack.substr(0,ixHaystackCurCol)), BSR(haystack.substr(ixHaystackCurCol)) );
      const auto rv( Regex_Match( d_ss.re(), d_captures, haystack, ixHaystackCurCol, pcre_exec_flags ) );
      if( rv == 0 || !d_captures[0].valid() ) {      // no match this line?
         curPt->col = *colLastPossibleMatchChar + 1; // next check next line
         return CONTINUE_SEARCH;
         }
      }
   else
#endif
      {
      const auto hsTail( haystack.substr( ixHaystackCurCol ) );
      const auto relIxMatch( d_pfxStrnstr( hsTail, srRawSearch ) );
      if( relIxMatch == stref::npos ) {              // no match this line?
         curPt->col = *colLastPossibleMatchChar + 1; // next check next line
         return CONTINUE_SEARCH;
         }
      d_captures.emplace_back( ixHaystackCurCol + relIxMatch, hsTail.substr( relIxMatch, srRawSearch.length() ) );
      }                                                               DB && DbgDumpCaptures( d_captures, "?" );
   return DoReplace( pFBuf, rlc, ixBOL, curPt, colLastPossibleMatchChar, ixLastPossibleLastMatchChar );
   }

STATIC_FXN bool CharWalkRectReplace( PFBUF pFBuf, const Rect &within, Point start, CharWalkerReplace &walker ) { enum { DB=0 };
   const bool fWholeLine( within.flMax.col == COL_MAX );
   DB && DBG( "%s: within=LINEs(%d-%d) COLs(%d,%d)", __func__, within.flMin.lin, within.flMax.lin, within.flMin.col, within.flMax.col );
   const auto tw( pFBuf->TabWidth() );
   for( Point curPt( start ) ; curPt.lin <= within.flMax.lin ; ++curPt.lin, curPt.col = within.flMin.col ) {
      if( ExecutionHaltRequested() ) {
         FlushKeyQueuePrimeScreenRedraw();
         return false;
         }
      IdxCol_cached  rlc( tw, pFBuf->PeekRawLine( curPt.lin ) );
      auto ixBOL( rlc.c2ci( within.flMin.col ) );
      auto colLastPossibleMatchChar( std::min( rlc.i2c_nocache( rlc.sr().length()-1 ), within.flMax.col ) );
      while( ( DB && DBG( "COL: cur %d vs %d lastPoss ixLastCh %" PR_SIZET, curPt.col, colLastPossibleMatchChar, rlc.sr().length()-1 ), curPt.col <= colLastPossibleMatchChar ) ) {
         const auto rv( walker.CheckNext( pFBuf, rlc, ixBOL, &curPt, &colLastPossibleMatchChar, fWholeLine ) );
         if( STOP_SEARCH == rv ) { return true; }
         if( REREAD_LINE_CONTINUE_SEARCH == rv ) {
            rlc.reset( pFBuf->PeekRawLine( curPt.lin ) );
            ixBOL = rlc.c2ci( within.flMin.col );
            // colLastPossibleMatchChar (cLPMC) is passed to CheckNext by non-const reference because if a
            // replacement (which may have inserted chars into (or deleted chars from) the "replacement
            // region") occurs on the line, CheckNext must adjust cLPMC accordingly so that further
            // processing on the line will constrain replacement activity to the range of chars which ends
            // with the last char in the pre-replacement range.
            }
         }
      }
   return false;
   }

//
//  Compile
//
//  Compile
//
//     Displays status of the current compile (if any) on the status bar.
//
//  Arg Compile
//
//     Runs the Extmake setting for the current file.
//
//     The command and arguments used to compile the file are specified by the
//     Extmake switch according to the extension of the file.
//
//  Arg <textarg> Compile
//
//     Runs the Extmake setting for <textarg>, where <textarg> is considered
//     the current file name, replacing %s in the specified command.
//     See: Extmake
//
//  Arg Arg <textarg> Compile
//
//     Runs the specified text as a program.
//
//     The program is assumed to display errors in the format:
//
//     <file> <row> <col> <message>
//
//  Arg Meta Compile
//
//     Stops a background compile after prompting for confirmation.
//
//  Returns
//
//  True:  Operation successfully initiated
//  False: Operation not initiated
//
#ifdef fn_waitcompiledone
bool ARG::waitcompiledone() {
   const auto tov( IS_NUMARG ? (NUMARG_VALUE * 1000) : 20000 );
   Msg( "%s waiting %d mS", __func__, tov );
   const auto rv( !CompileJobQueueWaitExeDoneTimedout( tov ) );
   Msg( "%s %s", __func__, rv ? "OK" : "timed out!" );
   return rv;
   }
#endif

int FBUF::SyncWriteIfDirty_Wrote() {
   SyncNoWrite();
   if( IsDirty() && FnmIsDiskWritable() ) {
      WriteToDisk();
      return 1;
      }
   return 0;
   }

STATIC_FXN bool SetNewSearchSpecifierOK( stref src, bool fRegex ) {
   VS_( if( s_searchSpecifier ) { s_searchSpecifier->Dbgf( "befor" ); } )
   std::unique_ptr<SearchSpecifier> ssNew( new SearchSpecifier( src, fRegex ) );
#if USE_PCRE
   const auto err( ssNew->HasError() );
   if( !err )
#endif
      {
      DeleteUp( s_searchSpecifier, ssNew.release() );
      g_SavedSearchString_Buf.assign( src );  // HACK to let ARG::grep inherit prev search strings
      }
   VS_( s_searchSpecifier->Dbgf( "after" ); )
   return
#if USE_PCRE
          !err
#else
           true
#endif
               ;
   }

STATIC_FXN void AddLineToLogStack( PFBUF pFbuf, stref str ) { // deletes all duplicates of the inserted line
   auto mods( 0u );
   for( auto ln(1); ln < pFbuf->LineCount(); ++ln ) {
      if( pFbuf->PeekRawLine( ln ) == str ) { // loop across all needles in [d_searchKey .. d_searchKey + d_searchKeyStrlen - 1]
         pFbuf->DelLine( ln-- ); // delete ALL duplicate strings
         ++mods;
         }
      }
   if( pFbuf->PeekRawLine( 0 ) != str ) {
      pFbuf->InsLineRaw( 0, str );
      ++mods;
      }
   if( mods ) {
      pFbuf->UnDirty();  // cosmetic
      }
   }

void AddToTextargStack( stref str ) {  0 && DBG( "%s: '%" PR_BSR "'", __func__, BSR(str) );
   AddLineToLogStack( g_pFBufTextargStack, str );
   }

void AddToSearchLog( stref str ) {  // *** CALLED BY LUA Libfunc
   AddLineToLogStack( g_pFBufSearchLog, str );
   }

STATIC_FXN bool SearchLogSwap() {
   if( g_CurFBuf() == g_pFBufSearchLog ) {
      fExecute( "setfile" );
      return true;
      }
   return false;
   }

////////////////////////////////////////////////////////////////////////////////////////////
//
// SearchLog - switch to <searchlog> pseudofile
//
// arg textarg SearchLog  (TEXTARG actually effectively NUMARG)
//                        - textarg MUST be a 1-char string having value ['1'..'9']
//                        - switches to <searchlog>, moves cursor to start of
//                          line number given in textarg.
//
// SearchLog              (NOARG)
//                        - switches to <searchlog>, cursor position unaffected
//
#ifdef DOUT_ENABLED
STATIC_FXN void testing();
#endif

bool ARG::searchlog() {
   auto yLine(0);
   if( TEXTARG == d_argType ) {
      if(  !isdigit( d_textarg.pText[0] )
        ||  chNUL != d_textarg.pText[1]
        ) { return Msg( "textarg of searchlog can only be single char ['1'..'9'] = line number!" ); }
      yLine = d_textarg.pText[0] - '0';
      }
   g_pFBufSearchLog->PutFocusOn();
   if( yLine > 0 ) {
      g_CurView()->MoveCursor( yLine-1, 0 );
      }
   return true;
   }

STATIC_FXN bool SearchSpecifierOK( const ARG &arg ) {
   if( arg.d_fMeta ) { g_fCase = !g_fCase; }
   switch( arg.d_argType ) {
      break;default:      return arg.BadArg();
      break;case TEXTARG: SearchLogSwap();
                          AddToSearchLog( arg.d_textarg.pText );
                          if( !SetNewSearchSpecifierOK( arg.d_textarg.pText, arg.d_cArg >= 2 ) ) {
                             return ErrorDialogBeepf( "bad search specifier '%s'", arg.d_textarg.pText );
                             }
      break;case NOARG:   if( s_searchSpecifier ) {
                              s_searchSpecifier->CaseUpdt();
                              }
                           else {
                              if( !g_pFBufSearchLog || FBOP::IsLineBlank( g_pFBufSearchLog, 0 ) ) {
                                 return ErrorDialogBeepf( "No search string specified, %s empty", kszSearchLog );
                                 }
                              const auto rl( g_pFBufSearchLog->PeekRawLine( 0 ) );
                              if( !SetNewSearchSpecifierOK( rl, false ) ) {
                                 return ErrorDialogBeepf( "bad search specifier '%" PR_BSR "'", BSR(rl) );
                                 }
                              }
      }
   return true;
   }

STATIC_FXN void GarbageCollectFBUF( PFBUF pFBuf, bool fGarbageCollect ) {
   if( fGarbageCollect ) {  // modern OS's do an excellent job of filesystem caching; re-read overhead is mostly line-end parsing which is quite fast these days
      pFBuf->FreeLinesAndUndoInfo();
   // DeleteAllViewsOntoFbuf( pFBuf );
      }
   // we don't want to clutter the editor state file with the names of files
   // we've only opened because of mfgrep/mfreplace activity, BUT this occurs
   // automatically because in mfgrep/mfreplace code, a searched FBUF for which
   // no state file entry (View) exists will not be linked to any View unless the
   // (mfreplace) activity actually causes the FBUF to get focus.  FBUFs not
   // linked to any View are not saved in the state file (state file generation
   // is driven by walking the windows' View lists).
   }

STATIC_FXN bool mf_RefreshFailedShowError( PFBUF pFBuf ) {
   g_fMfgrepRunning = true;
   const auto rv( pFBuf->RefreshFailedShowError() );
   g_fMfgrepRunning = false;
   return rv;
   }

STATIC_FXN void MFGrepProcessFile( stref filename, FileSearcher *d_fs ) {
   const auto pFBuf( FBOP::FindOrAddFBuf( filename ) );  0 && DBG( "d_dhdViewsOfFBUF.IsEmpty(%" PR_BSR ")==%c", BSR(filename), pFBuf->ViewCount()==0?'t':'f' );
   const auto fWeCanGarbageCollectFBUF( !pFBuf->HasLines() );
   if( mf_RefreshFailedShowError( pFBuf ) ) {
      return;
      }
   d_fs->SetInputFile( pFBuf );
   d_fs->FindMatches();
   if( d_fs->d_mh.VCanForgetCurFile() ) {
      GarbageCollectFBUF( pFBuf, fWeCanGarbageCollectFBUF );
      }
   else {
      DispDoPendingRefreshesIfNotInMacro();
      }
   }

STATIC_FXN PCChar findDelim( PCChar tokStrt, PCChar delimset, PCChar macroName ) {
   const auto pastTokEnd( StrToNextOrEos( tokStrt, delimset ) );
   if( chNUL == *pastTokEnd ) {
      ErrorDialogBeepf( "Macro '%s': value has unbalanced %s delimiter", macroName, delimset );
      return nullptr;
      }
   return pastTokEnd;
   }

std::string DupTextMacroValue( PCChar macroName ) {
   const auto pCmd( CmdFromName( macroName ) );
   if( !pCmd || !pCmd->IsRealMacro() ) {
      return std::string();
      }
   auto val( pCmd->MacroStref() );
   trim( val );
   return std::string( unquote( val ) );
   }

STATIC_FXN PathStrGenerator *MultiFileGrepFnmGenerator_() { enum { DB=0 };
   {
   const auto srcNm("mffile");
   const auto macroVal( DupTextMacroValue( srcNm ) );
   if( !IsStringBlank( macroVal ) ) {                                             DB && DBG( "%s: FindFBufByName[%s]( %" PR_BSR " )?", __func__, srcNm, BSR(macroVal) );
      const auto pFBufMfspec( FindFBufByName( macroVal.c_str() ) );
      if( pFBufMfspec && !FBOP::IsBlank( pFBufMfspec ) ) {
         if( FBOP::IsBlank( pFBufMfspec ) ) {
            if( 0 == cmp( pFBufMfspec->Name(), "<tagged-files>" ) ) {
               // Msg( "mffile:=\"%s\" but it's empty", pFBufMfspec->Name() );
               return nullptr;
               }
            }
         else {
            return new FilelistCfxFilenameGenerator( std::string(srcNm) + "=" + pFBufMfspec->Namestr(), pFBufMfspec );
            }
         }
      }
   }
   {
   const auto srcNm("<mfspec>");
   const auto pFBufMfspec( FindFBufByName( srcNm ) );
   if( pFBufMfspec && !FBOP::IsBlank( pFBufMfspec ) ) {
      return new FilelistCfxFilenameGenerator( std::string(srcNm) + " (buffer)", pFBufMfspec );
      }
   }
   auto txtMacro2CFG = [&] ( PCChar srcNm ) -> PathStrGenerator * {
      const auto macroVal( DupTextMacroValue( srcNm ) );
      if( !IsStringBlank( macroVal ) ) {                                          DB && DBG( "%s: FindFBufByName[%s]( %" PR_BSR " )?", __func__, srcNm, BSR(macroVal) );
         Path::str_t macroPVal( macroVal );
         LuaCtxt_Edit::ExpandEnvVarsOk( macroPVal );                              DB && DBG( "ExpandEnvVarsOk( %" PR_BSR " )", BSR(macroPVal) );
         return new CfxFilenameGenerator( std::string(srcNm) + " (macro) = " + macroPVal, macroPVal, ONLY_FILES );
         }
      return nullptr;
      };
   { const auto rv( txtMacro2CFG("mfspec"  ) ); if( rv ) { return rv; } }
   { const auto rv( txtMacro2CFG("mfspec_" ) ); if( rv ) { return rv; } }
   return nullptr;
   }

STATIC_FXN PathStrGenerator *MultiFileGrepFnmGenerator( bool retry_after_refreshing_taggedfiles=true ) { enum { DB=0 };
   auto rv( MultiFileGrepFnmGenerator_() );  // try to dereference any user-preconfigured "mf..." macros.
   if( !rv && retry_after_refreshing_taggedfiles ) {
      // Typical scenario: user HAS NOT preconfigured any of the "mf..." macros,
      // but has used e.g. universal-ctags[1] to generate a tags file.
      // [1] https://github.com/universal-ctags/ctags
      //
      // K supports using a tags file to guide multi-file grep by (if the first
      // attempt to dereference any preconfigured "mf..." macros fails to net
      // anything of value) attempting to switch to the "<tagged-files>" system
      // buffer (pseudofile) which is, when switched, autoloaded with the files
      // named by kind:file entries in the tags file.  The K handler for the
      // 'load "<tagged-files>" system buffer with content' event (function
      // read_tagged_files_from_tags_file) WILL if successful ALSO assign
      // "mffile:=<tagged-files>", so that subsequent calls to
      // MultiFileGrepFnmGenerator_ will return an (indirect) reference to the
      // "<tagged-files>" system buffer.
      fExecute( "arg \"<tagged-files>\" setfile", true );  // if no tags file was found, mffile will remain unset and...
      rv = MultiFileGrepFnmGenerator_();                   // ...MultiFileGrepFnmGenerator_ will (again) return nullptr.
      }
   return rv;
   }

#ifdef fn_mgl
bool ARG::mgl() {
   std::unique_ptr<PathStrGenerator>pGen( MultiFileGrepFnmGenerator( false ) );
   if( pGen ) {
      auto ix(0);
      Path::str_t pbuf;
      while( pGen.get()->VGetNextName( pbuf ) ) {
         DBG( "  [%d] = '%" PR_BSR "'", ix++, BSR(pbuf) );
         }
      }
   return true;
   }
#endif

// Note that this DOES NOT automatically recurse directories, it only visits the
// directories that are EXPLICITLY called out in macroName's content.  OTOH, if
// envvars occur in macroName's content, these are expanded, and each element of
// the expansion is searched.
bool ARG::mfgrep() {
   if( !SearchSpecifierOK( *this ) ) {
      return false;
      }
   MFGrepMatchHandler mh( g_pFBufSearchRslts );
   std::unique_ptr<FileSearcher> pSrchr( NewFileSearcher
      ( s_searchSpecifier->CanUseFastSearch() ? FileSearcher::fsFAST_STRING : FileSearcher::fsTABSAFE_STRING
      , mh.sm()
      , *s_searchSpecifier
      , mh
      ));
   if( !pSrchr ) {
      return false;
      }
   std::unique_ptr<PathStrGenerator>pGen( MultiFileGrepFnmGenerator() );
   if( !pGen ) {
      // ErrorDialogBeepf( "MultiFileGrepFnmGenerator -> nil && no tags" );
      return false;
      }
   else {                         0 && DBG( "%s using %" PR_BSR, __PRETTY_FUNCTION__, BSR(pGen.get()->srSrc()) );
      mh.InitLogFile( *(pSrchr.get()), pGen.get()->srSrc() );
      Path::str_t pbuf;
      while( pGen.get()->VGetNextName( pbuf ) ) {
         MFGrepProcessFile( pbuf.c_str(), pSrchr.get() );
         }
      }
   mh.ShowResults();
   return mh.VOverallRetval();
   }

#if USE_PCRE
STATIC_VAR CompiledRegex *  s_pSandR_CompiledSearchRegex;
STIL bool CheckRegExpReplacementString( CompiledRegex *, PCChar ) { return true; }
#endif

STATIC_FXN void MFReplaceProcessFile( PCChar filename, CharWalkerReplace *pMrcw ) {
   const auto pFBuf( FBOP::FindOrAddFBuf( filename ) );  0 && DBG( "d_dhdViewsOfFBUF.IsEmpty(%s)==%c", filename, pFBuf->ViewCount()==0?'t':'f' );
   const auto fWeCanGarbageCollectFBUF( !pFBuf->HasLines() );
   if( mf_RefreshFailedShowError( pFBuf ) ) {
      return;
      }
   ++pMrcw->d_iReplacementFileCandidates;
   const auto oldReplacementsMade( pMrcw->d_iReplacementsMade );
   Rect rgnSearch( pFBuf );
   CharWalkRectReplace( pFBuf, rgnSearch, rgnSearch.flMin, *pMrcw );
   if( oldReplacementsMade == pMrcw->d_iReplacementsMade ) {
      GarbageCollectFBUF( pFBuf, fWeCanGarbageCollectFBUF );
      }
   else {                                                0 && DBG( "%s CHANGED '%s'", __func__, pFBuf->Name() );
      ++pMrcw->d_iReplacementFiles;
      }
   }

// bool (*xform_string_fail)( std::string &inout );
//
// bool (*xform_string)( std::string &inout )
// class wrap_front_back {
//    const PCChar d_s;
// public:
//    wrap_front_back( PCChar ss ) : d_s( ss ) {}
//    void operator( std::string &inout ) const {
//       STATIC_CONST char s_b[] = "\\b";
//       inout.insert( 0, s_b );
//       inout.append( s_b );
//       }
//    };

#define REPLACE_STSEARCH_XFORM_LAMBDA 0
#if     REPLACE_STSEARCH_XFORM_LAMBDA
#define RSXL(  x )     x
#define RSXLC( x )  , x
#else
#define RSXL(  x )
#define RSXLC( x )
#endif

RSXL( template  <typename Lambda> )
STATIC_FXN bool GenericReplace_CollectInputs( bool fRegex, bool fInteractive, bool fMultiFileReplace RSXLC( Lambda transform_stSearch ) ) {
   STATIC_CONST char szReplace[] = "Replace string: "; // these two defined adjacently so ...
   STATIC_CONST char szSearch [] = "Search string:  "; // ... they are kept the same length
   DispDoPendingRefreshesIfNotInMacro();
   {
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( g_SnR_stSearch, szSearch, 0, nullptr, gts_DfltResponse+gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
   if( !pCmd || pCmd->IsFnCancel() || g_SnR_stSearch.empty() ) {
      return false;
      }
   }
   RSXL( transform_stSearch( g_SnR_stSearch ); )
   if( !SetNewSearchSpecifierOK( g_SnR_stSearch, fRegex ) ) {
      return false;
      }
   {
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( g_SnR_stReplacement, szReplace, 0, nullptr, gts_DfltResponse+gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
   if( !pCmd || pCmd->IsFnCancel() ) {
      return false;
      }
   }
   if( fMultiFileReplace && g_SnR_stReplacement.empty() && !ConIO::Confirm( "Empty replacement string, confirm: " ) ) {
      return false;
      }
#if USE_PCRE
   if(   s_searchSpecifier->IsRegex()
      && !CheckRegExpReplacementString( s_pSandR_CompiledSearchRegex, g_SnR_stReplacement.c_str() )
     ) {
      return ErrorDialogBeepf( "Invalid replacement pattern" );
      }
#endif
   return true;
   }

STATIC_FXN void DoMultiFileReplace( CharWalkerReplace &mrcw ) {
   const auto startingTopFbuf( g_CurFBuf() );
   std::unique_ptr<PathStrGenerator>pGen( MultiFileGrepFnmGenerator() );
   if( !pGen ) {
      ErrorDialogBeepf( "MultiFileGrepFnmGenerator -> nil" );
      }
   else {
      Path::str_t pbuf;
      while( pGen->VGetNextName( pbuf ) ) {  // VGetNextName returns false on user break (quit) via SetUserChoseEarlyCmdTerminate()
         MFReplaceProcessFile( pbuf.c_str(), &mrcw );
         }
      // Lua event handler GETFOCUS is called by PutFocusOn() and
      // l_hook_handler() calls lua_error() if ExecutionHaltRequested() is set
      if( USER_CHOSE_EARLY_CMD_TERMINATE == ExecutionHaltRequested() ) {
         ClrExecutionHaltRequest();
         }
      startingTopFbuf->PutFocusOn();
      Msg( "%d of %d occurrences replaced in %d of %d files%s"
            , mrcw.d_iReplacementsMade
                  , mrcw.d_iReplacementsPoss , mrcw.d_iReplacementFiles
                                                   , mrcw.d_iReplacementFileCandidates
                                                           , (mrcw.Interactive() && ExecutionHaltRequested()) ? " INTERRUPTED!" : ""
         );
      }
   }

RSXL( template <typename Lambda> )
STATIC_FXN bool GenericReplace( const ARG &arg, bool fInteractive, bool fMultiFileReplace RSXLC( Lambda transform_stSearch ) ) {
   if( !GenericReplace_CollectInputs( arg.d_cArg >= 2, fInteractive, fMultiFileReplace RSXLC( transform_stSearch ) ) ) {
      return false;
      }
   CharWalkerReplace mrcw( fInteractive, arg.d_fMeta ? !g_fReplaceCase : g_fReplaceCase, *s_searchSpecifier );
   if( fMultiFileReplace ) {
      DoMultiFileReplace( mrcw );
      }
   else {
      MainThreadPerfCounter pc;
      switch( arg.d_argType ) {
       break;default:
       break;case NOARG:     { Rect rn( true ); CharWalkRectReplace( g_CurFBuf(), rn, g_Cursor(), mrcw ); }
       break;case NULLARG:   { Rect rn( true ); CharWalkRectReplace( g_CurFBuf(), rn, g_Cursor(), mrcw ); }
       break;case LINEARG:   { Rect rn;
                                     rn.flMin.lin = arg.d_linearg.yMin;  rn.flMin.col = 0;
                               rn.flMax.lin = arg.d_linearg.yMax;  rn.flMax.col = COL_MAX;
                               CharWalkRectReplace( g_CurFBuf(), rn, rn.flMin, mrcw );
                             }
       break;case STREAMARG: if( arg.d_streamarg.flMin.lin == arg.d_streamarg.flMax.lin ) {
                                CharWalkRectReplace( g_CurFBuf(), arg.d_streamarg, arg.d_streamarg.flMin, mrcw );
                                }
                             else { Rect rn;
                                rn.flMin.lin = arg.d_streamarg.flMin.lin;      rn.flMin.col = 0;
                                rn.flMax.lin = arg.d_streamarg.flMax.lin - 1;  rn.flMax.col = COL_MAX;
                                CharWalkRectReplace( g_CurFBuf(), rn, arg.d_streamarg.flMin, mrcw );
                                rn.flMax.col = arg.d_streamarg.flMax.col;
                                rn.flMax.lin++;
                                rn.flMin.lin = rn.flMax.lin;
                                CharWalkRectReplace( g_CurFBuf(), rn, rn.flMin, mrcw );
                                }
       break;case BOXARG:    CharWalkRectReplace( g_CurFBuf(), arg.d_boxarg, arg.d_boxarg.flMin, mrcw );
       }
      if( fInteractive ) {
         Msg( "%d of %d occurrences replaced%s"
            , mrcw.d_iReplacementsMade
            , mrcw.d_iReplacementsPoss
            , (fInteractive && ExecutionHaltRequested()) ? " INTERRUPTED!" : ""
            );
         }
      else {
         Msg( "%d of %d occurrences replaced in %5.3f S; %" PR_SIZET " lines visited, %" PR_SIZET " CheckNext calls"
            , mrcw.d_iReplacementsMade
            , mrcw.d_iReplacementsPoss
            , pc.Capture()
            , mrcw.d_linesVisited
            , mrcw.d_CheckNextCallCount
            );
         }
      }

   // 20060526 klg commented out since if user hit 'Q' he probably wants to
   //              terminate any macro that this CMD may have been invoked within
   //
   // if( USER_CHOSE_EARLY_CMD_TERMINATE == ExecutionHaltRequested() )
   //    ClrExecutionHaltRequest(); // ExecutionHaltRequest is used in this code to implement the 'Q' response to interactive query
   return mrcw.d_iReplacementsMade != 0;
   }

bool ARG::mfreplace() { return GenericReplace( *this, true , true  RSXLC( [](std::string &inout) -> void {} ) ); }
bool ARG::qreplace()  { return GenericReplace( *this, true , false RSXLC( [](std::string &inout) -> void {} ) ); }
bool ARG::replace()   { return GenericReplace( *this, false, false RSXLC( [](std::string &inout) -> void {} ) ); }
bool ARG::mfrplcword() {
   const auto fInteractive( true );
   if( !GenericReplace_CollectInputs( false, fInteractive, true RSXLC( transform_stSearch ) ) ) {
      return false;
      }
   STATIC_CONST char s_b[] = "\\b";
   g_SnR_stSearch.insert( 0, s_b );
   g_SnR_stSearch.append( s_b );
   if( !SetNewSearchSpecifierOK( g_SnR_stSearch, true ) ) {
      return false;
      }
   CharWalkerReplace mrcw( fInteractive, true, *s_searchSpecifier );
   DoMultiFileReplace( mrcw );
   return mrcw.d_iReplacementsMade != 0;
   }

void FBOP::InsLineSorted_( PFBUF fb, bool descending, LINE ySkipLeading, stref src ) {
   const auto cmpSignMul( descending ? -1 : +1 );
   // find insertion point using binary search of RAW content (PeekRawLine)
   auto yMin( ySkipLeading );
   auto yMax( fb->LastLine() );
   while( yMin <= yMax ) {
      //                ( (yMax + yMin) / 2 );           // old overflow-susceptible version
      const auto cmpLine( yMin + ((yMax - yMin) / 2) );  // new overflow-proof version
      const auto rl( fb->PeekRawLine( cmpLine ) );
      auto rslt( cmpi( src, rl ) * cmpSignMul );
      if( 0 == rslt ) {
         rslt = cmp( src, rl ) * cmpSignMul;
         if( 0 == rslt ) {
            return; // drop DUPLICATES!
            }
         }
      if( rslt > 0 ) { yMin = cmpLine + 1; }
      if( rslt < 0 ) { yMax = cmpLine - 1; }
      }
   fb->InsLineRaw( yMin, src );  // note RAW insert
   }

STATIC_FXN void InsFnm( PFBUF pFbuf, PCChar fnm, const bool fSorted ) {
   auto pb( Path::UserName( fnm ) );
   if( fSorted ) { FBOP::InsLineSortedAscending( pFbuf, 0, pb ); } else { pFbuf->PutLastLineRaw( pb ); }
   }

int FBOP::ExpandWildcard( PFBUF fb, PCChar pszWildcardString, const bool fSorted ) { enum { ED=0 }; ED && DBG( "%s '%s'", __func__, pszWildcardString );
   auto rv(0);
   const PCChar pVbar( strrchr( pszWildcardString, '|' ) );
   if( pVbar ) {
      // the wildcard (or plain searchkey) to be applied recursively has been
      // merged by earlier processing into the full path based on the cwd, as
      // needed.  We need to recover the searchkey (as a separate string to
      // supply to the WildcardFilenameGenerator constructor)
      //
      pathbuf wcBuf, dirBuf;
      const auto pStart( Path::StrToPrevPathSepOrNull( pszWildcardString, pVbar ) );
      if( pStart ) {
         bcpy( wcBuf , se2sr( pStart+1         , pVbar  ) );
         bcpy( dirBuf, se2sr( pszWildcardString, pStart ) );
         }
      else {
         bcpy( wcBuf , se2sr( pszWildcardString, pVbar ) );
         bcpy( dirBuf, "." DIRSEP_STR );
         }                                                           ED && DBG( "wcBuf='%s'" , wcBuf  );
                                                                     ED && DBG( "dirBuf='%s'", dirBuf );
      Path::str_t pbuf, fbuf;
      DirListGenerator dlg( dirBuf );
      while( dlg.VGetNextName( pbuf ) ) {                            ED && DBG( "pbuf='%s'", pbuf.c_str() );
         WildcardFilenameGenerator wcg( __func__, FmtStr<_MAX_PATH>( "%s" DIRSEP_STR "%s", pbuf.c_str(), wcBuf ), ONLY_FILES );
         fbuf.clear();
         while( wcg.VGetNextName( fbuf ) ) {
            InsFnm( fb, fbuf.c_str(), fSorted );
            ++rv;
            }
         }
      }
   else {
      CfxFilenameGenerator wcg( __func__, pszWildcardString, FILES_AND_DIRS );
      Path::str_t fbuf;
      while( wcg.VGetNextName( fbuf ) ) {
         const auto chars( fbuf.length() );  ED && DBG( "wcg=%s", fbuf.c_str() );
         if( Path::IsDot( fbuf ) ) {
            continue; // drop the meaningless "." entry:
            }
         InsFnm( fb, fbuf.c_str(), fSorted );
         ++rv;
         }
      }
   return rv;
   }

//===============================================

void SearchSpecifier::CaseUpdt() {
#if USE_PCRE
   if( d_fRegex && (d_fRegexCase != g_fCase) ) {
      d_fRegexCase = g_fCase;
      // recompile
      d_re = Regex_Delete0( d_re );
      d_re = Regex_Compile( d_rawStr.c_str(), d_fRegexCase );
      }
#endif
   }

SearchSpecifier::SearchSpecifier( stref rawSrc, bool fRegex ) : d_fRegex(fRegex) {
   d_fNegateMatch = rawSrc.starts_with( "!!" );
   if( d_fNegateMatch ) {
      rawSrc.remove_prefix( 2 );
      }
   const auto fPromotingPlainSearchToRegex( !d_fRegex && g_fPcreAlways );
   d_rawStr.assign( fPromotingPlainSearchToRegex ? "\\Q" : "" );
   d_fRegex = d_fRegex || fPromotingPlainSearchToRegex;
   d_rawStr.append( rawSrc );
   d_fCanUseFastSearch = !d_fRegex && std::string::npos==d_rawStr.find( ' ' ) && std::string::npos==d_rawStr.find( HTAB );
#if USE_PCRE
   d_fRegexCase = g_fCase;
   d_re = d_fRegex ? Regex_Compile( d_rawStr.c_str(), d_fRegexCase ) : nullptr;
#endif
   }

SearchSpecifier::~SearchSpecifier() {
#if USE_PCRE
   d_re = Regex_Delete0( d_re );
#endif
   }

void SearchSpecifier::Dbgf( PCChar tag ) const {
  #if 1
   DBG( "SearchSpecifier %s: cs=%d, rex=%d, raw=%" PR_BSR "'"
      , tag
#if USE_PCRE
      , d_fRegexCase
      , d_re != nullptr
#else
      , g_fCase
      , false
      , false
#endif
      , BSR(d_rawStr)
      );
  #endif
   }

//===============================================

FileSearcher::FileSearcher( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : d_sm   ( sm )
   , d_ss   ( ss )
   , d_mh   ( mh )
   , d_pFBuf( nullptr  )
   {
   }

void FileSearcher::Dbgf() const {
   VS_(
      DBG( "%p d_pFBuf=%p %c cs=%s"
         , this
         , d_pFBuf
         , d_sm.d_fSearchForward ? 'F' : 'f'
         , g_fCase ? "sen" : "ign"
         );
      d_ss.Dbgf( "FileSearcher" );
      )
   }

//===============================================

FileSearcherString::FileSearcherString( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : FileSearcher( sm, ss, mh )
   , d_searchKey( ss.SrchStr() )
   , d_pfxStrnstr( g_fCase ? strnstr : strnstri )
   {
   }

FileSearcher::FindStrRslt FileSearcherString::VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) {
   stref offset_eaten( src ); offset_eaten.remove_prefix( src_offset );
   const auto ixMatch( d_pfxStrnstr( offset_eaten, d_searchKey ) );
   if( ixMatch == stref::npos ) {
      return 0;
      }
   return offset_eaten.substr( ixMatch, d_searchKey.length() ); // MATCH!
   }

//===============================================

#if USE_PCRE
FileSearcherRegex::FileSearcherRegex( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : FileSearcher( sm, ss, mh )
   {
   Assert( d_ss.IsRegex() );
   }

FileSearcher::FindStrRslt FileSearcherRegex::VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) {
   const auto rv( Regex_Match( d_ss.re(), d_captures, src, src_offset, pcre_exec_options ) );
   if( rv > 0 && d_captures[0].valid() ) {
      const auto srMatch( d_captures[0].value() );  0 && DBG( "RegEx:->MATCH=(%" PR_PTRDIFFT " L %" PR_SIZET ")='%" PR_BSR "'", srMatch.data() - src.data(), srMatch.length(), BSR(srMatch) );
      return srMatch;
      }                                             0 && DBG( "RegEx:->NO MATCH" );
   return FileSearcher::FindStrRslt( 0 );
   }
#endif

//===============================================

FileSearcherFast::FileSearcherFast( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : FileSearcher( sm, ss, mh )
   , d_pfxStrnstr( g_fCase ? strnstr : strnstri )
   {
   stref pS( ss.SrchStr() );
   // BUGBUG deprecate for now 20150101 KG
   const char AltSepChar
      (
         (      pS.length() >= 2
          &&    pS[0]=='!'
          &&
             (  pS[1]==','
             || pS[1]=='|'
             || pS[1]=='.'
             )
         ) ? pS[1] : chNUL
      );
   auto fNdAppendTrailingAltSepChar( false );
   if( AltSepChar ) {
      pS.remove_prefix( 2 );
      // IF ALTERNATION
      //    arg "!,AltSepChar,fNdAppendTrailingAltSepChar" grep
      //    arg arg "(AltSepChar|fNdAppendTrailingAltSepChar)" grep
      // BEING USED, LAST CHAR MUST AltSepChar!
      // Append if necessary
      if( pS.back() != AltSepChar ) {
         fNdAppendTrailingAltSepChar = true;
         }
      }
   d_searchKey.assign( pS );
   if( fNdAppendTrailingAltSepChar ) {
      d_searchKey += AltSepChar;
      }
   // gateway to alternation (logical OR) in grep/TEXTARG: "^![,.|]"
   // replace the user's chosen separator with the separator that
   // FileSearcherFast::FindMatches requires
                                                     0 && DBG( "srchStrLen=%" PR_BSRSIZET ": '%" PR_BSR "'", d_searchKey.length(), BSR(d_searchKey) );
   {
   auto ix(0);
   stref rk( d_searchKey );
   auto it( rk.cbegin() ); // want it beyond for loop
   auto pStart(it);
   for( ; it != rk.cend() ; ++it ) {
      if( AltSepChar == *it ) {                      0 && DBG( " '%s'", it+1 );
          d_pNeedles.emplace_back( pStart, std::distance( pStart, it ) );
          pStart = it + 1;
          }
      }
   d_pNeedles.emplace_back( pStart, std::distance( pStart, it ) );
                                               0 && DBG( "needleCount=%" PR_BSRSIZET, d_pNeedles.size() );
   }
   // for( int ix(0) ; ix < d_needleCount ; ++ix ) {
   //    DBG( "NeedleLen[%d]=%d", ix, d_pNeedleLens[ ix ] );
   //    }
   }

//
// FileSearcherFast::FindMatches supports logical alternation (OR):
// pszSrchStr is comprised of multiple \0-separated 'subneedles'.  The extent of
// subneedles within pszSrchStr is bounded by srchLen, which is why srchLen must
// be specified explicitly (srchLen == Strlen( pszSrchStr ) iff there is only one
// subneedle within pszSrchStr (the simple/standard search case)).
//
// <041019> klg added logical alternation support.  d_searchKey points to a
//              buffer having envp-like content: a sequence of multiple
//              \0-terminated strings.  d_searchKeyStrlen indicates the end of
//              the buffer, and NeedleLen( lix ) is the strlen of a needle[ix].
//
//              EX: d_searchKey = "this\0is\0a\0test\0", d_searchKeyStrlen = 15
//                  NeedleLen(0) == 4
//                  NeedleLen(1) == 2
//                  NeedleLen(2) == 1
//                  NeedleLen(3) == 4
//                  NeedleLen(4+)== 0
//
void FileSearcherFast::VFindMatches_() {
   const auto tw( d_pFBuf->TabWidth() );
   for( auto curPt(d_start) ; curPt < d_end && !ExecutionHaltRequested() ; ++curPt.lin, curPt.col = 0 ) {
      const auto rl( d_pFBuf->PeekRawLine( curPt.lin ) );
      if( !IsStringBlank( rl ) ) {
SEARCH_REMAINDER_OF_LINE_AGAIN:
         auto lix(0);
         for( const auto &needleSr : d_pNeedles ) {  // if( ExecutionHaltRequested() ) break;
            if( needleSr.length() <= rl.length() - curPt.col ) {
               const auto relIxMatch( d_pfxStrnstr( stref( rl.data() + curPt.col, rl.length() - curPt.col ), needleSr ) );
               if( relIxMatch != stref::npos ) {
                  const auto ixMatch( curPt.col + relIxMatch );
                  0 && DBG( "%s L=%d Needle[%d]=%" PR_BSR " ixM=%" PR_BSRSIZET, d_pFBuf->Name(), curPt.lin, lix, BSR(needleSr), ixMatch ); ++lix;
                  // To prevent the highlight from being misaligned,
                  // FoundMatchContinueSearching needs to be given a tab-corrected
                  // colMatchStart value.  d_searchKey.length() is perfectly
                  // adequate/correct because we won't be using FileSearcherFast if
                  // _the key_ contains spaces or tabs
                  {
                  const auto colMatchStart( ColOfFreeIdx ( tw, rl, ixMatch ) );
                  Point matchPt( curPt.lin, colMatchStart );
                  if( !d_mh.FoundMatchContinueSearching( d_pFBuf, matchPt, d_searchKey.length(), d_captures ) ) {
                     return;
                     }
                  }
                  curPt.col = ixMatch + d_searchKey.length();
                  goto SEARCH_REMAINDER_OF_LINE_AGAIN;
                  }
               }
            }
         }
      }
   }

// these stubs are required by the compiler, but will NEVER be called since
// FileSearcherFast::VFindMatches_ REPLACES FileSearcher::VFindMatches_, and
// FileSearcherFast::VFindMatches_ DOES NOT CALL OTHER CLASS METHODS
//
FileSearcher::FindStrRslt  FileSearcherFast::VFindStr_( stref src, sridx src_offset, int pcre_exec_options ) { Assert( 0 != 0 ); return 0; }

//===============================================

void FileSearcher::VFindMatches_() { enum { DB=0 };  VS_( DBG( "%csearch: START  y=%d, x=%d", d_sm.d_fSearchForward?'+':'-', d_start.lin, d_start.col ); )
   const auto tw( d_pFBuf->TabWidth() );
   if( d_sm.d_fSearchForward ) {                     VS_( DBG( "+search: START  y=%d, x=%d", d_start.lin, d_start.col ); )
      for( auto curPt(d_start) ; curPt < d_end && !ExecutionHaltRequested() ; ++curPt.lin, curPt.col = 0 ) {
         //***** Search A LINE:
         const auto rl( d_pFBuf->PeekRawLine( curPt.lin ) );
         IdxCol_cached conv( tw, rl );
         auto iC( conv.c2ci( curPt.col ) );
         if( conv.i2c( iC ) != curPt.col ) { // curPt.col is in a tab-spring, which means (a) curPt.col > 0, and (b) iC indexes a char outside (to the left of) the replace region[1]
            ++iC;                            // make iC index the first char in replace region  [1] but BUGBUG this fxn is not used by replace!
            }
         // find all matches on this line
         for( ; iC <= rl.length() /* <= so empty line can match Regex */ ; ++iC ) {
            const auto srMatch( VFindStr_( rl, iC, 0 ) );
            if( srMatch.noMatch() ) {
               break; // no matches on this line!
               }
            //*****  HOUSTON, WE HAVE A MATCH  *****
            const auto matchSr( srMatch.sr() );                                                               DB && DBG( "curPt.col0=%d", curPt.col );
            curPt.col  =          conv.i2c( (matchSr.data() - rl.data())                    )              ;  DB && DBG( "curPt.col1=%d", curPt.col );
            const auto matchCols( conv.i2c( (matchSr.data() - rl.data()) + matchSr.length() ) - curPt.col );
            if( !d_mh.FoundMatchContinueSearching( d_pFBuf, curPt, matchCols, d_captures ) ) {
               return;
               }
            iC = conv.c2ci( curPt.col );
            }
         }
      }
   else { /* search backwards (only msearch uses this; ++complex, ++++overhead) */   VS_( DBG( "-search: START  y=%d, x=%d", d_start.lin, d_start.col ); )
      for( auto curPt(d_start) ; curPt > d_end && !ExecutionHaltRequested() ; --curPt.lin, curPt.col = COL_MAX ) {
         if( curPt.col < 0 ) {
            continue;
            }
         const auto rl( d_pFBuf->PeekRawLine( curPt.lin ) );
         const IdxCol_nocache conv( tw, rl );
         auto iC( conv.c2ci( curPt.col ) );                       VS_( DBG( "-search: newline: x=%d,y=%d=>[%d/%d]='%" PR_BSR "'", curPt.col, curPt.lin, iC, rl.length(), BSR(rl) ); )
         if( iC < rl.length() ) { // if curPt.col is in middle of line...
            ++iC;                     // ... nd to incr to get correct maxCharsToSearch
            }
         const auto maxCharsToSearch( std::min( iC, rl.length() ) );
         const stref haystack( rl.data(), maxCharsToSearch );   VS_( DBG( "-search: HAYSTACK='%" PR_BSR "'", BSR(haystack) ); )
         // works _unless_ cursor is at EOL when 'arg arg "$" msearch'; in this
         // case, it keeps finding the EOL under the cursor (doesn't move to
         // prev one)
       #if USE_PCRE
         #define  SET_HaystackHas(startOfs)  (startOfs+maxCharsToSearch == rl.length() ? 0 : PCRE_NOTBOL)
       #else
         #define  SET_HaystackHas(startOfs)  (0)
       #endif
         const auto srMatch( VFindStr_( haystack, 0, SET_HaystackHas(0) ) );
         if( !srMatch.noMatch() ) {
            COL goodMatchChars( srMatch.sr().length() );
            auto iGoodMatch( srMatch.sr().data() - haystack.data() );
            /* line contains _A_ match? */  VS_( { stref match( haystack.substr( iGoodMatch, goodMatchChars ) ); DBG( "-search: LMATCH y=%d (%d L %d)='%" PR_BSR "'", curPt.lin, iGoodMatch, goodMatchChars, BSR(match) ); } )
            // find the rightmost match by repeatedly searching (left->right) until search fails, using the last good match
            while( iGoodMatch < maxCharsToSearch ) {
               const auto startIdx( iGoodMatch + 1 );   VS_( { auto newHaystack( haystack ); newHaystack.remove_prefix( startIdx ); DBG( "-search: iAYSTACK=%" PR_SIZET " '%" PR_BSR "'", startIdx, BSR(newHaystack) ); } )
               const auto srNextMatch( VFindStr_( haystack, startIdx, SET_HaystackHas(startIdx) ) );
               if( srNextMatch.noMatch() ) {
                  break;
                  }
               const auto iNextMatch( srNextMatch.sr().data() - haystack.data() );
               COL nextMatchChars( srNextMatch.sr().length() );
               iGoodMatch     = iNextMatch;
               goodMatchChars = nextMatchChars;         VS_( { stref match( haystack.substr( iGoodMatch, goodMatchChars ) ); DBG( "-search: +MATCH y=%d (%d L %d)='%" PR_BSR "'", curPt.lin, iGoodMatch, goodMatchChars, BSR(match) ); } )
               }
            curPt.col  =          conv.i2c( iGoodMatch                              )              ;
            const auto matchCols( conv.i2c( goodMatchChars + conv.c2ci( curPt.col ) ) - curPt.col );
                                                        VS_( DBG( "-search: #MATCH y=%d (%d L %d)=>COL(%d L %d)", curPt.lin, iGoodMatch, goodMatchChars, curPt.col, matchCols ); )
            if( !d_mh.FoundMatchContinueSearching( d_pFBuf, curPt, matchCols, d_captures ) ) {
               return;
               }
            }
         }
      }
   }


// not my proudest moment, but replicating the code below everywhere would be immoral!

STATIC_FXN FileSearcher *NewFileSearcher(
     FileSearcher::StringSearchVariant  type
   , const SearchScanMode              &sm
   , const SearchSpecifier             &ss
   , FileSearchMatchHandler            &mh
   )
   {  VS_( DBG( "NewFileSearcher enter" ); )
   FileSearcher *rv;
#if USE_PCRE
   if( ss.IsRegex() ) {
      if( ss.HasError() ) {
         return nullptr;
         }
      rv = new FileSearcherRegex( sm, ss, mh );            VS_( DBG( "  FileSearcherRegex %p", rv ); )
      }
   else
#endif
      {
      switch( type ) {
         break; default:                             Assert( 0 != 0 ); return nullptr;
         break; case FileSearcher::fsTABSAFE_STRING: rv = new FileSearcherString( sm, ss, mh );   VS_( DBG( "  FileSearcherString %p", rv ); )
         break; case FileSearcher::fsFAST_STRING:    rv = new FileSearcherFast( sm, ss, mh );     VS_( DBG( "  FileSearcherFast %p", rv ); )
         }
      }                                              VS_( rv->Dbgf(); )
   return rv;
   }

STATIC_FXN bool FindNextMatch( const ARG &arg, const SearchScanMode &sm ) {
   if( !SearchSpecifierOK( arg ) ) {
      return false;
      }
   Point curPt( g_CurView()->Cursor() );
   if(      &smBackwd == &sm ) { curPt.DecrOk(); }
   else if( &smFwd    == &sm ) { curPt.IncrOk(); }
   else                        { Assert( !"invalid sm value" ); }
   std::unique_ptr<FindPrevNextMatchHandler> mh( new FindPrevNextMatchHandler( sm.d_fSearchForward, s_searchSpecifier->IsRegex(), s_searchSpecifier->SrchStr() ) );
   std::unique_ptr<FileSearcher> pSrchr( NewFileSearcher( FileSearcher::fsTABSAFE_STRING, sm, *s_searchSpecifier, *mh ) );
   if( !pSrchr ) {
      return false;
      }
   pSrchr->SetInputFile();
   pSrchr->SetBoundsToEnd( curPt );
   pSrchr->FindMatches();
   mh->ShowResults();
   const auto rv( mh->VOverallRetval() );
   return rv;
   }

bool ARG::msearch()   { return FindNextMatch( *this, smBackwd ); }
bool ARG::psearch()   { return FindNextMatch( *this, smFwd    ); }

//-------------------------- pbal

class CharWalkerPBal : public CharWalker_ {
   const bool d_fFwd;
   const stref d_opens, d_closes;
   char       d_stack[100];
   int        d_stackIx;
   bool       d_fClosingMatchFound;
   Point      d_closingPt;
   void Push( char ch ) { 0 && DBG("Push[%3d]'%c'",d_stackIx,ch);
                          d_stack[ d_stackIx++ ] = ch; }
   char Pop()           { const char rv(d_stack[ --d_stackIx ]); 0 && DBG("Pop [%3d]'%c'",d_stackIx,rv); return rv; }
public:
   CharWalkerPBal( bool fFwd, char chStart )
      : d_fFwd( fFwd )
      , d_opens ( fFwd ? g_delims : g_delimMirrors )
      , d_closes( fFwd ? g_delimMirrors: g_delims  )
      , d_stackIx( 0 )
      , d_fClosingMatchFound( false )
      { Push( chStart ); }

   CheckNextRetval VCheckNext( stref rl, const Point &curPtIx, COL tabWidth ) override;
   bool  ClosingMatchFound() const { return d_fClosingMatchFound; }
   Point ClosingPt()         const { return d_closingPt;     }
   };

CheckNextRetval CharWalkerPBal::VCheckNext( stref rl, const Point &curPtIx, COL tabWidth ) {
   const auto ch( rl[curPtIx.col] );
   const auto iOpens( d_opens.find( ch ) );
   if( iOpens != eosr ) {
      if( d_stackIx >= ELEMENTS(d_stack)-1 ) { 0 && DBG( "%cbal STACK OVERFLOW at X=%d, Y=%d", d_fFwd ? '+' : '-', curPtIx.col+1, curPtIx.lin+1 );
         return STOP_SEARCH;
         }                          0 && DBG( "%cbal [%d] d_stackIx PUSH '%c' at X=%d, Y=%d", d_fFwd ? '+' : '-', d_stackIx, ch, curPtIx.col+1, curPtIx.lin+1 );
      Push( ch );
      return CONTINUE_SEARCH;
      }
   const auto iCloses( d_closes.find( ch ) );
   if( iCloses != eosr ) {
      const auto closes( d_opens[iCloses] );
      const auto shouldClose( Pop() );
      if( shouldClose != closes ) { 0 && DBG( "%cbal [%d] MISMATCH is '%c' != s/b '%c' at X=%d, Y=%d", d_fFwd ? '+' : '-', d_stackIx, ch, shouldClose, curPtIx.col+1, curPtIx.lin+1 );
         return STOP_SEARCH;
         }
      if( d_stackIx > 0 ) {         0 && DBG( "%cbal [%d] MATCH '%c' at X=%d, Y=%d", d_fFwd ? '+' : '-', d_stackIx, ch, curPtIx.col+1, curPtIx.lin+1 );
         return CONTINUE_SEARCH;
         }
      d_fClosingMatchFound = true;  0 && DBG( "%cbal CLOSURE at X=%d, Y=%d", d_fFwd ? '+' : '-', curPtIx.col+1, curPtIx.lin+1 );
      d_closingPt.lin = curPtIx.lin;
      d_closingPt.col = ColOfFreeIdx( tabWidth, rl, curPtIx.col );
      return STOP_SEARCH;
      }
   return CONTINUE_SEARCH;
   }

char CharAtCol( COL tabWidth, stref content, const COL colTgt ) {
   const auto ix( FreeIdxOfCol( tabWidth, content, colTgt ) );
   return ix < content.length() ? content[ ix ] : chNUL;
   }

char View::CharUnderCursor() {
   return CharAtCol( g_CurFBuf()->TabWidth(), g_CurFBuf()->PeekRawLine( Cursor().lin ), Cursor().col );
   }

bool View::PBalFindMatching( bool fVisibleOnly, Point *pPt ) {
   const auto startCh( CharUnderCursor() );
   if( !startCh ) { return false; }
                                              0 && DBG( "%s: %c in '%s'?", __func__, startCh, g_delims       );
                                              0 && DBG( "%s: %c in '%s'?", __func__, startCh, g_delimMirrors );
   bool fSearchFwd;
   if(      strchr( g_delims      , startCh ) ) { fSearchFwd = true;  }
   else if( strchr( g_delimMirrors, startCh ) ) { fSearchFwd = false; }
   else return false;
   CharWalkerPBal chSrchr( fSearchFwd, startCh );
   CharWalkToEnd( fSearchFwd, fVisibleOnly, FBuf(), Cursor(), chSrchr );
   if( chSrchr.ClosingMatchFound() && pPt ) {
      *pPt = chSrchr.ClosingPt();
      }
   return chSrchr.ClosingMatchFound();
   }

bool ARG::balch() {
   const FBufLocnNow cp;
   PCV;
   Point pt;
   if( pcv->PBalFindMatching( false, &pt ) ) {
       pcv->MoveCursor( pt.lin, pt.col );
       }
   return cp.Moved();
   }

bool View::next_balln( LINE yStart, bool fStopOnElse ) {
   const auto pFbuf( FBuf() );
   auto nest(0);
   for( auto iy(yStart + 1) ; iy < pFbuf->LineCount() ; ++iy ) {
      auto fStop(false);
      switch( FBOP::IsCppConditional( pFbuf, iy ) ) {
         break;case cppcNone :
         break;case cppcIf   : ++nest;
         break;case cppcElif : if( 0 == nest   && fStopOnElse ) { fStop = true; }
         break;case cppcElse : if( 0 == nest   && fStopOnElse ) { fStop = true; }
         break;case cppcEnd  : if( 0 == nest-- )                { fStop = true; }
         }
      if( fStop ) {
         MoveCursor( iy, 0 );
         return true;
         }
      }
   return false;
   }

bool View::prev_balln( LINE yStart, bool fStopOnElse ) {
   const auto pFbuf( FBuf() );
   auto nest(0);
   for( auto iy(yStart - 1) ; iy >= 0 ; --iy ) {
      auto fStop(false);
      switch( FBOP::IsCppConditional( pFbuf, iy ) ) {
         break;case cppcNone :
         break;case cppcEnd  : ++nest;
         break;case cppcElif : if( 0 == nest   && fStopOnElse ) { fStop = true; }
         break;case cppcElse : if( 0 == nest   && fStopOnElse ) { fStop = true; }
         break;case cppcIf   : if( 0 == nest-- )                { fStop = true; }
         }
      if( fStop ) {
         MoveCursor( iy, 0 );
         return true;
         }
      }
   return false;
   }

bool ARG::balln() {
   const FBufLocnNow cp;
   PCV;
   const auto pFbuf( g_CurFBuf() );
   switch( FBOP::IsCppConditional( pFbuf, d_noarg.cursor.lin ) ) {
      break;case cppcNone : pcv->prev_balln( d_noarg.cursor.lin, true  );
      break;case cppcIf   : pcv->next_balln( d_noarg.cursor.lin, true  );
      break;case cppcElif : pcv->next_balln( d_noarg.cursor.lin, true  );
      break;case cppcElse : pcv->next_balln( d_noarg.cursor.lin, true  );
      break;case cppcEnd  : pcv->prev_balln( d_noarg.cursor.lin, false );
      }
   return cp.Moved();
   }

//-------------------------- pword/mword

class CharWalkerPMWord : public CharWalker_ {
   bool d_fMeta;
public:
   CharWalkerPMWord( bool fPMWordSearchMeta ) : d_fMeta( fPMWordSearchMeta ) {}
   CheckNextRetval VCheckNext( stref rl, const Point &curPtIx, COL tabWidth ) override;
   };

STATIC_FXN bool firstCharOfWord( stref rl, COL ix ) {
   // extreme hack: detect various leading-punctuation-heavy path edges:
   //   "/[A-za-z]" [1], ".[\\/]", "..[\\/]" as first char of word
   const auto preblank( ix == 0 || isblank( rl[ix-1] ) );
   return (DIRSEP_CH=='/'
           && '/' == rl[ix] && preblank && (ix+1 < rl.length() && isalpha( rl[ix+1] ))  // [1] NB: ASSUMES that unix rootdirnm SHALL begin with ALPHA!!!
          )
       || (   '.' == rl[ix] && preblank && (   (ix+1 < rl.length()                    && Path::IsDirSepCh( rl[ix+1] ))
                                            || (ix+2 < rl.length() && '.' == rl[ix+1] && Path::IsDirSepCh( rl[ix+2] ))
                                           )
          );
   }

CheckNextRetval CharWalkerPMWord::VCheckNext( stref rl, const Point &curPtIx, COL tabWidth ) {
   if( !d_fMeta ) {  // rtn true iff curPtIx.col is FIRST CHAR OF WORD
      if( firstCharOfWord( rl, curPtIx.col ) ) {
         goto MOV_TO_CURPT;       // firstCharOfWord hack PART A: leading '/' in "/usr/bin" IS the first char of a word
         }
      if( !isWordChar( rl[curPtIx.col] ) || (curPtIx.col > 0 && isWordChar( rl[curPtIx.col-1] )) ) {
         return CONTINUE_SEARCH;
         }
      if( curPtIx.col > 0 && firstCharOfWord( rl, curPtIx.col-1 ) ) {
         return CONTINUE_SEARCH;  // firstCharOfWord hack PART B: 'u' in "/usr/bin" IS NOT the first char of a word
         }
MOV_TO_CURPT:
      g_CurView()->MoveCursor( Point( curPtIx.lin, ColOfFreeIdx( tabWidth, rl, curPtIx.col ) ) );
      return STOP_SEARCH;
      }
   else { // rtn true iff curPt.col is LAST CHAR OF WORD
      if( curPtIx.col <= 0 )                 { return CONTINUE_SEARCH; }
      if( !isWordChar( rl[curPtIx.col-1] ) ) { return CONTINUE_SEARCH; }
      if( !isWordChar( rl[curPtIx.col  ] ) ) { goto MOV_TO_CURPT; }
      if( curPtIx.col != rl.length()-1 )     { return CONTINUE_SEARCH; }
      g_CurView()->MoveCursor( Point( curPtIx.lin, ColOfFreeIdx( tabWidth, rl, curPtIx.col )+1 ) );
      return STOP_SEARCH;
      }
   }

STATIC_FXN bool PMword( bool fSearchFwd, bool fMeta ) {
   const FBufLocnNow cp;
   CharWalkerPMWord chSrchr( fMeta );
   CharWalkToEnd( fSearchFwd, false, g_CurFBuf(), g_CurView()->Cursor(), chSrchr );
   return cp.Moved();
   }

bool ARG::pword() { return PMword( true , (Get_g_ArgCount() > 0) ? !d_fMeta : d_fMeta ); }
bool ARG::mword() { return PMword( false,                                     d_fMeta ); }

//********************************************************************************************
//
// Grep variations
//
// A.  grep the current non-<grep>-file for a literal search string
//     TARGET=curfile
//     KEY   =arg
//     DEST  =next<grep>
//     AuxHdr=*none*
//
// B.  grep the current     <grep>-file for a literal search string
//     TARGET=cur-<grep>-file
//     KEY   =arg
//     DEST  =next<grep>
//     AuxHdr=cur-<grep>-file.auxhdr
//
// C.  grep the file which was the TARGET of the current <grep>-file for a literal search string
//     TARGET=cur-<grep>-file.targetfile
//     KEY   =arg
//     DEST  =next<grep>
//     AuxHdr=*none*
//
// D.  grep the file which was the TARGET of the current <grep>-file for a series of search strings found in the current <grep>-file
//     TARGET=cur-<grep>-file.targetfile
//     KEY   =cur-<grep>-file.non-hdr-lines
//     DEST  =next<grep>
//     AuxHdr=*none*
//

class CGrepper {
private:
   enum { ED=0 };
   const LINE        d_InfLines;
   std::vector<bool> d_MatchingLines;
   PFBUF             d_SrchFile;
   const LINE        d_MetaLineCount;
   MainThreadPerfCounter d_pc;
                 // if !d_fFindAllNegate, do ADDITIVE    search: look for and ADD lines that match
                 // if  d_fFindAllNegate, do SUBTRACTIVE search: look for and RMV lines that match
   const bool    d_fFindAllNegate;
public:
   CGrepper( PFBUF srchfile, LINE MetaLineCount, bool fNegate )
        // ORDER OF FOLLOWING CLAUSES DOES NOT CAUSE THE ORDERING OF THEIR EXECUTION!!!
      : d_InfLines( srchfile->LineCount() )
      , d_MatchingLines( srchfile->LineCount(), fNegate )
      , d_SrchFile( srchfile )
      , d_MetaLineCount( MetaLineCount )
      , d_fFindAllNegate( fNegate )
      {
      DBG( "CGrepper: d_SrchFile='%s', d_MetaLineCount=%i, srchLines=%d", d_SrchFile->Name(), d_MetaLineCount, d_InfLines );
      }
   ~CGrepper() {}
   void FindAllMatches();
   void LineMatches( LINE line ) { d_MatchingLines[ line ] = !d_fFindAllNegate; }
   // to write results
   LINE WriteOutput( PCChar thisMetaLine, PCChar origSrchfnm );
private:
   void RmvLine( LINE line ) { d_MatchingLines[ line ] = false; }
   CGrepper & operator=( const CGrepper &rhs ); // so assignment op can't be used...
   };

//****************************

class CGrepperMatchHandler : public FileSearchMatchHandler {
   NO_ASGN_OPR(CGrepperMatchHandler);
   CGrepper &d_cg;
public:
   CGrepperMatchHandler( CGrepper &cg ) : d_cg( cg ) {}
   bool VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) override;
   STATIC_CONST SearchScanMode &sm() { return smFwd; }
   };

bool CGrepperMatchHandler::VMatchActionTaken( PFBUF pFBuf, const Point &cur, COL MatchCols, const RegexMatchCaptures &captures ) {
   d_cg.LineMatches( cur.lin );
   return true;  // "action" taken!
   }

void CGrepper::FindAllMatches() {
   CGrepperMatchHandler mh( *this );
   std::unique_ptr<FileSearcher> pSrchr( NewFileSearcher( FileSearcher::fsFAST_STRING, mh.sm(), *s_searchSpecifier, mh ) );
   if( pSrchr ) {
       pSrchr->SetInputFile( d_SrchFile );
       pSrchr->SetBoundsToEnd( Point( d_MetaLineCount, 0 ) );
       pSrchr->FindMatches();
       }
   }

LINE CGrepper::WriteOutput
   ( PCChar thisMetaLine // MetaLine (string) to be concatenated for this search
   , PCChar origSrchfnm  // if file searched THIS TIME is not file which line#s refer to
   )
   { enum { DB=0 };
   std::string sbuf( d_SrchFile->Namestr() );                            DB && DBG( " ->%s|", sbuf.c_str() );
   if( LuaCtxt_Edit::from_C_lookup_glock( sbuf ) && !sbuf.empty() ) {
      const auto gbnm( sbuf.c_str() );                                   DB && DBG( "LuaCtxt_Edit::from_C_lookup_glock ->%s|", gbnm );
      const auto outfile( OpenFileNotDir_NoCreate( gbnm ) );
      if( !outfile )                                                    { Msg(    "nonexistent buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      Path::str_t GrepFBufname; int grepHdrLines;
      if( !(FBOP::IsGrepBuf( GrepFBufname, &grepHdrLines, outfile ) ) ) { Msg(   "non-grep-buf buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      if( !d_SrchFile->NameMatch( GrepFBufname ) )                      { Msg( "wrong haystack buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      auto numberedMatches(0);
      for( auto iy(0); iy < d_InfLines; ++iy ) {
         if( d_MatchingLines[iy] ) {
            ++numberedMatches;
            }
         }
      {
      SprintfBuf LastMetaLine( "%s %d %s", outfile->Name(), numberedMatches, thisMetaLine );
      outfile->InsLineRaw( grepHdrLines, LastMetaLine.c_str() );
      }
      const auto lwidth( uint_log_10( d_InfLines ) );
      for( auto iy(0); iy < d_InfLines; ++iy ) {
         if( d_MatchingLines[iy] ) {
            sbuf.assign( FmtStr<20>( "%*d  ", lwidth, iy + 1 ) );
            const auto rl( d_SrchFile->PeekRawLine( iy ) );
            sbuf.append( rl );
            FBOP::InsLineSortedAscending( outfile, grepHdrLines, sbuf );
            }
         }
      outfile->PutFocusOn();
      Msg( "%d lines %s", numberedMatches, "added" );
      return numberedMatches;
      }
   const auto PerfCnt( d_pc.Capture() );
   auto outfile( PseudoBuf( ePseudoBufType::GREP, true ) );
   outfile->MakeEmpty();
   outfile->SetTabWidth( d_SrchFile->TabWidth() ); // inherit tabwidth from searched file
                                                           ED && DBG( "WriteOutput: thisMetaLine='%s', origSrchfnm='%s' => '%s'", thisMetaLine, origSrchfnm?origSrchfnm:"", outfile->Name() );
   //
   // data BUFFER SIZE CALC phase
   //
   size_t imgBufBytes(0);
   const auto fFirstGen( 0 == d_MetaLineCount );
   auto MetaLinesToCopy(0);
   if( d_MetaLineCount > 0 ) {
      RmvLine( 0 );
      for( auto iy(1); iy < d_MetaLineCount; ++iy ) {
         imgBufBytes += d_SrchFile->LineLength( iy );
         ++MetaLinesToCopy;
         RmvLine( iy );
         }                                                 ED && DBG( "d_MetaLineCount=%i,MetaLinesToCopy=%i", d_MetaLineCount, MetaLinesToCopy );
      }
   SprintfBuf Line1( "*GREP* %s", origSrchfnm ? origSrchfnm : d_SrchFile->UserName().c_str() );
   const stref srLine1( Line1 );                           ED && DBG( "%s", Line1.c_str() );
   imgBufBytes += srLine1.length();
   auto numberedMatches(0);
   for( auto iy(0); iy < d_InfLines; ++iy ) {
      if( d_MatchingLines[iy] ) {
         imgBufBytes += d_SrchFile->LineLength( iy );
         if( iy >= d_MetaLineCount ) {
            ++numberedMatches;
            }
         }
      }
   SprintfBuf LastMetaLine( "%s %i %s t=%6.3f", outfile->Name(), numberedMatches, thisMetaLine, PerfCnt );
   const stref srLastMetaLine( LastMetaLine );
   imgBufBytes += srLastMetaLine.length();
   const auto lwidth( fFirstGen ? uint_log_10( d_InfLines ) : 0 );
   if( fFirstGen ) {
      imgBufBytes += (lwidth+2) * numberedMatches;
      }
   outfile->ImgBufAlloc( imgBufBytes, MetaLinesToCopy + 2 + numberedMatches );
   //
   // data COPYING phase
   //
   outfile->ImgBufAppendLine( srLine1 );
   for( auto iy(1); iy < d_MetaLineCount; ++iy ) {  ED && DBG( "auxhd=%i", iy );
      outfile->ImgBufAppendLine( d_SrchFile->PeekRawLine( iy ) );
      }
   outfile->ImgBufAppendLine( srLastMetaLine );
   for( auto iy(0); iy < d_InfLines; ++iy ) {
      if( d_MatchingLines[iy] ) {
         FixedCharArray<20> prefix;
         if( fFirstGen ) { // this is a 1st-generation search: include line # right justified w/in fixed-width field
            prefix.Sprintf( "%*d  ", lwidth, iy + 1 );
            }
         outfile->ImgBufAppendLine( prefix.c_str(), d_SrchFile->PeekRawLine( iy ) );
         }
      }
   outfile->Undo_Reinit();
   outfile->UnDirty();
   outfile->PutFocusOn();
   Msg( "%u line%s %s", numberedMatches, Add_s(numberedMatches), d_fFindAllNegate ? "removed" : "matched" );
   return numberedMatches;
   }

//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************

bool ARG::grep() { enum { ED=0 };
   if( !SearchSpecifierOK( *this ) ) {
      return false;
      }
   // read first line to get header size
   auto curfile( g_CurFBuf() );
   Path::str_t origSrchFnm; int metaLines;
   auto pSearchedFnm( FBOP::IsGrepBuf( origSrchFnm, &metaLines, curfile ) );
   if( pSearchedFnm ) { // Current file is a m2acc result file! (perhaps externally-concocted)
      if( d_fMeta ) { // fMeta says "re-search the _original_ file"!
         curfile = OpenFileNotDir_NoCreate( pSearchedFnm );
         if( !curfile ) {
            return Msg( "Couldn't open %s", pSearchedFnm );
            }
         d_fMeta = false;
         metaLines    = 0;
         pSearchedFnm = nullptr;
         }
      }
   // create aux header for THIS search
   const auto fNegate( s_searchSpecifier->MatchNegated() );
   const auto fUseRegEx( s_searchSpecifier->IsRegex() );
   SprintfBuf auxHdrBuf( "%s'%" PR_BSR "'%s, by Grep, case:%s"
      , fNegate   ? "*NOT* " : ""
      , BSR( s_searchSpecifier->SrchStr() )
      , fUseRegEx ? " (RE)"  : ""
      , g_fCase   ? "sen"    : "ign"
      );
   // At last, do the searching (the easy part...)
   CGrepper gr( curfile, metaLines, fNegate );
   gr.FindAllMatches();
   const auto Matches( gr.WriteOutput( auxHdrBuf, pSearchedFnm ) );
   return Matches > 0;
   }

bool ARG::fg() { enum { ED=0 }; // fgrep
   PFBUF srchfile;
   auto  metaLines( 0 ); // params that govern how...
   auto  curfile( g_CurFBuf() );
   if( TEXTARG == d_argType ) {
      srchfile = curfile;
      Path::str_t keysFnm( ("$FGS" DIRSEP_STR) + std::string( d_textarg.pText ) );
      curfile = OpenFileNotDir_NoCreate( keysFnm.c_str() );
      if( !curfile ) {
         return Msg( "Couldn't open '%s' [1]", keysFnm.c_str() );
         }
      if( d_cArg > 1 ) {
         curfile->PutFocusOn();
         return true;
         }
      }
   else {
      Path::str_t origSrchFnm;    // ...srchfile is processed
      if( FBOP::IsGrepBuf( origSrchFnm, &metaLines, curfile ) ) {
         srchfile = OpenFileNotDir_NoCreate( origSrchFnm.c_str() );
         }
      else {
         PCV;
         auto nextview( DLINK_NEXT( pcv, d_dlinkViewsOfWindow ) );
         if( !nextview || !nextview->FBuf() ) {
            return Msg( "no next file!" );
            }
         srchfile = nextview->FBuf();
         origSrchFnm.assign( srchfile->Namestr() );
         }
      if( !srchfile ) {
         return Msg( "Couldn't open '%s'[2]", origSrchFnm.c_str() );
         }
      }
   // create aux header for THIS search
   SprintfBuf auxHdrBuf( "fgrep keys from %s case:%s"
                                          , curfile->Name()
                                                  , g_fCase ? "sen" : "ign"
                       );
                                                ED && DBG( "'%s'", auxHdrBuf.c_str() );
   auto keyLen(2); // for alternation header (2 chars)
   for( auto line(metaLines); line < curfile->LineCount(); ++line ) {
      const auto rl( curfile->PeekRawLine( line ) );
      if( rl.length() > 0 ) {
         keyLen += rl.length() + 1;  // + 1 for keySep
         }
      }
   auto pszKey( PChar( alloca( keyLen ) ) );
   auto pB( pszKey );
   *pB++ = '!';  // prepend alternation header (2 chars)
   *pB++ = chNUL; // placeholder for keySep
   for( auto line(metaLines); line < curfile->LineCount(); ++line ) {
      const auto rl( curfile->PeekRawLine( line ) );
      if( rl.length() > 0 ) {
         memcpy( pB, rl.data(), rl.length()+1 );
                 pB  +=         rl.length()+1  ;
         }
      }
   const char keySep (
      ((memchr( pszKey+2, keyLen-2, ',' ) == nullptr) ? ',' :
      ((memchr( pszKey+2, keyLen-2, '|' ) == nullptr) ? '|' :
      ((memchr( pszKey+2, keyLen-2, '.' ) == nullptr) ? '.' : chNUL)))
      );
   if( !keySep ) {
      return Msg( "%s: cumulative key contains all possible separators [,|.]", __func__ );
      }
                                                ED && DBG( "KEY=%s", pszKey );
   auto pastEnd( pszKey+keyLen );
   for( auto pC(pszKey) ; pC < pastEnd ; ++pC ) {
      if( chNUL == *pC ) {
         *pC = keySep;
         }
      }
   if( !SetNewSearchSpecifierOK( pszKey, false ) ) {
      return false;
      }
   // At last, do the searching (the easy part...)
   CGrepper gr( srchfile, 0, false );
   gr.FindAllMatches();
   const auto Matches( gr.WriteOutput( auxHdrBuf, nullptr ) );
   Msg( "%u lines matched", Matches );
   return bool(Matches > 0);
   }

PCChar FBOP::IsGrepBuf( Path::str_t &dest, int *pGrepHdrLines, PCFBUF fb ) {
   if( fb->LineCount() == 0 ) {
FAIL: // dest gets filename of CURRENT buffer!  But generation is 0
      dest.assign( fb->Name() );
      *pGrepHdrLines = 0;
      return nullptr;
      }
   {
   const stref srgp( "*GREP* " );
   auto rl( fb->PeekRawLine( 0 ) );
   if( !rl.starts_with( srgp ) )       { goto FAIL; }
   rl.remove_prefix( srgp.length() );
   if( IsStringBlank( rl ) )           { goto FAIL; }
   dest.assign( unquote( rl ) );
   }
   auto iy(1);
   for( ; iy <= fb->LastLine() ; ++iy ) {
      auto rl( fb->PeekRawLine( iy ) );                    0 && DBG("[%d] %s' line=%" PR_BSR "'",iy, fb->Name(), BSR(rl) );
      if( !rl.starts_with( "<grep." ) ) { break; }
      }                                                    0 && DBG( "%s: %s final=[%d] '%s'", __func__, fb->Name(), iy, dest.c_str() );
   *pGrepHdrLines = iy;
   return dest.c_str();
   }

PView FindClosestGrepBufForCurfile( PView pv, PCChar srchFilename ) {
   if( !pv ) { pv = g_CurView(); }
   pv = DLINK_NEXT( pv, d_dlinkViewsOfWindow );
   while( pv ) {
      Path::str_t srchFnm; int dummy;
      if( FBOP::IsGrepBuf( srchFnm, &dummy, pv->FBuf() ) && Path::eq( srchFnm, srchFilename ) ) {
         return pv;
         }
      pv = DLINK_NEXT( pv, d_dlinkViewsOfWindow );
      }
   return nullptr;
   }

bool merge_grep_buf( PFBUF dest, PFBUF src ) {
   Path::str_t srcSrchFnm , destSrchFnm  ;
   int         srcHdrLines, destHdrLines ;
   if(    !FBOP::IsGrepBuf( srcSrchFnm , & srcHdrLines,  src )
       || !FBOP::IsGrepBuf( destSrchFnm, &destHdrLines,  dest)
       || !Path::eq( srcSrchFnm, destSrchFnm )
     ) { return false; }                                                 0 && DBG( "%s: %s copy [1..%d]", __func__, src->Name(), srcHdrLines );
   // insert/copy all src metalines (except 0) to dest
   for( auto iy(1) ; iy < srcHdrLines ; ++iy ) {
      dest->InsLineRaw( destHdrLines++, src->PeekRawLine( iy ) );
      }                                                                  0 && DBG( "%s: %s merg [%d..%d]", __func__, src->Name(), srcHdrLines, src->LineCount()-1 );
   // merge (copy while sorting) all match lines
   for( auto iy(srcHdrLines) ; iy < src->LineCount() ; ++iy ) {
      FBOP::InsLineSortedAscending( dest, destHdrLines, src->PeekRawLine( iy ) );
      }
   return true;
   }

bool ARG::gmg() { // arg "gmg" edhelp  # for docmentation
   const auto fDestroySrcsBufs( 0 == d_cArg );
   PFBUF dest(nullptr);
   Path::str_t srchFilename; int dummy;
   if( FBOP::IsGrepBuf( srchFilename, &dummy, g_CurFBuf() ) ) {
      dest = g_CurFBuf();                                                0 && DBG( "%s: dest=cur (%s)", __func__, dest->Name() );
      }
   else {
      srchFilename = g_CurFBuf()->Namestr();
      }                                                                  0 && DBG( "%s: will look for all greps of (%s)", __func__, srchFilename.c_str() );
   auto merges(0);
   PView pv(nullptr); // cannot auto in while()
   while( (pv=FindClosestGrepBufForCurfile( pv, srchFilename.c_str() )) ) {
      const auto src( pv->FBuf() );
      if( !dest ) {
         dest = src;
         }
      else {
         if( src != dest ) { // can only happen if we restarted scan from head of view list
            merge_grep_buf( dest, src ); ++merges;
            if( fDestroySrcsBufs ) {
               const auto fDidRmv( DeleteAllViewsOntoFbuf( src ) );
               if( fDidRmv ) { pv = nullptr; } // restart scan from head of view list
               }
            }
         }
      }
   if( dest ) {
      dest->UnDirty();
      dest->PutFocusOn();
      Msg( "merged %d other grep buffer%s into this one", merges, Add_s( merges ) );
      return true;
      }
   return Msg( "no grep buffers found" );
   }

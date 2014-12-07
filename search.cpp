//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

//   TO GET RID OF
//
// s_fSearchNReplaceUsingRegExp
// s_pSandR_CompiledSearchPattern

#include "ed_main.h"
#include "ed_search.h"
#include "fname_gen.h"

// !0 == VERBOSE_SEARCH logging
#if 0
#  define  VS_( x ) x
#else
#  define  VS_( x )
#endif

GLOBAL_VAR PFBUF g_pFBufSearchLog;
GLOBAL_VAR PFBUF g_pFBufSearchRslts;

struct DeleteObject { // "Effective STL" pg 38
   template <typename T>
   void operator()( const T *ptr ) const { delete ptr; }
   };

template <class P>
struct Freer {
   const P d_p;

   public:

   Freer(P p) : d_p(p) {}
   ~Freer() { Free_( d_p ); }
   P operator()() { return d_p; }
   };


FBufLocnNow::FBufLocnNow() : FBufLocn( g_CurFBuf(), g_Cursor() ) {}

bool FBufLocn::Moved() const {
   return !(InCurFBuf() && g_Cursor() == d_pt);
   }

bool FBufLocn::ScrollToOk() const {
   if( !IsSet() )
      return false;

   d_pFBuf->PutFocusOn();
   g_CurView()->MoveCursor( d_pt.lin, d_pt.col, d_width );
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

   STD_TYPEDEFS( SearchScanMode )

//****************************

class SearchStats { // found only in FileSearchMatchHandler
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

//****************************

class FileSearchMatchHandler {
   protected:

   int         d_lifetimeFileCount;
   int         d_lifetimeFileCountMatch      ;
   int         d_lifetimeFileCountMatchAction;
   SearchStats d_lifetimeStats;
   SearchStats d_curFileStats;
   bool        d_fScrollToFirstMatch;
   FBufLocn    d_flToScroll;

   Point       d_scrollPoint;
   MainThreadPerfCounter d_pc;

   public:

   FileSearchMatchHandler( bool fScrollToFirstMatch=true )
      : d_lifetimeFileCount           (0)
      , d_lifetimeFileCountMatch      (0)
      , d_lifetimeFileCountMatchAction(0)
      , d_fScrollToFirstMatch(fScrollToFirstMatch)
      { g_CurView()->FreeHiLiteRects(); }

   virtual ~FileSearchMatchHandler() {}

   // External Event Hooks

   virtual void VEnteringFile( PFBUF pFBuf ) { // could add a file skip retval here
                ++d_lifetimeFileCount;
                if( d_curFileStats.GetMatches()      ) ++d_lifetimeFileCountMatch;
                if( d_curFileStats.GetMatchActions() ) ++d_lifetimeFileCountMatchAction;

                d_curFileStats.Reset();
                }

           bool FoundMatchContinueSearching( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures );
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
      //
      virtual bool VMatchWithinColumnBounds( PFBUF pFBuf, Point &cur, COL MatchCols ) { return true; }; // cur MAY BE MODIFIED IFF returned false, to mv next srch to next inbounds rgn!!!
      virtual bool VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ); // cur MAY BE MODIFIED!!!
      virtual bool VContinueSearching() { return true; }

      // called by ShowResults
      //
      virtual void VShowResultsNoMacs() {}

   // common tools for SUBCLASS Event Hooks to use

   bool ScrollToFLOk() const; // useful in VShowResultsNoMacs()
   int GetLifetimeMatchCount()            const { return d_lifetimeStats.GetMatchActions(); }
   int GetLifetimeFileCount()             const { return d_lifetimeFileCount              ; }
   int GetLifetimeFileCountMatch()        const { return d_lifetimeFileCountMatch         ; }
   int GetLifetimeFileCountMatchAction()  const { return d_lifetimeFileCountMatchAction   ; }
   };

bool FileSearchMatchHandler::FoundMatchContinueSearching( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) {
   if( VMatchWithinColumnBounds( pFBuf, cur, MatchCols ) ) { // it IS a MATCH?
      if( d_fScrollToFirstMatch && !d_flToScroll.IsSet() )
         d_flToScroll.Set( pFBuf, cur, MatchCols );

      d_lifetimeStats.IncMatch();
      d_curFileStats .IncMatch();
      if( VMatchActionTaken( pFBuf, cur, MatchCols, pCaptures ) ) { // Is it also an ACTION?
         d_lifetimeStats.IncMatchAction();
         d_curFileStats .IncMatchAction();
         }
      }

   return VContinueSearching();
   }

bool FileSearchMatchHandler::VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) {
   PCV;
   if( pcv->FBuf() == pFBuf ) {
      pcv->SetMatchHiLite( cur, MatchCols, g_fCase );
      }
   return true;  // "action" taken!
   }


bool FileSearchMatchHandler::ScrollToFLOk() const { return d_flToScroll.ScrollToOk(); }

//****************************
//****************************

class FindPrevNextMatchHandler : public FileSearchMatchHandler {
   NO_COPYCTOR(FindPrevNextMatchHandler);
   NO_ASGN_OPR(FindPrevNextMatchHandler);

   const char   d_dirCh;
   const bool   d_fIsRegex;
   const PCChar d_SrchStr;
   const size_t d_SrchStrLen;
   const PChar  d_SrchDispStr;
         size_t d_SrchDispStrLen;

   void DrawDialog( PCChar hdr, PCChar trlr );

   protected:

   bool VContinueSearching() override { return false; };  // stop at first match

   public:

   FindPrevNextMatchHandler( bool fSearchForward, bool fIsRegex, PCChar srchStr );
   ~FindPrevNextMatchHandler() {
      Free_( PVoid(d_SrchStr) );
      Free_( PVoid(d_SrchDispStr) );
      }

   void VShowResultsNoMacs() override;
   };

GLOBAL_CONST char chRex = '/';

void FindPrevNextMatchHandler::DrawDialog( PCChar hdr, PCChar trlr ) {
   const auto de_color( 0x5f );
   const auto ss_color( g_fCase ? 0x4f : 0x2f );
   VideoFlusher vf;
   auto             chars (  VidWrStrColor( DialogLine(), 0    , hdr           , Strlen(hdr)      , g_colorInfo , false ) );
   if( d_fIsRegex ) chars += VidWrStrColor( DialogLine(), chars, &chRex        , sizeof(chRex)    , de_color    , false );
                    chars += VidWrStrColor( DialogLine(), chars, d_SrchDispStr , d_SrchDispStrLen , ss_color    , false );
   if( d_fIsRegex ) chars += VidWrStrColor( DialogLine(), chars, &chRex        , sizeof(chRex)    , de_color    , false );
                             VidWrStrColor( DialogLine(), chars, trlr          , Strlen(trlr)     , g_colorInfo , true  );
   }

FindPrevNextMatchHandler::FindPrevNextMatchHandler( bool fSearchForward, bool fIsRegex, PCChar srchStr )
   : FileSearchMatchHandler( true )
   , d_dirCh(fSearchForward ? '+' : '-')
   , d_fIsRegex(fIsRegex)
   , d_SrchStr( Strdup( srchStr ) )
   , d_SrchStrLen( Strlen(d_SrchStr) )
   , d_SrchDispStr( Strdup( srchStr ) )
   , d_SrchDispStrLen( Strlen(srchStr) )
   {
   d_SrchDispStrLen = PrettifyStrcpy( d_SrchDispStr, 1+d_SrchDispStrLen, srchStr, 1, g_chTabDisp, 0, g_chTrailSpaceDisp );

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
//****************************

class FileSearcher;

class MFGrepMatchHandler : public FileSearchMatchHandler {
   PFBUF d_pOutputFile;
   Xbuf  d_xb;

   protected:

   bool VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) override;
   void VShowResultsNoMacs() override;

   public:

   MFGrepMatchHandler( PFBUF pOutputFile )
      : d_pOutputFile(pOutputFile)
      {
      Msg( "searching cache" );  // rewrite the dialog line so user doesn't think we've hung
      }

   void InitLogFile( const FileSearcher &FSearcher );
   STATIC_CONST SearchScanMode &sm() { return smFwd; }
   };

bool MFGrepMatchHandler::VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) {
   if( 0 == GetLifetimeMatchCount() )
      LuaCtxt_Edit::LocnListInsertCursor(); // do this IFF a match was found

   {
   pFBuf->getLineTabxPerRealtabs( &d_xb, cur.lin );
   NOAUTO CPCChar frags[] = { pFBuf->Name(), FmtStr<40>( " %d %dL%d: ", cur.lin+1, cur.col+1, MatchCols ), d_xb.c_str() };
   d_pOutputFile->PutLastLine( frags, ELEMENTS(frags) );
   }

   MoveCursorToEofAllWindows( d_pOutputFile );

   return true;  // action taken!
   }

//****************************
//****************************

GLOBAL_VAR bool g_fFastsearch = true;

struct SearchSpecifier {
   PChar  d_rawStr;
   int    d_rawStrLen;
#if USE_PCRE
   bool   d_fRegexCase;   // state when last (re-)init'd
   PRegex d_re;
   bool   d_reCompileErr;
#endif
   bool   d_fCanUseFastSearch;

   void   Build( PCChar rawStr, int rawStrLen, bool fRegex );

   public:

   SearchSpecifier( PCChar rawStr, int rawStrLen, bool fRegex=false );
   SearchSpecifier( int, PCChar tail );
   ~SearchSpecifier();

   bool   IsRegex() const;
   bool   HasError() const;
#if USE_PCRE
   PCRegex GetRegex() const { return d_re; }
#endif
   bool   CanUseFastSearch() const { return d_fCanUseFastSearch && g_fFastsearch; }
   bool   CaseUpdt(); // in case case switch has changed since Regex was compiled
   void   Dbgf( PCChar tag ) const;
   PCChar SrchStr()    const { return d_rawStr ? d_rawStr : ""; }
   int    SrchStrLen() const { return d_rawStrLen; }
   };  STD_TYPEDEFS( SearchSpecifier )

STATIC_VAR PSearchSpecifier s_searchSpecifier;

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

   Xbuf                    d_xb;

   FileSearchMatchHandler &d_mh;

   Point                  d_start;
   Point                  d_end;
   PFBUF                  d_pFBuf;

   PCapturedStrings       d_pCaptures;

   FileSearcher( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh, int capturesNeeded=1 );

   virtual void   VPrepLine_( PChar lbuf ) const {};
   virtual PCChar VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const = 0; // rv=0 if no match found or PCChar within pBuf of match

   public:

   enum StringSearchVariant {
      fsTABSAFE_STRING ,  // FileSearcherString also bidirectional
      fsFAST_STRING    ,  // FileSearcherFast   about 20% faster than fsTABSAFE_STRING (WHEN SEARCHING CACHED FILES!!!)  (CANNOT SEARCH BACKWARD!)
      };

           void FindMatches();
   virtual void VFindMatches_();
   // void FindMatchesMultifile( PCChar macroContainingWildcards );
   // void SkipMatch();
   // void SkipLine();
   void SetInputFile( PFBUF pFBuf=g_CurFBuf() );
   void SetBoundsWholeFile();
   void SetBoundsToEnd( Point StartPt );
   void SetBounds( Point StartPt, Point EndPt );
   void SetBounds( const ARG &arg );
   void Dbgf() const;
   PCChar SrchStr() const { return d_ss.SrchStr(); }
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

GLOBAL_CONST auto g_PtInvalid = Point( -1, -1 );

void FileSearcher::ResolveDfltBounds() {
   auto &Bof( d_sm.d_fSearchForward ? d_start : d_end   );
   auto &Eof( d_sm.d_fSearchForward ? d_end   : d_start );
   if( g_PtInvalid == Bof )  Bof = Point( 0, 0 );
   if( g_PtInvalid == Eof )  Eof = Point( d_pFBuf->LastLine(), COL_MAX );
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
   if( d_pFBuf )
      d_mh.VLeavingFile( d_pFBuf );

   d_pFBuf = pFBuf;
   SetBoundsWholeFile();  // init dflt: whole file
   d_mh.VEnteringFile( d_pFBuf );
   }

FileSearcher::~FileSearcher() {
   if( d_pFBuf )
      d_mh.VLeavingFile( d_pFBuf );
   Delete0( d_pCaptures );
   }


GLOBAL_CONST char kszCompileHdr[] = "+^-^+";

void MFGrepMatchHandler::InitLogFile( const FileSearcher &FSearcher ) { // digression!
#if 1
   LuaCtxt_Edit::nextmsg_setbufnm( szSearchRslts );
   LuaCtxt_Edit::nextmsg_newsection_ok( SprintfBuf( "mfgrep::%s %s", FSearcher.IsRegex() ? "regex" : "str", FSearcher.SrchStr()) );
#else

   if( d_pOutputFile->LineCount() > 0 )
       d_pOutputFile->PutLastLine( "" );

   {
   CPCChar frags[] = { kszCompileHdr, " ", FSearcher.IsRegex() ? "mfgrep::regex" : "mfgrep::str", " ", FSearcher.SrchStr() };
   d_pOutputFile->PutLastLine( frags, ELEMENTS(frags) );
   }
   d_pOutputFile->PutLastLine( "" );
   d_pOutputFile->Set_LineCompile( d_pOutputFile->LastLine() );
#endif
   }


STATIC_FXN FileSearcher *NewFileSearcher( FileSearcher::StringSearchVariant type, const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );

// pointer to one of the two following functions
typedef PCChar (* pFxn_strstr) ( PCChar haystack, int haystackLen, PCChar needle, int needleLen );
STATIC_FXN  PCChar    strnstr      ( PCChar haystack, int haystackLen, PCChar needle, int needleLen );
STATIC_FXN  PCChar    strnstri     ( PCChar haystack, int haystackLen, PCChar needle, int needleLen );

class  FileSearcherString : public FileSearcher {
   const COL d_searchKeyStrlen;
   PChar     d_searchKey;

   NO_COPYCTOR(FileSearcherString);
   NO_ASGN_OPR(FileSearcherString);

   public:

   FileSearcherString( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   ~FileSearcherString() { Free0( d_searchKey ); }

   void   VPrepLine_( PChar lbuf ) const override;
   PCChar VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const override;
   };

class  FileSearcherFast : public FileSearcher {  // ONLY SEARCHES FORWARD!!!
   COL         d_searchKeyStrlen;
   PChar       d_searchKey;
   pFxn_strstr d_pfxStrnstr;

   int        *d_pNeedleLens;
   int         d_needleCount;

   NO_COPYCTOR(FileSearcherFast);
   NO_ASGN_OPR(FileSearcherFast);

   public:

   FileSearcherFast( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   virtual ~FileSearcherFast();
   void   VFindMatches_() override;
   void   VPrepLine_( PChar lbuf ) const override;
   PCChar VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const override;

   int NeedleLen( int ix ) const { return (ix < d_needleCount) ? d_pNeedleLens[ix] : 0; }
   };

#if USE_PCRE

class  FileSearcherRegex : public FileSearcher {
   NO_COPYCTOR(FileSearcherRegex);
   NO_ASGN_OPR(FileSearcherRegex);

   public:

   FileSearcherRegex( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh );
   PCChar VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const override;
   };

#endif

//*****************************************************************************************************

#define  REPLC_CLASSES  0


 #if REPLC_CLASSES

#define         s_fSearchNReplaceUsingRegExp    (s_searchSpecifier->IsRegex())
#define         s_pSandR_CompiledSearchPattern  (s_searchSpecifier->d_re     )

 #elif USE_PCRE

STATIC_VAR bool     s_fSearchNReplaceUsingRegExp;
STATIC_VAR PRegex   s_pSandR_CompiledSearchPattern;

 #endif


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
   if( failed && !ExecutionHaltRequested() )
      return ErrorDialogBeepf( "Cannot read '%s'", Name() );

   return failed;
   }

//=============================================================================================

STATIC_FXN void InsHiLite1Line( PView pView, const Point &pt, int Cols ) {
   const auto hiliteWidth( Cols > 0 ? Cols : 1 );
   pView->InsHiLite1Line( COLOR::HG , pt.lin, pt.col, pt.col + hiliteWidth - 1 );
   }

void View::SetStrHiLite( const Point &pt, COL Cols, int color ) {
   const auto hiliteWidth( Cols > 0 ? Cols : 1 );
   InsHiLite1Line( color, pt.lin, pt.col, pt.col + hiliteWidth - 1 );
   }

void View::SetMatchHiLite( const Point &pt, COL Cols, bool fErrColor ) {
   const auto colorIdx( fErrColor ? COLOR::ERR : COLOR::SEL );
   const auto hiliteWidth( Cols > 0 ? Cols : 1 );

   enum { MWHOSMHL = 0 }; // -> MASK_WUC_HILITES_ON_SEARCH_MATCH_HILIT_LINE
   if( MWHOSMHL && pt.col > 0 ) { InsHiLite1Line( COLOR::CXY, pt.lin, 0                   , pt.col               - 1 ); }
                                  InsHiLite1Line( colorIdx  , pt.lin, pt.col              , pt.col + hiliteWidth - 1 );
   if( MWHOSMHL               ) { InsHiLite1Line( COLOR::CXY, pt.lin, pt.col + hiliteWidth, COL_MAX                  ); }
   }

class HiLiteFreer {
   const PView d_View;

public:
   HiLiteFreer() :  d_View( g_CurView() ) {}
   ~HiLiteFreer() { d_View->FreeHiLiteRects(); }
   };


enum CheckNextRetval { STOP_SEARCH, CONTINUE_SEARCH, REREAD_LINE_CONTINUE_SEARCH, /* CONTINUE_SEARCH_NEXT_LINE */ };

class CharWalker {
public:
   virtual CheckNextRetval VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) = 0;
   virtual ~CharWalker() {};
   };


STIL COL ColPlusDeltaWithinStringRegion( COL tabWidth, PCChar pRawBuf, PCChar eos, COL xCol, int delta ) {
   return g_fRealtabs
          ? ColOfPtr( tabWidth, pRawBuf, PtrOfColWithinStringRegion( tabWidth, pRawBuf, eos, xCol ) + delta , eos )
          :                                                                                  xCol   + delta ;
   }

// CharWalkRect is called by PBalFindMatching, PMword, and GenericReplace
STATIC_FXN bool CharWalkRect( PFBUF pFBuf, const Rect &constrainingRect, const Point &start, bool fMoveFwd, CharWalker *pWalker ) {
   const auto tw( pFBuf->TabWidth() );
   #define SETUP_LINE_TEXT                                      \
           if( ExecutionHaltRequested() ) {                     \
              FlushKeyQueuePrimeScreenRedraw();                 \
              return false;                                     \
              }                                                 \
           PCChar bos, eos;                                     \
           pFBuf->PeekRawLineExists( curPt.lin, &bos, &eos );   \
           auto colLastPossibleLastMatchChar( ColOfPtr( tw, bos, eos-1, eos ) );

   #define CHECK_NEXT  {  \
           const auto rv( pWalker->VCheckNext( pFBuf, bos, eos, &curPt, &colLastPossibleLastMatchChar ) );  \
           if( STOP_SEARCH == rv ) return true;                                                             \
           if( REREAD_LINE_CONTINUE_SEARCH == rv ) { pFBuf->PeekRawLineExists( curPt.lin, &bos, &eos ); }   \
           }

   if( fMoveFwd ) { // -------------------- search FORWARD --------------------
      Point curPt( start.lin, start.col + 1 );
      for( auto yMax(constrainingRect.flMax.lin) ; curPt.lin <= yMax ; ) {
         SETUP_LINE_TEXT;
         NoMoreThan( &colLastPossibleLastMatchChar, constrainingRect.flMax.col );
         for(
            ; curPt.col <= colLastPossibleLastMatchChar
            ; curPt.col = ColPlusDeltaWithinStringRegion( tw, bos, eos, curPt.col, +1 )
            ) CHECK_NEXT
         ++curPt.lin;  curPt.col = constrainingRect.flMin.col;
         }
      }
   else { // -------------------- search BACKWARD --------------------
      Point curPt( start.lin, start.col - 1 );
      if( constrainingRect.flMin.col > curPt.col ) { --curPt.lin;  curPt.col = constrainingRect.flMax.col; }
      for( auto yMin(constrainingRect.flMin.lin) ; curPt.lin >= yMin ; ) {
         SETUP_LINE_TEXT;
         for( curPt.col = Min( colLastPossibleLastMatchChar, (curPt.col >= 0) ? curPt.col : constrainingRect.flMax.col )
            ; curPt.col >= constrainingRect.flMin.col
            ; curPt.col = ColPlusDeltaWithinStringRegion( tw, bos, eos, curPt.col, -1 )
            ) CHECK_NEXT
         --curPt.lin;
         }
      }
   return false;
   }

STATIC_VAR std::string g_SavedSearchString_Buf ;
GLOBAL_VAR std::string g_SnR_szSearch          ;
GLOBAL_VAR std::string g_SnR_szReplacement     ;

class ReplaceCharWalker : public CharWalker {
   Xbuf              d_xbb;
   CPCChar           d_pszSearch ;
   int               d_searchLen ;
   CPCChar           d_pszReplace;
   const int         d_replaceLen;
   bool              d_fDoReplaceQuery;
   const pfx_strncmp d_strncmp_fxn;

   public:

   int               d_iReplacementsPoss;
   int               d_iReplacementsMade;
   int               d_iReplacementFiles;
   int               d_iReplacementFileCandidates;

   ReplaceCharWalker(
        bool fDoReplaceQuery
      , bool fSearchCase
      )
      : d_pszSearch         ( g_SnR_szSearch.c_str()      )
      , d_searchLen         ( g_SnR_szSearch.length()       )
      , d_pszReplace        ( g_SnR_szReplacement.c_str() )
      , d_replaceLen        ( g_SnR_szReplacement.length()  )
      , d_fDoReplaceQuery   ( fDoReplaceQuery )
      , d_strncmp_fxn       ( fSearchCase ? strncmp : Strnicmp )
      , d_iReplacementsPoss ( 0 )
      , d_iReplacementsMade ( 0 )
      , d_iReplacementFiles ( 0 )
      , d_iReplacementFileCandidates ( 0 )
      {}

   CheckNextRetval VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) override;

   private:

   void DoFinalPartOfReplace(
        PFBUF  pFBuf                           // buffer where replace is taking place
      , PChar  lbuf                            // start of line buffer containing match, no detabbing done
      , PChar  pMatch                          // ptr (within lbuf) of first char in current match
      , Point *curPt
      , int   *pColLastPossibleLastMatchChar
      );
   };


// replace @ pMatch (in lbuf), adjust curPt->col and *pColLastPossibleLastMatchChar
void ReplaceCharWalker::DoFinalPartOfReplace( PFBUF pFBuf, PChar lbuf, PChar pMatch, Point *curPt, int *pColLastPossibleLastMatchChar ) {
   0 && DBG("DFPoR+ (%d,%d) LR=%d LoSB=%d", curPt->col, curPt->lin, d_replaceLen, Strlen( lbuf ) );

   memmove( pMatch  + d_replaceLen                 // blow open a hole ...
          , pMatch  + d_searchLen
          , Strlen( pMatch  + d_searchLen ) + 1
          );
   memcpy( pMatch, d_pszReplace, d_replaceLen );   // ... insert replacement string
   pFBuf->PutLine( curPt->lin, lbuf );             // ... and commit
   ++d_iReplacementsMade;

   // replacement done: position curPt->col for next search
   //
   if( d_searchLen != 0 || d_replaceLen != 0 ) {
      curPt->col += d_replaceLen - 1;
      }

   // now we have to figger out if *pColLastPossibleLastMatchChar grew/shrank
   //
   *pColLastPossibleLastMatchChar += d_replaceLen - d_searchLen;
   NoLessThan( pColLastPossibleLastMatchChar, 0 );

   0 && DBG("DFPoR- (%d,%d) L %d", curPt->col, curPt->lin, *pColLastPossibleLastMatchChar );
   0 && DBG("DFPoR- L=%d '%*s'", curPt->lin, *pColLastPossibleLastMatchChar, lbuf+curPt->col );
   }


CheckNextRetval ReplaceCharWalker::VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) {
   CPCChar pxCur( PtrOfColWithinStringRegionNoEos( pFBuf->TabWidth(), ptr, eos, curPt->col ) );

   0 && DBG( "%s ( %d, %d L %d ) for '%s' in '%-.*s'", __func__
                          , curPt->lin, curPt->col, d_searchLen
                                             , d_pszSearch
                                                     , *pColLastPossibleLastMatchChar
                                                         , pxCur
           );

   if( 0 != d_strncmp_fxn( d_pszSearch, pxCur, d_searchLen ) )
      return CONTINUE_SEARCH;

   const auto idxOfLastCharInMatch( curPt->col + d_searchLen - 1 );
   if( idxOfLastCharInMatch > *pColLastPossibleLastMatchChar ) {
      // match that lies partially OUTSIDE a BOXARG: skip
      0 && DBG( " '%-.*s' matches '%-.*s', but only '%-.*s' in bounds"
           , d_searchLen, pxCur
           , d_searchLen, d_pszSearch
           , *pColLastPossibleLastMatchChar - idxOfLastCharInMatch, pxCur
           );
      return CONTINUE_SEARCH;
      }

   ++d_iReplacementsPoss;  //##### it's A REPLACABLE MATCH


#if 0
   const auto lbuf( d_xbb.resize( 1+(eos - ptr) + d_replaceLen - d_searchLen ) );
   pFBuf->getLineRaw( &d_xbb, curPt->lin );
   const auto pMatch( lbuf + (pxCur - ptr) );
#else
   const auto lbuf( d_xbb.wresize( 1+FBOP::LineCols( pFBuf, curPt->lin ) + d_replaceLen - d_searchLen ) );
   pFBuf->getLineTabxPerRealtabs( &d_xbb, curPt->lin );
   const auto pMatch( lbuf + curPt->col );
#endif

   if( !d_fDoReplaceQuery ) { // non-interactive-replace?
      DoFinalPartOfReplace(
           pFBuf
         , lbuf
         , pMatch
         , curPt
         , pColLastPossibleLastMatchChar
         );
      return REREAD_LINE_CONTINUE_SEARCH;
      }

   //##### interactive-replace (mfreplace/qreplace) ONLY ...

   const auto pView( pFBuf->PutFocusOn() );
   pView->FreeHiLiteRects();
   DispDoPendingRefreshesIfNotInMacro();

 #if 1
   curPt->ScrollTo( d_searchLen );
 #else
   pView->MoveAndCenterCursor( *curPt, d_searchLen );
 #endif

   pView->SetMatchHiLite( *curPt, d_searchLen, true );
   DispDoPendingRefreshesIfNotInMacro();

   HiLiteFreer hf;
                                       //  allowed responses
                                       //  |       interactive dflt: cause retry by NOT matching any explicit case below
                                       //  |       |    macro dflt:
                                       //  |       |    |
   const auto ch( chGetCmdPromptResponse( "ynaq", '?', 'a', "Replace this occurrence? (Yes/No/All/Quit): " ) );
   switch( ch ) {
      default:  Assert( 0 ); // chGetCmdPromptResponse has bug or params wrong
      case -1 :                                      //  fall thru!
      case 'q': SetUserChoseEarlyCmdTerminate();  return STOP_SEARCH;
      case 'a': d_fDoReplaceQuery = false;           //  fall thru!
      case 'y': DoFinalPartOfReplace(                //  fall thru!
                     pFBuf                           //  fall thru!
                   , lbuf                            //  fall thru!
                   , pMatch                          //  fall thru!
                   , curPt                           //  fall thru!
                   , pColLastPossibleLastMatchChar   //  fall thru!
                   );                             return REREAD_LINE_CONTINUE_SEARCH;
      case 'n':                                   return CONTINUE_SEARCH;
      }
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

GLOBAL_CONST char szSetfile[] = "setfile";
GLOBAL_CONST char szCompile[] = "<compile>";

#if USE_PCRE
bool SearchSpecifier::HasError() const { return d_reCompileErr; }
bool SearchSpecifier::IsRegex()  const { return d_re != nullptr; }
#else
bool SearchSpecifier::HasError() const { return false; }
bool SearchSpecifier::IsRegex()  const { return false; }
#endif

STATIC_FXN bool SetNewSearchSpecifierOK( PCChar rawStr, PCChar eos, bool fRegex ) {
   if( !eos ) eos = Eos( rawStr );
   VS_( if( s_searchSpecifier ) { s_searchSpecifier->Dbgf( "befor" ); } )
   auto ssNew( new SearchSpecifier( rawStr, eos - rawStr, fRegex ) );
#if USE_PCRE
   const auto err( ssNew->HasError() );
   if( err ) {
      Delete0( ssNew );
      }
   else
#endif
      {
      Delete0( s_searchSpecifier );
      s_searchSpecifier = ssNew;
      g_SavedSearchString_Buf.assign( rawStr, eos - rawStr );  // HACK to let ARG::grep inherit prev search strings
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

STATIC_FXN void AddLineToLogStack( PFBUF pFbuf, PCChar str ) { // deletes all duplicates of the inserted line
   const auto argLen( Strlen( str ) );
   for( auto ln(0); ln < pFbuf->LineCount(); ++ln ) {
      PCChar pLine; size_t chars;
      if(  pFbuf->PeekRawLineExists( ln, &pLine, &chars ) // loop across all needles in [d_searchKey .. d_searchKey + d_searchKeyStrlen - 1]
        && chars == argLen
        && 0 == memcmp( str, pLine, argLen )
        ) {
         pFbuf->DelLine( ln );
         --ln;   // delete ALL duplicate strings
         }
      }

   pFbuf->InsLine( 0, str );
   pFbuf->UnDirty();  // cosmetic
   }

GLOBAL_VAR PFBUF g_pFBufTextargStack;

void AddToTextargStack( PCChar str ) {
   0 && DBG( "%s: '%s'", __func__, str );
   AddLineToLogStack( g_pFBufTextargStack, str );
   }

void AddToSearchLog( PCChar str ) {  // *** CALLED BY LUA Libfunc
   AddLineToLogStack( g_pFBufSearchLog, str );
   }

STATIC_FXN bool SearchLogSwap() {
   if( g_CurFBuf() == g_pFBufSearchLog ) {
      fExecute( szSetfile );
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
        ||      0 != d_textarg.pText[1]
        )
         return Msg( "textarg of searchlog can only be single char ['1'..'9'] = line number!" );

      yLine = d_textarg.pText[0] - '0';
      }

   g_pFBufSearchLog->PutFocusOn();
   if( yLine > 0 )
      g_CurView()->MoveCursor( yLine-1, 0 );

   return true;
   }


STATIC_FXN bool SearchSpecifierOK( ARG *pArg ) {
   if( pArg->d_fMeta )  g_fCase = !g_fCase;

   switch( pArg->d_argType ) {
      default:      return pArg->BadArg();

      case TEXTARG: SearchLogSwap();
                    AddToSearchLog( pArg->d_textarg.pText );
                    if( !SetNewSearchSpecifierOK( pArg->d_textarg.pText, nullptr, pArg->d_cArg >= 2 ) )
                       return ErrorDialogBeepf( "bad search specifier '%s'", pArg->d_textarg.pText );
                    break;

      case NOARG:   if( s_searchSpecifier ) {
                       s_searchSpecifier->CaseUpdt();
                       }
                    else {
                       if( !g_pFBufSearchLog && FBOP::IsLineBlank( g_pFBufSearchLog, 0 ) ) {
                          return ErrorDialogBeepf( "No search string specified, %s empty", szSearchLog );
                          }

                       PCChar bos, eos;
                       g_pFBufSearchLog->PeekRawLineExists( 0, &bos, &eos );
                       if( !SetNewSearchSpecifierOK( bos, eos, false ) ) {
                          return ErrorDialogBeepf( "bad search specifier '%s'", std::string( bos, eos - bos ).c_str() );
                          }
                       }
                    break;
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

STATIC_FXN void MFGrepProcessFile( PCChar filename, FileSearcher *d_fs ) {
   const auto pFBuf( FBOP::FindOrAddFBuf( filename ) );
   0 && DBG( "d_dhdViewsOfFBUF.IsEmpty(%s)==%c", filename, pFBuf->ViewCount()==0?'t':'f' );
   const auto fWeCanGarbageCollectFBUF( !pFBuf->HasLines() );
   if( pFBuf->RefreshFailedShowError() )
      return;

   d_fs->SetInputFile( pFBuf );
   d_fs->FindMatches();

   if( d_fs->d_mh.VCanForgetCurFile() ) {
      GarbageCollectFBUF( pFBuf, fWeCanGarbageCollectFBUF );
      }
   else {
      DispDoPendingRefreshesIfNotInMacro();
      }
   }

//-----------------------------------

STATIC_FXN PCChar findDelim( PCChar tokStrt, PCChar delimset, PCChar macroName ) {
   const auto pastTokEnd( StrToNextOrEos( tokStrt, delimset ) );
   if( 0 == *pastTokEnd ) {
      ErrorDialogBeepf( "Macro '%s': value has unbalanced %s delimiter", macroName, delimset );
      return nullptr;
      }
   return pastTokEnd;
   }


STATIC_FXN PChar DupTextMacroValue( PCChar macroName ) {
   const auto pCmd( CmdFromName( macroName ) );
   if( !pCmd || !pCmd->IsRealMacro() )
      return nullptr;

   decltype(macroName) pastTokEnd;
   auto tokStrt( StrPastAnyWhitespace( pCmd->MacroText() ) );
   0 && DBG( "%s: '%s'", __func__, tokStrt );
   switch( *tokStrt ) {
      case 0:    pastTokEnd = nullptr;                                  break;
      case '"':  pastTokEnd = findDelim( ++tokStrt, "\"", macroName );  break;
      case '\'': pastTokEnd = findDelim( ++tokStrt, "'" , macroName );  break;
      default:   pastTokEnd = StrToNextWhitespaceOrEos( tokStrt );      break;
      }

   if( nullptr == pastTokEnd )  // empty value or bad delim?
      return nullptr;

   const auto txtLen( pastTokEnd - tokStrt );
   if( 0 == txtLen )  // empty value?
      return nullptr;

   return Strdup( tokStrt, txtLen );
   }

STATIC_FXN PathStrGenerator *GrepMultiFilenameGenerator( PChar nmBuf=nullptr, size_t sizeofBuf=0 ) {
   {
   auto mfspec_text( DupTextMacroValue( "mffile" ) );
   if( mfspec_text ) {
      if( !IsStringBlank( mfspec_text ) ) {
         0 && DBG( "%s: FindFBufByName( %s )?", __func__, mfspec_text );
         const auto pFBufMfspec( FindFBufByName( mfspec_text ) );
         Free0( mfspec_text );
         if( pFBufMfspec && !FBOP::IsBlank( pFBufMfspec ) ) {
            if( nmBuf && sizeofBuf ) { safeSprintf( nmBuf, sizeofBuf, "%s (buffer,*mfptr)", pFBufMfspec->Name() ); }
            return new FilelistCfxFilenameGenerator( pFBufMfspec );
            }
         }
      Free0( mfspec_text );
      }
   }
   {
   const auto pFBufMfspec( FindFBufByName( "<mfspec>" ) );
   if( pFBufMfspec && !FBOP::IsBlank( pFBufMfspec ) ) {
      if( nmBuf && sizeofBuf ) { safeStrcpy( nmBuf, sizeofBuf, "<mfspec> (buffer)" ); }
      return new FilelistCfxFilenameGenerator( pFBufMfspec );
      }
   }

   {
   auto mfspec_text( DupTextMacroValue( "mfspec" ) );
   if( mfspec_text ) {
      if( !IsStringBlank( mfspec_text ) ) {
         if( nmBuf && sizeofBuf ) { safeStrcpy( nmBuf, sizeofBuf, "mfspec (macro)" ); }
         const auto rv( new CfxFilenameGenerator( mfspec_text, ONLY_FILES ) );
         Free0( mfspec_text );
         return rv;
         }
      Free0( mfspec_text );
      }
   }
   {
   auto mfspec_text( DupTextMacroValue( "mfspec_" ) );
   if( mfspec_text ) {
      if( !IsStringBlank( mfspec_text ) ) {
         if( nmBuf && sizeofBuf ) { safeStrcpy( nmBuf, sizeofBuf, "mfspec_ (macro)" ); }
         const auto rv( new CfxFilenameGenerator( mfspec_text, ONLY_FILES ) );
         Free0( mfspec_text );
         return rv;
         }
      Free0( mfspec_text );
      }
   }

   if( nmBuf && sizeofBuf ) { safeStrcpy( nmBuf, sizeofBuf, "no mfspec setting active" ); }
   return nullptr;
   }


#ifdef fn_mgl
bool ARG::mgl() {
   PathStrGenerator *pGen = GrepMultiFilenameGenerator();
   if( pGen ) {
      auto ix(0);
      pathbuf pbuf;
      while( pGen->GetNextName( BSOB(pbuf) ) )
         DBG( "  [%d] = '%s'", ix++, pbuf );
      Delete0( pGen );
      }

   return true;
   }
#endif


// Note that this DOES NOT automatically recurse directories, it only visits the
// directories that are EXPLICITLY called out in macroName's content.  OTOH, if
// envvars occur in macroName's content, these are expanded, and each element of
// the expansion is searched.
//

bool ARG::mfgrep() {
   if( !SearchSpecifierOK( this ) )
      return false;

   MFGrepMatchHandler mh( g_pFBufSearchRslts );
   auto pSrchr( NewFileSearcher
      ( s_searchSpecifier->CanUseFastSearch() ? FileSearcher::fsFAST_STRING : FileSearcher::fsTABSAFE_STRING
      , mh.sm()
      , *s_searchSpecifier
      , mh
      ));
   if( !pSrchr )  return false;
   mh.InitLogFile( *pSrchr );

   pathbuf gen_info;
   auto pGen( GrepMultiFilenameGenerator( gen_info, sizeof gen_info ) );
   if( pGen ) { 1 && DBG( "%s using %s", __PRETTY_FUNCTION__, gen_info );
      Path::str_t pbuf;
      while( pGen->VGetNextName( pbuf ) )
         MFGrepProcessFile( pbuf.c_str(), pSrchr );

      Delete0( pGen );
      }
   else {
      ErrorDialogBeepf( "GrepMultiFilenameGenerator -> nil" );
      }

   Delete0( pSrchr );

   mh.ShowResults();
   return mh.VOverallRetval();
   }


#if USE_PCRE
STIL bool CheckRegExpReplacementString( PRegex, PCChar ) { return false; }
#endif

STATIC_FXN void MFReplaceProcessFile( PCChar filename, ReplaceCharWalker *pMrcw ) {
   const auto pFBuf( FBOP::FindOrAddFBuf( filename ) );
   0 && DBG( "d_dhdViewsOfFBUF.IsEmpty(%s)==%c", filename, pFBuf->ViewCount()==0?'t':'f' );
   const auto fWeCanGarbageCollectFBUF( !pFBuf->HasLines() );
   if( pFBuf->RefreshFailedShowError() )
      return;

   ++pMrcw->d_iReplacementFileCandidates;
   const auto oldReplacementsMade( pMrcw->d_iReplacementsMade );

   Rect rgnSearch( pFBuf );
   CharWalkRect( pFBuf, rgnSearch, Point( rgnSearch.flMin, 0, -1 ), true, pMrcw );

   if( oldReplacementsMade == pMrcw->d_iReplacementsMade ) {
      GarbageCollectFBUF( pFBuf, fWeCanGarbageCollectFBUF );
      }
   else { 0 && DBG( "%s CHANGED '%s'", __func__, pFBuf->Name() );
      ++pMrcw->d_iReplacementFiles;
      }
   }


bool ARG::GenericReplace( bool fInteractive, bool fMultiFileReplace ) {
   STATIC_CONST char szReplace[] = "Replace string: "; // these two defined adjacently so ...
   STATIC_CONST char szSearch [] = "Search string:  "; // ... they are kept the same length

   DispDoPendingRefreshesIfNotInMacro();
   {
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( g_SnR_szSearch, szSearch, 0, nullptr, gts_DfltResponse+gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
   if( !pCmd || pCmd->IsFnCancel() || g_SnR_szSearch.empty() )
      return false;
   }

 #if REPLC_CLASSES

   if( !SetNewSearchSpecifierOK( g_SnR_szSearch.c_str(), nullptr, d_cArg >= 2 ) ) {
      return false; // RegexCompile internally shows diagnostics, but doesn't hv pause logic of ErrorDialogBeepf
      }

 #else

   #if USE_PCRE
   s_fSearchNReplaceUsingRegExp = d_cArg >= 2;
   if( s_fSearchNReplaceUsingRegExp ) {
      RegexDestroy( s_pSandR_CompiledSearchPattern );
      s_pSandR_CompiledSearchPattern = RegexCompile( g_SnR_szSearch.c_str(), g_fCase );
      if( !s_pSandR_CompiledSearchPattern ) {
         return false; // RegexCompile internally shows diagnostics, but doesn't hv pause logic of ErrorDialogBeepf
         }
      }
   #endif

 #endif

   {
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( g_SnR_szReplacement, szReplace, 0, nullptr, gts_DfltResponse+gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
   if( !pCmd || pCmd->IsFnCancel() )
      return false;
   }

   if( fMultiFileReplace && g_SnR_szReplacement.c_str()[0] == 0 && !ConIO::Confirm( "Empty replacement string, confirm: " ) )
      return false;

#if USE_PCRE
   if(   s_fSearchNReplaceUsingRegExp
      && !CheckRegExpReplacementString( s_pSandR_CompiledSearchPattern, g_SnR_szReplacement.c_str() )
     )
      return ErrorDialogBeepf( "Invalid replacement pattern" );
#endif

   ReplaceCharWalker mrcw( fInteractive, d_fMeta ? !g_fCase : g_fCase );

   if( fMultiFileReplace ) {
      const auto startingTopFbuf( g_CurFBuf() );
      auto pGen( GrepMultiFilenameGenerator() );
      if( !pGen ) {
         ErrorDialogBeepf( "GrepMultiFilenameGenerator -> nil" );
         }
      else {
         Path::str_t pbuf;
         while( pGen->VGetNextName( pbuf ) )
            MFReplaceProcessFile( pbuf.c_str(), &mrcw );

         Delete0( pGen );

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
                                                              , (fInteractive && ExecutionHaltRequested()) ? " INTERRUPTED!" : ""
            );
         }
      }
   else {
      Rect rgnSearch( true );

      switch( d_argType ) {
       default:        break;
       case NOARG:     break;
       case NULLARG:   break;

       case LINEARG:   rgnSearch.flMin.col = 0;
                       rgnSearch.flMax.col = COL_MAX;
                       rgnSearch.flMin.lin = d_linearg.yMin;
                       rgnSearch.flMax.lin = d_linearg.yMax;
                       break;

       case STREAMARG: if( d_streamarg.flMin.lin == d_streamarg.flMax.lin ) {
                          rgnSearch = d_streamarg;
                          }
                       else {
                          rgnSearch.flMin.col = 0;
                          rgnSearch.flMax.col = COL_MAX;
                          rgnSearch.flMin.lin = d_streamarg.flMin.lin;
                          rgnSearch.flMax.lin = d_streamarg.flMax.lin - 1;

                          CharWalkRect( g_CurFBuf(), rgnSearch, Point( rgnSearch.flMin.lin, d_streamarg.flMin.col - 1 ), true, &mrcw );

                          rgnSearch.flMax.col = d_streamarg.flMax.col;
                          rgnSearch.flMax.lin++;
                          rgnSearch.flMin.lin = rgnSearch.flMax.lin;
                          }
                       break;

       case BOXARG:    rgnSearch = d_boxarg;
                       break;
       }

      0 && DBG( "%s: rgnSearch=LINEs(%d-%d) COLs(%d,%d)", __func__, rgnSearch.flMin.lin, rgnSearch.flMax.lin, rgnSearch.flMin.col, rgnSearch.flMax.col );

      CharWalkRect( g_CurFBuf(), rgnSearch, Point( rgnSearch.flMin, 0, -1 ), true, &mrcw );

      Msg( "%d of %d occurrences replaced%s"
         , mrcw.d_iReplacementsMade
         , mrcw.d_iReplacementsPoss
         , (fInteractive && ExecutionHaltRequested()) ? " INTERRUPTED!" : ""
         );
      }

   // 20060526 klg commented out since if user hit 'Q' he probably wants to
   //              terminate any macro that this CMD may have been invoked within
   //
   // if( USER_CHOSE_EARLY_CMD_TERMINATE == ExecutionHaltRequested() )
   //    ClrExecutionHaltRequest(); // ExecutionHaltRequest is used in this code to implement the 'Q' response to interactive query

   return mrcw.d_iReplacementsMade != 0;
   }

bool ARG::mfreplace() { return GenericReplace( true , true  ); }
bool ARG::qreplace()  { return GenericReplace( true , false ); }
bool ARG::replace()   { return GenericReplace( false, false ); }

void FBOP::InsLineSorted_( PFBUF fb, PXbuf xb, bool descending, LINE ySkipLeading, PCChar ptr, PCChar eos ) {
   if( !eos ) eos = Eos( ptr );
   const auto cmpSignMul( descending ? -1 : +1 );

   // find insertion point using binary search
   //
   auto yMin( ySkipLeading );
   auto yMax( fb->LastLine() );

   while( yMin <= yMax ) {
      //                ( (yMax + yMin) / 2 );           // old overflow-susceptible version
      const auto cmpLine( yMin + ((yMax - yMin) / 2) );  // new overflow-proof version
      const auto xbChars( fb->getLineTabxPerRealtabs( xb, cmpLine ) );
      CPCChar pXb( xb->c_str() );
      auto cmp( stricmp_eos( ptr, eos, pXb, pXb+xbChars ) * cmpSignMul );
      if( 0 == cmp ) {
         cmp = strcmp_eos( ptr, eos, pXb, pXb+xbChars ) * cmpSignMul;
         if( 0 == cmp )
            return; // drop DUPLICATES!
         }
      if( cmp > 0 )  yMin = cmpLine + 1;
      if( cmp < 0 )  yMax = cmpLine - 1;
      }

   fb->InsLine( yMin, ptr, eos, xb );
   }


STATIC_FXN void InsFnm( PFBUF pFbuf, PXbuf pxb, PCChar fnm, const bool fSorted ) {
   auto pb( fnm );
   const auto ch( Path::DelimChar( fnm ) );
   char pbx[sizeof(pathbuf)+2];
   if( ch ) {
      pb = safeSprintf( BSOB(pbx), "%c%s%c", ch, fnm, ch );
      }
   if( fSorted ) FBOP::InsLineSortedAscending( pFbuf, pxb, 0, pb ); else pFbuf->PutLastLine( pb );
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
         safeStrcpy( BSOB(wcBuf) , pStart+1         , pVbar  );
         safeStrcpy( BSOB(dirBuf), pszWildcardString, pStart );
         }
      else {
         safeStrcpy( BSOB(wcBuf) , pszWildcardString, pVbar  );
         safeStrcpy( BSOB(dirBuf), ".\\" );
         }
      ED && DBG( "wcBuf='%s'" , wcBuf  );
      ED && DBG( "dirBuf='%s'", dirBuf );

      Path::str_t pbuf, fbuf;
      DirListGenerator dlg( dirBuf );
      Xbuf xb;
      while( dlg.VGetNextName( pbuf ) ) {                          ED && DBG( "pbuf='%s'", pbuf.c_str() );
         WildcardFilenameGenerator wcg( FmtStr<_MAX_PATH>( "%s" PATH_SEP_STR "%s", pbuf.c_str(), wcBuf ), ONLY_FILES );
         fbuf.clear();
         while( wcg.VGetNextName( fbuf ) ) {
            InsFnm( fb, &xb, fbuf.c_str(), fSorted );
            ++rv;
            }
         }
      }
   else {
      CfxFilenameGenerator wcg( pszWildcardString, FILES_AND_DIRS );
      Xbuf xb;
      Path::str_t fbuf;
      while( wcg.VGetNextName( fbuf ) ) {
         const auto chars( fbuf.length() );  ED && DBG( "wcg=%s", fbuf.c_str() );
         if( chars > 2 && strcmp( fbuf.c_str()+chars-2, PATH_SEP_STR "." ) == 0 )
            continue; // drop the meaningless "." entry:

         InsFnm( fb, &xb, fbuf.c_str(), fSorted );
         ++rv;
         }
      }
   return rv;
   }

STATIC_FXN PCChar searchFindString( PCChar pBuf, COL charsToSearch, PCChar pSrchKey, COL SrchKeyLen ) {
   0 && DBG( "searchFindString charsToSearch=%d SrchKeyLen=%d '%s'", charsToSearch, SrchKeyLen, pBuf );
   if( SrchKeyLen > charsToSearch ) { 0 && DBG( "searchFindString:0  SrchKeyLen > charsToSearch" );  // couldn't possibly have a match
      return nullptr;
      }

#if 0
   // removed since Assert IS being hit
   const auto schars( Strlen( pBuf ) );
   if( !(schars >= charsToSearch) ) {
      DBG( "!!! strlen( pBuf )[%d] < charsToSearch[%d]", schars, charsToSearch );
      }

   Assert( schars >= charsToSearch );
#endif

   const auto pFirstMatch( PCChar( memchr( pBuf, pSrchKey[0], charsToSearch ) ) );
   if( pFirstMatch ) {
      auto ixKey(0);
      auto ixBuf(pFirstMatch - pBuf);
      while( ixBuf < charsToSearch ) {
         if( pSrchKey[ixKey++] == pBuf[ixBuf++] ) {
            if( ixKey >= SrchKeyLen )
               return pBuf + ixBuf - SrchKeyLen; // MATCH!
            }
         else {
            ixBuf -= ixKey - 1;
            ixKey = 0;
            }
         }
      }

   return nullptr;
   }

//===============================================

bool SearchSpecifier::CaseUpdt() {
#if USE_PCRE
   if( d_re && (d_fRegexCase != g_fCase) ) {
      d_fRegexCase = g_fCase;

      // recompile
      Delete0( d_re );
      d_re = RegexCompile( d_rawStr, d_fRegexCase );
      if( !d_re )
         d_reCompileErr = true;
      }
#endif
   return g_fCase;
   }

void SearchSpecifier::Build( PCChar rawStr, int rawStrLen, bool fRegex ) {  // Assert( fRegex && rawStr ); // if fRegex true then rawStr cannot be 0
#if USE_PCRE
   d_re = nullptr;
   d_fRegexCase = g_fCase;
   d_reCompileErr = false;
#endif
   if( !rawStr ) {
      d_rawStrLen = 0;
      d_rawStr    = nullptr;
      }
   else {
      d_rawStrLen = rawStrLen;
      d_rawStr = Strdup( rawStr, d_rawStrLen );
#if USE_PCRE
      if( fRegex ) {
         d_fCanUseFastSearch = false;
         d_re = RegexCompile( d_rawStr, d_fRegexCase );
         d_reCompileErr = (d_re == nullptr);
         }
      else
#endif
         {
         d_fCanUseFastSearch = !strchr( d_rawStr, ' ' ) && !strchr( d_rawStr, HTAB ) ;
         }
      }
   }


SearchSpecifier::SearchSpecifier( PCChar rawStr, int rawStrLen, bool fRegex ) {
   Build( rawStr, rawStrLen, fRegex );
   }

SearchSpecifier::~SearchSpecifier() {
   Free0( d_rawStr );
#if USE_PCRE
   RegexDestroy( d_re );
#endif
   }

void SearchSpecifier::Dbgf( PCChar tag ) const {
  #if 1
   DBG( "SearchSpecifier %s: cs=%d, rex=%d, rerr=%d, raw=%s'"
      , tag
#if USE_PCRE
      , d_fRegexCase
      , d_re != nullptr
      , d_reCompileErr
#else
      , g_fCase
      , false
      , false
#endif
      , d_rawStr
      );
  #endif
   }

//===============================================

FileSearcher::FileSearcher( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh, int capturesNeeded )
   : d_sm   ( sm )
   , d_ss   ( ss )
   , d_mh   ( mh )
   , d_pFBuf( nullptr  )
   , d_pCaptures( new CapturedStrings( capturesNeeded ) )
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
   , d_searchKeyStrlen( Strlen(ss.d_rawStr) )
   , d_searchKey( Strdup( ss.d_rawStr, d_searchKeyStrlen ) )
   {
   if( !g_fCase )
      _strlwr( d_searchKey );
   }

void FileSearcherString::VPrepLine_( PChar lbuf ) const {
   if( !g_fCase )
      _strlwr( lbuf );
   }

PCChar FileSearcherString::VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const {
   const auto rv( searchFindString( pBuf+startingBufOffset, bufChars-startingBufOffset, d_searchKey, d_searchKeyStrlen ) );
   *pMatchChars = rv ? d_searchKeyStrlen : 0;
   return rv;
   }

//===============================================

#if USE_PCRE

FileSearcherRegex::FileSearcherRegex( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : FileSearcher( sm, ss, mh )
   {
   Assert( d_ss.IsRegex() );
   if( !d_ss.HasError() )
      d_pCaptures = new CapturedStrings( d_ss.GetRegex()->MaxPossCaptures() );
   }

#endif

STATIC_FXN PCChar ShowHaystackHas( HaystackHas has ) {
   switch( has ) {
      default:                   return "???";
      case STR_HAS_BOL_AND_EOL:  return "STR_HAS_BOL_AND_EOL";
      case STR_MISSING_BOL    :  return "STR_MISSING_BOL";
      case STR_MISSING_EOL    :  return "STR_MISSING_EOL";
      }
   }

#if USE_PCRE

PCChar FileSearcherRegex::VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const
   {
   VS_(
               DBG( "++++++" );
               DBG( "RegEx?[%d-],%s='%*.*s'", startingBufOffset, ShowHaystackHas(lineContent), bufChars - startingBufOffset, bufChars - startingBufOffset, pBuf + startingBufOffset );
      )
   const auto rv( d_ss.d_re->Match( startingBufOffset, pBuf, bufChars, pMatchChars, lineContent, d_pCaptures ) );
   VS_(
      if( rv ) DBG( "RegEx:->MATCH=(%d L %d)='%*.*s'", rv - pBuf, *pMatchChars, *pMatchChars, *pMatchChars, rv );
      else     DBG( "RegEx:->NO MATCH" );
               DBG( "------" );
      )
   return rv;
   }

#endif

//===============================================

FileSearcherFast::FileSearcherFast( const SearchScanMode &sm, const SearchSpecifier &ss, FileSearchMatchHandler &mh )
   : FileSearcher( sm, ss, mh )
   , d_pNeedleLens( nullptr )
   , d_needleCount( 1 ) // last needle is \0 terminated, so count it now
   {
   d_searchKeyStrlen = ss.d_rawStrLen;
   auto pS( ss.d_rawStr );

   const char AltSepChar
      (
         (      '!' == pS[0]
          &&
             (  ',' == pS[1]
             || '|' == pS[1]
             || '.' == pS[1]
             )
         ) ? pS[1] : 0
      );

   auto fNdAppendTrailingAltSepChar( 0 );
   if( AltSepChar ) {
      pS                += 2;
      d_searchKeyStrlen -= 2;

      // IF ALTERNATION BEING USED, LAST CHAR MUST AltSepChar!
      // Append if necessary

      if( pS[ d_searchKeyStrlen-1 ] != AltSepChar ) {
         ++d_searchKeyStrlen;
         fNdAppendTrailingAltSepChar = 1;
         }
      }

   AllocArrayNZ( d_searchKey, d_searchKeyStrlen + 1 + fNdAppendTrailingAltSepChar );
   memcpy( d_searchKey, pS, d_searchKeyStrlen );
   pS = d_searchKey + d_searchKeyStrlen;
   if( fNdAppendTrailingAltSepChar )
      *pS++ = AltSepChar; // arg "!,AltSepChar,fNdAppendTrailingAltSepChar" grep
                          // arg arg "(AltSepChar|fNdAppendTrailingAltSepChar)" grep
   *pS = '\0';

   if( !g_fCase ) {
      _strlwr( d_searchKey );
      d_pfxStrnstr = strnstri;
      }
   else {
      d_pfxStrnstr = strnstr;
      }

   // gateway to alternation (logical OR) in grep/TEXTARG: "^![,.|]"
   // replace the user's chosen separator with the separator that
   // FileSearcherFast::FindMatches requires
   //
   0 && DBG( "srchStrLen=%d: '%s'", d_searchKeyStrlen, d_searchKey );

   if( AltSepChar ) { 0 && DBG( "%s ALTN!", __func__ );
      for( PCChar pCC(d_searchKey) ; *pCC != 0 ; ++pCC )
         if( AltSepChar == *pCC )
             ++d_needleCount;

      d_pNeedleLens = new int [d_needleCount];

      auto ix(0);
      auto pStart(d_searchKey);
      PChar pC; // want pC beyond for loop
      for( pC=d_searchKey ; *pC != 0 ; ++pC ) {
         if( AltSepChar == *pC ) {
             *pC = 0;
             0 && DBG( " '%s'", pC+1 );
             d_pNeedleLens[ ix++ ] = pC - pStart;
             pStart = pC + 1;
             }
         }
      d_pNeedleLens[ ix ] = pC - pStart;

      0 && DBG( "needleCount=%d", d_needleCount );
      }
   else {
      0 && DBG( "needleCount=%d", d_needleCount );
      d_pNeedleLens = new int [d_needleCount];
      d_pNeedleLens[0] = Strlen( d_searchKey );
      }

   // for( int ix(0) ; ix < d_needleCount ; ++ix ) {
   //    DBG( "NeedleLen[%d]=%d", ix, d_pNeedleLens[ ix ] );
   //    }
   }

FileSearcherFast::~FileSearcherFast() {
   Free_( d_searchKey );
   delete [] d_pNeedleLens;
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
      PCChar pLine; size_t chars;
      if( d_pFBuf->PeekRawLineExists( curPt.lin, &pLine, &chars ) ) { // loop across all needles in [d_searchKey .. d_searchKey + d_searchKeyStrlen - 1]
SEARCH_REMAINDER_OF_LINE_AGAIN:
         auto lix(0);
         for( auto needle(d_searchKey) ; needle < d_searchKey + d_searchKeyStrlen ; ++lix ) {
            const auto needleLen( NeedleLen( lix ) );
            0 && DBG( "NeedleLen[%d]=%d", lix, needleLen );
            if( needleLen <= chars - curPt.col ) {
               const auto pMatchStart( d_pfxStrnstr( pLine + curPt.col, chars - curPt.col, needle, needleLen ) );
               if( pMatchStart ) {
                  // To prevent the highlight from being misaligned,
                  // FoundMatchContinueSearching needs to be given a tab-corrected
                  // colMatchStart value.  d_searchKeyStrlen is perfectly
                  // adequate/correct because we won't be using FileSearcherFast if
                  // _the key_ contains spaces or tabs
                  //
                  const auto colMatchStart( ColOfPtr( tw, pLine, pMatchStart, pLine+chars ) );
                  Point matchPt( curPt.lin, colMatchStart );
                  if( !d_mh.FoundMatchContinueSearching( d_pFBuf, matchPt, d_searchKeyStrlen, nullptr ) )
                     return;

                  curPt.col = (pMatchStart - pLine) + d_searchKeyStrlen;
                  goto SEARCH_REMAINDER_OF_LINE_AGAIN;
                  }
               }
            needle += needleLen + 1;
            }
         }
      }
   }

// these stubs are required by the compiler, but will NEVER be called since
// FileSearcherFast::VFindMatches_ REPLACES FileSearcher::VFindMatches_, and
// FileSearcherFast::VFindMatches_ DOES NOT CALL OTHER CLASS METHODS
//
void   FileSearcherFast::VPrepLine_( PChar lbuf ) const { Assert( 0 != 0 ); }
PCChar FileSearcherFast::VFindStr_( COL startingBufOffset, PCChar pBuf, COL bufChars, COL *pMatchChars, HaystackHas lineContent ) const { Assert( 0 != 0 ); return nullptr; }

//===============================================


class PtrCol {
   const COL    d_tw;
   const PCChar d_pStart;
   const PCChar d_pEos;

public:
   PtrCol( const COL tw, const PCChar pStart, const PCChar pEos )
      : d_tw    (  tw      )
      , d_pStart(  pStart  )
      , d_pEos  (  pEos    )
      {}

   COL    p2c ( PCChar pC   ) const { return ColOfPtr( d_tw, d_pStart, pC    , d_pEos ); }
   PCChar c2p_( COL    xCol ) const { return PtrOfColWithinStringRegion     ( d_tw, d_pStart, d_pEos, xCol   ); }
   PCChar c2p ( COL    xCol ) const { return PtrOfColWithinStringRegionNoEos( d_tw, d_pStart, d_pEos, xCol   ); }
   COL    cols(             ) const { return StrCols( d_tw, d_pStart, d_pEos ); }
   };

void FileSearcher::VFindMatches_() {
   const auto tw( d_pFBuf->TabWidth() );
   // VS_( DBG( "%csearch: START  y=%d, x=%d", d_sm.d_fSearchForward?'+':'-', d_start.lin, d_start.col ); )
   if( d_sm.d_fSearchForward ) {
      VS_( DBG( "+search: START  y=%d, x=%d", d_start.lin, d_start.col ); )
      for( auto curPt(d_start) ; curPt < d_end && !ExecutionHaltRequested() ; ++curPt.lin, curPt.col = 0 ) {
         //***** Search A LINE:
         const auto lnChars( d_pFBuf->getLineTabxPerTabDisp( &d_xb, curPt.lin ) );
         VPrepLine_( d_xb.wbuf() );
         const auto bos( d_xb.c_str() );
         const auto eos( bos+lnChars );
         const PtrCol pcc( tw, bos, eos );
         const auto lnCols( pcc.cols() );

         PCChar pC( pcc.c2p( curPt.col ) );
         if( pcc.p2c( pC ) != curPt.col )  // curPt.col is in a tab-spring, which means (a) curPt.col > 0, and (b) pC is pointing at a char outside the replace region[1]
            ++pC;                          // move pC to point to first char in replace region  [1] but BUGBUG this fxn is not used by replace!

         // find all matches on this line
         for( auto xCol(curPt.col) ; xCol <= lnCols // <= so empty line can match Regex
            ; pC   = pcc.c2p( curPt.col ) + 1,
              xCol = pcc.p2c( pC )
            ) {
            COL matchChars;
            pC = VFindStr_( pC - bos, bos, lnChars, &matchChars, STR_HAS_BOL_AND_EOL );
            if( nullptr == pC )
               break; // no matches on this line!

            //*****  HOUSTON, WE HAVE A MATCH  *****

            curPt.col  =          pcc.p2c( pC )                           ;
            const auto matchCols( pcc.p2c( pC + matchChars ) - curPt.col );

            if( !d_mh.FoundMatchContinueSearching( d_pFBuf, curPt, matchCols, d_pCaptures ) )
               return;

            // NoLessThan( &matchCols, 1 );  // so += matchCols will always move fwd
            }
         }
      }
   else { // search backwards (only msearch uses this; more complex)
      VS_( DBG( "-search: START  y=%d, x=%d", d_start.lin, d_start.col ); )
      for( auto curPt(d_start) ; curPt > d_end && !ExecutionHaltRequested() ; --curPt.lin, curPt.col = COL_MAX ) {
         const auto lnChars( d_pFBuf->getLineTabxPerTabDisp( &d_xb, curPt.lin ) );
         VPrepLine_( d_xb.wbuf() );
         const auto bos( d_xb.c_str() );
         const auto eos( bos+lnChars );
         const PtrCol pcc( tw, bos, eos );

         auto pLast( pcc.c2p( curPt.col ) );
         if( *pLast != 0 ) // if curPt.col is in middle of line...
            ++pLast;       // ... nd to incr to get correct maxCharsToSearch

         const auto maxCharsToSearch( pLast - bos );

         0 && DBG( "MaxCh2s=%" PR_PTRDIFFT "d", maxCharsToSearch );

         // works _unless_ cursor is at EOL when 'arg arg "$" msearch'; in this
         // case, it keeps finding the EOL under the cursor (doesn't move to
         // prev one)
         //
         #define  SET_HaystackHas(startOfs)  (startOfs+maxCharsToSearch == lnChars ? STR_HAS_BOL_AND_EOL : STR_MISSING_EOL)

         COL matchChars;
         auto pGoodMatch( VFindStr_( 0, bos, maxCharsToSearch, &matchChars, SET_HaystackHas(0) ) );
         if( pGoodMatch ) { // line contains a match?
            auto goodMatchChars( matchChars );
            VS_( DBG( "-search: LMATCH y=%d (%d L %d)='%*.*s'", curPt.lin, pGoodMatch - bos, matchChars, goodMatchChars, goodMatchChars, pGoodMatch ); )

            // the next loop is really nasty, so we add a deadman counter to break out of infinite loops (like 'arg "$" psearch')
        #define DEADMAN_CHK 0
        #if DEADMAN_CHK
            int deadman = maxCharsToSearch+1;
        #endif
            while( pGoodMatch < bos + maxCharsToSearch ) {
               const auto startIdx( pGoodMatch + 1 - bos );
               const auto pNextMatch( VFindStr_( startIdx, bos, maxCharsToSearch, &matchChars, SET_HaystackHas(startIdx) ) );
               if( !pNextMatch )
                  break;

               pGoodMatch = pNextMatch;
               goodMatchChars = matchChars;

               VS_( DBG( "-search: +MATCH y=%d (%d L %d)='%*.*s'", curPt.lin, pGoodMatch - bos, goodMatchChars, goodMatchChars, goodMatchChars, pGoodMatch ); )

        #if DEADMAN_CHK
               if( --deadman == 0 ) {
                  Msg( "internal error, %s inner loop hit deadman iteration limit", __func__ );
                  return;
                  }
        #endif
               }

            curPt.col  =          pcc.p2c( pGoodMatch                            )              ;
            const auto matchCols( pcc.p2c( goodMatchChars + pcc.c2p( curPt.col ) ) - curPt.col );

            VS_( DBG( "-search: !MATCH y=%d (%d L %d)=>COL(%d L %d)", curPt.lin, pGoodMatch - bos, goodMatchChars, curPt.col, matchCols ); )
            if( !d_mh.FoundMatchContinueSearching( d_pFBuf, curPt, matchCols, d_pCaptures ) )
               return;
            }
         }
      }
   }


// not my proudest moment, but replicating the code below everywhere would be immoral!

FileSearcher *NewFileSearcher(
     FileSearcher::StringSearchVariant  type
   , const SearchScanMode              &sm
   , const SearchSpecifier             &ss
   , FileSearchMatchHandler            &mh
   )
   {
   VS_( DBG( "NewFileSearcher enter" ); )
   FileSearcher *rv;
#if USE_PCRE
   if( ss.IsRegex() ) {
      if( ss.HasError() )
         return nullptr;

      rv = new FileSearcherRegex( sm, ss, mh );
      VS_( DBG( "  FileSearcherRegex %p", rv ); )
      }
   else
#endif
      {
      switch( type ) {
         default:
              Assert( 0 != 0 );
              return nullptr;

         case FileSearcher::fsTABSAFE_STRING:
              rv = new FileSearcherString( sm, ss, mh );
              VS_( DBG( "  FileSearcherString %p", rv ); )
              break;

         case FileSearcher::fsFAST_STRING:
              rv = new FileSearcherFast( sm, ss, mh );
              VS_( DBG( "  FileSearcherFast %p", rv ); )
              break;
         }
      }
   VS_( rv->Dbgf(); )
   return rv;
   }


bool GenericSearch( ARG *pArg, PCSearchScanMode sm ) {
   if( !SearchSpecifierOK( pArg ) )
      return false;

   Point                        curPt;
   if(      &smBackwd == sm ) { curPt = Point( g_CurView()->Cursor(), 0, -1 ); }
   else if( &smFwd == sm    ) { curPt = Point( g_CurView()->Cursor(), 0, +1 ); }
   else {   /*suppress warning*/curPt = Point(0,0);  Assert( !"invalid sm value" ); }

   auto mh( new FindPrevNextMatchHandler( sm->d_fSearchForward, s_searchSpecifier->IsRegex(), s_searchSpecifier->SrchStr() ) );

   auto pSrchr( NewFileSearcher( FileSearcher::fsTABSAFE_STRING, *sm, *s_searchSpecifier, *mh ) );
   if( !pSrchr ) {
      Delete0( mh );
      return false;
      }

   pSrchr->SetInputFile();
   pSrchr->SetBoundsToEnd( curPt );
   pSrchr->FindMatches();
   Delete0( pSrchr );

   mh->ShowResults();
   const auto rv( mh->VOverallRetval() );
   Delete0( mh );

   return rv;
   }


bool ARG::msearch()   { return GenericSearch( this, &smBackwd ); }
bool ARG::psearch()   { return GenericSearch( this, &smFwd    ); }


//-------------------------- pbal

struct CharWalkerPBal : public CharWalker {
   const bool d_fFwd;
   const bool d_fHiliteMatch;
   linebuf    d_stack;
   int        d_stackIx;

   bool       d_fClosureFound;
   Point      d_closingPt;

   void Push( char ch ) { /*DBG("Push[%3d]'%c'",d_stackIx,ch);*/ d_stack[ d_stackIx++ ] = ch;   }
   char Pop()           { const char rv(d_stack[ --d_stackIx ]); /*DBG("Pop [%3d]'%c'",d_stackIx,rv);*/ return rv; }

   public:

   CharWalkerPBal( bool fFwd, bool fHiliteMatch, char chStart )
      : d_fFwd( fFwd )
      , d_fHiliteMatch( fHiliteMatch )
      , d_stackIx( 0 )
      , d_fClosureFound( false )
      { /*DBG("");*/ Push( chStart ); }

   CheckNextRetval VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) override;
   };

#undef XXX

CheckNextRetval CharWalkerPBal::VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) {
   const auto pPt( PtrOfColWithinStringRegion( pFBuf->TabWidth(), ptr, eos, curPt->col ) );
   const char ch( pPt[ 0 ] );

   // if( !ch ) return CONTINUE_SEARCH;     test HACK

   PCChar pA, pB;
   if( d_fFwd ) { pA = g_delims      ; pB = g_delimMirrors; }
   else         { pA = g_delimMirrors; pB = g_delims      ; }

   CPCChar pE( strchr( pA, ch ) );
   if( pE ) {
      if( d_stackIx >= ELEMENTS(d_stack)-1 ) {
         Msg( "%cbal STACK OVERFLOW at X=%d, Y=%d", d_fFwd ? '+' : '-', curPt->col+1, curPt->lin+1 );
         return STOP_SEARCH;
         }

      0 && DBG( "%cbal [%d] d_stackIx PUSH '%c' at X=%d, Y=%d", d_fFwd ? '+' : '-', d_stackIx, ch, curPt->col+1, curPt->lin+1 );
      Push( ch );
      return CONTINUE_SEARCH;
      }
   else {
      CPCChar p3( strchr( pB, ch ) );
      if( p3 ) {
         const auto closes( pA[p3-pB] );
         const auto shouldClose( Pop() );
         if( shouldClose != closes ) {
            if( d_fHiliteMatch ) {
               Msg( "%cbal [%d] MISMATCH is '%c' != s/b '%c' at X=%d, Y=%d"
                     , d_fFwd ? '+' : '-'
                            , d_stackIx      , ch        , shouldClose
                                                                  , curPt->col+1
                                                                        , curPt->lin+1 );
               }
            return STOP_SEARCH;
            }

         if( d_stackIx > 0 ) { 0 && DBG( "%cbal [%d] MATCH '%c' at X=%d, Y=%d", d_fFwd ? '+' : '-', d_stackIx, ch, curPt->col+1, curPt->lin+1 );
            return CONTINUE_SEARCH;
            }

         if( d_fHiliteMatch ) {
            Msg( "%cbal CLOSURE at X=%d, Y=%d", d_fFwd ? '+' : '-', curPt->col+1, curPt->lin+1 );
            }

         d_fClosureFound = true;
         d_closingPt     = *curPt;
         return STOP_SEARCH;
         }
      }
   return CONTINUE_SEARCH;
   }


char View::CharUnderCursor() {
   Xbuf xb;
   const auto chars( g_CurFBuf()->getLineTabx( &xb, Cursor().lin ) );
   return Cursor().col >= chars ? 0 : xb.c_str()[ Cursor().col ];
   }

bool View::PBalFindMatching( bool fSetHilite, Point *pPt ) {
   const auto startCh( CharUnderCursor() );
   if( !startCh ) return false;

   bool fSearchFwd;
   if(      strchr( g_delims      , startCh ) ) fSearchFwd = true;
   else if( strchr( g_delimMirrors, startCh ) ) fSearchFwd = false;
   else return false;

   Rect rgnSearch( fSearchFwd );
   CharWalkerPBal chSrchr( fSearchFwd, fSetHilite, startCh );
   CharWalkRect( d_pFBuf, rgnSearch, Cursor(), fSearchFwd, &chSrchr );
   if( chSrchr.d_fClosureFound ) {
      if( pPt )         *pPt = chSrchr.d_closingPt;
      if( fSetHilite )  SetMatchHiLite( chSrchr.d_closingPt, 1, true );
      }
   return chSrchr.d_fClosureFound;
   }


bool ARG::balch() {
   const FBufLocnNow cp;
   PCV;
   Point pt;
   if( pcv->PBalFindMatching( false, &pt ) )
       pcv->MoveCursor( pt.lin, pt.col );

   return cp.Moved();
   }

bool View::next_balln( LINE yStart, bool fStopOnElse ) {
   const auto pFbuf( FBuf() );
   auto nest(0);
   for( auto iy(yStart + 1) ; iy < pFbuf->LineCount() ; ++iy ) {
      auto fStop(false);
      switch( FBOP::IsCppConditional( pFbuf, iy ) ) {
         case cppcNone : break;
         case cppcIf   : ++nest; break;
         case cppcElif : //lint -fallthrough
         case cppcElse : if( 0 == nest   && fStopOnElse ) { fStop = true; } break;
         case cppcEnd  : if( 0 == nest-- )                { fStop = true; } break;
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
         case cppcNone : break;
         case cppcEnd  : ++nest; break;
         case cppcElif : //lint -fallthrough
         case cppcElse : if( 0 == nest   && fStopOnElse ) { fStop = true; } break;
         case cppcIf   : if( 0 == nest-- )                { fStop = true; } break;
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
      case cppcNone : pcv->prev_balln( d_noarg.cursor.lin, true  ); break;
      case cppcIf   : //lint -fallthrough
      case cppcElif : //lint -fallthrough
      case cppcElse : pcv->next_balln( d_noarg.cursor.lin, true  ); break;
      case cppcEnd  : pcv->prev_balln( d_noarg.cursor.lin, false ); break;
      }

   return cp.Moved();
   }

//-------------------------- pword/mword

class PMWordCharWalker : public CharWalker {
   bool d_fPMWordSearchMeta;

   public:

   PMWordCharWalker( bool fPMWordSearchMeta ) : d_fPMWordSearchMeta( fPMWordSearchMeta ) {}
   CheckNextRetval VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) override;
   };

CheckNextRetval PMWordCharWalker::VCheckNext( PFBUF pFBuf, PCChar ptr, PCChar eos, Point *curPt, int *pColLastPossibleLastMatchChar ) {
   CPCChar pPt( PtrOfColWithinStringRegion( pFBuf->TabWidth(), ptr, eos, curPt->col ) );
   // if( pPt - ptr != curPt->col ) { DBG( "C2P x=%d -> %d", curPt->col, pPt - ptr ); }
   if( false == d_fPMWordSearchMeta ) { // rtn true iff curPt->col is FIRST CHAR OF WORD
      if( !isWordChar( pPt[0] ) )
         return CONTINUE_SEARCH;

      if(   curPt->col > 0
         && isWordChar( pPt[-1] )
        )
         return CONTINUE_SEARCH;

      curPt->ScrollTo();
      return STOP_SEARCH;
      }
   else { // rtn true iff curPt->col is LAST CHAR OF WORD
      if( curPt->col <= 0 )
         return CONTINUE_SEARCH;

      if( !isWordChar( pPt[-1] ) )
         return CONTINUE_SEARCH;

      if( !isWordChar( pPt[0] ) ) {
         curPt->ScrollTo();
         return STOP_SEARCH;
         }

      if( curPt->col != *pColLastPossibleLastMatchChar )
         return CONTINUE_SEARCH;

      g_CurView()->MoveCursor( curPt->lin, curPt->col+1 );
      return STOP_SEARCH;
      }
   }


STATIC_FXN bool PMword( bool fSearchFwd, bool fMeta ) {
   const FBufLocnNow cp;

   Rect rgnSearch( fSearchFwd );
   PMWordCharWalker chSrchr( fMeta );
   CharWalkRect( g_CurFBuf(), rgnSearch, g_CurView()->Cursor(), fSearchFwd, &chSrchr );

   return cp.Moved();
   }

bool ARG::pword() { return PMword( true , (ArgCount() > 0) ? !d_fMeta : d_fMeta ); }
bool ARG::mword() { return PMword( false,                               d_fMeta ); }

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
   // ORDER IS IMPORTANT HERE because it defines CTOR-call ordering, and there ARE dependencies!!!
   // ORDER IS IMPORTANT HERE because it defines CTOR-call ordering, and there ARE dependencies!!!
   // ORDER IS IMPORTANT HERE because it defines CTOR-call ordering, and there ARE dependencies!!!
   const LINE  d_InfLines;       // FIRST  !!!
   PChar       d_MatchingLines;  // SECOND !!! depends on d_InfLines

   // order not important for the following
   // order not important for the following
   // order not important for the following

   PFBUF         d_SrchFile;
   LINE          d_MetaLineCount;
   MainThreadPerfCounter d_pc;

                 // if !d_fFindAllNegate, do ADDITIVE    search: look for and ADD lines that match
                 // if  d_fFindAllNegate, do SUBTRACTIVE search: look for and RMV lines that match
   const bool    d_fFindAllNegate;

public:

   CGrepper( PFBUF srchfile, LINE MetaLineCount, bool fNegate )
        // ORDER OF FOLLOWING CLAUSES DOES NOT CAUSE THE ORDERING OF THEIR EXECUTION!!!
      : d_InfLines( srchfile->LineCount() )
      , d_MatchingLines( PChar( Alloc0d( sizeof(*d_MatchingLines) * d_InfLines ) ) )
      , d_SrchFile( srchfile )
      , d_MetaLineCount( MetaLineCount )
      , d_fFindAllNegate( fNegate )
      {
      DBG( "CGrepper: d_SrchFile='%s', d_MetaLineCount=%i, srchLines=%d", d_SrchFile->Name(), d_MetaLineCount, d_InfLines );
      // to set up initial conditions
      memset( d_MatchingLines, d_fFindAllNegate, sizeof(*d_MatchingLines) * d_InfLines );
      }

   ~CGrepper() { Free_( d_MatchingLines ); }

   void FindAllMatches( PCChar pSrchStr, bool fUseRegEx );
   void LineMatches( LINE line ) { d_MatchingLines[ line ] = !d_fFindAllNegate; }

   // to write results
   LINE WriteOutput( PCChar thisMetaLine, PCChar origSrchfnm );

private:
   void RmvLine( LINE line ) { d_MatchingLines[ line ] = 0; }

   CGrepper & operator=( const CGrepper &rhs ); // so assignment op can't be used...
   };

//****************************

class CGrepperMatchHandler : public FileSearchMatchHandler {
   NO_ASGN_OPR(CGrepperMatchHandler);

   CGrepper &d_cg;

   public:

   CGrepperMatchHandler( CGrepper &cg ) : d_cg( cg ) {}

   bool VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) override;

   STATIC_CONST SearchScanMode &sm() { return smFwd; }
   };

bool CGrepperMatchHandler::VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, PCCapturedStrings pCaptures ) {
   d_cg.LineMatches( cur.lin );
   return true;  // "action" taken!
   }


//****************************

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

//--------------------------------------------------------------

struct xlatLower {
   U8 my_lower_[256];  // contains lowercase version of its index
   xlatLower();
   };

xlatLower::xlatLower() {
   for( int ix(0); ix < ELEMENTS(my_lower_); ++ix )
      my_lower_[ix] = ix;

   for( int ix('A'); ix <= 'Z'; ++ix )
      my_lower_[ix] |= 'a' - 'A';
   }

char toLower( int ch ) {
   STATIC_VAR xlatLower low;
   return low.my_lower_[ U8(ch) ];
   }

//--------------------------------------------------------------

STATIC_FXN PCChar strnstri( PCChar haystack, int haystackLen, PCChar needle, int needleLen ) { // if fCase==0, ASSUMES needle has been LOWERCASED!!!
   const auto pEnd( haystack + haystackLen - needleLen + 1 ); // stop looking at
   while( haystack < pEnd ) {
      if( toLower( haystack[0] ) == needle[0] ) {
         auto pH( haystack );
         auto pN( needle   );
         do {
            ++pH;
            ++pN;
            if( 0 == *pN )
                return haystack;

            } while( toLower( *pH ) == *pN );
         }
      ++haystack;
      }
   return nullptr;
   }

//
// this is 6 TIMES faster(!) than a version which uses 'intrinsic'
// memcmp which uses rep cmpsd + rep cmpsb + many setup instructions!
//
STATIC_FXN PCChar strnstr( PCChar haystack, int haystackLen, PCChar needle, int needleLen ) { // if fCase==0, ASSUMES needle has been LOWERCASED!!!
   auto pEnd( haystack + haystackLen - needleLen + 1 ); // stop looking at
   while( haystack < pEnd ) {
      if( haystack[0] == needle[0] ) {
         auto pH( haystack );
         auto pN( needle   );
         do {
            ++pH;
            ++pN;
            if( 0 == *pN ) {
                return haystack;
                }
            } while( *pH == *pN );
         }
      ++haystack;
      }
   return nullptr;
   }


void CGrepper::FindAllMatches( PCChar pSrchStr, bool fUseRegEx ) {
   if( !SetNewSearchSpecifierOK( pSrchStr, nullptr, fUseRegEx ) )
      return;

   CGrepperMatchHandler mh( *this );
   auto pSrchr( NewFileSearcher( FileSearcher::fsFAST_STRING, mh.sm(), *s_searchSpecifier, mh ) );

   if( !pSrchr )
      return;

   pSrchr->SetInputFile( d_SrchFile );
   pSrchr->SetBoundsToEnd( Point( d_MetaLineCount, 0 ) );
   pSrchr->FindMatches();
   Delete0( pSrchr );
   }

LINE CGrepper::WriteOutput
   ( PCChar thisMetaLine // MetaLine (string) to be concatenated for this search
   , PCChar origSrchfnm  // if file searched THIS TIME is not file which line#s refer to
   )
   {
   Xbuf xb( d_SrchFile->Name() );
   0 && DBG( " ->%s|", xb.c_str() );
   if( LuaCtxt_Edit::from_C_lookup_glock( &xb ) && !xb.is_clear() ) {
      const auto gbnm( xb.c_str() );
      0 && DBG( "LuaCtxt_Edit::from_C_lookup_glock ->%s|", gbnm );
      const auto outfile( OpenFileNotDir_NoCreate( gbnm ) );
      pathbuf GrepFBufname;
      int     grepHdrLines;
      if( !outfile )                                                          { Msg(    "nonexistent buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      if( !(FBOP::IsGrepBuf( outfile, BSOB(GrepFBufname), &grepHdrLines ) ) ) { Msg(   "non-grep-buf buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      if( !(0 == strcmp( GrepFBufname, d_SrchFile->Name() )) )                { Msg( "wrong haystack buffer '%s' from LuaCtxt_Edit::from_C_lookup_glock?", gbnm ); return 0; }
      auto numberedMatches(0);
      for( auto iy(0); iy < d_InfLines; ++iy )
         if( d_MatchingLines[iy] )
            ++numberedMatches;
      {
      SprintfBuf LastMetaLine( "%s %d %s", outfile->Name(), numberedMatches, thisMetaLine );
      outfile->InsLine( grepHdrLines, LastMetaLine.k_str() );
      }
      Xbuf xbIns;
      const auto lwidth( uint_log_10( d_InfLines ) );
      for( auto iy(0); iy < d_InfLines; ++iy )
         if( d_MatchingLines[iy] ) {
            char buf[20]; auto pB( buf ); auto cbB( sizeof buf );
            snprintf_full( &pB, &cbB, "%*d  ", lwidth, iy + 1 );
            xb.assign( buf, pB - buf );

            PCChar ptr; size_t chars;
            d_SrchFile->PeekRawLineExists( iy, &ptr, &chars );
            xb.cat( ptr, chars );
            FBOP::InsLineSortedAscending( outfile, &xbIns, grepHdrLines, xb.c_str() );
            }
      outfile->PutFocusOn();
      Msg( "%d lines %s", numberedMatches, "added" );
      return numberedMatches;
      }

   const auto PerfCnt( d_pc.Capture() );

   auto outfile( PseudoBuf( GREP_BUF, true ) );
   outfile->MakeEmpty();
   outfile->SetTabWidthOk( d_SrchFile->TabWidth() ); // inherit tabwidth from searched file

   s_pFbufLog->FmtLastLine( "WriteOutput: thisMetaLine='%s', origSrchfnm='%s' => '%s'", thisMetaLine, origSrchfnm?origSrchfnm:"", outfile->Name() );

   //**************************************************************************
   //
   // data BUFFER SIZE CALC phase
   //
   auto imgBufBytes(0);
   const auto fFirstGen( 0 == d_MetaLineCount );

   auto MetaLinesToCopy(0);
   if( d_MetaLineCount > 0 ) {
      RmvLine( 0 );
      for( auto iy(1); iy < d_MetaLineCount; ++iy ) {
         imgBufBytes += d_SrchFile->LineLength( iy );
         ++MetaLinesToCopy;
         RmvLine( iy );
         }
      s_pFbufLog->FmtLastLine( "d_MetaLineCount=%i,MetaLinesToCopy=%i", d_MetaLineCount, MetaLinesToCopy );
      }

   pathbuf pbuf;
   SprintfBuf Line1( "*GREP* %s", origSrchfnm ? origSrchfnm : d_SrchFile->UserName( BSOB(pbuf) ) );
   const auto Line1Len( Strlen( Line1 ) );
   s_pFbufLog->FmtLastLine( "%s", Line1.k_str() );
   imgBufBytes += Line1Len;

   auto numberedMatches(0);

   for( auto iy(0); iy < d_InfLines; ++iy )
      if( d_MatchingLines[iy] ) {
         imgBufBytes += d_SrchFile->LineLength( iy );
         if( iy >= d_MetaLineCount )
            ++numberedMatches;
         }

   SprintfBuf LastMetaLine( "%s %i %s t=%6.3f", outfile->Name(), numberedMatches, thisMetaLine, PerfCnt );
   auto LastMetaLineLen( Strlen( LastMetaLine ) );
   imgBufBytes += LastMetaLineLen;

   const auto lwidth( fFirstGen ? uint_log_10( d_InfLines ) : 0 );

   if( fFirstGen )
      imgBufBytes += (lwidth+2) * numberedMatches;

   outfile->ImgBufAlloc( imgBufBytes, MetaLinesToCopy + 2 + numberedMatches );

   //**************************************************************************
   //
   // data COPYING phase
   //
   outfile->ImgBufAppendLine( Line1, Line1Len );

   for( auto iy(1); iy < d_MetaLineCount; ++iy ) {
      s_pFbufLog->FmtLastLine( "auxhd=%i", iy );
      outfile->ImgBufAppendLine( d_SrchFile, iy );
      }

   outfile->ImgBufAppendLine( LastMetaLine, LastMetaLineLen );

   for( auto iy(0); iy < d_InfLines; ++iy )
      if( d_MatchingLines[iy] ) {
         FixedCharArray<20> prefix;
         if( fFirstGen ) // this is a 1st-generation search: include line # right justified w/in fixed-width field
            prefix.Sprintf( "%*d  ", lwidth, iy + 1 );

         outfile->ImgBufAppendLine( d_SrchFile, iy, fFirstGen ? prefix.k_str() : nullptr );
         }

   outfile->ClearUndo();
   outfile->UnDirty();
   outfile->PutFocusOn();

   Msg( "%u line%s %s", numberedMatches, Add_s(numberedMatches), d_fFindAllNegate ? "removed" : "matched" );

   return numberedMatches;
   }

//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************


bool ARG::grep() {
   if( !SearchSpecifierOK( this ) )
      return false;

   Xbuf        srchTextBuf( g_SavedSearchString_Buf.c_str() );  // we mangle the search string below so make a copy
         auto  pszSrchStr( srchTextBuf.wbuf() );
   const auto  fNegate( strncmp( pszSrchStr, "!!", 2 ) == 0 );
   if( fNegate ) {
      pszSrchStr += 2;
      }

   // read first line to get header size
   //
   auto    curfile( g_CurFBuf() );
   int     metaLines;
   pathbuf origSrchFnm;
   auto    pSearchedFnm( FBOP::IsGrepBuf( curfile, BSOB(origSrchFnm), &metaLines ) );

   if( pSearchedFnm ) { // Current file is a m2acc result file! (perhaps externally-concocted)
      if( d_fMeta ) { // fMeta says "re-search the _original_ file"!
         curfile = OpenFileNotDir_NoCreate( pSearchedFnm );
         if( !curfile )
            return Msg( "Couldn't open %s", pSearchedFnm );

         d_fMeta = false;
         metaLines    = 0;
         pSearchedFnm = nullptr;
         }
      }

   // create aux header for THIS search
   //
   const auto fUseRegEx( d_cArg > 1 );
   SprintfBuf auxHdrBuf( "%s'%s'%s, by Grep, case:%s"
      , fNegate   ? "*NOT* " : ""
      , srchTextBuf.c_str()
      , fUseRegEx ? " (RE)"  : ""
      , g_fCase   ? "sen"    : "ign"
      );

   // At last, do the searching (the easy part...)
   //
   CGrepper gr( curfile, metaLines, fNegate );

   gr.FindAllMatches( pszSrchStr, fUseRegEx );

   const auto Matches( gr.WriteOutput( auxHdrBuf, pSearchedFnm ) );
   return Matches > 0;
   }


bool ARG::fg() { // fgrep
   PFBUF   srchfile;
   auto    metaLines( 0 ); // params that govern how...
   pathbuf origSrchFnm;    // ...srchfile is processed
   auto    curfile( g_CurFBuf() );

   if( TEXTARG == d_argType ) {
      srchfile = curfile;
      Path::str_t keysFnm( "$FGS" PATH_SEP_STR );  keysFnm += d_textarg.pText;
      curfile = OpenFileNotDir_NoCreate( keysFnm.c_str() );
      if( !curfile )
         return Msg( "Couldn't open '%s' [1]", origSrchFnm );

      if( d_cArg > 1 ) {
         curfile->PutFocusOn();
         return true;
         }
      }
   else {
      if( FBOP::IsGrepBuf( curfile, BSOB(origSrchFnm), &metaLines ) ) {
         srchfile = OpenFileNotDir_NoCreate( origSrchFnm );
         }
      else {
         PCV;
         auto nextview( DLINK_NEXT( pcv, dlinkViewsOfWindow ) );
         if( !nextview || !nextview->FBuf() )
            return Msg( "no next file!" );

         srchfile = nextview->FBuf();
         SafeStrcpy( origSrchFnm, srchfile->Name() );
         }

      if( !srchfile )
         return Msg( "Couldn't open '%s'[2]", origSrchFnm );
      }

   // create aux header for THIS search
   //
   SprintfBuf auxHdrBuf(
        "fgrep keys from %s case:%s"
      , curfile->Name()
      , g_fCase ? "sen" : "ign"
      );

   s_pFbufLog->FmtLastLine( "'%s'", auxHdrBuf.k_str() );

   auto keyLen(2); // for alternation header (2 chars)
   for( auto line(metaLines); line < curfile->LineCount(); ++line ) {
      const auto len( FBOP::LineCols( curfile, line ) );
      if( len )
         keyLen += len + 1;  // + 1 for keySep
      }

   auto pszKey( PChar( alloca( keyLen ) ) );
   auto pB( pszKey );
   *pB++ = '!'; // prepend alternation header (2 chars)
   *pB++ = '\0'; // placeholder for keySep

   Xbuf xb;
   for( auto line(metaLines); line < curfile->LineCount(); ++line ) {
      const auto len( curfile->getLineTabx( &xb, line ) );
      if( 0 == len ) {
         memcpy( pB, xb.c_str(), len+1 );
                 pB  +=         len+1;
         }
      }

   const char keySep (
      ((memchr( pszKey+2, keyLen-2, ',' ) == nullptr) ? ',' :
      ((memchr( pszKey+2, keyLen-2, '|' ) == nullptr) ? '|' :
      ((memchr( pszKey+2, keyLen-2, '.' ) == nullptr) ? '.' : 0)))
      );
   if( !keySep )
      return Msg( "%s: cumulative key contains all possible separators [,|.]", __func__ );

   DBG( "KEY=%s", pszKey );

   auto pastEnd( pszKey+keyLen );
   for( auto pC(pszKey) ; pC < pastEnd ; ++pC )
      if( 0 == *pC )
          *pC = keySep;

   // At last, do the searching (the easy part...)
   //
   CGrepper gr( srchfile, 0, false );

   gr.FindAllMatches( pszKey, false );

   const auto Matches( gr.WriteOutput( auxHdrBuf, nullptr ) );

   Msg( "%u lines matched", Matches );
   return bool(Matches > 0);
   }


PChar FBOP::IsGrepBuf( PCFBUF fb, PChar fnmbuf, const size_t sizeof_fnmbuf, int *pGrepHdrLines ) {
   if( fb->LineCount() == 0 ) {
FAIL: // fnmbuf gets filename of CURRENT buffer!  But generation is 0
      safeStrcpy( fnmbuf, sizeof_fnmbuf, fb->Name() );
      *pGrepHdrLines = 0;
      return nullptr;
      }
   {
   STATIC_CONST char grep_prefix[] = "*GREP* ";
   PCChar ptr; size_t chars;
   if( !fb->PeekRawLineExists( 0, &ptr, &chars ) || !streq_LenOfFirstStr( grep_prefix, KSTRLEN(grep_prefix), ptr, chars ) )
      { goto FAIL; }
   safeStrcpy( fnmbuf, sizeof_fnmbuf, ptr+KSTRLEN(grep_prefix), chars-KSTRLEN(grep_prefix) );
   if( 0 == fnmbuf[0] ) goto FAIL;
   }
   auto iy(1);
   for( ; iy <= fb->LastLine() ; ++iy ) {
      STATIC_CONST char grep_fnm[] = "<grep.";
      PCChar ptr; size_t chars;
      if( !fb->PeekRawLineExists( iy, &ptr, &chars ) || !streq_LenOfFirstStr( grep_fnm, KSTRLEN(grep_fnm), ptr, chars ) )
         { break; }
      0 && DBG("[%d] %s' line=%*s'",iy, fb->Name(), pd2Int(chars), ptr );
      }
   0 && DBG( "%s: %s final=[%d] '%s'", __func__, fb->Name(), iy, fnmbuf );
   *pGrepHdrLines = iy;
   return fnmbuf;
   }

PView FindClosestGrepBufForCurfile( PView pv, PCChar srchFilename ) {
   if( !pv ) pv = g_CurViewHd().First();
   pv = DLINK_NEXT( pv, dlinkViewsOfWindow );
   while( pv ) {
      pathbuf srchFnm; int dummy;
      if(    FBOP::IsGrepBuf( pv->FBuf(), BSOB(srchFnm), &dummy )
          && 0 == strcmp( srchFnm, srchFilename )
        )
         return pv;
      pv = DLINK_NEXT( pv, dlinkViewsOfWindow );
      }
   return nullptr;
   }


bool merge_grep_buf( PFBUF dest, PFBUF src ) {
   pathbuf srcSrchFnm , destSrchFnm  ;
   int     srcHdrLines, destHdrLines ;
   if(    !FBOP::IsGrepBuf( src , BSOB(srcSrchFnm ), & srcHdrLines )
       || !FBOP::IsGrepBuf( dest, BSOB(destSrchFnm), &destHdrLines )
       ||  0 != strcmp( srcSrchFnm, destSrchFnm )
     ) return false;

   0 && DBG( "%s: %s copy [1..%d]", __func__, src->Name(), srcHdrLines );
   Xbuf xbd;
   // insert/copy all src metalines (except 0) to dest
   for( auto iy(1) ; iy < srcHdrLines ; ++iy ) {
      PCChar bos, eos;
      src ->PeekRawLineExists( iy, &bos, &eos );
      dest->InsLine( destHdrLines++, bos, eos, &xbd );
      }
   0 && DBG( "%s: %s merg [%d..%d]", __func__, src->Name(), srcHdrLines, src->LineCount()-1 );
   // merge (copy while sorting) all match lines
   for( auto iy(srcHdrLines) ; iy < src->LineCount() ; ++iy ) {
      PCChar bos, eos;
      src ->PeekRawLineExists( iy, &bos, &eos );
      FBOP::InsLineSortedAscending( dest, &xbd, destHdrLines, bos, eos );
      }
   return true;
   }


bool ARG::gmg() { // arg "gmg" edhelp  # for docmentation
   const auto fDestroySrcsBufs( 0 == d_cArg );
   PFBUF dest(nullptr);
   pathbuf srchFilename;
   int     dummy;
   if( FBOP::IsGrepBuf( g_CurFBuf(), BSOB(srchFilename), &dummy ) ) {
      dest = g_CurFBuf();
      0 && DBG( "%s: dest=cur (%s)", __func__, dest->Name() );
      }
   else {
      safeStrcpy( BSOB(srchFilename), g_CurFBuf()->Name() );
      }
   0 && DBG( "%s: will look for all greps of (%s)", __func__, srchFilename );

   auto merges(0);
   PView pv(nullptr);
   while( (pv=FindClosestGrepBufForCurfile( pv, srchFilename )) ) {
      const auto src( pv->FBuf() );
      if( !dest )
         dest = src;
      else {
         if( src != dest ) { // can only happen if we restarted scan from head of view list
            merge_grep_buf( dest, src ); ++merges;
            if( fDestroySrcsBufs ) {
               const auto fDidRmv( DeleteAllViewsOntoFbuf( src ) );
               if( fDidRmv ) pv = nullptr; // restart scan from head of view list
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

//
// Copyright 1991 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "fname_gen.h"

/*
   chops string cur in place (WRITES TO cur!!!)

   cur points to an ascizz string (an ASCII string with TWO (2) trailing NUL chars).

   on return, each sequence of chars in src matching any char in delim will have
   been replaced by one (1) NUL.  The final/second NUL at end of src remains.

   This avoids the need to maintain any external 'fields' data structure (array of
   pointers) (which has to be separate malloc'd) which the other version of this function
   requires.

   In the event multiple adjacent delims are present in the source string, the tail of the
   src string is "moved left" to ensure only one (1) NUL exists prior to the next
   substring (or terminating second sequential NUL).  In extreme cases this can lead to
   pathological O(n^2) activity, but it is hoped that this fxn will not be used on long
   strings.

   EX: in:  ";-;abc-;def;;-ghi;;\0\0"  (pszDelim=";-")
       out: "abc\0def\0ghi\0\0"        (\0=NUL)
        0         1         2         3
        0123456789012345678901234567890123456789
   arg ";;;;;;;;;*.h;;;;;;;;;*.cpp;;;;;;;;;" setfile
 */
void ChopAscizzOnDelim( PChar cur, const PCChar pszDelim ) {
   const auto p0( cur );
   auto eos( Eos( cur ) + 2 ); // +2 includes BOTH NULs
   while(1) {
      const auto c0( cur );
      cur = StrPastAny( c0, pszDelim );     // move cur past any extra delims
      if( c0 != cur ) { // moved past extra delims? collapse trailing non-delim content (starting @cur) to c0
         const auto shortenBy ( cur - c0  );
         const auto chars2move( eos - cur );  0 && DBG( "moving %" PR_PTRDIFFT "u <- %" PR_PTRDIFFT "u L %" PR_PTRDIFFT "u (eos=%" PR_PTRDIFFT "u, -%" PR_PTRDIFFT "u)", c0-p0, cur-p0, chars2move, eos-p0, shortenBy );
         memmove( c0, cur, chars2move );
         eos -= shortenBy;
         }
      if( !*c0  ) { break; }                // no trailing non-delim content?
      cur = StrToNextOrEos( c0, pszDelim );
      if( !*cur ) { break; }                // no delim?
      *cur++ = '\0';                        // NUL-out delim, examine content following it (in next iter)
      }
   }

//------------------------------------------------------------------------------

void DiceableString::DBG() const {
   auto ix( 0u );
   PCChar cur( nullptr );
   while( cur=GetNext( cur ) ) {
      ::DBG( "[%u]=%s|", ix++, cur );
      }
   }

//------------------------------------------------------------------------------
//
// UserNameDelimChars are characters which the user may use to surround
// (delimit) filenames; they are typicaly only used when a filename contains
// spaces.  Multiple UserNameDelimChars are offered in case a filename contains
// a certain UserNameDelimChar (an unusual case indeed).
//
STATIC_FXN char IsUserFnmDelimChar( char ch ) {
   switch( ch ) {
      default:   return 0;
      case '"' :
      case '\'':
      case '|' : return ch;
      }
   }

//------------------------------------------------------------------------------

STATIC_FXN bool IsolateFilename( sridx *pMin, sridx *pMax, stref rl ) {
   // could add: lots and lots o special per-FileType code to extract
   //    filenames from various language-specific include statements
   sridx ixStart(0);
   sridx ixEnd;
   if(   IsUserFnmDelimChar( rl[ixStart] )
         // Next match of rl[0] is assumed to be closing one; since WE
         // (in <files>, etc.) _generate_ the delim, and in so doing
         // choose a delim which isn't found in the filename itself,
         // it's a reasonable assumption to make.
         //
      && ((ixEnd=ToNextOrEnd( rl[0], rl, 1 )), atEnd( rl, ixEnd ))
     ) {
      ++ixStart; // ixEnd is set correctly
      }
   else {
      ixStart = FirstNonBlankOrEnd( rl, ixStart   );
      ixEnd   = FirstBlankOrEnd   ( rl, ixStart+1 );
      }
   if( ixStart >= ixEnd ) return false;

   *pMin = ixStart;
   *pMax = ixEnd  ;

   return true;
   }

int FBUF::GetLineIsolateFilename( Path::str_t &st, LINE yLine, COL xCol ) const {
   if( yLine > LastLine() )
      return -1;

   auto rl( PeekRawLine( yLine ) ); PCChar bos( rl.data() ); PCChar eos( rl.data()+rl.length() );
   const auto ixCol( CaptiveIdxOfCol( g_CurFBuf()->TabWidth(), rl, xCol ) );
   rl.remove_prefix( ixCol );
   sridx oMin, oMax;
   if( !IsolateFilename( &oMin, &oMax, rl ) ) { return 0; }
   st.assign( rl.data()+oMin, oMax-oMin );
   return 1;
   }

//------------------------------------------------------------------------------

//
// multi-file grep/replace file selection:
//
// There are two forms for specifying files: a macro ("mfspec_", "mfspec"), or
// the contents of a buffer ("<mfx>" or *mfptr)
//
// The macro forms are a string value containing an array of ';' separated
// entries, each of which is a CFX ...  arg "CFX" help
//
// The buffer forms interpret each line as a macro-form string.
//
// Clearly this offers a surprising amount of semantic content in a
// "syntactically minimalist" package, so I think it's useful to preserve this
// syntatic option, while adding other filename source types.
//
// How?  By defining variants of the interface class StringGenerator:
//
//  * WildcardFilenameGenerator  (takes simple wildcard specifier)
//  * CfxFilenameGenerator (takes string having CFX semantics)
//  * FilelistCfxFilenameGenerator  (takes FBUF containing 1 CFX per line)
//
// Instances of any of these generators could be passed to (or even _created_ by
// variants of) objects which process multiple files.
//
// The StringGenerator type of class/object is basically the "turning on its
// head" of the current procedural code: automatic variables that now live
// safely on the stack in the procedureal model will now have to live inside the
// ...Generator object
//

bool WildcardFilenameGenerator::VGetNextName( Path::str_t &dest ) {
   RTN_false_ON_BRK;
   dest = d_dm.GetNext();
   return !dest.empty();
   }

//-----------------------------------

bool FilelistCfxFilenameGenerator::VGetNextName( Path::str_t &dest ) {
   dest.clear();
   while( true ) {
      if( d_pCfxGen ) {
         if( d_pCfxGen->VGetNextName( dest ) )
            return true;

         Delete0( d_pCfxGen );
         }
      RTN_false_ON_BRK;

      const auto glif_rv( d_pFBuf->GetLineIsolateFilename( d_sbuf, d_curLine++, 0 ) );
      if( glif_rv < 0 ) return false;  // no more lines
      if( glif_rv > 0 ) d_pCfxGen = new CfxFilenameGenerator( d_sbuf, ONLY_FILES );
      }
   }

//------------------------------------------------------------------------------

enum { ENV_REFS_NONE, ENV_REFS_PARSE_ERR, ENV_REFS_UNAMBIG, ENV_REFS_MID_LIST, ENV_REFS_LEADING_LIST };

#include "dlink.h"

class StrSubstituterGenerator {
   NO_COPYCTOR(StrSubstituterGenerator);
   NO_ASGN_OPR(StrSubstituterGenerator);

   struct SubStrSubstituter {
      NO_COPYCTOR(SubStrSubstituter);
      NO_ASGN_OPR(SubStrSubstituter);

      DLinkEntry<SubStrSubstituter> dlink;

      const int  d_xReplStart;
      const int  d_replLen;
      int        d_valueIdx;
      int        d_nValues;
      PPCChar    d_values;

      SubStrSubstituter( PCChar pValue, int chValDelim, const int xReplStart_, const int replLen_ );
      ~SubStrSubstituter() {
         if( d_values ) {
            Free0( d_values[0] ); // d_values[1..n] point within same alloc as d_values[0]
            Free0( d_values    );
            }
         }
      };

   bool                         d_fCombinationsExhaused;
   Path::str_t                  d_pBaseString;
   DLinkHead<SubStrSubstituter> d_SubStrSubstituter;

   public:

   StrSubstituterGenerator() : d_fCombinationsExhaused(false) {}
   ~StrSubstituterGenerator();

   void SetBaseStr( PCChar pBase ) { d_pBaseString = pBase; }
   bool GetNextString( std::string &st );

   void AddMultiExpandableSeg( PCChar pValue, int chValDelim, int xReplStart, int replLen );

   private:

   void NextCombination();
   };


StrSubstituterGenerator::SubStrSubstituter::SubStrSubstituter( PCChar pValue, int chValDelim, const int xReplStart_, const int replLen_ )
   : d_xReplStart(xReplStart_)
   , d_replLen(replLen_)
   , d_valueIdx(0)
   , d_nValues(1)
   , d_values(nullptr)
   {
   auto pVal( Strdup( pValue ) );
   for( auto pc( pValue ) ; *pc ; ++pc ) {
      if( *pc == chValDelim ) ++d_nValues;
      }
   AllocArrayNZ( d_values, d_nValues );
   for( auto iy(0) ; ; ++iy ) {
      d_values[ iy ] = pVal;  // [0] is the Strdup()'d string itself, which must be freed later!
      const auto pSep( strchr( pVal, chValDelim ) );
      if( !pSep )
         break;
      *pSep = '\0';
      pVal = pSep+1; // [1..n] are pointers within [0], the Strdup()'d string
      }
   }

void StrSubstituterGenerator::AddMultiExpandableSeg( PCChar pValue, int chValDelim, int xReplStart, int replLen ) {
   SubStrSubstituter *pNew( new SubStrSubstituter( pValue, chValDelim, xReplStart, replLen ) );
   DLINK_INSERT_LAST(d_SubStrSubstituter, pNew, dlink);
   }

StrSubstituterGenerator::~StrSubstituterGenerator() {
   while( auto pEl=d_SubStrSubstituter.front() ) { // zap list
      DLINK_REMOVE_FIRST( d_SubStrSubstituter, pEl, dlink );
      delete pEl;
      }
   }

void StrSubstituterGenerator::NextCombination() {
   DLINKC_LAST_TO_FIRST(d_SubStrSubstituter,dlink,pSSS) {
      if( ++pSSS->d_valueIdx < pSSS->d_nValues )
         return;

      pSSS->d_valueIdx = 0;
      }

   d_fCombinationsExhaused = true;
   }

bool StrSubstituterGenerator::GetNextString( std::string &st ) {
   if( d_fCombinationsExhaused ) {
      st.clear();
      return false;
      }

   PCChar pLit0( d_pBaseString.c_str() );
   PCChar pLit( pLit0 );

   DLINKC_FIRST_TO_LASTA(d_SubStrSubstituter,dlink,pSSS) {
      const auto prevLitLen( pSSS->d_xReplStart - (pLit - pLit0) );
      st.append( pLit, prevLitLen );
      st.append( pSSS->d_values[ pSSS->d_valueIdx ] );
      pLit += prevLitLen + pSSS->d_replLen;
      }
   st.append( pLit );

   NextCombination();
   return true;
   }

//------------------------------------------------------------------------------

//
// Parses a string complying with CFX grammar and (if pSSG is NZ) prepares it
// for a StrSubstituterGenerator
//
// You can run this with a NULL pSSG in order to pre-check (examine rv)
//
STATIC_FXN int CFX_to_SSG( const PCChar inbuf, StrSubstituterGenerator *pSSG ) {
   if( pSSG )  pSSG->SetBaseStr( inbuf );

   auto pC( inbuf );
   decltype(pC) pRefStart;

   auto rv( ENV_REFS_NONE );
   while( (pRefStart=StrToNextOrEos( pC, "$%" )) && *pRefStart ) {
      auto pName( pRefStart + 1 );
      char term;
      switch( *pRefStart ) {
         case '$': if( '(' == pRefStart[1] ) {
                      ++pName;
                      term = ')';
                      }
                   else if( '{' == pRefStart[1] ) {
                      ++pName;
                      term = '}';
                      }
                   else
                      term = ':';
                   break;

         case '%': term = '%';
                   break;

         default:  /*suppress warning*/term = 0; Assert( 0 );
         }
      const decltype(pName) pTerm( strchr( pName, term ) );
      if( !pTerm )
         return ENV_REFS_PARSE_ERR;

      pC = pTerm + 1;
      if( pTerm == pName ) // empty reference?
         continue;         // leave as literal

      const auto fLeadingEnvRef( pRefStart == inbuf );

      pathbuf pbuf;
      safeStrcpy( BSOB(pbuf), pName, pTerm );
      PCChar pValue( getenv( pbuf ) );
      0 && DBG( "getenv( %s ) = %p (%s)", pbuf, pValue, pValue ? pValue : "" );
      if( !pValue && strchr( pbuf, '|' ) ) { // undefined and "name" contains '|'?
         0 && DBG( "INLINE '%s'", pbuf );
         pValue = xlatCh( pbuf, '|', Path::chEnvSep ); // use ostensible envvar name as literal envvar value
         }
      else if( (!pValue || !*pValue) && fLeadingEnvRef ) { // undefined or has no value?
         pValue = ".";  // leading value == cwd
         }

      if( pValue && *pValue ) { // defined and has value?
         if( pSSG )  pSSG->AddMultiExpandableSeg( pValue, Path::chEnvSep, pRefStart - inbuf, pTerm - pRefStart + 1 );

         // const auto fHasWC( StrToNextOrEos( pValue, "*?" )[0] != 0 );
         const auto fIsList( ToBOOL( strchr( pValue, Path::chEnvSep ) ) );

         if(      fLeadingEnvRef      )  { rv = fIsList ? ENV_REFS_LEADING_LIST : ENV_REFS_UNAMBIG; }
         else if( fIsList             )  { rv = ENV_REFS_MID_LIST; }
         else if( ENV_REFS_NONE == rv )  { rv = ENV_REFS_UNAMBIG; }
         }
      }
   return rv;
   }


#ifdef fn_cfx

// $PATH:\%USERNAME%\*.exe  $PATH:${}$(PATH)*.exe  %USERPROFILE%\\\* $PATH:*.exe
// %TAGZ%\*.h;%TAGZ%*.hpp;$(TAGZ)*.hxx;$TAGZ:*.kh;$(TAGZ)*.c;$TAGZ:*.cpp;$TAGZ:*.cc;$TAGZ:*.cxx;$TAGZ:*.kc
// mfspec:=".\*.h;.\*.hpp;$(TAGZ)*.hxx;$TAGZ:*.kh;$(TAGZ)*.c;$TAGZ:*.cpp;$TAGZ:*.cc;$TAGZ:*.cxx;$TAGZ:*.kc"
// %PATH%\*.bat;${USERPROFILE}\Application Data\Kevins Editor\*.$(lua|tmp)

bool ARG::cfx() {
   switch( d_argType ) {
      default:       return BadArg();
      case TEXTARG:  break;
      }

   DBG( "[-] %s", d_textarg.pText );

   StrSubstituterGenerator ssg;
   CFX_to_SSG( d_textarg.pText, &ssg );
   pathbuf pbuf;
   auto ix(0);
   while( ssg.GetNextString( BSOB(pbuf) ) ) {
      DBG( "[%d] %s", ix++, pbuf );
      }

   return true;
   }

#endif

//------------------------------------------------------------------------------

bool DirListGenerator::VGetNextName( Path::str_t &dest ) {
   if( d_output.empty() )
      return false;

   auto pEl( d_output.front() );
   DLINK_REMOVE_FIRST( d_output, pEl, dlink );
   dest = pEl->string;
   FreeStringListEl( pEl );
   return true;
   }

void DirListGenerator::AddName( stref name ) {
   InsStringListEl( d_input , name );
   InsStringListEl( d_output, name );
   }

DirListGenerator::DirListGenerator( PCChar dirName ) {
   NewScope {
      Path::str_t pStartDir( dirName ? dirName : Path::GetCwd() );
      AddName( pStartDir );
      }

   Path::str_t pbuf;
   while( auto pNxt=d_input.front() ) {
      DLINK_REMOVE_FIRST( d_input, pNxt, dlink );
      pbuf = Path::str_t( pNxt->string ) + (PATH_SEP_STR "*");
      FreeStringListEl( pNxt );

      0 && DBG( "Looking in ='%s'", pbuf.c_str() );

      WildcardFilenameGenerator wcg( pbuf.c_str(), ONLY_DIRS );

      while( wcg.VGetNextName( pbuf ) ) {
         if( !Path::IsDotOrDotDot( pbuf ) )
            AddName( pbuf );

         0 && DBG( "   PBUF='%s' DBUF='%" PR_BSR "'", pbuf.c_str(), BSR(Path::RefFnm( pbuf )) );
         }
      }
   }

DirListGenerator::~DirListGenerator() {
   DeleteStringList( d_input  );
   DeleteStringList( d_output );
   }

//------------------------------------------------------------------------------

#ifdef fn_wct  // testbed for code used in setfile

bool ARG::wct() {
   pathbuf searchSpec;

   switch( d_argType ) {
      default:       return BadArg();
      case NOARG:    SafeStrcpy( searchSpec, "*" )  ;            break;
      case TEXTARG:  SafeStrcpy( searchSpec, d_textarg.pText );  break;
      }

   DirListGenerator dirs;

   pathbuf pbuf;
   const auto pF( g_CurFBuf() );
   while( dirs.VGetNextName( BSOB(pbuf) ) ) {
      // if( !Path::IsDotOrDotDot( pbuf.c_str() ) )
      //    pF->FmtLastLine( pbuf );

      WildcardFilenameGenerator wcg( FmtStr<MAX_PATH>( "%s" PATH_SEP_STR "%s", pbuf, searchSpec ), ONLY_FILES );
      pathbuf fbuf;
      while( wcg.VGetNextName( BSOB(fbuf) ) ) {
         pF->FmtLastLine( fbuf );
         }
      }

   return true;
   }

#endif

//------------------------------------------------------------------------------

enum { MFSPEC_D=0 };

CfxFilenameGenerator::CfxFilenameGenerator( stref macroText, WildCardMatchMode matchMode )
   : d_splitLine( macroText, Path::EnvSepStr() )
   , d_matchMode( matchMode )
   {
   // d_splitLine.DBG();
   MFSPEC_D && DBG( "%s::'%" PR_BSR "' (INCOMPLETE)", __func__, BSR(macroText) );
   // d_splitLine.DBG();
   }

CfxFilenameGenerator::~CfxFilenameGenerator() {
   Delete0( d_pWcGen );
   Delete0( d_pSSG );
   }

bool CfxFilenameGenerator::VGetNextName( Path::str_t &dest ) {
   RTN_false_ON_BRK;
   dest.clear();

   if( d_pWcGen ) {
NEXT_WILDCARD_MATCH:
      if( d_pWcGen->VGetNextName( dest ) ) {
         MFSPEC_D && DBG( "%s+ '%s'", __func__, dest.c_str() );
         return true;
         }
      Delete0( d_pWcGen );
      }

   if( d_pSSG ) {
NEXT_SSG_COMBINATION:
      if( d_pSSG->GetNextString( dest ) ) {
         MFSPEC_D && DBG( "%s+ WcGen <= '%s'", __func__, dest.c_str() );
         d_pWcGen = new WildcardFilenameGenerator( dest.c_str(), d_matchMode );
         goto NEXT_WILDCARD_MATCH;
         }

      Delete0( d_pSSG );
      }
   d_pszEntrySuffix = d_splitLine.GetNext( d_pszEntrySuffix );
   if( d_pszEntrySuffix ) {
      MFSPEC_D && DBG( "%s+ ENVMAP <= '%s'", __func__, d_pszEntrySuffix );
      d_pSSG = new StrSubstituterGenerator;
      CFX_to_SSG( d_pszEntrySuffix, d_pSSG );
      goto NEXT_SSG_COMBINATION;
      }

   MFSPEC_D && DBG( "%s- Exhausted", __func__ );
   return false;
   }

//-------------------------------------------------------------------------------------------------
//
// the current problem:
//
// We would LIKE to be able to use SearchEnvDirListForFile to open files,
// HOWEVER:
//
// setfile takes some special parameter values
//  - pseudofile, e.g. "<ascii>"
//  - wildcard  , e.g. "*.*"
//
// PSEUDOFILE:
//    If a pseudofile is passed to CfxFilenameGenerator, it will return no
//    filenames (since pseudofile names are invalid OS filenames).  So this is
//    not a problem.
//
// WILDCARD:
//    This is a problem, since wildcards are quite valid to be passed to
//    CfxFilenameGenerator.  In this case, CfxFilenameGenerator will
//    return filenames that are a series of matches of the wildcard.  So we call
//    mfg.VGetNextName() ONCE and use the result (or lack thereof) directly.
//
//    The problem is that in cases (directly analogous to searching PATH
//    for a executable file), we want this behavior: for example
//    "$GUIDIFF:;$PATH:kdiff3.exe".  The first match is used.
//
//    BUT in other cases, we want to use CfxFilenameGenerator only to resolve
//    any embedded envvarspecs as a prerequisite to absolutizing the path part
//    of the argument, but not to expand any wildcards present in the NAME part
//    of the argument string (in this case, we'll subsequently be opening a new
//    WC-pseudofile which contains all WC matches of the argument string using
//    Fileread_Wildcard).  If we allow CfxFilenameGenerator to expand all WCs
//    in its argument, no WC chars will be present in any of
//    CfxFilenameGenerator's output (which prevents later WC expansion).
//
//    Further: in two cases, CfxFilenameGenerator will not be able to
//    provide a single path-expansion of its argument:
//
//    1) single-string argument containing WC chars with ENVVAR-prefix where ENVVAR expands to a LIST.
//    2) multi-string argument containing WC chars.
//
//    In either of these cases, if we're setfile'ing on the argument, the only
//    reason we're calling this function is to obtain as unambiguous a NAME for
//    the to-be-created WC-pseudofile which is going to be filled by
//    Fileread_Wildcard.  So maybe the best thing to do is return the argument
//    unaltered, and let Fileread_Wildcard do its thing.
//
//    So how do we tell the difference between the two cases above vs.  cases
//    where the path resolution is unambiguous?
//
//
// We DO want ENVIRONMENT VARIABLE expansion
//
// Current matrix
// --------------
//
// path        name        action               OK?
// ----        ----        ------               ---
// constant    constant    constant              Y      %PATH%\ls.*
// constant    wildcard    first WC match        N      $PATH:\ls.*
// envvar      constant    constant              Y      $(PATH)\ls.*
// envvar      wildcard    first WC match        N
// envlist     constant
// envlist     wildcard
//
// We need a "keep name-wildcard" option, and then need to SPLIT the pszSrc
// parameter into
//

STATIC_FXN void SearchEnvDirListForFile( Path::str_t &dest, const PCChar pszSrc, bool fKeepNameWildcard ) { enum { VERBOSE=1 };
   if( fKeepNameWildcard ) {
      if(  ToBOOL( strchr( pszSrc, Path::chEnvSep ) )  // presence of Path::chEnvSep overrides fKeepNameWildcard (since what
        || FBUF::FnmIsPseudo( pszSrc )                 // follows only makes sense if pszSrc is a single filename)
        ) {                                                                           VERBOSE && DBG( "%s *** '%s'", __func__, pszSrc );
         goto OUTPUT_EQ_INPUT;
         }

      const auto fname( Path::CpyFnameExt( pszSrc ) );                                VERBOSE && DBG( "%s *** '%s' name='%s'", __func__, pszSrc, fname.c_str() );
      if( HasWildcard( fname ) ) {
         goto OUTPUT_EQ_INPUT;
         }

      auto path( Path::CpyDirnm( pszSrc ) );                                          VERBOSE && DBG( "%s *** '%s' path='%s'", __func__, pszSrc, path.c_str() );
      if( path.empty() ) {
#if defined(_WIN32)
         path = ".";   // supply "." so CfxFilenameGenerator works
#endif
         }
      else {
         if( path.length() > 3 && Path::IsPathSepCh( path.back() ) )
            path.pop_back();  // remove the trailing PathSepCh so CfxFilenameGenerator works
         }
      CfxFilenameGenerator mfg( path.c_str(), ONLY_DIRS );
      if( mfg.VGetNextName( dest ) ) { // only care about FIRST match
         dest += fname;                                                               VERBOSE && DBG( "%s '%s' =.> '%s'"     , __func__, pszSrc, dest.c_str() );
         return;
         }
      else {
         const auto abs_pb( Path::Absolutize( pszSrc ) );
         if( abs_pb.empty() ) {                                                       VERBOSE && DBG( "%s '%s' =;> '%s'"     , __func__, pszSrc, abs_pb.c_str() );
            return;
            }
         dest = abs_pb;
         }
      }
   else { // normal expansion
#if defined(_WIN32)
      CfxFilenameGenerator mfg( pszSrc, ONLY_FILES );
      if( mfg.VGetNextName( dest ) ) {  /* only care about FIRST match */             VERBOSE && DBG( "%s '%s' =:> '%s'"     , __func__, pszSrc, dest.c_str() );
         return;
         }
#else
      dest = pszSrc;  /* FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME */     VERBOSE && DBG( "%s '%s' =:> '%s'"     , __func__, pszSrc, dest.c_str() );
      return;
#endif
      }

OUTPUT_EQ_INPUT:
   dest = pszSrc;                                                                     VERBOSE && DBG( "%s '%s' => NO MATCH!", __func__, pszSrc );
   }

void SearchEnvDirListForFile( Path::str_t &st, bool fKeepNameWildcard ) {
   ALLOCA_STRDUP( tmp, srcLen, st.c_str(), st.length() );
   SearchEnvDirListForFile( st, tmp, fKeepNameWildcard );
   }

Path::str_t CompletelyExpandFName_wEnvVars( PCChar pszSrc ) { enum { DB=0 };
   if( FBUF::FnmIsPseudo( pszSrc ) ) {                               DB && DBG( "%s- (FnmIsPseudo) '%s'", __func__, pszSrc );
      return Path::str_t( pszSrc );
      }

   Path::str_t st( pszSrc );
   if( LuaCtxt_Edit::ExpandEnvVarsOk( st ) ) {
      DB && DBG( "%s post-Lua expansion='%s'->'%s'", __func__, pszSrc, st.c_str() );
      }
   else {
      if( '$' == pszSrc[0] )
         SearchEnvDirListForFile( st );
      else
         st = pszSrc;
      }
   DB && DBG( "%s post-expansion='%s'", __func__, st.c_str() );

   st = Path::Absolutize( st.c_str() );
   DB && DBG( "%s- '%s'", __func__, st.c_str() );
   return st;
   }

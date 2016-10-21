//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
#include "my_fio.h"

//*******************************  BEGIN TABS  *******************************
//*******************************  BEGIN TABS  *******************************
//*******************************  BEGIN TABS  *******************************

GLOBAL_VAR bool g_fRealtabs;
GLOBAL_VAR bool g_fM4backtickquote;

class Tabber {
   const int d_tabWidth;
   int  FillCountToNextTabStop ( int col ) const { return d_tabWidth - (col % d_tabWidth) ; }
public:
   Tabber( int tabWidth ) : d_tabWidth(tabWidth) {}
   int  ColOfNextTabStop       ( int col ) const { return col + FillCountToNextTabStop( col ); }
   int  ColOfPrevTabStop       ( int col ) const { return col - (1 + ((col - 1) % d_tabWidth)); }
   bool ColAtTabStop           ( int col ) const { return (col % d_tabWidth) == 0; }
   };

void swidTabwidth( PChar dest, size_t sizeofDest, void *src ) {
   scpy( dest, sizeofDest, "status ln's \"e?\"" );
   }

PCChar swixTabwidth( stref param ) { enum { DB=0 }; DB && DBG( "%s+ %" PR_BSR "'", __func__, BSR(param) );
   const auto val( StrToInt_variable_base( param, 10 ) );
   const auto inRange( val >= 1 && val <= MAX_TAB_WIDTH );
   if( inRange ) {
      g_iTabWidth = val;
      DispNeedsRedrawAllLinesAllWindows();
      }
   return inRange ? nullptr : "tabwidth: Value must be between 1 and 8";
   }

void FBUF::SetTabWidth_( COL newTabWidth, PCChar funcnm_ ) { enum { DB=0 }; DB && DBG( "%s:%s %d <- %s", __func__, Name(), newTabWidth, funcnm_ );
   const auto inRange( newTabWidth >= MIN_TAB_WIDTH && newTabWidth <= MAX_TAB_WIDTH );
   if( inRange ) {
      d_TabWidth = newTabWidth;
      }
   }

STATIC_CONST PCChar s_entabNames[] = {
   [ENTAB_0_NO_CONV]                   = "none"     ,
   [ENTAB_1_LEADING_SPCS_TO_TABS]      = "leading"  ,
   [ENTAB_2_SPCS_NOTIN_QUOTES_TO_TABS] = "exoquote" ,
   [ENTAB_3_ALL_SPC_TO_TABS]           = "all"      ,
   };
CompileTimeAssert( MAX_ENTAB_INVALID == ELEMENTS(s_entabNames) );  // static_assert does not work here

bool FBUF::SetEntabOk( int newEntab ) {
   const auto inRange( newEntab >= ENTAB_0_NO_CONV && newEntab < MAX_ENTAB_INVALID );
   if( inRange ) {
      d_Entab = eEntabModes(newEntab);
      Msg( "%s entab set to %s (%d)", Name(), s_entabNames[ d_Entab ], d_Entab );
      }
   return inRange;
   }

void swidEntab( PChar dest, size_t sizeofDest, void *src ) {
   safeSprintf( dest, sizeofDest, "%s (%d)", s_entabNames[ g_CurFBuf()->Entab() ], g_CurFBuf()->Entab() );
   }

PCChar swixEntab( stref param ) { enum { DB=1 }; DB && DBG( "%s+ %" PR_BSR "'", __func__, BSR(param) );
   COL  newval;
   for( newval=0 ; newval < ELEMENTS(s_entabNames) ; ++newval ) {
      if( 0==cmpi( param, s_entabNames[newval] ) ) {
         break;
         }
      }                                                            DB && DBG( "%s: %d", __func__, newval );
   if( !(newval < ELEMENTS(s_entabNames)) ) { newval = StrToInt_variable_base( param, 10 ); }
                                                                   DB && DBG( "%s: %d for %s", __func__, newval, g_CurFBuf()->Name() );
   const auto setOk( g_CurFBuf()->SetEntabOk( newval ) );
   return setOk ? nullptr : SwiErrBuf.Sprintf( "entab value '%" PR_BSR "' must be one of 'none', 'leading', 'exoquote' or 'all'", BSR(param) );
   }

STATIC_FXN bool spacesonly( stref::const_iterator ptr, stref::const_iterator eos ) {
   return std::all_of( ptr, eos, []( char ch ){ return ch == ' '; } );
   }

template <typename T>
void PrettifyWriter
   ( std::string &dest, T dit, const COL dofs
   , const size_t maxCharsToWrite
   , const stref src, const COL src_xMin
   , const COL tabWidth, const char chTabExpand, const char chTrailSpcs
   ) { // proper tabx requires walking src from its beginning, even though we aren't necessarily _copying_ from the beginning.
   auto sit( src.cbegin() );
   COL xCol( 0 ); COL dix( 0 );
   const auto wr_char = [&]( char ch ) { if( xCol++ >= src_xMin ) { *dit++ = ch; ++dix; } };
   const Tabber tabr( tabWidth );
   // certain chTabExpand values have magical side-effects:
   STATIC_CONST char bsbullet[] = { BIG_BULLET, SMALL_BULLET, '\0' };
   const stref srTabExpand( // srTabExpand (instead of chTabExpand) could be passed in, but mind the chTabExpand == '\0' case below!
      chTabExpand == BIG_BULLET ? stref( bsbullet   ) :
      chTabExpand == '^'        ? stref( "^`"       ) :
      chTabExpand == '-'        ? stref( "-->"      ) :
      chTabExpand == '<'        ? stref( "<->"      ) :
                                  stref( &chTabExpand, sizeof(chTabExpand) )
      );
   const auto chLast( srTabExpand[ srTabExpand.length() > 2 ? 2 : 0 ] );
   const auto chFill( srTabExpand[ srTabExpand.length() > 1 ? 1 : 0 ] );
   while( sit != src.cend() && dix < maxCharsToWrite ) {
      const auto ch( *sit++ );
      if( ch != HTAB || chTabExpand == '\0' ) {
         wr_char( ch );
         }
      else { // expand an HTAB-spring
         const auto tgt( tabr.ColOfNextTabStop( xCol ) );
         wr_char( (xCol == tgt-1) ? chLast : srTabExpand[0] );
         while( xCol < tgt && dix < maxCharsToWrite ) {
            wr_char( (xCol == tgt-1) ? chLast : chFill );
            }
         }
      }
   if( chTrailSpcs && (sit == src.cend() || spacesonly( sit, src.cend() )) ) { // _trailing_ spaces on the source side
      stref destseg( dest.data() + dofs, dix ); // what we wrote above
      const auto ix_last_non_white( destseg.find_last_not_of( SPCTAB ) );
      if( ix_last_non_white != dix-1 ) { // any trailing blanks at all?
         // ix_last_non_white==eosr means ALL are blanks
         const auto rlen( ix_last_non_white==eosr ? dix : dix-1 - ix_last_non_white );
         for( auto iy( dofs + (dix-rlen) ); iy < dofs + dix ; ++iy ) {
            if( dest[iy] == ' ' ) { // don't touch tab-replacement done above
                dest[iy] = chTrailSpcs;
                }
            }
         }
      }
   }

void PrettifyMemcpy
   ( std::string &dest, COL xLeft
   , size_t maxCharsToWrite
   , stref src, COL src_xMin
   , COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   PrettifyWriter< decltype( begin(dest) ) >       ( dest , begin(dest) + xLeft,  xLeft, maxCharsToWrite, src, src_xMin, tabWidth, chTabExpand, chTrailSpcs );
   }

STATIC_FXN void PrettifyInsert
   ( std::string &dest
   , size_t maxCharsToWrite
   , stref src, COL src_xMin
   , COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   PrettifyWriter< decltype(back_inserter(dest)) > ( dest, back_inserter(dest) ,      0, maxCharsToWrite, src, src_xMin, tabWidth, chTabExpand, chTrailSpcs );
   }

void FormatExpandedSeg // more efficient version: recycles (but clear()s) dest, should hit the heap less frequently
   ( std::string &dest, size_t maxCharsToWrite
   , stref src, COL src_xMin, COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   dest.clear();
   PrettifyInsert( dest, maxCharsToWrite, src, src_xMin, tabWidth, chTabExpand, chTrailSpcs );
   }

std::string FormatExpandedSeg // less efficient version: uses virgin dest each call, thus hits the heap each time
   ( size_t maxCharsToWrite
   , stref src, COL src_xMin, COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   std::string dest;
   PrettifyInsert( dest, maxCharsToWrite, src, src_xMin, tabWidth, chTabExpand, chTrailSpcs );
   return dest;
   }

COL ColPrevTabstop( COL tabWidth, COL xCol ) { return Tabber( tabWidth ).ColOfPrevTabStop( xCol ); }
COL ColNextTabstop( COL tabWidth, COL xCol ) { return Tabber( tabWidth ).ColOfNextTabStop( xCol ); }

bool FBOP::IsLineBlank( PCFBUF fb, LINE yLine ) {
   return IsStringBlank( fb->PeekRawLine( yLine ) );
   }

bool FBOP::IsBlank( PCFBUF fb ) {
   for( auto iy( 0 ); iy < fb->LineCount() ; ++iy ) {
      if( !FBOP::IsLineBlank( fb, iy ) ) {
         return false;
         }
      }
   return true;
   }

//      const Tabber &TabberParam;
typedef const Tabber  TabberParam;  // 3 calls using this type take less code (-512 byte GCC incr)

STATIC_FXN void spcs2tabs_outside_quotes( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto quoteCh( '\0' );
   auto destCol( 0 );
   auto fNxtChEscaped( false );
   auto fInQuotedRgn(  false );
   auto sit( src.cbegin() );
   while( sit != src.cend() ) {
      if( !fInQuotedRgn ) {
         if( !fNxtChEscaped ) {
            auto x_Cx( 0 );
            while( sit != src.cend() && (*sit == ' ' || *sit == HTAB) ) {
               if( *sit == HTAB ) {
                  x_Cx = 0;
                  destCol = tabr.ColOfNextTabStop( destCol );
                  *dit++ = HTAB;
                  }
               else {
                  ++x_Cx;
                  ++destCol;
                  if( tabr.ColAtTabStop(destCol) ) {
                     *dit++ = (x_Cx == 1) ? ' ' : HTAB;
                     x_Cx = 0;
                     }
                  }
               ++sit;
               }
            while( x_Cx-- ) {
               *dit++ = ' ';
               }
            }
         if( sit != src.cend() && !fNxtChEscaped ) {
            switch( *sit ) {
               case chQuot1:
               case chQuot2:  fInQuotedRgn = true;
                              quoteCh = *sit;
                              break;
               case chESC:    fNxtChEscaped = true; // ESCAPE char, not PathSepCh!
                              break;
               default:       break;
               }
            }
         else {
            fNxtChEscaped = false;
            }
         }
      else {
         if( sit != src.cend() && !fNxtChEscaped ) {
            if(      *sit == quoteCh ) { fInQuotedRgn = false; }
            else if( *sit == chESC   ) { fNxtChEscaped = true; } // ESCAPE char, not PathSepCh!
            }
         else {
            fNxtChEscaped = false;
            }
         }
      if( sit != src.cend() ) {
         *dit++ = *sit++;
         ++destCol;
         }
      }
   }

STATIC_FXN void spcs2tabs_all( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto xCol(0);
   auto sit( src.cbegin() );
   while( sit != src.cend() ) {
      auto ix(0);
      while( sit != src.cend() && (*sit == ' ' || *sit == HTAB) ) {
         if( *sit == HTAB ) {
            ix = 0;
            xCol = tabr.ColOfNextTabStop( xCol );
            *dit++ = HTAB;
            }
         else {
            ++ix;
            ++xCol;
            if( tabr.ColAtTabStop(xCol) ) {
               *dit++ = (--ix == 0) ? ' ' : HTAB;
               ix = 0;
               }
            }
         ++sit;
         }
      while( ix-- ) {
         *dit++ = ' ';
         }
      if( sit != src.cend() ) {
         *dit++ = *sit++;
         ++xCol;
         }
      }
   }

STATIC_FXN void spcs2tabs_leading( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto xCol( 0 );
   auto ix(0);
   auto sit( src.cbegin() );
   for( ; sit != src.cend() && (*sit == ' ' || *sit == HTAB) ; ++sit ) {
      if( *sit == HTAB ) {
         ix = 0;
         xCol = tabr.ColOfNextTabStop( xCol );
         *dit++ = HTAB;
         }
      else {
         ++ix;
         ++xCol;
         if( tabr.ColAtTabStop(xCol) ) {
            *dit++ = (--ix == 0) ? ' ' : HTAB;
            ix = 0;
            }
         }
      }
   while( ix-- ) {
      *dit++ = ' ';
      }
   for( ; sit != src.cend() ; ++sit ) {
      *dit++ = *sit;
      }
   }

//*******************************  END TABS *******************************
//*******************************  END TABS *******************************
//*******************************  END TABS *******************************

//==========================================================================================

class lineIterator { enum { DV=0 };
   stref d_remainder;
public:
   lineIterator( stref remainder ) : d_remainder( remainder ) { DV&&DBG( "ctor: '%" PR_BSR "'", BSR(d_remainder) ); }
   bool empty() const { return d_remainder.empty(); }
   stref next() {
      if( d_remainder.empty() ) { return stref(); }
      auto len( nposToEnd( d_remainder, d_remainder.find_first_of( "\n\r" ) ) );
      const auto rv( d_remainder.substr( 0, len ) );
      d_remainder.remove_prefix( len );
      if( !d_remainder.empty() ) { // d_remainder[0] === '\n' or '\r'
         auto toRmv( 1 );
         // accommodate all possible EOL sequences:
         // Windows => "\x0D\x0A".
         // UNIX    => "\x0A"
         // MacOS   => "\x0A\x0D"
         // ???     => "\x0D"
         if( d_remainder.length() > 1 &&
               (d_remainder[0] == '\n' && d_remainder[1] == '\r')
            || (d_remainder[0] == '\r' && d_remainder[1] == '\n')
           ) { ++toRmv; }
         d_remainder.remove_prefix( toRmv ); // skip logical newline (may be one or two characters)
         }
      DV&&DBG( "next: '%" PR_BSR "'", BSR(rv) );
      return rv;
      }
   };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void FBUF::cat( PCChar pszNewLineData ) {  // used by Lua's method of same name
   BoolOneShot first;
   std::string tmp;
   lineIterator li( pszNewLineData );
   while( !li.empty() ) {
      auto ln( li.next() );
      if( first && !ln.empty() ) {
         const auto rl( PeekRawLine( LastLine() ) );
         std::string lbuf; lbuf.reserve( rl.length() + ln.length() );
         lbuf.assign( rl.data(), rl.length() );
         lbuf.append( ln.data(), ln.length() );
         PutLine( LastLine(), lbuf, tmp );
         }
      else {
         PutLine( LineCount(), ln, tmp );
         }
      }
   }

int FBUF::PutLastMultiline( PCChar buf ) {
   lineIterator li( buf );
   auto lineCount( 0 );
   std::string tmp;
   while( !li.empty() ) {
      PutLine( LineCount(), li.next(), tmp );
      ++lineCount;
      }
   return lineCount;
   }

void FBUF::xvsprintf( PXbuf pxb, LINE lineNum, PCChar format, va_list val ) {
   std::string tmp;
   pxb->vFmtStr( format, val );
   lineIterator li( pxb->sr() );
   while( !li.empty() ) {
      InsLine( lineNum++, li.next(), tmp );
      }
   }

void FBUF::Vsprintf( LINE lineNum, PCChar format, va_list val ) {
   std::string tmp;
   Xbuf xb;
   xb.vFmtStr( format, val );
   lineIterator li( xb.sr() );
   while( !li.empty() ) {
      InsLine( lineNum++, li.next(), tmp );
      }
   }

void FBUF::xFmtLine( PXbuf pxb, LINE lineNum, PCChar format, ...  ) {
   va_list val;  va_start( val, format );
   xvsprintf( pxb, lineNum, format, val );
   va_end( val );
   }

void FBUF::FmtLine( LINE lineNum, PCChar format, ...  ) {
   va_list val;  va_start( val, format );
   Vsprintf( lineNum, format, val );
   va_end( val );
   }

void FBUF::xvFmtLastLine( PXbuf pxb, PCChar format, va_list val ) {
   xvsprintf( pxb, LineCount(), format, val );
   }

void FBUF::vFmtLastLine( PCChar format, va_list val ) {
   Vsprintf( LineCount(), format, val );
   }

void FBUF::xFmtLastLine( PXbuf pxb, PCChar format, ...  ) {
   va_list val;  va_start( val, format );
   xvFmtLastLine( pxb, format, val );
   va_end( val );
   }

void FBUF::FmtLastLine( PCChar format, ...  ) {
   va_list val;  va_start( val, format );
   vFmtLastLine( format, val );
   va_end( val );
   }

void FBUF::PutLine( LINE yLine, stref srSrc, std::string &tmpbuf ) {
   // if( IsNoEdit() ) { DBG( "%s on noedit=%s", __PRETTY_FUNCTION__, Name() ); }
   BadParamIf( , IsNoEdit() );
   if( ENTAB_0_NO_CONV != Entab() ) {
      tmpbuf.clear();
      const Tabber tabr( this->TabWidth() );
      switch( Entab() ) { // compress spaces into tabs per this->Entab()
         default:
         case ENTAB_0_NO_CONV:                   Assert( 0 );                                                    break;
         case ENTAB_1_LEADING_SPCS_TO_TABS:      spcs2tabs_leading       ( back_inserter(tmpbuf), srSrc, tabr ); break;
         case ENTAB_2_SPCS_NOTIN_QUOTES_TO_TABS: spcs2tabs_outside_quotes( back_inserter(tmpbuf), srSrc, tabr ); break;
         case ENTAB_3_ALL_SPC_TO_TABS:           spcs2tabs_all           ( back_inserter(tmpbuf), srSrc, tabr ); break;
         }
      srSrc = tmpbuf;
      }
   if( !TrailSpcsKept() ) {
      auto trailSpcs = 0;
      for( auto it( srSrc.crbegin() ) ; it != srSrc.crend() && isblank( *it ) ; ++it ) {
         ++trailSpcs;
         }
      srSrc.remove_suffix( trailSpcs );
      }
   if(   yLine > LastLine()             // no existing content?
      || srSrc != PeekRawLine( yLine )  // new content != existing content?
     ) {
      DirtyFBufAndDisplay();
      const auto minLineCount( yLine + 1 );
      if( LineCount() < minLineCount ) { 0 && DBG("%s Linecount=%d", __func__, minLineCount );
         FBOP::PrimeRedrawLineRangeAllWin( this, LastLine(), yLine ); // needed with addition of g_chTrailLineDisp; past-EOL lines need to be overwritten
         SetLineInfoCount( minLineCount );
         SetLineCount    ( minLineCount );
         }
      else {
         FBOP::PrimeRedrawLineRangeAllWin( this, yLine, yLine );
         }
      UndoReplaceLineContent( yLine, srSrc );  // actually perform the write to this FBUF
      }
   }

void FBUF::PutLine( LINE yLine, const std::vector<stref> &vsrSrc, std::string &stbuf0, std::string &stbuf1 ) {
   stbuf0.clear();
   for( const auto &sr : vsrSrc ) {
      stbuf0.append( sr.data(), sr.length() );
      }
   PutLine( yLine, stbuf0, stbuf1 );
   }

void FBUF::PutLine( LINE yLine, CPCChar pa[], int elems ) {
   std::string sbuf, tmp;
   for( auto ix(0); ix<elems; ++ix ) {
      sbuf.append( pa[ix] );
      }
   PutLine( yLine, sbuf, tmp );
   }

COL ColOfFreeIdx( COL tabWidth, stref content, sridx offset ) {
   const Tabber tabr( tabWidth );
   auto xCol( 0 );
   for( auto it( content.cbegin() ) ; it != content.cend() ; ++it ) {
      if( std::distance( content.cbegin(), it ) == offset ) {
         return xCol;
         }
      switch( *it ) {
         default  : ++xCol;                                break;
         case HTAB: xCol = tabr.ColOfNextTabStop( xCol );  break;
         }
      }
   return xCol + (offset - content.length());  // 'offset' indexes _past_ content: assume all chars past EOL are spaces (non-tabs)
   }

STATIC_FXN bool DeletePrevChar( const bool fEmacsmode ) { PCFV;
   const auto yLine( pcv->Cursor().lin );
   if( pcv->Cursor().col == 0 ) { // cursor @ beginning of line?
      if( yLine == 0 ) {
         return false; // no prev char
         }
      auto xCol( FBOP::LineCols( pcf, yLine-1 ) );
      if( fEmacsmode ) { // join current and prev lines
         pcf->DelStream( xCol, yLine-1, 0, yLine );
         }
      pcv->MoveCursor( yLine-1, xCol );
      return true;
      }
   const auto x0( pcv->Cursor().col );
   const auto colsDeld( FBOP::DelChar( pcf, yLine, x0 - 1 ) );
   pcv->MoveCursor( yLine, x0 - Max( colsDeld, 1 ) );
   return true;
   }

bool ARG::cdelete  () { return DeletePrevChar( false ); }
bool ARG::emacscdel() { return DeletePrevChar( true  ); }

//------------------------------------------------------------------------------

STATIC_FXN void GetLineWithSegRemoved( PFBUF pf, std::string &dest, const LINE yLine, const COL xLeft, const COL boxWidth, bool fCollapse ) {
   pf->getLineTabxPerRealtabs( dest, yLine );
   const auto tw( pf->TabWidth() );
   const auto xEolNul( StrCols( tw, dest ) );
   if( xEolNul <= xLeft ) { 0 && DBG( "%s xEolNul(%d) <= xLeft(%d)", __func__, xEolNul, xLeft );
      return;
      }
   const auto ixLeft( CaptiveIdxOfCol( tw, dest, xLeft ) );
   const auto xRight( xLeft + boxWidth ); // dest[xRight] will be 0th char of kept 2nd segment
   if( xRight >= xEolNul ) { // trailing segment of line is being deleted?
      0 && DBG( "%s trim, %u <= %d '%c'", __func__, xEolNul, xRight, dest[ixLeft] );
      dest.resize( ixLeft ); // the first (leftmost) char in the selected box
      return;
      }
   const auto ixRight( CaptiveIdxOfCol( tw, dest, xRight ) );
   const auto charsInGap( ixRight - ixLeft );
   if( charsInGap > 0 ) {
      if( fCollapse ) { /* Collapse */     0 && DBG( "b4:%s'", dest.c_str() );
         dest.erase( ixLeft, charsInGap ); 0 && DBG( "af:%s'", dest.c_str() );
         }
      else { // fill Gap w/blanks
         dest.replace( ixLeft, charsInGap, charsInGap, ' ' );
         }
      }
   }

void FBUF::DelBox( COL xLeft, LINE yTop, COL xRight, LINE yBottom, bool fCollapse ) {
   if( xRight < xLeft ) {
      return;
      }
   0 && DBG( "%s Y:[%d,%d] X:[%d,%d]", __func__, yTop, yBottom, xLeft, xRight );
   AdjMarksForBoxDeletion( this, xLeft, yTop, xRight, yBottom );
   const auto boxWidth( xRight - xLeft + 1 );
   std::string src; std::string stmp;
   for( auto yLine( yTop ); yLine <= yBottom; ++yLine ) {
      GetLineWithSegRemoved( this, src, yLine, xLeft, boxWidth, fCollapse );
      PutLine( yLine, src, stmp );
      }
   }

// NB: See "STREAM definition" in ARG::FillArgStruct to understand parameters!
// Nutshell: LAST CHAR OF STREAM IS EXCLUDED from operation!
//
void FBUF::DelStream( COL xStart, LINE yStart, COL xEnd, LINE yEnd ) {
   if( yStart == yEnd ) { 0 && DBG( "%s [%d..%d]", __func__, xStart, xEnd-1 );
      DelBox( xStart, yStart, xEnd-1, yStart ); // xEnd-1 because "LAST CHAR OF STREAM IS EXCLUDED from operation!"
      return;
      }
   std::string stFirst; DupLineSeg( stFirst, yStart, 0, xStart-1 );
   DelLines( yStart, yEnd - 1 );
   std::string stLast;  DupLineSeg( stLast, yStart, xEnd, COL_MAX );
   stFirst += stLast;
   PutLine( yStart, stFirst, stLast );
   AdjMarksForInsertion( this, this, xEnd, yStart, COL_MAX, yStart, xStart, yStart );
   }

//====================================================================================================

// BUGBUG: multi-clipboard support; not yet used.
// TBD:
//    1.  How to show clip-select menu?  (Use LUA!)
//    2.  Hiding MULTIPLE clips: use regex /<clip[0-9]>/ ?
//
struct clipInfo {
   PFBUF  pFBuf;
   int    contentType;
   };

STATIC_VAR struct {
   int      curIdx;
   clipInfo info[5];
   } s_Clip;

PFBUF GetClipFBufToRead( int *pClipboardArgType ) {
   const auto &cinfo( s_Clip.info[ s_Clip.curIdx ] );
   if( cinfo.pFBuf ) {
      *pClipboardArgType = cinfo.contentType;
      }
   return cinfo.pFBuf;
   }

PFBUF GetNextClipFBufToWrite( int clipboardArgType ) {
   const auto start( s_Clip.curIdx );
   while( start != (s_Clip.curIdx = (s_Clip.curIdx + 1) % ELEMENTS( s_Clip.info )) ) {
      auto &cinfo( s_Clip.info[ s_Clip.curIdx ] );
      if( cinfo.pFBuf ) {
         if( !cinfo.pFBuf->IsNoEdit() ) {
            cinfo.pFBuf->MakeEmpty();
            cinfo.pFBuf->MoveCursorToBofAllViews();
            cinfo.contentType = clipboardArgType;
            return cinfo.pFBuf;
            }
         }
      else {
         cinfo.contentType = clipboardArgType;
         return FBOP::FindOrAddFBuf( FmtStr<12>( "<clip%d>", s_Clip.curIdx ).c_str(), &cinfo.pFBuf );
         }
      }
   // _ALL_ aClipFBufs have been made readonly!  User has to pick one to overwrite, OR cancel the copy-to-clip op
   // MenuChooseClip( "Choose <clip> to overwrite" );
   const auto menuChoice( -1 );
   if( menuChoice < 0 ) {
      return nullptr;
      }
   s_Clip.curIdx = menuChoice;
   auto &cinfo( s_Clip.info[ s_Clip.curIdx ] );
   cinfo.pFBuf->ClrNoEdit();
   cinfo.pFBuf->MakeEmpty();
   cinfo.pFBuf->MoveCursorToBofAllViews();
   cinfo.contentType = clipboardArgType;
   return cinfo.pFBuf;
   }

//====================================================================================================

STIL void Clipboard_Prep( int clipboardArgType ) {
   if( g_pFbufClipboard != g_CurFBuf() ) {
       g_pFbufClipboard->MakeEmpty();
       }
   g_ClipboardType = clipboardArgType;
   }

STATIC_FXN void PCFV_Copy_STREAMARG_ToClipboard( ARG::STREAMARG_t const &d_streamarg ) {
   Clipboard_Prep( STREAMARG );  FBOP::CopyStream( g_pFbufClipboard, 0, 0, g_CurFBuf(), d_streamarg.flMin.col, d_streamarg.flMin.lin, d_streamarg.flMax.col, d_streamarg.flMax.lin );
   }

STATIC_FXN void PCFV_Copy_BOXARG_ToClipboard( ARG::BOXARG_t const &d_boxarg ) {
   Clipboard_Prep( BOXARG    );  FBOP::CopyBox   ( g_pFbufClipboard, 0, 0, g_CurFBuf(), d_boxarg.flMin.col, d_boxarg.flMin.lin, d_boxarg.flMax.col, d_boxarg.flMax.lin );
   }

STATIC_FXN void PCFV_Copy_LINEARG_ToClipboard( ARG::LINEARG_t const &d_linearg ) {
   Clipboard_Prep( LINEARG   );  FBOP::CopyLines(  g_pFbufClipboard, 0,    g_CurFBuf(), d_linearg.yMin, d_linearg.yMax );
   }

void PCFV_delete_STREAMARG( ARG::STREAMARG_t const &d_streamarg, bool copyToClipboard ) { PCFV;
   if( copyToClipboard ) { PCFV_Copy_STREAMARG_ToClipboard( d_streamarg ); }
   pcf->DelStream(  d_streamarg.flMin.col, d_streamarg.flMin.lin, d_streamarg.flMax.col, d_streamarg.flMax.lin );
   pcv->MoveCursor( d_streamarg.flMin.lin, d_streamarg.flMin.col );
   }

void PCFV_BoxInsertBlanks( ARG::BOXARG_t const &d_boxarg ) { PCFV;
   FBOP::CopyBox( pcf,
        d_boxarg.flMin.col, d_boxarg.flMin.lin, nullptr
      , d_boxarg.flMin.col, d_boxarg.flMin.lin
      , d_boxarg.flMax.col, d_boxarg.flMax.lin
      );
   }

void PCFV_delete_LINEARG( ARG::LINEARG_t const &d_linearg, bool copyToClipboard ) { PCFV;
   // LINEARG or BOXARG: Deletes the specified text and copies it to the
   // clipboard.  The argument is a LINEARG or BOXARG regardless of the
   // current selection mode.  The argument is a LINEARG if the starting
   // and ending points are in the same column.
   if( copyToClipboard ) { PCFV_Copy_LINEARG_ToClipboard( d_linearg ); }
   pcf->DelLines( d_linearg.yMin, d_linearg.yMax );
   pcv->MoveCursor( d_linearg.yMin, g_CursorCol() );
   }

void PCFV_delete_BOXARG( ARG::BOXARG_t const &d_boxarg, bool copyToClipboard, bool fCollapse ) { PCFV;
   if( copyToClipboard ) { PCFV_Copy_BOXARG_ToClipboard( d_boxarg ); }
   pcf->DelBox( d_boxarg.flMin.col, d_boxarg.flMin.lin, d_boxarg.flMax.col, d_boxarg.flMax.lin, fCollapse );
   pcv->MoveCursor( d_boxarg.flMin.lin, d_boxarg.flMin.col );
   }

void PCFV_delete_ToEOL( Point const &curpos, bool copyToClipboard ) { PCFV;
   auto xMax( FBOP::LineCols( pcf, curpos.lin ) );
   decltype(xMax) xLeft ;
   decltype(xMax) xRight;
   if( curpos.col <= xMax ) {
      xLeft  = curpos.col;
      xRight = xMax;
      }
   else {
      xLeft  = xMax;
      xRight = curpos.col;
      }
   --xRight;
   0 && DBG( "%s: xRight=%d", __func__, xRight );
   PCFV_delete_BOXARG( {curpos.lin, xLeft, curpos.lin, xRight }, copyToClipboard );
   }

bool ARG::sdelete() { PCFV;
   switch( d_argType ) {
    default:        return BadArg();
    case NOARG:     FBOP::DelChar( pcf, pcv->Cursor().lin, pcv->Cursor().col ); break; // Delete the CHARACTER at the cursor w/o saving it to <clipboard>
    case NULLARG:   PCFV_delete_STREAMARG( { d_nullarg.cursor.lin, d_nullarg.cursor.col, d_nullarg.cursor.lin+1, 0 }, !d_fMeta ); break;  // Deletes text from the cursor to the end of the line, INCLUDING THE LINE BREAK.
    case BOXARG:    // STREAMARG ³ BOXARG ³ LINEARG: Deletes the selected stream of text
                    // from the starting point of the selection to the cursor and copies
                    // it to the clipboard.  This always deletes a stream of text,
                    // regardless of the current selection mode.
                    //lint -fallthrough
    case LINEARG:   ConvertLineOrBoxArgToStreamArg();
                    //lint -fallthrough
    case STREAMARG: PCFV_delete_STREAMARG( d_streamarg, !d_fMeta ); break;
    }
   return true;
   }

bool ARG::ldelete() { PCFV;
   if( d_argType == STREAMARG ) {
      ConvertStreamargToLineargOrBoxarg();
      }
   switch( d_argType ) {
    default:        return BadArg();
    case NOARG:     PCFV_delete_LINEARG( { d_noarg.cursor.lin, d_noarg.cursor.lin }, !d_fMeta ); break; // Deletes the line at the cursor
    case NULLARG:   PCFV_delete_ToEOL( d_nullarg.cursor, !d_fMeta );                             break; // Deletes text from the cursor to the end of the line
    case LINEARG:   PCFV_delete_LINEARG( d_linearg, !d_fMeta );                                  break;
    case BOXARG:    PCFV_delete_BOXARG( d_boxarg, !d_fMeta );                                    break;
    }
   return true;
   }

bool ARG::udelete() { // "user interface" delete; does not convert BOX/LINE/STREAM ARGs; intended to replace ldelete on user's keyboard
   switch( d_argType ) {
    default:        return BadArg();
    case NOARG:     PCFV_delete_LINEARG( { d_noarg.cursor.lin, d_noarg.cursor.lin }, !d_fMeta ); break; // Deletes the line at the cursor
    case NULLARG:   PCFV_delete_ToEOL( d_nullarg.cursor, !d_fMeta );                             break; // Deletes text from the cursor to the end of the line
    case STREAMARG: PCFV_delete_STREAMARG( d_streamarg, !d_fMeta );                              break;
    case LINEARG:   PCFV_delete_LINEARG( d_linearg, !d_fMeta );                                  break;
    case BOXARG:    PCFV_delete_BOXARG( d_boxarg, !d_fMeta, d_cArg < 2 );                        break;
    }
   return true;
   }

bool ARG::delete_() {  // BUGBUG make this NOT save to clipboard!!! (current workaround: del key assigned to "meta delete")
   switch( d_argType ) {
      default:      sdelete();  return true;
      case LINEARG: //lint -fallthrough
      case BOXARG:  ldelete();  return true;
      }
   }

bool ARG::sinsert() { PCF;
   switch( d_argType ) {
    default:          return BadArg();
    case NOARG:       FBOP::CopyBox( pcf,
                           d_noarg.cursor.col, d_noarg.cursor.lin, nullptr
                         , d_noarg.cursor.col, d_noarg.cursor.lin
                         , d_noarg.cursor.col, d_noarg.cursor.lin
                         );
                      return true;
    case NULLARG:     FBOP::CopyStream( pcf,
                           d_nullarg.cursor.col, d_nullarg.cursor.lin
                         , nullptr
                         , d_nullarg.cursor.col, d_nullarg.cursor.lin
                         , 0                   , d_nullarg.cursor.lin + 1
                         );
                      return true;
    case BOXARG:      //lint -fallthrough
    case LINEARG:     ConvertLineOrBoxArgToStreamArg();
                      //lint -fallthrough
    case STREAMARG:   FBOP::CopyStream( pcf,
                           d_streamarg.flMin.col, d_streamarg.flMin.lin
                         , nullptr
                         , d_streamarg.flMin.col, d_streamarg.flMin.lin
                         , d_streamarg.flMax.col, d_streamarg.flMax.lin
                         );
                      return true;
    }
   }

bool ARG::copy() {
   switch( d_argType ) {
    default:        return BadArg();
    case NOARG:     PCFV_Copy_LINEARG_ToClipboard  ( { d_noarg.cursor.lin, d_noarg.cursor.lin } );  break;  // Copies the line at the cursor to the clipboard.
    case LINEARG:   PCFV_Copy_LINEARG_ToClipboard  ( d_linearg   );  break;
    case STREAMARG: PCFV_Copy_STREAMARG_ToClipboard( d_streamarg );  break;
    case BOXARG:    PCFV_Copy_BOXARG_ToClipboard   ( d_boxarg    );  break;
    case TEXTARG:   0 && DBG( "%s: textarg.len=%d", __func__, Strlen( d_textarg.pText ) );
                    if( d_textarg.pText[0] == 0 ) {
                       Clipboard_Prep( 0 );  // 0 == EMPTY
                       }
                    else {
                       Clipboard_Prep( BOXARG );
                       std::string stmp;
                       g_pFbufClipboard->PutLine( 0, d_textarg.pText, stmp );
                       }
                    break;
    }
   return true;
   }

bool ARG::linsert() { PCF;
   if( d_argType == STREAMARG ) {
      ConvertStreamargToLineargOrBoxarg();
      }
   switch( d_argType ) {
    default:        return BadArg();
    case NULLARG:   {
                    // Inserts or deletes blanks at the beginning of a line to move the
                    // first nonblank character to the cursor.
                    // (same as NOARG aligncol?)
                    const auto rl( pcf->PeekRawLine( d_nullarg.cursor.lin ) );
                    const auto ixNonb( FirstNonBlankOrEnd( rl ) );
                    const auto  xNonb( CaptiveIdxOfCol( pcf->TabWidth(), rl, ixNonb ) );
                    std::string sbuf;
                    if     ( xNonb > d_nullarg.cursor.col ) {
                       GetLineWithSegRemoved( pcf, sbuf, d_nullarg.cursor.lin, d_nullarg.cursor.col, xNonb - d_nullarg.cursor.col, true );
                       }
                    else if( xNonb < d_nullarg.cursor.col ) {
                       pcf->DupLineForInsert( sbuf, d_nullarg.cursor.lin, xNonb, d_nullarg.cursor.col - xNonb );
                       }
                    if( sbuf.length() ) {
                       std::string stmp;
                       pcf->PutLine( d_nullarg.cursor.lin, sbuf, stmp );
                       }
                    } break;
    case NOARG:     pcf->InsBlankLinesBefore( d_noarg.cursor.lin );  // Inserts one blank line above the current line.
                    break;
    case LINEARG:   // LINEARG or BOXARG: Inserts blanks within the specified area.  The
                    // argument is a linearg or boxarg regardless of the current selection
                    // mode.  The argument is a linearg if the starting and ending points are
                    // in the same column.
                    pcf->InsBlankLinesBefore( d_linearg.yMin, d_linearg.yMax - d_linearg.yMin + 1 );
                    break;
    case BOXARG:    PCFV_BoxInsertBlanks( d_boxarg );
                    break;
    }
   return true;  // Linsert always returns true.
   }

COL FBOP::DelChar( PFBUF fb, LINE yPt, COL xPt ) {
   const auto tw( fb->TabWidth() );
   const auto rl( fb->PeekRawLine( yPt ) );
   const auto lc0( StrCols( tw, rl ) );
   xPt = TabAlignedCol( tw, rl, xPt );
   if( xPt >= lc0 ) { // xPt to right of line content?
      return 0;       // this is a nop
      }
   // Here we are deleting a CHARACTER, whereas DelBox deletes COLUMNS; if the character is an HTAB,
   // the # of COLUMNS to be deleted in order to delete the underlying CHARACTER is variable:
   // NB: xNxtChar != xPt+1 iff realtabs and (HTAB==rl[ ix(xPt) ])
   const auto xNxtChar( ColOfNextChar( tw, rl, xPt ) );
   fb->DelBox( xPt, yPt, xNxtChar-1, yPt );
   const auto rv( xNxtChar - xPt ); 0 && DBG( "%s returns %d-%d= %d", __func__, xNxtChar, xPt, rv );
   return rv;
   }

COL FBOP::PutChar_( PFBUF fb, LINE yLine, COL xCol, char theChar, bool fInsert, std::string &tmp1, std::string &tmp2 ) {
   const auto lc0( FBOP::LineCols( fb, yLine ) );
   fb->DupLineForInsert( tmp1, yLine, xCol, fInsert ? 1 : 0 );        0 && DBG( "%s 1=%" PR_BSR "'", __func__, BSR(tmp1) );
   const auto tw( fb->TabWidth() );
   const auto destIx( CaptiveIdxOfCol( tw, tmp1, xCol ) );
   if( !fInsert && theChar == tmp1[ destIx ] ) {
      return 0;       // this is a nop
      }
   tmp1[ destIx ] = theChar;                                          0 && DBG( "%s 2=%" PR_BSR "'", __func__, BSR(tmp1) );
   fb->PutLine( yLine, tmp1, tmp2 );
   // everything that follows is to determine the number of columns added by the insertion of theChar
   // (which is used to determine the new cursor position if a user keystroke op caused us to be doing this)
   // BUGBUG: this might be WRONG in the case of overwrite (replace, !fInsert) if theChar or what is replaces is an HTAB!
   const auto colsInserted( ColOfNextChar( tw, tmp1, xCol ) - xCol );
   if( xCol < lc0 ) { // actual content added?
      AdjMarksForInsertion( fb, fb, xCol, yLine, COL_MAX, yLine, xCol+colsInserted, yLine );
      }
   return colsInserted;
   }

#ifdef fn_xquote

STATIC_FXN PCCMD GetGraphic() {
   CPCCMD pCmd( CmdFromKbdForExec() );
   if( !pCmd || !pCmd->IsFnGraphic() ) {
      return nullptr;
      }
   return pCmd;
   }

STATIC_FXN int GetHexDigit() {
   PCCMD pCmd;
   while( !(pCmd=GetGraphic()) || !isxdigit( pCmd->d_argData.chAscii() ) ) {
      continue;
      }
   const char ch( tolower( pCmd->d_argData.chAscii() ) );
   return (ch <= '9') ? ch - '0' : ch + 10 - 'a';
   }

bool ARG::xquote() { // Xquote
   fnMsg( "hit 2 hex chars" );     auto val(  GetHexDigit() );
   fnMsg( "hit 1 more hex char" );      val = GetHexDigit() + (val * 16);
   const char buf[2] = { char(val & 0xFF), 0 };
   fnMsg( "0x%02X (%c)", val, val );
   return PushVariableMacro( buf );
   }

#endif

bool ARG::graphic() { enum { DB=0 };
   const char usrChar( d_pCmd->d_argData.chAscii() );
   // <000612> klg Finally did this!  Been needing it for YEARS!
   // g_delims, g_delimMirrors
   //                                                m4 `quoting'
   //                                                |
   STATIC_CONST char chOpeningDelim[]    = "_%'\"(<{[`";
   STATIC_CONST char chClosingDelim[]    = "_%'\")>}]`";
   STATIC_CONST char chClosingDelim_m4[] = "_%'\")>}]'";
   const auto ixMatch( stref(chOpeningDelim).find( usrChar ) );
   const char chClosing( ixMatch==stref::npos ? '\0' : (g_fM4backtickquote ? chClosingDelim_m4 : chClosingDelim)[ ixMatch ] );
   std::string tmp1, tmp2;
   if( d_argType == BOXARG ) {
      if( usrChar == ' ' ) { // insert spaces
         linsert();
         return true;
         }
      if( chClosing ) {
         const auto fConformRight( (d_cArg > 1 || (usrChar == chQuot2 || usrChar == chQuot1 || usrChar == chBackTick)) ); // word-conforming bracketing of a BOXARG?
         // if certain chars are hit when a BOX selection is current, surround the
         // selected text with matching delimiters (depending on the char hit)
         //
         const auto pf( g_CurFBuf() );
         const auto tw( pf->TabWidth() );
         for( auto curLine( d_boxarg.flMin.lin ); curLine <= d_boxarg.flMax.lin ; ++curLine ) {
            auto xMax( d_boxarg.flMax.col+1 );
            if( fConformRight ) {
               const auto rl( pf->PeekRawLine( curLine ) );                       DB && DBG( "rl='%" PR_BSR "'", BSR(rl) );
               const auto ixMin( FreeIdxOfCol( tw, rl, d_boxarg.flMin.col ) );
               if( ixMin < rl.length() ) {
                  const auto ixMax( FreeIdxOfCol( tw, rl, xMax ) );
                  auto rlSeg( rl.substr( ixMin, ixMax-ixMin ) );                  DB && DBG( "rlSeg='%" PR_BSR "'", BSR(rlSeg) );
                  rmv_trail_blanks( rlSeg );                                      DB && DBG( "rlSeg='%" PR_BSR "'", BSR(rlSeg) );
                  xMax = ColOfFreeIdx( tw, rl, ixMin + rlSeg.length() );
                  }
               }
            FBOP::InsertChar( pf, curLine, xMax              , chClosing, tmp1, tmp2 );
            FBOP::InsertChar( pf, curLine, d_boxarg.flMin.col, usrChar  , tmp1, tmp2 );
            }
         return true;
         }
      else if( 0 && (',' == usrChar) ) {
         // TBD: loop looking for spacey regions, replacing first char of each spacey region with a comma
         }
      }
   else if( d_argType == STREAMARG ) {
      if( chClosing ) {
         const auto pf( g_CurFBuf() );
         FBOP::InsertChar( pf, d_streamarg.flMax.lin, d_streamarg.flMax.col, chClosing, tmp1, tmp2 );
         FBOP::InsertChar( pf, d_streamarg.flMin.lin, d_streamarg.flMin.col, usrChar  , tmp1, tmp2 );
         return true;
         }
      }
   DelArgRegion();
   return PutCharIntoCurfileAtCursor( usrChar, tmp1, tmp2 );
   }

bool ARG::insert() {
   switch( d_argType ) {
    case BOXARG:  //-lint fallthrough
    case LINEARG: linsert(); return true;
    default:      sinsert(); return true;
    }
   }

bool ARG::emacsnewl() {
   if( ArgCount() != 0 ) {
      return newline();
      }
   const auto pfb( g_CurFBuf() );
   const auto xIndent( FBOP::GetSoftcrIndent( pfb ) );
   // Original bug report 20070305:
   //
   // "emacsnewl "touches" the current line even when the cursor is beyond
   // EOL and thus the content of said line is unchanged (this causes
   // tab-replacement changes).  Investigation shows that maybe
   // CopyStream should not be used, or that DupLineForInsert needs to be
   // modified to use the entab settings from the dest?"
   //
   // 20090228 kgoodwin My fix: avoid CopyStream if cursor at/past EoL:
   //    CopyStream always touches (rewrites) the current line in this case,
   //    modifying tabs, etc.
   //
   // PCChar bos, eos;
   // if( pfb->PeekRawLineExists( g_CursorLine(), &bos, &eos ) && ColOfPtr( pfb->TabWidth(), bos, eos-1, eos ) < g_CursorCol() ) {
   //    pfb->InsLine( g_CursorLine() + 1, "" );
   //    }
   // else {
      FBOP::CopyStream( pfb,
           g_CursorCol(), g_CursorLine()     // dest
         , nullptr // space-fill
         , g_CursorCol(), g_CursorLine()     // src
         , xIndent      , g_CursorLine() + 1 // src
         );
   // }
   g_CurView()->MoveCursor( g_CursorLine() + 1, xIndent );
   return true;
   }

bool ARG::paste() {
   switch( d_argType ) {
    default:        break;
    case STREAMARG: //-lint fallthrough
    case BOXARG:    //-lint fallthrough
    case LINEARG:   DelArgRegion(); // Replace the selected text with the contents of <clipboard>
                    break;
    case TEXTARG:   {
                    g_pFbufClipboard->MakeEmpty();
                    if( d_cArg < 2 ) {
                       Clipboard_PutText( d_textarg.pText );
                       }
                    else {
#if 0
                       //  Arg Arg <textarg> Paste
                       //
                       //      Copies the contents of the file specified by <textarg> to the
                       //      current file above the current line.
                       //
                       //  Arg Arg !<textarg> Paste
                       //
                       //      Runs <textarg> as an operating-system command, capturing the
                       //      command's output to standard output. The output is copied to the
                       //      clipboard and inserted above the current line.
                       //
                       pathbuf tmpfilenamebuf;
                       Pathbuf cmdstrbuf;
                       tmpfilenamebuf[0] = '\0'; // init to 'no tmpfile created'
                       auto pSrcFnm( StrPastAnyBlanks( d_textarg.pText ) ); // arg arg "!dir" paste
                       if( *pSrcFnm == '!' ) {
                          NOAUTO CPCChar pszCmd( pSrcFnm + 1 );
                          const auto tmpx( CompletelyExpandFName_wEnvVars( "$TMP:" PATH_SEP_STR "paste.$k$" ) );
                          bcpy( tmpfilenamebuf, tmpx.c_str() );
                          0 && DBG( "tmp '%s'", tmpfilenamebuf );
                          pSrcFnm = tmpfilenamebuf;
                          cmdstrbuf.Sprintf( "%s >\"%s\" 2>&1", pszCmd, tmpfilenamebuf );
                          RunChildSpawnOrSystem( cmdstrbuf );
                          }
                       if( FBUF::FnmIsPseudo( pSrcFnm ) ) {
                          cmdstrbuf.Strcpy( pSrcFnm );
                          }
                       else {
                          CompletelyExpandFName_wEnvVars( BSOB(cmdstrbuf), pSrcFnm );
                          }
                       const auto pFBuf( FindFBufByName( cmdstrbuf ) );
                       if( pFBuf ) {
                          if( pFBuf->RefreshFailedShowError() ) {
                             return false;
                             }
                          FBOP::CopyLines( g_pFbufClipboard, 0, pFBuf, 0, pFBuf->LastLine() );
                          }
                       else { // couldntFindFile
                          g_pFbufClipboard->ReadOtherDiskFileNoCreateFailed( cmdstrbuf );
                          }
                       if( tmpfilenamebuf[0] ) {
                          unlinkOk( tmpfilenamebuf );
                          }
                       g_ClipboardType = LINEARG;
#endif
                       }
                    }
                    break;
    } // switch

   0 && DBG( "g_ClipboardType = %04X", g_ClipboardType );

   switch( g_ClipboardType ) {
    default:        return false;
    case LINEARG:   FBOP::CopyLines( g_CurFBuf(), g_CursorLine(), g_pFbufClipboard, 0, g_pFbufClipboard->LastLine() );
                    return true;
    case STREAMARG: FBOP::CopyStream( g_CurFBuf(),
                         g_CursorCol()                                                   , g_CursorLine()
                       , g_pFbufClipboard
                       , 0                                                               , 0
                       , FBOP::LineCols( g_pFbufClipboard, g_pFbufClipboard->LastLine() ), g_pFbufClipboard->LastLine()
                       );
                    return true;
    case BOXARG:    {
                    const COL boxWidth( FBOP::LineCols( g_pFbufClipboard, 0 ) ); // w/clipboard in BOXARG mode is assumed that all lines have sm len
                    if( boxWidth == 0 ) { return false; }
                    FBOP::CopyBox( g_CurFBuf(),
                         g_CursorCol(), g_CursorLine()
                       , g_pFbufClipboard
                       , 0, 0
                       , boxWidth - 1 , g_pFbufClipboard->LastLine()
                       );
                    }
                    return true;
    }
   }

GLOBAL_VAR ARG noargNoMeta; // s!b modified!

bool PutCharIntoCurfileAtCursor( char theChar, std::string &tmp1, std::string &tmp2 ) { PCFV;
   if( pcf->CantModify() ) {
      return false;
      }
   auto yLine( pcv->Cursor().lin );
   auto xCol ( pcv->Cursor().col );
   if( g_fWordwrap && g_iRmargin > 0 ) {
      if( theChar == ' ' && xCol >= g_iRmargin ) {
         const auto xIndent( FBOP::GetSoftcrIndent( pcf ) );
         FBOP::CopyStream( pcf
                         , xCol   , yLine
                         , nullptr
                         , xCol   , yLine
                         , xIndent, yLine + 1
                         );
         pcv->MoveCursor( yLine + 1, xIndent );
         return true;
         }
      if( g_iRmargin + 5 <= xCol ) {
         pcf->DupLineForInsert( tmp1, yLine, xCol, 0 );
         const auto lbuf( tmp1.c_str() );
         for( auto ix( xCol - 1 ); ix > 1; --ix ) {
            if(   lbuf[ix-1] == ' '
               && lbuf[ix  ] == ' '
              ) {
               if( ix > 1 ) {
                  const auto xEnd( FBOP::GetSoftcrIndent( pcf ) );
                  const auto yNextLine( yLine + 1 );
                  FBOP::CopyStream( pcf
                                  , ix  , yLine
                                  , nullptr
                                  , ix  , yLine
                                  , xEnd, yNextLine
                                  );
                  xCol += xEnd - ix;
                  yLine = yNextLine;
                  pcv->MoveCursor( yLine, xCol );
                  break;
                  }
               }
            }
         }
      }
   pcv->MoveCursor( yLine, xCol + FBOP::InsertChar( pcf, yLine, xCol, theChar, tmp1, tmp2 ) );
   return true;
   }

sridx FreeIdxOfCol( const COL tabWidth, stref content, const COL colTgt ) {
   if( colTgt <= 0 ) {
      return 0;
      }
   const Tabber tabr( tabWidth );
   auto col( 0 );
   for( auto it( content.cbegin() ) ; it != content.cend() ; ++it ) {
      if( *it == HTAB ) {   col = tabr.ColOfNextTabStop( col );  }
      else              { ++col; }
      if( col > colTgt ) {
         return std::distance( content.cbegin(), it );
         }
      }
   return content.length() + (colTgt - col);
   }

STATIC_FXN void sweep_CaptiveIdxOfCol( COL tw, PCChar content ) {
   const stref bbb( content );
   for( int ix( 0 ) ; ix <= bbb.length() + 3 ; ++ix ) {
      // printf( "%s( %s, %d ) -> %" PR_BSRSIZET "u\n", __func__, content, ix, CaptiveIdxOfCol( tw, bbb, ix ) );
      // above generates a warning: format '%u' expects argument of type 'unsigned int' while
      // passing the identical format string to FmtStr does not elicit such a warning
      printf( "%s", FmtStr<100>( "%s( %s, %d ) -> %" PR_BSRSIZET "u\n", __func__, content, ix, CaptiveIdxOfCol( tw, bbb, ix ) ).k_str() );
      }
   printf( "\n" );
   }

STATIC_FXN void sweep_FreeIdxOfCol( COL tw, PCChar content ) {
   const stref bbb( content );
   for( int ix( 0 ) ; ix <= bbb.length() + 3 ; ++ix ) {
      // see above for why we're using FmtStr here... bizarre!
      printf( "%s", FmtStr<100>( "%s( %s, %d ) -> %" PR_BSRSIZET "u\n", __func__, content, ix, FreeIdxOfCol( tw, bbb, ix ) ).k_str() );
      }
   printf( "\n" );
   }

STATIC_FXN void sweep_ColOfFreeIdx( COL tw, PCChar content, int maxCol ) {
   const stref bbb( content );
   for( int ix( 0 ) ; ix <= maxCol ; ++ix ) {
      printf( "%s( %s, %d ) -> %d\n", __func__, content, ix, ColOfFreeIdx( tw, bbb, ix ) );
      }
   printf( "\n" );
   }

void test_CaptiveIdxOfCol() {
   {
   auto testcmp = []( PCChar s1, PCChar s2 ) {
      const auto rv( cmpi( s1, s2 ) );
      const auto sv( Stricmp( s1, s2 ) );
      printf( "%s%c%s\n"  , s1, rv==0?'=':rv<0?'<':'>', s2 );
      printf( "%s%c%s\n\n", s1, sv==0?'=':rv<0?'<':'>', s2 );
      };
   auto a0 = "alpha", a1 = "Test", a2 = "test", a3 = "testing";
   testcmp(a0, a1);
   testcmp(a1, a0);
   testcmp(a0, a2);
   testcmp(a1, a2);
   testcmp(a1, a3);
   }
   if( 0 ) {
      const auto tw( 3 );
      sweep_CaptiveIdxOfCol( tw, ""    ); sweep_FreeIdxOfCol( tw, ""    ); sweep_ColOfFreeIdx( tw, ""   , 3    );
      sweep_CaptiveIdxOfCol( tw, "ab"  ); sweep_FreeIdxOfCol( tw, "ab"  ); sweep_ColOfFreeIdx( tw, "ab" , 5    );
      sweep_CaptiveIdxOfCol( tw, "\tb" ); sweep_FreeIdxOfCol( tw, "\tb" ); sweep_ColOfFreeIdx( tw, "\tb", tw*3 );
      sweep_CaptiveIdxOfCol( tw, "b\t" ); sweep_FreeIdxOfCol( tw, "b\t" ); sweep_ColOfFreeIdx( tw, "b\t", tw*3 );
      }
   }

//--------------------------------------------------------------------------------------------------

stref FBUF::PeekRawLineSeg( LINE yLine, COL xMinIncl, COL xMaxIncl ) const {
   auto rl( PeekRawLine( yLine ) );
   const auto tw( TabWidth() );
   const auto ixMinIncl( FreeIdxOfCol( tw, rl, xMinIncl ) );
   if( ixMinIncl >= rl.length() ) { return stref(); }
   const auto ixMaxIncl( CaptiveIdxOfCol( tw, rl, xMaxIncl ) ); 0 && DBG( "%d[%d/%" PR_SIZET ",%d/%" PR_SIZET "]=%" PR_SIZET "=%" PR_BSR "'", yLine, xMinIncl, ixMinIncl, xMaxIncl, ixMaxIncl, rl.length(), BSR(rl) );
   rl.remove_suffix( (rl.length()-1) - ixMaxIncl );
   rl.remove_prefix( ixMinIncl );                               0 && DBG( "%d[%d/%" PR_SIZET ",%d/%" PR_SIZET "]=%" PR_SIZET "=%" PR_BSR "'", yLine, xMinIncl, ixMinIncl, xMaxIncl, ixMaxIncl, rl.length(), BSR(rl) );
   return rl;
   }

#ifdef fn_rawline
bool ARG::rawline() {
   switch( d_argType ) {
    default:      return BadArg(); // arg "rawline:alt+r" assign
    case BOXARG: {const auto rls( g_CurFBuf()->PeekRawLineSeg( d_boxarg.flMin.lin, d_boxarg.flMin.col, d_boxarg.flMax.col ) );
                  const auto disp( FormatExpandedSeg( COL_MAX, rls, 0, g_CurFBuf()->TabWidth(), BIG_BULLET, SMALL_BULLET ) );
                  Msg( "PeekRawLineSeg '%" PR_BSR "'", BSR(disp) );
                  return !rls.empty();
                 }
    }
   }
#endif

COL FBUF::getLine_( std::string &dest, LINE yLine, int chExpandTabs ) const {
   FormatExpandedSeg( dest, COL_MAX, PeekRawLine( yLine ), 0, TabWidth(), chExpandTabs, ' ' );
   return dest.length();
   }

// gap: [xMinIncl..xMaxIncl]
// rules:
//    - if any chars exist in gap
//         then rv will be filled with these chars:
//            WITHOUT trailing-space padding being added
//            so if the line ends in the middle of the gap, rv.length() < gapChars
//    - if NO chars exist in gap
//         then rv.empty() == true
void FBUF::DupLineSeg( std::string &dest, LINE yLine, COL xMinIncl, COL xMaxIncl ) const {
   dest.clear();
   if( yLine >= 0 && yLine <= LastLine() ) {
      FormatExpandedSeg( dest, xMaxIncl - xMinIncl + 1, PeekRawLine( yLine ), xMinIncl, TabWidth() );
      }
   }

// open a (space-filled) insertCols-wide hole, with dest[xIns] containing the first inserted space;
//    original dest[xIns] is moved to dest[xIns+insertCols]
// if insertCols == 0 && dest[xIns] is not filled by existing content, spaces will be added [..xIns); dest[xIns] = 0
//
int FBUF::DupLineForInsert( std::string &dest, const LINE yLine, COL xIns, COL insertCols ) const { enum { DB=0 };
   getLineTabxPerRealtabs( dest, yLine );
   const auto tw       ( TabWidth() );
   const auto lineCols ( StrCols( tw, dest ) );                   DB && DBG( "%s: %" PR_BSR "| L %d (%d)", __func__, BSR(dest), lineCols, xIns );
   if( xIns > lineCols ) {                 // line shorter than insert point?
      dest.append( xIns - lineCols, ' ' ); // append spaces thru dest[xIns-1]; dest[xIns] == 0
      }
   if( insertCols > 0 ) {
      const auto ixIns( FreeIdxOfCol( tw, dest, xIns ) );         DB && DBG( "%s: %" PR_BSR "| L %d (%d) [%" PR_SIZET "]", __func__, BSR(dest), lineCols, xIns, ixIns );
      dest.insert( ixIns, insertCols, ' ' );
      }                                                           DB && DBG( "%s: %" PR_BSR "| L %" PR_SIZET " (%d)", __func__, BSR(dest), dest.length(), xIns );
   return dest.length();
   }

//--------------------------------------------------------------------------------------------------

#ifdef fn_csort
#error
struct LineSortRec {
   LINE    yLine;
   Linebuf lbuf;
   char    keydata[0];
   };

typedef  LineSortRec * PLineSortRec;
typedef PLineSortRec *PPLineSortRec;

STATIC_FXN int CDECL__ CmpLSRAscend( PCVoid p1, PCVoid p2 ) {
   return strcmp( (*PPLineSortRec( p1 ))->keydata
                , (*PPLineSortRec( p2 ))->keydata
                );
   }

STATIC_FXN int CDECL__ CmpLSRDescend( PCVoid p1, PCVoid p2 ) {
   return -CmpLSRAscend( p1, p2 );
   }

void FBOP::SortLineRange( PFBUF fb, const LINE yMin, const LINE yMax, const bool fAscending, const bool fCase, const COL xMin, const COL xMax, const bool fRmvDups ) {
   Assert( xMax >= xMin );
   Assert( yMax >= yMin );
   const auto lines   ( yMax - yMin + 1 );
   const auto keyBytes( xMax - xMin + 1 + 1 ); // extra +1 for NUL byte
   const auto ruKeyBytes( ROUNDUP_TO_NEXT_POWER2( keyBytes, 16 ) );  0 && DBG( "XXXXX  %d - > %d", keyBytes, ruKeyBytes );
   const auto sizeofLSR( sizeof( LineSortRec ) + ruKeyBytes );
   auto papLSR( PPLineSortRec( Alloc0d( lines * sizeof(PLineSortRec) ) ) );
   auto ppLSR ( papLSR );
   auto paLSR ( PLineSortRec( AllocNZ( lines * sizeofLSR ) ) );
   auto pLSR  ( paLSR );
   const auto yPastEnd( yMax + 1 );
   for( auto yY( yMin ) ; yY < yPastEnd ; ++yY ) {
      pLSR->yLine = yY;
      const auto chars( fb->getLineTabx( pLSR->lbuf, yY ) );
      if( chars > yMin ) {
         scpy( pLSR->keydata, keyBytes, pLSR->lbuf + xMin );
         if( !fCase ) { _strlwr( pLSR->keydata ); }
         }
      else {
         pLSR->keydata[0] = '\0';
         }
      fb->DupRawLine( pLSR->lbuf, yY );
      *ppLSR++ = pLSR;
      pLSR = PLineSortRec( PChar(pLSR) + sizeofLSR );
      }
   qsort( papLSR, lines, sizeof(PLineSortRec), fAscending ? CmpLSRAscend : CmpLSRDescend );
   ppLSR = papLSR;
   auto yMaxEff( yPastEnd );
   for( auto yY( yMin ) ; yY < yMaxEff ; ) {
      if(   fRmvDups
         && (ppLSR != papLSR) // ppLSR - 1 exists?
         && 0 == strcmp( (*ppLSR)->keydata, (*(ppLSR-1))->keydata )
        ) {
         // line is dup of it's predecessor (maybe later save to <dups0> ???)
         ++ppLSR; // skip *ppLSR w/o copying
         --yMaxEff;  // one less line in dest
         }
      else {
         fb->PutLine( yY++, (*ppLSR++)->lbuf );
         }
      }
   if( yMaxEff <= yPastEnd ) {
      fb->DelLine( yMaxEff, yPastEnd );
      }
   Free0( paLSR );
   Free0( papLSR );
   }

bool ARG::csort() {
   LINE yMin, yMax;  GetLineRange  ( &yMin, &yMax );
   COL  xMin, xMax;  GetColumnRange( &xMin, &xMax );
   FBOP::SortLineRange( g_CurFBuf(), yMin, yMax, d_cArg > 1, g_fCase, xMin, xMax, g_fMeta );
   return true;
   }
#endif

//--------------------------------------------------------------------------------------------------

void LineInfo::PutContent( stref src ) { // assume previous content has been destroyed!
   if( src.length() == 0 ) {
      d_pLineData = nullptr;
      }
   else {
      AllocBytesNZ( d_pLineData, src.length(), __func__ );
      memcpy( CAST_AWAY_CONST(PChar)(d_pLineData), src.data(), src.length() );
      }
   d_iLineLen = src.length();
   }

void FBUF::PutLineSeg( const LINE yLine, const stref &ins, std::string &stmp, std::string &dest, const COL xLeftIncl, const COL xRightIncl, const bool fInsert ) {
   enum { DE=0 };                                                              DE && DBG( "%s+ L %d [%d..%d] <= '%" PR_BSR "' )", __func__, yLine, xLeftIncl, xRightIncl, BSR(ins) );
   // insert ins into gap = [xLeftIncl, xRightIncl]
   // rules:
   //    - portion of ins placed into line is NEVER longer than gap
   //    - if chars WILL exist to right of gap, i.e.
   //         if  fInsert AND existing chars at or to right of xLeftIncl
   //         OR
   //         if !fInsert AND existing chars       to right of xRightIncl
   //         then ins is space padded to fill gap and will NOT terminate string.
   //      else ins is NOT space padded, will terminate string, perhaps to left of xRightIncl
   if( !fInsert && xLeftIncl == 0 && xRightIncl >= FBOP::LineCols( this, yLine ) ) { // a two-parameter call?
      DE && DBG( "%s- PutLine(simple) )", __func__ );
      PutLine( yLine, ins, stmp ); // optimal/trivial line-replace case
      }
   else { // segment ins/overwrite case
      const sridx holewidth( xRightIncl - xLeftIncl + 1 );
      const auto inslen( Min( ins.length(), holewidth ) );                     DE && DBG( "%s [%d L gap/inslen=%" PR_BSRSIZET "u/%" PR_BSRSIZET "u]", __func__, xLeftIncl, holewidth, inslen );
      DupLineForInsert( dest, yLine, xLeftIncl, fInsert ? holewidth : 0 );
      const auto tw( TabWidth() );
      const auto lcols( StrCols( tw, dest ) );
      const auto maxCol( fInsert ? lcols : xLeftIncl+inslen );                 DE && DBG( "%s GL4Ins: cch/col=%" PR_BSRSIZET "u/%d maxCol=%" PR_BSRSIZET "u", __func__, dest.length(), lcols, maxCol );
      const auto ixLeftIncl( FreeIdxOfCol( tw, dest, xLeftIncl ) );
      Assert( lcols >= xLeftIncl );
      // dest contains:
      //    !fInsert: at least xLeftIncl           chars
      //     fInsert: at least xLeftIncl+holewidth chars
      //
      // in any case, we need to copy (ins L inslen) into buf+xLeftIncl
      dest.replace( ixLeftIncl, inslen, ins.data(), inslen );
      // now, either
      // a. terminate the seg-zone immediately after (ins L inslen)
      // b. space-pad the remainder of the seg-zone (IF there are any original-line chars on the trailing side of the seg-zone), or
      if( lcols < xRightIncl ) {
         dest.resize( ixLeftIncl + inslen );
         }
      else if( holewidth > inslen ) {
         dest.replace( ixLeftIncl + inslen, holewidth - inslen, holewidth - inslen, ' ' );
         }
      DE && DBG( "%s- PutLine(merged) )", __func__ );
      PutLine( yLine, dest, stmp );
      }
   }

void LineInfo::FreeContent( const FBUF &fbuf ) {
   if( fCanFree_pLineData( fbuf ) ) {
      Free0( d_pLineData );
      }
   }

void FBUF::DirtyFBufAndDisplay() {
   if( IsDirty() ) {
      return;
      }
   if( this == g_CurFBuf() ) {
      DispNeedsRedrawStatLn();
      }
   SetDirty();
   }

enum { LineHeadSpace =  1
                 //    50
     , FileReadLineHeadSpace = 21
     };

void FBUF::InsertLines__( const LINE yInsAt, const LINE lineInsertCount, const bool fSaveUndoInfo ) {
   if( yInsAt > LastLine() ) { // no existing line inserted in front of?
      return;
      }
   FBOP::PrimeRedrawLineRangeAllWin( this, yInsAt, LineCount() + lineInsertCount );
   DirtyFBufAndDisplay();
   if( fSaveUndoInfo ) {
      UndoInsertLineRangeHole( yInsAt, lineInsertCount );  // generate a undo record
      }
   // 20091218 kgoodwin this version, in the event we have to realloc d_paLineInfo[], saves a redundant hole-opening MoveArray which is now done as part of copying
   // was: InsertLineInfo( yInsAt, lineInsertCount );  // make sure there's a place to open a hole into
   if( lineInsertCount > 0 ) {
      const auto linesNeeded( LineCount() + lineInsertCount );
      if( !d_paLineInfo ) {
         SetLineInfoCount( linesNeeded );
         }
      else {
         if( d_naLineInfoElements < linesNeeded ) {
            const auto linesToAlloc( linesNeeded + LineHeadSpace );
            LineInfo *pNewLi;  AllocArrayNZ( pNewLi, linesToAlloc, "Expanding d_paLineInfo" );
            if( yInsAt > 0 ) {             /* leading subrange exists?  copy it */                           0 && DBG( "%s -mov [%d]<-[%d] L %d", __func__, 0, 0, yInsAt );
               MoveArray( pNewLi                           , d_paLineInfo         ,               yInsAt );
               }
            if( !(yInsAt > LastLine()) ) { /* trailing subrange exists?  copy it */                          0 && DBG( "%s +mov [%d]<-[%d] L %d", __func__, (yInsAt+lineInsertCount), yInsAt, LineCount() - yInsAt );
               MoveArray( pNewLi + (yInsAt+lineInsertCount), d_paLineInfo + yInsAt, LineCount() - yInsAt );
               }
            FreeUp( d_paLineInfo, pNewLi );
            d_naLineInfoElements = linesToAlloc;
            if( linesToAlloc > linesNeeded ) {  // tail-hole exists?
               const auto arg0( d_naLineInfoElements - LineHeadSpace );  0 && DBG( "%s InitLineInfoRange(%d,%d)", __func__, arg0, LineHeadSpace );
               InitLineInfoRange( arg0, LineHeadSpace );  // empty the tail-hole
               }
            }
         else { // open a hole
            if( !(yInsAt > LastLine()) ) {
               MoveArray( d_paLineInfo + (yInsAt+lineInsertCount), d_paLineInfo + yInsAt, LineCount() - yInsAt );
               }
            }
         InitLineInfoRange( yInsAt, lineInsertCount );  // empty the hole
         }
      }
   // 20091218 kgoodwin
   IncLineCount( lineInsertCount );
   FBufEvent_LineInsDel( yInsAt, lineInsertCount );
   }

// This is an "INTERESTING" fxn!  It's called both by upper-level edit code (as
// DelLine) AND by Undo/Redo code (as DeleteLines__ForUndoRedo).  When
// DeleteLines__ForUndoRedo is called, fSaveUndoInfo == false.
//
// DelLine NEVER calls with (fSaveUndoInfo == false), so the memory
// leak which is noted below actually "can't happen"
//
void FBUF::DeleteLines__( LINE firstLine, LINE lastLine, bool fSaveUndoInfo ) {
   0 && DBG("%s [%d..%d]", __func__, firstLine, lastLine );
   if( firstLine > LastLine() ) { // if user is deleting lines that are displayed, but not actually present in the file
      return;
      }
   BadParamIf( , (firstLine > lastLine) );
   FBOP::PrimeRedrawLineRangeAllWin( this, firstLine, LineCount() );
   DirtyFBufAndDisplay();
   Min( &lastLine, LastLine() );
   if( fSaveUndoInfo ) {
      UndoSaveLineRange( firstLine, lastLine );
      }
 #if 0  // if you do this (to fix what looks like a memory leak), undo/redo BREAKS because it calls this API with fSaveUndoInfo == false
   else { // d_paLineInfo[firstLine..lastLine] abt to be overwritten: free
      for( auto iy(firstLine) ; iy <= lastLine ; ++iy ) {
         d_paLineInfo[iy].FreeContent();
         }
      }
 #endif
   // slide higher-numbered-line's lineinfo's down to fill in d_paLineInfo[firstLine..lastLine]
   MoveArray( d_paLineInfo + firstLine, d_paLineInfo + lastLine + 1, LastLine() - lastLine );
   const auto yDelta( lastLine - firstLine + 1 );
   IncLineCount( -yDelta ); // CANNOT swap with next stmt
   InitLineInfoRange( LineCount(), yDelta ); // fill in hole left by "slide ... down"
   FBufEvent_LineInsDel( firstLine, -yDelta );
   }

void FBUF::FreeLinesAndUndoInfo() { // purely destructive!
   DestroyMarks();
   { // formerly DestroyLineInfoArray()
   if( d_paLineInfo ) {
      for( auto iy(0) ; iy < d_LineCount ; ++iy ) {
         d_paLineInfo[iy].FreeContent( *this );
         }
      Free0( d_paLineInfo );
      }
   d_LineCount = 0;
   }
   d_naLineInfoElements = 0;
   DiscardUndoInfo();   // call before Free0( d_pOrigFileImage ) (some EditOp's have d_pFBuf)!
   Free0( d_pOrigFileImage );
   d_cbOrigFileImage = 0;
   UnDirty();
   }

void FBUF::FBufEvent_LineInsDel( LINE yLine, LINE lineDelta ) { // negative lineDelta value signifies deletion of lines
   Set_yChangedMin( yLine );
   DLINKC_FIRST_TO_LASTA( d_dhdViewsOfFBUF, dlinkViewsOfFBUF, pv ) {
      pv->ViewEvent_LineInsDel( yLine, lineDelta );
      }
   if( lineDelta > 0 )      { AdjustMarksForLineInsertion( yLine, lineDelta, this ); }
   else if( lineDelta < 0 ) { AdjustMarksForLineDeletion ( this, 0, yLine, COL_MAX, yLine - lineDelta + 1 ); }
   }

int FBUF::ViewCount() const {
   auto rv( 0 );
   DLINKC_FIRST_TO_LASTA( d_dhdViewsOfFBUF, dlinkViewsOfFBUF, pv ) {
      ++rv;
      }
   return rv;
   }

//***********************************************************************************************
//***********************************************************************************************
//***********************************************************************************************

//
// CopyLines: copy (RAW) a group of lines from one file to another.
//
// this:     The destination file.
//
// pFBufSrc: The source file.  Cannot be same as this (dest file).
//           If pFBufSrc == 0:
//              N = (ySrcEnd - ySrcStart + 1) blank lines are inserted into the
//              destination file.
//
//              If this is ALL you're trying to do, USE InsBlankLinesBefore()
//              INSTEAD!!!  The (pFBufSrc == 0) case exists to support
//              CopyStream() which has similar blank-fill variant semantics
//              which are not worth (from a duplicated-code perspective) moving
//              to a separate API.
//
//           else:
//              Lines [ySrcStart..ySrcEnd] are copied/inserted into the
//              destination file.
//
// The lines are inserted into the destination file directly BEFORE line
// yDestStart.
//
// Do not confuse CopyLines with PutLine. PutLine replaces a line, but
// does not affect the total number of lines. CopyLines inserts one or
// more lines, which increases the length of the file.
//

void FBOP::CopyLines( PFBUF FBdest, LINE yDestStart, PCFBUF FBsrc, LINE ySrcStart, LINE ySrcEnd ) {
   0 && DBG( "CopyLines [%d..%d] -> [%d]  '%s' -> '%s'"
           , ySrcStart, ySrcEnd
           , yDestStart
           , FBsrc  && FBsrc ->Name() ? FBsrc ->Name() : "?"
           , FBdest && FBdest->Name() ? FBdest->Name() : "?"
           );
   if( FBsrc == FBdest )     { DBG( "%s: src and dest cannot be the same buffer", FUNC ); return; }
   if( ySrcStart > ySrcEnd ) { DBG( "%s: ySrcStart > ySrcEnd", FUNC ); return; }
   FBdest->InsBlankLinesBefore( yDestStart, ySrcEnd - ySrcStart + 1 );
   if( FBsrc ) {
      std::string tmp;
      for( ; ySrcStart <= ySrcEnd; ++ySrcStart, ++yDestStart ) {
         FBdest->PutLine( yDestStart, FBsrc->PeekRawLine( ySrcStart ), tmp );
         }
      }
   }

// CopyStream copies a stream of text, including line breaks, from one
// file to another.
//
// The same file cannot serve as both source and destination.
//
// If pFBufSrc is 0, a stream of blanks is inserted.
//
// The text is inserted into the destination file just before the
// location specified by <xDst> and <yDest>.
//
// NB: See "STREAM definition" in ARG::FillArgStruct to understand parameters!
// Nutshell: Last char of stream IS NOT INCLUDED in operation!
//
void FBOP::CopyStream( PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcStart, LINE ySrcStart, COL xSrcEnd, LINE ySrcEnd ) {
   BadParamIf( , (FBdest == FBsrc) );
   BadParamIf( , (ySrcStart > ySrcEnd) );
   BadParamIf( , (ySrcStart == ySrcEnd && xSrcStart >= xSrcEnd) );
   if( ySrcStart == ySrcEnd ) { // single line?
      FBOP::CopyBox( FBdest, xDst, yDst, FBsrc, xSrcStart, ySrcStart, xSrcEnd-1, ySrcEnd );
      return;
      }
   //*** copy middle portion of stream
   FBOP::CopyLines( FBdest, yDst+1, FBsrc, ySrcStart+1, ySrcEnd );
   //*** merge & write last line of FBsrc stream  [srcbuf:destbuf]
   std::string destbuf; FBdest->DupLineForInsert( destbuf, yDst, xDst, 0 ); // rd dest line containing insertion point
   std::string srcbuf; auto tws( 1 );
   if( FBsrc ) {
      tws = FBsrc->TabWidth();
      FBsrc->DupLineForInsert( srcbuf, ySrcEnd, xSrcEnd, 0 );  // rd last line of src test
      }
   else {
      if( xSrcEnd > 0 ) { srcbuf.assign( xSrcEnd, ' ' ); }
      }
   const auto yDstLast( yDst + (ySrcEnd - ySrcStart) );
   std::string stmp;
   const auto ixDst   ( FreeIdxOfCol( FBdest->TabWidth(), destbuf, xDst ) ); // where destbuf text PAST insertion point; where destbuf content is split
   {
   const auto ixSrcEnd( FreeIdxOfCol( tws, srcbuf, xSrcEnd ) );
   srcbuf.replace( ixSrcEnd, srcbuf.length() - ixSrcEnd, destbuf, ixDst, std::string::npos ); // destbuf text PAST insertion point -> srcbuf past xSrcEnd
   //*** merge & write last line of FBsrc stream  srcbuf[0..ixSrcEnd) : destbuf[ixDst..end]]
   FBdest->PutLine( yDstLast, srcbuf, stmp );
   destbuf.erase( ixDst ); // this belongs as else case for if( FBsrc ) below, but uses this scope's ixDst
   }
   //*** merge & write first line of FBsrc stream [destbuf:srcbuf]
   if( FBsrc ) {
      FBsrc->DupLineForInsert( srcbuf, ySrcStart, xSrcStart, 0 );
      const auto ixSrcStart( CaptiveIdxOfCol( tws, srcbuf, xSrcStart ) );
      const auto alen( srcbuf.length() - ixSrcStart + 1 );
      destbuf.replace( ixDst, alen, srcbuf, ixSrcStart, alen );
      }
   FBdest->PutLine( yDst, destbuf, stmp );
   AdjMarksForInsertion( FBdest, FBdest, xDst     , yDst     , COL_MAX, yDst     , xSrcEnd-1, yDstLast );
   AdjMarksForInsertion( FBsrc , FBdest,         0, ySrcEnd  , xSrcEnd, ySrcEnd  ,         0, yDstLast );
   AdjMarksForInsertion( FBsrc , FBdest, xSrcStart, ySrcStart, COL_MAX, ySrcStart, xDst     , yDst     );
   }

// CopyBox copies a box of text from one file to another.
//
// The source and destination files are specified by pFBufSrc and
// <pFBufDest>. If pFBufSrc is 0, a box of blank spaces is inserted
// into the destination file.
//
// These arguments define the box to be copied:
//
// Argument     Position
//
// xLeft      First column
// yTop       First line
// xRight     Last column
// yBottom    Last line
//
// The text is inserted into the destination file just before the
// location specified by <xDest> and <yDest>.
//
// The same file can serve as source and destination, but the source
// and destination regions must not overlap. To copy overlapped
// regions, you must create a temporary intermediate file.
//
void FBOP::CopyBox( PFBUF FBdest, COL xDst, LINE yDst, PCFBUF FBsrc, COL xSrcLeft, LINE ySrcTop, COL xSrcRight, LINE ySrcBottom ) {
   BadParamIf( , (xSrcRight < xSrcLeft) );
   0 && DBG( "%s: ( xDst=%d, yDst=%d, src=%s, xSrcLeft=%d, ySrcTop=%d, xSrcRight=%d, ySrcBottom=%d )", __func__, xDst, yDst, FBsrc->Name(), xSrcLeft, ySrcTop, xSrcRight, ySrcBottom );
   if( (FBsrc == FBdest) &&
       (  (WithinRangeInclusive( xSrcLeft,                        xDst, xSrcRight ) && WithinRangeInclusive( ySrcTop, yDst                       , ySrcBottom) )
       || (WithinRangeInclusive( xSrcLeft, xSrcRight - xSrcLeft + xDst, xSrcRight ) && WithinRangeInclusive( ySrcTop, yDst - ySrcTop + ySrcBottom, ySrcBottom) )
       )
     ) { return; }
   AdjMarksForInsertion( FBsrc, FBdest, xSrcLeft, ySrcTop, xSrcRight, ySrcBottom, xDst, yDst );
   const auto tws( FBsrc ? FBsrc ->TabWidth() : 0 );
   const auto twd(         FBdest->TabWidth()     );
   const auto boxWidth( xSrcRight - xSrcLeft + 1 );
   std::string stSrc, stDst;  // BUGBUG one buffer would be nicer ...
   for( auto ySrc( ySrcTop ); ySrc <= ySrcBottom ; ++ySrc, ++yDst ) {
      FBdest->DupLineForInsert( stDst, yDst, xDst, boxWidth );
      if( FBsrc ) {
         FBsrc->DupLineForInsert( stSrc, ySrc, xSrcRight + 1, 0 );
         const auto ixLeft ( CaptiveIdxOfCol( tws, stSrc, xSrcLeft  ) );
         const auto ixRight( CaptiveIdxOfCol( tws, stSrc, xSrcRight ) );
         stDst.replace( CaptiveIdxOfCol( twd, stDst, xDst ), boxWidth, stSrc, ixLeft, ixRight - ixLeft + 1 );
         }
      FBdest->PutLine( yDst, stDst, stSrc );
      }
   }

void FBUF::SetLineInfoCount( const LINE linesNeeded ) {
   if( !d_paLineInfo || d_naLineInfoElements < linesNeeded ) { 0 && DBG( "XPf2NL LineInfo[] %s: (%d,%d) -> %d", Name(), LineCount(), d_naLineInfoElements, linesNeeded );
      const auto linesToAlloc( linesNeeded + LineHeadSpace );
      LineInfo *pNewLi;
      AllocArrayNZ( pNewLi, linesToAlloc, "Expanding d_paLineInfo" );
      if( d_paLineInfo ) {
         MoveArray( pNewLi, d_paLineInfo, LineCount() );
         Free_( d_paLineInfo );
         }
      d_paLineInfo = pNewLi;
      d_naLineInfoElements = linesToAlloc;
      InitLineInfoRange( LineCount(), linesToAlloc - LineCount() );
      }
   }

STIL void rdNoiseSeek() { DisplayNoise( kszRdNoiseSeek ); }
STIL void rdNoiseAllc() { DisplayNoise( kszRdNoiseAllc ); }
STIL void rdNoiseRead() { DisplayNoise( kszRdNoiseRead ); }
STIL void rdNoiseScan() { DisplayNoise( kszRdNoiseScan ); }

bool FBUF::ReadDiskFileFailed( int hFile ) {
   MakeEmpty();
   rdNoiseSeek();
   auto MBCS_skip( 0 );
   {
   auto fileBytes( fio::SeekEoF( hFile ) );
   0 && DBG( "fio::SeekEoF returns %8" WL( PR__i64 "u", "ld" ), fileBytes );
   Assert( fileBytes >= 0 );
   fio::SeekBoF( hFile );
   if( fileBytes > UINT_MAX ) {
      Msg( "filesize is larger than UINT_MAX" );
      return true;
      }
   rdNoiseAllc();
   PChar fileBuf;
   AllocBytesNZ( fileBuf, fileBytes+1, __func__ );
   fileBuf[fileBytes] = '\0'; // so wcslen works
   VR_(
      DBG( "ReadDiskFile %s: %uKB buf=[%p..%p)"
         , Name()
         , fileBytes/1024
         , fileBuf
         , fileBuf + fileBytes
         );
      )
   rdNoiseRead();
   MainThreadPerfCounter pc;
   if( !fio::ReadOk( hFile, fileBuf, fileBytes ) ) {
      Free0( fileBuf );
      ErrorDialogBeepf( "Read NOT OK!" );
      return true;
      }
   //--------------------------------------------------------------------------------------------------------
   // UTF-8:    [EX: .vcxproj files generated by VS13 are UTF-8]
   //
   // AS LONG AS the content is limited to Code Points representable by a single
   // octet (IOW, ASCII content, thus both UTF-8 and UTF-16 are 100% unnecessary),
   // UTF-8 files are directly editable.
   //
   STATIC_CONST unsigned char BoM_UTF8[] = { 0xEF, 0xBB, 0xBF }; // http://en.wikipedia.org/wiki/Byte_order_mark#UTF-8
   if( fileBytes >= sizeof(BoM_UTF8) ) {
      if( 0==memcmp( fileBuf, &BoM_UTF8, sizeof(BoM_UTF8) ) ) {
         MBCS_skip = sizeof(BoM_UTF8);
         // TODO BUGBUG set UTF8 content flag (and OBTW, support display, editing of UTF8 content!)
         d_OrigFileImageContentEncoding = TXTENC_UTF8;
         }
      }
   //
   // UTF-16:   [EX: .tlog files generated by MSBuild for VS13 contain UTF-16]
   //
   // UTF-16 files aren't readable when depicted by K (since for the most common
   // Code Points, the encoding is {char,0x00}.  Following is a hacky way to
   // achieve this end: convert UTF-16 content to CP_OEMCP.
   //
   // 20140424 kgoodwin
   //
   STATIC_CONST unsigned short BoM_UTF16( 0xFEFF ); // http://en.wikipedia.org/wiki/Byte_order_mark#UTF-16
   if( fileBytes >= sizeof(BoM_UTF16) ) {
      if( 0==memcmp( fileBuf, &BoM_UTF16, sizeof(BoM_UTF16) ) ) {
#ifdef _WIN32
         // adapted from http://stackoverflow.com/questions/3082620/convert-utf-16-to-utf-8
         Win32::LPCWSTR pszTextUTF16( reinterpret_cast<Win32::LPCWSTR>( fileBuf + sizeof(BoM_UTF16) ) );
         const auto utf16len( wcslen(pszTextUTF16) );
         const auto cp(
            //CP_UTF8   // if we could display UTF8, this would be the correct choice
              CP_OEMCP  // but we can't (and may never), so this is the safest choice
            );
         const auto utf8len( Win32::WideCharToMultiByte( cp, 0, pszTextUTF16, utf16len, nullptr, 0, nullptr, nullptr ) ); // note this returns an int (even in x64)
         0 && DBG( "reading UTF-16 file \"%s\": fileBytes=%" PR__i64 "u, utf16len=%" PR_SIZET ", utf8len=%d", Name(), fileBytes, utf16len, utf8len );
         PChar utf16buf;
         AllocBytesNZ( utf16buf, utf16len+1, __func__ );
         utf16buf[utf16len] = '\0';
         Win32::WideCharToMultiByte( cp, 0, pszTextUTF16, utf16len, utf16buf, utf8len, nullptr, nullptr );
         // adjust external state accordingly
         FreeUp( fileBuf, utf16buf );
         fileBytes = utf8len;
#endif
         }
      }
   //--------------------------------------------------------------------------------------------------------
   d_pOrigFileImage  = fileBuf;
   d_cbOrigFileImage = fileBytes;
   } // fileBuf, fileBytes out of scope
   //--------------------------------------------------------------------------------------------------------
   rdNoiseScan();
#if VERBOSE_READ
   const auto tmIO( pc.Capture() );
#endif
   d_EolMode = platform_eol;
   const auto initial_sample_lines( d_cbOrigFileImage == 0 ? 1        // alloc dummy so HasLines() will be true, preventing repetitive disk rereads
                                                           : 508 );   // slightly less than a power of 2
   d_naLineInfoElements = initial_sample_lines;
   VR_( DBG( "ReadDiskFile LineInfo       0 -> %7d", d_naLineInfoElements ); )
   AllocArrayNZ(      d_paLineInfo, d_naLineInfoElements, "initial d_paLineInfo" );
   InitLineInfoRange( 0           , d_naLineInfoElements );
   if( d_cbOrigFileImage > 0 ) {
      struct {
         int leadBlankLines = 0;
         int lead_Tab_Lines = 0;
         } tabStats;
      auto numCRs( 0 );
      auto numLFs( 0 );
      auto curLineNum( 0 );
      auto pLi( d_paLineInfo );
      auto pCurImageBuf( d_pOrigFileImage + MBCS_skip );
      const auto pPastImageBufEnd( d_pOrigFileImage + d_cbOrigFileImage );
      while( pCurImageBuf < pPastImageBufEnd ) { // we are about to read line 'curLineNum'
         if( d_naLineInfoElements <= curLineNum /* CID128050 */ && curLineNum > 0 /* CID128050 */ ) { // need to reallocate d_paLineInfo
            // this is a little obscure, since for brevity I'm using 'curLineNum' as an alias
            // for 'LineCount()'; these are equivalent since 'curLineNum' hasn't been stored yet.
            const auto abpl_scale( 1.025 );
            const double avgBytesPerLine( Max( abpl_scale, static_cast<double>(pCurImageBuf - d_pOrigFileImage) / curLineNum ) );
                  double dNewLineCntEstimate( (d_cbOrigFileImage / avgBytesPerLine) * abpl_scale );
            if( dNewLineCntEstimate <= d_naLineInfoElements ) {
                dNewLineCntEstimate = Max( static_cast<double>(d_naLineInfoElements * 1.125)
                                         , static_cast<double>(d_naLineInfoElements + FileReadLineHeadSpace)
                                         );
               }
            const auto newLineCntEstimate( static_cast<LINE>(dNewLineCntEstimate) );
            VR_(
               DBG( "ReadDiskFile LineInfo %7d -> %7d (avg=%4.1f)"
                  , d_naLineInfoElements
                  , newLineCntEstimate
                  , avgBytesPerLine
                  );
               )
            d_naLineInfoElements = Min( INT_MAX, newLineCntEstimate );
            //------------------------------------------------------------------------------------
            LineInfo *pNewLi;
            AllocArrayNZ( pNewLi, d_naLineInfoElements, "revised d_paLineInfo" );
            MoveArray(    pNewLi, d_paLineInfo, curLineNum );
            FreeUp( d_paLineInfo, pNewLi );
            InitLineInfoRange( curLineNum, d_naLineInfoElements - curLineNum );
            pLi = d_paLineInfo + curLineNum;
            }
         switch( *pCurImageBuf ) {
            case HTAB: ++tabStats.lead_Tab_Lines;
            case ' ' : ++tabStats.leadBlankLines; //lint -fallthrough
            default  : break;                     //lint -fallthrough
            }
         const auto pLineStart( pCurImageBuf );
         SetLineCount( curLineNum + 1 );
         auto cbEOL( 0 );
         do { // scan to next EOL-marker
            const auto ch( *pCurImageBuf++ );
#if 1
            if( ch == 0 ) {
               ++cbEOL;
               break;
               }
            if( ch == '\x0A' ) { // LF=Unix EOL?  treated as TRUE EOL indicator
               ++numLFs;
               ++cbEOL;
               break;            // treat as EOL
               }
            if( ch == '\x0D' ) { // CR=(start of) MS EOL?  simply skipped while awaiting next LF
               ++cbEOL;
               ++numCRs;
               continue;
               }
            if( cbEOL ) {        // unexpected: prev ch was CR but this ch is not LF (ancient Macintosh EOL=CR only???)
               --pCurImageBuf;   // current char is part of NEXT line
               break;            // treat as EOL
               }
#else
            if( (ch < ' ') && (ch != HTAB) ) { // ALL "control chars" except HTAB rendered invisible (swallowed up into EOL)
               ++cbEOL;
               switch( ch ) {
                  case '\x0A': ++numLFs; goto IS_EOL; // LF=Unix EOL?  treated as TRUE EOL indicator
                  case '\x0D': ++numCRs; break;       // CR=(start of) MS EOL?  simply skipped while awaiting next LF
                  default:               break;
                  }
               }
            else {
               if( cbEOL ) {      // unexpected: prev ch was CR but this ch is not LF (ancient Macintosh EOL=CR only???)
                  --pCurImageBuf; // current char is part of NEXT line
IS_EOL:
                  break;          // treat as EOL
                  }
               }
#endif
            } while( pCurImageBuf < pPastImageBufEnd );
         if( 0 && ExecutionHaltRequested() ) {
            ConIn::FlushKeyQueueAnythingFlushed();
            MakeEmpty();
            MoveCursorToBofAllViews();
            return true;
            }
         pLi->d_pLineData = pLineStart;
         pLi->d_iLineLen = (pCurImageBuf - pLineStart) - cbEOL;
         0 && DBG( "ReadDiskFile %s: L %d = %p L %d", Name(), curLineNum, pLi->d_pLineData, pLi->d_iLineLen );
         ++pLi;
         ++curLineNum;
         } // while( pCurImageBuf < pPastImageBufEnd )
      // switch away from the default EOL iff file is pure non-default
      0 && DBG( "numCRs=%u, numLFs%u", numCRs, numLFs );
#if defined(_WIN32)
      if( numLFs > 0 && numCRs == 0      ) { d_EolMode = EolLF  ; }
#else
      if( numLFs > 0 && numCRs == numLFs ) { d_EolMode = EolCRLF; }
#endif
      d_Entab = ENTAB_0_NO_CONV;
      enum { PERCENT_LEAD_BLANK_TO_CAUSE_ENTAB_MODE = 50 };
      if(  tabStats.leadBlankLines > 0
        && (    (tabStats.lead_Tab_Lines > (MAX_LINES / 100))  // mk sure no ovflw
           || (((tabStats.lead_Tab_Lines * 100) / tabStats.leadBlankLines) > PERCENT_LEAD_BLANK_TO_CAUSE_ENTAB_MODE )
           )
        ) {
         d_Entab = ENTAB_1_LEADING_SPCS_TO_TABS;
         }
      // maybe add a LineInfo shrinker algorithm here?
#if VERBOSE_READ
      NewScope {
         const auto tmScan( pc.Capture() );
         double pctSCAN( 100 * (tmScan / (tmScan + tmIO)) );
         unsigned wastedLIbytesNow( (d_naLineInfoElements - LineCount()) * sizeof(*d_paLineInfo) );
         STATIC_VAR unsigned wastedLIbytes ;  wastedLIbytes += wastedLIbytesNow     ;
         STATIC_VAR unsigned PredLC        ;  PredLC        += d_naLineInfoElements ;
         STATIC_VAR unsigned actualLC      ;  actualLC      += LineCount()          ;
         STATIC_VAR unsigned files         ;  files         += 1                    ;
         DBG( "ReadDiskFile Done  scan/IO=%4.1f%%  %7d (avg=%4.1f) wastage=%d/%dKB (%4.1f%%) cum=%dKB (%4.1f%%), %dKB per %d files"
            , pctSCAN
            , LineCount()
            , (static_cast<double>(d_cbOrigFileImage) - numBytesToProcessInImageBuf) / static_cast<double>(LineCount())
            , wastedLIbytesNow / 1024
            , (d_naLineInfoElements) * sizeof(*d_paLineInfo) / 1024
            , 100.0*(static_cast<double>(d_naLineInfoElements) - LineCount()) / static_cast<double>(LineCount())
            , wastedLIbytes / 1024
            , 100.0*(static_cast<double>(PredLC) - actualLC) / (double)actualLC
            , (wastedLIbytes / 1024) / files
            , files
            );
         }
#endif
      }
   UnDirty();
   return false;
   }

// these are new work based on ReadDiskFile 15/16-Mar-2003 klg

void FBUF::ImgBufAlloc( size_t bufBytes, int PreallocLines ) {
   d_ImgBufBytesWritten = 0;
   0 && DBG( "ImgBufAlloc Bytes=%" PR_SIZET ", Lines=%d", bufBytes, PreallocLines );
   d_cbOrigFileImage = bufBytes;
   AllocArrayNZ( d_pOrigFileImage, bufBytes, __func__ );
   SetLineCount( 0 );
   SetLineInfoCount( PreallocLines );
   }

LineInfo & FBUF::ImgBufNextLineInfo() {
   // LineCount is the NUMBER of the NEW LINE!
   SetLineInfoCount( LineCount()+1 );
   auto &newLI( d_paLineInfo[ LineCount() ] );
   if( LineCount() == 0 ) {
      newLI.d_pLineData = d_pOrigFileImage;
      }
   else {
      auto &preLI( d_paLineInfo[ LastLine() ] );
      newLI.d_pLineData = preLI.d_pLineData + preLI.d_iLineLen;
      }
   IncLineCount( 1 );  // LineCount() NOT CHANGED UNTIL HERE
   return newLI;
   }

void FBUF::ImgBufAppendLine( stref src ) {
   auto &newLI( ImgBufNextLineInfo() );
   newLI.d_iLineLen = src.length();
   memcpy( CAST_AWAY_CONST(PChar)(newLI.d_pLineData), src.data(), newLI.d_iLineLen );
   0 && DBG( "ImgBufAppendLine Bytes=%d", newLI.d_iLineLen );
   d_ImgBufBytesWritten += newLI.d_iLineLen;
   }

void FBUF::ImgBufAppendLine( PFBUF pFBufSrc, int srcLineNum, PCChar prefix ) {
   auto &newLI( ImgBufNextLineInfo() );
   auto preLen(0);
   if( prefix ) {
      memcpy( CAST_AWAY_CONST(PChar)(newLI.d_pLineData), prefix, (preLen = Strlen( prefix )) );
      }
   auto &srcLI( pFBufSrc->d_paLineInfo[srcLineNum] );
   memcpy( CAST_AWAY_CONST(PChar)(newLI.d_pLineData)+preLen, srcLI.d_pLineData, srcLI.d_iLineLen );
   newLI.d_iLineLen = srcLI.d_iLineLen + preLen;
   0 && DBG( "FBUF::ImgBufAppendPFLine Bytes=%d", newLI.d_iLineLen );
   d_ImgBufBytesWritten += newLI.d_iLineLen;
   }

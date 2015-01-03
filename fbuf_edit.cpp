//
// Copyright 1991 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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

public:

   Tabber( int tabWidth ) : d_tabWidth(tabWidth) {}
   int  FillCountToNextTabStop ( int col ) const { return d_tabWidth - (col % d_tabWidth) ; }
   int  ColOfNextTabStop       ( int col ) const { return col + FillCountToNextTabStop( col ); }
   int  ColOfPrevTabStop       ( int col ) const { return col - ((d_tabWidth == 0) ? 0 : 1 + ((col - 1) % d_tabWidth)); }
   bool ColAtTabStop           ( int col ) const { return (col % d_tabWidth) == 0; }
   };


void swidTabwidth( PChar dest, size_t sizeofDest, void *src ) {
   safeStrcpy( dest, sizeofDest, "status ln's \"e?\"" );
   }

PCChar swixTabwidth( stref param ) {
   const auto val( StrToInt_variable_base( param, 10 ) );
   const auto inRange( val >= 1 && val <= MAX_TAB_WIDTH );
   if( inRange ) {
      g_iTabWidth = val;
      DispNeedsRedrawAllLinesAllWindows();
      }
   return inRange ? nullptr : "tabwidth: Value must be between 1 and 8";
   }

void FBUF::SetTabWidthOk( COL NewTabWidth ) {
   const auto inRange( NewTabWidth >= 1 && NewTabWidth <= MAX_TAB_WIDTH );
   if( inRange )
      d_TabWidth = NewTabWidth;
   }

bool FBUF::SetTabconvOk( int newTabconv ) {
   const auto inRange( newTabconv >= TABCONV_0_NO_CONV && newTabconv < MAX_TABCONV_INVALID );
   if( inRange ) {
      d_Tabconv = eSpc2TabConvs(newTabconv);
      Msg( "tabconv set to %d", d_Tabconv );
      }

   return inRange;
   }

void swidTabconv( PChar dest, size_t sizeofDest, void *src ) {
   safeSprintf( dest, sizeofDest, "%d", g_CurFBuf()->TabConv() );
   }

PCChar swixTabconv( stref param ) {
   const auto setOk( g_CurFBuf()->SetTabconvOk( StrToInt_variable_base( param, 10 ) ) );
   return setOk ? nullptr : SwiErrBuf.Sprintf( "invalid tabconv value '%" PR_BSR "'", BSR(param) );
   }

COL TabAlignedCol( COL tabWidth, PCChar pS, PCChar eos, COL xCol, COL xBias ) {
   return ColOfPtr( tabWidth, pS, PtrOfColAnyWhere( tabWidth, pS, eos, xCol ) + xBias, eos );
   }

STATIC_FXN COL TabAlignedCol( COL tabWidth, PCChar pS, COL xColTgt ) {
   Assert( !(xColTgt < 0) );
   if( !g_fRealtabs || xColTgt < 0 )
      return xColTgt;

   const Tabber tabr( tabWidth );
   COL xCol( 0 );
   while( const auto ch = *pS++ ) {
      const auto xPrev( xCol );
      xCol = (ch == HTAB) ? tabr.ColOfNextTabStop( xCol ) : xCol+1;
      if( xCol > xColTgt )
         return xPrev;
      }

   return xColTgt;
   }

STATIC_FXN bool spacesonly( stref::const_iterator ptr, stref::const_iterator eos ) {
   for( ; ptr != eos; ++ptr ) {
      if( *ptr != ' ' )
         return false;
      }
   return true;
   }


#if defined(_WIN32)
   #define XLAT_chFill( chFill ) \
                    if( chFill == BIG_BULLET && USING_MS_OEM_CHARSET ) { \
                        chFill = SMALL_BULLET;                           \
                        }
#else
   #define XLAT_chFill( chFill )
#endif

// intent (20141221) is that FormatExpandedSeg replace PrettifyStrcpy and PrettifyMemcpy
// what's preventing this from happening?
// 1) dest receives a terminating NUL merely by dint of being a std::string
// 2) Xbuf offers a writable-string (PChar) interface to the underlying allocated buffer
//    (while std::string does NOT)
// 3) PrettifyMemcpy is called multiple times on the same buffer, to generate a console
//    display line

/*
   I'm working toward addressing these difficulties


*/

void PrettifyAppend( std::string &dest, stref src, COL xStart, size_t maxChars, COL tabWidth, char chTabExpand, char chTrailSpcs ) {
   // NB: we DO NOT clear dest!!!
   const auto initial_dest_length( dest.length() );
   auto dit( back_inserter(dest) );
   if( !chTabExpand || !StrContainsTabs( src ) ) {
      if( xStart <= src.length() ) {
         src.remove_prefix( xStart );
         const auto CopyBytes( Min( src.length(), maxChars ) );
         auto sit( src.cbegin() );
         for( auto ix( 0u ) ; ix < CopyBytes ; ++ix ) {
            *dit++ = *sit++;
            }
         if( chTrailSpcs && CopyBytes==src.length() ) {
            const auto drend( dest.rbegin() + (initial_dest_length - dest.length()) + 1 );
            for( auto drit( dest.rbegin() ) ; drit != drend && *drit == ' ' ; ++drit ) {
               *drit == chTrailSpcs;
               }
            }
         }
      return;
      }

   // the only way to solve the problem of "what happens if xStart is in the
   // middle of a tab-expansion?" is to walk the src string from its beginning,
   // even though we aren't necessarily _copying_ from the beginning.

   COL xCol( 0 );
   auto WR_CHAR = [&]( char ch ) { if( xCol++ >= xStart ) { *dit++ = ch; } };
   const Tabber tabr( tabWidth );
   auto sit( src.cbegin() );
   while( sit != src.cend() && dest.length() < maxChars ) {
      const auto ch( *sit++ );
      if( ch != HTAB ) {
         WR_CHAR(ch);
         }
      else {
         const auto tgt( tabr.ColOfNextTabStop( xCol ) );
         auto chFill( chTabExpand );                       // chTabExpand == BIG_BULLET has special behavior:
         while( xCol < tgt && dest.length() < maxChars ) { // col containing actual HTAB will disp as BIG_BULLET
            WR_CHAR(chFill);                               // remaining fill-in chars will show as SMALL_BULLET
            XLAT_chFill( chFill )
            }
         }
      }

   if( chTrailSpcs ) {
      // sit points just after the last source-char copied/xlated;
      //    sit == src.cend() (if the above loop terminated because 'sit == src.cend()')
      // OR sit != src.cend() (if the above loop terminated due to 'dest.length() < maxChars' being false)
      if( sit == src.cend() || spacesonly( sit, src.cend() ) ) { // _trailing_ spaces on the source side
         const auto drend( dest.rbegin() + (initial_dest_length - dest.length()) + 1 );
         for( auto drit( dest.rbegin() ) ; drit != drend && *drit == ' ' ; ++drit ) { // xlat all trailing spaces present in dest
            *drit == chTrailSpcs;
            }
         }
      }
   }

void FormatExpandedSeg // more efficient version: recycles (but clear()s) dest, should hit the heap less frequently
   ( std::string &dest, stref src
   , COL xStart, size_t maxChars, COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   dest.clear();
   PrettifyAppend( dest, src, xStart, maxChars, tabWidth, chTabExpand, chTrailSpcs );
   }

std::string FormatExpandedSeg // less efficient version: uses virgin dest each call, thus hits the heap each time
   ( stref src
   , COL xStart, size_t maxChars, COL tabWidth, char chTabExpand, char chTrailSpcs
   ) {
   std::string dest;
   PrettifyAppend( dest, src, xStart, maxChars, tabWidth, chTabExpand, chTrailSpcs );
   return dest;
   }

// a terminating NUL IS NOT added!!!  Call PrettifyStrcpy if you need a terminating NUL.
// return value is # of chars actually copied into pDestBuf
COL PrettifyMemcpy
   ( const PChar pDestBuf, const size_t sizeof_dest
   , stref src
   , COL tabWidth, char chTabExpand, COL xStart, char chTrailSpcs
   ) {
   // src.data() IS NOT NUL terminated (since it can be a pointer into a file image buffer)!!!
   //
   if( !chTabExpand || !StrContainsTabs( src ) ) {
      if( xStart > src.length() ) { return 0; }
      src.remove_prefix( xStart );

      const auto CopyBytes( Min( src.length(), sizeof_dest ) );
      memcpy( pDestBuf, src.data(), CopyBytes );

      if( chTrailSpcs && CopyBytes==src.length() ) {
         for( auto pC(pDestBuf + CopyBytes - 1) ; *pC == ' ' ; --pC ) {
            *pC = chTrailSpcs;
            }
         }

      Assert( CopyBytes <= sizeof_dest );
      return CopyBytes;
      }

   // the only way to solve the problem of "what happens if xStart is in the
   // middle of a tab-expansion?" is to walk the src string from its beginning,
   // even though we aren't necessarily _copying_ from the beginning.

   const Tabber tabr( tabWidth );
   const auto pDestRightmostWritable( pDestBuf + sizeof_dest - 1 );
         auto pD(pDestBuf);
#define  PD_EFF   (pD - xStart)
   COL xCol( 0 );
   auto WR_CHAR = [&]( char ch ) {
      if( PD_EFF >= pDestBuf ) { *PD_EFF = ch; }
      ++pD; ++xCol;
      };

   auto it( src.cbegin() );
   while( it != src.cend() && PD_EFF <= pDestRightmostWritable ) {
      const auto ch( *it++ );
      if( ch != HTAB ) {
         WR_CHAR(ch);
         }
      else {
         const auto limit( pD + tabr.FillCountToNextTabStop( xCol ) );
         auto chFill( chTabExpand );                                 // chTabExpand == BIG_BULLET has special behavior:
         while( pD < limit && PD_EFF <= pDestRightmostWritable ) {   // col containing actual HTAB will disp as BIG_BULLET
            WR_CHAR(chFill);                                         // remaining fill-in chars will show as SMALL_BULLET
            XLAT_chFill( chFill )
            }
         }
      }

   const auto copyBytes(PD_EFF - pDestBuf);
   if( copyBytes > 0 ) {
      if( chTrailSpcs ) {
         // it points just after the last source-char copied/xlated;
         //    it == src.cend() (if the above loop terminated because it == src.cend())
         // OR it  < src.cend() (if the above loop terminated due to PD_EFF <= pDestRightmostWritable being false)
         if( it == src.cend() || spacesonly( it, src.cend() ) ) { // _trailing_ spaces on the source side
            for( --pD ; PD_EFF >= pDestBuf && *PD_EFF == ' ' ; --pD ) { // xlat all trailing spaces present in dest
               *PD_EFF = chTrailSpcs;
               }
            }
         }
      return copyBytes;
      }
   return 0;
   }
#undef  PD_EFF

// a terminating NUL IS appended!!!  Use PrettifyMemcpy if you DON'T want one.
// return value is # of chars actually copied into dest - 1; i.e. it does NOT count the terminating NUL
COL PrettifyStrcpy( const PChar dest, size_t sizeof_dest, stref src, COL tabWidth, char chTabExpand, COL xStart, char chTrailSpcs ) {
   const auto chars( PrettifyMemcpy( dest, sizeof_dest-1, src, tabWidth, chTabExpand, xStart, chTrailSpcs ) );
   dest[chars] = '\0';
   return chars;
   }

COL ColPrevTabstop( COL tabWidth, COL xCol ) { return Tabber( tabWidth ).ColOfPrevTabStop( xCol ); }
COL ColNextTabstop( COL tabWidth, COL xCol ) { return Tabber( tabWidth ).ColOfNextTabStop( xCol ); }

COL StrCols( COL tabWidth, const stref &src ) {
   const Tabber tabr( tabWidth );
   auto col( 0 );
   for( auto it( src.cbegin() ) ; it != src.cend() ; ++it ) {
      col = (*it == HTAB) ? tabr.ColOfNextTabStop( col ) : col + 1;
      }
   return col;
   }

COL FBOP::LineCols( PCFBUF fb, LINE yLine ) {
   return StrCols( fb->TabWidth(), fb->PeekRawLine( yLine ) );
   }

bool FBOP::IsLineBlank( PCFBUF fb, LINE yLine ) {
   return IsStringBlank( fb->PeekRawLine( yLine ) );
   }

bool FBOP::IsBlank( PCFBUF fb ) {
   for( auto iy( 0 ); iy < fb->LineCount() ; ++iy )
      if( !FBOP::IsLineBlank( fb, iy ) )
         return false;

   return true;
   }

//      const Tabber &TabberParam;
typedef const Tabber  TabberParam;  // 3 calls using this type take less code (-512 byte GCC incr)

STATIC_FXN void spcs2tabs_outside_quotes( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto quoteCh( '\0' );
   auto destCol( 0 );
   auto fNxtChEscaped( false );
   auto fInQuotedRgn(  false );
   auto sit( src.crbegin() );
   while( sit != src.crend() ) {
      if( !fInQuotedRgn ) {
         if( !fNxtChEscaped ) {
            auto x_Cx( 0 );
            while( sit != src.crend() && (*sit == ' ' || *sit == HTAB) ) {
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

         if( sit != src.crend() && !fNxtChEscaped ) {
            if( fInQuotedRgn )
               goto TO_ELSE;

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
         if( sit != src.crend() && !fNxtChEscaped ) {
TO_ELSE:
            if( *sit == quoteCh )
               fInQuotedRgn = false;
            else if( *sit == chESC ) // ESCAPE char, not PathSepCh!
               fNxtChEscaped = true;
            }
         else {
            fNxtChEscaped = false;
            }
         }

      if( sit != src.crend() ) {
         *dit++ = *sit++;
         ++destCol;
         }
      }
   }

STATIC_FXN void spcs2tabs_all( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto xCol(0);
   auto sit( src.crbegin() );
   while( sit != src.crend() ) {
      auto ix(0);
      while( sit != src.crend() && (*sit == ' ' || *sit == HTAB) ) {
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

      if( sit != src.crend() ) {
         *dit++ = *sit++;
         ++xCol;
         }
      }
   }

STATIC_FXN void spcs2tabs_leading( string_back_inserter dit, stref src, TabberParam tabr ) {
   auto xCol( 0 );
   auto ix(0);
   auto sit( src.crbegin() );
   for( ; sit != src.crend() && (*sit == ' ' || *sit == HTAB) ; ++sit ) {
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

   for( ; sit != src.crend() ; ++sit ) {
      *dit++ = *sit;
      }
   }


//*******************************  END TABS *******************************
//*******************************  END TABS *******************************
//*******************************  END TABS *******************************

//==========================================================================================

void FBUF::cat( PCChar pszNewLineData ) {
   auto pBuf( pszNewLineData );
   std::string tmp;
   Xbuf xb;
   while( 1 ) {
      decltype(pBuf) pNL( strchr( pBuf, '\n' ) );
      if( pNL == pszNewLineData ) {  // leading \n?
         pBuf = pNL + 1;
         pNL = strchr( pBuf, '\n' );
         }
      if( pBuf == pszNewLineData ) {
         getLineTabx_DEPR( &xb, LastLine() );
         PutLine( LastLine(), xb.cat( pBuf, pNL-pBuf ), tmp );
         }
      else {
         PutLine( LineCount(), se2bsr( pBuf, pNL ), tmp );
         }
      if( !pNL ) break;
      pBuf = pNL + 1;
      }
   }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

STATIC_FXN int StrlenWOTrailBlanks( PCChar pszString, PCChar eos=nullptr ) {
   if( !eos ) eos = Eos(pszString);
   const decltype(pszString) pPrevNonBlank( StrPastPrevBlankOrNull( pszString, eos ) );
   return pPrevNonBlank ? pPrevNonBlank - pszString + 1 : 0;
   }

int StrTruncTrailBlanks( PChar pszString ) {
   const auto len( StrlenWOTrailBlanks( pszString ) );
   pszString[ len ] = '\0';
   return len;
   }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int FBUF::PutLastMultiline( PCChar buf ) {
   const auto pEos( buf + Strlen( buf ) );
   std::string tmp;
   auto pX( buf );
   auto pSegStart( buf );
   auto lineCount( 0 );
   while( pX < pEos ) {
      // EXPECTED ("Windows Standard") EOL sequence is "\x0D\x0A".
      // in some cases (Clearcase diff GUI), UNIX newlines ('\x0A', I'm
      // guessing) ALONE are used as line terminators.  This causes
      // havoc, since existing code here never makes a line-break, and
      // the '\x0A' chars are literally shown.  28-Jan-2004 klg
      //
      PCChar pSegEnd( nullptr ), pNxtSegStart( nullptr );
      switch( pX[0] ) {
         case '\x0D': pSegEnd = pX;  pNxtSegStart = pX + 1 + (pX[1] == '\x0A' ? 1 : 0);  break; // normal (Windows) case
         case '\x0A': pSegEnd = pX;  pNxtSegStart = pX + 1 + (pX[1] == '\x0D' ? 1 : 0);  break;
         }
      if( pSegEnd ) {
         PutLine( LineCount(), se2bsr( pSegStart, pSegEnd ), tmp );
         ++lineCount;
         pX = pSegStart = pNxtSegStart;
         }
      else
         ++pX;
      }

   if( pSegStart < pEos ) {
      PutLine( LineCount(), se2bsr( pSegStart, pEos ), tmp );
      ++lineCount;
      }

   return lineCount;
   }


void FBUF::xvsprintf( PXbuf pxb, LINE lineNum, PCChar format, va_list val ) {
   std::string tmp;
   pxb->vFmtStr( format, val );
   auto pBuf( pxb->wbuf() );
   for(;;) {
      const auto pNL( Strchr( pBuf, '\n' ) );
      if(  pNL ) *pNL = '\0';
      InsLine( lineNum++, pBuf, tmp );
      if( !pNL ) break;
      pBuf = pNL + 1;
      }
   }

void FBUF::Vsprintf( LINE lineNum, PCChar format, va_list val ) {
   std::string tmp;
   Xbuf xb;
   xb.vFmtStr( format, val );
   auto pBuf( xb.wbuf() );
   for(;;) {
      const auto pNL( Strchr( pBuf, '\n' ) );
      if(  pNL ) *pNL = '\0';
      InsLine( lineNum++, pBuf, tmp );
      if( !pNL ) break;
      pBuf = pNL + 1;
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

void FBUF::PutLine( LINE yLine, stref srSrc, std::string &stbuf ) {
   // if( IsNoEdit() ) { DBG( "%s on noedit=%s", __PRETTY_FUNCTION__, Name() ); }
   BadParamIf( , IsNoEdit() );
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

   if( TABCONV_0_NO_CONV != d_Tabconv ) {
      stbuf.clear();
      const Tabber tabr( this->TabWidth() );
      switch( d_Tabconv ) { // compress spaces into tabs per this->d_Tabconv:
         default:
         case TABCONV_0_NO_CONV:                   Assert( 0 ); break;
         case TABCONV_1_LEADING_SPCS_TO_TABS:      spcs2tabs_leading       ( back_inserter(stbuf), srSrc, tabr ); break;
         case TABCONV_2_SPCS_NOTIN_QUOTES_TO_TABS: spcs2tabs_outside_quotes( back_inserter(stbuf), srSrc, tabr ); break;
         case TABCONV_3_ALL_SPC_TO_TABS:           spcs2tabs_all           ( back_inserter(stbuf), srSrc, tabr ); break;
         }
      srSrc = stbuf;
      }
   if( !TrailSpcsKept() ) {
      auto trailSpcs = 0;
      for( auto it( srSrc.crbegin() ) ; it != srSrc.crend() && *it == ' ' ; ++it ) {
         ++trailSpcs;
         }
      srSrc.remove_suffix( trailSpcs );
      }
   UndoReplaceLineContent( yLine, srSrc );  // after all this buildup, JUST WRITE THE DAMNED THING:
   }

void FBUF::PutLine( LINE yLine, CPCChar pa[], int elems ) {
   Xbuf xb; std::string tmp;
   for( auto ix(0); ix<elems; ++ix ) {
      xb.cat( pa[ix] );
      }
   PutLine( yLine, xb.bsr(), tmp );
   }

//
// find COL of pointer (pWithinLineBuf) within (or past the end of) a sz string
// (pLineBuf).  If pString contains HTABs, returned value will never be the
// column of a tab fill character (since this doesn't exist in pString).
//
COL ColOfPtr( COL tabWidth, const PCChar pString, const PCChar pWithinString, PCChar eos ) {
   if( pWithinString < pString )
      return -1;

   if( !eos ) { eos = Eos( pString ); }
   const Tabber tabr( tabWidth );
   auto xCol( 0 );
   auto pX( pString );
   while( pX < eos ) {
      if( pX >= pWithinString ) { // we have found or gone past pWithinString
         return xCol;
         }
      switch( *pX++ ) {
         default  : ++xCol;                                break;
         case HTAB: xCol = tabr.ColOfNextTabStop( xCol );  break;
         }
      }
   return xCol + (pWithinString - eos);  // 'pWithinString' actually points _at or beyond_ eos: assume all chars past EOL are spaces (non-tabs)
   }

COL ColOfFreeIdx( COL tabWidth, const stref &content, sridx offset ) {
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

//------------------------------------------------------------------------------
//
// DeletePrevChar implements ARG::emacscdel _AND_ ARG::cdelete (below)
//

STATIC_FXN bool DeletePrevChar( const bool fEmacsmode ) { PCFV;
   const auto yLine( pcv->Cursor().lin );
   if( pcv->Cursor().col == 0 ) { // cursor @ beginning of line?
      if( yLine == 0 )
         return false; // no prev char

      auto xCol( FBOP::LineCols( pcf, yLine-1 ) );
      if( fEmacsmode ) // join current and prev lines
         pcf->DelStream( xCol, yLine-1, 0, yLine );

      pcv->MoveCursor( yLine-1, xCol );
      return true;
      }

   const auto xCol( pcv->Cursor().col - 1 );
   FBOP::DelChar( pcf, xCol, yLine );
   pcv->MoveCursor( yLine, xCol );
   return true;
   }

bool ARG::cdelete  () { return DeletePrevChar( false ); }
bool ARG::emacscdel() { return DeletePrevChar( true  ); }

//
//------------------------------------------------------------------------------

STATIC_FXN void GetLineWithSegRemoved( PFBUF pf, std::string &dest, const LINE yLine, const COL xLeft, const COL boxWidth, bool fCollapse ) {
   pf->getLineTabxPerRealtabs( dest, yLine );
   const auto xEolNul( dest.length() );
   if( xEolNul <= xLeft )
      return;

// const auto destbuf( dest.data() );
// const auto eos( dest.data()+dest.length() );
   const auto tw( pf->TabWidth() );
   const auto ixLeft( CaptiveIdxOfCol( tw, dest, xLeft ) );
// const auto pxLeft( PtrOfColWithinStringRegion( tw, destbuf, eos, xLeft ) );

   const auto xRight( xLeft + boxWidth );
   if( xRight >= xEolNul ) { // trailing segment of line is being deleted?
      0 && DBG( "%s trim, %" PR_SIZET "u <= %d '%c'", __func__, xEolNul, xRight, dest[ixLeft] );
      dest.resize( ixLeft ); // the first (leftmost) char in the selected box
      return;
      }

   if( boxWidth == 0 )
      return;

// const auto charsInGap( GreaterOf( static_cast<ptrdiff_t>(1), PtrOfColWithinStringRegion( tw, destbuf, eos, xRight ) - pxLeft ) );
   const auto charsInGap( boxWidth );
   if( fCollapse ) { // Collapse
//    const auto pxEol(                                         PtrOfColWithinStringRegion( tw, destbuf, eos, xEolNul ) ); // *pxEol === '\0'
//    memmove( pxLeft, pxLeft+charsInGap, pxEol - pxLeft - charsInGap + 1 );
      dest.erase( ixLeft, charsInGap );
      }
   else { // fill Gap w/blanks
      dest.replace( ixLeft, charsInGap, charsInGap, ' ' );
      }
   }

void FBUF::DelBox( COL xLeft, LINE yTop, COL xRight, LINE yBottom, bool fCollapse ) {
   if( xRight < xLeft )
      return;

   AdjMarksForBoxDeletion( this, xLeft, yTop, xRight, yBottom );
   const auto boxWidth( xRight - xLeft + 1 );
   std::string src; std::string stmp;
   for( auto yLine( yTop ); yLine <= yBottom; ++yLine ) {
      GetLineWithSegRemoved( this, src, yLine, xLeft, boxWidth, fCollapse );
      PutLine( yLine, src, stmp );
      }
   }

//
// NB: See "STREAM definition" in ARG::FillArgStruct to understand parameters!
// Nutshell: Last char of stream IS NOT INCLUDED in operation!
//
void FBUF::DelStream( COL xStart, LINE yStart, COL xEnd, LINE yEnd ) {
   if( yStart == yEnd ) { // xEnd-1 because "Last char of stream IS NOT INCLUDED in operation!"
      0 && DBG( "%s [%d..%d]", __func__, xStart, xEnd-1 );
      DelBox( xStart, yStart, xEnd-1, yStart );
      return;
      }
   std::string stFirst; GetLineSeg( stFirst, yStart, 0, xStart-1 );
   DelLines( yStart, yEnd - 1 );
   std::string stLast;  GetLineSeg( stLast, yStart, xEnd, COL_MAX );
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
   if( cinfo.pFBuf )
      *pClipboardArgType = cinfo.contentType;

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
         return FBOP::FindOrAddFBuf( FmtStr<12>( "<clip%d>", s_Clip.curIdx ), &cinfo.pFBuf );
         }
      }
   // _ALL_ aClipFBufs have been made readonly!  User has to pick one to overwrite, OR cancel the copy-to-clip op

   // MenuChooseClip( "Choose <clip> to overwrite" );
   const auto menuChoice( -1 );
   if( menuChoice < 0 )
      return nullptr;

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
   if( copyToClipboard ) PCFV_Copy_STREAMARG_ToClipboard( d_streamarg );
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
   if( copyToClipboard ) PCFV_Copy_LINEARG_ToClipboard( d_linearg );
   pcf->DelLines( d_linearg.yMin, d_linearg.yMax );
   pcv->MoveCursor( d_linearg.yMin, g_CursorCol() );
   }

void PCFV_delete_BOXARG( ARG::BOXARG_t const &d_boxarg, bool copyToClipboard, bool fCollapse ) { PCFV;
   if( copyToClipboard ) PCFV_Copy_BOXARG_ToClipboard( d_boxarg );
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
    case NOARG:     FBOP::DelChar( pcf, pcv->Cursor() ); break; // Delete the CHARACTER at the cursor w/o saving it to <clipboard>
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
   if( d_argType == STREAMARG )
      ConvertStreamargToLineargOrBoxarg();

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
   if( d_argType == STREAMARG )
      ConvertStreamargToLineargOrBoxarg();

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
                       pcf->GetLineForInsert( sbuf, d_nullarg.cursor.lin, xNonb, d_nullarg.cursor.col - xNonb );
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

void FBOP::PutChar_( PFBUF fb, LINE yLine, COL xCol, char theChar, bool fInsert, std::string &tmp1, std::string &tmp2 ) {
   fb->GetLineForInsert( tmp1, yLine, xCol, fInsert ? 1 : 0 );        0 && DBG( "%s=%" PR_BSR "'", __func__, BSR(tmp1) );
   tmp1[ CaptiveIdxOfCol( fb->TabWidth(), tmp1, xCol ) ] = theChar;   0 && DBG( "%s=%" PR_BSR "'", __func__, BSR(tmp1) );
   if( fInsert ) {
      AdjMarksForInsertion( fb, fb, xCol, yLine, COL_MAX, yLine, xCol+1, yLine );
      }
   fb->PutLine( yLine, tmp1, tmp2 );
   }


#ifdef fn_xquote

STATIC_FXN PCCMD GetGraphic() {
   CPCCMD pCmd( CmdFromKbdForExec() );
   if( !pCmd || !pCmd->IsFnGraphic() )
      return nullptr;

   return pCmd;
   }

STATIC_FXN int GetHexDigit() {
   PCCMD pCmd;
   while( !(pCmd=GetGraphic()) || !isxdigit( pCmd->d_argData.chAscii() ) )
      continue;

   const char ch( toLower( pCmd->d_argData.chAscii() ) );
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

STATIC_FXN COL colPastPrevBlanks( PCChar ptr, int lineChars, COL startCol ) {
   NoGreaterThan( &startCol, lineChars ); // lineChars does ! include EOL
   const decltype(ptr) pC( StrPastPrevBlankOrNull( ptr, ptr + startCol ) );
   return pC ? pC - ptr : startCol;
   }

bool ARG::graphic() {
   const char usrChar( d_pCmd->d_argData.chAscii() );
   if( d_argType == BOXARG ) {
      if( usrChar == ' ' ) { // insert spaces
         linsert();
         return true;
         }

      // <000612> klg Finally did this!  Been needing it for YEARS!
      //
      // g_delims, g_delimMirrors
      //
      //                                                m4 `quoting'
      //                                                |
      STATIC_CONST char chOpeningDelim[]    = "_%'\"(<{[`";
      STATIC_CONST char chClosingDelim[]    = "_%'\")>}]`";
      STATIC_CONST char chClosingDelim_m4[] = "_%'\")>}]'";
             const auto pMatch( strchr( chOpeningDelim, usrChar ) );
      if( pMatch ) {
         const char chClosing( (g_fM4backtickquote?chClosingDelim_m4:chClosingDelim)[ pMatch - chOpeningDelim ] );
         // if certain chars are hit when a BOX selection is current, surround the
         // selected text with matching delimiters (depending on the char hit)
         //
         const auto pf( g_CurFBuf() );
         std::string tmp1, tmp2;
         for( auto curLine( d_boxarg.flMin.lin ); curLine <= d_boxarg.flMax.lin ; ++curLine ) {
            const auto chars( pf->getLineTabx( tmp1, curLine ) );
            const auto xRight(
                 (d_cArg > 1 || (usrChar == chQuot2 || usrChar == chQuot1 || usrChar == chBackTick))  // word-conforming bracketing of a BOXARG?
                 ? colPastPrevBlanks( tmp1.c_str(), tmp1.length(), d_boxarg.flMax.col+1 )
                 : d_boxarg.flMax.col
               );
            FBOP::InsertChar( pf, curLine, xRight+1          , chClosing, tmp1, tmp2 );
            FBOP::InsertChar( pf, curLine, d_boxarg.flMin.col, usrChar  , tmp1, tmp2 );
            }
         return true;
         }
      else if( 0 && (',' == usrChar) ) {
         // TBD: loop looking for spacey regions, replacing first char of each spacey region with a comma
         }
      }

   DelArgRegion();
   std::string tmp1, tmp2;
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
   if( ArgCount() != 0 )
      return newline();

   const auto pfb( g_CurFBuf() );
   const auto xIndent( FBOP::GetSoftcrIndent( pfb ) );

   // Original bug report 20070305:
   //
   // "emacsnewl "touches" the current line even when the cursor is beyond
   // EOL and thus the content of said line is unchanged (this causes
   // tab-replacement changes).  Investigation shows that maybe
   // CopyStream should not be used, or that GetLineForInsert needs to be
   // modified to use the tabconv settings from the dest?"
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
                       std::string stmp;
                       g_pFbufClipboard->PutLine( 0, d_textarg.pText, stmp );
                       g_ClipboardType = BOXARG;
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
                          SafeStrcpy( tmpfilenamebuf, tmpx.c_str() );
                          0 && DBG( "tmp '%s'", tmpfilenamebuf );
                          pSrcFnm = tmpfilenamebuf;
                          cmdstrbuf.Sprintf( "%s >\"%s\" 2>&1", pszCmd, tmpfilenamebuf );
                          RunChildSpawnOrSystem( cmdstrbuf );
                          }

                       if( FBUF::FnmIsPseudo( pSrcFnm ) )
                          cmdstrbuf.Strcpy( pSrcFnm );
                       else
                          CompletelyExpandFName_wEnvVars( BSOB(cmdstrbuf), pSrcFnm );

                       const auto pFBuf( FindFBufByName( cmdstrbuf ) );
                       if( pFBuf ) {
                          if( pFBuf->RefreshFailedShowError() )
                             return false;

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
                    if( boxWidth == 0 )  return false;
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
   if( pcf->CantModify() )
      return false;

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
         pcf->GetLineForInsert( tmp1, yLine, xCol, 0 );
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
   FBOP::InsertChar( pcf, yLine, xCol, theChar, tmp1, tmp2 );
   noargNoMeta.right();
   return true;
   }

//
// pszString could be PCChar, but then we'd run into
// PChar-input-becomes-PCChar-output problems (which in other cases have been
// resolved using templates)
//


// pEos points AFTER last valid char in pS; if pS were a standard C string, *pEos == 0, BUT pS MAY NOT BE a standard C string!
// if fKeepPtrWithinStringRegion then retval <= pEos
PChar PtrOfCol_( COL tabWidth, const PChar pS, const PChar pEos, const COL colTgt, const bool fKeepPtrWithinStringRegion ) {
   if( colTgt == 0 )
      return pS;

   if( colTgt < 0 )
      return pS - 1;

#if 1 // ==0 to test the "realtabs:yes" ... code below
   const COL colEos( pEos - pS );
   if( !( /* g_fRealtabs && */ StrContainsTabs( pS, colEos )) ) { // this is the most common exit path
      return pS +
         ((fKeepPtrWithinStringRegion && colTgt > colEos)
         ? colEos
         : colTgt
         );
      }
#endif

   // "realtabs:yes" AND there's an HTAB in the string
   //
   const Tabber tabr( tabWidth );
   auto colPrevTabStop( 0 );
   auto pPastPrevTab( pS );
   while( 1 ) {
      const auto pTab( StrNxtTabOrNull( pPastPrevTab, pEos ) );
      if( !pTab ) { // no more tabs ?
         if( colTgt <= (colPrevTabStop + (pEos - pPastPrevTab)) || !fKeepPtrWithinStringRegion )
            return pPastPrevTab + (colTgt - colPrevTabStop);
         else
            return pEos;
         }

      // have a tab:
      colPrevTabStop += pTab - pPastPrevTab;
      if( colTgt < colPrevTabStop )
         return pTab - (colPrevTabStop - colTgt);

      const auto colNextTabStop( tabr.ColOfNextTabStop( colPrevTabStop ) );
      if( colTgt >= colPrevTabStop && colTgt < colNextTabStop )
         return pTab;

      colPrevTabStop = colNextTabStop;
      pPastPrevTab = pTab + 1;
      }
   }


// pEos points AFTER last valid char in pS; if pS were a standard C string, *pEos == 0, BUT pS MAY NOT BE a standard C string!
// retval < pEos
sridx FreeIdxOfCol( const COL tabWidth, const stref &content, const COL colTgt ) {
   if( colTgt <= 0 ) { return 0; }

#if 1 // ==0 to test the "realtabs:yes" ... code below
   if( tabWidth <= 1 || !( /* g_fRealtabs && */ StrContainsTabs( content )) ) { // this is the most common exit path
      return colTgt;
      }
#endif

   // "realtabs:yes" AND there's an HTAB in the string
   //
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


// pEos points AFTER last valid char in pS; if pS were a standard C string, *pEos == 0, BUT pS MAY NOT BE a standard C string!
// retval < pEos
PChar PtrOfColWithinStringRegionNoEos( COL tabWidth, const PChar pS, const PChar pEos, const COL colTgt ) {
   if( colTgt == 0 )
      return pS;

   if( colTgt < 0 )
      return pS - 1;

#if 1 // ==0 to test the "realtabs:yes" ... code below
   const COL colEos( pEos - pS );
   if( !( /* g_fRealtabs && */ StrContainsTabs( pS, colEos )) ) { // this is the most common exit path
      return pS +
         ((colTgt >= colEos)
         ? colEos-1
         : colTgt
         );
      }
#endif

   // "realtabs:yes" AND there's an HTAB in the string
   //
   const Tabber tabr( tabWidth );
   auto colPrevTabStop( 0 );
   auto pPastPrevTab( pS );
   while( 1 ) {
      const auto pTab( StrNxtTabOrNull( pPastPrevTab, pEos ) );
      if( !pTab ) { // no more tabs ?
         if( colTgt < (colPrevTabStop + (pEos - pPastPrevTab)) )
            return pPastPrevTab + (colTgt - colPrevTabStop);
         else
            return pEos-1; // point AT last char in string
         }

      // have a tab:
      colPrevTabStop += pTab - pPastPrevTab;
      if( colTgt < colPrevTabStop )
         return pTab - (colPrevTabStop - colTgt);

      const auto colNextTabStop( tabr.ColOfNextTabStop( colPrevTabStop ) );
      if( colTgt >= colPrevTabStop && colTgt < colNextTabStop )
         return pTab;

      colPrevTabStop = colNextTabStop;
      pPastPrevTab = pTab + 1;
      }
   }

//--------------------------------------------------------------------------------------------------

bool FBUF::PeekRawLineExists( LINE lineNum, PPCChar ppLbuf, size_t *pChars ) const {
   const auto exists( KnownLine( lineNum ) );
   if(   exists
      && LineLength(lineNum) > 0
      && (*ppLbuf = d_paLineInfo[ lineNum ].GetLineRdOnly())
     ) {
      *pChars = LineLength(lineNum);
      }
   else {
      *ppLbuf = "";
      *pChars = 0;
      }
   return exists;
   }

bool FBUF::PeekRawLineExists( LINE lineNum, PPCChar ppLbuf, PPCChar ppEos ) const {
   size_t chars;
   const auto rv( PeekRawLineExists( lineNum, ppLbuf, &chars ) );
   *ppEos = *ppLbuf + chars;
   return rv;
   }

stref FBUF::PeekRawLine( LINE lineNum ) const {
   const auto exists( KnownLine( lineNum ) );
   auto len( 0 );
   if( exists && (len=LineLength(lineNum)) > 0 ) {
      return stref( d_paLineInfo[ lineNum ].GetLineRdOnly(), len );
      }
   else {
      return stref( "", 0 );
      }
   }

stref FBUF::PeekRawLineSeg( LINE lineNum, COL xMinIncl, COL xMaxIncl ) const {
   auto rl( PeekRawLine( lineNum ) );
   const auto tw( TabWidth() );
   const auto ixMinIncl( FreeIdxOfCol( tw, rl, xMinIncl ) );
   if( ixMinIncl >= rl.length() ) return stref( "" );
   const auto ixMaxIncl( CaptiveIdxOfCol( tw, rl, xMaxIncl ) ); 0 && DBG( "%d[%d/%" PR_SIZET "u,%d/%" PR_SIZET "u]=%" PR_SIZET "u=%" PR_BSR "'", lineNum, xMinIncl, ixMinIncl, xMaxIncl, ixMaxIncl, rl.length(), BSR(rl) );
   rl.remove_suffix( rl.length() - ixMaxIncl - 1 );
   rl.remove_prefix( ixMinIncl );                               0 && DBG( "%d[%d/%" PR_SIZET "u,%d/%" PR_SIZET "u]=%" PR_SIZET "u=%" PR_BSR "'", lineNum, xMinIncl, ixMinIncl, xMaxIncl, ixMaxIncl, rl.length(), BSR(rl) );
   return rl;
   }


void FBUF::getLineRaw( std::string &dest, LINE yLine ) const {
   const auto rv( PeekRawLine( yLine ) );
   dest.assign( rv.data(), rv.length() );
   }

COL FBUF::getLine_( std::string &dest, LINE yLine, int chExpandTabs ) const {
   const auto rl( PeekRawLine( yLine ) );
   FormatExpandedSeg( dest, rl, 0, COL_MAX, TabWidth(), ' ', chExpandTabs );
   return dest.length();
   }

// returns strlen of returned line
COL FBUF::getLine_DEPR( PXbuf pXb, LINE yLine, int chExpandTabs ) const {
   const auto rv( PeekRawLine( yLine ) );
   const auto tw( TabWidth() );
   const auto size( 1+StrCols( tw, rv ) );
   const auto pDest( pXb->wresize( size ) );
   return PrettifyStrcpy( pDest, size, rv, tw, chExpandTabs );
   }


// gap: [xLeftIncl, xRightIncl]
// rules:
//    - if any chars exist in gap
//         then rv will be filled with these chars:
//            WITHOUT trailing-space padding being added
//            so if the line ends in the middle of the gap, rv.length() < gapChars
//    - if NO chars exist in gap
//         then rv.empty() == true
void FBUF::GetLineSeg( std::string &dest, LINE yLine, COL xLeftIncl, COL xRightIncl ) const {
   dest.clear();
   if( yLine >= 0 && yLine <= LastLine() ) {
      const auto rl( PeekRawLine( yLine ) );
      FormatExpandedSeg( dest, rl, xLeftIncl, xRightIncl - xLeftIncl + 1, TabWidth() );
      }
   }

// open a (space-filled) insertCols-wide hole, with dest[xIns] containing the first inserted space;
//    original dest[xIns] is moved to dest[xIns+insertCols]
// if insertCols == 0 && dest[xIns] is not filled by existing content, spaces will be added [..xIns); dest[xIns] = 0
//
int FBUF::GetLineForInsert( std::string &dest, const LINE yLine, COL xIns, COL insertCols ) const { enum { DB=0 };
   const auto tw       ( TabWidth() );
   auto       lineChars( getLineTabxPerRealtabs( dest, yLine ) );
   auto       strCols  ( StrCols( tw, dest.c_str() ) );
   const auto lineCols ( ColOfFreeIdx( tw, dest, dest.length() ) );
   DB && DBG( "%s: gLTPR |%" PR_BSR "| L %d/%d (%d)", __func__, BSR(dest), lineCols, strCols, xIns );
   // Assert( lineCols == lineChars );

   if( lineCols < xIns ) { // line shorter than caller requires? append spaces thru dest[xIns-1]; dest[xIns] == 0
      dest.append( xIns - lineCols, ' ' );
      }
   if( insertCols > 0 ) {
      const auto ix( FreeIdxOfCol( tw, dest, xIns ) );
      DB && DBG( "%s: gLTPR |%" PR_BSR "| L %d/%d (%d) [%" PR_SIZET "u]", __func__, BSR(dest), lineCols, strCols, xIns, ix );
      dest.insert( ix, insertCols, ' ' );
      }
   DB && DBG( "%s: gLTPR |%" PR_BSR "| L %" PR_SIZET "u (%d)", __func__, BSR(dest), dest.length(), xIns );
   return dest.length();
   }
/*
x
x
*/

// open a (space-filled) insertCols-wide hole, with dest[xIns] containing the first inserted space;
//    original dest[xIns] is moved to dest[xIns+insertCols]
// if insertCols == 0 && dest[xIns] is not filled by existing content, spaces will be added [..xIns); dest[xIns] = 0
//
int FBUF::GetLineForInsert( PXbuf pXb, const LINE yLine, COL xIns, COL insertCols ) const {
   auto       lineChars( getLineTabxPerRealtabs_DEPR( pXb, yLine ) );
   auto       dest     ( pXb->wbuf() );
   const auto tw       ( TabWidth() );
   auto       lineCols ( StrCols( tw, dest ) );
   0 && DBG( "%s: gLTPR |%s| L %" PR_SIZET "u/%d (%d)", __func__, dest, pXb->length(), lineCols, xIns );
   // Assert( lineCols == lineChars );

   if( lineCols < xIns ) { // line shorter than caller requires? append spaces thru dest[xIns-1]; dest[xIns] == 0
      const auto deltaCols( xIns - lineCols );
      dest = pXb->wresize( lineChars + deltaCols + 1 );
      memset( dest+lineChars, ' ', deltaCols );
      lineChars += deltaCols; // since appended chars are spaces ...
      lineCols  += deltaCols; // ... cols and chars are equivalent
      dest[ lineChars ] = '\0';  // note that: lineChars == xIns && dest[ xIns ] == 0
      }

   if( insertCols == 0 )
      return lineChars;

#if 1
   dest = pXb->wresize( lineChars + insertCols + 1 );
   auto pVX0( PtrOfColAnyWhere( tw, dest, Eos(dest), xIns ) );
   memmove( pVX0 + insertCols, pVX0, Strlen(pVX0)+1 );
   memset(  pVX0             , ' ' , insertCols );
#else
   const auto xIns_TA( TabAlignedCol( tw, dest, xIns ) );
   insertCols += xIns - xIns_TA;
   dest = pXb->wresize( lineChars + insertCols + 1 );
   auto pVX0( PtrOfColAnyWhere          ( tw, dest, xIns_TA ) );
   auto pVX1( PtrOfColWithinStringRegion( tw, dest, xIns_TA ) );
   memmove( pVX0 + insertCols, pVX1, Strlen(pVX1)+1 );
   memset( pVX1, ' ', insertCols + (pVX0 - pVX1) );
#endif
   return Strlen(dest); // there ought to be a faster way!
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

   const auto ruKeyBytes( ROUNDUP_TO_NEXT_POWER2( keyBytes, 16 ) );
   const auto sizeofLSR( sizeof( LineSortRec ) + ruKeyBytes );

   0 && DBG( "XXXXX  %d - > %d", keyBytes, ruKeyBytes );

   auto papLSR( PPLineSortRec( Alloc0d( lines * sizeof(PLineSortRec) ) ) );
   auto ppLSR ( papLSR );
   auto paLSR ( PLineSortRec( AllocNZ( lines * sizeofLSR ) ) );
   auto pLSR  ( paLSR );
   const auto yPastEnd( yMax + 1 );
   for( auto yY( yMin ) ; yY < yPastEnd ; ++yY ) {
      pLSR->yLine = yY;
      const auto chars( fb->getLineTabx( pLSR->lbuf, yY ) );
      if( chars > yMin ) {
         safeStrcpy( pLSR->keydata, keyBytes, pLSR->lbuf + xMin );
         if( !fCase )  _strlwr( pLSR->keydata );
         }
      else {
         pLSR->keydata[0] = '\0';
         }

      fb->getLineRaw( pLSR->lbuf, yY );
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

   if( yMaxEff <= yPastEnd )
      fb->DelLine( yMaxEff, yPastEnd );

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

void LineInfo::PutContent( PCChar pNewLine, int newLineBytes ) { // assume previous content has been destroyed!
   if( newLineBytes == 0 ) {
      d_pLineData = nullptr;
      }
   else {
      AllocBytesNZ( d_pLineData, newLineBytes, __func__ );
      memcpy( d_pLineData, pNewLine, newLineBytes );
      }
   d_iLineLen = newLineBytes;
   }

void FBUF::PutLineSeg( const LINE lineNum, const stref &ins, std::string &stmp, std::string &dest, const COL xLeftIncl, const COL xRightIncl, const bool fInsert ) {
   enum { DE=0 };                                       DE && DBG( "%s+ L %d [%d..%d] <= '%" PR_BSR "' )", __func__, lineNum, xLeftIncl, xRightIncl, BSR(ins) );
   // insert ins into gap = [xLeftIncl, xRightIncl]
   // rules:
   //    - portion of ins placed into line is NEVER longer than gap
   //    - if chars WILL exist to right of gap, i.e.
   //         if  fInsert AND existing chars at or to right of xLeftIncl
   //         OR
   //         if !fInsert AND existing chars       to right of xRightIncl
   //         then ins is space padded to fill gap and will NOT terminate string.
   //      else ins is NOT space padded, will terminate string, perhaps to left of xRightIncl
   if( !fInsert && xLeftIncl == 0 && xRightIncl >= FBOP::LineCols( this, lineNum ) ) { // a two-parameter call?
      DE && DBG( "%s- PutLine(simple) )", __func__ );
      PutLine( lineNum, ins, stmp ); // optimal/trivial line-replace case
      }
   else { // segment ins/overwrite case

#if 1
      const sridx holewidth( xRightIncl - xLeftIncl + 1 );
      const auto inslen( Min( ins.length(), holewidth ) );                     DE && DBG( "%s [%d L gap/inslen=%" PR_BSRSIZET "u/%" PR_BSRSIZET "u]", __func__, xLeftIncl, holewidth, inslen );
      GetLineForInsert( dest, lineNum, xLeftIncl, fInsert ? holewidth : 0 );
      const auto lcols( StrCols( TabWidth(), dest ) );
      const auto maxCol( fInsert ? lcols : xLeftIncl+inslen );                 DE && DBG( "%s GL4Ins: cch/col=%" PR_BSRSIZET "u/%d maxCol=%" PR_BSRSIZET "u", __func__, dest.length(), lcols, maxCol );
      Assert( lcols >= xLeftIncl );

      // dest contains:
      //    !fInsert: at least xLeftIncl           chars
      //     fInsert: at least xLeftIncl+holewidth chars
      //
      // in any case, we need to copy (ins L inslen) into buf+xLeftIncl
      dest.replace( xLeftIncl, inslen, ins.data(), inslen );
      // now, either
      // a. terminate the seg-zone immediately after (ins L inslen)
      // b. space-pad the remainder of the seg-zone (IF there are any original-line chars on the trailing side of the seg-zone), or
      if( lcols < xRightIncl ) {
         dest.resize( xLeftIncl + inslen );
         }
      else if( holewidth > inslen ) {
         dest.replace( xLeftIncl + inslen, holewidth - inslen, holewidth - inslen, ' ' );
         }
      DE && DBG( "%s- PutLine(merged) )", __func__ );
      PutLine( lineNum, dest, stmp );
#else

      dbllinebuf buf;
      auto lineChars( getLineTabx( BSOB(buf), lineNum ) );
      if( !fInsert )
         NoMoreThan( &xRightIncl, lineChars - 1 ); // prevent gratuitous trailspace generation

      auto gapChars( xRightIncl - xLeftIncl + 1); // as defined by caller
      auto segChars( Strlen( ins ));              // what he gave us to fill it in

      auto gapStart( buf+xLeftIncl );
      if( fInsert && lineChars > xLeftIncl ) { // Inserting & gap falls inside existing string?
         0 && DBG( "%s L %d A", __func__, lineNum );
         // if caller passed in a string longer than the gap is wide make the
         // gap as wide as the string being inserted (gapChars only matters when
         // the gap > length(ins))
         //
         NoSmallerThan( &gapChars, segChars );

         // NB: CODE BELOW COULD FORCE LONGER THAN LEGAL LINE, SO WE USE dbllinebuf buf
         memmove(gapStart+gapChars, gapStart, Strlen(gapStart)+1 ); // open gap, COPYING EoL
         // NB: CODE ABOVE COULD FORCE LONGER THAN LEGAL LINE, SO WE USE dbllinebuf buf
         memcpy( gapStart, ins, segChars );                         // ins new str seg w/no termination
         if( gapChars > segChars )
            memset( gapStart+segChars, ' ', gapChars-segChars );    // fill right part of gap
         }
      else {
         0 && DBG( "%s B Ln %d, gap=%d, seg=%d", __func__, lineNum, gapChars, segChars );

         if( lineChars < xLeftIncl ) { // existing string doesn't reach to gap?
            memset( buf+lineChars, ' ', xLeftIncl-lineChars ); // fill in upto gap
            lineChars = xLeftIncl;
            memcpy( gapStart, ins, segChars+1 );
            }
         else if( lineChars > xRightIncl ) { // existing string exists to right of gap?
            if( segChars > gapChars ) { // overwrite gap, push right those existing chars to right of gap
               0 && DBG( "A" );
               memmove( gapStart+gapChars+(segChars-gapChars), gapStart+gapChars, Strlen(gapStart+gapChars)+1 ); // widen gap, COPYING EoL
               memcpy( gapStart, ins, segChars );
               }
            else if( segChars < gapChars ) { // overwrite gap with ins + space-fill
               0 && DBG( "B" );
               memcpy( gapStart         , ins, segChars );
               memset( gapStart+segChars, ' ', gapChars-segChars );
               }
            else {
               0 && DBG( "C" );
               memcpy( gapStart, ins, segChars );
               }
            }
         else { // existing line ends within gap; simply overwrite
            0 && DBG( "D" );
            memcpy( gapStart, ins, segChars+1 ); // COPY EoL
            }
         }
      PutLine( lineNum, buf, stmp );
#endif
      }
   }

void LineInfo::FreeContent( const FBUF &fbuf ) {
   if( fCanFree_pLineData( fbuf ) ) {
      Free0( d_pLineData );
      }
   }

void FBUF::DirtyFBufAndDisplay() {
   if( IsDirty() )
      return;

   if( this == g_CurFBuf() )
      DispNeedsRedrawStatLn();

   SetDirty();
   }


enum { LineHeadSpace =  1
                 //    50
     , FileReadLineHeadSpace = 21
     };

void FBUF::InsertLines__( const LINE yInsAt, const LINE lineInsertCount, const bool fSaveUndoInfo ) {
   if( yInsAt > LastLine() ) // no existing line inserted in front of?
      return;

   FBOP::PrimeRedrawLineRangeAllWin( this, yInsAt, LineCount() + lineInsertCount );
   DirtyFBufAndDisplay();

   if( fSaveUndoInfo )
      UndoInsertLineRangeHole( yInsAt, lineInsertCount );  // generate a undo record

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

//
// This is an "INTERESTING" fxn!  It's called both by upper-level edit code (as
// DelLine) AND by Undo/Redo code (as DeleteLines__ForUndoRedo).  When
// DeleteLines__ForUndoRedo is called, fSaveUndoInfo == false.
//
// DelLine NEVER calls with (fSaveUndoInfo == false), so the memory
// leak which is noted below actually "can't happen"
//
void FBUF::DeleteLines__( LINE firstLine, LINE lastLine, bool fSaveUndoInfo ) {
   0 && DBG("%s [%d..%d]", __func__, firstLine, lastLine );
   if( firstLine > LastLine() ) // if user is deleting lines that are displayed, but not actually present in the file
      return;

   BadParamIf( , (firstLine > lastLine) );

   FBOP::PrimeRedrawLineRangeAllWin( this, firstLine, LineCount() );
   DirtyFBufAndDisplay();
   Min( &lastLine, LastLine() );

   if( fSaveUndoInfo )
      UndoSaveLineRange( firstLine, lastLine );
 #if 0  // if you do this (to fix what looks like a memory leak), undo/redo BREAKS because it calls this API with fSaveUndoInfo == false
   else { // d_paLineInfo[firstLine..lastLine] abt to be overwritten: free
      for( auto iy(firstLine) ; iy <= lastLine ; ++iy )
         d_paLineInfo[iy].FreeContent();
      }
 #endif

   // slide higher-numbered-line's lineinfo's down to fill in d_paLineInfo[firstLine..lastLine]

   MoveArray( d_paLineInfo + firstLine, d_paLineInfo + lastLine + 1, LastLine() - lastLine );

   const auto yDelta( lastLine - firstLine + 1 );
   IncLineCount( -yDelta ); // CANNOT swap with next stmt
   InitLineInfoRange( LineCount(), yDelta ); // fill in hole left by "slide ... down"

   FBufEvent_LineInsDel( firstLine, -yDelta );
   }


void FBUF::FreeOrigFileImage() {
   Free0( d_pOrigFileImage );
   d_cbOrigFileImage = 0;
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

   DiscardUndoInfo();   // call before FreeOrigFileImage() (some EditOp's have d_pFBuf)!
   FreeOrigFileImage(); // call after  DiscardUndoInfo()!

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

int FBUF::ViewCount() {
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

//
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


   Xbuf xbFirst; FBdest->GetLineForInsert( &xbFirst, yDst, xDst, 0 ); // rd dest line containing insertion point
   Xbuf xbLast;
   if( FBsrc ) {
      FBsrc->GetLineForInsert( &xbLast, ySrcEnd, xSrcEnd, 0 );  // rd last line of src test
      }
   else {
      if( xSrcEnd > 0 ) { memset( xbLast.wresize( xSrcEnd ), ' ', xSrcEnd ); } // if is not strictly necessary, but silences a GCC warning "memset used with constant zero length parameter" (when this entire fxn is inlined!!!)
      }

   const auto yDstLast( yDst + (ySrcEnd - ySrcStart) );
   const auto twd( FBdest->TabWidth() );
   std::string stmp;
   {
   const auto pDestSplit( PtrOfColWithinStringRegion( twd, xbFirst.wbuf(), xbFirst.wbuf()+xbFirst.length(), xDst ) ); // dest text PAST insertion point
   auto taillen( Strlen( pDestSplit ) );
   auto srcbuf( xbLast.wresize( xSrcEnd + taillen + 1 ) );  // worst case, ignores possible tab compression
   strcpy( PtrOfColWithinStringRegion( twd, srcbuf, Eos(srcbuf), xSrcEnd ), pDestSplit ); // dest text PAST insertion point -> srcbuf past xSrcEnd
   FBdest->PutLine( yDstLast, srcbuf, stmp );
   pDestSplit[0] = '\0'; // this belongs as else case for if( FBsrc ) below, but uses this scope's pDestSplit
   }

   //*** merge & write first line of FBsrc stream [destbuf:srcbuf]
   if( FBsrc ) {
      FBsrc->GetLineForInsert( &xbLast, ySrcStart, xSrcStart, 0 );
      const auto pSrc( PtrOfColWithinStringRegion( FBsrc->TabWidth(), xbLast.wbuf(), xbLast.wbuf()+xbLast.length(), xSrcStart ) );
            auto taillen( Strlen( pSrc ) );
      const auto dstbuf( xbFirst.wresize( xDst + taillen + 1 ) );
      const auto pDestSplit( PtrOfColWithinStringRegion( twd, dstbuf, Eos(dstbuf), xDst ) ); // dest text PAST insertion point
      memcpy( pDestSplit, pSrc, taillen + 1 );
      }
   FBdest->PutLine( yDst, xbFirst.c_str(), stmp );

   AdjMarksForInsertion( FBdest, FBdest, xDst     , yDst     , COL_MAX, yDst     , xSrcEnd-1, yDstLast );
   AdjMarksForInsertion( FBsrc , FBdest,         0, ySrcEnd  , xSrcEnd, ySrcEnd  ,         0, yDstLast );
   AdjMarksForInsertion( FBsrc , FBdest, xSrcStart, ySrcStart, COL_MAX, ySrcStart, xDst     , yDst     );
   }

//
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
      FBdest->GetLineForInsert( stDst, yDst, xDst, boxWidth );
      if( FBsrc ) {
         FBsrc->GetLineForInsert( stSrc, ySrc, xSrcRight + 1, 0 );
         const auto srl( CaptiveIdxOfCols( tws, stSrc, xSrcLeft, xSrcRight ) );
         stDst.replace( CaptiveIdxOfCol( twd, stDst, xDst ), boxWidth, stSrc, srl.ix0, srl.ix1 - srl.ix0 + 1 );
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
   auto fileBytes( fio::SeekEoF( hFile ) );
   0 && DBG( "fio::SeekEoF returns %8" WL( PR__i64 "u", "ld" ), fileBytes );
   Assert( fileBytes >= 0 );
   fio::SeekBoF( hFile );
   if( fileBytes > UINT_MAX ) {
      Msg( "filesize is larger than UINT_MAX" );
      return true;
      }

   d_cbOrigFileImage = fileBytes;

   rdNoiseAllc();
   AllocBytesNZ( d_pOrigFileImage, fileBytes+1, __func__ );
   d_pOrigFileImage[fileBytes] = '\0'; // so wcslen works

   VR_(
      DBG( "ReadDiskFile %s: %uKB buf=[%p..%p)"
         , Name()
         , fileBytes/1024
         , d_pOrigFileImage
         , d_pOrigFileImage + d_cbOrigFileImage
         );
      )

   rdNoiseRead();

   struct {
      int leadBlankLines = 0;
      int lead_Tab_Lines = 0;
      } tabStats;

   MainThreadPerfCounter pc;

   if( !fio::ReadOk( hFile, d_pOrigFileImage, fileBytes ) ) {
      FreeOrigFileImage();
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
      if( 0==memcmp( d_pOrigFileImage, &BoM_UTF16, sizeof(BoM_UTF16) ) ) {
#ifdef _WIN32
         // adapted from http://stackoverflow.com/questions/3082620/convert-utf-16-to-utf-8
         Win32::LPCWSTR pszTextUTF16( reinterpret_cast<Win32::LPCWSTR>( d_pOrigFileImage + sizeof(BoM_UTF16) ) );
         const auto utf16len( wcslen(pszTextUTF16) );
         const auto cp(
            //CP_UTF8   // if we could display UTF8, this would be the correct choice
              CP_OEMCP  // but we can't (and may never), so this is the safest choice
            );
         const auto utf8len( Win32::WideCharToMultiByte( cp, 0, pszTextUTF16, utf16len, nullptr, 0, nullptr, nullptr ) ); // note this returns an int (even in x64)
         0 && DBG( "reading UTF-16 file \"%s\": fileBytes=%" PR__i64 "u, utf16len=%" PR_SIZET "u, utf8len=%d", Name(), fileBytes, utf16len, utf8len );
         PChar utf16buf;
         AllocBytesNZ( utf16buf, utf16len+1, __func__ );
         utf16buf[utf16len] = '\0';
         Win32::WideCharToMultiByte( cp, 0, pszTextUTF16, utf16len, utf16buf, utf8len, nullptr, nullptr );
         // adjust external state accordingly
         FreeUp( d_pOrigFileImage, utf16buf );
         d_cbOrigFileImage = fileBytes = utf8len;
#endif
         }
      }
   //--------------------------------------------------------------------------------------------------------

   rdNoiseScan();

#if VERBOSE_READ
   const auto tmIO( pc.Capture() );
#endif

   d_EolMode = platform_eol;

   auto InitLineInfo = [this] ( int initial_sample_lines ) {
      d_naLineInfoElements = initial_sample_lines;
      VR_( DBG( "ReadDiskFile LineInfo       0 -> %7d", d_naLineInfoElements ); )

      AllocArrayNZ(      d_paLineInfo, d_naLineInfoElements, "initial d_paLineInfo" );
      InitLineInfoRange( 0           , d_naLineInfoElements );
      };

   if( fileBytes == 0 ) {
      InitLineInfo( 1 ); // alloc dummy so HasLines() will be true, preventing repetitive disk rereads
      }
   else {
      auto numCRs( 0 );
      auto numLFs( 0 );
      auto curLineNum( 0 );
      InitLineInfo( 508 ); // slightly less than a power of 2
      auto pLi( d_paLineInfo );
      auto pCurImageBuf( d_pOrigFileImage );
      const auto pPastImageBufEnd( d_pOrigFileImage + fileBytes );

      while( pCurImageBuf < pPastImageBufEnd ) { // we are about to read line 'curLineNum'
         if( d_naLineInfoElements <= curLineNum ) { // need to reallocate d_paLineInfo
            // this is a little obscure, since for brevity I'm using 'curLineNum' as an alias
            // for 'LineCount()'; these are equivalent since 'curLineNum' hasn't been stored yet.
            //
            const double avgBytesPerLine(static_cast<double>(pCurImageBuf - d_pOrigFileImage) / curLineNum);
                  double dNewLineCntEstimate( (fileBytes / avgBytesPerLine) * 1.025 );
            if( dNewLineCntEstimate <= d_naLineInfoElements )
                dNewLineCntEstimate = Max( static_cast<double>(d_naLineInfoElements * 1.125)
                                         , static_cast<double>(d_naLineInfoElements + FileReadLineHeadSpace)
                                         );

            const auto newLineCntEstimate( static_cast<LINE>(dNewLineCntEstimate) );

            VR_(
               DBG( "ReadDiskFile LineInfo %7d -> %7d (avg=%4.1f)"
                  , d_naLineInfoElements
                  , newLineCntEstimate
                  , avgBytesPerLine
                  );
               )

            d_naLineInfoElements = newLineCntEstimate;
            //
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

            if( ch == 0x0A ) { // LF=Unix EOL?  treated as TRUE EOL indicator
               ++numLFs;
               ++cbEOL;
               break;          // treat as EOL
               }

            if( ch == 0x0D ) { // CR=(start of) MS EOL?  simply skipped while awaiting next LF
               ++cbEOL;
               ++numCRs;
               continue;
               }

            if( cbEOL ) {      // unexpected: prev ch was CR but this ch is not LF (ancient Macintosh EOL=CR only???)
               --pCurImageBuf; // current char is part of NEXT line
               break;          // treat as EOL
               }
#else
            if( (ch < ' ') && (ch != HTAB) ) {      // ALL "control chars" except HTAB rendered invisible (swallowed up into EOL)
               ++cbEOL;
               switch( ch ) {
                  case 0x0A: ++numLFs; goto IS_EOL; // LF=Unix EOL?  treated as TRUE EOL indicator
                  case 0x0D: ++numCRs; break;       // CR=(start of) MS EOL?  simply skipped while awaiting next LF
                  default:             break;
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

         if( ExecutionHaltRequested() ) {
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
#if defined(_WIN32)
      if( numCRs == 0 && numLFs > 0 ) { d_EolMode = EolLF  ; }
#else
      if( numLFs == 0 && numCRs > 0 ) { d_EolMode = EolCRLF; }
#endif

      d_Tabconv = TABCONV_0_NO_CONV;

      enum { PERCENT_LEAD_BLANK_TO_CAUSE_TABCONV_MODE = 50 };
      if(  tabStats.leadBlankLines > 0
        && (    (tabStats.lead_Tab_Lines > (MAX_LINES / 100))  // mk sure no ovflw
           || (((tabStats.lead_Tab_Lines * 100) / tabStats.leadBlankLines) > PERCENT_LEAD_BLANK_TO_CAUSE_TABCONV_MODE )
           )
        ) {
         d_Tabconv = TABCONV_1_LEADING_SPCS_TO_TABS;
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
            , ((double)fileBytes - numBytesToProcessInImageBuf) / (double)LineCount()
            , wastedLIbytesNow / 1024
            , (d_naLineInfoElements) * sizeof(*d_paLineInfo) / 1024
            , 100.0*((double)d_naLineInfoElements - LineCount()) / (double)LineCount()

            , wastedLIbytes / 1024
            , 100.0*((double)PredLC - actualLC) / (double)actualLC
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
   0 && DBG( "ImgBufAlloc Bytes=%" PR_SIZET "u, Lines=%d", bufBytes, PreallocLines );
   d_cbOrigFileImage = bufBytes;
   AllocArrayNZ( d_pOrigFileImage, bufBytes, __func__ );
   SetLineCount( 0 );
   SetLineInfoCount( PreallocLines );
   }

LineInfo & FBUF::ImgBufNextLineInfo() {
   // LineCount is the NUMBER of the NEW LINE!

   SetLineInfoCount( LineCount()+1 );

   auto &newLI( d_paLineInfo[ LineCount() ] );

   if( LineCount() == 0 )
      newLI.d_pLineData = d_pOrigFileImage;
   else {
      auto &preLI( d_paLineInfo[ LastLine() ] );
      newLI.d_pLineData = preLI.d_pLineData + preLI.d_iLineLen;
      }

   IncLineCount( 1 );  // LineCount() NOT CHANGED UNTIL HERE

   return newLI;
   }

void FBUF::ImgBufAppendLine( PCChar pNewLineData, int LineLen ) {
   auto &newLI( ImgBufNextLineInfo() );
   newLI.d_iLineLen = LineLen;
   memcpy( newLI.d_pLineData, pNewLineData, newLI.d_iLineLen );
   0 && DBG( "ImgBufAppendLine Bytes=%d", newLI.d_iLineLen );
   d_ImgBufBytesWritten += newLI.d_iLineLen;
   }

void FBUF::ImgBufAppendLine( PFBUF pFBufSrc, int srcLineNum, PCChar prefix ) {
   auto &newLI( ImgBufNextLineInfo() );
   auto preLen(0);
   if( prefix )
      memcpy( newLI.d_pLineData, prefix, (preLen = Strlen( prefix )) );

   auto &srcLI( pFBufSrc->d_paLineInfo[srcLineNum] );
   memcpy( newLI.d_pLineData+preLen, srcLI.d_pLineData, srcLI.d_iLineLen );
   newLI.d_iLineLen = srcLI.d_iLineLen + preLen;
   0 && DBG( "FBUF::ImgBufAppendPFLine Bytes=%d", newLI.d_iLineLen );
   d_ImgBufBytesWritten += newLI.d_iLineLen;
   }

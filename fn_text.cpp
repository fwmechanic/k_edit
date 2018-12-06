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

sridx first_alpha( stref src, sridx start ) {
   if( start < src.length() ) {
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( !isalpha( *it ) ) {
            return std::distance( src.cbegin(), it );
            }
         }
      }
   return std::distance( src.cbegin(), src.end() );
   }

STATIC_FXN char first_alpha( PCChar str ) {
   while( (*str != '\0') && (!isalpha(*str)) ) {
      ++str;
      }
   return *str;
   }

int FlipCase( int ch ) {
   if( isalpha(ch) ) { return islower(ch) ? _toupper(ch) : _tolower(ch); }
   switch( ch ) {
      case '+' : return '-' ;
      case '-' : return '+' ;
      case '/' : return '\\';
      case '\\': return '/' ;
      case chBackTick : return chQuot1    ;
      case chQuot1    : return chBackTick ;
      default:  return ch;
      }
   }

STATIC_FXN PChar DupLineSeg_( PFBUF pfb, std::string &st, LINE yLine, COL xLeftIncl, COL xRightIncl ) {
   pfb->DupLineSeg( st, yLine, xLeftIncl, xRightIncl );
   return CAST_AWAY_CONST(PChar)( st.c_str() );
   }

STATIC_CONST char no_alpha[] = "Warning: no alphabetic chars found in argument!";

#define USE_TEXTARG_NULLEOx_edit 1
#if     USE_TEXTARG_NULLEOx_edit
/*
   20141227 turns out this is a _really_ unusual case: user NULLEOW/L arg region
   is _EDITED_.  I'm not sure if there are any other examples of this.
*/
class TEXTARG_NULLEOx_edit {
protected:
         FBUF &d_fb;
   const ARG  &d_arg;
   std::string d_str;
   bool        d_rv;
   virtual bool VEdit() = 0;
public:
   TEXTARG_NULLEOx_edit( const ARG &arg_, FBUF &fb_ )
      : d_fb( fb_ )
      , d_arg( arg_ )
      , d_str( d_arg.d_textarg.pText )
      , d_rv( false )
      {}

   bool Edit() {
      d_rv = VEdit();
      if( d_rv ) {
         const auto xLeft( d_arg.d_textarg.ulc.col );
         std::string t0,t1;
         d_fb.PutLineSeg( d_arg.d_textarg.ulc.lin, d_str.data(), t0,t1, xLeft, xLeft + d_str.length() - 1 );  // overlay converted string
         }
      return d_rv;
      }
   virtual ~TEXTARG_NULLEOx_edit() {}
   };

class TEXTARG_NULLEOx_edit_flipcase : public TEXTARG_NULLEOx_edit {
   bool VEdit() override {
      const auto a1( FirstAlphaOrEnd( d_str, 0 ) );
      if( a1 == d_str.length() ) {
         return d_arg.fnMsg( no_alpha );
         } 0 && DBG( "a1= %" PR_SIZET, a1 );
      const auto fx( islower( d_str[a1] ) ? ::toupper : ::tolower );
      std::transform( d_str.cbegin()+a1, d_str.cend(), d_str.begin()+a1, fx );
      return true;
      }
public:
   TEXTARG_NULLEOx_edit_flipcase( const ARG &arg_, FBUF &fb_ ) : TEXTARG_NULLEOx_edit( arg_, fb_ ) {}
   };

#endif

bool ARG::flipcase() {
   std::string stbuf;
   PCF;
   switch( d_argType ) {
      // case STREAMARG: someday!?
      case BOXARG: {
               auto yMin( d_boxarg.flMin.lin );
         const auto yMax( d_boxarg.flMax.lin );
         const auto xMin( d_boxarg.flMin.col );
         const auto xMax( d_boxarg.flMax.col );
         // locate first alphabetic char in box
         char zc;
         do {
            const auto inbuf( DupLineSeg_( pcf, stbuf, yMin, xMin, xMax ) );
            zc = first_alpha( inbuf );
            } while( (zc == '\0') && (++yMin <= yMax) );
         if( zc == '\0' ) {
            return fnMsg( no_alpha );
            }
         auto pfx_casexlat( ToBOOL( islower(zc) ) ? _strupr : _strlwr );
         std::string t0,t1;
         for( ; yMin <= yMax; ++yMin ) {
            const auto inbuf( DupLineSeg_( pcf, stbuf, yMin, xMin, xMax ) );
            pfx_casexlat(inbuf);
            pcf->PutLineSeg( yMin, inbuf, t0,t1, xMin, xMax );
            }
         return true;
         }
      case NOARG: {
         const auto rls( pcf->PeekRawLineSeg( d_noarg.cursor.lin, d_noarg.cursor.col, d_noarg.cursor.col ) ); if( rls.empty() ) { return false; }
         const auto newCh( FlipCase( rls[0] ) ); if( newCh == rls[0] ) { return false; }
         std::string tmp1, tmp2;
         FBOP::ReplaceChar( pcf, d_noarg.cursor.lin, d_noarg.cursor.col, newCh, tmp1, tmp2 );
         return true;
         }
      case TEXTARG: { // actually NULLEOW (never BOXSTR)
        #if USE_TEXTARG_NULLEOx_edit
         TEXTARG_NULLEOx_edit_flipcase editor( *this, *g_CurFBuf() );
         return editor.Edit();
        #else
         linebuf argBuf;
         bcpy( argBuf, d_textarg.pText );
         const auto zc( first_alpha( argBuf ) );
         if( !zc ) {
            return fnMsg( no_alpha );
            }
         islower(zc) ? _strupr(argBuf) : _strlwr(argBuf);
         auto xLeft( d_textarg.ulc.col );
         std::string t0,t1;
         pcf->PutLineSeg( d_textarg.ulc.lin, argBuf, t0,t1, xLeft, xLeft+Strlen(argBuf)-1 );  // overlay converted string
         return true;
        #endif
         }
      default:
         return false;
      }
   }

//=================================================================================================

bool ARG::longline() {
   PCFV;
   LINE yMin; LINE yMax; COL xMin; COL xMax;
   const auto fMoveLine( !GetSelectionLineColRange( &yMin, &yMax, &xMin, &xMax ) );
   if( fMoveLine ) {
      yMin = 0;
      yMax = pcf->LineCount();
      }
   auto max_lnum( 0 );  auto max_len ( 0 );
   for( auto iy(yMin); iy <= yMax; ++iy ) {
      const auto len( FBOP::LineCols( pcf, iy ) );
      if( len > max_len ) {
         max_lnum = iy;
         max_len  = len;
         }
      }
   const Point Cursor( pcv->Cursor() );
   pcv->MoveCursor( fMoveLine ? max_lnum : Cursor.lin, max_len );
   return true;
   }

//=================================================================================================

bool ARG::quik() {
   fnMsg( "?" );
   const auto ch( tolower( CharAsciiFromKybd() ) );
   switch( ch ) {
      default :  return Msg( "quik + %c ?", ch );
      case 'a':  return mword();
      case 'f':  return pword();
      case 'e':  return up();
      case 's':  return left();
      case 'd':  return right();
      case 'x':  return down();
      }
   }

//  Get line, blank-padded on the right to xMin inclusive.
//
STATIC_FXN void GetMinLine( std::string &dest, LINE y, COL xMin, PFBUF pFBuf ) {
   pFBuf->DupLineForInsert( dest, y, xMin+1, 0 );
   }

//  Draw a box using the OEM (IBM PC) box drawing chars
//
bool ARG::makebox() {
   enum eCorners { NW, EW, NE, SW, SE, NS, WW, EE, NN, SS, CE, CORNER_CHARS };
   STATIC_VAR CPCChar achBox[] = {
    // NW
    // |EW
    // ||NE
    // |||SW
    // ||||SE
    // |||||NS
    // ||||||WW
    // |||||||EE
    // ||||||||NN
    // |||||||||SS
    // ||||||||||CE
    // |||||||||||
      "ÚÄ¿ÀÙ³Ã´ÂÁÅ",
      "ÉÍ»È¼ºÌ¹ËÊÎ",
      "ÕÍ¸Ô¾³ÆµÑÏØ",
      "ÖÄ·Ó½ºÇ¶ÒÐ×",
      "+-+++|+++++",
      "=====|=====",
      "°°°°°°°°°°°",
      "±±±±±±±±±±±",
      "²²²²²²²²²²²",
      "ÛÛÛÛÛÛÛÛÛÛÛ",
      "þþþþþþþþþþþ",
      };

   fnMsg( "Key->Box: 0->Å 1->Î 2->Ø 3->× 4->+ 5->= 6->° 7->± 8->² 9->Û A->þ ?->?" );
   fnMsg( "Key->Box: 1->Å 2->Î 3->Ø 4->× 5->+ 6->= 7->° 8->± 9->² 0->Û A->þ ?->?" );
   const char ch( tolower( CharAsciiFromKybd() ) );
   if( ch == 27 ) { // ESC == cancel
      return false;
      }
   char   cbuf[CORNER_CHARS];
   PCChar pchBox;
   STATIC_CONST char achMenuChar[]="1234567890A";
   if( (pchBox = strchr( achMenuChar, ch )) == nullptr ) {
      // Character not in menu, so use it as the drawing character
      for( auto &cbc : cbuf ) { cbc = ch; }
      pchBox = cbuf;
      }
   else {
      pchBox = achBox[pchBox - achMenuChar];
      }
   std::string buf;
   std::string stTmp;
   switch( d_argType ) {
      default: return BadArg();
      case NOARG: // replace char @ at cursor position w/"CE" of selected type
         GetMinLine( buf, d_noarg.cursor.lin, d_noarg.cursor.col, g_CurFBuf() );
         buf[d_noarg.cursor.col] = pchBox[CE];
         g_CurFBuf()->PutLineEntab( d_noarg.cursor.lin, buf, stTmp );
         g_CurView()->MoveCursor( d_noarg.cursor.lin, d_noarg.cursor.col + 1 );
         break;
      case BOXARG: {
         const auto fBox(  d_cArg == 1 );
               auto xTemp( d_boxarg.flMin.col );
               auto yTemp( d_boxarg.flMin.lin );
         if( d_boxarg.flMin.col == d_boxarg.flMax.col ) { // VERTICAL LINE?
            GetMinLine( buf, yTemp, xTemp, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[EW]) && fBox) ? pchBox[NN] : pchBox[NS];
            g_CurFBuf()->PutLineEntab( yTemp++, buf, stTmp );
            while( yTemp < d_boxarg.flMax.lin ) {
               GetMinLine( buf, yTemp, xTemp, g_CurFBuf() );
               buf[xTemp] = ((buf[xTemp]==pchBox[EW]) && fBox) ? pchBox[CE] : pchBox[NS];
               g_CurFBuf()->PutLineEntab( yTemp++, buf, stTmp );
               }
            GetMinLine( buf, yTemp, xTemp, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[EW]) && fBox) ? pchBox[SS] : pchBox[NS];
            g_CurFBuf()->PutLineEntab( yTemp, buf, stTmp );
            }
         else if( d_boxarg.flMin.lin == d_boxarg.flMax.lin ) { // HORIZONTAL LINE?
            GetMinLine( buf, d_boxarg.flMin.lin, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[WW] : pchBox[EW];    ++xTemp;
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[CE] : pchBox[EW]; ++xTemp;
               }
            buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[EE] : pchBox[EW];    ++xTemp;
            g_CurFBuf()->PutLineEntab( d_boxarg.flMin.lin, buf, stTmp );
            }
         else { // BOX
            // TOP LINE
            //
            GetMinLine( buf, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp++] = pchBox[NW];
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp++] = pchBox[EW];
               }
            buf[xTemp++] = pchBox[NE];
            g_CurFBuf()->PutLineEntab( yTemp++, buf, stTmp );
            // MIDDLE LINE(S)
            //
            while( yTemp < d_boxarg.flMax.lin ) {
               xTemp = d_boxarg.flMin.col;
               GetMinLine( buf, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
               buf[xTemp] = pchBox[NS];
               for( ++xTemp ; xTemp < d_boxarg.flMax.col ; ++xTemp ) {
                  if( d_cArg > 1 ) {
                     buf[xTemp] = pchBox[CE];
                     }
                  }
               buf[xTemp] = pchBox[NS];
               g_CurFBuf()->PutLineEntab( yTemp++, buf, stTmp );
               }
            // BOTTOM LINE
            //
            xTemp = d_boxarg.flMin.col;
            GetMinLine( buf, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp++] = pchBox[SW];
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp++] = pchBox[EW];
               }
            buf[xTemp] = pchBox[SE];
            g_CurFBuf()->PutLineEntab( yTemp, buf, stTmp );
            }
         }
         break;
      }
   return true;
   }

//=================================================================================================

// "library" function

unsigned uint_log_10( unsigned num ) {
   // figure out min width of fixed-width field needed to hold line #
   //    poor-man's log10 function:  28-Mar-2001 klg
   auto rv( 1u );
#if 0
   while( num >= 10 ) {
      num /= 10;
      ++rv;
      }
   return rv;
#else
   // https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920?_fb_noscript=1
   for(;;) {
      if( num <    10 ) { return rv + 0; }
      if( num <   100 ) { return rv + 1; }
      if( num <  1000 ) { return rv + 2; }
      if( num < 10000 ) { return rv + 3; }
      // Skip ahead by 4 orders of magnitude
      num /= 10000U;
      rv += 4;
      }
#endif
   }

typedef int rtl_errno_t;

// http://stackoverflow.com/a/34774296  "How do you implement strtol under const-correctness?"
STATIC_FXN int32_t strtol_er( rtl_errno_t &conv_errno, PCChar nptr, PCChar &endptr, int base ) {
   errno = 0;
   const auto rv( strtol( nptr, &CAST_AWAY_CONST(PChar &)(endptr), base ) );
   conv_errno = errno;
   errno = 0;
   return rv;
   }

bool ARG::vrepeat() { enum {DB=0};
   auto lx( d_boxarg.flMin.lin );
   std::string st; g_CurFBuf()->DupLineSeg( st, lx, d_boxarg.flMin.col, d_boxarg.flMax.col ); // get line containing fill segment
   CPCChar inbuf( st.c_str() );
   DB && DBG( "fillseg [%d..%d] = '%s'", d_boxarg.flMin.col, d_boxarg.flMax.col, inbuf );
   const auto fInsertArg( d_cArg < 2 );
   std::string t0,t1;
   if( !d_fMeta ) {
      for( ++lx ; lx <= d_boxarg.flMax.lin; ++lx ) { // each line in boxarg
         g_CurFBuf()->PutLineSeg( lx, inbuf, t0,t1, d_boxarg.flMin.col, d_boxarg.flMax.col, fInsertArg );
         }
      }
   else { // insert incrementing sequence of numbers, with initial value given in top line of BOXARG
      PCChar pe; rtl_errno_t conv_errno;
      int val( strtol_er( conv_errno, st.c_str(), pe, 0 ) );
      if( conv_errno /* || *pe != 0 */ ) {
         return Msg( "%s could not convert first-line content '%s' to int", __func__, st.c_str() );
         }
      const auto ixMaxDigit( pe - st.c_str() - 1 ); // pe points at char AFTER digits
      const auto ixMinDigit( FirstDigitOrEnd( st ) ); if( atEnd( st, ixMinDigit ) ) { return Msg( "internal error, no first digit?" ); }
      enum { MAX_INT_PRINT_CHARS = 9 };
      const auto fLead0( '0' == st[ixMinDigit] );
      const auto width_DueToArgWidth( st.length() );  DB && DBG( "width_DueToArgWidth=%" PR_SIZET, width_DueToArgWidth );
      decltype(width_DueToArgWidth) width_DueToArgHeight( uint_log_10( val + (1 + d_boxarg.flMax.lin - d_boxarg.flMin.lin) ) );  DB && DBG( "width_DueToArgHeight=%" PR_SIZET, width_DueToArgHeight );
      const auto width( std::max( width_DueToArgHeight, width_DueToArgWidth ) );
      if( width > MAX_INT_PRINT_CHARS ) { return Msg( "internal error, width %" PR_SIZET " > %d", width, MAX_INT_PRINT_CHARS ); }

      FmtStr<7>fmts( "%%%s%" PR_SIZET "d", fLead0 ? "0":"", width ); const auto fmt( fmts.k_str() );   DB && DBG( "fmt='%s'", fmt );
      FmtStr<1+MAX_INT_PRINT_CHARS> st0( fmt, val ); auto ps0 = st0.k_str();  DB && DBG( "st0='%s'", ps0 );
      const auto xMax( d_boxarg.flMin.col+width-1 );
      bool ins;
      if( xMax == d_boxarg.flMax.col ) {
         ins = false;
         }
      else {
         g_CurFBuf()->DelBox( d_boxarg.flMin.col, lx, d_boxarg.flMax.col, lx );
         ins = true;
         }
      g_CurFBuf()->PutLineSeg( lx, FmtStr<1+MAX_INT_PRINT_CHARS>( fmt, val ).k_str(), t0,t1, d_boxarg.flMin.col, xMax, ins );
      for( ++lx ; lx <= d_boxarg.flMax.lin; ++lx ) { // each line in boxarg
         g_CurFBuf()->PutLineSeg( lx, FmtStr<1+MAX_INT_PRINT_CHARS>( fmt, ++val ).k_str(), t0,t1, d_boxarg.flMin.col, xMax, true );
         }
      }
   return true;
   }

bool ARG::numlines () {
   const auto lwidth( uint_log_10( g_CurFBuf()->LineCount() ) );
   std::string t0,t1;
   for( auto line(0); line < g_CurFBuf()->LineCount(); ++line ) {
      g_CurFBuf()->PutLineSeg( line, FmtStr<33>( "%*u ", lwidth, line+1 ).k_str(), t0,t1, 0, 0, true ); // insert @ BoL
      }
   return true;
   }

//////////////////////////////////////////////////////////////////////////
//
// Arg <textarg> EraseBuf  Removes the buffer named by textarg.
//
//////////////////////////////////////////////////////////////////////////

bool ARG::erasebuf() {
   //
   // EraseBuf - erase entire contents of buffer (FBUF) (does not affect file on disk
   //            unless you forcibly save it)
   //
   // TEXTARG -  erase buffer named by textarg (if buffer corresponds to a disk
   //            file, the textarg MUST contain the full (drive too!) pathname
   //            of the file)
   //
   // NOTE: Since the MODIFIES flag is not set for this function, the editor
   //       does not know that the buffer has been changed, and WILL NOT
   //       update any corresponding disk file!
   //
   auto delfl( FindFBufByName( d_textarg.pText ) );
   if( !delfl ) {
      return fnMsg( "buffer '%s' doesn't exist", d_textarg.pText );
      }
   auto curfile( g_CurFBuf() );
   delfl->MakeEmpty();
   delfl->MoveCursorToBofAllViews();   // BUGBUG this is wierd: port to Lua?
   delfl->PutFocusOn();              // BUGBUG this is wierd: port to Lua?
   g_CurView()->MoveCursor( 0, 0 );  // BUGBUG this is wierd: port to Lua?
   curfile->PutFocusOn();
   return true;
   }

void Clipboard_PutText( stref sr ) {
   g_pFbufClipboard->MakeEmpty();
   g_pFbufClipboard->PutLineRaw( 0, sr );
   g_ClipboardType = BOXARG;
   }

void Clipboard_PutText_Multiline( stref sr ) {
   g_pFbufClipboard->MakeEmpty();
   const int lineCount( g_pFbufClipboard->PutLastMultilineRaw( sr ) );
   g_ClipboardType = (lineCount == 1) ? BOXARG : LINEARG;
   }

#ifdef fn_ascii2hex

//////////////////////////////////////////////////////////////////////////
//
//  <single-line boxarg> ascii2hex Display the hex values of the chars
//                               in the box, up to the width of the display
//                               <910823>   KevinG
//
// Display the hex value of the char under the cursor <910823>   KevinG
//

bool ARG::ascii2hex() {
   linebuf buf; buf[0] = '\0'; auto pB(buf); auto cbB( sizeof(buf) );
   for( auto pC(d_textarg.pText) ; *pC ; ++pC ) {
      snprintf_full( &pB, &cbB, "0x%02X,", *pC );
      }
   if( pB > buf ) {
       pB[-1] = '\0';
       }
   Clipboard_PutText( buf );
   Msg( "%s", buf );

   return true;
   }

#endif

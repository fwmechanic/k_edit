//
//  Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"


boost::string_ref::size_type first_alpha( boost::string_ref src, boost::string_ref::size_type start ) {
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
   while( (*str != '\0') && (!isalpha(*str)) )
      ++str;

   return *str;
   }

char FlipCase( char ch ) {
   if( isalpha(ch) )  return islower(ch) ? _toupper(ch) : _tolower(ch);
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

STATIC_FXN PChar GetLineSeg_( PFBUF pfb, std::string &st, LINE yLine, COL xLeftIncl, COL xRightIncl ) {
   st = pfb->GetLineSeg( yLine, xLeftIncl, xRightIncl );
   return const_cast<PChar>( st.c_str() );
   }

STATIC_CONST char no_alpha[] = "Warning: no alphabetic chars found in argument!";

#define USE_TEXTARG_NULLEOx_edit 0
#if     USE_TEXTARG_NULLEOx_edit
/*
   20141227 turns out this is a _really_ unusual case: use specifies NULLEOW/L,
   and the region so defined is EDITED.  I'm not sure if there are any other
   examples of this.
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
         d_fb.PutLineSeg( d_arg.d_textarg.ulc.lin, d_str.data(), xLeft, xLeft + d_str.length() - 1 );  // overlay converted string
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
         } DBG( "a1= %" PR_SIZET "u", a1 );
      const auto fx( islower( d_str[a1] ) ? ::toupper : ::tolower );
      std::transform( d_str.cbegin(), d_str.cend(), d_str.begin(), fx );
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
            const auto inbuf( GetLineSeg_( pcf, stbuf, yMin, xMin, xMax ) );
            zc = first_alpha( inbuf );
            } while( (zc == '\0') && (++yMin <= yMax) );

         if( zc == '\0' )
            return fnMsg( no_alpha );

         auto pfx_casexlat( ToBOOL( islower(zc) ) ? _strupr : _strlwr );
         for( ; yMin <= yMax; ++yMin ) {
            const auto inbuf( GetLineSeg_( pcf, stbuf, yMin, xMin, xMax ) );
            pfx_casexlat(inbuf);
            pcf->PutLineSeg( yMin, inbuf, xMin, xMax );
            }
         return true;
         }

      case NOARG: {
         const auto rl( pcf->PeekRawLine( d_noarg.cursor.lin ) );
         const auto ix( CaptiveIdxOfCol( pcf->TabWidth(), rl, d_noarg.cursor.col ) );  if( ix == rl.length() ) { return false; }
         const auto newCh( FlipCase( rl[ix] ) );   if( newCh == rl[ix] ) { return false; }
         Xbuf xb;
         FBOP::ReplaceChar( pcf, d_noarg.cursor.lin, d_noarg.cursor.col, newCh, &xb );
         return true;
         }

      case TEXTARG: { // actually NULLEOW (never BOXSTR)
        #if USE_TEXTARG_NULLEOx_edit
         TEXTARG_NULLEOx_edit_flipcase editor( *this, *g_CurFBuf() );
         return editor.Edit();
        #else
         linebuf argBuf;
         SafeStrcpy( argBuf, d_textarg.pText );
         const auto zc( first_alpha( argBuf ) );
         if( !zc )
            return fnMsg( no_alpha );

         islower(zc) ? _strupr(argBuf) : _strlwr(argBuf);

         auto xLeft( d_textarg.ulc.col );
         pcf->PutLineSeg( d_textarg.ulc.lin, argBuf, xLeft, xLeft+Strlen(argBuf)-1 );  // overlay converted string
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
   const auto ch( toLower( CharAsciiFromKybd() ) );
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
STATIC_FXN PChar GetMinLine( PXbuf pxb, LINE y, COL xMin, PFBUF pFBuf ) {
   pFBuf->GetLineForInsert( pxb, y, xMin+1, 0 );
   return pxb->wbuf();
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

   const char ch( toLower( CharAsciiFromKybd() ) );
   if( ch == 27 )  // ESC == cancel
      return false;

   char   cbuf[CORNER_CHARS];
   PCChar pchBox;
   STATIC_CONST char achMenuChar[]="1234567890A";
   if( (pchBox = strchr( achMenuChar, ch )) == nullptr ) {
      // Character not in menu, so use it as the drawing character
      memset( cbuf, ch, sizeof(cbuf) );
      pchBox = cbuf;
      }
   else {
      pchBox = achBox[pchBox - achMenuChar];
      }

   Xbuf  xb;
   PChar buf;
   switch( d_argType ) {
      default: return BadArg();
      case NOARG: // replace char @ at cursor position w/"CE" of selected type
         buf = GetMinLine( &xb, d_noarg.cursor.lin, d_noarg.cursor.col, g_CurFBuf() );
         buf[d_noarg.cursor.col] = pchBox[CE];
         g_CurFBuf()->PutLine( d_noarg.cursor.lin, buf );
         g_CurView()->MoveCursor( d_noarg.cursor.lin, d_noarg.cursor.col + 1 );
         break;

      case BOXARG: {
         const auto fBox(  d_cArg == 1 );
               auto xTemp( d_boxarg.flMin.col );
               auto yTemp( d_boxarg.flMin.lin );

         if( d_boxarg.flMin.col == d_boxarg.flMax.col ) { // VERTICAL LINE?
            buf = GetMinLine( &xb, yTemp, xTemp, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[EW]) && fBox) ? pchBox[NN] : pchBox[NS];
            g_CurFBuf()->PutLine( yTemp++, buf );
            while( yTemp < d_boxarg.flMax.lin ) {
               buf = GetMinLine( &xb, yTemp, xTemp, g_CurFBuf() );
               buf[xTemp] = ((buf[xTemp]==pchBox[EW]) && fBox) ? pchBox[CE] : pchBox[NS];
               g_CurFBuf()->PutLine( yTemp++, buf );
               }
            buf = GetMinLine( &xb, yTemp, xTemp, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[EW]) && fBox) ? pchBox[SS] : pchBox[NS];
            g_CurFBuf()->PutLine( yTemp, buf );
            }
         else if( d_boxarg.flMin.lin == d_boxarg.flMax.lin ) { // HORIZONTAL LINE?
            buf = GetMinLine( &xb, d_boxarg.flMin.lin, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[WW] : pchBox[EW];    ++xTemp;
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[CE] : pchBox[EW]; ++xTemp;
               }
            buf[xTemp] = ((buf[xTemp] == pchBox[NS]) && fBox) ? pchBox[EE] : pchBox[EW];    ++xTemp;
            g_CurFBuf()->PutLine( d_boxarg.flMin.lin, buf );
            }
         else { // BOX
            // TOP LINE
            //
            buf = GetMinLine( &xb, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp++] = pchBox[NW];
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp++] = pchBox[EW];
               }
            buf[xTemp++] = pchBox[NE];
            g_CurFBuf()->PutLine( yTemp++, buf );

            // MIDDLE LINE(S)
            //
            while( yTemp < d_boxarg.flMax.lin ) {
               xTemp = d_boxarg.flMin.col;
               buf = GetMinLine( &xb, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
               buf[xTemp] = pchBox[NS];
               for( ++xTemp ; xTemp < d_boxarg.flMax.col ; ++xTemp ) {
                  if( d_cArg > 1 )
                     buf[xTemp] = pchBox[CE];
                  }
               buf[xTemp] = pchBox[NS];
               g_CurFBuf()->PutLine( yTemp++, buf );
               }

            // BOTTOM LINE
            //
            xTemp = d_boxarg.flMin.col;
            buf = GetMinLine( &xb, yTemp, d_boxarg.flMax.col, g_CurFBuf() );
            buf[xTemp++] = pchBox[SW];
            while( xTemp < d_boxarg.flMax.col ) {
               buf[xTemp++] = pchBox[EW];
               }
            buf[xTemp] = pchBox[SE];
            g_CurFBuf()->PutLine( yTemp, buf );
            }
         }
         break;
      }
   return true;
   }

//=================================================================================================

// "library" function

int uint_log_10( int num ) {
   Assert( num >= 0 );
   // figure out min width of fixed-width field needed to hold line #
   //    poor-man's log10 function:  28-Mar-2001 klg
   auto rv( 1 );
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

bool ARG::vrepeat() {
   auto lx( d_boxarg.flMin.lin );
   std::string st = g_CurFBuf()->GetLineSeg( lx++, d_boxarg.flMin.col, d_boxarg.flMax.col ); // get line containing fill segment
   CPCChar inbuf( st.c_str() );
   0 && DBG( "fillseg [%d..%d] = '%s'", d_boxarg.flMin.col, d_boxarg.flMax.col, inbuf );

   const auto fInsertArg( d_cArg < 2 );

   if( !d_fMeta ) {
      for( ; lx <= d_boxarg.flMax.lin; ++lx )                       // each line in boxarg
         g_CurFBuf()->PutLineSeg( lx, inbuf, d_boxarg.flMin.col, d_boxarg.flMax.col, fInsertArg );
      }
   else {
      --lx;
      int width( d_boxarg.flMax.col - d_boxarg.flMin.col + 1 );
      0 && DBG( "%s+ width=%d", __func__, width );

      int    val;
      PCChar fmt;
      CPCChar pNum( StrPastAnyBlanks( inbuf ) );
      if( StrSpnSignedInt( pNum ) ) {
         width              -= pNum - inbuf;
         d_boxarg.flMin.col += pNum - inbuf;
         val = atoi( pNum );
         fmt = (pNum[0] == '0') ? "%0*d" : "%*d";
         }
      else {
         val = 1;
         fmt = "%0*d";
         }
      //
      // If largest number in series takes more digits than box is wide, insert
      // extra chars in box on each line (except first)
      //
      const auto minWidth( uint_log_10( val + (1 + d_boxarg.flMax.lin - d_boxarg.flMin.lin) ) );
      NoLessThan( &width, minWidth );

      auto fInsert( false );
      for( ; lx <= d_boxarg.flMax.lin; ++lx ) {                     // each line in boxarg
         g_CurFBuf()->PutLineSeg( lx, FmtStr<16>( fmt, width, val++ ), d_boxarg.flMin.col, d_boxarg.flMax.col, fInsert );
         fInsert = fInsertArg;
         }
      }

   return true;
   }

bool ARG::numlines () {
   const auto lwidth( uint_log_10( g_CurFBuf()->LineCount() ) );
   for( auto line(0); line < g_CurFBuf()->LineCount(); ++line ) {
      g_CurFBuf()->PutLineSeg( line, FmtStr<33>( "%*u ", lwidth, line+1 ), 0, 0, true ); // insert @ BoL
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
   if( !delfl )
      return fnMsg( "buffer '%s' doesn't exist", d_textarg.pText );

   auto curfile( g_CurFBuf() );
   delfl->MakeEmpty();
   delfl->MoveCursorToBofAllViews();   // BUGBUG this is wierd: port to Lua?
   delfl->PutFocusOn();              // BUGBUG this is wierd: port to Lua?
   g_CurView()->MoveCursor( 0, 0 );  // BUGBUG this is wierd: port to Lua?
                                     // BUGBUG this is wierd: port to Lua?
   curfile->PutFocusOn();
   return true;
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
   for( auto pC(d_textarg.pText) ; *pC ; ++pC )
      snprintf_full( &pB, &cbB, "0x%02X,", *pC );

   if( pB > buf )
       pB[-1] = '\0';

   g_pFbufClipboard->MakeEmpty();
   g_pFbufClipboard->PutLine( 0, buf );
   g_ClipboardType = BOXARG;

   Msg( "%s", buf );

   return true;
   }

#endif

//=================================================================================================

#if MACRO_BACKSLASH_ESCAPES

void StrUnDoubleBackslashes( PChar pszString ) {
   if( !pszString || *pszString == 0 )
      return;

   auto   pWr( pszString );
   PCChar pRd( pszString );

   do {
      if( (*pWr++ = *pRd++) == '\\' && *pRd == '\\' ) {
         ++pRd;
         }
      } while( *pRd != 0 );
   }

#endif

//-------------------------------------------------------------------------------------------------------

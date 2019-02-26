//
// Copyright 2015-2019 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
#include "stringlist.h"

//
// A single textArgBuf has a major disadvantage: macros which use any inline
// literal args overwrite the last USER arg.  Define SINGLE_TextArgBuffer to 0
// to remove this annoying behavior.
//
// <041220> klg  will do the same thing for SearchSpecifiers
//
// <041220> klg  this has trouble with macros that prompt the user: what user
//               types is not what ends up being supplied to the cmd executed.
//               So the transition from Interpreting() to !Interpreting()
//               needs to be replaced.  Need more research into internals, esp
//               GetTextargString() nuances.
//

#define  SINGLE_TextArgBuffer  1

#if SINGLE_TextArgBuffer
   STATIC_VAR std::string  s_textArgBuffer;
   STATIC_FXN std::string &TextArgBuffer() { return s_textArgBuffer; }
#else
   STATIC_VAR std::string  s_macroTextArgBuffer, s_userTextArgBuffer;
   STATIC_FXN std::string &TextArgBuffer() { return Interpreter::Interpreting() ? s_macroTextArgBuffer : s_userTextArgBuffer; }
#endif

STATIC_VAR Point s_SelAnchor;
STATIC_VAR Point s_SelEnd;

GLOBAL_VAR int      g_iArgCount; // write ONLY via Clr_g_ArgCount(), Inc_g_ArgCount(); read ONLY via Get_g_ArgCount()
STATIC_FXN int  Inc_g_ArgCount() { return ++g_iArgCount; }
STATIC_FXN void Clr_g_ArgCount() {          g_iArgCount = 0; }

void ClearArgAndSelection() { PCV;
   pcv->FBuf()->BlankAnnoDispSrcEdge( BlankDispSrc_SEL, false );
   pcv->FreeHiLiteRects();
   s_SelEnd.lin = -1;
   if( Get_g_ArgCount() > 0 ) {                                       0 && DBG( "%s+", __func__ );
   //      MoveCursor
      pcv->MoveCursor_NoUpdtWUC( s_SelAnchor.lin, s_SelAnchor.col );  0 && DBG( "%s-", __func__ );
      Clr_g_ArgCount();
      }
   }

GLOBAL_VAR bool g_fBoxMode = true;  // global/switchval

void ExtendSelectionHilite( const Point &pt ) { PCV;
   pcv->FBuf()->BlankAnnoDispSrcEdge( BlankDispSrc_SEL, true );
   pcv->FreeHiLiteRects();  // ###############################################################
   if( g_fBoxMode ) {
      Rect hilite;
      hilite.flMin.lin = std::min( s_SelAnchor.lin, pt.lin );
      hilite.flMax.lin = std::max( s_SelAnchor.lin, pt.lin );
      auto fLinesel( false );
      if( pt.col > s_SelAnchor.col ) {
         hilite.flMin.col = s_SelAnchor.col;
         hilite.flMax.col = pt         .col  - 1;
         }
      else {
         if(  pt.col == s_SelAnchor.col
           && pt.lin != s_SelAnchor.lin
           ) {
            fLinesel = true;
            hilite.flMin.col = 0;
            hilite.flMax.col = COL_MAX;
            }
         else {
            if( pt.col < s_SelAnchor.col ) {
               hilite.flMin.col = pt         .col;
               hilite.flMax.col = s_SelAnchor.col - 1;
               }
            else {
               hilite.flMin.col = s_SelAnchor.col;
               hilite.flMax.col = pt         .col;
               }
            }
         }
   // if( !Interpreter::Interpreting() ) // comment out so selword macro leaves updated dialog line
         {
         FixedCharArray<100> buf;
         if( fLinesel ) {
            buf.Sprintf( "Arg [%d]  %d lines [%d..%d]"
               , Get_g_ArgCount()
               , hilite.height()
               , hilite.flMin.lin + 1
               , hilite.flMax.lin + 1
               );
            }
         else {
            buf.Sprintf( "Arg [%d]  %dw x %dh box (%d,%d) (%d,%d)"
               , Get_g_ArgCount()
               , hilite.width()
               , hilite.height()
               , hilite.flMin.lin + 1
               , hilite.flMin.col + 1
               , hilite.flMax.lin + 1
               , hilite.flMax.col + 1
               );
            }
         DispRawDialogStr( buf.k_str() );
         }
      pcv->InsHiLiteBox( ColorTblIdx::SEL, hilite );  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      }
   else { // STREAM mode
      if( s_SelAnchor.lin == pt.lin ) { // 1-LINE STREAM
         pcv->InsHiLite1Line( ColorTblIdx::SEL, s_SelAnchor.lin, s_SelAnchor.col, pt.col + ((s_SelAnchor.col < pt.col) ? -1 : 0) );  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
         }
      else { // MULTILINE STREAM
         // redraw ANCHOR-line hilite
         pcv->InsHiLite1Line( ColorTblIdx::SEL, s_SelAnchor.lin, s_SelAnchor.col, (s_SelAnchor.lin <= pt.lin) ? COL_MAX : 0 );  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
         // redraw middle line(s) hilite, if any
         const auto yDelta( pt.lin - s_SelAnchor.lin );
         if( Abs(yDelta) > 1 ) {
            Rect hilite;
            hilite.flMin.lin = std::min( s_SelAnchor.lin, pt.lin ) + 1;
            hilite.flMax.lin = std::max( s_SelAnchor.lin, pt.lin ) - 1;
            hilite.flMin.col = 0;
            hilite.flMax.col = COL_MAX;
            pcv->InsHiLiteBox( ColorTblIdx::SEL, hilite );  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
            }
         // redraw CURSOR-line hilite
         COL xFirst, xLast;
         if( s_SelAnchor.lin > pt.lin ) { xFirst = COL_MAX;  xLast = pt.col    ; }
         else                           { xFirst = 0      ;  xLast = pt.col - 1; }
         pcv->InsHiLite1Line( ColorTblIdx::SEL, pt.lin, xFirst, xLast );  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
         }
      }
   s_SelEnd = pt;
   }

//--------------------------------------------------------------

bool ARG::bp() {
   int *pcrash( nullptr );
   *pcrash = 0; // intentional crash
   SW_BP;
   return true;
   }

bool ARG::cancel() {               0 && DBG( "%s+", __func__ );
   switch( d_argType ) {
      case NOARG: MsgClr();
                  break;
      default:    if( !Interpreter::Interpreting() ) {
                     fnMsg( "Argument cancelled" );
                     }
                  ClearArgAndSelection();
                  break;
      }
   if( !Interpreter::Interpreting() ) {
      DispNeedsRedrawVerticalCursorHilite();
      }
   FlushKeyQueuePrimeScreenRedraw();
   return true;
   }

PCChar ARG::ArgTypeName() const {  0 && DBG( "%s: %X", __func__, ActualArgType() );
   switch( ActualArgType() ) {
      default        : return "unknown";
      case NOARG     : return "NOARG";
      case NULLARG   : return "NULLARG";
      case TEXTARG   : return "TEXTARG";
      case LINEARG   : return "LINEARG";
      case STREAMARG : return "STREAMARG";
      case BOXARG    : return "BOXARG";
      }
   }

std::string ArgTypeNames( int argval ) {
   STATIC_CONST struct {
      int    mask;
      PCChar name;
      } tbl[] = {
         { NOARG      , "NOARG"      },
         { TEXTARG    , "TEXTARG"    },
         { NULLARG    , "NULLARG"    },
         {   NULLEOL  ,   "NULLEOL"  },
         {   NULLEOW  ,   "NULLEOW"  },
         { LINEARG    , "LINEARG"    },
         { BOXARG     , "BOXARG"     },
         { STREAMARG  , "STREAMARG"  },
         { NUMARG     , "NUMARG"     },
         { MARKARG    , "MARKARG"    },
         { BOXSTR     , "BOXSTR"     },
         { MODIFIES   , "MODIFIES"   },
         { KEEPMETA   , "KEEPMETA"   },
         { WINDOWFUNC , "WINDOWFUNC" },
         { CURSORFUNC , "CURSORFUNC" },
         { MACROFUNC  , "MACROFUNC"  },
      };
   std::string rv;
   BoolOneShot first;
   for( const auto &te : tbl ) {
      if( te.mask & argval ) {
         if( !first ) { rv.append( "+" ); }
         rv.append( te.name );
         }
      }
   return rv;
   }

bool ARG::BadArg() const {
   ClearArgAndSelection();
   return ErrorDialogBeepf( "'%s' Invalid Argument (%s)", CmdName(), ArgTypeName() );
   }

bool ARG::ErrPause( PCChar fmt, ...  ) const {
   SprintfBuf title( "'%s' %s", CmdName(), fmt );
   va_list args;
   va_start( args, fmt );
   VErrorDialogBeepf( title, args );
   va_end(args);
   return false;
   }

bool ARG::fnMsg( PCChar fmt, ... ) const { // like ::Msg(), but prefixes the name of the active EdFxn
   SprintfBuf title( "%s: %s", CmdName(), fmt );
   va_list args;
   va_start( args, fmt );
   VMsg( title, args );
   va_end(args);
   return false;
   }

// Those ARG::Xxx that interpret a selection arg as LINEARG|BOXARG or
// STREAMARG _regardless of the g_fBoxMode setting_ use
// ConvertLineOrBoxArgToStreamArg() and ConvertStreamargToLineargOrBoxarg() to
// get ARG in their preferred arg type:
//
void ARG::ConvertLineOrBoxArgToStreamArg() {
   if( d_argType == LINEARG ) {
      d_streamarg.flMin.lin = d_linearg.yMin;
      d_streamarg.flMax.lin = d_linearg.yMax;
      d_streamarg.flMin.col =
      d_streamarg.flMax.col = s_SelAnchor.col;
      }
   else {
      d_streamarg.flMin.lin = d_boxarg.flMin.lin;
      d_streamarg.flMax.lin = d_boxarg.flMax.lin;

      if( (  s_SelAnchor.lin == d_boxarg.flMin.lin
          && s_SelAnchor.col == d_boxarg.flMin.col
          ) ||
          (  s_SelAnchor.lin == d_boxarg.flMax.lin
          && (d_boxarg.flMax.col - s_SelAnchor.col == -1)
          )
        ) {
         d_streamarg.flMin.col = d_boxarg.flMin.col;
         d_streamarg.flMax.col = d_boxarg.flMax.col + 1;
         }
      else {
         d_streamarg.flMin.col = d_boxarg.flMax.col + 1;
         d_streamarg.flMax.col = d_boxarg.flMin.col;
         }
      }
   d_argType = STREAMARG;
   }

void ARG::ConvertStreamargToLineargOrBoxarg() {
   if( d_streamarg.flMin.col == d_streamarg.flMax.col ) {
      d_argType      = LINEARG;
      d_linearg.yMin = d_streamarg.flMin.lin;
      d_linearg.yMax = d_streamarg.flMax.lin;
      return;
      }

   d_argType          = BOXARG;
   d_boxarg.flMin.lin = d_streamarg.flMin.lin;
   d_boxarg.flMax.lin = d_streamarg.flMax.lin;

   if( d_streamarg.flMin.col > d_streamarg.flMax.col ) {
      d_boxarg.flMin.col = d_streamarg.flMax.col;
      d_boxarg.flMax.col = d_streamarg.flMin.col - 1;
      }
   else {
      d_boxarg.flMin.col = d_streamarg.flMin.col;
      d_boxarg.flMax.col = d_streamarg.flMax.col - 1;
      }
   }

void TermNulleow( std::string &st ) {
   for( auto it=st.begin()+1 ; it < st.end(); ++it ) {
      if( !isWordChar( *it ) ) {
         const auto idx( std::distance( st.begin(), it ) );
         st.erase( idx );
         break;
         }
      }
   }

bool GetSelectionLineColRange( LINE *yMin, LINE *yMax, COL *xMin, COL *xMax ) { // intended use: selection-smart CURSORFUNC's
   const auto Cursor( g_CurView()->Cursor() );
   if( Get_g_ArgCount() > 0 ) {
      *yMin = std::min( s_SelAnchor.lin, Cursor.lin );
      *yMax = std::max( s_SelAnchor.lin, Cursor.lin );
      *xMin = std::min( s_SelAnchor.col, Cursor.col );
      *xMax = std::max( s_SelAnchor.col, Cursor.col );
      return true;
      }
   else {
      *yMin = Cursor.lin;
      *yMax = Cursor.lin;
      *xMin = Cursor.col;
      *xMax = Cursor.col;
      return false;
      }
   }

stref View::GetBOXSTR_Selection() {
   if( this == g_CurView() ) {
      const auto cursor( Cursor() );
      if( Get_g_ArgCount() > 0 /* && s_SelAnchor.lin == cursor.lin */ ) {   0 && DBG("cur=%d,%d anchor=%d,%d",s_SelAnchor.lin,s_SelAnchor.col,cursor.lin,cursor.col);
         const auto xMin( std::min( s_SelAnchor.col, cursor.col ) );
         const auto xMax( std::max( s_SelAnchor.col, cursor.col ) );
         auto sr( FBuf()->PeekRawLineSeg( cursor.lin, xMin, xMax-1 ) );     0 && DBG("x:%d,%d '%" PR_BSR "'", xMin, xMax-1, BSR(sr) );
         trim( sr );
         return sr;
         }
      }
   return stref();
   }

bool ARG::BOXSTR_to_TEXTARG( LINE yOnly, COL xMin, COL xMax ) {
   d_pFBuf->DupLineSeg( TextArgBuffer(), yOnly, xMin, xMax-1 );
   d_argType         = TEXTARG;
   d_textarg.ulc.col = xMin;
   d_textarg.ulc.lin = yOnly;
   d_textarg.pText   = TextArgBuffer().c_str();                             0 && DBG( "BOXSTR='%s'", d_textarg.pText );
   return false;
   }

STATIC_VAR bool s_fHaveLiteralTextarg;

// consumes g_ArgCount, s_fHaveLiteralTextarg
bool ARG::IngestArgTextAndSelection() { enum {DB=0};                                          DB && DBG( "%s+", __func__ );
   // capture some global values into locals:
   const auto fHaveLiteralTextarg( s_fHaveLiteralTextarg );  s_fHaveLiteralTextarg = false;
   d_cArg = Get_g_ArgCount();                                Clr_g_ArgCount();
   d_pFBuf = g_CurFBuf();
   const auto Cursor( g_CurView()->Cursor() );
   if( d_cArg == 0 ) {
      if( d_pCmd->d_argType & NOARGWUC ) {
         auto start( Cursor );
         const auto wuc( GetWordUnderPoint( d_pFBuf, &start ) );
         if( !wuc.empty() ) {
            d_argType       = TEXTARG;
            d_textarg.ulc   = Cursor;
            TextArgBuffer().assign( wuc.data(), wuc.length() );
            d_textarg.pText = TextArgBuffer().c_str();                                        DB && DBG( "NOARGWUC='%s'", d_textarg.pText );
            return false; //==================================================================
            }
         }
      if( d_pCmd->d_argType & NOARG ) {
         d_argType      = NOARG;
         d_noarg.cursor = Cursor;                                                             DB && DBG( "%s NOARG", __func__ );
         return false; //=====================================================================
         }                                                                                    DB && DBG( "%s !NOARG", __func__ );
      return true; //=========================================================================
      }
   g_CurView()->MoveCursor_NoUpdtWUC( s_SelAnchor.lin, s_SelAnchor.col );
   auto NumArg_value(0);
   if( fHaveLiteralTextarg ) {
      if( (d_pCmd->d_argType & NUMARG) && StrSpnSignedInt( TextArgBuffer().c_str() ) ) {
         if( (NumArg_value = atoi( TextArgBuffer().c_str() )) ) {
            s_SelAnchor.lin = std::max( 0, s_SelAnchor.lin + NumArg_value + (NumArg_value > 0) ? (-1) : (+1) );
            }
         }
      else {
         FBufLocn locn;
         if( (d_pCmd->d_argType & MARKARG) && d_pFBuf->FindMark( TextArgBuffer().c_str(), &locn ) ) {
            s_SelAnchor = locn.Pt();                                                          DB && DBG( "FillArgStruct MarkFound '%s'", TextArgBuffer().c_str() );
            }
         else { // enum { DB=1 };
            if( d_pCmd->d_argType & TEXTARG ) {
               d_argType       = TEXTARG;
               d_textarg.ulc   = Cursor;
               d_textarg.pText = TextArgBuffer().c_str();                                     DB && DBG( "TEXTARG='%s'", d_textarg.pText );
               return false; //===============================================================
               }                                                                              DB && DBG( "%s !TEXTARG", __func__ );
            return true; //===================================================================
            }
         }
      }
   if( s_SelAnchor == Cursor && NumArg_value == 0 ) {
      if( d_pCmd->d_argType & (NULLEOL | NULLEOW) ) {
         d_pFBuf->DupLineSeg( TextArgBuffer(), s_SelAnchor.lin, s_SelAnchor.col, COL_MAX );
         if( d_pCmd->d_argType & NULLEOW ) { TermNulleow( TextArgBuffer() ); }
         d_argType       = TEXTARG;
         d_textarg.ulc   = Cursor;
         d_textarg.pText = TextArgBuffer().c_str();                                           DB && DBG( "NULLEO%c='%s'", (d_pCmd->d_argType & NULLEOW)?'W':'C', d_textarg.pText );
         return false; //=====================================================================
         }
      if( d_pCmd->d_argType & NULLARG ) {
         d_argType        = NULLARG;
         d_nullarg.cursor = Cursor;                                                           DB && DBG( "NULLARG" );
         return false; //=====================================================================
         }                                                                                    DB && DBG( "%s !NULLARG", __func__ );
      return true; //=========================================================================
      }
   const auto xMin( std::min( s_SelAnchor.col, Cursor.col ) );
   const auto xMax( std::max( s_SelAnchor.col, Cursor.col ) );
   const auto yMin( std::min( s_SelAnchor.lin, Cursor.lin ) );
   const auto yMax( std::max( s_SelAnchor.lin, Cursor.lin ) );
   if( (d_pCmd->d_argType & BOXSTR) && s_SelAnchor.lin == Cursor.lin ) {                      DB && DBG( "%s BOXSTR_to_TEXTARG", __func__ );
      return BOXSTR_to_TEXTARG( Cursor.lin, xMin, xMax ); //==================================
      }
   if( g_fBoxMode ) {
      if( (d_pCmd->d_argType & LINEARG) && s_SelAnchor.col == Cursor.col ) { // no movement in X (COL) direction
         d_argType      = LINEARG;
         d_linearg.yMin = yMin;
         d_linearg.yMax = yMax;                                                               DB && DBG( "LINEARG [%d..%d]", d_linearg.yMin, d_linearg.yMax );
         return false; //=====================================================================
         }
      if( (d_pCmd->d_argType & BOXARG) && s_SelAnchor.col != Cursor.col ) {
         d_argType          = BOXARG;
         d_boxarg.flMin.col = xMin;
         d_boxarg.flMin.lin = yMin;
         d_boxarg.flMax.col = xMax - 1; // subtract out the offset that's used to differentiate a BOXARG from a LINEARG
         d_boxarg.flMax.lin = yMax;                                                           DB && DBG( "BOXARG ulc=(%d,%d) lrc=(%d,%d)", d_boxarg.flMin.col, d_boxarg.flMin.lin, d_boxarg.flMax.col, d_boxarg.flMax.lin );
         return false; //=====================================================================
         }                                                                                    DB && DBG( "%s !SELARG: argType=%08X", __func__, d_pCmd->d_argType );
      return true; //=========================================================================
      }

   if( d_pCmd->d_argType & STREAMARG ) {
      //
      // STREAM definition:
      //
      // [(d_streamarg.flMin.col,d_streamarg.flMin.lin)..(d_streamarg.flMax.col,d_streamarg.flMax.lin))
      //
      // In English: a stream of text from start (inclusive) to end (NOT
      //             inclusive); THE LAST CHARACTER IS NOT INCLUDED
      //
      // This is the "stream" definition used by API's like DelStream and CopyStream
      //
      d_argType = STREAMARG;
      const auto fFwdSel( s_SelAnchor < Cursor );
      if( fFwdSel ) {
         d_streamarg.flMin = s_SelAnchor;
         d_streamarg.flMax = Cursor     ;
         }
      else {
         d_streamarg.flMin = Cursor     ;
         d_streamarg.flMax = s_SelAnchor;
         }                                                                                    DB && DBG( "stream (%d,%d), (%d,%d)", d_streamarg.flMin.lin, d_streamarg.flMin.col, d_streamarg.flMax.lin, d_streamarg.flMax.col );
      return false; //========================================================================
      }                                                                                       DB && DBG( "%s !ARG match", __func__ );
   return true; //============================================================================
   }

// trims leading and trailing blanks of each contributing line, ensures lines' contrib sare joined by ONE blank
std::string StreamArgToString( PFBUF pfb, Rect stream ) {
   std::string dest;
   const auto yMax( std::min( pfb->LastLine(), stream.flMax.lin ) );
   if( stream.flMin.lin > yMax ) {
      return dest;
      }
   auto append_dest = [&dest]( stref src ) {
      src.remove_prefix( FirstNonBlankOrEnd( src, 0 ) );
      if( !src.empty() ) {
         if( !dest.empty() ) { dest.append( " " ); }
         dest.append( src.data(), src.length() ); // <-- could be replaced by multi-blank-eater
         rmv_trail_blanks( dest );
         }
      };
   if( stream.flMin.lin == yMax ) {
      append_dest( pfb->PeekRawLineSeg( stream.flMin.lin, stream.flMin.col, stream.flMax.col-1 ) );
      }
   else {
      auto yLine( stream.flMin.lin );
      append_dest( pfb->PeekRawLineSeg( yLine, stream.flMin.col, COL_MAX ) );
      for( ++yLine ; yLine < yMax ; ++yLine ) {
         append_dest( pfb->PeekRawLine( yLine ) );
         }
      append_dest( pfb->PeekRawLineSeg( yLine, 0, stream.flMax.col-1 ) );
      }
   return dest;
   }

#ifdef fn_stream

bool ARG::stream() {  // test for StreamArgToString
   auto cArg(0);      // stream:alt+k
   switch( d_argType ) {
      default:        return BadArg();
      case STREAMARG: {
                      const auto ststr( StreamArgToString( g_CurFBuf(), d_streamarg ) );
                      DBG( "Stream:%s|", ststr.c_str() );
                      }
                      break;
      }
   return true;
   }

#endif

//-------------------------------

STATIC_FXN bool ConsumeMeta() {
   const auto fMetaWas( g_fMeta );
                        g_fMeta = false;
   if( fMetaWas != g_fMeta ) { DispNeedsRedrawStatLn(); }
   return fMetaWas;
   }

bool ARG::meta() {
   g_fMeta = !g_fMeta;
   DispNeedsRedrawStatLn();
   return g_fMeta;
   }

bool ARG::InitOk( PCCMD pCmd ) {
   d_pCmd  = pCmd;
   d_fMeta = (d_pCmd->d_argType & KEEPMETA) ? false : ConsumeMeta();
   d_cArg  = 0;
   d_argType      = NOARG;
   d_noarg.cursor = g_CurView()->Cursor();
   if( d_pCmd->d_argType & TAKES_ARG ) { // arg, meta, CURSORFUNC's will FAIL this test
      if( IngestArgTextAndSelection() ) {
         ClearArgAndSelection();
         return ErrorDialogBeepf( "Bad argument: '%s' requires %s", CmdName(), ArgTypeNames( d_pCmd->d_argType ).c_str() );
         }
      ClearArgAndSelection();
      }
   return true;
   }

bool ARG::Invoke() { 0 && DBG( "%s %s", FUNC, CmdName() );
   d_pCmd->IncrCallCount();
   // most of what follows is monitoring activity, not functionality-related...
   constexpr auto MONITOR_INVOCATION( 0 && DEBUG_LOGGING );
   STATIC_VAR int s_nestLevel;
   if( MONITOR_INVOCATION ) {
      ++s_nestLevel;
      enum { NEST_CHARS = 6 };
      const auto ixEos( s_nestLevel * NEST_CHARS );
      if( 0 && g_fDvlogcmds && !d_pCmd->isCursorOrWindowFunc() && ixEos < sizeof(linebuf)-1 ) {
         linebuf lbuf;
         for( auto ix=0 ; ix < ixEos ; ++ix ) { lbuf[ix] = '>'; }
         lbuf[ ixEos ] = '\0';                 DBG( "%s %-15s", lbuf, CmdName() );
         }
      }
   g_CurFBuf()->UndoInsertCmdAnnotation( d_pCmd );
   const auto rv( CALL_METHOD( *this, d_pCmd->d_func )() );
   MONITOR_INVOCATION && --s_nestLevel;
   return rv;
   }

STATIC_VAR ARG s_RepeatArg;

void ARG::SaveForRepeat() const {
   //--------  save new repeat-arg information  --------
   // (basically, this is the assignment operator for ARG)
   if( s_RepeatArg.d_argType == TEXTARG ) {
      Free0( s_RepeatArg.d_textarg.pText );
      }
   s_RepeatArg = *this;
   if( d_argType == TEXTARG ) {
      s_RepeatArg.d_textarg.pText = Strdup( d_textarg.pText );
      }
   }

bool ARG::repeat() {
   return s_RepeatArg.d_pCmd ? s_RepeatArg.Invoke() : fnMsg( "no command to repeat" );
   }

bool CMD::BuildExecute() const {  0 && DBG( "%s+ %s", __func__, Name() );
   if( (d_argType & MODIFIES) && g_CurFBuf()->CantModify() ) {
      ClearArgAndSelection();
      return false;
      }
   ARG argStruct;
   if( !argStruct.InitOk( this ) ) {
      return false;
      }
   if( IsCmdXeqInhibitedByRecord() && d_func != fn_record ) {
      return false;
      }
   if( d_func != fn_repeat && !Interpreter::Interpreting() ) {
      argStruct.SaveForRepeat();
      }
   return argStruct.Invoke();
   }

// local class that displays the prompt for GetTextargString, so this function can
// be embedded in the code that obtains the next CMD (a child of CMD_reader),
// but only if necessary (ie. if the USER actually needs to hit some keys).
//
class EditPrompt {
   const PCChar d_pszPrompt;
   const PCChar d_pszEditText;
   const COL    d_xCursor;
   const int    d_colorAttribute;
public:
   EditPrompt( PCChar pszPrompt, PCChar pszEditText, int colorAttribute, COL xCursor )
       : d_pszPrompt(pszPrompt)
       , d_pszEditText(pszEditText)
       , d_xCursor(xCursor)
       , d_colorAttribute(colorAttribute)
       { 0 && DBG( "%p %s: '%s'", this, __func__, d_pszPrompt );
       }
   void Write() const;
   void UnWrite() const;
   };

void EditPrompt::Write() const { 0 && DBG( "%p %s: '%s'", this, __func__, d_pszPrompt );
   const auto promptLen( Strlen( d_pszPrompt ) );
   auto oEditText( std::max( 0, d_xCursor - EditScreenCols() + promptLen + 1 ) );
   auto editTextLen( Strlen( d_pszEditText ) );
   if( oEditText > 0 ) {
      oEditText   -= oEditText % g_iHscroll;
      oEditText   += g_iHscroll;
      editTextLen -= oEditText;
      }
   {
   const auto editTextShown( std::min( EditScreenCols() - promptLen, editTextLen ) );
   VideoFlusher vf;
   VidWrStrColor( DialogLine(), 0        , d_pszPrompt            , promptLen    , d_colorAttribute, false );
   VidWrStrColor( DialogLine(), promptLen, d_pszEditText+oEditText, editTextShown, g_colorStatus   , false );
   if( promptLen + editTextShown < EditScreenCols() ) {
      VidWrStrColor( DialogLine(), promptLen+editTextShown, " "   , 1            , d_colorAttribute, true  );
      }
   }
   CursorLocnOutsideView_Set( DialogLine(), d_xCursor - oEditText + promptLen );
   }

void EditPrompt::UnWrite() const { CursorLocnOutsideView_Unset(); }

class GetTextargString_CMD_reader : public CMD_reader
   {
   const EditPrompt &d_ep;
protected:
   void VWritePrompt()   override { d_ep.Write  (); }
   void VUnWritePrompt() override { d_ep.UnWrite(); }
public:
   GetTextargString_CMD_reader( const EditPrompt &ep ) : d_ep( ep ) {}
   PCCMD GetNextCMD( bool fGetKbInput ); // OVERRIDE parent-class method
   };

PCCMD GetTextargString_CMD_reader::GetNextCMD( bool fKbInputOnly ) {
   if( fKbInputOnly ) {
      VWritePrompt();
      d_fAnyInputFromKbd = true;
      const auto rv( CmdFromKbdForExec() );
      VUnWritePrompt();
      return rv;
      }
   else { // VWritePrompt() called internal to GetNextCMD_ExpandAnyMacros if needed
      return GetNextCMD_ExpandAnyMacros( false );
      }
   }

STATIC_FXN void Bell_FlushKeyQueue_WaitForKey() {
   ConOut::Bell();
   ConIn::FlushKeyQueueAnythingFlushed();
   WaitForKey( 1 );
   }

GTS::eRV GTS::begline() {
   xCursor_ = 0;
   return KEEP_GOING;
   }
GTS::eRV GTS::home() { return begline(); }

GTS::eRV GTS::up() {
   if( textargStackPos_ < 0 ) {
      AddToTextargStack( stb_ );
      textargStackPos_ = 0;
      }
   if( textargStackPos_ < g_pFBufTextargStack->LastLine() ) {
      g_pFBufTextargStack->DupRawLine( stb_, ++textargStackPos_ );
      xCursor_ = stb_.length();
      }
   return KEEP_GOING;
   }
GTS::eRV GTS::down() {
   if( textargStackPos_ > 0 ) {
      g_pFBufTextargStack->DupRawLine( stb_, --textargStackPos_ );
      xCursor_ = stb_.length();
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::emacscdel() { // dup of cdelete
   if( xCursor_ > 0 ) {
      --xCursor_;
      if( xCursor_ < stb_.length() ) {
         stb_.erase( xCursor_, 1 );
         }
      }
   return KEEP_GOING;
   }
GTS::eRV GTS::cdelete() { return emacscdel(); }

GTS::eRV GTS::emacsnewl() { // dup of newline
   if( flags_ & gts_OnlyNewlAffirms ) {
      return DONE;
      }
   ConOut::Bell();
   return KEEP_GOING;
   }
GTS::eRV GTS::newline() { return emacsnewl(); }

GTS::eRV GTS::endline() {
   xCursor_ = stb_.length();  // past end
   return KEEP_GOING;
   }
GTS::eRV GTS::left() {
   if( xCursor_ > 0 ) {
      --xCursor_;
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::mword() {
   const auto pb( stb_.c_str() ); const auto len( stb_.length() );
   if( xCursor_ >= len ) {
      xCursor_ = len - 1;
      }
   while( xCursor_ > 0 ) {
      if( --xCursor_ == 0 ) {
         break;
         }
      if( !isWordChar( pb[xCursor_-1] ) && isWordChar( pb[xCursor_] ) ) {
         break;
         }
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::pword() {
   const auto pb( stb_.c_str() ); const auto len( stb_.length() );
   while( xCursor_ < len ) {
      ++xCursor_;
      if( !isWordChar( pb[xCursor_] ) && isWordChar( pb[xCursor_+1] ) ) {
         ++xCursor_;
         break;
         }
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::arg() {
   if( xCursor_ < stb_.length() ) {
      stb_.erase( xCursor_ );  // delete all chars at or following (under or to the right of) the cursor
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::restcur() {
   // alt+center=alg+arg: does the converse of arg:
   if( xCursor_ < stb_.length() ) {
      stb_.erase( 0, xCursor_ ); // delete all chars preceding (to the left of) the cursor
      xCursor_ = 0;
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::right() {  0 && DBG( "%s: %d, %" PR_SIZET, __func__, xCursor_, stb_.length() );
   if( g_CurFBuf() && stb_.length() == xCursor_ ) {
      const auto xx( xColInFile_ + xCursor_ );
      std::string stTmp;
      g_CurFBuf()->DupLineSeg( stTmp, g_CursorLine(), xx, xx );    0 && DBG( "%d='%" PR_BSR "'", xx, BSR(stTmp) );
      if( !stTmp.empty() ) {
         stb_.push_back( stTmp[0] );
         }
      }
   ++xCursor_;
   return KEEP_GOING;
   }

GTS::eRV GTS::delete_() {
   if( xCursor_ < stb_.length() ) {
      stb_.erase( xCursor_, 1 );
      }
   return KEEP_GOING;
   }
GTS::eRV GTS::sdelete() { return delete_(); }

GTS::eRV GTS::insert() {
   stb_.insert( xCursor_, 1, ' ' );
   return KEEP_GOING;
   }
GTS::eRV GTS::sinsert() { return insert(); }

GTS::eRV GTS::flipcase() {
   if( xCursor_ < stb_.length() ) {
      stb_[ xCursor_ ] = FlipCase( stb_[ xCursor_ ] );
      }
   return KEEP_GOING;
   }

GTS::eRV GTS::meta() {
   noargNoMeta.meta();
   return KEEP_GOING;
   }

GTS::eRV GTS::cancel() {
   return DONE;
   }

GTS::eRV GTS::graphic() { // !!! called by macro_graphic !!!
   if( fInitialStringSelected_ ) {
      xCursor_ = 0;
      if( xCursor_ < stb_.length() ) {
         stb_.erase( xCursor_ );
         }
      }                                                            0 && DBG( "graphic @ x=%d (stlen=%" PR_SIZET ")", xCursor_, stb_.length() );
   if( xCursor_ > stb_.length() ) { 0 && DBG( "append %" PR_SIZET " spaces", xCursor_ - stb_.length() );
      stb_.append( xCursor_ - stb_.length(), ' ' );
      }
   const auto ch( pCmd_->d_argData.chAscii() );
   stb_.insert( xCursor_++, 1, ch );
   return KEEP_GOING;
   }

#ifdef fn_dispmstk
GTS::eRV GTS::dispmstk() {
   noargNoMeta.dispmstk();
   return KEEP_GOING;
   }
#endif

// BUGBUG
// GTS::eRV GTS::up() {
//    else if( pCmd_->NameMatch( "swapchar" ) ) {
//       if( xCursor_+1 < stb_.length() ) {
//          std::swap( stb_[xCursor_+0], stb_[xCursor_+1] );
//          }
//       return KEEP_GOING;
//       }

class TabCompletion_filesystem {
   bool        fBellAndFreezeKbInput = false;
   DirMatches *pDirContent = nullptr;
   std::string pbTabxBase;
public:
   TabCompletion_filesystem() {}
   ~TabCompletion_filesystem() {
      Delete0( pDirContent );
      }
   void Deactivate() {
      Delete0( pDirContent );
      pbTabxBase.clear(); // forget prev used WC
      }
   void GetNext( std::string &stb, COL &xCursor ) {
      if( !pDirContent ) {
         if( pbTabxBase.empty() ) { // no prev'ly used WC?
            pbTabxBase = stb;   // create based on curr content
            }
         pDirContent = new DirMatches( pbTabxBase.c_str(), HasWildcard( pbTabxBase ) ? nullptr : "*", FILES_AND_DIRS, false );
         }
      Path::str_t nxt;
      do {
         nxt = pDirContent->GetNext();
         } while( Path::IsDotOrDotDot( nxt ) );
      if( !nxt.empty() ) {
         stb = nxt;
         xCursor = stb.length();  // past end
         }
      else {
         Delete0( pDirContent );
         stb = pbTabxBase;
         xCursor = ixFirstWildcardOrEos( stb ); // show user seed in case he wants to edit or iterate again thru WC expansion loop
         fBellAndFreezeKbInput = true;
         }
      }
   void BellAndFreezeKbInputIfExhausted() {
      // BUGBUG GetTextargString_CMD_reader may prevent the following
      // fBellAndFreezeKbInput code from achieving it's intended task
      //
      if( fBellAndFreezeKbInput ) {
         // goal is to freeze the KB input so if user holds tab down, he
         // can easily get back to the display of pbTabxBase.
         //
         // fBellAndFreezeKbInput exists because where fBellAndFreezeKbInput is
         // set is temporarily prior to RefreshPromptAndEditInput being called;
         // freezing there shows the old dialog line content, which is not
         // useful; instead defer the freeze to here, when the latest dialog
         // line content has actually been displayed.
         //
         fBellAndFreezeKbInput = false;
         Bell_FlushKeyQueue_WaitForKey();
         }
      }
   };

STATIC_FXN PCCMD GetTextargString_( std::string &stb, PCChar pszPrompt, int xCursor, PCCMD pCmd, int flags, bool *pfGotAnyInputFromKbd ) {
   // pCmd if valid (currently only when we're called by ArgMainLoop) will be ARG::graphic, the first char of a typed arg.
   enum { DBG_GTA=1 };                                                DBG_GTA && DBG( "+%s CMD='%s' dest='%s' flags=%X prompt='%s'", __func__, pCmd?pCmd->Name():"(none)", stb.c_str(), flags, pszPrompt?pszPrompt:"" );
   const auto fSavedMeta( g_fMeta );                                  0 && DBG( "%s+ g_fMeta=%d, fSavedMeta=%d", __func__, g_fMeta, fSavedMeta );
   const auto xColInFile( pCmd ? s_SelAnchor.col : g_CursorCol() );   0 && DBG( "%s+ xColInFile=%d (%d : %d)", __func__, xColInFile, s_SelAnchor.col, g_CursorCol() );
   *pfGotAnyInputFromKbd = false;
   auto textargStackPos( -1 );
   TabCompletion_filesystem tcf;
   while(1) { //******************************************************************
      tcf.BellAndFreezeKbInputIfExhausted();
      const auto fInitialStringSelected( ToBOOL(flags & gts_DfltResponse) );
      if( !pCmd ) {
         EditPrompt ep( pszPrompt, stb.c_str(), fInitialStringSelected ? g_colorError : g_colorInfo, xCursor );
         GetTextargString_CMD_reader gtas( ep );
         pCmd = gtas.GetNextCMD( ToBOOL(flags & gts_fKbInputOnly) );
         if( !pCmd ) {
            break;
            }
         if( gtas.GotAnyInputFromKbd() ) {
            *pfGotAnyInputFromKbd = true;
            }
         }
      // process pCmd
      if( pCmd->d_argData.eka.EdKcEnum == EdKC_tab ) { // 20100222 hack: look at EdKcEnum since new tab key assignment is to a Lua function
         tcf.GetNext( stb, xCursor );  // tab-handling
         }
      else {
         tcf.Deactivate();
         if( pCmd->d_GTS_fxn ) {
            GTS gts = { // yes, this statement relies on GCC-only features <sigh>
               .xCursor_                = xCursor     ,
               .stb_                    = stb         ,
               .textargStackPos_        = textargStackPos,
               //-------------------------------- ref/const-value boundary
               .pCmd_                   = pCmd         ,
               .xColInFile_             = xColInFile   ,
               .flags_                  = flags        ,
               .fInitialStringSelected_ = fInitialStringSelected,
               };
            const auto rv( CALL_METHOD( gts, pCmd->d_GTS_fxn )() );
            if( rv == GTS::DONE ) {
               break;
               }
            }
         else {
            if( !pCmd->isCursorFunc() && !(flags & gts_OnlyNewlAffirms) ) {
               break;
               }
            ConOut::Bell();
            }
         }
      // Some editing or cursor movement was done and we will be continuing to edit.
      // Consume meta + pCmd
      if( !(pCmd->d_argType & KEEPMETA) ) {
         g_fMeta = false;
         }
      pCmd = nullptr;
      flags &= ~gts_DfltResponse;
      } /*** while ***/                                               DBG_GTA && DBG( "-%s CMD='%s' arg='%s'" , __func__ , pCmd?pCmd->Name():"" , stb.c_str() );
   g_fMeta = fSavedMeta;
   if( *pfGotAnyInputFromKbd ) {
      ViewCursorRestorer cr;
      }                                                               0 && DBG( "%s- g_fMeta=%d, fSavedMeta=%d", __func__, g_fMeta, fSavedMeta );
   return pCmd;
   }

PCCMD GetTextargString( std::string &dest, PCChar pszPrompt, int xCursor, PCCMD pCmd, int flags, bool *pfGotAnyInputFromKbd ) {
   // catch (& hide) std::string-related exceptions in GetTextargString_
   try {
      return GetTextargString_( dest, pszPrompt, xCursor, pCmd, flags, pfGotAnyInputFromKbd );
      }
   catch( const std::out_of_range& exc ) {
      Msg( "%s caught std::out_of_range %s", __func__, exc.what() );
      }
   catch( ... ) {
      Msg( "%s caught other exception", __func__ );
      }
   Bell_FlushKeyQueue_WaitForKey();
   return nullptr;
   }

//--------------------------------------------------------------

GLOBAL_VAR bool g_fSelectionActive; // read by IsSelectionActive(), which is used by mouse code

#ifdef fn_selkeymaptogl
bool ARG::selkeymaptogl() {
   return SelKeymapToggle();
   }
#endif

STATIC_FXN bool CollectTextOrSelectArg_Execute() { // Called on first invocation (i.e. when Get_g_ArgCount()==0) of ARG::arg or ARG::Lastselect.
   ExtendSelectionHilite( g_Cursor() ); // candidate for removal: IncArgCnt_DropAnchor() (called by ARG::arg()) already does this?
   g_fSelectionActive = true;           // move to IncArgCnt_DropAnchor()?
   while( auto pCmd = CMD_reader().GetNextCMD() ) {
      if(   pCmd->d_func == fn_arg
       #ifdef fn_argselkeymap
         || pCmd->d_func == fn_argselkeymap
       #endif
        ) { // ARG::arg* _IS NOT CALLED_: instead inline-execute here:
         Inc_g_ArgCount();
         ExtendSelectionHilite( g_Cursor() ); // selection hilite has not changed, however status line (displaying arg-count) must be updated
         continue; //================================================================
         }
      if( pCmd->isCursorFunc() || pCmd->d_func == fn_meta ) {
         // fn_meta and all CURSORFUNC's are called w/o ARG buildup and may alter the selection state
         g_fFuncRetVal = pCmd->BuildExecute();
         ExtendSelectionHilite( g_Cursor() );
         continue; //================================================================
         }
      // We HAVE a valid CMD that is not arg, meta, or a CURSORFUNC
      // We SHALL call pCmd->BuildExecute() and return from this function
      g_fSelectionActive = false; // this fn is consuming the selection
      if( g_Cursor() == s_SelAnchor ) { // no selection in effect? (i.e. is NULLARG)
         if( pCmd->IsFnGraphic() ) {    // user typed a literal char?
            // Feed literal char embedded in CMD["graphic"] (pCmd) into GetTextargString
            TextArgBuffer().clear(); // arg NULLARG graphic starts with an empty TEXTARG buffer
            bool fGotAnyInputFromKbd;
            pCmd = GetTextargString( TextArgBuffer(), FmtStr<20>( "Arg [%d]? ", Get_g_ArgCount() ), 0, pCmd, 0, &fGotAnyInputFromKbd );
            if( !pCmd ) { // DO NOT filter-out 'cancel' here; needs to go thru remainder of ARG buildup so that 'lasttext' works
               return false; //*********************************************************
               }
            s_fHaveLiteralTextarg = true;
            if( fGotAnyInputFromKbd ) {
               AddToTextargStack( TextArgBuffer() );
               }
            // a valid CMD[?] (pCmd) was invoked by user to exit GetTextargString; execute it on TEXTARG
            // (IngestArgTextAndSelection() grabs TEXTARG string from TextArgBuffer() if s_fHaveLiteralTextarg).
            }
         }
      else { PCV; // about to invoke pCmd on "selection in effect"; save the latter for ARG::lastselect()
         pcv->d_LastSelectAnchor   = s_SelAnchor;
         pcv->d_LastSelectCursor   = g_Cursor();
         pcv->d_LastSelect_isValid = true;
         }
      return pCmd->BuildExecute(); //************************************************
      }
   return false;
   }

STATIC_FXN void IncArgCnt_DropAnchor() {
   if( Inc_g_ArgCount() == 1 ) { // was 0, now 1?
      s_SelAnchor = g_Cursor();
      ExtendSelectionHilite( Point( s_SelAnchor, 0, 1 ) );
      }
   }

bool ARG::arg() { // can only be called with ...
   Assert( Get_g_ArgCount() == 0 );
   IncArgCnt_DropAnchor();
   return CollectTextOrSelectArg_Execute();
   }

#ifdef fn_argselkeymap
bool ARG::argselkeymap() {
   SelKeymapEnable();
   return arg();
   }
#endif

bool ARG::lastselect() {
   if( Get_g_ArgCount() > 0 ) {
      return false;
      }
   PCV;
   if( !pcv->d_LastSelect_isValid ) {
      return Msg( "view has no previous selection" );
      }
   s_SelAnchor = pcv->d_LastSelectAnchor;
   pcv->MoveCursor( pcv->d_LastSelectCursor );
   Inc_g_ArgCount();
   CollectTextOrSelectArg_Execute();
   return true;
   }

//
//  fExecute - execute strToExecute, an editor macro definition.
//             Does NOT return until execution of strToExecute is complete (or
//             an error occurs).
//
//  *** IF strToExecute is modified by the execution of this macro (say, it's a
//  *** static buffer like TextArgBuffer()), BAD THINGS _WILL_ HAPPEN!
//
//  strToExecute can contain various types of arguments, macros, and editor
//  functions.  If text arguments are included in strToExecute, they must be
//  enclosed in double quotation marks preceded by a backslash(\").
//
//  return value is that of the last-executed function.
//

GLOBAL_VAR int g_fExecutingInternal; // to support correct recording-to-macro

bool fExecute( PCChar strToExecute, bool fInternalExec ) { 0 && DBG( "%s '%s'", __func__, strToExecute );
   if( fInternalExec ) { ++g_fExecutingInternal; }
   if( Interpreter::PushMacroStringOk( strToExecute, Interpreter::breakOutHere ) ) {
      FetchAndExecuteCMDs( false );
      }
   if( fInternalExec ) { --g_fExecutingInternal; }
   return g_fFuncRetVal;
   }

STATIC_FXN bool GetTextargStringNXeq( std::string &str, int cArg, COL xCursor ) {
   for( auto ix(0) ; ix < cArg ; ++ix ) {
      IncArgCnt_DropAnchor();
      }
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( str, FmtStr<25>( "Arg [%d]: ", Get_g_ArgCount() ), xCursor, nullptr, gts_DfltResponse, &fGotAnyInputFromKbd ) );
   if( !pCmd ) { // DO NOT filter-out 'cancel' here; needs to go thru remainder of ARG buildup so that 'lasttext' works
      return false;
      }
   s_fHaveLiteralTextarg = true;
   if( fGotAnyInputFromKbd ) {
      AddToTextargStack( str );
      }
   return pCmd->BuildExecute();
   }

#ifdef fn_cliptext
bool ARG::cliptext() { // patterned after lasttext
   auto cArg(0);
   switch( d_argType ) {
      default:        return BadArg();
      case NULLARG:   cArg = 1 + d_cArg;  break;
      case NOARG:     cArg = 1;           break;
      }
   WinClipGetFirstLine( TextArgBuffer() );
   return GetTextargStringNXeq( TextArgBuffer(), cArg, 0 );
   }
#endif

bool ARG::lasttext() {
   auto cArg(0);
   switch( d_argType ) {
      default:        return BadArg();
      case NULLARG:   cArg = 1 + d_cArg;  break;
      case NOARG:     cArg = 1;           break;
      case LINEARG:   cArg = d_cArg; g_CurFBuf()->DupLineSeg( TextArgBuffer(), d_linearg.yMin, 0, COL_MAX );
                      break;
      case BOXARG:    cArg = d_cArg; g_CurFBuf()->DupLineSeg( TextArgBuffer(), d_boxarg.flMin.lin, d_boxarg.flMin.col, d_boxarg.flMax.col );
                      break;
      case STREAMARG: cArg = d_cArg; TextArgBuffer() = StreamArgToString( g_CurFBuf(), d_streamarg );
                      break;
      }
   return GetTextargStringNXeq( TextArgBuffer(), cArg, TextArgBuffer().length() );
   }

bool ARG::prompt() {
   switch( d_argType ) {
      default:        return BadArg();
      case TEXTARG:   break;
      }
   std::string src( d_textarg.pText );
   for( auto ix(0) ; ix < d_cArg ; ++ix ) {
      IncArgCnt_DropAnchor();
      }
   TextArgBuffer().clear();
   bool fGotAnyInputFromKbd;
   const auto pCmd( GetTextargString( TextArgBuffer(), src.c_str(), 0, nullptr, gts_fKbInputOnly+gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
   if( !pCmd || pCmd->IsFnCancel() ) {
      return false;
      }
   s_fHaveLiteralTextarg = true;
   if( fGotAnyInputFromKbd ) {
      AddToTextargStack( TextArgBuffer() );
      }
   return true;
   }

#ifdef fn_selcmd
//
// selcmd - accept a selection argument, then prompt the user for the command
//          name, find the command, and invoke it on the selection argument.
//
// Why this exists:
//
//    There are a finite number of keys to assign (bind) functions to.  And the
//    deeper you go into the pool, the harder it is to remember the binding of
//    keys on-demand (this is probably a function of the more unusual keys being
//    seldom-used).
//
//    BUT!  Functions that accept selection or text arguments ARE USELESS
//    (cannot accept those arguments) _IF they are NOT BOUND_; ARG::execute is the
//    only way to execute unbound functions, and ARG::execute itself consumes
//    (because it USES) any selection argument passed to it.
//
//    So I'd like a way to invoke functions by name (most importantly
//    interactively), while having _the invoked function_ receive the selection
//    argument.  All I would have to remember is the _name_ of the function, not
//    the key I assigned it to weeks ago (unless it was never assigned to any
//    key, in which case I have to find some unused key to bind it to NOW,
//    hopelessly breaking my train of thought)
//
//    NOTE: 'selcmd' invocations ARE recordable/replayable!
//
//    NOTE: There is no reason to use 'selcmd' in a non-recorded macro.
//
//
//  Implementation PROBLEM #1:
//
//     Selection highlight is turned off when user is prompted to "Enter cmd
//     name".  This is cosmetic, but would be nice to fix, maybe via a new
//     CMD::d_argType attribute SHOW_SELN_TIL_DONE?
//
//  Implementation LIMITATION #2:
//
//     arg boxarg selcmd "makebox"
//
//        >>>>>> arg
//        >>>>>>>>>>>> selcmd
//        >>>>>>>>>>>>>>>>>> makebox
//
//     effective ARG of makebox is boxarg of selcmd (exactly our goal!)
//
//     ---------------
//
//     arg "mb:=makebox" assign
//     arg boxarg selcmd "mb"
//
//        >>>>>> arg
//        >>>>>>>>>>>> selcmd
//        >>>>>>>>>>>>>>>>>> mb
//        >>>>>> makebox
//
//     effective ARG of makebox is NOARG (boo!)
//
//     Diagnosis:
//
//      - In the functioning case (arg boxarg selcmd "makebox"), selection-arg
//        is consumed by 'selcmd', and handed from 'selcmd' to 'makebox' via
//        ARG::Invoke(); control DOES NOT return to the interpreter in between
//        'selcmd' and 'makebox' fn calls.
//
//      - In the NON-functioning case, control DOES return to the interpreter in
//        between 'selcmd' and 'makebox' fn calls: what selcmd Invoke()s is
//        RunMacro (which really only _pushes_ the macro's expanded text into
//        the interpreter pipe and returns to Invoke()).  The interpreter
//        subsequently parses and invokes 'makebox', but the selection-arg
//        context of selcmd has long since disappeared.
//
//     For now I have ruled "Implementation LIMITATION #2" a "non-problem",
//     since the whole purpose of selcmd is to invoke functions that accept
//     selection args, and no macros can fit that category.
//
//     The reservation I have about permanently accepting this limitation is
//     exactly the failing test case; often, I use (2-letter) macro
//     abbreviations for longer-/descriptively-named functions, to make it easy
//     to type them interactively into ARG::execute.
//
//      - An extension which renders this less a limitation would be to offer
//        tab-completion for functions (matching only those functions that
//        support the pending argType?).  This would be a generalization of
//        GTA's current filename tab-completion facility (would need
//        polymorphic class tree to impl), and the irony is that, for
//        "reverse-polish" command processing (our claim to fame), such
//        polymorphic tab-completion is USELESS (since there is now only one
//        tab-completion facility, it executes even if the arg context has
//        nothing to do with filenames, which is fairly lame).  Ah, the
//        challenges of purist design!
//
//      - a kludge which would keep everyone happy (except perhaps KISS'ers like
//        me) would be the addition of a new "lexical entity" possibly called a
//        "function alias"; this would be a CMD which was a copy of the CMD to
//        which it referred (same function pointer, same argType), but with a
//        different name.  The challenge is, how are these created?
//
//           Initial idea: at DefineMacro time, if defn string parses to a
//           single token which can be resolved to a non-macro CMD, then add a
//           renamed dup of this CMD to the macro heap.  However, this violates
//           the 'deferred-evaluation of macros' rule which governs the behavior
//           of macros...
//
//      - or how about a popup TUI menu containing all matches?
//
// 20050912 klg Initial 90% implementation
//
// 20050913 klg Devised the BuildXeqArgCmdStr solution to "Implementation LIMITATION #2".
//              Also added TEXTARG support.
//
STATIC_FXN bool BuildXeqArgCmdStr( bool fMeta, int cArg, PCChar cmdName, PCChar pTextarg ) {
   linebuf lbuf; lbuf[0] = '\0'; auto pLbuf( lbuf ); auto lbufBytes( sizeof lbuf );
   if( !pTextarg ) {
      snprintf_full( &pLbuf, &lbufBytes, "lastselect " );
      --cArg;
      }
   NoMoreThan( &cArg, 9 );
   for( auto ix(0); ix < cArg ; ++ix ) { snprintf_full( &pLbuf, &lbufBytes, "arg " );              }
   if( fMeta )                         { snprintf_full( &pLbuf, &lbufBytes, "meta " );             }
   if( pTextarg )                      { snprintf_full( &pLbuf, &lbufBytes, "\"%s\" ", pTextarg ); }
   snprintf_full( &pLbuf, &lbufBytes, "%s", cmdName );
   Msg( "running '%s'", lbuf );
   return fExecute( lbuf );
   // COULD NOT have used RunCmdTextarg here since CmdFromName( cmdName )->func may be ARG::macro!
   }

bool ARG::selcmd() { // selcmd:alt+0
                     // mb:=makebox
   0 && DBG( "%s [0] d_argType=0x%X", __func__, d_argType );
   std::string cmdNameBuf;
   cmdNameBuf.assign("");
   FmtStr<40> prompt_( "cmd name? [%d] ", d_cArg );
   while( 1 ) {
      bool fGotAnyInputFromKbd;
      const auto GtaTermCmd( GetTextargString( cmdNameBuf, prompt_, 0, nullptr, gts_OnlyNewlAffirms, &fGotAnyInputFromKbd ) );
      if( !GtaTermCmd || GtaTermCmd->IsFnCancel() ) {
         return fnMsg( "cancelled" );
         }
      if( fGotAnyInputFromKbd ) {
         AddToTextargStack( cmdNameBuf );
         }
      const PCCMD newCmd( CmdFromName( cmdNameBuf ) );
      if( !newCmd ) {
         fnMsg( "'%s' is unknown cmd", cmdNameBuf.c_str() );
         continue;
         }
      0 && DBG( "%s [1] d_argType=0x%X", __func__, d_argType );
      0 && DBG( "%s [2] CMD=%s, d_argType=0x%X", __func__, cmdNameBuf.c_str(), newCmd->d_argType );
      const auto box2str( d_argType == BOXARG && (newCmd->d_argType & BOXSTR) && d_boxarg.flMin.lin == d_boxarg.flMax.lin );
      0 && DBG( "%s [3] boxs=%d %d %d", __func__, box2str, d_boxarg.flMin.lin, d_boxarg.flMax.lin );
      if( (newCmd->d_argType & d_argType) || box2str ) {
         d_pCmd = newCmd; // DO THIS BEFORE CALLING Invoke() !!!!!!!!!!!!!!!!!!!!!!
         //
         // special handling to convert BOXARG into TEXTARG: needed because the
         // CMD for which the ARG (this) was constructed is selcmd, while the
         // command being executed by selcmd (here) is the newCmd obtained above.
         // In order to allow it to work with any user selection argument (which
         // should be passed on to newCmd), selcmd takes (selcmd->d_argType) pretty
         // much any possible ARG type, but BOXSTR is an optional mode which not
         // all pCmds will support/request, thus BOXSTR is not set in
         // selcmd->d_argType, thus automatic conversion from BOXARG -> TEXTARG
         // is not done by core ARG creation code like it otherwise would be.
         //
         // So we do it here...
         //
         if( box2str ) {
            BOXSTR_to_TEXTARG( d_boxarg.flMin.lin, d_boxarg.flMin.col, d_boxarg.flMax.col+1 );
            }
         0 && DBG( "%s CMD '%s'", __func__, newCmd->Name() );
         return Invoke();
         }
      if( 0 ) {
         //
         // if selcmd were to call Invoke() on the macro CMD as above, what selcmd
         // Invoke()s is RunMacro (which really only _pushes_ the macro's
         // expanded text ("makebox") into the interpreter pipe and returns to
         // Invoke(), which returns here.  We then we return, and the interpreter
         // subsequently parses 'makebox' from its pipe and Invoke()s it, but the
         // selection-arg context of this selcmd has long since disappeared.
         //
         // The solution is to use fExecute to invoke the macro a w/reconstructed
         // arg:
         //
         // 20091215 kgoodwin all of this now seems completely superfluous
         //
         switch( d_argType ) {
            default:         return fnMsg( "bad arg for '%s'", newCmd->Name() );
            case BOXARG:     ATTR_FALLTHRU;
            case LINEARG:    ATTR_FALLTHRU;
            case STREAMARG:  return BuildXeqArgCmdStr( d_fMeta, d_cArg, cmdNameBuf.c_str(), nullptr );
            case TEXTARG:    return BuildXeqArgCmdStr( d_fMeta, d_cArg, cmdNameBuf.c_str(), d_textarg.pText );
            }
         }
      return fnMsg( "%s: unsupported arg for '%s'", __func__, newCmd->Name() );
      }
   }

#endif // fn_selcmd

//#############################################################################

bool ARG::unassigned() {
   return ErrorDialogBeepf( "%s is not assigned to any editor function", KeyNmOfEdkc( d_pCmd->d_argData.eka.EdKcEnum ).c_str() );
   }

bool ARG::boxstream() {
   g_fBoxMode = !g_fBoxMode;
   if( Get_g_ArgCount() > 0 ) {
      ExtendSelectionHilite( g_CurView()->Cursor() );
      }
   return g_fBoxMode;
   }

//#############################################################################

int ARG::GetLineRange( LINE *pyMin, LINE *pyMax ) const {
   switch( d_argType ) {
      default:        return 1; // NOT OK
      case NOARG:     *pyMin = 0;
                      *pyMax = g_CurFBuf()->LastLine();
                      return 0; // OK
      case NULLARG:   ATTR_FALLTHRU;
      case NULLEOL:   ATTR_FALLTHRU;
      case NULLEOW:   *pyMin = d_nullarg.cursor.lin;
                      *pyMax = d_nullarg.cursor.lin;
                      return 0; // OK
      case BOXARG:    *pyMin = d_boxarg.flMin.lin;
                      *pyMax = d_boxarg.flMax.lin;
                      return 0; // OK
      case LINEARG:   *pyMin = d_linearg.yMin;
                      *pyMax = d_linearg.yMax;
                      return 0; // OK
      case STREAMARG: *pyMin = d_streamarg.flMin.lin;
                      *pyMax = d_streamarg.flMax.lin;
                      return 0; // OK
      }
   }

void ARG::BeginPt( Point *pPt ) const {
   switch( d_argType ) {
    case LINEARG:   pPt->Set( d_linearg.yMin       , 0                     ); break;
    case BOXARG:    pPt->Set( d_boxarg.flMin.lin   , d_boxarg.flMin.col    ); break;
    case STREAMARG: pPt->Set( d_streamarg.flMin.lin, d_streamarg.flMin.col ); break;
    default:        pPt->Set( g_CursorLine()       , g_CursorCol()         ); break;
    }               0 && DBG( "%s=(%d,%d)", __func__, pPt->lin, pPt->col );
   }

void ARG::EndPt( Point *pPt ) const {
   switch( d_argType ) {
    case LINEARG:   pPt->Set( d_linearg.yMax       , COL_MAX               ); break;
    case BOXARG:    pPt->Set( d_boxarg.flMax.lin   , d_boxarg.flMax.col    ); break;
    case STREAMARG: pPt->Set( d_streamarg.flMax.lin, d_streamarg.flMax.col ); break;
    default:        pPt->Set( g_CursorLine()       , g_CursorCol()         ); break;
    }               0 && DBG( "%s=(%d,%d)", __func__, pPt->lin, pPt->col );
   }

bool ARG::Beyond( const Point &pt ) const { // Beyond() DOES NOT pay attention to the x dimension (COLumn), while Within() DOES
   switch( d_argType ) {
    case LINEARG:   return pt.lin >       d_linearg.yMax;
    case BOXARG:    return pt     > Point(d_boxarg.flMax);
    case STREAMARG: return pt     > Point(d_streamarg.flMax);
    default:        return pt.lin > g_CurFBuf()->LastLine(); // traversing "none of the above" is treated as a whole-file traverse
    }
   }

void ARG::GetColumnRange( COL *pxMin, COL *pxMax ) const {
   switch( d_argType ) {
    case BOXARG: *pxMin = d_boxarg.flMin.col;
                 *pxMax = d_boxarg.flMax.col;
                 break;
    default:     *pxMin = 0;
                 *pxMax = COL_MAX;
                 break;
    }
   }

bool ARG::Within( const Point &pt, COL len ) const { // Within() pays attention to the x dimension (COLumn), while Beyond() DOES NOT
   const auto xLast( pt.col + std::max( len-1, 0 ) );
   switch( d_argType ) {
    case LINEARG:   return WithinRangeInclusive( d_linearg.yMin, pt.lin, d_linearg.yMax   );
    case BOXARG:    return WithinRangeInclusive( d_boxarg.flMin.lin, pt.lin, d_boxarg.flMax.lin )
                        && WithinRangeInclusive( d_boxarg.flMin.col, pt.col, d_boxarg.flMax.col )
                        && WithinRangeInclusive( d_boxarg.flMin.col, xLast , d_boxarg.flMax.col );
    case STREAMARG: if( !WithinRangeInclusive( d_streamarg.flMin.lin, pt.lin, d_streamarg.flMax.lin ) ) {
                       return false;
                       }
                    if( pt.lin == d_streamarg.flMin.lin ) {
                       return WithinRangeInclusive( d_streamarg.flMin.col, pt.col, COL_MAX )
                           && WithinRangeInclusive( d_streamarg.flMin.col, xLast , COL_MAX )
                           ;
                       }
                    if( pt.lin == d_streamarg.flMax.lin ) {
                       return WithinRangeInclusive( 0, pt.col, d_streamarg.flMax.col )
                           && WithinRangeInclusive( 0, xLast , d_streamarg.flMax.col )
                           ;
                       }
                    return true;
                    // traversing "none of the above" is treated as a whole-file traverse
    default:        return !(pt.lin >  g_CurFBuf()->LastLine());
    }
   }

void ARG::ColsOfArgLine( LINE yLine, COL *pxLeftIncl, COL *pxRightIncl ) const {
   *pxLeftIncl  = 0;
   *pxRightIncl = COL_MAX;
   switch( d_argType ) { // further constrain x based on Arg:
      default:        break;
      case LINEARG:   break;
      case BOXARG:    *pxLeftIncl  = d_boxarg.flMin.col;
                      *pxRightIncl = d_boxarg.flMax.col;
                      break;
      case STREAMARG: if(      yLine == d_streamarg.flMin.lin ) { *pxLeftIncl  = d_streamarg.flMin.col; }
                      else if( yLine == d_streamarg.flMax.lin ) { *pxRightIncl = d_streamarg.flMax.col; }
                      break;
      }
   }

COL ARG::GetLine( std::string &st, LINE yLine ) const { // setup x constraints and call DupLineSeg
   COL xLeftIncl, xRightIncl;
   ColsOfArgLine( yLine, &xLeftIncl, &xRightIncl );
   d_pFBuf->DupLineSeg( st, yLine, xLeftIncl, xRightIncl );
   return st.length();
   }

bool ARG::execute() {
   STATIC_CONST char kszMeta_[] = "meta ";
   bool rv;
   switch( d_argType ) {
    default:       return BadArg();
    // BUGBUG this code should use AppendLineToMacroSrcString or a derivative,
    // NOT StrToNextMacroTermOrEos to parse comments, etc.
    case TEXTARG:  if( d_cArg == 1 ) { // meta is passed thru to macro invoked
                      std::string strToExecute( (d_fMeta?kszMeta_:"") + std::string( d_textarg.pText ) );  // *** MUST *** COPY d_textarg.pText to stack buffer (strToExecute)
                      strToExecute.erase( StrToNextMacroTermOrEos( strToExecute.c_str() ) - strToExecute.c_str() );
                      rv = fExecute( strToExecute.c_str(), false );                                        //              else nested macros get broken!
                      }
                   else { // hacky-kludgy way to get direct access to shell cmds w/o a new key asgnmt: arg arg "ls -l" execute
                      Path::str_t cmd( d_textarg.pText );
                      LuaCtxt_Edit::ExpandEnvVarsOk( cmd ); // BEFORE fChangeFile so curfile envvar expansions are correct
                      0 && DBG( "execute (%" PR_SIZET ") '%s'", cmd.length(), cmd.c_str() );
                      if( 1 ) {
                         StartInternalShellJob( new StringList( cmd.c_str() ), false );
                         rv = true;
                         }
                      else {
                         fChangeFile( kszCompile );
                         rv = CompilePty_CmdsAsyncExec( StringList( cmd.c_str() ), true ) > 0;
                         }
                      }
                   break;
    case BOXARG:   ATTR_FALLTHRU;
    case LINEARG:  0 && DBG( "%s: d_cArg=%d", __func__ , d_cArg );
                   if( d_cArg == 1 ) {
                      std::string dest;
                      for( ArgLineWalker aw( this ); !aw.Beyond() ; aw.NextLine() ) {
                         if( aw.GetLine() ) {
                            aw.buf_erase( StrToNextMacroTermOrEos( aw.c_str() ) - aw.c_str() );  0 && DBG( "2: '%" PR_BSR "'", BSR( aw.lineref() ) );
                            dest.append( aw.lineref().data(), aw.lineref().length() );
                            dest.append( " " );
                            }
                         }
                      if( dest.empty() ) {
                         return false;
                         }
                      rv = fExecute( dest.c_str(), false );
                      }
                   else {
                      auto pSL( new StringList() );
                      for( ArgLineWalker aw( this ); !aw.Beyond() ; aw.NextLine() ) {
                         if( aw.GetLine() ) {                                                    1 && DBG( "--- '%" PR_BSR "'", BSR( aw.lineref() ) );
                            pSL->push_front( aw.lineref() );
                            }
                         }
                      StartInternalShellJob( pSL, false );
                      rv = true;
                      }
                   break;
    }
   DispDoPendingRefreshesIfNotInMacro();
   return rv;
   }

int chGetCmdPromptResponse( PCChar szAllowedResponses, int chDfltInteractiveResponse, int chDfltMacroResponse, const ColoredStrefs &csrs ) {
   //-------------------------------------------------------------
   //
   // chGetAnyMacroPromptResponse()
   //
   // rtns -1 if no/wrong prompt response seen
   // rtns 0 if neutral response
   // rtns char if actual response
   //
   int chMacroPromptResponse( Interpreter::chGetAnyMacroPromptResponse() );
   if(   (chMacroPromptResponse == Interpreter::AskUser)
      || (chMacroPromptResponse == Interpreter::UseDflt && ((chMacroPromptResponse=chDfltMacroResponse) == 0))
      || (strchr( szAllowedResponses, (chMacroPromptResponse=tolower( chMacroPromptResponse )) ) == nullptr)
     ) {
      DispDoPendingRefreshes();
      const auto xCol( VidWrColoredStrefs( DialogLine(), 0, csrs ) );
      CursorLocnOutsideView_Set( DialogLine(), xCol );
      ViewCursorRestorer cr;
      PCV;
      const auto yStart( pcv->Origin().lin );
      char fdbk[3] = { 0,'?',0 };
      do {
         VideoFlusher vf;
         const auto pCmd( CmdFromKbdForExec() );
         if( pCmd->IsFnCancel() ) {
            STATIC_CONST char kszCancelled[] = "cancelled";
            VidWrStrColor( DialogLine(), xCol, kszCancelled, KSTRLEN(kszCancelled), g_colorInfo, false );
            0 && DBG( "%s- -1", __func__ );
            return -1;
            }
         #if 1
         if( pCmd->d_func == &ARG::arg   ) { pcv->ScrollOrigin_Y_Abs( yStart ); continue; } else
         if( pCmd->d_func == &ARG::up    ) { pcv->ScrollOrigin_Y_Rel(   -1   ); continue; } else
         if( pCmd->d_func == &ARG::down  ) { pcv->ScrollOrigin_Y_Rel(   +1   ); continue; } else
         if( pCmd->d_func == &ARG::left  ) { pcv->ScrollOrigin_X_Rel(   -1   ); continue; } else
         if( pCmd->d_func == &ARG::right ) { pcv->ScrollOrigin_X_Rel(   +1   ); continue; } else
         #endif
            {
            fdbk[0] = chMacroPromptResponse = tolower( pCmd->IsFnGraphic() && isprint( pCmd->d_argData.chAscii() ) ? pCmd->d_argData.chAscii() : chDfltInteractiveResponse );
            VidWrStrColor( DialogLine(), xCol, fdbk, sizeof(fdbk)-1, g_colorInfo, false );
            }
         } while( strchr( szAllowedResponses, chMacroPromptResponse ) == nullptr );
      }                                                       0 && DBG( "%s- '%c'", __func__, chMacroPromptResponse );
   return chMacroPromptResponse;
   }

int chGetCmdPromptResponse( PCChar szAllowedResponses, int chDfltInteractiveResponse, int chDfltMacroResponse, PCChar pszPrompt, ... )
   {  0 && DBG( "%s+ '%s'", __func__, pszPrompt );
   //-------------------------------------------------------------
   //
   // chGetAnyMacroPromptResponse()
   //
   // rtns -1 if no/wrong prompt response seen
   // rtns 0 if neutral response
   // rtns char if actual response
   //
   int chMacroPromptResponse( Interpreter::chGetAnyMacroPromptResponse() );
   if(   (chMacroPromptResponse == Interpreter::AskUser)
      || (chMacroPromptResponse == Interpreter::UseDflt && ((chMacroPromptResponse=chDfltMacroResponse) == 0))
      || (strchr( szAllowedResponses, (chMacroPromptResponse=tolower( chMacroPromptResponse )) ) == nullptr)
     ) {
      DispDoPendingRefreshes();
      char lbuf[_MAX_PATH+1];
      va_list val;
      va_start(val, pszPrompt);
      use_vsnprintf( lbuf, sizeof(lbuf), pszPrompt, val );
      va_end(val);
      const auto xCol( Strlen( lbuf ) );
      Msg( "%s", lbuf );  0 && DBG( "%s: '%s'", __func__, lbuf );
      CursorLocnOutsideView_Set( DialogLine(), xCol );
      ViewCursorRestorer cr;
      PCV;
      const auto yStart( pcv->Origin().lin );
      char fdbk[3] = { 0,'?',0 };
      do {
         VideoFlusher vf;
         const auto pCmd( CmdFromKbdForExec() );
         if( pCmd->IsFnCancel() ) {
            STATIC_CONST char kszCancelled[] = "cancelled";
            VidWrStrColor( DialogLine(), xCol, kszCancelled, KSTRLEN(kszCancelled), g_colorInfo, false );
            0 && DBG( "%s- -1", __func__ );
            return -1;
            }
         #if 1
         if( pCmd->d_func == &ARG::arg   ) { pcv->ScrollOrigin_Y_Abs( yStart ); continue; } else
         if( pCmd->d_func == &ARG::up    ) { pcv->ScrollOrigin_Y_Rel(   -1   ); continue; } else
         if( pCmd->d_func == &ARG::down  ) { pcv->ScrollOrigin_Y_Rel(   +1   ); continue; } else
         if( pCmd->d_func == &ARG::left  ) { pcv->ScrollOrigin_X_Rel(   -1   ); continue; } else
         if( pCmd->d_func == &ARG::right ) { pcv->ScrollOrigin_X_Rel(   +1   ); continue; } else
         #endif
            {
            fdbk[0] = chMacroPromptResponse = tolower( pCmd->IsFnGraphic() && isprint( pCmd->d_argData.chAscii() ) ? pCmd->d_argData.chAscii() : chDfltInteractiveResponse );
            VidWrStrColor( DialogLine(), xCol, fdbk, sizeof(fdbk)-1, g_colorInfo, false );
            }
         } while( strchr( szAllowedResponses, chMacroPromptResponse ) == nullptr );
      }              0 && DBG( "%s- '%c'", __func__, chMacroPromptResponse );
   return chMacroPromptResponse;
   }

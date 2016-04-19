//
// Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

bool ARG::true_ () { return true ; }
bool ARG::false_() { return false; }

bool ARG::wrstatefile() {
   WriteStateFile();
   return true;
   }

bool ARG::cwd() {
   return PushVariableMacro( Path::GetCwd().c_str() );
   }

bool ARG::dirty() {
   return g_CurFBuf()->IsDirty();
   }

bool ARG::seteol() {
   // d_cArg == 1: set UNIX EOL mode
   // d_cArg >= 2: set DOS  EOL mode
   return g_CurFBuf()->SetEolModeChanged( d_cArg >= 2 ? EolCRLF : EolLF );
   }

#if defined(_WIN32)
bool FBUF::MakeDiskFileWritable() {
   auto fFileROAttrChanged( false );
   if( FnmIsDiskWritable() ) {
      FileAttribs fa( Name() );
      if( fa.Exists() && fa.IsReadonly() ) {
         if( fa.MakeWritableFailed( Name() ) ) {
            return Msg( "Could not make '%s' writable!", Name() );
            }
         fFileROAttrChanged = true;
         }
      }
   SetDiskRW();
   if( !IsNoEdit() ) { // buffer already RW?
      if( fFileROAttrChanged ) {
         return !Msg( "%s buf was RW, diskfile now RW", Name() );
         }
      return Msg( "%s buf was RW, did nothing", Name() );
      }
   SetDiskRW();
   ClrNoEdit();
   Msg( "%s buf now RW, diskfile %s RW"
      , Name()
      , fFileROAttrChanged ? "now" : "was"
      );
   return true;
   }
#endif


#ifdef fn_rw

bool ARG::rw() {
   return g_CurFBuf()->MakeDiskFileWritable();
   }

#endif

//**********************************************

// fn_deltalogtm: works, but not needed anymore

#ifdef fn_deltalogtm

// code that might be useful someday: variable sprint based on widths of scanned strings

#if 0
   {
   const int l1( captures.Len(1) );
   const int l2( captures.Len(2) );
   safeSprintf( BSOB(lb), FmtStr<20>( "%%0%d.%df", l1 + l2 + 1, l2 ), result );
   }
#endif

#define  THE_EASY_WAY  0

class FcLogTm {
   enum { e_YYYY, e_MM, e_DD, e_hh, e_mm, e_ss, e_usec, e_ArrayEls };
   int v[e_ArrayEls];
   int YYYY()        const { return v[ e_YYYY ]; }
   int MM  ()        const { return v[ e_MM   ]; }
   int DD  ()        const { return v[ e_DD   ]; }
   int hh  ()        const { return v[ e_hh   ]; }
   int mm  ()        const { return v[ e_mm   ]; }
   int ss  ()        const { return v[ e_ss   ]; }
   int usec()        const { return v[ e_usec ]; }
   void YYYY( int val )    { v[ e_YYYY ] = val; }
   void MM  ( int val )    { v[ e_MM   ] = val; }
   void DD  ( int val )    { v[ e_DD   ] = val; }
   void hh  ( int val )    { v[ e_hh   ] = val; }
   void mm  ( int val )    { v[ e_mm   ] = val; }
   void ss  ( int val )    { v[ e_ss   ] = val; }
   void usec( int val )    { v[ e_usec ] = val; }
   STATIC_CONST PCChar nms[e_ArrayEls];
   PCChar FldName( int fnum ) const { return nms[fnum]; }
   typedef int (FcLogTm::*pfxUnitConv)( int ith ); // declare type of pointer to class method
   STATIC_CONST pfxUnitConv tuConvTbl[e_ArrayEls];
   STATIC_CONST int         daysInMonth[13];
   int Sec2Usec( int ) const    { return 1000000; }
   int Min2Sec(  int ) const    { return 60; }
   int Hr2Min(   int ) const    { return 60; }
   int Day2Hr(   int ) const    { return 24; }
   int Mon2Day(  int ith ) {
      return daysInMonth[ith] +
         (   (ith == 2)          // Feb?
          && ((YYYY() % 4) == 0) // leap year (for the next 996 years...)?
         ) ? 1 : 0;              // add one day to Feb for leap year
      }
   int Yr2Mon( int )       { return 12; }
   bool BorrowOk( int fieldToDecr );
   bool decrStepOk( const FcLogTm& decr_by, int fieldToDecr );
public:
   FcLogTm() {
      v[ e_YYYY ] = 0;
      v[ e_MM   ] = 0;
      v[ e_DD   ] = 0;
      v[ e_hh   ] = 0;
      v[ e_mm   ] = 0;
      v[ e_ss   ] = 0;
      v[ e_usec ] = 0;
      }
   FcLogTm( CompiledRegex::capture_container &pc );
   FcLogTm( const FcLogTm &from );
   void  decr( const FcLogTm& decr_by );
   PChar ToStr( PChar buf, int bufSize ) const;
   STATIC_CONST char szRe[];
   };
                           //      1       2       3        4       5       6          7
                           //      Y       M       D        h       m       s          u
const char   FcLogTm::szRe[] = "(\\d{4})(\\d{2})(\\d{2}):(\\d{2})(\\d{2})(\\d{2})\\.(\\d{6})";

const PCChar FcLogTm::nms[e_ArrayEls] = { "YYYY", "MM", "DD", "hh", "mm", "ss", "usec" };

             //                           Jan-1=0
             //                           |  Jan
             //                           |  |
const int    FcLogTm::daysInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//  1 of these = N of these

const FcLogTm::pfxUnitConv FcLogTm::tuConvTbl[e_ArrayEls] =
   { &FcLogTm::Yr2Mon
   , &FcLogTm::Mon2Day
   , &FcLogTm::Day2Hr
   , &FcLogTm::Hr2Min
   , &FcLogTm::Min2Sec
   , &FcLogTm::Sec2Usec
   };

bool FcLogTm::BorrowOk( int fieldToDecr ) {
   if( fieldToDecr < 0 ) {
      Msg( "Underflow!" );
      return false;
      }
   auto &l(      v[ fieldToDecr+1 ] );
   auto &l_borr( v[ fieldToDecr ]   );
   0 && DBG( "Borrowing from %s=%d (now %s=%d)", FldName( fieldToDecr ), l_borr, FldName( fieldToDecr+1 ), l );
   if( l_borr == 0 ) {
      if( !BorrowOk( fieldToDecr-1 ) ) {
         return false;
         }
      }
   --l_borr;
   const auto more( (this->*tuConvTbl[fieldToDecr])( l_borr-1 ) ); // whooee!  http://groups.google.com/groups?hl=en&lr=&selm=Xns947E7EF2B844BukcoREMOVEfreenetrtw%40195.129.110.200&rnum=5
   l += more;
   0 && DBG( "Borrowed 1=%d from %s to get %d %s", more, FldName( fieldToDecr ), l, FldName( fieldToDecr+1 ) );
   return true;
   }

bool FcLogTm::decrStepOk( const FcLogTm& decr_by, int fieldToDecr ) {
   auto       &l(         v[ fieldToDecr ] );
   const auto &r( decr_by.v[ fieldToDecr ] );
   0 && DBG( "%s: %d - %d", FldName( fieldToDecr ), l, r );
   while( l < r ) { // borrow needed?
      if( !BorrowOk( fieldToDecr-1 ) ) {
         return false;
         }
      }
   l -= r;
   return true;
   }

void FcLogTm::decr( const FcLogTm& decr_by ) {
#if THE_EASY_WAY
   double dt = difftime( &d_tt, &decr_by.d_tt );
   usec -= decr_by.usec;
   if( usec < 0 ) {
      usec += 1000000;
      dt   -= 1;
      }
#else
   for( auto ix(e_usec); ix >= e_YYYY; --ix ) {
      if( !decrStepOk( decr_by, ix ) ) {
         break;
         }
      }
#endif
   linebuf lb; DBG( "post-decr: %s", ToStr( BSOB(lb) ) );
   }

PChar FcLogTm::ToStr( PChar buf, int bufSize ) const {
#if THE_EASY_WAY
   struct tm stm = *localtime( &d_tt );
   safeSprintf( buf, bufSize, "%04d%02d%02d:%02d%02d%02d.%06d"
      , stm.tm_year +1900
      , stm.tm_mon  +1
      , stm.tm_mday +1
      , stm.tm_hour
      , stm.tm_min
      , stm.tm_sec
      , usec()
      );
#else
   safeSprintf( buf, bufSize, "%04d%02d%02d:%02d%02d%02d.%06d"
      , YYYY()
      , MM  ()
      , DD  ()
      , hh  ()
      , mm  ()
      , ss  ()
      , usec()
      );
#endif
   return buf;
   }

FcLogTm::FcLogTm( const FcLogTm &from ) { // copy ctor
#if THE_EASY_WAY
#else
   YYYY( from.YYYY() );
   MM  ( from.MM  () );
   DD  ( from.DD  () );
   hh  ( from.hh  () );
   mm  ( from.mm  () );
   ss  ( from.ss  () );
   usec( from.usec() );
#endif
   }

FcLogTm::FcLogTm( CompiledRegex::capture_container &pc ) { DBG( "Count=%d", pc->Count() );
   for( auto ix(0) ; ix < pc->Count() ; ++ix ) {
      DBG( "%d: '%s'", ix, pc->Str(ix) );
      }
   Assert( pc->Count() == 8 ); // remember: 0th capture is the implicit "whole match"!
#if THE_EASY_WAY
   struct tm stm;
   stm.tm_year( atoi( pc[1] ) -1900 );
   stm.tm_mon ( atoi( pc[2] ) -1    );
   stm.tm_mday( atoi( pc[3] ) -1    );
   stm.tm_hour( atoi( pc[4] )       );
   stm.tm_min ( atoi( pc[5] )       );
   stm.tm_sec ( atoi( pc[6] )       );
   d_tt = mktime( &stm );
#else
   YYYY( atoi( pc[1] ) ); 0 && DBG( "YYYY:%s", pc[1] );
   MM  ( atoi( pc[2] ) ); 0 && DBG( "MM  :%s", pc[2] );
   DD  ( atoi( pc[3] ) ); 0 && DBG( "DD  :%s", pc[3] );
   hh  ( atoi( pc[4] ) ); 0 && DBG( "hh  :%s", pc[4] );
   mm  ( atoi( pc[5] ) ); 0 && DBG( "mm  :%s", pc[5] );
   ss  ( atoi( pc[6] ) ); 0 && DBG( "ss  :%s", pc[6] );
#endif
   usec( atoi( pc[7] ) ); 0 && DBG( "usec:%s", pc[7] );
   // linebuf lb; DBG( "%s", ToStr( BSOB(lb) ) );
   }


//*****************************************************************************

class FcLogTmMatchHandler : public FileSearchMatchHandler {
   NO_ASGN_OPR(FcLogTmMatchHandler);
   FcLogTm d_ltmBaseline;
   bool    d_foundBaseline;
   const ARG &d_arg;
public:
   FcLogTmMatchHandler( ARG &arg ) : d_foundBaseline( false ), d_arg( arg ) {}
   STATIC_CONST SearchScanMode &sm() { return smFwd; }
protected:
   bool VMatchActionTaken(        PFBUF pFBuf, Point &cur, COL MatchCols, CompiledRegex::capture_container &pCaptures );
   bool VMatchWithinColumnBounds( PFBUF pFBuf, Point &cur, COL MatchCols ); // cur MAY BE MODIFIED IFF returned false, to mv next srch to next inbounds rgn!!!
   };

bool FcLogTmMatchHandler::VMatchWithinColumnBounds( PFBUF pFBuf, Point &cur, COL MatchCols ) {
   return d_arg.Within( cur, MatchCols );
   }

bool FcLogTmMatchHandler::VMatchActionTaken( PFBUF pFBuf, Point &cur, COL MatchCols, CompiledRegex::capture_container &pCaptures ) {
   FcLogTm val( pCaptures );
   if( d_arg.d_cArg == 1 && !d_foundBaseline ) {
      d_foundBaseline = true;
      d_ltmBaseline = val;
      }
   FcLogTm rslt( val );
   rslt.decr( d_ltmBaseline );
   if( d_arg.d_cArg > 1 ) {
      d_ltmBaseline = val;
      }
   linebuf lb;
   rslt.ToStr( BSOB(lb) );
   std::string t0,t1;
   pFBuf->PutLineSeg( cur.lin, lb, t0,t1, cur.col, cur.col + MatchCols );
   InsHiLite1Line( pFBuf, cur, MatchCols );
   // DispNeedsRedrawAllLinesAllWindows();
   return true;  // "action" taken!
   }

// 20050102:235959.999997
// 20050102:235959.999998
// 20050102:235959.999999
// 20050103:000000.000000
// 20050103:000000.000001
// 20050103:000000.000002
// 20050103:000000.000003



//*****************************************************************************

bool ARG::deltalogtm() {
   // VERY application-specific bool ARG::<041130> klg
   //
   // arg boxarg  deltat
   // arg textarg deltat
   //
   //    On first line of selection, find first match of FcLogTm::szRe and
   //    convert it to FcLogTm (dBaseline).
   //
   //    Then, for each match of FcLogTm::szRe in the selection area (including
   //    the first match!), convert to FcLogTm, subtract dBaseline from it,
   //    format as string, and write back to file.
   //
   // arg arg boxarg  deltat
   // arg arg textarg deltat
   //
   //    same as single arg case, EXCEPT replacement string value is the delta
   //    from the previous match's value, not the first match's value
   //
   // ASSUMES that FcLogTm values ALWAYS INCREASE as we move fwd thru the file
   //

   SearchSpecifier ss( FcLogTm::szRe, true );
   if( ss.HasError() ) {
      return false;
      }
   FcLogTmMatchHandler mh( *this );
   FileSearcher *pSrchr = new FileSearcherRegex( mh.sm(), ss, mh, false );
   pSrchr->SetInputFile();
   pSrchr->SetBounds( *this );
   pSrchr->FindMatches();
   Delete0( pSrchr );
   return true;
   }

// save a test case here!

// 20050102:235959.999997
// 20050102:235959.999998
// 20050102:235959.999999
// 20050103:000000.000000
// 20050103:000000.000001
// 20050103:000000.000002
// 20050103:000000.000003

// 20050301:122000.666666
// 20050301:122000.666667
// 20050301:122000.666668
// 20050301:122000.666669

#endif

//-------------------------------------------------------------------------------------------------

#ifdef fn_mergefilenames

bool ARG::mergefilenames() {
   switch( d_argType ) {
      case TEXTARG: {
           linebuf lbuf;
           Path::Union( BSOB(lbuf), d_textarg.pText, g_CurFBuf()->Name() );
           Msg( "%s", lbuf );
           return true;
           }
      case NOARG: {
           pathbuf drive, dir, fname, ext;
           _splitpath( g_CurFBuf()->Name(), drive, dir, fname, ext );
           Msg( "%s | %s | %s | %s", drive, dir, fname, ext );
           return true;
           }
      default:
           return BadArg();
      }
   }

#endif

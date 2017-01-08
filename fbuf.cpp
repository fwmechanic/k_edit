// -*- c is foobar -*-
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
#include "fname_gen.h"
#include "my_fio.h"

#if      FBUF_TREE
   GLOBAL_VAR RbTree * g_FBufIdx;
   STATIC_VAR RbCtrl s_FBufIdxRbCtrl = { AllocNZ_, Free_, };
   void FBufIdxInit() { g_FBufIdx = rb_alloc_tree( &s_FBufIdxRbCtrl ); }
#else
   GLOBAL_VAR FBufHead g_FBufHead;
#endif

// All inlines that alloc mem call this (terminating) function in their error-path
//
void MemErrFatal( PCChar opn, size_t byteCount, PCChar msg ) {
   Msg( "%s %s( %" PR_SIZET " ) failed: %s", __func__, opn, byteCount, msg );
   EditorExit( 4, false );
   }

// Helper rtnes for templates of same names
#if      MEM_BP
GLOBAL_VAR bool g_fMemBpEnabled;
#endif

PVoid AllocNZ_( size_t bytes )             { MEM_CBP(); return malloc( bytes ); }
PVoid Alloc0d_( size_t bytes )             { MEM_CBP(); return calloc( bytes, 1 ); }
PVoid ReallocNZ_( PVoid pv, size_t bytes ) { MEM_CBP(); return realloc( pv, bytes ); }
void  Free_( PVoid pv )                    { MEM_CBP(); free( pv ); }

// turn stref into ASCIZ (i.e. having ONE '\0' appended), w/extra_nuls _additional_ '\0' chars appended if requested
PChar Strdup( stref src, size_t extra_nuls ) {
   const auto rv( PChar( AllocNZ_( src.length()+1+extra_nuls ) ) );
   memcpy( rv, src.data(), src.length() );
   memset( rv+src.length(), 0, 1+extra_nuls );
   return rv;
   }

//============================================================================================
// ********************  consistency checking code!!!  ********************

void IdleIntegrityCheck() {
   }

// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�
// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�

GLOBAL_VAR bool g_fTrailSpace;

FBUF::FBUF( stref filename, PPFBUF ppGlobalPtr )
   : d_fPreserveTrailSpc  ( g_fTrailSpace )
   {
   ChangeName( filename );
   SetGlobalPtr( ppGlobalPtr );
   InitUndoInfo();
   }

FBUF::~FBUF() {
   FreeLinesAndUndoInfo();
   }

void FBUF::RemoveFBufOnly() {  // ONLY USE THIS AT SHUTDOWN TIME, or within private_RemovedFBuf()!
   0 && DBG( "%s %p", __func__, this );
   0 && DBG( "%s %s", __func__, this->Name() );
#if FBUF_TREE
   rb_delete_node( g_FBufIdx, d_pRbNode );
#else
   DLINK_REMOVE(g_FBufHead, this, dlinkAllFBufs);
#endif
   delete this;  // http://www.parashift.com/c++-faq-lite/delete-this.html
   }

STIL PFBUF NewFbuf( stref filename, PPFBUF ppGlobalPtr ) { // this _IS_ the PFBUF constructor (separate for greppability)
   return new FBUF( filename, ppGlobalPtr );
   }

PFBUF FBUF::AddFBuf( stref filename, PFBUF *ppGlobalPtr ) { 0 && DBG( "%s <- %" PR_BSR "", __func__, BSR(filename) );
   const auto pFBuf( NewFbuf( filename, ppGlobalPtr ) );
#if FBUF_TREE
   rb_insert_gen( g_FBufIdx, pFBuf->Name(), rb_strcmpi, pFBuf );
#else
   DLINK_INSERT_LAST(g_FBufHead, pFBuf, dlinkAllFBufs);
#endif
   return pFBuf;
   }

PFBUF FBOP::FindOrAddFBuf( stref filename, PFBUF *ppGlobalPtr ) {
   const auto pFBuf( FindFBufByName( filename ) );
   if( pFBuf ) {
      if( ppGlobalPtr ) {
         Assert( !pFBuf->HasGlobalPtr() );
         pFBuf->UnsetGlobalPtr();
         pFBuf->SetGlobalPtr( ppGlobalPtr );
         }
      return pFBuf;
      }
   return AddFBuf( filename, ppGlobalPtr );
   }

bool FBOP::PopFirstLine( std::string &st, PFBUF pFbuf ) {
   if( !pFbuf || pFbuf->LineCount() == 0 ) {
      return false;
      }
   pFbuf->DupRawLine( st, 0 );
   pFbuf->DelLine( 0 );
   return true;
   }

// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�
// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�

bool FBUF::CantModify() const {
   if( IsNoEdit() || g_fViewOnly ) {
      Msg( "no-edit buffer may not be modified" );
      return true;
      }
   return false;
   }

// BUGBUG this should really be a AllViewsOntoFbuf event dispatcher
void MakeEmptyAllViewsOntoFbuf( PFBUF pFBuf ) {
   for( auto wix(0) ; wix < g_iWindowCount(); ++wix ) {
      const auto pWin( g_Win( wix ) );
      DLINKC_FIRST_TO_LASTA( pWin->ViewHd, dlinkViewsOfWindow, pv ) {
         if( pv->FBuf() == pFBuf ) {
            if( pv->LineCompile_Valid() ) {
                pv->Set_LineCompile( 0 );
                }
            }
         }
      }
   }

// BEWARE!!!  DO NOT fResetCursorPosition when switching to any file which
// is heretofore unloaded, but for which we have View information (from a
// state-file), will cause the cursor position to be reset to (0,0) (i.e.
// we effectively LOSE the cursor position info saved in the state file).
// fResetCursorPosition s/b set for psudofiles which are rewritten from
// scratch with generated content (thus any prior cursor position is
// meaningless)
//
void FBUF::MakeEmpty() {
   // CAREFUL!  - MakeEmpty is NOT a good place to set the cursor-line to 0,
   // because, in the frequent case of editor-filled buffers, we call MakeEmpty,
   // then refill the buffer.  Sometimes (like when using the InsLineSorted...
   // methods) this means calling InsertLines__(), which calls FBufEvent_LineInsDel,
   // which moves the cursor if the insertion was at a lower line number (for
   // instance, here we set the cursor to Y=0; if a new line-0 is inserted, the
   // cursor is moved "downward" (Y=1), which is counter to our goal.
   //
   // 20060720 klg
   //
   FBOP::PrimeRedrawLineRangeAllWin( this, 0, LineCount() );
   FreeLinesAndUndoInfo();
   InitUndoInfo();
   MakeEmptyAllViewsOntoFbuf( this );
   }

//============================================================================

COL FBOP::MaxCommonLeadingBlanksInLinerange( PCFBUF fb, LINE yTop, LINE yBottom ) {
   auto leadBlank( COL_MAX );
   const auto tw( fb->TabWidth() );
   for( auto iy(yTop) ; iy <= yBottom; ++iy ) {
      const auto rl( fb->PeekRawLine( iy ) );
      if( !IsStringBlank( rl ) ) {
         NoMoreThan( &leadBlank, ColOfFreeIdx( tw, rl, FirstNonBlankOrEnd( rl ) ) );
         }
      }
   return leadBlank == COL_MAX ? 0 : leadBlank;
   }

//==========================================================================================

class MaxIndentAccumulator {
   int      d_sampleCount;
   unsigned d_firstIndentAt[10];
public:
   MaxIndentAccumulator() : d_sampleCount( 0 ) {
      for( auto &val : d_firstIndentAt ) { val = 0; }
      }
   int addSample( sridx indent, bool fLastLine );
   };

int MaxIndentAccumulator::addSample( sridx indent, bool fLastLine ) {
   auto fSampled( false );
   if( indent > 0 && indent < ELEMENTS(d_firstIndentAt) ) {
      ++d_firstIndentAt[indent];
      ++d_sampleCount;
      fSampled = true;
      }
   // periodically check for a clear winner
   //
   if( fLastLine || (1 && fSampled && ((d_sampleCount % 512) == 0)) ) {
     #if 0
      unsigned milliQ[ ELEMENTS(d_firstIndentAt) ];
      for( auto ix(2) ; ix < ELEMENTS(d_firstIndentAt)-1 ; ++ix ) { // special range: [ix-1] && [ix+1] defined for all ix!
         milliQ[ix] = ( (1000*d_firstIndentAt[ix]) / static_cast<double>( d_firstIndentAt[ix-1] + d_firstIndentAt[ix+1] + 1 ) );
         }
      1 && DBG( "indent=%% @ %u 2=%u 3=%u 4=%u 5=%u 6=%u 7=%u 8=%u"
         , d_sampleCount
         , milliQ[ 2]
         , milliQ[ 3]
         , milliQ[ 4]
         , milliQ[ 5]
         , milliQ[ 6]
         , milliQ[ 7]
         , milliQ[ 8]
         );
      struct {
         unsigned ix;
         unsigned val;
         } best[] = { {0,0}, {0,0}, };
      best[0].ix  = 2;
      best[0].val = milliQ[2];
      for( auto ix(3) ; ix < ELEMENTS(d_firstIndentAt)-1 ; ++ix ) {
         if( milliQ[ ix ] <
         }
     #endif
      // if fLastLine, we have limited information, so relax criteria
      const auto PrevNextIndentCountVsThisCount( fLastLine ? 6 : 12 );
      for( auto ix(2) ; ix < ELEMENTS(d_firstIndentAt)-1 ; ++ix ) { // special range: [ix-1] && [ix+1] defined for all ix!
         const auto ThisIndentCount( d_firstIndentAt[ix] );
         if(  ((ThisIndentCount * 5) > d_sampleCount)  // must be at least 20%
            && (ThisIndentCount > (d_firstIndentAt[ix-1] * PrevNextIndentCountVsThisCount))
            && (ThisIndentCount > (d_firstIndentAt[ix+1] * PrevNextIndentCountVsThisCount))
           ) {
            1 && DBG( "indent=? @ %u 1=%u 2=%u 3=%u 4=%u 5=%u 6=%u 7=%u 8=%u 9=%u"
               , d_sampleCount
               , d_firstIndentAt[ 1]
               , d_firstIndentAt[ 2]
               , d_firstIndentAt[ 3]
               , d_firstIndentAt[ 4]
               , d_firstIndentAt[ 5]
               , d_firstIndentAt[ 6]
               , d_firstIndentAt[ 7]
               , d_firstIndentAt[ 8]
               , d_firstIndentAt[ 9]
               );
            1 && DBG( "indent=%d @ %d %s", ix, d_sampleCount, fLastLine ? "LAST" : "" );
            return ix; // clear winner
            }
         }
      }
   return fLastLine ? 1 : 0; // gaak!
   }

//==========================================================================================

void FBUF::CalcIndent( bool fWholeFileScan ) { 0 && DBG( "%s ********************************************", __func__ );
   MaxIndentAccumulator maxIn;
   const auto tw( TabWidth() );
   for( auto yLine(0) ; yLine < LineCount() ; ++yLine ) {
      const auto rl( PeekRawLine( yLine ) );
      const auto ixNonb( FirstNonBlankOrEnd( rl ) );
      if( !atEnd( rl, ixNonb ) ) {
         const auto goodIndentValue( maxIn.addSample( ColOfFreeIdx( tw, rl, ixNonb ), false ) );
         if( goodIndentValue && !fWholeFileScan ) {
            d_IndentIncrement = goodIndentValue;
            return;
            }
         }
      }
   d_IndentIncrement = maxIn.addSample( 0, true );
   }

#ifdef fn_in

bool ARG::in() { // for debugging CalcIndent
   g_CurFBuf()->CalcIndent( true );
   return true;
   }

#endif

bool FileStat::Refresh( int fd ) {
   // const auto fbytes( fio::SeekEoF( fh ) );
   struct_stat stat_buf;
   if( 0 != func_fstat( fd, &stat_buf ) ) {
      d_ModifyTime = 0;
      d_Filesize   = 0;
      d_mode       = 0;
      return false;
      }
   else {
      d_ModifyTime = stat_buf.st_mtime;
      d_Filesize   = stat_buf.st_size;
      d_mode       = stat_buf.st_mode;
      return true;
      }
   }

FileStat GetFileStat( PCChar pszFilename ) { enum { DB=0 };
   FileStat rv;
   // Why is the file size reported incorrectly for files that are still being written to?
   // http://blogs.msdn.com/b/oldnewthing/archive/2011/12/26/10251026.aspx
   // this DOES read return a changing file size for a file being actively written, HOWEVER
   int fh;  // this does not affect st_mtime value returned by stat, though it does cause st_size to change
   if( !fio::OpenFileFailed( &fh, pszFilename, false ) ) {
      const auto fbytes( fio::SeekEoF( fh ) );
      rv.Refresh( fh );
      fio::Close( fh );
      DB&&DBG("%s: %" WL( PR__i64 "u", "jd" ) " bytes", pszFilename, fbytes );
      }
   return rv;
   }

FBUF::DiskFileVsFbufStatus FBUF::checkDiskFileStatus() const {
   if( !FnmIsDiskWritable() )          { return DISKFILE_SAME_AS_FBUF; }
   const auto pName( Name() );
   if( HasWildcard( pName ) )          { return DISKFILE_SAME_AS_FBUF; }
   const auto diskm( GetFileStat( pName ) );
   if( diskm.none() )                  { return DISKFILE_NO_EXIST; }
   const auto s1_v_s2( cmp( diskm, d_LastFileStat ) );
   if( s1_v_s2 > 0 )                   { return DISKFILE_NEWERTHAN_FBUF; }
   if( s1_v_s2 < 0 )                   { return DISKFILE_OLDERTHAN_FBUF; }
                                         return DISKFILE_SAME_AS_FBUF;
   }

void FBUF::SetLastFileStatFromDisk() {
   d_LastFileStat = GetFileStat( Name() );
   }

#ifdef fn_su
bool ARG::silentUpdate() {
   g_CurFBuf()->SetSilentUpdateMode();
   Msg( "SilentUpdateMode=on for current file" );
   return true;
   }
#endif

bool FBUF::UpdateFromDisk( bool fPromptBeforeRefreshing ) { // Returns true iff file changed, false if not
   auto why( "newer" );
   switch( checkDiskFileStatus() ) {
    default:                      return false;
    case DISKFILE_SAME_AS_FBUF:   return false;
    case DISKFILE_NO_EXIST:       return Msg( "File has been deleted" );

    case DISKFILE_OLDERTHAN_FBUF: why = "older"; // EX: did a 'ct unco', restoring an old file version underneath us
                                  //lint -fallthrough
    case DISKFILE_NEWERTHAN_FBUF:
                                  0 && DBG( "trying update" );
                                  if(    fPromptBeforeRefreshing
                                      #ifdef fn_su
                                      && !SilentUpdateMode()
                                      #endif
                                   // && DBG( "confirming" )
                                      && !ConIO::Confirm( Sprintf2xBuf( "%s has been changed DiskFile %s than buffer: Refresh? ", Name(), why ) )
                                    )
                                     {
                                     0 && DBG( "not confirmed" );
                                     SetLastFileStatFromDisk();
                                     return false;
                                     }
                                  if( ReadDiskFileNoCreateFailed() ) {
                                     return false;
                                     }
                                #ifdef fn_su
                                  if( SilentUpdateMode() )
                                     Msg( "%s Silently Updated from %s diskfile", Name(), why );
                                #endif
                                  DispNeedsRedrawTotal();
                                  return true;
    }
   }

// does NOT work
#define  VASL(     nm, list )   const PCChar nm[] = list
#define  VASLPASS( nm )         nm, ELEMENTS(nm)

// VASL( these, {p1,p2} );
// printlist( VASLPASS( name ) );

#if 0  // might be useful for an assertion later?
bool FbufKnown( PFBUF pThis ) {
   // BEWARE!  There is a possible RACE CONDITION for callers of this function!
   // It is comparing (but not deref'ing) a pointer to a FBUF memory block that
   // may have been freed BUT then reallocated to a new FBUF (code can work in
   // mysterious and unexpected ways!).  So we MUST run this check before
   // another FBUF is possibly allocated into the same memory block!
   //
#if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead,d_link,pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf == pThis ) {
         return true;
         }
      }
   return false;
   }
#endif

bool FBUF::private_RemovedFBuf() { // adjusts g_CurFBuf() as necessary: does NOT touch any View (caller must do this!)
   0 && DBG( "%s '%s'", __func__, Name() );
   if( !HasGlobalPtr() ) { // temp: never delete FBUF's having nz GlobalPtr
      RemoveFBufOnly();
      return true;
      }
   return false;
   }

// Makes the file specified by this the current file and displays it in the
// active window.  The file is moved to the top of the file instance list for
// the active window (its file history).
//
PView FBUF::PutFocusOn() { enum { DB=0 }; DB && DBG( "%s+ %s", __func__, this->Name() );
   {
   bool fContentChanged;                            DB && DBG( "%s+ %s %sHasLines %sIsAutoRead", __func__, this->Name(), this->HasLines()?"":"!", this->IsAutoRead()?"":"!" );
   if( !this->HasLines() || this->IsAutoRead() ) {
      if( this->ReadDiskFileAllowCreateFailed() ) { // content read (wildcard buffer expansion=FBufRead_Wildcard->ExpandWildcard) failed?
         // 20110917 made this change (comment out DeleteAllViewsOntoFbuf)...
         // does not break -c (cleanup) mode.  WHY?:
         // 20050316 don't _always_ want a "failure to open a file" to result in
         // ZAPing the View which was used to attempt to open the file; maybe
         // it's on a network drive which "blinked-out" and will be back very
         // soon?  Or maybe it's a build-output file which doesn't exist because
         // the last build attempt failed (but I will want to look at it in a
         // few seconds)?
         DeleteAllViewsOntoFbuf( this );
         return nullptr;
         }                                          DB && DBG( "%s+ %s %sHasLines %sIsAutoRead", __func__, this->Name(), this->HasLines()?"":"!", this->IsAutoRead()?"":"!" );
      fContentChanged = true;
      }
   else {
      const auto fSyncUpdated( this->SyncNoWrite() > 0 );
      fContentChanged = fSyncUpdated;
      }
   if( !fContentChanged && g_CurFBuf() == this ) {  // fileAlreadyHasView?
      Assert( g_CurView() && g_CurView()->FBuf() == this );
      return g_CurView();
      }
   DB && DBG( "%s is %s will be %s", __func__, g_CurFBuf()?g_CurFBuf()->Name():"", this->Name() );
   g_UpdtCurFBuf( this ); //##########################################################################
   // Assert( this == g_CurFBuf() );
   FBOP::CurFBuf_AssignMacros_RsrcLd(); // note that some assignments map to g_CurFBuf() so g_UpdtCurFBuf( this ) above is an absolute prerequisite
   if( d_fNeverReceivedFocus ) {
      SetTabWidth( g_iTabWidth );
      }
   if( fContentChanged ) {
      CalcIndent();
      }
   }
   const auto pCurView( this->PutFocusOnView() );
   DispNeedsRedrawCurWin();
   LuaCtxt_ALL::call_EventHandler( "GETFOCUS" );
   DB && DBG( "%s- %s", __func__, this->Name() );
   return pCurView;
   }

PFBUF FindFBufByName( stref name ) {
#if FBUF_TREE
   const auto pNd( rb_find_sri( g_FBufIdx, name /*, rb_strcmpi */ ) );
   return pNd ? IdxNodeToFBUF( pNd ) : nullptr;
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf) {
      if( pFBuf->NameMatch( name ) ) {
         return pFBuf;
         }
      }
   return nullptr;
#endif
   }

// indent/softcr

STIL int NextIndent( int curIndent, int indentIncr ) {
   const auto rv( ((curIndent+indentIncr) / indentIncr) * indentIncr );  0 && DBG( "%s %d, %d => %d", __func__, curIndent, indentIncr, rv );
   return rv;
   }

STIL int PrevIndent( int curIndent, int indentIncr )  { return ((curIndent-indentIncr) / indentIncr) * indentIncr; }

STATIC_FXN COL SoftcrForCFiles( PCFBUF fb, COL xCurIndent, LINE yStart, stref rl, sridx ixNonb, COL xNonb ) { enum { DB=0 };
   auto indent( fb->IndentIncrement() );
   if( indent == 0 ) {
      indent = fb->TabWidth();
      if( indent == 0 ) {
         indent = 4;
         }
      }                                       DB && DBG( "%s 1 %" PR_BSR "'", __func__, BSR(rl) );
   rl.remove_prefix( ixNonb );                DB && DBG( "%s 2 %" PR_BSR "'", __func__, BSR(rl) );
   rmv_trail_blanks( rl );                    DB && DBG( "%s 3 %" PR_BSR "'", __func__, BSR(rl) );
   if( rl.ends_with( "{" ) ) {                DB && DBG( "%s rl.ends_with( \"{\" )", __func__ );
      return xNonb + indent;
      }
   STATIC_VAR stref c_statement_names[] = {
      "if"       ,
      "else"     ,
      "for"      ,
      "while"    ,
      "do"       ,
      "case"     ,
      "switch"   ,
      "default"  ,
      "struct"   ,
      "class"    ,
      };
   for( const auto &sr : c_statement_names ) {
      if( rl.starts_with( sr ) && (rl.length()==sr.length() || (rl.length()>sr.length() && !isalpha( rl[sr.length()]) ) ) ) {
         DB && DBG( "%s c_statement_names[%" PR_BSR "]", __func__, BSR(sr) );
         return xNonb + indent;
         }
      }
   DB && DBG( "%s eoFxn, -1", __func__ );
   return -1;
   }

int FBOP::GetSoftcrIndent( PFBUF fb ) { // cursor has NOT been moved
   if( !g_fSoftCr )  { return 0; }
   const auto yStart( g_CursorLine() );
   const auto luaVal( GetSoftcrIndentLua( fb, yStart ) );
   if( luaVal >= 0 ) { 0 && DBG( "%s luaVal=%d", __func__, luaVal );
      return luaVal;
      }
   const auto tw( fb->TabWidth() );
   COL rv;
   {
   const auto thisRl( fb->PeekRawLine( yStart ) );
   const auto ixNonb( FirstNonBlankOrEnd( thisRl ) );
   if( atEnd( thisRl, ixNonb ) ) {
      rv = 0;
      }
   else {
      rv = ColOfFreeIdx( tw, thisRl, ixNonb );
      if( fb->FTypeEq( "clang" ) ) {
         const auto rv_C( SoftcrForCFiles( fb, rv, yStart, thisRl, ixNonb, rv ) );
         if( rv_C >= 0 ) {  0 && DBG( "SoftCR C: %d", rv_C );
            return rv_C;
            }
         }
      }
   }
   {
   const auto nextRl( fb->PeekRawLine( yStart + 1 ) );
   const auto ixNonb( FirstNonBlankOrEnd( nextRl ) );
   if( !atEnd( nextRl, ixNonb ) ) {
      rv = ColOfFreeIdx( tw, nextRl, ixNonb );
      }
   }
   0 && DBG( "SoftCR dflt: %d", rv );  Assert( rv >= 0 );
   return rv;
   }

bool ARG::noedit() {
   DispNeedsRedrawStatLn();
   if( !d_fMeta ) {
      // Noedit
      //
      // Toggle the no-edit state of the editor.
      //
      // If the editor was started with the /R (read only) option, this command
      // removes the no-edit limitation.  If the editor is not in no-edit mode,
      // this command turns on no-edit mode.
      //
      return (g_fViewOnly = !g_fViewOnly);
      }
   //
   // Meta Noedit
   //
   // Toggle the no-edit mode for the current file.
   //
   if( !(g_CurFBuf()->FnmIsPseudo()) ) {
      g_CurFBuf()->ToglNoEdit();
      }
   // Returns
   //
   // True:  File or editor in no-edit mode; modification disallowed
   // False: File or editor in edit mode; modification allowed
   //
   return g_CurFBuf()->IsNoEdit();
   }

//-----------------------------------------------------------------

int FBUF::SyncNoWrite() {
   if( HasLines() ) {
      if( UpdateFromDisk( IsDirty() ) ) { 0 && DBG( "%s'd '%s'", __func__, Name() );
         return 1;
         }
      }
   return 0;
   }

//
// ARG::sync walks the file list and refreshes all internally cached files,
// prompting only when the file in the cache (buffer) is dirty (a bad
// situation!).
//
// Generally THIS IS NOT NEEDED, because switching to a file (even if only
// internally when an mfgrep or similar is being performed) will cause the same
// checks to be performed.
//
bool ARG::sync() {
   auto filesRefreshed( 0 );
  #if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
  #else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
  #endif
      {
     #if FBUF_TREE
      const auto pFBuf( IdxNodeToFBUF( pNd ) );
     #endif
      filesRefreshed += pFBuf->SyncNoWrite();
      }
   Msg( "sync: %d file%s refreshed", filesRefreshed, Add_s( filesRefreshed ) );
   return true;
   }
//
//-----------------------------------------------------------------

void WriteAllDirtyFBufs() {
   auto saveCount( 0 );
  #if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
  #else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
  #endif
      {
     #if FBUF_TREE
      const auto pFBuf( IdxNodeToFBUF( pNd ) );
     #endif
      saveCount += pFBuf->SyncWriteIfDirty_Wrote();
      }
   Msg( "Saved %d file%s", saveCount, Add_s( saveCount ) );
   }

bool ARG::saveall() {
   WriteAllDirtyFBufs();  // Saves all modified disk files. Pseudofiles are not saved.
   return true;
   }

EditorFilesStatus_t EditorFilesStatus() {
   EditorFilesStatus_t rv;
  #if FBUF_TREE
   rb_traverse( pNd, g_FBufIdx )
  #else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
  #endif
      {
  #if FBUF_TREE
      PCFBUF pFBuf = IdxNodeToFBUF( pNd );
  #endif
      if( pFBuf->HasLines() && pFBuf->FnmIsDiskWritable() ) {
         ++rv.openFBufs;
         if( pFBuf->IsDirty() ) {
            ++rv.dirtyFBufs;
            }
         }
      }
   return rv;
   }

STATIC_FXN bool RmvFileByName( stref filename ) {
   const auto pfToRm( FindFBufByName( filename ) );
   if( !pfToRm ) {
      return Msg( "no FBUF for '%" PR_BSR "'", BSR(filename) );
      }
   if( pfToRm->IsDirty() && !ConIO::Confirm( Sprintf2xBuf( "Forget DIRTY FBUF '%" PR_BSR "'?", BSR(filename) ) ) ) {
      return Msg( "Dirty FBUF not forgotten '%" PR_BSR "'", BSR(filename) );
      }
   const auto fDidRmv( DeleteAllViewsOntoFbuf( pfToRm ) );
   Msg( "%s FBUF '%" PR_BSR "'", fDidRmv ? "Forgot" : "COULDN'T FORGET", BSR(filename) );
   return fDidRmv;
   }

STATIC_FXN bool RefreshCurFileIfMatchesPseudoName( PCChar pseudoName ) {
   if( g_CurFBuf() == FindFBufByName( pseudoName ) ) {
      return ReadPseudoFileOk( g_CurFBuf() );
      }
   return false;
   }

bool ARG::refresh() {
   switch( d_argType ) {
    default:       return BadArg();
    case NOARG:    { PCF;
                   if(  pcf->FnmIsDiskWritable() // no confirm if WC file
                     && pcf->IsDirty()           // only confirm if DIRTY file
                     && !ConIO::Confirm( "WARNING: current buffer has unsaved edits; are you SURE you want to reread this file? " )
                     ) {
                      return false;
                      }
                   pcf->ReadDiskFileNoCreateFailed();
                   DispNeedsRedrawStatLn();
                   return true;
                   }
    case NULLARG:  if( DLINK_NEXT( g_CurView(), dlinkViewsOfWindow ) == nullptr ) {
                      return fnMsg( "no alternate file" );
                      }
                   if( !ConIO::Confirm( "Forget the current file (from all windows)?! " ) ) {
                      return false;
                      }
                   KillTheCurrentView();
                   DispNeedsRedrawAllLinesAllWindows();
                   SetWindowSetValidView( -1 );
                   return true;
    case TEXTARG:  {
                   const auto rv( RmvFileByName( d_textarg.pText ) );
                   RefreshCurFileIfMatchesPseudoName( kszFiles );
                   return rv;
                   }
    case BOXARG:   //lint -fallthrough
    case LINEARG:  {
                   auto attemptCount( 0 );
                   auto rmvCount( 0 );
                   for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                      if( aw.GetLine() ) {
                         rmvCount += RmvFileByName( aw.lineref() );
                         ++attemptCount;
                         }
                      }
                   fnMsg( "Forgot %d of %d files", rmvCount, attemptCount );
                   RefreshCurFileIfMatchesPseudoName( kszFiles );
                   return (rmvCount > 0) && (rmvCount == attemptCount);
                   }
    }
   }

int DoubleBackslashes( PChar pDest, size_t sizeofDest, PCChar pSrc ) {
   const auto rv( pDest );
   const auto pLastNul( pDest + sizeofDest - 1 );
   while( pDest < pLastNul && (*pDest++ = *pSrc) != 0 ) {
      if( *pSrc == '\\' ) {
         *pDest++ = '\\';
         }
      ++pSrc;
      }
   *pLastNul = '\0';
   return pDest - rv; // strlen of resulting string
   }

void StrUnDoubleBackslashes( PChar pszString ) {
   if( !pszString || *pszString == 0 ) {
      return;
      }
   auto   pWr( pszString );
   PCChar pRd( pszString );
   do {
      if( (*pWr++ = *pRd++) == '\\' && *pRd == '\\' ) {
         ++pRd;
         }
      } while( *pRd != 0 );
   }

PChar xlatCh( PChar pStr, int fromCh, int toCh ) {
   const auto rv( pStr );
   for( ; *pStr ; ++pStr ) {
      if( *pStr == fromCh ) {
          *pStr =  toCh;
          }
      }
   return rv;
   }

STATIC_FXN stref is_content_diff( PCFBUF pFile ) {
   0 && DBG( "%s called on %s %s", __PRETTY_FUNCTION__, pFile->HasLines()?"LINE-FUL":"LINE-LESS", pFile->Name() );
   auto lnum(0);
   stref rl;
#define  CHKL()       (rl=pFile->PeekRawLine( lnum ), lnum <= pFile->LastLine())
#define  CMPL(kstr)   (rl.starts_with( kstr ))
#define  NXTL()       (++lnum,CHKL())
   // from http://git-scm.com/docs/git-diff   search for "new file mode"
   STATIC_CONST char diff_[] = "diff ";
   STATIC_CONST char min_ [] = "--- ";
   STATIC_CONST char pls_ [] = "+++ ";
   //
   //     skip these:
   //
   STATIC_CONST char om__[] = "old mode "          ; // old mode <mode>
   STATIC_CONST char nm__[] = "new mode "          ; // new mode <mode>
   STATIC_CONST char dfm_[] = "deleted file mode " ; // deleted file mode <mode>
   STATIC_CONST char nfm_[] = "new file mode "     ; // new file mode <mode>
   STATIC_CONST char cpfr[] = "copy from"          ; // copy from <path>
   STATIC_CONST char cpto[] = "copy to"            ; // copy to <path>
   STATIC_CONST char mvfm[] = "rename from"        ; // rename from <path>
   STATIC_CONST char mvto[] = "rename to"          ; // rename to <path>
   STATIC_CONST char simi[] = "similarity index"   ; // similarity index <number>
   STATIC_CONST char disi[] = "dissimilarity index"; // dissimilarity index <number>
   STATIC_CONST char idx_[] = "index "             ; // index <hash>..<hash> <mode>
   STATIC_CONST char IdxQ[] = "Index: "            ; // Index: des_v2_1_fe_v2_4_star_v7_1_ps_v2_0/python/CommonFunctionsDES.py  (svn)
   STATIC_CONST char dopt[] = "DIFFOPTS="          ; // this is VERY particular to a bat file I wrote this is first line prefixing std svn diff output 20140313
   STATIC_CONST char eql_[] = "==================================================================="; // (svn)

#if 0
   auto ok( CHKL() && CMPL( diff_ ) || CMPL( IdxQ ) || (CMPL( dopt ) && NXTL() && CMPL( IdxQ )) );
   if( ok ) {
      while( NXTL() &&
         // skip these
         CMPL( om__ ) ||
         CMPL( nm__ ) ||
         CMPL( dfm_ ) ||
         CMPL( nfm_ ) ||
         CMPL( cpfr ) ||
         CMPL( cpto ) ||
         CMPL( mvfm ) ||
         CMPL( mvto ) ||
         CMPL( simi ) ||
         CMPL( disi ) ||
         CMPL( idx_ ) ||
         CMPL( eql_ )
        ) {}
      ok = CHKL() && CMPL( min_ ) && NXTL() && CMPL( pls_ );
      }
#else
   while( CHKL() &&
      // skip these
      CMPL( diff_ ) ||
      CMPL( IdxQ ) ||
      CMPL( om__ ) ||
      CMPL( nm__ ) ||
      CMPL( dfm_ ) ||
      CMPL( nfm_ ) ||
      CMPL( cpfr ) ||
      CMPL( cpto ) ||
      CMPL( mvfm ) ||
      CMPL( mvto ) ||
      CMPL( simi ) ||
      CMPL( disi ) ||
      CMPL( idx_ ) ||
      CMPL( eql_ )
     ) { NXTL(); }
   const auto ok = CHKL() && CMPL( min_ ) && NXTL() && CMPL( pls_ );
#endif
   0 && DBG( "%s %s (%d) ? %s", __PRETTY_FUNCTION__, pFile->Name(), pFile->LineCount(), ok?"yes":"no" );
   return ok ? "diff" : "";
#undef  CHKL
#undef  NXTL
#undef  CMPL
   }

/*
   20150906_110400 real WIP: need a general split stref into walkable/iterable strefs solution
                             split and match (all w/o touching heap)
   an API/interface problem is how to signify "end-of-walk" condition: empty
   stref is not ideal since it's a legit mid-split outcome; in this case I'd
   need to have next() skip these
*/
class stref_split_walk {
   const stref whole_thing;
   sridx ix = 0;
public:
   stref_split_walk( stref whole_thing_ ) : whole_thing( whole_thing_ ) {}
   stref next() {
      if( ix == eosr ) {
         ix = 0;
         return "";
         }
      auto first( eosr );
      for( ; ix < whole_thing.length() ; ++ix ) {
         if( isspace( whole_thing[ix] ) ) {
            if( first != eosr ) {
               return whole_thing.substr( first, ix - first );
               }
            }
         else {
            if( first == eosr ) {
               first = ix;
               }
            }
         }
      ix = eosr;
      if( first != eosr ) {
         return whole_thing.substr( first, whole_thing.length() - first );
         }
      return "";
      }

   };

STATIC_FXN stref shebang_binary_name( PCFBUF pfb ) { // should be simple, right?
   auto rl0( pfb->PeekRawLine( 0 ) );
   if( !rl0.starts_with( "#!" ) ) { return ""; }
   const auto i1( FirstBlankOrEnd( rl0, 2 ) );    // assume: no spaces in path of binary
   rl0.remove_suffix( rl0.length() - i1 );        // strip command tail
   const auto ls( rl0.find_last_of( "/" ) );      // assume: unix dirsep, regardless of platform
   const auto i0( ls != stref::npos ? ls+1 : 2 ); // could be bare binary name (i.e. filetype)
   if( i0 >= i1 ) { return ""; }                  // nothing at all?
   const auto shebang( rl0.substr( i0, i1 - i0 ) ); 0 && DBG( "%" PR_BSR "<=shebang", BSR(shebang) );
   if( shebang == "env" ) {
      auto rl( pfb->PeekRawLine( 0 ) ); rl.remove_prefix( i1 );
      stref_split_walk ssw( rl );
      stref tok;
      while( tok = ssw.next() , !tok.empty() ) {  0 && DBG( "tok=%" PR_BSR "'", BSR(tok) );
         if( !tok.starts_with( '-' ) ) {
            return tok;
            }
         }
      }
   return shebang;
   }

STATIC_FXN stref emacs_major_mode( PCFBUF pfb ) { // http://www.gnu.org/software/emacs/manual/html_node/emacs/Choosing-Modes.html
#if 0 // following WORKS, except  isolate "mode: param"  functionality has not been written (pending impl of stref_split_walk etc.)
   // find first nonblank line
   stref rv( "" );
   for( auto yLine(0) ; yLine < pfb->LineCount() ; ++yLine ) {
      const auto rl( pfb->PeekRawLine( yLine ) );
      if( !IsStringBlank( rl ) && !(yLine==0 && rl.starts_with( "#!" )) ) {
         rv = rl;
         break;
         }
      }
   if( 0 == rv.length() ) {
      return rv;
      }
   stref mode_delim( "-*-" );
   const auto first( rv.find( mode_delim ) );
   if( first != eosr ) {
      rv.remove_prefix( first + mode_delim.length() );
      const auto second( rv.find( mode_delim ) );
      if( second != eosr ) {
         rv.remove_suffix( rv.length() - second ); 1 && DBG( "emacs_major_mode=%" PR_BSR "'", BSR(rv) );
         // isolate "mode: param"
         }
      }
#endif
   return "";
   }

STATIC_FXN stref content_to_ftype( PCFBUF pfb ) {
   typedef stref (*content_deducer_t)( PCFBUF pfb );
   STATIC_CONST content_deducer_t content_deducers[] = {
      emacs_major_mode     ,
      shebang_binary_name  ,
      is_content_diff      ,
      };
   for( const auto fxn : content_deducers ) {
      const auto rv( fxn( pfb ) );
      if( !rv.empty() ) { return rv; }
      }
   return "";
   }

STATIC_FXN PCChar fnm_to_ftype( PCFBUF pfb ) {
   const auto srNm( pfb->Namesr() );
   const auto srFnm( Path::RefFnm( srNm ) );
   if(   Path::endsWith( srFnm, "makefile" )
        #if !defined(_WIN32)
        || Path::endsWith( srFnm, "Makefile" )
        #endif
     ) {                                            0 && DBG( "%s %" PR_BSR "' is %s", __func__, BSR(srFnm), "make" );
      return "make";
      }
  #if 1 || !defined(_WIN32)
   if( srFnm.starts_with( ".bash" ) ) {             1 && DBG( "%s %" PR_BSR "' is %s", __func__, BSR(srFnm), "bash" );
      return "bash";
      }
  #endif
   return "";
   }

STATIC_CONST char s_sftype_prefix[] = "ftype:";
enum { SIZEOF_MAX_FTYPE=51 };
STATIC_VAR char s_cur_Ftype         [ SIZEOF_MAX_FTYPE ];
STATIC_VAR char s_cur_FtypeSectionNm[ SIZEOF_MAX_FTYPE + KSTRLEN(s_sftype_prefix) ];

void swidFtype( PChar dest, size_t sizeofDest, void *src ) {
   scpy( dest, sizeofDest, s_cur_Ftype );
   }

STATIC_FXN stref Get_s_cur_Ftype() {
   return s_cur_Ftype;
   }

STATIC_FXN void Set_s_cur_Ftype( stref ftype ) {
   bcpy( s_cur_Ftype, ftype );    0 && DBG( "%s %s", __func__, s_cur_Ftype );
   }

PCChar swixFtype( stref param ) {
   Set_s_cur_Ftype( param );
   return nullptr;
   }


STATIC_FXN void FtypeRestoreDefaults() {
   swixDelims( "{[(" );
   }

STATIC_FXN bool RsrcFileLdSectionFtype( stref ftype ) {
   FtypeRestoreDefaults();
   char section[ SIZEOF_MAX_FTYPE + KSTRLEN(s_sftype_prefix) ];
   bcat( bcpy( section, s_sftype_prefix ).length(), section, ftype );              0 && DBG( "%s %s", __func__, section );
   auto dummy( 0 );
   const auto fSectionExists( RsrcFileLdAllNamedSections( section, &dummy ) );
   bcpy( s_cur_FtypeSectionNm, fSectionExists ? section : s_sftype_prefix );
   return fSectionExists;
   }

PCChar LastRsrcFileLdSectionFtypeNm       () { return s_cur_FtypeSectionNm+KSTRLEN(s_sftype_prefix); }
PCChar LastRsrcFileLdSectionFtypeSectionNm() { return s_cur_FtypeSectionNm; }

STATIC_FXN stref SetRsrcExt_( PCFBUF fb ) { // all rv's shall NOT have leading '.'
   stref rv;
   if( FnmIsLogicalWildcard( fb->Namestr() ) ) {
      rv = "*";
      }
   else if( fb->FnmIsPseudo() || !fb->FnmIsDiskWritable() ) {
      rv = "<>";
      }
   else {
      rv = content_to_ftype( fb );
      if( rv.empty() ) {
         rv = Path::RefExt( fb->Namestr() );
         if( !rv.empty() ) {
            rv.remove_prefix( 1 ); // Path::RefExt() preserves the leading '.' that defines the start of the file extension; remove it
            }
         if( rv.empty() ) {
            rv = fnm_to_ftype( fb );
            }
         }
      }                                                            1 && DBG( "%s %s -> '%" PR_BSR "'", __func__, fb->Name(), BSR(rv) );
   return rv;
   }

void FBUF::SetRsrcExt() {
   const stref srDot( "." );
   const auto ext( ::SetRsrcExt_( this ) );
   d_RsrcExt.reserve( srDot.length() + ext.length() );
   d_RsrcExt.assign( sr2st( srDot ) + sr2st( ext ) ); // d_RsrcExt shall have leading '.'
   }

STATIC_FXN bool DefineStrMacro( stref name, stref strval ) {       0 && DBG( "%s '%" PR_BSR "'='%" PR_BSR "'"     , __func__, BSR(name), BSR(strval) );
   const auto str( "\"" + sr2st(strval) + "\"" );
   const auto rv( DefineMacro( name, str ) );                      0 && DBG( "%s '%" PR_BSR "'='%" PR_BSR "'-> %c", __func__, BSR(name), BSR(strval), rv?'y':'n' );
   return rv;
   }

void FBOP::CurFBuf_AssignMacros_RsrcLd() { const auto fb( g_CurFBuf() );  1 && DBG( "%s '%s'", __func__, fb->Name() );
   if( g_pFBufAssignLog ) { g_pFBufAssignLog->FmtLastLine( "##### %s -> %s", __func__, fb->Name() ); }
   // 1. assigns "curfile..." macros based on g_CurFBuf()->Namestr()
   // 2. loads rsrc file section for [extension and] ftype of g_CurFBuf()
  #if MACRO_BACKSLASH_ESCAPES
   dbllinebuf dblbuf;
   Pathbuf pbuf( fb->Name() );
   DoubleBackslashes( BSOB(dblbuf), pbuf );
   DefineStrMacro( "curfile", dblbuf );
  #else
   DefineStrMacro( "curfile", fb->Namestr() );
  #endif
   DefineStrMacro( "curfilename", Path::RefFnm  ( fb->Namestr() ) );
   DefineStrMacro( "curfileext" , Path::RefExt  ( fb->Namestr() ) );
   DefineStrMacro( "curfilepath", Path::RefDirnm( fb->Namestr() ) );
   if( !fb->IsRsrcLdBlocked() ) { // ONLY AFTER curfile, curfilepath, curfilename, curfileext assigned:
      const auto rsrcExt( fb->GetRsrcExt() );
      // weirdness: ftype is a switch setting which (among other things) guides
      // content-sensitive display hiliting.  The ftype setting is cached by the
      // FBuf.  This is ASSUMED to be set in rsrcExt sections.  Since switch values
      // are currently stored in global (boo!) variables, we snoop the rsrcExt
      // section assignment process ...
      swixFtype( "unknown" ); // default
      RsrcFileLdAllNamedSections( rsrcExt );
      const auto ftype( Get_s_cur_Ftype() );  // snoop ftype
      fb->SetFType( ftype );                                          0 && DBG( "%s '%" PR_BSR "' '%s' =======", __func__, BSR(ftype), fb->Name() );
      RsrcFileLdSectionFtype( ftype );
      }
   else {
      fb->SetFType( Get_s_cur_Ftype() );
      }
   }

STATIC_FXN Path::str_t xlat_fnm( PCChar pszName ) {
   auto rv( CompletelyExpandFName_wEnvVars( pszName ) );
   if( rv.empty() ) {
      ErrorDialogBeepf( "Cannot access %s - %s", pszName, strerror( errno ) );
      const auto fb( FindFBufByName( pszName ) );
      if( fb ) { DeleteAllViewsOntoFbuf( fb ); }
      }
   return rv;
   }

// Prefer OpenFileNotDir_ in lieu of fChangeFile if you don't want to have the
// PFBUF-stack-order affected (any new FBuf opened is placed at BOTTOM/END of list)
//
// SEE ALSO: fChangeFile
//
// NOTE!  DO NOT call this fxn directly!  Call either
//   * OpenFileNotDir_NoCreate
//   * OpenFileNotDir_CreateSilently
// instead.
//
PFBUF OpenFileNotDir_( PCChar pszName, bool fCreateOk ) { enum { DP=0 }; // heavily patterned after fChangeFile, but w/o reliance on global vars
                                                                 DP && DBG( "OFND+     '%s'", pszName );
   const auto fnamebuf( xlat_fnm( pszName ) );
   if( fnamebuf.empty() ) {
      return nullptr;
      }
   auto pFBuf( FindFBufByName( fnamebuf ) );
   CPCChar pFnm( fnamebuf.c_str() );                             DP && DBG( "OFND xlat='%s'", pFnm );
   if( !pFBuf ) {
      if( !FBUF::FnmIsPseudo( pFnm ) ) {
         FileAttribs fa( pFnm );
         if( fa.Exists() ) {
            if( fa.IsDir() ) {                                      DP && DBG( "OFND! isD '%s'", pFnm );
               Msg( "%s does not open directories like %s", __func__, pFnm );
               return nullptr;
               }
            }
         else {
            if( !fCreateOk ) {
               Msg( "File '%s' does not exist", pFnm );             DP && DBG( "OFND! NoD '%s'", pFnm );
               return nullptr;
               }
            }
         }
      pFBuf = AddFBuf( pFnm );
      }
   if( !pFBuf->HasLines() && pFBuf->ReadDiskFileAllowCreateFailed( fCreateOk ) ) {
                                                                 DP && DBG( "OFND! !rd '%s'", pFnm );
      DeleteAllViewsOntoFbuf( pFBuf );
      return nullptr;
      }
                                                                 DP && DBG( "OFND- ok  '%s'", pFnm );
   return pFBuf;
   }

#include "rm_util.h"

//
// SetCwdOk is THE ONLY PLACE from which Path::SetCwdOk may be called!
//
GLOBAL_VAR PFBUF g_pFBufCwd;
STATIC_FXN bool SetCwdOk( PCChar newCwd, bool fSave, bool *pfCwdChanged ) {
   // if( !IsDir(newCwd) ) { // appropriate for the extra-paranoid
   //    DBG( "not a dir '%s'", newCwd );
   //    return false;
   //    }
   const auto cwdBefore( Path::GetCwd() );
   if( !Path::SetCwdOk( newCwd ) ) { // Msg( "chdir(%s) failed: %s", newCwd, strerror( errno ) );
      return false;
      }
   const auto cwdAfter( Path::GetCwd() );
   *pfCwdChanged = !Path::eq( cwdBefore, cwdAfter );
   if( !*pfCwdChanged ) {
      Msg( "cwd unchanged" );
      }
   else {
      Msg( "Changed directory to %s", cwdAfter.c_str() );
      if( fSave ) {
         std::string tmp;
         g_pFBufCwd->InsLine( 0, cwdBefore, tmp );
         }
      }
   return true;
   }

bool ARG::popd() { // arg "_sdup" _spush arg "fn" _spush arg _spop _spop msearch
   std::string ts;
   return FBOP::PopFirstLine( ts, g_pFBufCwd ) ? fChangeFile( ts.c_str(), false ) : ErrPause( "empty cwd stack" );
   }

// The fChangeFile function changes the current file, drive, or directory to
// the specified pszName and returns true if successful.
//
// SEE ALSO: OpenFileNotDir_
//
bool fChangeFile( PCChar pszName, bool fCwdSave ) { enum { DP=0 };  DP && DBG( "%s+ '%s'", __func__, pszName );
   const auto fnamebuf( xlat_fnm( pszName ) );
   if( fnamebuf.empty() ) {  DP && DBG( "%s- xlat_fnm FAIL '%s'", __func__, pszName );
      return false;
      }
   CPCChar pFnm( fnamebuf.c_str() );
   auto pFBuf( FindFBufByName( fnamebuf ) );     DP && DBG( "%s '%s' -->PFBUF %p", __func__, pFnm, pFBuf );
   if( !pFBuf ) {
      auto fCwdChanged( false );
      if( SetCwdOk( pFnm, fCwdSave, &fCwdChanged ) ) {
         if( fCwdChanged ) { // if cwd changed, display new cwd's contents
            DP && DBG( "%s recurse", __func__ );
            fChangeFile( "*", false ); // recursive, but will only nest ONE level
            }
         DP && DBG( "%s- SetCwdOk '%s'", __func__, pszName );
         return true;
         }
      pFBuf = AddFBuf( pFnm );               DP && DBG( "%s AddFBuf'%s' -->PFBUF %p", __func__, pFnm, pFBuf );
      }
   const auto rv( nullptr != pFBuf->PutFocusOn() );
   DP && DBG( "%s- PutFocusOn=%c '%s'", __func__, rv?'t':'f', pszName );
   return rv; // pFBuf == g_CurFBuf() == g_CurView()->FBuf()
   }

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//
// BEGIN   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// BEGIN   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// BEGIN   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
//
// to get the UNadorned name of the file, use FBUF::Name()
// to get the   ADORNED name of the file, use FBUF::UserName()
// use IsolateFilename() to convert from ADORNED name to UNadorned name
//

char Path::DelimChar( PCChar fnm ) { // BUGBUG this needs to be (a) purpose-clarified, (b) made per OS (shell?)
   const stref srfnm( fnm );
   if( atEnd( srfnm, ToNextOrEnd( stref(" ,&;^"), srfnm, 0 ) ) ) { return 0; }   // no delim needed
   if( atEnd( srfnm, ToNextOrEnd( chQuot2       , srfnm, 0 ) ) ) { return chQuot2; } // "
   if( atEnd( srfnm, ToNextOrEnd( chQuot1       , srfnm, 0 ) ) ) { return chQuot1; } // '
   return '|'; // last ditch: ugly, but NEVER a valid filename char(?)
   }

Path::str_t Path::UserName( PCChar name ) {
   const char delimCh( Path::DelimChar( name ) );
   if( delimCh ) {
      Path::str_t rv;
      rv.push_back( delimCh );
      rv.append( name );
      rv.push_back( delimCh );
      return rv;
      }
   else {
      return name;
      }
   }

Path::str_t FBUF::UserName() const {
   return Path::UserName( Name() );
   }

// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
//
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

GLOBAL_VAR int g_iBlankAnnoDispSrcMask = BlankDispSrc_DIRTY | BlankDispSrc_SEL;

void FBUF::BlankAnnoDispSrcEdge( int cause, bool fReveal ) {
   /* this function is called when one of the potential per-FBUF sources changes
      state.  The event source is ID'd by cause, and its new state is fReveal.
      all of the events/sources here are per-FBUF, but these are masked by
      a global switch, g_iBlankAnnoDispSrcMask

      source:      scope     example cause of state change
      always?      global    user changes value of switch  <- not impl
      always-FBUF  FBUF      user runs ARG::ftab, changes value of setting for this FBUF
      dirty        FBUF      user edits or saves FBUF
      selection    FBUF/View user makes arg selection
   */
   // always de/assert the source
   if( fReveal ) { d_BlankAnnoDispSrcAsserted |=  cause; }
   else          { d_BlankAnnoDispSrcAsserted &= ~cause; }
   const auto wantReveal( 0 != (d_BlankAnnoDispSrcAsserted & (g_iBlankAnnoDispSrcMask | BlankDispSrc_USER_ALWAYS)) );
   const auto revealedBefore( d_fRevealBlanks );
   d_fRevealBlanks = wantReveal; // update even if not currently being viewed
   if( ViewCount() && revealedBefore != wantReveal ) { 0 && DBG( "about to call DispNeedsRedrawCurWin();" );
      DispNeedsRedrawCurWin();
      }
   }

void FBUF::SetDirty( bool fDirty ) {
   d_fDirty = fDirty;
   BlankAnnoDispSrcEdge( BlankDispSrc_DIRTY, d_fDirty );
   }

struct DisplayNoiseBlanker {
   ~DisplayNoiseBlanker() { DisplayNoiseBlank(); }
   };

STATIC_FXN bool FBufRead_Wildcard( PFBUF pFBuf ) {
   pFBuf->MakeEmpty();
   FBOP::ExpandWildcardSorted( pFBuf, pFBuf->Name() );
   pFBuf->ClearUndo();
   pFBuf->UnDirty();
   return true;
   }

// keep the following msg strings in sync!
STIL void DialogLdFile ( PCChar name )                 { Msg( "Next file is %s", name ); }
STIL void DialogLddFile( PCChar name, unsigned bytes ) { Msg( "Next file is %s: %u bytes", name, bytes ); }

STIL void rdNoiseOpen() { DisplayNoise( kszRdNoiseOpen ); }

#if 1

// write: classic look

STIL void wrNoiseBak  () { DisplayNoise( " Bak                 " ); }
STIL void wrNoiseOpen () { DisplayNoise( " Bak Open            " ); }
STIL void wrNoiseWrite() { DisplayNoise( " Bak Open Write      " ); }
STIL void wrNoiseRenm () { DisplayNoise( " Bak Open Write Renm " ); }

#else

// write: duty-cycle look

STIL void wrNoiseBak  () { DisplayNoise( " Bak                 " ); }
STIL void wrNoiseOpen () { DisplayNoise( "     Open            " ); }
STIL void wrNoiseWrite() { DisplayNoise( "          Write      " ); }
STIL void wrNoiseRenm () { DisplayNoise( "                Renm " ); }

#endif

const Eol_t platform_eol = WL( EolCRLF, EolLF );

bool FBUF::FBufReadOk_( bool fAllowDiskFileCreate, bool fCreateSilently ) {
   VR_( DBG( "FRd+ %s", Name() ); )
// if( !Interpreter::Interpreting() )  20061003 klg commented out since calling mfgrep inside a macro (as I'm doing now) hides this, which I don't want.
      {
      DialogLdFile( Name() );
      }
   d_EolMode = platform_eol;                                   0 && DBG( "%s+ %s'", FUNC, Name() );
   if( FnmIsLogicalWildcard( Namestr() ) ) {                   0 && DBG( "%s+ %s' FnmIsLogicalWildcard", FUNC, Name() );
      FBufRead_Wildcard( this );
      return true;
      }
   if( FnmIsPseudo() ) {                                       0 && DBG( "%s+ %s' FnmIsPseudo", FUNC, Name() );
      return ReadPseudoFileOk( this );
      }
   if( !FnmIsDiskWritable() ) {
      return Msg( "%s failed! FnmIs!DiskWritable", __func__ );
      }
  #if defined(_WIN32)
   // scenario: WinNT filesystems are case-preserving but case-insensitive
   // user edits file in K, then later renames it, changing only case of name
   // user opens in K, retrieves old-cased nm from state file, edits and saves, writing to old-cased name :-(
   const auto canonFnm( Path::CanonizeCase( Name() ) );
   if( !eq( canonFnm, Namestr() ) ) { // NB: CASE-SENSITIVE equality-check intended here!
       ChangeName( canonFnm.c_str() );
       }
  #else
   // Linux filesys is case-sensitive so this scenario cannot happen
  #endif
   //###########  disk file read !!!
   rdNoiseOpen();
   DisplayNoiseBlanker dblank;
   int hFile;
   if( fio::OpenFileFailed( &hFile, Name(), false ) ) {
      if( errno != ENOENT ) {
         return Msg( "Cannot open %s - %s", Name(), strerror( errno ) );
         }
      if( Namesr().starts_with( "/tmp/" ) ) {
         DBG( "FRd! not creating /tmp/ file" );
         return false;
         }
      if(    !fAllowDiskFileCreate
         || (!fCreateSilently && !ConIO::Confirm( Sprintf2xBuf( "%s does not exist. Create? ", Name() ) ))
        ) {
         // SW_BP;
         DBG( "FRd! user denied create" );
         return false;
         }
      if( fio::OpenFileFailed( &hFile, Name(), false, DFLT_TEXTFILE_CREATE_MODE ) ) {
         return Msg( "Cannot create %s - %s", Name(), strerror( errno ) );
         }
      VR_( DBG( "FRd: created newfile '%s'", Name() ); )
      std::string tmp;
      PutLine( 0, "", tmp );
      }
   else {
      if( ReadDiskFileFailed( hFile ) ) {
         DBG( "FRd! ReadDiskFile failed" );
         //---------------------------------------------------------------------
         // This is a little wierd.  While creating and testing the
         // EdKC_EVENT_ProgramGotFocus handling code, I encountered a problem
         // whereby once a read error occurs when attempting to refresh an FBUF
         // during a EdKC_EVENT_ProgramGotFocus event
         // (RefreshCheckAllWindowsFBufs, SyncNoWrite), there would be no further
         // attempt to automatically refresh the FBUF.
         //
         // Investigation revealed that this was because there is a check in
         // SyncNoWrite that doesn't perform UpdateFromDisk if the FBUF has no
         // lines (this prevents trying to "refresh" FBUFs that have never been
         // read from disk in the first place; there are almost always many such
         // laying around due to file-history accumulation in the state file).
         //
         // Anyway, the workaround is to _give_ such an FBUF some lines so it
         // passes the HasLines test.  This also gives the user a reminder that
         // this blank screen IS NOT a actual representation of the disk file.
         //
         // 20070309 kgoodwin
         PutLastLine( "*** The last attempt to read this file from disk FAILED!!! ***" );
         UnDirty();  // MUST do this otherwise the refresh inserts a prompt
         //---------------------------------------------------------------------
         FlushKeyQueuePrimeScreenRedraw();
         fio::Close( hFile );
         return false;
         }
      // if( !Interpreter::Interpreting() )  20061003 klg commented out since calling mfgrep inside a macro (as I'm doing now) hides this, which I don't want.
      DialogLddFile( Name(), d_cbOrigFileImage );
      VR_( DBG( "FRd: VMRead done" ); )
      }
   fio::Close( hFile );
   ClrNoEdit();
  #if defined(_WIN32)
   if( IsFileReadonly( Name() ) ) {
      VR_( DBG( "FRd: is RO_FILE" ); )
      SetDiskRO();
      if( !g_fEditReadonly ) {
         SetNoEdit();
         }
      }
   else {
      VR_( DBG( "FRd: is NOT RO_FILE" ); )
      SetDiskRW();
      }
  #endif
   SetLastFileStatFromDisk();
   VR_( DBG( "FRd- OK" ); )
   return true;
   }

bool FBUF::FBufReadOk( bool fAllowDiskFileCreate, bool fCreateSilently ) {
   const auto rv( FBufReadOk_( fAllowDiskFileCreate, fCreateSilently ) );
   if( !IsRsrcLdBlocked() ) {
      SetRsrcExt();
      }
   return rv;
   }

bool FBUF::ReadOtherDiskFileNoCreateFailed( PCChar pszName ) {
   rdNoiseOpen();
   int hFile;
   if( fio::OpenFileFailed( &hFile, pszName, false ) ) {
      ErrorDialogBeepf( "%s does not exist", pszName );
      return true;
      }
   const auto failed( ReadDiskFileFailed( hFile ) );
   fio::Close( hFile );
   return failed;
   }

//----------------------------------------------------------------------------------

GLOBAL_VAR int g_iBackupMode = bkup_UNDEL; // keep as int, GLOBAL_VAR because this is a SWITCH

STATIC_FXN PCChar kszBackupMode( int backupMode ) {
   switch( backupMode ) {
      default        :
      case bkup_NONE : return "none" ;
      case bkup_BAK  : return "bak"  ;
      case bkup_UNDEL: return "undel";
      }
   }

void swidBackup( PChar dest, size_t sizeofDest, void *src ) {
   scpy( dest, sizeofDest, kszBackupMode( g_iBackupMode ) );
   }

PCChar swixBackup( stref param ) {
   if(      0==cmpi( param, "bak"   ) )  { g_iBackupMode = bkup_BAK   ; } // backup:none
   else if( 0==cmpi( param, "undel" ) )  { g_iBackupMode = bkup_UNDEL ; } // backup:bak
   else if( 0==cmpi( param, "none"  ) )  { g_iBackupMode = bkup_NONE  ; } // backup:undel
   else {  return SwiErrBuf.Sprintf( "'%s' value must be one of 'undel', 'bak' or 'none'", kszBackup ); }
   return nullptr;
   }

enum { VERBOSE_WRITE = 1 };
STATIC_FXN bool backupOldDiskFile( PCChar fnmToBkup, int backupMode ) {
   VERBOSE_WRITE && DBG( "FWr: bkup[%s] start d='%s'", kszBackupMode( backupMode ), fnmToBkup );
   switch( backupMode ) {
    case bkup_BAK:   { VERBOSE_WRITE && DBG( "FWr: bkup_BAK" );  // the extension of the previous version of the file is changed to .BAK
                     const auto BAK_FileNm( Path::Union( ".bak", fnmToBkup ) );
                     VERBOSE_WRITE && DBG( "FWr: bkup_BAK  %s -> %s", fnmToBkup, BAK_FileNm.c_str() );  // the extension of the previous version of the file is changed to .BAK
                    #if defined(_WIN32)
                     // In posix/Linux, rename() attempts to unlink any existing destination FAILS iff
                     //    this attempt fails, therefore we do not need to attempt our own preemptive unlink
                     // in Win32/MSVC RTL, rename() FAILS if the destination already exists
                     // In Win32/MSVC RTL, rename() FAILS if the destination already exists
                     if( !unlinkOk( BAK_FileNm.c_str() ) ) {
                        if( errno != ENOENT ) {
                           return Msg( "Can't delete %" PR_BSR " - %s", BSR(Path::RefFnameExt( BAK_FileNm )), strerror( errno ) );
                           }
                        }
                    #endif
                     if( !MoveFileOk( fnmToBkup, BAK_FileNm.c_str() ) ) {
                        const auto emsg( strerror( errno ) );
                        if( FileOrDirExists( fnmToBkup ) ) {
                           return Msg( "Can't rename %s to %s - %s", fnmToBkup, BAK_FileNm.c_str(), emsg );
                           }
                        }
                     } break;
    case bkup_UNDEL: // The old file is moved to a child directory.  The number of copies saved is
                     { VERBOSE_WRITE && DBG( "FWr: bkup_UNDEL" ); // specified by the Undelcount switch.
                     const auto rc( SaveFileMultiGenerationBackup( fnmToBkup ) );
                     switch( rc ) {
                        case SFMG_OK             : break;
                        case SFMG_NO_SRCFILE     : break; // return Msg( "Can't stat %s", fnmToBkup );
                        case SFMG_CANT_MV_ORIG   : return Msg( "Can't move %s", fnmToBkup );
                        case SFMG_CANT_MK_BAKDIR : return Msg( "Can't create dest dir" );
                        default:                   return Msg( "unknown SFMG error %d", rc );
                        }
                     }
                     //-lint fallthrough
    case bkup_NONE:  // No backup: the editor overwrites the file.
                     // here we delete the file to be overwritten
                     VERBOSE_WRITE && DBG( "FWr: bkup_NONE" );
                     if( !unlinkOk( fnmToBkup ) ) {
                        const auto emsg( strerror( errno ) );
                        if( FileOrDirExists( fnmToBkup ) ) {
                           return Msg( "Can't delete %s - %s", fnmToBkup, emsg );
                           }
                        }
                     break;
    }
   return true;
   }

STATIC_FXN bool FBUF_WriteToDiskOk( PFBUF pFBuf, PCChar pszDestName );

bool FBUF::write_to_disk( PCChar destFileNm ) {
   // BUGBUG there are security-hazard file/directory race conditions to be found here!
  #if defined(_WIN32)
  #if 0
   //
   // NOTES 20090721 kgoodwin editing NTFS ADS (Alt Data STREAMS) hits the rocks
   //                         here (at least).  The standard file write algorithm is
   //
   // 1. write FBUF content to (new) temp file
   // 2. move old file by renaming
   // 3. rename temp file to old file's name
   //
   // Having an FBUF mapped to an ADS causes problems.  An ADS is really a piece
   // of an existing file (except when the Win32 APIs support direct access, for
   // example for reading).
   //
   // Backup algo for streams:
   //
   // 1. copy old file to backup filename
   // 2. write FBUF's stream to original file
   //
   // At different times we need the full name including ADS, other times we
   // need the base name (ADS name removed).  It is easy to toggle between the
   // two by assignment to *pStream.
   //
   // The remaining step is to make the backup functions perform the ADS
   // copy-based backup algorithm above.
   //
   Path::str_t destFnm( destFileNm );
   PChar pStream = strchr( destFnm+3, ':' );
   if( pStream ) { *pStream = '\0'; }
  #endif
   {
   FileAttribs dest( destFileNm );
   if( dest.Exists() && dest.IsReadonly() ) {
      if(   ConIO::Confirm( Sprintf2xBuf( "File '%s' is readonly; overwrite anyway?", destFileNm ) )
         && dest.MakeWritableFailed( destFileNm )
        ) {
         return Msg( "Could not make '%s' writable!", destFileNm );
         }
      if( IsFileReadonly( destFileNm ) ) {
         return Msg( "'%s' is not writable", destFileNm );
         }
      }
   }
  #endif
   const auto tmpFnm( Path::Union( ".$k$", destFileNm ) );
   DisplayNoiseBlanker dblank;
   if( !FBUF_WriteToDiskOk( this, tmpFnm.c_str() ) ) {
      // cannot write tmpFnm: directory is probably not writable
      // last chance: try overwriting target file directly
      if( FBUF_WriteToDiskOk( this, destFileNm ) ) {
         return !Msg( "Overwrote %s", Name() );
         }
      return Msg( "Cannot write %s", destFileNm );
      }
   d_tmLastWrToDisk = time( nullptr ); // note this is not == stat mtime, thus not comparable to same
   wrNoiseBak();
   const auto abs_dest( Path::Absolutize( destFileNm ) );
   if( !backupOldDiskFile( abs_dest.c_str(), d_backupMode==bkup_USE_SWITCH ? g_iBackupMode : d_backupMode ) ) {
      unlinkOk( tmpFnm.c_str() );
      return false;
      }
   wrNoiseRenm();
   if( !MoveFileOk( tmpFnm.c_str(), destFileNm ) ) {
      Msg( "Can't rename %s to %s - %s", tmpFnm.c_str(), destFileNm, strerror( errno ) );
      unlinkOk( tmpFnm.c_str() );
      return false;
      }
   if( !NameMatch( destFileNm ) ) {
      ChangeName( destFileNm );
      }
  #if defined(_WIN32)
   SetDiskRW();
  #endif
   UnDirty(); SetLastFileStatFromDisk(); DispNeedsRedrawStatLn();
   return !Msg( "Saved  %s", Name() ); // xtra spc to match Msg( "Saving %s" ... above
   }

void FBUF::ChangeName( stref newName ) { // DBG( "%s %" PR_BSR "'", __func__, BSR(newName) );
   d_filename.assign( newName.data(), newName.length() ); // THE ONLY PLACE where FBUF::d_filename is changed!!!
   d_fFnmDiskWritable = ::Path::IsLegalFnm( Name() );
   }

bool FBUF::WriteToDisk( PCChar pszSavename ) {
   Path::str_t dest;
   if( pszSavename && *pszSavename ) {
      dest = Path::Absolutize( pszSavename );
      if( dest.empty() ) {
         return Msg( "Cannot resolve '%s'", pszSavename );
         }
      }
   const auto fFnmChanging( dest[0] && !NameMatch( dest ) );
   Path::str_t pseudoFnm;
   PPFBUF gp( nullptr );  // if it has a gp, it is a pseudo file
   if( !fFnmChanging )            { dest      = Name(); }
   else if( (gp=GetGlobalPtr()) ) { pseudoFnm = Name(); } // if it is a pseudo file w/a gp, it needs to hang around
   if( fFnmChanging ) { VERBOSE_WRITE && DBG( "FWr+ %s -> %s", Name(), dest.c_str() ); }
   else               { VERBOSE_WRITE && DBG( "FWr+ %s", Name() );                           }
   if( !FnmIsDiskWritable() && !fFnmChanging ) { // unwritable FBUF not being saved to alt file?
      return Msg( "Can't save pseudofile to disk under it's own name; use 'arg arg \"diskname\" setfile'" );
      }
   VERBOSE_WRITE && DBG( "FWr: dst=%s", dest.c_str() );
   if( IsDir( dest.c_str() ) ) {
      return Msg( "Cannot write '%s'; directory in the way", dest.c_str() );
      }
   VERBOSE_WRITE && DBG( "FWr: A" );
   const auto saveOk( write_to_disk( dest.c_str() ) );
   if( 0 ) {
      struct_stat stat_buf;
      if(  func_stat( dest.c_str(), &stat_buf ) == -1
        || stat_buf.st_size == 0
        ) {
         ConIO::DbgPopf( "write_to_disk() leaked!  '%s' does NOT exist (or is 0 bytes) on disk !!!", dest.c_str() );
         }
      }
   if( saveOk && gp ) { // a FBUF having a GlobalPtr (which by defn WAS a pseudofile) has been saved under a new name?
                        // must create a new FBUF having the old name, and point the GlobalPtr at it
      UnsetGlobalPtr(); // no GlobalPtr associated with 'this' anymore
      FBOP::FindOrAddFBuf( pseudoFnm, gp ); // associate GlobalPtr with a new FBUF created specifically to bear the old name
      }
   return saveOk;
   }

bool FBUF::SaveToDiskByName( PCChar pszNewName, bool fNeedUserConfirmation ) {
   if( pszNewName[0] == '\0' ) {
      return Msg( "new filename is empty?" );
      }
   const auto expandedFnm( CompletelyExpandFName_wEnvVars( pszNewName ) );
   {
   auto pDupFBuf( FindFBufByName( expandedFnm ) );
   if( pDupFBuf && pDupFBuf == this ) {
      return Msg( "current filename and new filename are same; nothing done" );
      }
   if( fNeedUserConfirmation && !ConIO::Confirm( Sprintf2xBuf( "Do you want to save this file as %s ?", expandedFnm.c_str() ) ) ) {
      FlushKeyQueuePrimeScreenRedraw();
      return false;
      }
   if( pDupFBuf ) {
      if( pDupFBuf->IsDirty() && !ConIO::Confirm( "You are currently editing the new-named file in a different buffer!  Destroy edits?!" ) ) {
         return false;
         }
      pDupFBuf = pDupFBuf->ForciblyRemoveFBuf();
      }
   }
   if( !WriteToDisk( expandedFnm.c_str() ) ) { DBG( "%s: WriteToDisk( '%s' -> '%s' ) FAILED?", __func__, Name(), expandedFnm.c_str() );
      return false;
      }
   SetRememberAfterExit();
   0 && DBG( "%s: wrote( '%s' -> '%s' ) OK", __func__, Name(), expandedFnm.c_str() );
   return true;
   }

STATIC_FXN bool IfOnlyOneFilespecInCurWcFileSwitchToIt() {
   // if only ONE filespec matched, open that file
   const auto pFBuf( g_CurFBuf() ); // new wild-pseudofile should have been made current
   if( pFBuf->LineCount() != 1 ) {
      return true;
      }
   Path::str_t fnm;
   if( pFBuf->GetLineIsolateFilename( fnm, 0, 0 ) < 1 ) { // read first line from pseudofile
      return Msg( "GetLineIsolateFilename(0) failed!" );
      }
   const auto pFn( fnm.c_str() );
   if( IsDir( pFn ) ) {
      return true;
      }
   const auto new_pFbuf( OpenFileNotDir_NoCreate( pFn ) );  // switch to the only file
   if( !new_pFbuf ) {
      return Msg( "Failure switching to top file!" );
      }
   new_pFbuf->PutFocusOn();
   DeleteAllViewsOntoFbuf( pFBuf );  // zap the wildcard pseudofile so it can't go on-screen
   return !Msg( "There was only one match: you're looking at it!" );
   }

//----------------------------------------------------------------------------------

namespace BufdWr { // BufdWr "mini module"
   // this is a static buffer namespace rather than a class so the file
   // write operation doesn't make any (significant) heap demands
   //
   STATIC_VAR struct {
      size_t bytesToWriteThisTime;
      PChar pWriteBufferCurrent;
      char  WriteBuffer[ 128 * 1024 ];
      } s_Buf;

   STATIC_FXN void ResetVars() {
      s_Buf.bytesToWriteThisTime = 0;
      s_Buf.pWriteBufferCurrent = s_Buf.WriteBuffer;
      }

   STATIC_FXN bool FlushFailed( int hFile ) {
      const auto rv( !fio::WriteOk( hFile, s_Buf.WriteBuffer, s_Buf.bytesToWriteThisTime ) );
      ResetVars();
      return rv;
      }

   STATIC_FXN bool WrFailed( int hFile, PCChar pSrcData, size_t byteCount ) {
      for( size_t bytesLeft=byteCount ; bytesLeft ;  ) {
         const size_t cpyBytes( SmallerOf( bytesLeft, sizeof(s_Buf.WriteBuffer) - s_Buf.bytesToWriteThisTime ) );
         memcpy( s_Buf.pWriteBufferCurrent, pSrcData, cpyBytes );
         s_Buf.pWriteBufferCurrent  += cpyBytes;
         s_Buf.bytesToWriteThisTime += cpyBytes;
         pSrcData                   += cpyBytes;
         bytesLeft                  -= cpyBytes;
         if(    s_Buf.bytesToWriteThisTime == sizeof s_Buf.WriteBuffer
             && FlushFailed( hFile )
           ) {
            return true;
            }
         }
      return false;
      }
   };

STATIC_FXN bool FileWrErr( int hFile_Write, PCChar pszDestName, PCChar Dialog_fmtstr ) {
   fio::Close( hFile_Write );
   unlinkOk( pszDestName );
   if( Dialog_fmtstr ) {
      Msg( Dialog_fmtstr, pszDestName );
      }
   FlushKeyQueuePrimeScreenRedraw();
   return false;
   }

PCChar EolName( Eol_t eol ) { return eol==EolLF ? "LF" : "CRLF"; }

GLOBAL_VAR bool g_fTrailLineWrite   = false;
GLOBAL_VAR bool g_fForcePlatformEol = false;

STATIC_FXN bool FBUF_WriteToDiskOk( PFBUF pFBuf, PCChar pszDestName ) { enum {DB=0}; // hidden/private FBUF method
   wrNoiseOpen();
   int hFile_Write;
   const auto create_mode( pFBuf->GetLastFileStat().d_mode );
   if(   fio::OpenFileFailed( &hFile_Write, pszDestName, true )
      && fio::OpenFileFailed( &hFile_Write, pszDestName, true, create_mode ? create_mode : DFLT_TEXTFILE_CREATE_MODE )
     ) {
      return Msg( "Cannot open or create %s - %s", pszDestName, strerror( errno ) );
      }
   wrNoiseWrite();
   const auto fForced( g_fForcePlatformEol && pFBuf->EolMode() != platform_eol );
   if( fForced ) {
      pFBuf->SetEolModeChanged( platform_eol );
      }
   Msg( "Saving %s%s", pszDestName, fForced ? " with platform EOL forced":"" );
   const stref srEol( pFBuf->EolMode() == EolLF ? "\x0A" : "\x0D\x0A" );
   BufdWr::ResetVars();
   auto maxLine( pFBuf->LineCount() );
   if( !g_fTrailLineWrite ) {
      while( maxLine > 0 ) {
         if( !FBOP::IsLineBlank( pFBuf, --maxLine ) ) {
            break;
            }
         }
      ++maxLine;
      }
   for( auto yLine( 0 ); yLine < maxLine; ++yLine ) {
      if( ExecutionHaltRequested() ) {
         return FileWrErr( hFile_Write, pszDestName, "User break writing '%s'" );
         }
      const auto src( pFBuf->PeekRawLine( yLine ) );
      if(   (src.length() && BufdWr::WrFailed( hFile_Write, src  .data(), src  .length() ))
         ||                  BufdWr::WrFailed( hFile_Write, srEol.data(), srEol.length() )
        ) {
         return FileWrErr( hFile_Write, pszDestName, ExecutionHaltRequested() ? "User break writing '%s'" : "Out of space on '%s'" );
         }
      }
   if( BufdWr::FlushFailed( hFile_Write ) ) {
      return FileWrErr( hFile_Write, pszDestName, ExecutionHaltRequested() ? "User break writing '%s'" : "Out of space on '%s'" );
      }
   fio::Fsync( hFile_Write );
   fio::Close( hFile_Write );
   DB&&DBG( "disk-wrote '%s'", pszDestName );
   return true;
   }

//
//  Setfile
//
//     Switches to the most recently edited file.
//
//  Arg Setfile
//
//     Switches to the file name that begins at the cursor and ends with the
//     first blank.
//
//  Arg <textarg> Setfile
//
//     Switches to the specified file.
//
//     The argument may be a drive or directory name, in which case the
//     editor changes the current drive or directory.
//
//     If <textarg> is a wildcard, and the wildcard matches more than one file,
//     the editor creates a pseudofile that lists the files matching the
//     specified wildcard.  You can then open files from this list by using Arg
//     Setfile.
//
//     If <textarg> is a wildcard, and the wildcard matches only one file, that
//     file is opened
//
//  Meta ... Setfile
//
//     As above, but does not save changes for the current file.
//
//  Arg Arg <textarg> Setfile
//
//     Save the file under the specified name.
//
//  Arg Arg Setfile
//
//     Saves the current file (iff dirty).
//
//  Arg Arg Arg setfile
//
//     Saves all dirty files (via ARG::saveall) and performs tmpsave (via ARG::wrstatefile)
//
//  Returns
//
//  True:  File opened successfully
//  False: No alternate file, the specified file does not exist and you did
//         not wish to create it, or the current file needs to be saved and
//         cannot be saved
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  Types of input strings we might see in setfile:
//
//  * AFP=absolute file-path (EX: NOARG gets from next file)
//  * RFP=relative file-path
//  * WC =contains wildcard chars (and not an URL, which can too)
//  * PSU=pseudofile
//  * URL
//
//  Types of FBUFs that result when an input string is post-processed into an
//  FBUF:
//
//  * AF1=single absolute file, displays file's content
//  * WCX=wildcard expansion,   contains list of AFP's one per WCX-file-line
//  * PSU=pseudofile
//
//  [idea: Would it be useful to make these subclasses of FBUF?  Prob not.
//  Rather it's the NAMES that seem to need to have additional context
//  associated with them, so that as they pass thru various processing steps,
//  they don't have to be reinvestigated in detail each time]
//
//  (Since PSU are trivially detected, we will not discuss them further here)
//  [idea: the '>' filename prefix char is available for use]
//
//  The primary challenge in handling this diversity of input types is
//  when/whether to absolutize.  It is CRITICAL that RFP, in all of it's guises,
//  be absolutized, in order to avoid duplicate FBUF's which differ in path of
//  name-case, but which refer to the same disk file.
//
//  Less critical, but still important to help the user be clear, is to
//  absolutize those WCX which need it, while avoiding making the name of the
//  resultant FBUF so long that it's incomprehensible (and exceeds internal
//  string-length limits.  With the advent of powerful and compact
//  pre-WC-expansion MFspec syntax, this is a major problem, and I wonder if a
//  good solution in this case is to devise a new post-processed filename syntax
//  which expresses "MFspec evaluated when cwd=XXX".
//
//  For example, given a filespec "$(TAGZ)\$(*.c|*.h|*.cpp)", evaluated when
//  CWD=c:\klg\sn\k\, it becomes:
//
//      ">c:\klg\sn\k\>$(TAGZ)\$(*.c|*.h|*.cpp)"
//  or maybe just
//       "c:\klg\sn\k\>$(TAGZ)\$(*.c|*.h|*.cpp)"
//
//  Of course, such syntax would also have to be supported as an input string
//  (such as when the file is "reread").
//
//  One thing which is impossible to distinguish by mere "examination" is
//  whether a file-path (containing embedded envvar refs) resolves to ONLY ONE
//  expansion that exists, many, or none.  Why does this matter?  Because the
//  decision as to how to post-process as a AFP vs as a WCX probably determines
//  how the filename is handled earlier: if the filespec is a list, but it
//  resolves into only one file, then that one file should be opened directly.
//  But if it matches many files, we'd like to see a WCX.  (Note that
//  IfOnlyOneFilespecInCurWcFileSwitchToIt() already performs this logic for
//  setfile).
//
//  Due to the support for the decoding of embedded envvar refs (which for
//  purposes of this discussion also includes '|'-separated "literal" envvar
//  refs) in RFP's and WCX's, it seems impossible to simply examine a non-URL
//  non-PSU and tell whether it needs absolutizing: you CAN tell whether it has
//  already been absolutized, but without (at least experimental) expansion of
//  embedded envvar refs, we can't tell whether it NEEDS TO BE absolutized.
//

bool fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( PCChar pbuf ) { 0 && DBG( "%s: %s", __func__, pbuf );
   const auto fHasWildcard( FnmIsLogicalWildcard( pbuf ) );
   const auto retVal( fChangeFile( pbuf ) );
   return retVal && fHasWildcard ? IfOnlyOneFilespecInCurWcFileSwitchToIt() : retVal;
   }

STATIC_FXN bool RenameCurFileOk( PCChar pszNewName, bool fNeedUserConfirmation ) {
   const auto saveOk( g_CurFBuf()->SaveToDiskByName( pszNewName, fNeedUserConfirmation ) );
   if( saveOk ) {
      DispNeedsRedrawStatLn();
      FBOP::CurFBuf_AssignMacros_RsrcLd();
      }
   return saveOk;
   }

bool ARG::shex() {
   switch( d_argType ) {
    default:      return BadArg();
    case TEXTARG: StartShellExecuteProcess( StrPastAnyBlanks( d_textarg.pText ) );
                  return true;
    }
   }

bool ARG::setfile() {
   MainThreadPerfCounter pc;
   Path::str_t fnm;
   switch( d_argType ) {
    default:      return BadArg(); //--------------------------------------------
    case NOARG:   {
                  const auto nxtvw( DLINK_NEXT( g_CurView(), dlinkViewsOfWindow ) );
                  if( !nxtvw ) { return fnMsg( "no alternate file" ); }
                  fnm = nxtvw->FBuf()->Namestr();
                  } break; //-------------------------------------------------------
    case NULLARG: if( d_cArg > 1 ) {
                     if( !d_fMeta && d_cArg >= 3 )                { return fExecute( "wrstatefile saveall" );  }
                     if( d_cArg == 2 && !g_CurFBuf()->IsDirty() ) { return !fnMsg( "current file not dirty" ); }
                     return g_CurFBuf()->WriteToDisk() == false;   // *** 'arg arg setfile' saves current file to disk
                     }
                  if( g_CurFBuf()->GetLineIsolateFilename( fnm, d_nullarg.cursor.lin, d_nullarg.cursor.col ) < 1 ) { // read first line from pseudofile
                     return false;
                     }
                  break; //-------------------------------------------------------
    case TEXTARG: if( d_cArg >= 2 ) { // save file with new name, pFBuf takes on new name identity
                     if( !FnmIsLogicalWildcard( d_textarg.pText ) ) {
                        return RenameCurFileOk( d_textarg.pText, !d_fMeta );
                        }
                     // ADD TBD special handling for wildcard specs when d_cArg >= 2
                     }
                  fnm = d_textarg.pText;
                  break; //-------------------------------------------------------
    }
   0 && DBG( "%s: %s", __func__, fnm.c_str() );
   if( d_argType != NOARG ) { // for NOARG we HAVE a definitive name, skip the following reinterpretations
      if( LuaCtxt_Edit::ExecutedURL( fnm.c_str() ) ) { // WINDOWS-ONLY HOOK
         return true;
         }
      LuaCtxt_Edit::ExpandEnvVarsOk( fnm );
      SearchEnvDirListForFile( fnm, true );
      }
   const auto rv( fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( fnm.c_str() ) );
   DBG( "### %s t=%9.6f %s", __func__, pc.Capture(), fnm.c_str() );
   0 && DBG( "%s: %s", __func__, fnm.c_str() );
   return rv;
   }

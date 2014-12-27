//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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
   Msg( "%s %s( %" PR_SIZET "u ) failed: %s", __func__, opn, byteCount, msg );
   EditorExit( 4, false );
   }

// Helper rtnes for templates of same names
//

#if      MEM_BP
GLOBAL_VAR bool g_fMemBpEnabled;
#endif

PVoid AllocNZ_( size_t bytes )             { MEM_CBP(); return malloc( bytes ); }
PVoid Alloc0d_( size_t bytes )             { MEM_CBP(); return calloc( bytes, 1 ); }
PVoid ReallocNZ_( PVoid pv, size_t bytes ) { MEM_CBP(); return realloc( pv, bytes ); }
void  Free_( void *pv )                    { MEM_CBP(); free( pv ); }

PChar Strdup( PCChar st, int chars ) {
   const auto rv( PChar( AllocNZ_( chars+1 ) ) );
   memcpy( rv, st, chars );
   rv[chars] = '\0';
   return rv;
   }

PChar Strdup( PCChar st             ) { return Strdup( st, Strlen( st ) ); }
PChar Strdup( PCChar st, PCChar eos ) { return Strdup( st, eos-st       ); }

//
//============================================================================================

// ********************  consistency checking code!!!  ********************
// ********************  consistency checking code!!!  ********************
// ********************  consistency checking code!!!  ********************

#define  DO_IDLE_FILE_VALIDATE  0
#if      DO_IDLE_FILE_VALIDATE

void LineInfo::CheckDump( PCChar msg, PFBUF pFBuf, LINE yLine ) const {
   DBG( "%s '%s' L %d has bad len: %x, p=%p"
      , msg
      , pFBuf->Name()
      , yLine
      , GetLineLen()
      , GetLineRdOnly()
      );
   }

int FBUF::DbgCheck() {
   if( !d_paLineInfo )
      return 0;

   auto errors(0);
   auto pLi( d_paLineInfo );
   for( auto pLiPastEnd(pLi + LineCount()) ; pLi < pLiPastEnd ; ++pLi ) {
      if( pLi->Check( "idleCheck", this, pLi - d_paLineInfo ) )
         ++errors;
      }
   return errors;
   }

STATIC_FXN PFBUF CheckPFBUF( PFBUF pFBuf, int *errors ) {
   if( pFBuf ) {
      *errors += pFBuf->DbgCheck();
      return pFBuf->Next();
      }
   return nullptr;
   }

#endif// DO_IDLE_FILE_VALIDATE

void IdleIntegrityCheck() {
#if DO_IDLE_FILE_VALIDATE
   auto errors( 0 );
   const auto next( CheckPFBUF( g_CurFBuf(), &errors ) );
   if( next )
      CheckPFBUF( next, &errors );

   if( errors ) {
      DBG( "idleCheck %d ERRORS ***********************************************", errors );
      ConIO::DbgPopf( "idleCheck detected %d ERRORS!!!", errors );
      }
#endif// DO_IDLE_FILE_VALIDATE
   }

// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�
// 께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께�

GLOBAL_VAR bool g_fTrailSpace;

FBUF::FBUF( PCChar filename, PPFBUF ppGlobalPtr )
   : d_fPreserveTrailSpc  ( g_fTrailSpace )
   , d_TabWidth           ( 1 )
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


STIL PFBUF NewFbuf( PCChar filename, PPFBUF ppGlobalPtr ) { // this _IS_ the PFBUF constructor (separate for greppability)
   return new FBUF( filename, ppGlobalPtr );
   }


PFBUF FBUF::AddFBuf( PCChar filename, PFBUF *ppGlobalPtr ) { 0 && DBG( "%s <- %s", __func__, filename );
   const auto pFBuf( NewFbuf( filename, ppGlobalPtr ) );
#if FBUF_TREE
   rb_insert_gen( g_FBufIdx, pFBuf->Name(), rb_strcmpi, pFBuf );
#else
   DLINK_INSERT_LAST(g_FBufHead, pFBuf, dlinkAllFBufs);
#endif
   return pFBuf;
   }

PFBUF FBOP::FindOrAddFBuf( PCChar filename, PFBUF *ppGlobalPtr ) {
   const auto pFBuf( FindFBufByName( filename ) );
   if( pFBuf ) {
      if( ppGlobalPtr ) {
         Assert( !pFBuf->HasGlobalPtr() );
         pFBuf->UnsetGlobalPtr(); // Just in case Assert failed but we're still running
         pFBuf->SetGlobalPtr( ppGlobalPtr );
         }
      return pFBuf;
      }

   return AddFBuf( filename, ppGlobalPtr );
   }

bool FBOP::PopFirstLine( std::string &st, PFBUF pFbuf ) {
   if( !pFbuf || pFbuf->LineCount() == 0 )
      return false;

   st = pFbuf->getLineRaw( 0 );
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
            if( pv->Get_LineCompile() >= 0 )
                pv->Set_LineCompile( 0 );
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
         NoMoreThan( &leadBlank, ColOfFreeIdx( tw, rl, NextNonBlankOrEnd( rl ) ) );
         }
      }
   return leadBlank == COL_MAX ? 0 : leadBlank;
   }

//==========================================================================================

int IsWFilesName( PCChar pszName ) { // pszName matches "<win4>"
   STATIC_CONST char hdr[] = "<win";
   const auto len( Strlen( pszName ) );
   if(    len > (KSTRLEN( hdr ) + 1)
       && 0 == memcmp( hdr, pszName, KSTRLEN( hdr ) )
       && pszName[len-1] == '>'
     ) {
      auto pNum( pszName + KSTRLEN( hdr ) );
      while( isDecDigit(*pNum) )
         pNum++;

      if( *pNum != '>' )
         return -1;

      Assert( pNum - (pszName + KSTRLEN( hdr )) == 1 );

      const auto wnum( pszName[ KSTRLEN( hdr ) ] - '0' );
      if( !(wnum < g_iWindowCount()) )
         return -2;

      return wnum;
      }

   return -1;
   }


//==========================================================================================

class MaxIndentAccumulator {
   int d_sampleCount;
   int d_firstIndentAt[10];

public:

   MaxIndentAccumulator() : d_sampleCount( 0 ) {
      memset( d_firstIndentAt, 0, sizeof(d_firstIndentAt) );
      }

   int addSample( PCChar buf, bool fLastLine );
   };


int MaxIndentAccumulator::addSample( PCChar buf, bool fLastLine ) {
   auto fSampled( false );
   const int indent( StrPastAnyBlanks(buf) - buf );
   if( indent > 0 && indent < ELEMENTS(d_firstIndentAt) ) {
      ++d_firstIndentAt[indent];
      ++d_sampleCount;
      fSampled = true;
      }

   // periodically check for a clear winner
   //
   if( (fSampled && ((d_sampleCount % 1024) == 0)) || fLastLine ) {
      0 && DBG( "%s @ %d 0=%d 1=%d 2=%d 3=%d 4=%d 5=%d 6=%d 7=%d 8=%d 9=%d"
         , __func__
         , d_sampleCount
         , d_firstIndentAt[ 0]
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

      // if fLastLine, we have limited information, so relax criteria
      const auto PrevIndentCountVsSample(        fLastLine ? 8 : 32 );
      const auto PrevNextIndentCountVsThisCount( fLastLine ? 5 : 10 );

      for( auto ix(1) ; ix < ELEMENTS(d_firstIndentAt)-1 ; ++ix ) { // special range: [ix-1] && [ix+1] defined for all ix!
         const auto PrevIndentCount( d_firstIndentAt[ix-1] );
         const auto NextIndentCount( d_firstIndentAt[ix+1] );
         const auto ThisIndentCount( d_firstIndentAt[ix  ] );

         if(   ((ThisIndentCount * 5) > d_sampleCount)  // must be at least 20%
            && (ThisIndentCount > (PrevIndentCount * PrevNextIndentCountVsThisCount))
            && (ThisIndentCount > (NextIndentCount * PrevNextIndentCountVsThisCount))
           ) {
            0 && DBG( "%s = %d @ %d %s", __func__, ix, d_sampleCount, fLastLine ? "LAST" : "" );
            return ix; // clear winner
            }
         }
      }

   return fLastLine ? 1 : 0; // gaak!
   }

//==========================================================================================

void FBUF::CalcIndent( bool fWholeFileScan ) { 0 && DBG( "%s ********************************************", __func__ );
   MaxIndentAccumulator maxIn;
   d_IndentIncrement = 0;
   std::string xb;
   for( auto yLine(0) ; yLine < LineCount() ; ++yLine ) {
      getLineTabx( xb, yLine );
      const auto goodIndentValue( maxIn.addSample( xb.c_str(), false ) );
      if( goodIndentValue && !fWholeFileScan ) {
         d_IndentIncrement = goodIndentValue;
         return;
         }
      }
   d_IndentIncrement = maxIn.addSample( "", true );
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
   if( 0 != func_fstat( fd, &stat_buf ) )
      {
      d_ModifyTime = 0;
      d_Filesize   = 0;
      return false;
      }
   else {
      d_ModifyTime = stat_buf.st_mtime;
      d_Filesize   = stat_buf.st_size;
      return true;
      }
   }

FileStat GetFileStat( PCChar pszFilename ) { enum { DB=0 };
   FileStat rv;
   // Why is the file size reported incorrectly for files that are still being written to?
   // http://blogs.msdn.com/b/oldnewthing/archive/2011/12/26/10251026.aspx
   // this DOES read return a changing file size for a file being actively written, HOWEVER
   int fh;  // this does not affect st_mtime value returned by stat, though it does cause st_size to change
   if( !fio::OpenFileFailed( &fh, pszFilename, false, false ) ) {
      const auto fbytes( fio::SeekEoF( fh ) );
      rv.Refresh( fh );
      fio::Close( fh );
      DB&&DBG("%s: %" WL( PR__i64 "u", "ld" ) " bytes", pszFilename, fbytes );
      }
   return rv;
   }

FBUF::DiskFileVsFbufStatus FBUF::checkDiskFileStatus() const {
   if( !FnmIsDiskWritable() )           return DISKFILE_SAME_AS_FBUF;
   const auto pName( Name() );
   if( HasWildcard( pName ) )           return DISKFILE_SAME_AS_FBUF;
   const auto diskm( GetFileStat( pName ) );
   if( diskm.none() )                   return DISKFILE_NO_EXIST;
   const auto s1_v_s2( cmp( diskm, d_LastFileStat ) );
   if( s1_v_s2 > 0 )                    return DISKFILE_NEWERTHAN_FBUF;
   if( s1_v_s2 < 0 )                    return DISKFILE_OLDERTHAN_FBUF;
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
                                      && !ConIO::Confirm( "%s has been changed DiskFile %s than buffer:  Refresh? ", Name(), why )
                                    )
                                     {
                                     0 && DBG( "not confirmed" );
                                     SetLastFileStatFromDisk();
                                     return false;
                                     }

                                  if( ReadDiskFileNoCreateFailed() )
                                     return false;

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
   RbNode *pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead,d_link,pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf == pThis )
         return true;
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

//
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
   }
   DB && DBG( "%s is %s will be %s", __func__, g_CurFBuf()?g_CurFBuf()->Name():"", this->Name() );

   FBOP::AssignFromRsrc( this );

   if( d_TabWidth < 2 ) {
      d_TabWidth = g_iTabWidth;
      }

   const auto pCurView( this->PutFocusOnView() );

   DispNeedsRedrawCurWin();

   LuaCtxt_ALL::call_EventHandler( "GETFOCUS" );

   DB && DBG( "%s- %s", __func__, this->Name() );

   return pCurView;
   }

PFBUF FindFBufByName( PCChar pFullName ) {
   Assert( pFullName );
#if FBUF_TREE
   const auto pNd( rb_find_gen( g_FBufIdx, pFullName, rb_strcmpi ) );
   return pNd ? IdxNodeToFBUF( pNd ) : nullptr;
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf) {
      if( pFBuf->NameMatch( pFullName ) )
         return pFBuf;
      }
   return nullptr;
#endif
   }

// indent/softcr

STATIC_FXN bool SliceStrRtnFirstLastTokens( PXbuf pxb, PPCChar pszFirstTok, PPCChar pszLastTok ) {
   auto pC( StrPastAnyBlanks( pxb->c_str() ) );
   if( *pC == 0 )
      return false;

   *pszFirstTok = pC; // save first result

   // We want to detect '{', '}' which are not words, so skip first char,
   // whatever it is, to ensure we don't obliterate a token that has no
   // wordchars.
   //
   pC = StrPastWord( pC+1 );
   if( *pC ) {
      pxb->mid_term( pC-pxb->c_str() ); // place backtrack-stopper
      pC = Eos( pC + 1 );

      while( *(--pC) != 0 && *pC != ' ' )
         continue;

      if( *(++pC) == 0 )
         pC = nullptr;
      }
   else {
      pC = nullptr;
      }

   *pszLastTok = pC;

   0 && DBG( "%s  first='%s'  last='%s'", __func__, *pszFirstTok, *pszLastTok );
   return true;
   }

STATIC_FXN int FindStringMatchingArrayOfString( CPCChar papszStringsToCmp[], PCChar pszTarget, bool fCase ) {
   const auto my_strcmp( fCase ? &strcmp : &Stricmp );
   for( auto papszFirst( papszStringsToCmp ) ; *papszStringsToCmp ; ++papszStringsToCmp ) {
      if( 0 == my_strcmp( *papszStringsToCmp, pszTarget ) ) { 0 && DBG( "'%s' matches '%s'", pszTarget, *papszStringsToCmp );
         return papszStringsToCmp - papszFirst + 1;
         }
      }
   return 0;
   }

STIL int NextIndent( int curIndent, int indentIncr ) {
   const auto rv( ((curIndent+indentIncr) / indentIncr) * indentIncr );
   0 && DBG( "%s %d, %d => %d", __func__, curIndent, indentIncr, rv );
   return rv;
   }

STIL int PrevIndent( int curIndent, int indentIncr )  { return ((curIndent-indentIncr) / indentIncr) * indentIncr; }

COL FBOP::SoftcrForCFiles( PCFBUF fb, COL xCurIndent, LINE yStart, PXbuf pxb ) {
   auto indent( fb->IndentIncrement() );
   if( indent == 0 ) {
      indent = fb->TabWidth();
      if( indent == 0 ) {
         indent = 4;
         }
      }

   PCChar xFirst, xLast;
   if( !SliceStrRtnFirstLastTokens( pxb, &xFirst, &xLast ) ) {  0 && DBG( "%s CurLine empty, -1", __func__ );
      return -1;
      }

   if( xFirst[0] == '}' || (xLast && xLast[0] == '}') ) { 0 && DBG( "%s CurLine 1=}, prevI", __func__ );
      return PrevIndent( xCurIndent, indent );
      }

   if( xLast && xLast[0] == '{' ) { 0 && DBG( "%s CurLine 9={, nextI", __func__ );
      return NextIndent( xCurIndent, indent );
      }

   STATIC_VAR CPCChar c_statement_names[] = {
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
      nullptr    ,
      };

   if( FindStringMatchingArrayOfString( c_statement_names, xFirst, true ) ) { 0 && DBG( "%s CurLine strmatch, nextI", __func__ );
      return NextIndent( xCurIndent, indent );
      }

   // if( yStart > 0 ) { // prev line exists
   //    getLineTabx( pLinedata, yStart - 1 );
   //    if( !SliceStrRtnFirstLastTokens( pLinedata, xFirst, xLast ) ) {
   //       0 && DBG( "%s PrevLine empty, -1", __func__ );
   //       return -1;
   //       }
   //    if( xFirst[0] == '}' || (xLast && xLast[0] == '}') ) {
   //       0 && DBG( "%s PrevLine 1=} || 9=}, prevI", __func__ );
   //       return PrevIndent( xCurIndent, indent );
   //       }
   //    if( FindStringMatchingArrayOfString( c_statement_names, xFirst, true ) ) {
   //       0 && DBG( "%s PrevLine strmatch, prevI", __func__ );
   //       return PrevIndent( xCurIndent, indent );
   //       }
   //    }

   0 && DBG( "%s eoFxn, -1", __func__ );
   return -1;
   }


int FBOP::GetSoftcrIndent( PFBUF fb ) {
   if( !g_fSoftCr )  return 0;

   const auto luaVal( GetSoftcrIndentLua( fb, g_CursorLine() ) );
   if( luaVal >= 0 )  return luaVal;

   const auto yStart( g_CursorLine() );
   Xbuf xb;
   fb->getLineTabx( &xb, yStart );
   COL rv;
   {
   const auto lbuf0( xb.c_str() );
         auto pX( StrPastAnyBlanks( lbuf0 ) );
   pX = *pX ? pX : lbuf0;
   rv = pX - lbuf0;  // Assert( rv >= 0 );
   switch( fb->FileType() ) {
      default: break;
      case ftype1_C: {
           const auto rv_C( FBOP::SoftcrForCFiles( fb, rv, yStart, &xb ) );
           if( rv_C >= 0 ) {  0 && DBG( "SoftCR C: %d", rv_C );
              return rv_C;
              }
           }
           break;
      }
   }
   fb->getLineTabx( &xb, yStart + 1 );
   {
   auto lbuf1( xb.c_str() );  auto pY( StrPastAnyBlanks( lbuf1 ) );
   if( lbuf1[0] && *pY )
      rv = pY - lbuf1;
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
   if( !(g_CurFBuf()->FnmIsPseudo()) )
      g_CurFBuf()->ToglNoEdit();

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
   RbNode *pNd;
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
   RbNode *pNd;
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

STATIC_FXN bool RmvFileByName( PCChar filename ) {
   const auto pfToRm( FindFBufByName( filename ) );
   if( !pfToRm )
      return Msg( "no FBUF for '%s'", filename );

   if( pfToRm->IsDirty() && !ConIO::Confirm( "Forget DIRTY FBUF '%s'?", filename ) )
      return Msg( "Dirty FBUF not forgotten '%s'", filename );

   const auto fDidRmv( DeleteAllViewsOntoFbuf( pfToRm ) );
   Msg( "%s FBUF '%s'", fDidRmv ? "Forgot" : "COULDN'T FORGET", filename );
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
                     )
                      return false;

                   pcf->ReadDiskFileNoCreateFailed();
                   DispNeedsRedrawStatLn();
                   return true;
                   }

    case NULLARG:  if( DLINK_NEXT( g_CurView(), dlinkViewsOfWindow ) == nullptr )
                      return fnMsg( "no alternate file" );

                   if( !ConIO::Confirm( "Forget the current file (from all windows)?! " ) )
                      return false;

                   KillTheCurrentView();
                   DispNeedsRedrawAllLinesAllWindows();
                   SetWindowSetValidView( -1 );
                   return true;

    case TEXTARG:  {
                   const auto rv( RmvFileByName( d_textarg.pText ) );
                   RefreshCurFileIfMatchesPseudoName( szFiles );
                   return rv;
                   }

    case BOXARG:   //lint -fallthrough
    case LINEARG:  {
                   auto attemptCount( 0 );
                   auto rmvCount( 0 );
                   for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                      if( aw.GetLine() ) {
                         rmvCount += RmvFileByName( aw.c_str() );
                         ++attemptCount;
                         }
                      }
                   fnMsg( "Forgot %d of %d files", rmvCount, attemptCount );
                   RefreshCurFileIfMatchesPseudoName( szFiles );
                   return (rmvCount > 0) && (rmvCount == attemptCount);
                   }
    }
   }

int DoubleBackslashes( PChar pDest, size_t sizeofDest, PCChar pSrc ) {
   const auto rv( pDest );
   const auto pLastNul( pDest + sizeofDest - 1 );
   while( pDest < pLastNul && (*pDest++ = *pSrc) != 0 ) {
      if( *pSrc == '\\' )
         *pDest++ = '\\';

      ++pSrc;
      }

   *pLastNul = '\0';
   return pDest - rv; // strlen of resulting string
   }

PChar xlatCh( PChar pStr, int fromCh, int toCh ) {
   const auto rv( pStr );
   for( ; *pStr ; ++pStr )
      if( *pStr == fromCh )
          *pStr =  toCh;

   return rv;
   }

Path::str_t FBOP::GetRsrcExt( PCFBUF fb ) {
   Path::str_t rv;
   if( FnmIsLogicalWildcard( fb->Name() ) ) {
      rv = ".*";
      }
   else {
      rv = Path::CpyExt( fb->Name() );
      if( rv.empty() )
         rv = !fb->FnmIsDiskWritable() ? ".<>" : ".";
      }

#if !FNM_CASE_SENSITIVE
   string_tolower( rv );
#endif

   0 && DBG( "%s '%s'", __func__, rv.c_str() );
   return rv;
   }

STATIC_FXN bool DefineStrMacro( PCChar pszMacroName, PCChar pszMacroString ) { 0 && DBG( "%s '%s'='%s'", __func__, pszMacroName, pszMacroString );
   const std::string str( "\"" + std::string(pszMacroString) + "\"" );
   return DefineMacro( pszMacroName, str.c_str() );
   }

void FBOP::AssignFromRsrc( PCFBUF fb ) {  0 && DBG( "%s '%s'", __func__, fb->Name() );
   // 1. assigns "curfile..." macros based on this FBUF
   // 2. loads rsrc file section for extension of this FBUF
#if MACRO_BACKSLASH_ESCAPES
   dbllinebuf dblbuf;
   Pathbuf pbuf( fb->Name() );
   DoubleBackslashes( BSOB(dblbuf), pbuf );
   DefineStrMacro( "curfile", dblbuf );
#else
   DefineStrMacro( "curfile", fb->Name() );
#endif
   DefineStrMacro( "curfilename", Path::CpyFnm  ( fb->Name() ).c_str() );
   DefineStrMacro( "curfilepath", Path::CpyDirnm( fb->Name() ).c_str() );
   const auto ext( FBOP::GetRsrcExt( fb ) );
   DefineStrMacro( "curfileext", ext.c_str() );
   if( !fb->IsRsrcLdBlocked() ) {
      LoadFileExtRsrcIniSection( ext.c_str() ); // call only after curfile, curfilepath, curfilename, curfileext assigned
      }
   }


STATIC_FXN Path::str_t xlat_fnm( PCChar pszName ) {
   auto rv( CompletelyExpandFName_wEnvVars( pszName ) );
   if( rv.empty() ) {
      ErrorDialogBeepf( "Cannot access %s - %s", pszName, strerror( errno ) );
      const auto fb( FindFBufByName( pszName ) );
      if( fb )  DeleteAllViewsOntoFbuf( fb );
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
   if( fnamebuf.empty() )
      return nullptr;

   CPCChar pFnm( fnamebuf.c_str() );                             DP && DBG( "OFND xlat='%s'", pFnm );
   auto pFBuf( FindFBufByName( pFnm ) );
   if( !pFBuf ) {
      if( !FBUF::FnmIsPseudo( pFnm ) ) {
         FileAttribs fa( pFnm );
         if( !fa.Exists() ) {
            if( fCreateOk )
               goto ADD_FBUF;
                                                                 DP && DBG( "OFND! NoD '%s'", pFnm );
            Msg( "File '%s' does not exist", pFnm );
            return nullptr;
            }
         if( fa.IsDir() ) {                                      DP && DBG( "OFND! isD '%s'", pFnm );
            Msg( "%s does not open directories like %s", __func__, pFnm );
            return nullptr;
            }
         }
ADD_FBUF:
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
#if defined(_WIN32)
      SetCwdChanged( cwdAfter.c_str() );
#endif
      Msg( "Changed directory to %s", cwdAfter.c_str() );
      if( fSave )  g_pFBufCwd->InsLine( 0, cwdBefore.c_str() );
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
   auto pFBuf( FindFBufByName( pFnm ) );     DP && DBG( "%s '%s' -->PFBUF %p", __func__, pFnm, pFBuf );
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

char Path::DelimChar( PCChar fnm ) {
   if( !strchr( fnm, ' '  ) )  return 0;    // no delim needed
   if( !strchr( fnm, '"'  ) )  return '"';  // "
   if( !strchr( fnm, '\'' ) )  return '\''; // '
   return '|'; // last ditch: ugly, but NEVER a valid filename char(?)
   }

char FBUF::UserNameDelimChar() const {
   return Path::DelimChar( Name() );
   }

PChar FBUF::UserName( PChar dest, size_t destSize ) const {
   const char delimCh( UserNameDelimChar() );
   if( delimCh )
      safeSprintf( dest, destSize, "%c%s%c", delimCh, Name(), delimCh );
   else
      safeStrcpy(  dest, destSize, Name() );
   return dest;
   }

// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
// END   SUPPORT FILENAMES WITH SPACES    SUPPORT FILENAMES WITH SPACES
//
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

void FBUF::SetDirty( bool fDirty ) {
   d_fDirty = fDirty;
   #define  AUTOHIDE_TABS_WHEN_UNDIRTY  1
   #if      AUTOHIDE_TABS_WHEN_UNDIRTY
   // hacky attempt to hide this if the user is just browsing code
   const auto td_before( d_fTabDisp );
   d_fTabDisp = fDirty;
   d_fTrailDisp = fDirty;
   if( ViewCount() && td_before != fDirty ) {
      0 && DBG( "about to call DispNeedsRedrawCurWin();" );
      DispNeedsRedrawCurWin();
      }
   #endif
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

bool FBUF::FBufReadOk( bool fAllowDiskFileCreate, bool fCreateSilently ) {
   VR_( DBG( "FRd+ %s", Name() ); )

// if( !Interpreter::Interpreting() )  20061003 klg commented out since calling mfgrep inside a macro (as I'm doing now) hides this, which I don't want.
      DialogLdFile( Name() );

   d_EolMode = platform_eol;
                                                               0 && DBG( "%s+ %s'", FUNC, Name() );
   if( FnmIsLogicalWildcard( Name() ) ) {                      0 && DBG( "%s+ %s' FnmIsLogicalWildcard", FUNC, Name() );
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
   if( Strcmp( canonFnm.c_str(), Name() ) ) { // NB: CASE-SENSITIVE compare intended!
       ChangeName( canonFnm.c_str() );
       }
#else
   // Linux filesys is case-sensitive so this scenario cannot happen
#endif

   //###########  disk file read !!!

   rdNoiseOpen();

   DisplayNoiseBlanker dblank;

   int hFile;
   if( fio::OpenFileFailed( &hFile, Name(), false, false ) ) {
      if( errno != ENOENT ) {
         return Msg( "Cannot open %s - %s", Name(), strerror( errno ) );
         }

      if(    !fAllowDiskFileCreate
         || (!fCreateSilently && !ConIO::Confirm( "%s does not exist. Create? ", Name() ))
        ) {
         // SW_BP;
         DBG( "FRd! user denied create" );
         return false;
         }

      if( fio::OpenFileFailed( &hFile, Name(), false, true ) )
         return Msg( "Cannot create %s - %s", Name(), strerror( errno ) );

      VR_( DBG( "FRd: created newfile '%s'", Name() ); )

      PutLine( 0, "" );
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
      if( !g_fEditReadonly )
         SetNoEdit();
      }
   else {
      VR_( DBG( "FRd: is NOT RO_FILE" ); )
      SetDiskRW();
      }
#endif

   SetLastFileStatFromDisk();

   //---------------------------------------------------------------------------------

   STATIC_CONST
   struct {
      PCChar    ext;
      eFileType type;
      }
   ext_2_type[] = {
      { ".c"  , ftype1_C },
      { ".cpp", ftype1_C },
      { ".cxx", ftype1_C },
      { ".h"  , ftype1_C },
      { ".hpp", ftype1_C },
      };

   const auto pb( Path::CpyExt( Name() ) );
   0 && DBG( "%s EXT=%s", __func__, pb.c_str() );
   for( const auto &e2t : ext_2_type ) {
      if( Stricmp( e2t.ext, pb.c_str() ) == 0 ) {
         0 && DBG( "%s TYPE=%s (%d)", __func__, e2t.ext, e2t.type );
         SetFileType( e2t.type );
         break;
         }
      }

   // CalcIndent();

   //---------------------------------------------------------------------------------

   VR_( DBG( "FRd- OK" ); )
   return true;
   }

bool FBUF::ReadOtherDiskFileNoCreateFailed( PCChar pszName ) {
   rdNoiseOpen();

   int hFile;
   if( fio::OpenFileFailed( &hFile, pszName, false, false ) ) {
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
   safeStrcpy( dest, sizeofDest, kszBackupMode( g_iBackupMode ) );
   }

PCChar swixBackup( PCChar param ) {
   if(      Stricmp( param, "bak"   ) == 0 )  { g_iBackupMode = bkup_BAK   ; } // backup:none
   else if( Stricmp( param, "undel" ) == 0 )  { g_iBackupMode = bkup_UNDEL ; } // backup:bak
   else if( Stricmp( param, "none"  ) == 0 )  { g_iBackupMode = bkup_NONE  ; } // backup:undel
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
                           return Msg( "Can't delete %s - %s", Path::CpyFnameExt( BAK_FileNm.c_str() ).c_str(), strerror( errno ) );
                           }
                        }
#endif
                     if( !MoveFileOk( fnmToBkup, BAK_FileNm.c_str() ) ) {
                        const auto emsg( strerror( errno ) );
                        if( FileOrDirExists( fnmToBkup ) )
                           return Msg( "Can't rename %s to %s - %s", fnmToBkup, BAK_FileNm.c_str(), emsg );
                        }
                     } break;

    case bkup_UNDEL: // The old file is moved to a child directory.  The number of copies saved is
                     { VERBOSE_WRITE && DBG( "FWr: bkup_UNDEL" ); // specified by the Undelcount switch.
                     const auto rc( SaveFileMultiGenerationBackup( fnmToBkup ) );
                     switch( rc ) {
                        default:               return Msg( "Can't delete old version of %s", fnmToBkup );
                        case SFMG_OK:          break;
                        case SFMG_NO_EXISTING: break;
                        }
                     }
                     //-lint fallthrough
    case bkup_NONE:  // No backup: the editor overwrites the file.
                     // here we delete the file to be overwritten
                     VERBOSE_WRITE && DBG( "FWr: bkup_NONE" );
                     if( !unlinkOk( fnmToBkup ) ) {
                        const auto emsg( strerror( errno ) );
                        if( FileOrDirExists( fnmToBkup ) )
                           return Msg( "Can't delete %s - %s", fnmToBkup, emsg );
                        }
                     break;
    }
   return true;
   }


bool FBUF::write_to_disk( PCChar destFileNm ) {
   // BUGBUG there are security-hazard file/directory race conditions to be found here!
   Path::str_t destFnm( destFileNm );

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
   PChar pStream = strchr( destFnm+3, ':' );
   if( pStream )  *pStream = '\0';
#endif

#if defined(_WIN32)
   {
   FileAttribs dest( destFnm.c_str() );
   if( dest.Exists() && dest.IsReadonly() ) {
      if(   ConIO::Confirm( "File '%s' is readonly; overwrite anyway?", destFnm.c_str() )
         && dest.MakeWritableFailed( destFnm.c_str() )
        )
         return Msg( "Could not make '%s' writable!", destFnm.c_str() );

      if( IsFileReadonly( destFnm.c_str() ) ) {
         return Msg( "'%s' is not writable", destFnm.c_str() );
         }
      }
   }
#endif

   const auto tmpFnm( Path::Union( ".$k$", destFnm.c_str() ) );
   DisplayNoiseBlanker dblank;

   extern bool FBUF_WriteToDiskOk( PFBUF pFBuf, PCChar pszDestName );
   if( !FBUF_WriteToDiskOk( this, tmpFnm.c_str() ) )
      return false;

   wrNoiseBak();

   const auto abs_dest( Path::Absolutize( destFnm.c_str() ) );
   if( !backupOldDiskFile( abs_dest.c_str(), d_backupMode==bkup_USE_SWITCH ? g_iBackupMode : d_backupMode ) ) {
      unlinkOk( tmpFnm.c_str() );
      return false;
      }

   wrNoiseRenm();

   if( !MoveFileOk( tmpFnm.c_str(), destFnm.c_str() ) ) {
      Msg( "Can't rename %s to %s - %s", tmpFnm.c_str(), destFnm.c_str(), strerror( errno ) );
      unlinkOk( tmpFnm.c_str() );
      return false;
      }

   if( !NameMatch( destFnm.c_str() ) )
      ChangeName( destFnm.c_str() ); //

#if defined(_WIN32)
   SetDiskRW(),
#endif
   UnDirty(), SetLastFileStatFromDisk(); DispNeedsRedrawStatLn();
   return !Msg( "Saved  %s", Name() ); // xtra spc to match Msg( "Saving %s" ... above
   }


void FBUF::ChangeName( PCChar newName ) { // THE ONLY PLACE WHERE AN FBUF's NAME MAY BE SET!!!
   d_filename = newName;
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

   const auto fFnmChanging( dest[0] && !NameMatch( dest.c_str() ) );
   Path::str_t pseudoFnm;
   PPFBUF gp( nullptr );  // if it has a gp, it is a pseudo file
   if( !fFnmChanging )             dest      = Name();
   else if( (gp=GetGlobalPtr()) )  pseudoFnm = Name();  // if it is a pseudo file w/a gp, it needs to hang around

   if( fFnmChanging ) { VERBOSE_WRITE && DBG( "FWr+ %s -> %s", Name(), dest.c_str() ); }
   else               { VERBOSE_WRITE && DBG( "FWr+ %s", Name() );                           }

   if( !FnmIsDiskWritable() && !fFnmChanging ) // unwritable FBUF not being saved to alt file?
      return Msg( "Can't save pseudofile to disk under it's own name; use 'arg arg \"diskname\" setfile'" );

   VERBOSE_WRITE && DBG( "FWr: dst=%s", dest.c_str() );

   if( IsDir( dest.c_str() ) )
      return Msg( "Cannot write '%s'; directory in the way", dest.c_str() );

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
      FBOP::FindOrAddFBuf( pseudoFnm.c_str(), gp ); // associate GlobalPtr with a new FBUF created specifically to bear the old name
      }
   return saveOk;
   }

bool FBUF::SaveToDiskByName( PCChar pszNewName, bool fNeedUserConfirmation ) {
   if( pszNewName[0] == '\0' )
      return Msg( "new filename is empty?" );

   const auto filenameBuf( CompletelyExpandFName_wEnvVars( pszNewName ) );
   {
   auto pDupFBuf( FindFBufByName( filenameBuf.c_str() ) );
   if( pDupFBuf && pDupFBuf == this )
      return Msg( "current filename and new filename are same; nothing done" );

   if( fNeedUserConfirmation && !ConIO::Confirm( "Do you want to save this file as %s ?", filenameBuf.c_str() ) ) {
      FlushKeyQueuePrimeScreenRedraw();
      return false;
      }

   if( pDupFBuf ) {
      if( pDupFBuf->IsDirty() && !ConIO::Confirm( "You are currently editing the new-named file in a different buffer!  Destroy edits?!" ) )
         return false;

      pDupFBuf = pDupFBuf->ForciblyRemoveFBuf();
      }
   }

   if( !WriteToDisk( filenameBuf.c_str() ) ) { DBG( "%s: WriteToDisk( '%s' -> '%s' ) FAILED?", __func__, Name(), filenameBuf.c_str() );
      return false;
      }

   SetRememberAfterExit();

   0 && DBG( "%s: wrote( '%s' -> '%s' ) OK", __func__, Name(), filenameBuf.c_str() );
   return true;
   }

STATIC_FXN bool IfOnlyOneFilespecInCurWcFileSwitchToIt() {
   // if only ONE filespec matched, open that file
   const auto pFBuf( g_CurFBuf() ); // new wild-pseudofile should have been made current
   if( pFBuf->LineCount() != 1 )
      return true;

   Path::str_t fnm;
   if( pFBuf->GetLineIsolateFilename( fnm, 0, 0 ) < 1 ) // read first line from pseudofile
      return Msg( "GetLineIsolateFilename(0) failed!" );

   const auto pFn( fnm.c_str() );
   if( IsDir( pFn ) )
      return true;

   const auto new_pFbuf( OpenFileNotDir_NoCreate( pFn ) );  // switch to the only file
   if( !new_pFbuf )
      return Msg( "Failure switching to top file!" );

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
           )
            return true;
         }
      return false;
      }
   };


STATIC_FXN bool FileWrErr( int hFile_Write, PCChar pszDestName, PCChar Dialog_fmtstr ) {
   fio::Close( hFile_Write );
   unlinkOk( pszDestName );
   if( Dialog_fmtstr )
      Msg( Dialog_fmtstr, pszDestName );

   FlushKeyQueuePrimeScreenRedraw();
   return false;
   }

PCChar EolName( Eol_t eol ) { return eol==EolLF ? "LF" : "CRLF"; }

GLOBAL_VAR bool g_fTrailLineWrite   = false;
GLOBAL_VAR bool g_fForcePlatformEol = false;

bool FBUF_WriteToDiskOk( PFBUF pFBuf, PCChar pszDestName ) { enum {DB=0}; // hidden/private FBUF method
   wrNoiseOpen();
   int hFile_Write;
   if(   fio::OpenFileFailed( &hFile_Write, pszDestName, true, false )
      && fio::OpenFileFailed( &hFile_Write, pszDestName, true, true  )
     ) {
      return Msg( "Cannot open or create %s - %s", pszDestName, strerror( errno ) );
      }

   wrNoiseWrite();

   const auto fForced( g_fForcePlatformEol && pFBuf->EolMode() != platform_eol );
   if( fForced ) {
      pFBuf->SetEolModeChanged( platform_eol );
      }
   Msg( "Saving %s%s", pszDestName, fForced ? " with platform EOL forced":"" );
   PCChar pszEolStr;  size_t eolStrlen;
   if( pFBuf->EolMode() == EolLF ) { STATIC_CONST char   lf[] =     "\x0A"; pszEolStr = lf  ; eolStrlen = KSTRLEN(lf  ); DB&&DBG( "%s w/LF%s"  , __func__, fForced ? " forced":"" ); }
   else                            { STATIC_CONST char crlf[] = "\x0D\x0A"; pszEolStr = crlf; eolStrlen = KSTRLEN(crlf); DB&&DBG( "%s w/CRLF%s", __func__, fForced ? " forced":"" ); }

   BufdWr::ResetVars();

   auto maxLine( pFBuf->LineCount() );
   if( !g_fTrailLineWrite ) {
      while( maxLine > 0 )
         if( !FBOP::IsLineBlank( pFBuf, --maxLine ) )
            break;

      ++maxLine;
      }

   for( auto yLine( 0 ); yLine < maxLine; ++yLine ) {
      const auto src( pFBuf->PeekRawLine( yLine ) );
      if( ExecutionHaltRequested() ) {
         return FileWrErr( hFile_Write, pszDestName, "User break writing '%s'" );
         }

      if(   (src.length() && BufdWr::WrFailed( hFile_Write, src.data(), src.length() ))
         ||                  BufdWr::WrFailed( hFile_Write, pszEolStr , eolStrlen    )
        ) {
         return FileWrErr( hFile_Write, pszDestName, ExecutionHaltRequested() ? "User break writing '%s'" : "Out of space on '%s'" );
         }
      }

   if( BufdWr::FlushFailed( hFile_Write ) )
      return FileWrErr( hFile_Write, pszDestName, ExecutionHaltRequested() ? "User break writing '%s'" : "Out of space on '%s'" );

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
      FBOP::AssignFromRsrc( g_CurFBuf() );
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
                  if( !nxtvw ) return fnMsg( "no alternate file" );
                  fnm = nxtvw->FBuf()->Name();
                  } break; //-------------------------------------------------------

    case NULLARG: if( d_cArg > 1 ) {
                     if( !d_fMeta && d_cArg >= 3 )                 return fExecute( "wrstatefile saveall" );
                     if( d_cArg == 2 && !g_CurFBuf()->IsDirty() )  return !fnMsg( "current file not dirty" );
                     return g_CurFBuf()->WriteToDisk() == false;   // *** 'arg arg setfile' saves current file to disk
                     }

                  if( g_CurFBuf()->GetLineIsolateFilename( fnm, d_nullarg.cursor.lin, d_nullarg.cursor.col ) < 1 ) // read first line from pseudofile
                     return false;
                  break; //-------------------------------------------------------

    case TEXTARG: if( d_cArg >= 2 ) { // save file with new name, pFBuf takes on new name identity
                     if( !FnmIsLogicalWildcard( d_textarg.pText ) )
                        return RenameCurFileOk( d_textarg.pText, !d_fMeta );

                     // ADD TBD special handling for wildcard specs when d_cArg >= 2
                     }
                  fnm = d_textarg.pText;
                  break; //-------------------------------------------------------
    }
   0 && DBG( "%s: %s", __func__, fnm.c_str() );

   if( d_argType != NOARG ) { // for NOARG we HAVE a definitive name, skip the following reinterpretations
      if( LuaCtxt_Edit::ExecutedURL( fnm.c_str() ) )  // WINDOWS-ONLY HOOK
         return true;

      LuaCtxt_Edit::ExpandEnvVarsOk( fnm );
      SearchEnvDirListForFile( fnm, true );
      }

   const auto rv( fChangeFileIfOnlyOneFilespecInCurWcFileSwitchToIt( fnm.c_str() ) );
   DBG( "### %s t=%9.6f %s", __func__, pc.Capture(), fnm.c_str() );
   0 && DBG( "%s: %s", __func__, fnm.c_str() );
   return rv;
   }

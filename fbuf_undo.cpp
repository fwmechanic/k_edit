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

//*********************************************************************************************************************************
//*********************************************************************************************************************************
//*********************************************************************************************************************************
//
//       LOW-LEVEL UNDO/REDO CODE
//
//*********************************************************************************************************************************
//*********************************************************************************************************************************
//*********************************************************************************************************************************

#define  USE_DBGF_EDOPS     0

#if      USE_DBGF_EDOPS
#define  DBG_EDOP( x )      x
#else
#define  DBG_EDOP( x )
#endif

STATIC_CONST char kszPCur [] = ">>****>> PCur >>****>>";
STATIC_CONST char kszBound[] = "----------------------";
STATIC_CONST char kszSpcs [] = "                      " DBG_EDOP( "    " );


STATIC_FXN void getLineInfoStr( PPChar ppBuf, size_t *pBufbytes, const LineInfo &li ) {
   if( li.GetLineRdOnly() != nullptr ) snprintf_full( ppBuf, pBufbytes, " %p \"%.*s", li.GetLineRdOnly() , li.GetLineLen() , li.GetLineRdOnly() );
   else                                snprintf_full( ppBuf, pBufbytes, DBG_EDOP( "          " ) " $" ); // invalid line ptr => blank line
   }


#define  UNDO_REDO_MARKS  0
#if      UNDO_REDO_MARKS
#define     IF_UNDO_REDO_MARKS( x )  x
#else
#define     IF_UNDO_REDO_MARKS( x )
#endif

#if UNDO_REDO_MARKS

STATIC_FXN void copyUndoMarks( PFBUF pFBuf, NamedPointHead &SavedMarkList, int LineAdjustValue ) {
   if( SavedMarkList.empty() ) {
      return;
      }

   // PMarkBuf newMarks = PMarkBuf( Alloc0d( SavedMarkList->bufBytes ) );
   // memcpy( newMarks, SavedMarkList, SavedMarkList->bufBytes );

   // adjAndCopyMarksToCache( pFBuf, newMarks, 0, LineAdjustValue );
   }

#endif //UNDO_REDO_MARKS

//---------------------------------------------------------------------------------------------------------------------
//
// utility interface to allow output to be directed to PFBUF, dbgview (Windows OutputDebugString()), or both

struct OutputWriter {
   virtual void VWriteLn( PCChar string ) const = 0;
   };

struct FbufWriter : public OutputWriter {
   PFBUF d_pFBuf;
   FbufWriter( PFBUF pFBuf ) : d_pFBuf(pFBuf) {}
   virtual void VWriteLn( PCChar string ) const { d_pFBuf->PutLastLine( string ); }
   };

struct DBGWriter : public OutputWriter {
   virtual void VWriteLn( PCChar string ) const;
   };

struct DBGFbufWriter : public OutputWriter {
   PFBUF d_pFBuf;
   DBGFbufWriter( PFBUF pFBuf ) : d_pFBuf(pFBuf) {}
   virtual void VWriteLn( PCChar string ) const;
   };
//
//---------------------------------------------------------------------------------------------------------------------

void DBGWriter::VWriteLn( PCChar string ) const { DBG( "%s", string ); }

void DBGFbufWriter::VWriteLn( PCChar string ) const {
   DBG( "%s", string );
   d_pFBuf->PutLastLine( string );
   }

//----------------------------------------------------------------------------


class EditRec { // abstract base class   public EditRec
protected:
   const PFBUF d_pFBuf;
   EditRec( PFBUF pFBuf, int only );
   EditRec( PFBUF pFBuf );
private:
   DBG_EDOP( STATIC_VAR unsigned d_SerNumSeed; )
   DBG_EDOP( const      unsigned d_SerNum; )
public:
   EditRec  *d_pRedo; // public cuz clients walk EditRec lists themselves
   EditRec  *d_pUndo; // public cuz clients walk EditRec lists themselves
   void Dbgf() const;
   virtual     ~EditRec() = 0;
   virtual void VUndo()    = 0;
   virtual void VRedo()    = 0;
   virtual void VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const = 0; // for ARG::eds
   // for PutUndoBoundary
   //
   virtual bool VIsBoundary()    const  { return false; } // rtn true iff isA EdOpBoundary
   virtual bool VUpdtBoundary()         { return false; } // rtn true (and updated content) iff isA EdOpBoundary
   // for SetUndoStateToBoundrec
   //
   virtual bool VUpdtFromBoundary()     { return false; } // rtn true (and updated PFBUF from content) iff isA EdOpBoundary
   // for UndoReplaceLineContent
   //
   virtual bool VIsAltContentOfLine( LINE lineNum ) { return false; }
protected:
   NO_COPYCTOR(EditRec);
   NO_ASGN_OPR(EditRec);
   };

DBG_EDOP( unsigned EditRec::d_SerNumSeed; )

EditRec::~EditRec() {} // defn for pure virtual destructor

EditRec::EditRec( PFBUF pFBuf, int only_placeholder )
   : d_pFBuf(pFBuf)
#if  USE_DBGF_EDOPS
   , d_SerNum(++d_SerNumSeed) )
#endif
   {
   d_pRedo = nullptr;
   d_pUndo = nullptr;
   d_pFBuf->d_UndoCount    = 0;
   d_pFBuf->d_pNewestEdit  = this;
   d_pFBuf->d_pCurrentEdit = this;
   d_pFBuf->d_pOldestEdit  = this;
   }

//
// 'this' is always a newly heap-allocated EditRec which is to receive the
//       contents of 'src', and then be linked into the EditOp list at
//       pFBuf->d_pNewestEdit
//

void DBGEditOp( const EditRec *er );

EditRec::EditRec( PFBUF pFBuf )
   : d_pFBuf(pFBuf)
#if  USE_DBGF_EDOPS
   , d_SerNum(++d_SerNumSeed) )
#endif
   {
   d_pFBuf->AddNewEditOpToListHead( this );
   }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class EdOpAltLineContent : public EditRec {
   const LINE     d_lineNum;   // number of line that was operated on
         LINE     d_fbufLines; // length of file beforehand
         LineInfo d_li;        // the lineContent
   IF_UNDO_REDO_MARKS( NamedPointHead d_MarkListHd; )
   void swapContent() {
      Assert( d_pFBuf->d_LineCount == d_fbufLines ); // UPDT: this HAS blown!
      d_pFBuf->FBufEvent_LineInsDel( d_lineNum, 0 );
      std::swap( d_pFBuf->d_paLineInfo[ d_lineNum ], d_li );
      std::swap( d_pFBuf->d_LineCount, d_fbufLines ); // this seems redundant, but IS NOT in edge cases!
      }
public:
   EdOpAltLineContent( PFBUF pFBuf, LINE lineNum, LineInfo *pLineInfo )
      : EditRec    (      pFBuf         )
      , d_lineNum  ( lineNum            )
      , d_fbufLines( pFBuf->LineCount() )
      , d_li       ( std::move( *pLineInfo ) ) // after this, *pLineInfo contains empty value!  Somebody'd better be OVERWRITING *pLineInfo (which is intended to lie within FBUF::d_paLineInfo[]) with a non-empty value before it's dereferenced again!
      {
      IF_UNDO_REDO_MARKS( d_MarkListHd = updateMarksForLineDeletion_DupMarks( pFBuf, lineNum, lineNum ); )
      }
   ~EdOpAltLineContent() { d_li.FreeContent( *d_pFBuf ); }
   void VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const override {
      snprintf_full(  ppBuf, pBufBytes, DBG_EDOP( "%03X " ) "Replace Line %3d      "
   #if USE_DBGF_EDOPS
         , d_SerNum
   #endif
         , d_lineNum + 1
         );
      getLineInfoStr( ppBuf, pBufBytes, d_li );
      }
   void VUndo()                             override { swapContent(); }
   void VRedo()                             override { swapContent(); }
   bool VIsAltContentOfLine( LINE lineNum ) override { return d_lineNum == lineNum; }
   };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class EdOpLineRangeInsert : public EditRec {
   const LINE d_fileLength; // length of file beforehand
   const LINE d_firstLine;  // number of first line that was operated on
   const LINE d_linesInserted;  // number of lines inserted
public:
   EdOpLineRangeInsert( PFBUF pFBuf, LINE firstLine, int lineCount );
   // ~EdOpLineRangeInsert() {} // does nothing
   void VUndo() override;
   void VRedo() override;
   void VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const override;
   };

EdOpLineRangeInsert::EdOpLineRangeInsert( PFBUF pFBuf, LINE firstLine, int linesInserted )
   : EditRec        ( pFBuf              )
   , d_fileLength   ( pFBuf->LineCount() )
   , d_firstLine    ( firstLine          )
   , d_linesInserted( linesInserted      )
   {
   }

void EdOpLineRangeInsert::VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const {
   snprintf_full( ppBuf, pBufBytes, DBG_EDOP( "%03X " ) "Insert Lines %3d L %3d"
#if USE_DBGF_EDOPS
      , d_SerNum
#endif
      , d_firstLine + 1
      , d_linesInserted
   // , d_fileLength
      );
   }

void EdOpLineRangeInsert::VUndo() {
   d_pFBuf->DeleteLines__ForUndoRedo( d_firstLine, d_firstLine + d_linesInserted - 1 );
   d_pFBuf->SetLineCount( d_fileLength );
   }

void EdOpLineRangeInsert::VRedo() {
   d_pFBuf->InsertLines__ForUndoRedo( d_firstLine, d_linesInserted );
   d_pFBuf->SetLineCount( d_fileLength + d_linesInserted );
   }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class EdOpLineRangeDelete : public EditRec {
   const LINE      d_fileLength; // length of file beforehand
   const LINE      d_firstLine;  // number of first line that was operated on
   const LINE      d_linesDeleted;  // number of lines deleted
         LineInfo *d_paLi;
   IF_UNDO_REDO_MARKS( NamedPointHead  d_MarkListHd; )
public:
   EdOpLineRangeDelete( PFBUF pFBuf, LINE firstLine_, LINE lastLine );
   ~EdOpLineRangeDelete();
   void VUndo() override;
   void VRedo() override;
   void VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const override;
   };

EdOpLineRangeDelete::EdOpLineRangeDelete( PFBUF pFBuf, LINE firstLine_, LINE lastLine )
   : EditRec        ( pFBuf )
   , d_fileLength   ( pFBuf->LineCount() )
   , d_firstLine    ( firstLine_ )
   , d_linesDeleted ( lastLine - firstLine_ + 1 )
   , d_paLi         ( DupArray( pFBuf->d_paLineInfo + d_firstLine, d_linesDeleted ) )
   {
   IF_UNDO_REDO_MARKS( d_MarkListHd = updateMarksForLineDeletion_DupMarks( pFBuf, firstLine_, lastLine ); )
   }

EdOpLineRangeDelete::~EdOpLineRangeDelete() {
#if 1
   Free0( d_paLi ); // DON'T free what d_paLi[..] points to!
#else
   // This ...
   //
   DestroyLineInfoArray( paLi, &cLine );
   //
   // ...  would seem to be correct (in terms of avoiding memory leakage), BUT
   // LEADS TO A CRASH when you paste a line past eof, then delete it, then undo
   // everything and type a graphic (to trigger undo-list garbage collection).
   //
   // The issue: there can be an EdOpAltLineContent instance which, when in
   // undone state, contains a LineInfo which is also in a later (orig xeq time)
   // EdOpLineRangeDelete.d_paLi.  When, after undo'ing some/all, we execute a
   // modifying CMD, the "undone side" of the EditRec list is destroyed (using
   // RmvOneEdOp_fNextIsBoundary in d_pFBuf->AddNewEditOpToListHead( this )
   // within EditRec(pFBuf)).  In this case the EdOpLineRangeDelete is destroyed
   // first; when the EdOpAltLineContent is destroyed, a double free occurs.
   //
   // The current solution is to NOT FREE paLi in ~EdOpLineRangeDelete().  But
   // in the normal case this causes a MEMORY LEAK.
   //
#endif
   }

void EdOpLineRangeDelete::VUndo() {
   d_pFBuf->InsertLines__ForUndoRedo( d_firstLine        , d_linesDeleted );
   MoveArray( d_pFBuf->d_paLineInfo + d_firstLine, d_paLi, d_linesDeleted );
   d_pFBuf->SetLineCount( d_fileLength );
   IF_UNDO_REDO_MARKS( copyUndoMarks( d_pFBuf, d_MarkListHd, d_firstLine ); )
   }

void EdOpLineRangeDelete::VRedo() {
   d_pFBuf->DeleteLines__ForUndoRedo( d_firstLine, d_firstLine + d_linesDeleted - 1 );
   d_pFBuf->SetLineCount( d_fileLength - d_linesDeleted );
   }

void EdOpLineRangeDelete::VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const {
   const auto savepBuf( *ppBuf );
   const auto saveBufBytes( *pBufBytes );
   snprintf_full( ppBuf, pBufBytes, DBG_EDOP( "%03X " ) "Delete Lines %3d L %3d"
#if USE_DBGF_EDOPS
      , d_SerNum
#endif
      , d_firstLine+1
      , d_linesDeleted
   // , d_fileLength
      );
   if( d_linesDeleted > 0 ) {
      getLineInfoStr( ppBuf, pBufBytes, d_paLi[0] );
      for( auto line(1); line < d_linesDeleted; ++line ) {
         ow.VWriteLn( savepBuf );
         *ppBuf     = savepBuf;
         *pBufBytes = saveBufBytes;
         snprintf_full(  ppBuf, pBufBytes, kszSpcs );
         getLineInfoStr( ppBuf, pBufBytes, d_paLi[line] );
         }
      }
   }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//  Undo design note: the undo information (which is rightly per-FBUF)
//  _includes_ (in EdOpBoundary) cursor location.  However, different Views on
//  the same FBUF maintain separate cursor positions.  If the user executes an
//  un/redo step, the active View will suffer a cursor location change (in a way
//  that makes sense).  Any other views on the affected FBUF will suffer a
//  relative cursor movement if the undone action includes line insertion or
//  deletion at lines "above" the cursor.
//
//  Design loose-end: starting with a new file: paste a line past EOF, then
//  undo; leaves file length at grown value (this is very visible since
//  g_chTrailLineDisp was added).  Since file is not dirty, there's no liklihood
//  of it being saved, so only the visual aspect is relevant.
//

class EdOpBoundary : public EditRec {
   const PCChar d_pszCmdName;
         bool   d_fDirty;       // copy of file status of same name
   FileStat     d_LastFileStat; // Date/Time of last modify
         Point  d_ViewOrigin;
         Point  d_ViewCursor;
public:
   EdOpBoundary( PFBUF pFBuf );           // normal ctor
   EdOpBoundary( PFBUF pFBuf, int only ); // no-focus-info ctor
   ~EdOpBoundary() {}
   void VUndo() override {}
   void VRedo() override {}
   void VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const override;
   bool VIsBoundary() const override { return true; }
   bool VUpdtBoundary() override;
   bool VUpdtFromBoundary() override;
   };

bool EdOpBoundary::VUpdtBoundary() {
   d_fDirty       = d_pFBuf->IsDirty();
   d_LastFileStat = d_pFBuf->GetLastFileStat();
   d_ViewOrigin   = g_CurView()->Origin();
   d_ViewCursor   = g_CurView()->Cursor();
   return true;
   }

bool EdOpBoundary::VUpdtFromBoundary() {
   d_fDirty || (d_pFBuf->d_LastFileStat != d_LastFileStat) ? d_pFBuf->SetDirty() : d_pFBuf->UnDirty();
   PCV;
   if( pcv->FBuf() == d_pFBuf ) {
      pcv->ScrollOriginAndCursor( d_ViewOrigin, d_ViewCursor );
      DispNeedsRedrawStatLn();
      }
   DispNeedsRedrawAllLinesAllWindows();
   return true;
   }

STATIC_VAR PCCMD pNewCmd;

void FBUF::UndoInsertCmdAnnotation( PCCMD Cmd ) {
   pNewCmd = Cmd;
   }

EdOpBoundary::EdOpBoundary( PFBUF pFBuf )
   : EditRec     ( pFBuf )
   , d_pszCmdName( pNewCmd ? pNewCmd->Name() : "(none)" )
   {
   VUpdtBoundary();
   }

EdOpBoundary::EdOpBoundary( PFBUF pFBuf, int only ) // ONLY to be used when assigning EMPTY Undo list to a PFBUF
   : EditRec      ( pFBuf, only )
   , d_pszCmdName ( "---"       )
   , d_fDirty     ( pFBuf->IsDirty() )
   , d_ViewOrigin ( 0, 0        )
   , d_ViewCursor ( 0, 0        )
   {
   }

void EdOpBoundary::VShow( OutputWriter const &ow, PPChar ppBuf, size_t *pBufBytes, int fIsCur ) const {
   snprintf_full( ppBuf, pBufBytes, DBG_EDOP( "%03X " ) "%s {%s} [%cts=%08lX,%09" PR_FILESIZET " org=(%d,%d) cur=(%d,%d)" DBG_EDOP( " %s" ) "]"
#if USE_DBGF_EDOPS
      , d_SerNum
#endif
      , fIsCur ?  kszPCur : kszBound
      , d_pszCmdName
      , d_fDirty ? '*' : ' '
      , d_LastFileStat.d_ModifyTime   // Date/Time of last modify
      , d_LastFileStat.d_Filesize     // Date/Time of last modify
      , d_ViewOrigin.col
      , d_ViewOrigin.lin
      , d_ViewCursor.col
      , d_ViewCursor.lin
#if USE_DBGF_EDOPS
      , d_pFBuf->Name()
#endif
      );
   }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ShowEdits ("eds") show editops list of current buffer
//

void DumpEditOp( OutputWriter const &ow, PFBUF pmf, const EditRec *er ) {
   linebuf buf; auto pBuf( buf ); auto bufBytes( sizeof buf );
   er->VShow( ow, &pBuf, &bufBytes, er == pmf->CurrentEdit() );
   ow.VWriteLn( buf );
   }

void DumpEditOp( PFBUF ofile, PFBUF pmf, EditRec *er ) {
   DumpEditOp( FbufWriter( ofile ), pmf, er );
   }

void DBGEditOp( const EditRec *er ) {
   linebuf buf; auto pBuf( buf ); auto bufBytes( sizeof buf );
   DBGWriter ow;
   er->VShow( ow, &pBuf, &bufBytes, false );
   ow.VWriteLn( buf );
   }

int FBUF::GetUndoCounts( int *pBRTowardUndo, int *pARTowardUndo, int *pBRTowardRedo, int *pARTowardRedo ) const {
   *pBRTowardUndo = *pBRTowardRedo = 0; // BR = BoundRecs only
   *pARTowardUndo = *pARTowardRedo = 0; // AR = all records
   auto pEr( OldestEdit() );
   for( ; pEr && pEr != CurrentEdit() ; pEr=pEr->d_pRedo ) { pEr->VIsBoundary() ? ++(*pBRTowardUndo) : ++(*pARTowardUndo); }
   for( ; pEr                         ; pEr=pEr->d_pRedo ) { pEr->VIsBoundary() ? ++(*pBRTowardRedo) : ++(*pARTowardRedo); }
   return *pBRTowardUndo + *pBRTowardRedo;
   }

bool ARG::eds() { // nee ARG::showedits
   STATIC_VAR unsigned edBufNum;
   auto pFBuf( g_CurFBuf() );
   auto obuf( OpenFileNotDir_NoCreate( FmtStr<_MAX_PATH+20>( "<eds_%s_%X>", pFBuf->Name(), edBufNum++ ) ) );
   obuf->MakeEmpty();
   obuf->FmtLastLine( "UndoCount=%u\n(oldest)", pFBuf->UndoCount() );
   for( auto pEr(pFBuf->OldestEdit()) ; pEr ; pEr=pEr->d_pRedo ) {
      DumpEditOp( obuf, pFBuf, pEr );
      }
   obuf->PutLastLine( "(newest)" );
   obuf->ClearUndo();
   obuf->UnDirty();
   obuf->PutFocusOn();
   return true;
   }

// <041101> klg  for debug/test: 'rmeds'

#ifdef fn_rmeds

bool ARG::rmeds() {
   g_CurFBuf()->ClearUndo();
   return true;
   }

#endif

//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FBUF::RmvOneEdOp_fNextIsBoundary( bool fFromListHead ) {
   // DBG_EDOP( _ASSERTE( _CrtCheckMemory() ); )
   EditRec *pEr; EditRec *pNextEr;
   if( fFromListHead ) {
      pEr              = d_pNewestEdit;
      pNextEr          = pEr->d_pUndo;
      d_pNewestEdit    = pNextEr;
      pNextEr->d_pRedo = nullptr;
      }
   else {
      pEr              = d_pOldestEdit;
      pNextEr          = pEr->d_pRedo;
      d_pOldestEdit    = pNextEr;
      pNextEr->d_pUndo = nullptr;
      }
   DBG_EDOP( DBGEditOp( pEr ); )
   Delete0( pEr );
   // DBG_EDOP( _ASSERTE( _CrtCheckMemory() ); )
   return pNextEr->VIsBoundary();
   }

void FBUF::AddNewEditOpToListHead( EditRec *pEr ) {
   //
   // Since the to-be-added EditOp will be inserted at (and become the new)
   // d_pCurrentEdit, all previous UNDONE EditOps in the range
   // [d_pNewestEdit..d_pCurrentEdit) ARE DISCARDED here
   //
   // BUGBUG add logic to SAVE these records rather than
   // discard them.  Alternativly, add a UI dialog to confirm
   // the op?  Problem with the latter is that this is deep in
   // the bowels; UI interaction is "not advisable" (IOW
   // problematic) here.
   //
                                                DBG_EDOP( auto fShown = false; )
   while( d_pNewestEdit != d_pCurrentEdit ) {
                                                DBG_EDOP(
                                                   if( !fShown ) {
                                                      fShown = true;
                                                      DBG( "**************** BEGIN GarbageCollBeforeEdOpIns *********" );
                                                      }
                                                   )
      if( d_pNewestEdit->VIsBoundary() ) {
         --d_UndoCount;
         }
      RmvOneEdOp_fNextIsBoundary( true );
      }
                                                DBG_EDOP(
                                                   if( fShown ) {
                                                      DBG( "**************** END   GarbageCollBeforeEdOpIns *********" );
                                                      }
                                                   DBG( "**************** BEGIN EdOpIns *********" );
                                                   DBGEditOp( this );
                                                   DBG( "**************** END   EdOpIns *********" );
                                                   )
   d_pNewestEdit->d_pRedo = pEr;
   pEr->d_pRedo = nullptr;
   pEr->d_pUndo = d_pNewestEdit;
   d_pNewestEdit  = pEr;
   d_pCurrentEdit = pEr;
   }

void FBUF::UndoReplaceLineContent( LINE lineNum, stref newContent ) {
   0 && DBG( "%s+ L%d %s", __func__, lineNum, Name() );
   BadParamIf( , IsNoEdit() );
   if( d_pNewestEdit == nullptr ) {
      InitUndoInfo();
      }
   const auto pLineInfo( d_paLineInfo + lineNum );
   if( d_pNewestEdit->VIsAltContentOfLine( lineNum ) ) {
      // multiple sequential edits on the same line are effectively coalesced into an EditRec containing the oldest pLineData
      pLineInfo->FreeContent( *this );  // free (and therefore eternally forget) intermediate-edit *pLineInfo (line content)
      }
   else {
      // this is the first sequential edit on this line: MOVE current *pLineInfo
      // (line content) in undo system:
      //
      new EdOpAltLineContent( this, lineNum, pLineInfo );
      }
   pLineInfo->PutContent( newContent );
   Set_yChangedMin( lineNum );
   0 && DBG( "%s- L%d %s", __func__, lineNum, Name() );
   }

void FBUF::UndoInsertLineRangeHole( LINE firstLine, int lineCount ) {
   if( IsNoEdit() ) { return; }
   new EdOpLineRangeInsert( this, firstLine, lineCount );
   }

void FBUF::UndoSaveLineRange( LINE firstLine, LINE lastLine ) {
   if( IsNoEdit() ) { return; }
   new EdOpLineRangeDelete( this, firstLine, lastLine );
   }

void FBUF::PutUndoBoundary() {
   if( IsNoEdit() ) { return; }
   if( d_pCurrentEdit->VUpdtBoundary() ) {
      0 && DBG( "PUB: updt" );
      }
   else {
      0 && DBG( "PUB: new" );
      // add a new BoundaryRec based on current state
      new EdOpBoundary( this );
      ++d_UndoCount;
      while( d_UndoCount > g_iMaxUndo ) {
         if( RmvOneEdOp_fNextIsBoundary( false ) ) {
            --d_UndoCount;
            }
         }
      }
   }

void FBUF::DiscardUndoInfo() {
   while( d_pOldestEdit ) {
      EditRec *er( d_pOldestEdit );
      d_pOldestEdit = d_pOldestEdit->d_pRedo;
      Delete0( er );
      }
   d_UndoCount    = 0;
   d_pCurrentEdit = nullptr;
   d_pNewestEdit  = nullptr;
   d_pOldestEdit  = nullptr;
   }

void FBUF::InitUndoInfo() {
   new EdOpBoundary( this, 1 );  // plug a boundary record into EditList as a placeholder
   }

void FBUF::ClearUndo() {
   DiscardUndoInfo();
   InitUndoInfo();
   }

bool FBUF::EditOpUndo() {
   d_pCurrentEdit->VUndo(); // polymorphic
   d_pCurrentEdit = d_pCurrentEdit->d_pUndo;
   return !d_pCurrentEdit->VIsBoundary();
   }

bool FBUF::EditOpRedo() {
   d_pCurrentEdit->VRedo(); // polymorphic
   d_pCurrentEdit = d_pCurrentEdit->d_pRedo;
   return !d_pCurrentEdit->VIsBoundary();
   }

bool FBUF::AnythingToUndoRedo( bool fChkRedo ) const {
   if( d_pCurrentEdit == nullptr ) { return false; }
   return ToBOOL(fChkRedo ? d_pCurrentEdit->d_pRedo : d_pCurrentEdit->d_pUndo);
   }

void FBUF::SetUndoStateToBoundrec() {
   d_pCurrentEdit->VUpdtFromBoundary();
   }

bool FBUF::DoUserUndoOrRedo( bool fRedo ) {
   // Undo: Reverses the last editing change.  The maximum number of times this
   //       can be performed is set by the Undocount switch, which limits the
   //       number of EditRec records which can be associated with an FBUF.
   //
   // Redo: Re-applies an editing change previously cancelled with Undo.
   //
   // Returns:  True:  An operation was undone or redone
   //           False: There was nothing to undo or redo
   //
   if( !AnythingToUndoRedo( fRedo ) ) {
      if( !Interpreter::Interpreting() ) {
         Msg( fRedo ? "Nothing to ReDo" : "Nothing to UnDo" );
         }
      return false;
      }
   PutUndoBoundary();
   while( (fRedo ? EditOpRedo() : EditOpUndo()) ) {
      continue;
      }
   SetUndoStateToBoundrec();
   return true;
   }

bool ARG::undo() { return g_CurFBuf()->DoUserUndoOrRedo( false ); }
bool ARG::redo() { return g_CurFBuf()->DoUserUndoOrRedo( true  ); }

//
// Copyright 1991 - 2012 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

struct NamedPoint {
   DLinkEntry<NamedPoint> dlink;
   Point                  d_pt;
   PChar                  d_pszName;

   NamedPoint( const Point& pt, PCChar name ) : d_pt(pt), d_pszName( Strdup(name) ) {}
   ~NamedPoint() { Free0( d_pszName ); }

   bool         NameMatch( PCChar pszName ) const { return strcmp( d_pszName, pszName ) == 0; }
   PCChar       Name()                      const { return d_pszName; }
   const Point &Pt()                        const { return d_pt; }
   void         AddLineDelta( int delta )         {        d_pt.lin += delta; }
   }; STD_TYPEDEFS( NamedPoint )


STATIC_FXN void NamedPointListDestroy( NamedPointHead &hd ) {
   while( auto pEl=hd.First() ) {
      DLINK_REMOVE_FIRST( hd, pEl, dlink );
      delete pEl;
      }
   Assert( hd.empty() );
   }

void FBUF::DestroyMarks() { NamedPointListDestroy( d_MarkHead ); }

STATIC_FXN void NamedPointListInsert( NamedPointHead &hd, PNamedPoint pNew ) {
   DLINKC_FIRST_TO_LASTA( hd, dlink, pNP ) {
      if( pNew->Pt() < pNP->Pt() ) {
         DLINK_INSERT_BEFORE( hd, pNP, pNew, dlink );
         return;
         }
      }
   DLINK_INSERT_LAST( hd, pNew, dlink );
   }


STATIC_FXN PCNamedPoint NamedPointFind( const NamedPointHead &hd, PCChar pszName ) {
   DLINKC_FIRST_TO_LASTA( hd, dlink, pNP ) {
      if( pNP->NameMatch( pszName ) )
         return pNP;
      }
   return nullptr;
   }

struct MarkRmvr {
   const PCChar d_markname;
   const Point *d_pPt;
   int          matchCount;
   MarkRmvr( PCChar markname, Point *pPt ) : d_markname(markname), d_pPt(pPt) {}
   void operator()( NamedPoint *pThis ) {
      if( pThis->NameMatch( d_markname ) && (!d_pPt || *d_pPt != pThis->Pt()) ) {
         ++matchCount;
         delete pThis;
         // DLINK_REMOVE( hd, pThis, dlink );
         }
      }
   };


STATIC_FXN int NamedPointListDeleteAllMatches( NamedPointHead &hd, PCChar pszName, const Point *pPt=nullptr ) {
   auto matchCount(0);
   PNamedPoint pThis, pNext;
   DLINK_FIRST_TO_LAST( hd, dlink, pThis, pNext ) {
      if( pThis->NameMatch( pszName ) && (!pPt || *pPt != pThis->Pt()) ) {
         ++matchCount;
         DLINK_REMOVE( hd, pThis, dlink );
         Delete0( pThis );
         }
      }
   return matchCount;
   }


STATIC_FXN void NamedPointListAddLineOfsPast( NamedPointHead &hd, const LINE yThld, const LINE yDelta ) {
   auto fPastThld(false);
   DLINKC_FIRST_TO_LASTA( hd, dlink, pNP ) {
      if( !fPastThld && !(fPastThld = (pNP->Pt().lin > yThld)) ) {
         pNP->AddLineDelta( yDelta );
         }
      }
   }


/////////////////////////////////////////////////////////////////////////////////////////
//
// ARG::showmarks ("mks"): show all marks defined, per file
//

STATIC_FXN int NamedPointListDump( const NamedPointHead &hd, PFBUF ofile, PCChar pszFilename ) {
   auto markCount(0);
   DLINKC_FIRST_TO_LASTA( hd, dlink, pNP ) {
      ++markCount;
      ofile->FmtLastLine( "%s %u %uL1: %s"
         , pszFilename
         , pNP->Pt().lin + 1
         , pNP->Pt().col + 1
         , pNP->Name()
         );
      }
   return markCount;
   }


bool ARG::showmarks() {
   STATIC_VAR int markdump_ctr;
   auto ofile( OpenFileNotDir_NoCreate( FmtStr<25>( "<marks_%u>", markdump_ctr++ ) ) );
   ofile->MakeEmpty();

   auto totalMarks(0);
#if FBUF_TREE
   PRbNode pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      totalMarks += NamedPointListDump( pFBuf->d_MarkHead, ofile, pFBuf->Name() );
      }
   ofile->ClearUndo();
   ofile->UnDirty();
   ofile->SetNoEdit();
   ofile->PutFocusOn();

   return true;
   }
//
/////////////////////////////////////////////////////////////////////////////////////////

bool FBUF::FindMark( PCChar pszMarkname, FBufLocn *pFL ) {
   auto pMatchMk = NamedPointFind( d_MarkHead, pszMarkname );
   if( !pMatchMk )
      return false;

   pFL->Set( this, pMatchMk->Pt() );
   return true;
   }

struct FileHasMark {
   PCChar    d_pszMarkname;
   FBufLocn *d_pFL;
   FileHasMark( PCChar pszMarkname, FBufLocn *pFL ) : d_pszMarkname(pszMarkname), d_pFL (pFL ) {}
   bool operator()( PFBUF pFBuf ) { return pFBuf->FindMark( d_pszMarkname, d_pFL ); }
   };

STATIC_FXN bool FindMarkAllFiles( PCChar pszMarkname, FBufLocn *pFL ) {
#if FBUF_TREE
   PRbNode pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      if( pFBuf->FindMark( pszMarkname, pFL ) )
         return true;
      }
   return false;
   }

/////////////////////////////////////////////////////////////////////////////////////////

bool MarkGoto( PCChar pszMarkname ) {
   0 && DBG( "MK: %s+ '%s'", __func__, pszMarkname );
   FBufLocn markLocn;
   if( !FindMarkAllFiles( pszMarkname, &markLocn ) )
      return ErrorDialogBeepf( "Mark '%s' not found", pszMarkname );

   markLocn.ScrollToOk();
   // 0 && DBG( "MK: %s- (%d,%d) in '%s'", __func__, markLocn.d_pt.col, markLocn.d_pt.lin, pFBuf->Name() );
   return true;
   }


STATIC_FXN int MarkDelete( PCChar pszMarkname, bool fVerbose=true ) {
   int totalMarks = 0;
#if FBUF_TREE
   PRbNode pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      auto pFBuf( IdxNodeToFBUF( pNd ) );
#endif
      totalMarks += NamedPointListDeleteAllMatches( pFBuf->d_MarkHead, pszMarkname );
      }

   if( fVerbose ) {
      if( totalMarks )
         Msg( "%s: %d mark%s deleted", pszMarkname, totalMarks, Add_s(totalMarks) );
      else
         ErrorDialogBeepf( "%s: Mark not found", pszMarkname );
      }

   return totalMarks;
   }


void MarkDefineAtCurPos( PCChar pszNewMarkName ) {
   const int totalMarks( MarkDelete( pszNewMarkName, false ) );
   0 && DBG( "%s: zapped %d prev defines of '%s'", __func__, totalMarks, pszNewMarkName );
   NamedPointListInsert( g_CurFBuf()->d_MarkHead, new NamedPoint( g_Cursor(), pszNewMarkName ) );
   }


bool ARG::mark() {
   DBG("MK: fn_mark");

   switch( d_argType ) {
    default:      return BadArg();
    case NOARG:   g_CurView()->MoveCursor( 0, 0 );  return true;
    case NULLARG: g_CurView()->ScrollToPrev();      return true;
    case TEXTARG: if( StrSpnSignedInt( d_textarg.pText ) ) {
                     g_CurView()->MoveCursor( atoi( d_textarg.pText ) - 1, 0 );
                     return true;
                     }

                  if( d_cArg == 2 ) {
                     if( d_fMeta )
                        MarkDelete( d_textarg.pText, true );
                     else
                        MarkDefineAtCurPos( d_textarg.pText );
                     return true;
                     }

                  return MarkGoto( d_textarg.pText );
    }
   }

void AdjMarksForBoxDeletion( PFBUF pFBuf, COL xLeft, LINE yTop, COL xRight, LINE yBottom ) {
   0 && DBG( "MK: %s '%s'", __func__, pFBuf->Name() );
   }

void AdjustMarksForLineDeletion( PFBUF pFBuf,int xLeft,int yTop,int xRight,int yBottom ) {
   0 && DBG( "MK: %s '%s'", __func__, pFBuf->Name() );
   }

void AdjustMarksForLineInsertion( LINE Line, int LineDelta, PFBUF pFBuf ) {
   0 && DBG( "MK: %s '%s'", __func__, pFBuf->Name() );
   NamedPointListAddLineOfsPast( pFBuf->d_MarkHead, Line, LineDelta );
   }

#if 0

void UpdateMarksForBoxDelete( PFBUF pFBuf, COL xLeft, LINE yTop, COL xRight, LINE yBottom ); {
   if( pFBuf->d_MarkHead ) {
      const Point upperLeftCorner ( yTop   , xLeft  );
      const Point lowerRightCorner( yBottom, xRight );
      auto fAFlag(false);
      for( auto pMark( &pFBuf->d_MarkHead->mark ); pMark->Valid(); pMark=pMark->Next() ) {
         if( !fAFlag && upperLeftCorner >= pMark->pt ) pastTopLeftAlready_QQQ
            break;

         if( pMark->pt >= lowerRightCorner )
            break;

         fAFlag = true;

         if( pMark->pt.col >= xLeft && xRight >= pMark->pt.col ) { // nextMarkInBuf
            pMark->pt.col = pMark->pt.col - xLeft + 1;
            pMark->pt.lin = pMark->pt.lin - yTop  + 1;
            }
         }
      }
   }

#endif

void AdjMarksForInsertion( PCFBUF pFBufSrc, PFBUF pFBufDest, int xLeft, int yTop, int xRight, int yBottom, int xDest, int yDest )
   {
#if 0
   auto pFBuf(pFBufSrc);
   if( pFBufSrc ) {
      if(   pFBufSrc  != pFBufDest         // file -> file copy (not simple insertion of blank lines)?
         && pFBufSrc  != g_pFbufClipboard  // neither of the two PFBUFs is <clipboard>
         && pFBufDest != g_pFbufClipboard
        )
         return;                           // then there is nothing to do!
      }
   else {
      pFBuf = pFBufDest;
      const COL xtmp( xRight + 1 );
      xRight = BUFBYTES;
      xDest  = xtmp;
      }

   UpdateMarksForBoxDelete( pFBuf, xLeft, yTop, xRight, yBottom );
#endif
   }

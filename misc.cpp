//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

char Xbuf::ds_empty = 0;

// takes counted-string param so *pStart doesn't have to be forcibly
// NUL-terminated (it may be a const string)
//
PChar Getenv( PCChar pStart, int len ) {
   ALLOCA_STRDUP( buf, slen, pStart, len )    0 && DBG("Getenv '%s'", buf );
   return getenv( buf );
   }

PChar GetenvStrdup( PCChar pStart, size_t len ) {
   ALLOCA_STRDUP( buf, slen, pStart, len )    0 && DBG("GetenvStrdup '%s'", buf );
   return GetenvStrdup( buf );
   }

PChar GetenvStrdup( PCChar pszEnvName ) {
   auto penv( getenv( pszEnvName ) );
   if( penv )
       penv = Strdup( penv );

   return penv;
   }

STATIC_FXN void CreateGrepBufFromLinerange( PFBUF fromfile, PFBUF outfile, LINE yTop, LINE yBottom ) {
   auto imgBufBytes(0);
   const auto copiedLines(yBottom-yTop+1);
   const auto lwidth( uint_log_10( fromfile->LineCount() ) );

   SprintfBuf Line1( "*GREP* %s", fromfile->Name() );
   const auto L1Len( Strlen( Line1 ) );
   imgBufBytes += L1Len;

   SprintfBuf Line2( "%s lines"     , outfile->Name()    );
   const auto L2Len( Strlen( Line2 ) );
   imgBufBytes += L2Len;

   for( auto ix(yTop); ix <= yBottom; ++ix )
      imgBufBytes += fromfile->LineLength( ix );

   outfile->MakeEmpty();
   outfile->ImgBufAlloc( imgBufBytes + (lwidth+2) * copiedLines, copiedLines + 2 );
   outfile->ImgBufAppendLine( Line1, L1Len );
   outfile->ImgBufAppendLine( Line2, L2Len );

   for( auto ix(yTop); ix <= yBottom; ++ix )
      outfile->ImgBufAppendLine( fromfile, ix, FmtStr<20>( "%*d  ", lwidth, ix+1 ) );
   }


STATIC_FXN void AppendLinerangeToGrepBuf( PFBUF fromfile, PFBUF grepBuf, LINE yTop, LINE yBottom ) {
   const auto lwidth( uint_log_10( fromfile->LineCount() ) );
   for( auto ix(yTop); ix <= yBottom; ++ix ) {
      PCChar ptr;  size_t chars;  // ptr[chars] is NOT valid: ptr[chars] DOES NOT contain '\0'!  The trick "%.*s" is used!!!
      if( fromfile->PeekRawLineExists( ix, &ptr, &chars ) )
          grepBuf->FmtLastLine( "%*d  %.*s", lwidth, ix+1, pd2Int(chars), ptr );
      else
          grepBuf->FmtLastLine( "%*d", lwidth, ix+1 );
      }
   FBOP::LuaSortLineRange( grepBuf, 2, grepBuf->LastLine(), 0, lwidth );
   }


//------------------------------------------------------------------------------
//
// ChopStringFieldsOnDelim - split a string on delim(s)
//
// line        string to split (not altered)
// fields      array of pointers to linebuf's
// fieldCount  number of pointers to linebuf's in fields
//
STATIC_FXN int ChopStringFieldsOnDelim( PChar line, PChar fields[], const int fieldCount, PCChar pszDelim ) {
   auto segStart( line );
   auto fn(0);
   for( ; line && fn < fieldCount; ++fn ) {
      segStart = StrPastAny( segStart, pszDelim );
      if( !*segStart )
         return fn;

      if( fields ) fields[fn] = segStart;

      segStart = StrToNextOrEos( segStart, pszDelim );
      if( *segStart == 0 )
         return fn+1;

      if( fields ) *segStart++ = '\0';
      }

   return -fn;
   }

STATIC_FXN bool CopyNumberedLinesToNewFile( PFBUF srcfile, PFBUF destfile, ARG *pArg ) {
   LINE yTop, yBottom;
   if( pArg->GetLineRange( &yTop, &yBottom ) )
       return false;

   auto fUseMFGrepFmt( false );
   {
   linebuf    destGrepFBufname;
   int        destGrepHdrLines;
   const auto destIsGrep( ToBOOL( FBOP::IsGrepBuf( destfile, BSOB(destGrepFBufname), &destGrepHdrLines ) ) );

   if( destIsGrep ) {
      linebuf   srcGrepFBufname;
      int       srcGrepHdrLines;
      const auto srcIsGrep( ToBOOL( FBOP::IsGrepBuf( srcfile, BSOB(srcGrepFBufname), &srcGrepHdrLines ) ) );

      if( srcIsGrep ) {
         if( 0 != strcmp( srcGrepFBufname, destGrepFBufname ) ) {
            return Msg( "src & dest are both grep bufs && they have different source files!" );
            }
         else {
            // src & dest are both grep bufs && they have same source files!
            // optimization opportunity: block-copy src lines to dest and sort
            merge_grep_buf( destfile, srcfile );

            destfile->ClearUndo();
            destfile->UnDirty();
            destfile->PutFocusOn();
            return true;
            }
         }
      else if( 0 != strcmp( destGrepFBufname, srcfile->Name() ) ) {
         fUseMFGrepFmt = true;

         // CONVERT EXISTING DESTFILE CONTENT TO fUseMFGrepFmt FORMAT
         //
         const auto lwidth( uint_log_10( srcfile->LineCount() ) );
         destfile->DelLines( 0, 1 );
         auto srcfileLine(1);
         auto npos(1);
         Xbuf xb;
         for( auto ix(0); ix < destfile->LineCount(); ++ix ) {
            auto lineChars( destfile->getLineTabx( &xb, ix ) );
            auto lbuf( xb.wbuf() );
            if( 2 == sscanf( lbuf, "%d%n", &srcfileLine, &npos ) ) {
               auto pTail( StrPastAnyWhitespace( lbuf + npos ) );
               if( *pTail ) { DBG( "tail=%s", pTail ); }

               PChar bp[2];
               const auto fields( ChopStringFieldsOnDelim( lbuf, bp, ELEMENTS(bp), szSpcTab ) );
               if( fields == 2 ) {
               // s_pFbufLog->FmtLastLine( " !! '%s'", bp[1] );
                  destfile->FmtLine( ix, "%s %*d: %s", destGrepFBufname, lwidth, srcfileLine, bp[1] );
                  }
               else
                  destfile->FmtLine( ix, "%s %*d:"   , destGrepFBufname, lwidth, srcfileLine       );
               }
            else {
               if( lineChars )
                  destfile->FmtLine( ix, "%s %*s: %s", destGrepFBufname, lwidth, "?", lbuf  );
               else
                  destfile->FmtLine( ix, "%s %*s:"   , destGrepFBufname, lwidth, "?"        );
               }
            }
         }
      }
   }

   if( fUseMFGrepFmt ) {
      const auto leadWhite( FBOP::MaxCommonLeadingWhitespaceInLinerange( srcfile, yTop, yBottom ) );
      const auto lwidth( uint_log_10( srcfile->LineCount() ) );
      for( auto ix(yTop); ix <= yBottom; ++ix ) {
         PCChar ptr; size_t chars;
         // ptr[chars] is NOT valid: ptr[chars] DOES NOT contain '\0'!  The trick "%.*s" is used!!!
         if( !srcfile->PeekRawLineExists( ix, &ptr, &chars ) )
             destfile->FmtLastLine( "%s %*d:", srcfile->Name(), lwidth, ix+1 );
         else
             destfile->FmtLastLine( "%s %*d: %.*s", srcfile->Name(), lwidth, ix+1, pd2Int(chars-leadWhite), ptr+leadWhite );
         }
      }
   else {
      if( destfile->LineCount() == 0 )
         CreateGrepBufFromLinerange( srcfile, destfile, yTop, yBottom );
      else
         AppendLinerangeToGrepBuf( srcfile, destfile, yTop, yBottom );
      }
   return true;
   }


// ****************************************************************************
//
// Copy hook: two args cause copying to new scratch pseudofile
//
// arg arg boxarg|linearg copy   create new _pseudofile_ "<sel%u>" and paste selection into it
//

//
// BUGBUG  finish this: need to specify a common dir
// BUGBUG  finish this: need a GREP ALL NOTES command (macro)
//
// arg note            create new scratch disk file in switch 'notedir' "YYYYMMDD_HHMMSS.note"
// arg "textarg" note  create new scratch disk file in switch 'notedir' "YYYYMMDD_HHMMSS.textarg"

   // int fWrNoteDiskFile = d_fMeta;
   // if( fWrNoteDiskFile )
   //    {
   //    pathbuf buf;
   //    SYSTEMTIME sysTm;
   //    GetLocalTime( &sysTm );
   //    safeSprintf( buf, "%04u%02u%02u_%02u%02u%02u.note"
   //       , sysTm.wYear
   //       , sysTm.wMonth
   //       , sysTm.wDayOfWeek
   //       , sysTm.wDay
   //       , sysTm.wHour
   //       , sysTm.wMinute
   //       , sysTm.wSecond
   //       );
   //    tofile = NewOrExisting (buf);
   //    }
   // else

//

STATIC_VAR struct {
   PCChar name;
   int    counter;
   } pseudoBufInfo[] = {
     {"grep"},
     {"sel"},
   };

PFBUF PseudoBuf( ePseudoBufType pseudoBufType, int fNew ) {
   auto &selNum( pseudoBufInfo[ pseudoBufType ].counter );
   if( fNew )
      ++selNum;

   return OpenFileNotDir_NoCreate( FmtStr<20>( "<%s.%u>", pseudoBufInfo[ pseudoBufType ].name, selNum ) );
   }

bool ARG::nextselbuf() {
   ++pseudoBufInfo[ SEL_BUF ].counter;
   return true;
   }

//
// Goals:
//
// 1) allow easy integration of multiple grep outputs (same file searched)
// 2) allow manual addition of lines into grep buffers OR
// 3)       manual addition of lines from different files into pseudofile, with mfgrep headers
// 4) convert a grep buffer into the equivalent mfgrep-headers-per-line file
//
//
#if 0 // BUGBUG have these features been moved into ARG::copy?

STATIC_FXN void CopyBoxOrLineargToTopOfFile( PFBUF fromfile, PFBUF tofile, ARG *pArg ) {
   switch( pArg->d_argType ) {
      case BOXARG: FBOP::CopyBox( tofile, 0, 0, fromfile
                      , pArg->d_boxarg.flMin.col, pArg->d_boxarg.flMin.lin
                      , pArg->d_boxarg.flMax.col, pArg->d_boxarg.flMax.lin
                      );
                   break;

      default:     { // overload everything else (NO, NULL, STREAM, LINE) into a LINEARG
                   LINE yTop, yBottom;
                   if( pArg->GetLineRange( &yTop, &yBottom ) )
                       return;

                   FBOP::CopyLines( tofile, 0, fromfile, yTop, yBottom );
                   }
                   break;
      }
   }

bool ARG::copyNew() {
   if( d_cArg >= 2 ) {
      PFBUF tofile, fromfile( g_CurFBuf() );
      if( d_cArg == 2 ) { // dest file is EXISTING <sel_n>
         tofile = PseudoBuf( SEL_BUF, 0 );
         if( fromfile == tofile )
            return false;
         CopyBoxOrLineargToTopOfFile( fromfile, tofile, pArg );
         }
      else { // >= 3: dest file is <grep.N>, lines are numbered
         tofile = PseudoBuf( GREP_BUF, d_fMeta );
         if( fromfile == tofile )
            return false;
         CopyNumberedLinesToNewFile( fromfile, tofile, pArg );
         }

      tofile.UnDirty();
      tofile.PutFocusOn();

      return true;
      }
   else { // dest file is <clipboard>
      return (*copy)( argData, pArg );
      }
   }

#endif

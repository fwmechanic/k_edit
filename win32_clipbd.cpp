//
// Copyright 1991 - 2014 by Kevin L. Goodwin; All rights reserved
//

#include "ed_main.h"

#include "win32_pvt.h"

//
//#############################################################################################################################
//##########################################  Win32 CLIPBOARD INTERFACE  ######################################################
//#############################################################################################################################
//

STATIC_CONST char ClipUnavail[] = "Windows Clipboard Unavailable";

STATIC_FXN bool PrepClip( long size, Win32::HGLOBAL *hglbCopy, PInt sizeofHglbCopy, PPChar bufptr ) {
   if( !Win32::OpenClipboard( Win32::GetActiveWindow() ) ) {
      *bufptr = nullptr;         // 2 stmts so -O3 optimizer can see the (constant) retval in this path
      return Msg( ClipUnavail ); // error: can't do the op
      }

   Win32::EmptyClipboard();

   // Allocate a global memory object for the text.

   ++size; // for EoL ('\0')
   DBG( "WinClip new [%lu]", size );
   *sizeofHglbCopy = size;
   *hglbCopy = Win32::GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size * sizeof(Win32::TCHAR) );
   if( *hglbCopy == nullptr ) {
      Win32::CloseClipboard();
      return Msg( "Win32::GlobalAlloc could not allocate %lu bytes!", size );
      }

   // Lock the handle so we can copy text into this buffer
   *bufptr = PChar(Win32::GlobalLock(*hglbCopy));
   return true;
   }

//
// "CF_OEMTEXT - Text format containing characters in the OEM character set.
// Each line ends with a carriage return/linefeed (CR-LF) combination.  A null
// character signals the end of the data."
//

bool ARG::towinclip() {
   Win32::HGLOBAL  hglbCopy;
   int             hglbBytes;
   char           *bufptr; // this WILL TRAVERSE a buffer
   Xbuf            lbuf;

   if(  d_argType == NOARG
     || d_argType == LINEARG
     || d_argType == BOXARG
     ) {
      LINE  yMin, yMax;
      COL   xLeft(0), xRight(COL_MAX); // assume whole line
      PFBUF pFBuf(g_CurFBuf());
      PCChar srcNm(nullptr);
      if( d_argType == NOARG ) {
         DBG( "NOARG->WinClip" );
         if( g_pFbufClipboard->LineCount() == 0 )
            return ErrPause( "<clipboard> is empty!" );
         pFBuf = g_pFbufClipboard;
         yMin  = 0;
         yMax  = pFBuf->LastLine();
         srcNm = "<clipboard>";
         }
      else if( d_argType == LINEARG ) {
         yMin  = d_linearg.yMin;
         yMax  = d_linearg.yMax;
         }
      else { // if( d_argType == BOXARG ) these are equiv, and this way silences some warnings
         xLeft  = d_boxarg.flMin.col;
         xRight = d_boxarg.flMax.col;
         yMin   = d_boxarg.flMin.lin;
         yMax   = d_boxarg.flMax.lin;
         }

      Msg( "%s->WinClip %d lines", srcNm ? srcNm : ArgTypeName(), (yMax - yMin)+1 );

      if( yMax == yMin ) {
         pFBuf->GetLineSeg( lbuf, yMin, xLeft, xRight ); // read line into our buffer
         goto SINGLE_LINE; // HACK O'RAMA!
         }

      // determine # of chars on each line
      long size(0);
      for( auto lineNum(yMin); lineNum <= yMax; ++lineNum ) {
         size += pFBuf->GetLineSeg( lbuf, lineNum, xLeft, xRight ) + 2; // + 2 for '\r\n'
         }

      if( !PrepClip( size, &hglbCopy, &hglbBytes, &bufptr ) ) // +1 for trailing '\0'
         return false;

      // copy source data into into *bufptr
      for( auto lineNum(yMin); lineNum <= yMax; ++lineNum ) {
         const PCChar bs( bufptr );
         const auto chars( pFBuf->GetLineSeg( lbuf, lineNum, xLeft, xRight ) );
         memcpy( bufptr, lbuf.c_str(), chars );
         bufptr += chars;
         *bufptr++ = 0x0D; // line terminator '\r\n'
         *bufptr++ = 0x0A;
         hglbBytes -= bufptr - bs;
         }
      *bufptr++ = '\0'; // buffer terminator
      }
   else if( d_argType == TEXTARG ) {
      lbuf.assign( d_textarg.pText );

SINGLE_LINE: // HACK O'RAMA!

      const auto blen( lbuf.length() );
      Msg( "%s->WinClip %Iu|%s|", ArgTypeName(), blen, lbuf.c_str() );

      if( !PrepClip( blen, &hglbCopy, &hglbBytes, &bufptr ) )
         return false;

      memcpy( bufptr, lbuf.c_str(), blen+1 );
      }
   else
      return BadArg();

   // we're done copying text into this buffer, so unlock it before...
   Win32::GlobalUnlock( hglbCopy );

   // ...giving the buffer to the clipboard
   if( !Win32::SetClipboardData( CF_OEMTEXT, hglbCopy ) ) {
      Win32::CloseClipboard();
      return ErrPause( "Win32::SetClipboardData write failed" ); // error: can't do the op
      }

   Win32::CloseClipboard();
   return true;
   }


// CF_TEXT format seems to be more prevalently generated than CF_OEMTEXT by our
// beloved GUI apps, so it's what we ask for...
//
bool ARG::fromwinclip() {
   auto retVal(false);

   if( !Win32::OpenClipboard( Win32::GetActiveWindow() ) )
      ErrPause( ClipUnavail ); // error: can't do the op
   else {
      if( !Win32::IsClipboardFormatAvailable(CF_TEXT) )
         ErrPause( "CF_TEXT format data not available" );
      else {
         auto hglb( Win32::GetClipboardData( CF_TEXT ) );
         if( !hglb )
            ErrPause( "GetClipboardData failed" );
         else {
            const PCChar pClip( PChar(Win32::GlobalLock( hglb )) );
            if( !pClip )
               ErrPause( "GlobalLock on ClipboardData failed" );
            else {
               //################################################################
               g_pFbufClipboard->MakeEmpty();
               const int lineCount( g_pFbufClipboard->PutLastMultiline( pClip ) );
               g_ClipboardType = (lineCount == 1) ? BOXARG : LINEARG;
               Msg( "WinClip -> <clipboard> ok" );
               //################################################################
               Win32::GlobalUnlock( hglb );
               retVal = true;
               }
            }
         }
      Win32::CloseClipboard();
      }
   return retVal;
   }


void WinClipGetFirstLine( std::string &xb ) {
   if( !Win32::OpenClipboard( Win32::GetActiveWindow() ) )
      xb = ClipUnavail;
   else {
      if( !Win32::IsClipboardFormatAvailable(CF_TEXT) )
         xb = "CF_TEXT format data not available";
      else {
         auto hglb( Win32::GetClipboardData( CF_TEXT ) );
         if( !hglb )
            xb = "GetClipboardData failed";
         else {
            const auto pClip( PCChar(Win32::GlobalLock( hglb )) );
            if( !pClip )
               xb = "GlobalLock on ClipboardData failed";
            else {
               xb.assign( pClip, StrToNextOrEos( pClip, "\x0D\x0A" ) - pClip );
               Win32::GlobalUnlock( hglb );
               }
            }
         }
      Win32::CloseClipboard();
      }
   }

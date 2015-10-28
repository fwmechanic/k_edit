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

// Win32 Clipboard interface

#include "ed_main.h"
#include "win32_pvt.h"

STATIC_CONST char ClipUnavail[] = "Windows Clipboard Unavailable";

typedef  Win32::HGLOBAL  hglbCopy_t;

STATIC_FXN PChar PrepClip( long size, hglbCopy_t &hglbCopy ) {
   if( !Win32::OpenClipboard( Win32::GetActiveWindow() ) ) {
      Msg( ClipUnavail );
      return nullptr;
      }

   Win32::EmptyClipboard();

   // Allocate a global memory object for the text.
   0 && DBG( "WinClip new [%lu]", size );
   hglbCopy = Win32::GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size * sizeof(Win32::TCHAR) );
   if( hglbCopy == nullptr ) {
      Win32::CloseClipboard();
      Msg( "Win32::GlobalAlloc could not allocate %lu bytes!", size );
      return nullptr;
      }

   // Lock the handle so we can copy text into this buffer
   return static_cast<PChar>( Win32::GlobalLock( hglbCopy ) );
   }

STATIC_FXN PCChar WrToClipEMsg( hglbCopy_t hglbCopy ) {
   // we're done copying text into this buffer, so unlock it before...
   Win32::GlobalUnlock( hglbCopy );

   // ...giving the buffer to the clipboard
   //
   // "CF_OEMTEXT - Text format containing characters in the OEM character set.
   // Each line ends with a carriage return/linefeed (CR-LF) combination.  A null
   // character signals the end of the data."
   //
   const auto wrOk( Win32::SetClipboardData( CF_OEMTEXT, hglbCopy ) );
   Win32::CloseClipboard();
   return wrOk ? nullptr : "Win32::SetClipboardData write failed" ;
   }

STATIC_FXN size_t sizeof_eol() { return 2; }
STATIC_FXN void cat_eol( PChar &bufptr ) {
   *bufptr++ = 0x0D; // line terminator '\r\n'
   *bufptr++ = 0x0A;
   }

STATIC_FXN PCChar DestNm() { return "Win"; }

bool ARG::towinclip() {
   hglbCopy_t      hglbCopy;
   PChar           bufptr; // this WILL TRAVERSE a buffer
   std::string     stbuf;

   if(  d_argType == NOARG
     || d_argType == LINEARG
     || d_argType == BOXARG
     ) {
      LINE  yMin, yMax;
      COL   xLeft(0), xRight(COL_MAX); // assume whole line
      PFBUF pFBuf(g_CurFBuf());
      PCChar srcNm(nullptr);
      if( d_argType == NOARG ) {
         0 && DBG( "NOARG->%sClip", DestNm() );
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
      Msg( "%s->%sClip %d lines", srcNm ? srcNm : ArgTypeName(), DestNm(), (yMax - yMin)+1 );

      if( yMax == yMin ) {
         pFBuf->DupLineSeg( stbuf, yMin, xLeft, xRight ); // read line into our buffer
         goto SINGLE_LINE; // HACK O'RAMA!
         }

      { // determine # of chars on each line
      long size(0);
      for( auto lineNum(yMin); lineNum <= yMax; ++lineNum ) {
         pFBuf->DupLineSeg( stbuf, lineNum, xLeft, xRight );
         size += stbuf.length() + sizeof_eol();
         }

      if( nullptr == (bufptr=PrepClip( 1+size, hglbCopy )) ) // ++ for trailing '\0'
         return false;
      }
      // copy source data into into *bufptr
      for( auto lineNum(yMin); lineNum <= yMax; ++lineNum ) {
         const PCChar bs( bufptr );
         pFBuf->DupLineSeg( stbuf, lineNum, xLeft, xRight );
         const auto chars( stbuf.length() );
         memcpy( bufptr, stbuf.c_str(), chars );
         bufptr += chars;
         cat_eol( bufptr );
         }
      *bufptr++ = '\0'; // buffer terminator
      }
   else if( d_argType == STREAMARG ) {
      stbuf = StreamArgToString( g_CurFBuf(), d_streamarg );
      goto SINGLE_LINE; // HACK O'RAMA!
      }
   else if( d_argType == TEXTARG ) {
      stbuf = d_textarg.pText;
SINGLE_LINE: // HACK O'RAMA!
      const auto blen( stbuf.length() );
      Msg( "%s(%" PR_SIZET "u)->%sClip:\"%s\"", ArgTypeName(), blen, DestNm(), stbuf.c_str() );
      if( nullptr == (bufptr=PrepClip( 1+blen, hglbCopy )) )
         return false;

      memcpy( bufptr, stbuf.c_str(), 1+blen );
      }
   else {
      return BadArg();
      }

   const auto errmsg( WrToClipEMsg( hglbCopy ) );
   if( errmsg ) {
      return ErrPause( "%s", errmsg );
      }
   return nullptr == errmsg;
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
               Clipboard_PutText_Multiline( pClip );
               Win32::GlobalUnlock( hglb );
               Msg( "WinClip -> <clipboard> ok" );
               retVal = true;
               }
            }
         }
      Win32::CloseClipboard();
      }
   return retVal;
   }

void WinClipGetFirstLine( std::string &dest ) {
   if( !Win32::OpenClipboard( Win32::GetActiveWindow() ) )
      dest = ClipUnavail;
   else {
      if( !Win32::IsClipboardFormatAvailable(CF_TEXT) )
         dest = "CF_TEXT format data not available";
      else {
         auto hglb( Win32::GetClipboardData( CF_TEXT ) );
         if( !hglb )
            dest = "GetClipboardData failed";
         else {
            const auto pClip( PCChar(Win32::GlobalLock( hglb )) );
            if( !pClip )
               dest = "GlobalLock on ClipboardData failed";
            else {
               dest.assign( pClip, StrToNextOrEos( pClip, "\x0D\x0A" ) - pClip );
               Win32::GlobalUnlock( hglb );
               }
            }
         }
      Win32::CloseClipboard();
      }
   }

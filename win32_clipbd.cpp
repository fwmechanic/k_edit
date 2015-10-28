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
   if( !wrOk ) {
      Win32::GlobalFree( hglbCopy );
      }
   return wrOk ? nullptr : "Win32::SetClipboardData write failed" ;
   }

STATIC_FXN size_t sizeof_eol() { return 2; }
STATIC_FXN void cat_eol( PChar &bufptr ) {
   *bufptr++ = 0x0D; // line terminator '\r\n'
   *bufptr++ = 0x0A;
   }

STATIC_FXN PCChar DestNm() { return "Win"; }

#include "impl_towinclip.h"

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

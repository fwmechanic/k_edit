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
         if( g_pFbufClipboard->LineCount() == 0 ) {
            return ErrPause( "<clipboard> is empty!" );
            }
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
      if( nullptr == (bufptr=PrepClip( 1+size, hglbCopy )) ) { // ++ for trailing '\0'
         return false;
         }
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
      Msg( "%s(%" PR_SIZET ")->%sClip:\"%s\"", ArgTypeName(), blen, DestNm(), stbuf.c_str() );
      if( nullptr == (bufptr=PrepClip( 1+blen, hglbCopy )) ) {
         return false;
         }
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

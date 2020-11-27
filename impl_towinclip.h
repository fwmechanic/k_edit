//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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
     || d_argType == NULLARG
     || d_argType == LINEARG
     || d_argType == BOXARG
     ) {
      LINE  yMin, yMax;
      COL   xLeft(0), xRight(COL_MAX); // assume whole line
      PFBUF pFBuf(g_CurFBuf());
      PCChar srcNm(nullptr);
      if( d_argType == NOARG ) {
         pFBuf = g_pFbufClipboard;
         if( pFBuf->LineCount() == 0 ) {
            return ErrPause( "<clipboard> is empty!" );
            }
         yMin  = 0;
         yMax  = pFBuf->LastLine();
         srcNm = pFBuf->Name();
         }
      else if( d_argType == NULLARG ) {
         if( d_cArg == 1 ) { // NULLEOW
            pFBuf->DupLineSeg( stbuf, d_nullarg.cursor.lin, d_nullarg.cursor.col, COL_MAX );
            TermNulleow( stbuf );
            goto SINGLE_LINE; // HACK O'RAMA!
            }
         else { // d_cArg > 1
            yMin = 0;
            yMax = pFBuf->LastLine();
            srcNm = "this file";
            }
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
      Msg( "%s (%d lines, %ld bytes)->%sClip", srcNm ? srcNm : ArgTypeName(), (yMax - yMin)+1, size, DestNm() );
      if( nullptr == (bufptr=PrepClip( 1+size, hglbCopy )) ) { // 1+ for trailing '\0'
         return false;
         }
      }
      // copy source data into into *bufptr
      for( auto lineNum(yMin); lineNum <= yMax; ++lineNum ) {
         pFBuf->DupLineSeg( stbuf, lineNum, xLeft, xRight );
         memcpy( bufptr, stbuf.c_str(), stbuf.length() );
         bufptr += stbuf.length();
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
   const auto errmsg( WrToClipEMsg( hglbCopy ) ); // MUST be called to free e.g. bufptr=PrepClip
   if( errmsg ) {
      return ErrPause( "%s", errmsg );
      }
   return true;
   }

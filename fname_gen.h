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

#pragma once

#include "ed_main.h"

class PathStrGenerator {  // interface class
   NO_COPYCTOR(PathStrGenerator);
   NO_ASGN_OPR(PathStrGenerator);

public:
   PathStrGenerator() {}
   virtual bool VGetNextName( Path::str_t &dest ) = 0;
   virtual ~PathStrGenerator() {}
   };

class WildcardFilenameGenerator : public PathStrGenerator {
   NO_COPYCTOR(WildcardFilenameGenerator);
   NO_ASGN_OPR(WildcardFilenameGenerator);

   DirMatches d_dm;

public:
   WildcardFilenameGenerator( PCChar string, WildCardMatchMode matchMode=ONLY_FILES ) : d_dm( string, nullptr, matchMode ) {}
   // DTOR auto-generated
   bool VGetNextName( Path::str_t &dest ) override;
   };

class DirListGenerator : public PathStrGenerator {
   NO_COPYCTOR( DirListGenerator );
   NO_ASGN_OPR( DirListGenerator );

   StringListHead  d_input ;
   StringListHead  d_output;

   void AddName( stref name );

public:

   DirListGenerator( PCChar dirName=nullptr );
   ~DirListGenerator();

   bool VGetNextName( Path::str_t &dest ) override;
   };


class DicedOnDelimString : public DiceableString {
public:
   DicedOnDelimString( stref str, PCChar pszDelims )
      : DiceableString( str )
      {
      extern void ChopAscizzOnDelim( PChar cur, PCChar pszDelim );
      ChopAscizzOnDelim( d_heapString, pszDelims );
      }
   };

class StrSubstituterGenerator;  // "static"

class CfxFilenameGenerator : public PathStrGenerator {
   NO_COPYCTOR(CfxFilenameGenerator);
   NO_ASGN_OPR(CfxFilenameGenerator);

   WildcardFilenameGenerator *d_pWcGen = nullptr;
   StrSubstituterGenerator   *d_pSSG = nullptr;
   DicedOnDelimString         d_splitLine;
   WildCardMatchMode          d_matchMode;

   PCChar d_pszEntrySuffix = nullptr;

   public:

   CfxFilenameGenerator( stref macroText, WildCardMatchMode matchMode );
   bool VGetNextName( Path::str_t &dest ) override;
   virtual ~CfxFilenameGenerator();
   };

//-----------------------------------

class FilelistCfxFilenameGenerator : public PathStrGenerator {
   NO_COPYCTOR( FilelistCfxFilenameGenerator );
   NO_ASGN_OPR( FilelistCfxFilenameGenerator );

   PFBUF                  d_pFBuf;
   LINE                   d_curLine = 0;
   CfxFilenameGenerator  *d_pCfxGen = nullptr;
   Path::str_t            d_sbuf;

   public:

   FilelistCfxFilenameGenerator( PFBUF pFBuf ) : d_pFBuf( pFBuf ) {}
   bool VGetNextName( Path::str_t &dest ) override;
   virtual ~FilelistCfxFilenameGenerator() { Delete0( d_pCfxGen ); }
   };

//-----------------------------------

//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
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
   }; STD_TYPEDEFS( PathStrGenerator )

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

   void AddName( PCChar name );

public:

   DirListGenerator( PCChar dirName=nullptr );
   ~DirListGenerator();

   bool VGetNextName( Path::str_t &dest ) override;
   };


#define  DICING  1
#if      DICING

class DicedOnDelimString : public DiceableString {
public:
   DicedOnDelimString( PCChar str, PCChar pszDelims )
      : DiceableString( str )
      {
      extern void ChopAscizzOnDelim( PChar cur, PCChar pszDelim );
      ChopAscizzOnDelim( d_heapString, pszDelims );
      }
   };

#else

class SplitString {  // "static"
   PChar     d_heapString;
   int       d_argi = 0;
   int       d_argc = 0;
   PPChar    d_argv = nullptr;

   void init_SplitString( PCChar pszDelims );

public:
   SplitString( PCChar str, PCChar pszDelims );
   ~SplitString();
   PChar GetNext();
   void  Restart() { d_argi = 0; }
   void DBG() const {
      for( int ix=0; ix < d_argc ; ++ix )
         ::DBG( "%s: [%d]='%s'", __func__, ix, d_argv[ix] );
      }
   };

#endif

class StrSubstituterGenerator;  // "static"

class CfxFilenameGenerator : public PathStrGenerator {
   NO_COPYCTOR(CfxFilenameGenerator);
   NO_ASGN_OPR(CfxFilenameGenerator);

   WildcardFilenameGenerator *d_pWcGen = nullptr;
   StrSubstituterGenerator   *d_pSSG = nullptr;
#if DICING
   DicedOnDelimString
#else
   SplitString
#endif
                              d_splitLine;
   WildCardMatchMode          d_matchMode;

   PCChar d_pszEntrySuffix = nullptr;

   public:

   CfxFilenameGenerator( PCChar macroText, WildCardMatchMode matchMode );
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
   Path::str_t            d_xb;

   public:

   FilelistCfxFilenameGenerator( PFBUF pFBuf ) : d_pFBuf( pFBuf ) {}
   bool VGetNextName( Path::str_t &dest ) override;
   virtual ~FilelistCfxFilenameGenerator() { Delete0( d_pCfxGen ); }
   };

//-----------------------------------

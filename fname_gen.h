//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once

#include "ed_main.h"

class StringGenerator {  // interface class
   NO_COPYCTOR(StringGenerator);
   NO_ASGN_OPR(StringGenerator);

public:
   StringGenerator() {}
   virtual bool VGetNextName( std::string &dest ) = 0;
   virtual bool VGetNextName( PXbuf dest ) = 0;
   virtual ~StringGenerator() {}
   }; STD_TYPEDEFS( StringGenerator )

class WildcardFilenameGenerator : public StringGenerator {
   NO_COPYCTOR(WildcardFilenameGenerator);
   NO_ASGN_OPR(WildcardFilenameGenerator);

   DirMatches d_dm;

public:
   WildcardFilenameGenerator( PCChar string, WildCardMatchMode matchMode=ONLY_FILES ) : d_dm( string, nullptr, matchMode ) {}
   // DTOR auto-generated
   bool VGetNextName( std::string &dest ) override;
   bool VGetNextName( PXbuf dest ) override;
   };

class DirListGenerator : public StringGenerator {
   NO_COPYCTOR( DirListGenerator );
   NO_ASGN_OPR( DirListGenerator );

   StringListHead  d_input ;
   StringListHead  d_output;

   void AddName( PCChar name );

public:

   DirListGenerator( PCChar dirName=nullptr );
   ~DirListGenerator();

   bool VGetNextName( PXbuf dest ) override;
   bool VGetNextName( std::string &dest ) override;
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

class CfxFilenameGenerator : public StringGenerator {
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
   bool VGetNextName( PXbuf dest ) override;
   bool VGetNextName( std::string &dest ) override;
   virtual ~CfxFilenameGenerator();
   };

//-----------------------------------

class FilelistCfxFilenameGenerator : public StringGenerator {
   NO_COPYCTOR( FilelistCfxFilenameGenerator );
   NO_ASGN_OPR( FilelistCfxFilenameGenerator );

   PFBUF                  d_pFBuf;
   LINE                   d_curLine = 0;
   CfxFilenameGenerator  *d_pCfxGen = nullptr;
   Xbuf                   d_xb;

   public:

   FilelistCfxFilenameGenerator( PFBUF pFBuf ) : d_pFBuf( pFBuf ) {}
   bool VGetNextName( PXbuf dest ) override;
   bool VGetNextName( std::string &dest ) override;
   virtual ~FilelistCfxFilenameGenerator() { Delete0( d_pCfxGen ); }
   };

//-----------------------------------

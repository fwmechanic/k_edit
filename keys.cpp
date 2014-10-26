//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"


bool SetKeyOk( PCChar pszCmdName, PCChar pszKeyName ) {
   const auto EdKC( edkcFromKeyname( pszKeyName ) );
   if( EdKC ) {
      const auto pCmd( CmdFromName( pszCmdName ) );
      if( pCmd ) {
         g_Key2CmdTbl[ EdKC ] = pCmd;
         return true;
         }
      }

   return false;
   }

char CharAsciiFromKybd() {
   return CmdDataFromNextKey().Ascii;
   }

void EventCmdSupercede( PCMD pOldCmd, PCMD pNewCmd ) {
   for( auto &pCmd : g_Key2CmdTbl ) {
      if( pCmd == pOldCmd )
          pCmd =  pNewCmd;
      }
   }

void StrFromCmd( PChar dest, size_t sizeofDest, const CMD &CmdToFind ) {
   dest[0] = '\0';
   for( const auto &pCmd : g_Key2CmdTbl ) {
      if( pCmd == &CmdToFind ) {
         StrFromEdkc( dest, sizeofDest, &pCmd - g_Key2CmdTbl );
         break;
         }
      }
   }

PChar StringOfAllKeyNamesFnIsAssignedTo( PChar dest, size_t sizeofDest, PCCMD pCmdToFind, PCChar sep ) {
   dest[0] = '\0';
   if( pCmdToFind != pCMD_graphic ) {
      auto pCur( dest );
      for( const auto &pCmd : g_Key2CmdTbl ) {
         if( pCmd == pCmdToFind ) {
            if( pCur != dest ) {
               if( snprintf_full( &pCur, &sizeofDest, "%s", sep ) )
                  break;
               }
            if( KeyStr_full( &pCur, &sizeofDest, &pCmd - g_Key2CmdTbl ) )
               break;
            }
         }
      }
   return dest;
   }

void PAssignShowKeyAssignment( const CMD &Cmd, PFBUF pFBufToWrite ) {
   if( Cmd.IsFnUnassigned() || Cmd.IsFnGraphic() )
      return;

   FmtStr<50> cmdNm( "%-20s: ", Cmd.Name() );
   const PCChar pText( Cmd.IsRealMacro() ? Cmd.MacroText() :
#if AHELPSTRINGS
      (Cmd.d_HelpStr && *Cmd.d_HelpStr ? Cmd.d_HelpStr
#endif
      : ""
#if AHELPSTRINGS
      )
#endif
      );

   auto fFoundAssignment(false);
   for( const auto &pCmd : g_Key2CmdTbl ) {
      if( pCmd == &Cmd ) {
         char keyNm[50];
         StrFromEdkc( BSOB(keyNm), &pCmd - g_Key2CmdTbl );
         pFBufToWrite->PutLastLine( SprintfBuf( "%s%-*s # %s"
               , cmdNm.k_str()
               , g_MaxKeyNameLen
               , keyNm
               , !fFoundAssignment ? pText : "|"
               )
            );
         fFoundAssignment = true;
         }
      }

   if( !fFoundAssignment )
      pFBufToWrite->PutLastLine( SprintfBuf( "%s%-*s # %s", cmdNm.k_str(), g_MaxKeyNameLen, "", pText ) );
   }


STATIC_CONST char spinners[] = { '-', '\\', '|', '/' };

class SpinChar {
   int d_spindex;

public:
   SpinChar() : d_spindex(0) {}
   char next() {
      const auto rv( spinners[d_spindex++] );
      d_spindex %= sizeof(spinners);
      return rv;
      }
   };


void WaitForKey( int secondsToWait ) {
// if( Interpreter::Interpreting() )
//    return;

   secondsToWait = Min( secondsToWait, 1 );
   FlushKeyQueueAnythingFlushed();

   SpinChar sc;

   const auto timeEnd( time( nullptr ) + secondsToWait + 1 );
   time_t timeNow;
   auto maxWidth(0);
   do {
      if( FlushKeyQueueAnythingFlushed() )
         break;

      SleepMs( 50 );

      timeNow = time( nullptr );
      const auto spinner( sc.next() );
      FmtStr<71> msg( " You have %c%2Id%c seconds to press any key ", spinner, timeEnd - timeNow, spinner );
      const auto mlen( msg.Len() );
      NoLessThan( &maxWidth, mlen );
      VidWrStrColorFlush( DialogLine(), EditScreenCols() - mlen, msg.k_str(), mlen, g_colorError, true );

      } while( timeNow < timeEnd );

   VidWrStrColorFlush( DialogLine(), EditScreenCols() - maxWidth, "", 0, g_colorInfo, true );
   }

#ifdef fn_waitkey15

bool ARG::waitkey15() {
   WaitForKey( 15 );
   return true;
   }

#endif

int ShowAllUnassignedKeys( PFBUF pFBuf ) { // pFBuf may be 0 if caller is only interested in # of avail keys
   auto count(0);
   auto tblCol(0);
   Linebuf lbuf;

   const auto col_width( g_MaxKeyNameLen + 1 );
   enum { KBUF_WIDTH = 32 };
   Assert( g_MaxKeyNameLen <= KBUF_WIDTH );

   for( const auto &pCmd : g_Key2CmdTbl ) {
      if( pCmd->IsFnUnassigned() ) {
         char KeyStringBuf[KBUF_WIDTH+1];
         StrFromEdkc( BSOB(KeyStringBuf), &pCmd - g_Key2CmdTbl );
         if( KeyStringBuf[0] ) {
            ++count;
            if( pFBuf ) {
               sprintf( lbuf + (col_width * tblCol), "%-*s ", col_width-1, KeyStringBuf );
               if( tblCol++ == (g_CurWin()->d_Size.col / col_width) - 1 ) {
                  tblCol = 0;
                  pFBuf->PutLastLine( lbuf );
                  }
               }
            }
         }
      }

   if( pFBuf && tblCol > 0 ) {
      pFBuf->PutLastLine( lbuf );
      }

   return count;
   }

PCCMD CmdFromKbdForInfo( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ) {
   const auto cd( CmdDataFromNextKey_Keystr( pKeyStringBuffer, pKeyStringBufferBytes ) );
   return cd.EdKcEnum == 0 ? pCMD_unassigned : g_Key2CmdTbl[ cd.EdKcEnum ];
   }

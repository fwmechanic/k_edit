//
//  Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include <ncurses.h>
#include <stdio.h>
#include "conio.h"
#include "ed_main.h"

void ConIn::WaitForKey(){}

bool ConIn::FlushKeyQueueAnythingFlushed(){ return flushinp(); }

bool ConIO::Confirm( PCChar pszPrompt, ... ){ return false; }

int ConIO::DbgPopf( PCChar fmt, ... ){ return 0; }

EdKC_Ascii ConIn::CmdDataFromNextKey() {
   EdKC_Ascii rv;
   const auto newch( getch() );
   rv.Ascii    = newch;
   rv.EdKcEnum = newch;
   return rv;
   }

EdKC_Ascii ConIn::CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ) {
   const auto rv( ConIn::CmdDataFromNextKey() );
   pKeyStringBuffer[0] = rv.Ascii;
   pKeyStringBuffer[1] = 0;
   return rv;
   }

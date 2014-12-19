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
   rv.Ascii    = 'x';
   rv.EdKcEnum = 'x';
   return rv;
   }

EdKC_Ascii ConIn::CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ) {
   pKeyStringBuffer[0] = 'x';
   pKeyStringBuffer[1] = 0;
   EdKC_Ascii rv;
   rv.Ascii    = 'x';
   rv.EdKcEnum = 'x';
   return rv;
   }

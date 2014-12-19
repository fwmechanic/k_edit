//
//  Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include <ncurses.h>
#include <stdio.h>
#include "conio.h"
#include "ed_main.h"

CmdData CmdDataFromNextKey() {
   CmdData rv;
   rv.pszMacroDef = nullptr;
   return rv;
   }

void WaitForKey(){}

bool FlushKeyQueueAnythingFlushed(){ return flushinp(); }

bool ConIO::Confirm( PCChar pszPrompt, ... ){ return false; }

int ConIO::DbgPopf( PCChar fmt, ... ){ return 0; }

CmdData CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ) {
   pKeyStringBuffer[0] = 0;
   CmdData rv;
   rv.pszMacroDef = nullptr;
   return rv;
   }

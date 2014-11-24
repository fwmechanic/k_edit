//
// Copyright 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

void AssertDialog_( PCChar function, int line ) {
   fprintf( stderr, "Assertion failed, %s L %d", function, line );
   abort();
   }

Path::str_t Path::GetCwd() { // quick and dirty AND relies on GLIBC getcwd( nullptr, 0 ) semantics which are NONPORTABLE
   PChar mallocd_cwd = getcwd( nullptr, 0 );
   Path::str_t rv( mallocd_cwd );
   free( mallocd_cwd );
   return rv;
   }

PCChar OsVerStr() { return "Linux"; }

int EditorLoadCount() {
   return 1;
   }

size_t GetProcessMem() {
   return 1024;
   }

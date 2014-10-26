//
// Copyright 1991 - 2012 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#pragma once


#include "ed_main.h"
#include "my_strutils.h"

class Getopt
   {
   const int     d_argc;       // saved parameter
   const PPCChar d_argv;       // saved parameter
   const PCChar  d_pOptSet;    // saved parameter

   int           d_argi;       // index of which argument is next
   PCChar        d_pOptarg;    // pointer to argument of current option (if any)
   PCChar        d_pAddlOpt;   // remember next option char's location.  If d_pAddlOpt is not nullptr
                               // it is pointing into a string at an option character (a SW
                               // char has already been seen)
   char          d_chLastopt;  // for error reporting

   typedef char pathbuf[_MAX_PATH+1];


   protected:

   Path::str_t d_pgm; // name w/o path or extension

   public:

   virtual void VErrorOut( PCChar emsg ) = 0;

   Getopt( int argc_, PPCChar argv_, PCChar optset_ );
   int    NextOptCh();
   PCChar optarg()  const { return d_pOptarg; }
   PCChar nextarg() { return (d_argi < d_argc) ? d_argv[d_argi++] : nullptr; }
   };

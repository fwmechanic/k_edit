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
#include "my_strutils.h"

class Getopt
   {
   const int     d_argc;       // saved parameter
   const PPCChar d_argv;       // saved parameter
   const PCChar  d_pOptSet;    // saved parameter

   int           d_argi     = 1;       // index of which argument is next
   PCChar        d_pOptarg  = nullptr; // pointer to argument of current option (if any)
   PCChar        d_pAddlOpt = nullptr; // remember next option char's location.  If d_pAddlOpt is not nullptr
                                       // it is pointing into a string at an option character (a SW
                                       // char has already been seen)
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

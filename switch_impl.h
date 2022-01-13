//
// Copyright 2015-2018 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

// Disable a picky gcc-8 compiler warning
#if defined(__GNUC__) && (__GNUC__ >= 8)
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

//
//  Switch definition table defintions
//

class SWI_intf {
 public:
   SWI_intf() {}
   virtual ~SWI_intf() {}
   virtual std::string defn( stref newValue ) = 0; // { return "not implemented!"; }
   virtual std::string disp()                 = 0; // { return "not implemented!"; }
   };

struct enum_nm { int val; PCChar name; };

struct SWI_impl_factory {
   SWI_intf *SWIs( stref (* get_)(), stref (* set_)( stref ) );
   SWI_intf *SWIsb( void (*dsp_)( PChar dest, size_t sizeofDest ), void (* set_)( stref ) );
   SWI_intf *SWIi_bv( bool &var_ );
   SWI_intf *SWIi_iv( int &var_ );
   SWI_intf *SWIi_ci( int (*get_)(), void (*set_)(int), int (*min_)(), int (*max_)(), bool fUseConstrained_=true );
   SWI_intf *SWI_color( uint8_t &var_ );
   SWI_intf *SWI_chdisp( char &var_ );
   SWI_intf *SWI_enum( int (*get_)(), void (*set_)(int), const enum_nm *enums_, size_t num_enums_ );
   };

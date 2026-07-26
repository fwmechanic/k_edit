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

#include <memory>
#include <span>

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
   using Ptr = std::unique_ptr<SWI_intf>;

   Ptr SWIs( stref (* get_)(), stref (* set_)( stref ) );
   Ptr SWIsb( void (*dsp_)( span<char> dest ), void (* set_)( stref ) );
   Ptr SWIi_bv( bool &var_ );
   Ptr SWIi_iv( int &var_ );
   Ptr SWIi_ci( int (*get_)(), void (*set_)(int), int (*min_)(), int (*max_)(), bool fUseConstrained_=true );
   Ptr SWI_color( uint8_t &var_ );
   Ptr SWI_chdisp( char &var_ );
   Ptr SWI_enum( int (*get_)(), void (*set_)(int), span<const enum_nm> enums_ );
   };

static_assert( sizeof(SWI_impl_factory::Ptr) == sizeof(SWI_intf *) );
static_assert( sizeof(span<char>) == sizeof(char *) + sizeof(size_t) );

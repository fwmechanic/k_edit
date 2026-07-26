//
// Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#include "ed_main.h"

#include "switch_impl.h"

// toolbox:
STATIC_FXN std::string dispBool( bool val ) { return val ? "yes" : "   no"; }
STATIC_FXN std::string defnBool( bool &fChanged, bool &val, stref newValue ) {
   const auto oldVal( val );
   if(   0==cmpi( "no", newValue ) || ("0" == newValue ) ) {
      val = false;
      fChanged = val!=oldVal;
      return "";
      }
   if(   0==cmpi( "yes", newValue ) || ("1" == newValue) ) {
      val = true;
      fChanged = val!=oldVal;
      return "";
      }
   if(   0==cmpi( "invert", newValue ) || ("-" == newValue) ) {
      val = !val;
      fChanged = val!=oldVal;
      return "";
      }
   return FmtStr<200>( "Boolean switch needs 'yes', 'no', or 'invert' (0/1/-) value, not '%" PR_BSR "'", BSR(newValue) ).c_str();
   }

STATIC_FXN std::string dispInt( int val ) { return FmtStr<20>( "%d", val ).c_str(); }
STATIC_FXN std::string defnInt( stref newValue, bool &fChanged, int &val, int min=INT_MIN, int max=INT_MAX, bool fUseConstrained=true ) {
   int errno_; uintmax_t numVal_ull; stref txtConvd; UI bs; std::tie( errno_, numVal_ull, txtConvd, bs ) = conv_u( newValue, 10 );
   if( errno_ || numVal_ull > INT_MAX ) {
      if( txtConvd == newValue ) {
         return FmtStr<200>( "could not convert '%" PR_BSR "' to int", BSR(newValue) ).c_str();
         }
      else {
         return FmtStr<200>( "could not convert '%" PR_BSR "' (of '%" PR_BSR "') to int", BSR(txtConvd), BSR(newValue) ).c_str();
         }
      }
   int numVal_int( numVal_ull );
   int constrVal( numVal_int );
   Constrain( min, &constrVal, max );
   if( !fUseConstrained && constrVal != numVal_int ) {
      return FmtStr<50>( "%" PR_BSR " (%d) not within [%d..%d]", BSR(newValue), numVal_int, min, max ).c_str();
      }
   fChanged = constrVal != val;
   if( fChanged ) {
      val = constrVal;
      }
   return "";
   }

STATIC_FXN std::string dispUInt( int val ) { return FmtStr<20>( "%d", val ).c_str(); }
STATIC_FXN std::string defnUInt( stref newValue, bool &fChanged, UI &val, UI min=0, UI max=INT_MAX, bool fUseConstrained=true ) {
   int errno_; uintmax_t numVal_ull; stref txtConvd; UI bs; std::tie( errno_, numVal_ull, txtConvd, bs ) = conv_u( newValue, 10 );
   if( errno_ || numVal_ull > UINT_MAX ) {
      if( txtConvd == newValue ) {
         return FmtStr<200>( "could not convert '%" PR_BSR "' to int", BSR(newValue) ).c_str();
         }
      else {
         return FmtStr<200>( "could not convert '%" PR_BSR "' (of '%" PR_BSR "') to int", BSR(txtConvd), BSR(newValue) ).c_str();
         }
      }
   UI numVal_uint( numVal_ull );
   UI constrVal( numVal_uint );
   Constrain( min, &constrVal, max );
   if( !fUseConstrained && constrVal != numVal_uint ) {
      return FmtStr<50>( "%" PR_BSR " (%u) not within [%u..%u]", BSR(newValue), numVal_uint, min, max ).c_str();
      }
   fChanged = constrVal != val;
   if( fChanged ) {
      val = constrVal;
      }
   return "";
   }


class SWIs : public SWI_intf {
   stref (* const d_get)();
   stref (* const d_set)( stref );
 public:
   SWIs( stref (* get_)(), stref (* set_)( stref ) )
      : d_get(get_)
      , d_set(set_)
      {}
   std::string defn( stref newValue ) override { return std::string( d_set( newValue ) ); }
   std::string disp() override { return std::string( d_get() ); }
   };

class SWIsb : public SWI_intf {
   void  (* const d_dsp)( span<char> dest );
   void  (* const d_set)( stref );
 public:
   SWIsb( void (*dsp_)( span<char> dest ), void (* set_)( stref ) )
      : d_dsp(dsp_)
      , d_set(set_)
      {}
   std::string defn( stref newValue ) override { d_set( newValue ); return ""; }
   std::string disp() override {
      linebuf lbuf; lbuf[0] = '\0';
      d_dsp( span{lbuf} );
      return lbuf;
      }
   };

class SWIi_bv : public SWI_intf {
   bool &d_var;
 public:
   SWIi_bv( bool &var_ ) : SWI_intf(), d_var(var_) {}
   SWIi_bv(SWIi_bv&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      const auto rv( defnBool( fChanged, d_var, newValue ) );
      return rv;
      }
   std::string disp() override {
      return dispBool( d_var );
      }
   };

class SWIi_iv : public SWI_intf {
   int &d_var;
 public:
   SWIi_iv( int &var_ ) : d_var(var_) {}
   SWIi_iv(SWIi_iv&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      const auto rv( defnInt( newValue, fChanged, d_var ) );
      return rv;
      }
   std::string disp() override {
      return dispInt( d_var );
      }
   };

class SWIi_ci : public SWI_intf {
 protected:
   int   (* const d_get)();
   void  (* const d_set)(int);
   int   (* const d_min)();
   int   (* const d_max)();
   const bool d_fUseConstrained;
 public:
   SWIi_ci( int (*get_)(), void (*set_)(int), int (*min_)(), int (*max_)(), bool fUseConstrained_ )
      : d_get(get_)
      , d_set(set_)
      , d_min(min_)
      , d_max(max_)
      , d_fUseConstrained(fUseConstrained_)
      {}
   SWIi_ci(SWIi_ci&& mE) = default;
   std::string defn( stref newValue ) override {
      bool fChanged;
      int val = d_get();
      const auto rv( defnInt( newValue, fChanged, val, d_min(), d_max(), d_fUseConstrained ) );
      if( fChanged ) {
         d_set( val );
         }
      return rv;
      }
   std::string disp() override {
      return dispInt( d_get() );
      }
   };

class SWI_color : public SWI_intf {
   uint8_t &d_var;
 public:
   SWI_color( uint8_t &var_ ) : d_var(var_) {}
   std::string defn( stref newValue ) override {
      int errno_; uintmax_t newVal; stref txtConvd; UI bs; std::tie( errno_, newVal, txtConvd, bs ) = conv_u( newValue, 16 );
      if( errno_ ) {
         return FmtStr<200>( "could not convert %" PR_BSR "", BSR(newValue) ).c_str();
         }
      if( newVal > UCHAR_MAX ) {
         return FmtStr<200>( "value 0x%jX exceeds max allowed (0x%X)", newVal, UCHAR_MAX ).c_str();
         }
      if( d_var != newVal ) {
         d_var = newVal;
         DispNeedsRedrawTotal();  // if color is changed interactively or in a startup macro the change did not affect all lines w/o this change
         }
      return "";
      }
   std::string disp() override {
      return FmtStr<20>( "%02X", d_var ).c_str();
      }
   };

class SWI_chdisp : public SWI_intf {
   char &d_var;
 public:
   SWI_chdisp( char &var_ ) : d_var(var_) {}
   std::string defn( stref newValue ) override {
      int errno_; uintmax_t newVal; stref txtConvd; UI bs; std::tie( errno_, newVal, txtConvd, bs ) = conv_u( newValue, 10 );
      if( errno_ ) {
         d_var = newValue.length() == 1 ? newValue[0] : ' ';
         }
      else {
         if( newVal > UCHAR_MAX ) {
            return FmtStr<200>( "value 0x%jX exceeds max allowed (0x%X)", newVal, UCHAR_MAX ).c_str();
            }
         d_var = newVal;
         }
      DispNeedsRedrawAllLinesAllWindows();
      return "";
      }
   std::string disp() override {
      return FmtStr<20>( "0x%02X (%c)", d_var, d_var ).c_str();
      }
   };

class SWI_enum : public SWI_intf {
   int   (* const d_get)();
   void  (* const d_set)(int);
   const span<const enum_nm> d_enums;
   std::string    d_str_allowed_names;
 public:
   SWI_enum( int (*get_)(), void (*set_)(int), span<const enum_nm> enums_ )
      : d_get(get_)
      , d_set(set_)
      , d_enums(enums_)
      {
      d_str_allowed_names = "{ ";
      for( const auto &entry : d_enums ) {
         d_str_allowed_names += entry.name;
         d_str_allowed_names += ", ";
         }
      if( d_str_allowed_names.length() > 2 ) {
         d_str_allowed_names[d_str_allowed_names.length()-2] = ' ';
         d_str_allowed_names[d_str_allowed_names.length()-1] = '}';
         }
      }
   std::string defn( stref newValue ) override {
      for( const auto &entry : d_enums ) {
         if( 0==cmpi( newValue, entry.name ) ) {
            if( d_get() != entry.val ) {
               d_set( entry.val );
               }
            return "";
            }
         }
      return FmtStr<200>( "value %" PR_BSR " not in %" PR_BSR "", BSR(newValue), BSR(d_str_allowed_names) ).c_str();
      }
   std::string disp() override {
      const auto val( d_get() );
      for( const auto &entry : d_enums ) {
         if( 0==cmpi( val, entry.val ) ) {
            return entry.name;
            }
         }
      return "?";
      }
   };


SWI_impl_factory::Ptr SWI_impl_factory::SWIs( stref (* get_)(), stref (* set_)( stref ) )                                                { return std::make_unique<::SWIs>( get_,set_ )                                   ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWIsb( void (*dsp_)( span<char> dest ), void (* set_)( stref ) )                           { return std::make_unique<::SWIsb>( dsp_, set_ )                                 ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWIi_bv( bool &var_ )                                                                            { return std::make_unique<::SWIi_bv>( var_ )                                     ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWIi_iv( int &var_ )                                                                             { return std::make_unique<::SWIi_iv>( var_ )                                     ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWIi_ci( int (*get_)(), void (*set_)(int), int (*min_)(), int (*max_)(), bool fUseConstrained_ ) { return std::make_unique<::SWIi_ci>( get_, set_, min_, max_, fUseConstrained_ ) ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWI_color( uint8_t &var_ )                                                                       { return std::make_unique<::SWI_color>( var_ )                                   ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWI_chdisp( char &var_ )                                                                         { return std::make_unique<::SWI_chdisp>( var_ )                                  ; }
SWI_impl_factory::Ptr SWI_impl_factory::SWI_enum( int (*get_)(), void (*set_)(int), span<const enum_nm> enums_ )                    { return std::make_unique<::SWI_enum>( get_, set_, enums_ )                      ; }

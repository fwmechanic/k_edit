//
// Copyright 2015-2016 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

class RegexMatchCapture {
   sridx d_ofs;
   stref d_value;
public:
   RegexMatchCapture()                       : d_ofs(           eosr       ), d_value(    ) {}
   RegexMatchCapture( int ofs_, stref val_ ) : d_ofs(ofs_ < 0 ? eosr : ofs_), d_value(val_) {}
   bool  valid()  const { return d_ofs != eosr; }
   stref value()  const { return d_value; }
   sridx offset() const { return d_ofs  ; }
   };
typedef std::vector<RegexMatchCapture> RegexMatchCaptures;
extern int DbgDumpCaptures( RegexMatchCaptures &captures, PCChar tag );

#if USE_PCRE

#include "pcre2.h"

class  CompiledRegex;
extern CompiledRegex *Regex_Compile( stref pszSearchStr, bool fCase );
extern CompiledRegex *Regex_Delete0( CompiledRegex *pcr );
extern RegexMatchCaptures::size_type Regex_Match( CompiledRegex *pcr, RegexMatchCaptures &captures, stref haystack, COL haystack_offset, int pcre_exec_options );
extern void register_atexit_search();
extern stref RegexVersion();

#else

#define register_atexit_search()
STIL stref RegexVersion() { return ""; }

#endif

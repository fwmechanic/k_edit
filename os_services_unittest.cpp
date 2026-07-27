//
// Copyright 2026 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#define UNITTEST_OS_SERVICES
#include "os_services.cpp"

#include <cassert>
#include <cstdio>
#include <string>

int main() {
   constexpr auto oneCharName = "K_EDIT_ENV_TEST_A";
   constexpr auto multiCharName = "K_EDIT_ENV_TEST_ABC";

   unsetenv( oneCharName );
   unsetenv( multiCharName );

   assert( PutEnvChkOk("K_EDIT_ENV_TEST_A=one") );
   assert( std::string(getenv(oneCharName)) == "one" );
   assert( PutEnvChkOk("K_EDIT_ENV_TEST_ABC=three") );
   assert( std::string(getenv(multiCharName)) == "three" );
   assert( PutEnvChkOk("K_EDIT_ENV_TEST_ABC=") );
   assert( getenv(multiCharName) == nullptr );
   assert( !PutEnvChkOk("=missing-name") );
   assert( !PutEnvChkOk("K_EDIT_ENV_TEST_ABC") );

   unsetenv( oneCharName );
   puts( "PASS" );
   return 0;
   }

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

#define UNITTEST_LINUX_PROCESS
#include "linux_process.cpp"

#include <cassert>

STATIC_FXN void test_qx( PCChar command, int expectedStatus, PCChar expectedOutput ) {
   std::string output;
   assert( qx( output, command ) == expectedStatus );
   assert( output == expectedOutput );
   }

STATIC_FXN void test_ignored_graceful_signals() {
   piped_forker piper;
   piper.IgnoreGracefulSignalsForTest();
   assert( piper.ForkChildOk("sleep 10") );
   std::this_thread::sleep_for( std::chrono::milliseconds(50) );
   assert( piper.ReapForTest() == 128 + SIGKILL );
   }

STATIC_FXN void test_explicit_termination() {
   piped_forker piper;
   assert( piper.ForkChildOk("sleep 10") );
   assert( piper.TerminateProcessGroup() );
   const auto status( piper.ReapForTest() );
   assert( status == 128 + SIGTERM || status == 128 + SIGKILL );
   }

int main() {
   test_qx( "printf immediate", 0, "immediate" );
   test_qx( "printf failed; exit 7", 7, "failed" );
   test_qx( "sleep 0.05; printf delayed", 0, "delayed" );
   test_ignored_graceful_signals();
   test_explicit_termination();
   puts( "PASS" );
   return 0;
   }

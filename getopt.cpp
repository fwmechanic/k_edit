//
// Copyright 1991 - 2011 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "getopt.h"

STIL bool isSwitchPrefixCh( int ch ) {
   return
#if defined(_WIN32)
          ch == '/' ||
#endif
          ch == '-';
   }


Getopt::Getopt( int argc_, PPCChar argv_, PCChar optset_ )
   : d_argc( argc_ )
   , d_argv( argv_ )
   , d_pOptSet( optset_ )
   , d_argi( 1 )
   , d_pAddlOpt( nullptr )
   , d_pgm( Path::CpyFnm( d_argv[0] ) )
   {
   }

//
//   Parse the command line options, System V style.
//
//   Standard option syntax is:
//
//     option ::= SW [optLetter]* [argLetter space* argument]
//
//   where
//     - SW is either '/' or '-'
//     - there is no space before any optLetter or argLetter.
//     - opt/arg letters are alphabetic, not punctuation characters.
//     - optLetters, if present, must be matched in d_pOptSet.
//     - argLetters, if present, are found in d_pOptSet followed by ':'.
//     - argument is any white-space delimited string.  Note that it
//       can include the SW character.
//     - upper and lower case letters are distinct.
//
//   There may be multiple option clusters on a command line, each
//   beginning with a SW, but all must appear before any non-option
//   arguments (arguments not introduced by SW).  Opt/arg letters may
//   be repeated: it is up to the caller to decide if that is an error.
//
//   The character SW appearing alone as the last argument is an error.
//   The lead-in sequence SWSW ("--" or "//") causes itself and all the
//   rest of the line to be ignored (allowing non-options which begin
//   with the switch char).
//
//   The string *d_pOptSet allows valid opt/arg letters to be recognized.
//   argLetters are followed with ':'.  Getopt () returns the value of
//   the option character found, or 0 if no more options are in the
//   command line.  If option is an argLetter then the global d_pOptarg is
//   set to point to the argument string (having skipped any white-space).
//
//   The global d_argi is initially 1 and is always left as the index
//   of the next argument of d_argv[] which getopt has not taken.  Note
//   that if "--" or "//" are used then d_argi is stepped to the next
//   argument before getopt() returns 0.
//
//   If an error occurs, that is an SW char precedes an unknown letter,
//   then getopt() will return a '?' character and normally prints an
//   error message via perror().
//
//   For example, if the switch char is '-' and
//
//     *d_pOptSet == "A:F:PuU:wXZ:"
//
//   then 'P', 'u', 'w', and 'X' are option letters and 'F', 'U', 'Z'
//   are followed by arguments.  A valid command line may be:
//
//     aCommand  -uPFPi -X -A L someFile
//
//   where:
//     - 'u' and 'P' will be returned as isolated option letters.
//     - 'F' will return with "Pi" as its argument string.
//     - 'X' is an isolated option.
//     - 'A' will return with "L" as its argument.
//     - "someFile" is not an option, and terminates getOpt.  The
//       caller may collect remaining arguments using argv pointers.
//

int Getopt::NextOptCh() {
   if( d_argi < d_argc ) { // any arguments remain?
      if( !d_pAddlOpt ) { // not in midst of option group at present
         if( !(d_pAddlOpt=d_argv[d_argi]) || !isSwitchPrefixCh( *d_pAddlOpt++ ) )
            goto NOT_AN_OPTION;

         // d_pAddlOpt now points to char after SW
         if( isSwitchPrefixCh( *d_pAddlOpt ) ) { // two SW in a row means "stop processing options"
            d_argi++;
            goto NOT_AN_OPTION;
            }
         }

      const char ch( *d_pAddlOpt++ );
      if( ch == '\0' ) { // no more options?
         d_argi++;
         goto NOT_AN_OPTION;
         }

      const PCChar pOptMatch( strchr( d_pOptSet, ch ) );
      if( !pOptMatch || ch == ':' )
         VErrorOut( FmtStr<_MAX_PATH+30>( "%s: invalid option -%c\n", d_pgm.c_str(), ch ) );

      if( pOptMatch[1] == ':' ) { // ch takes arg?
         d_argi++;                     // next d_argv element is arg for this option
         if( *d_pAddlOpt == '\0' ) {   // at end of current d_argv element? try the next one...
            if( d_argc <= d_argi )     // no more d_argv's avail?
               VErrorOut( FmtStr<70>( "%s: option -%c missing argument\n", d_pgm.c_str(), ch ) );

            d_pAddlOpt = d_argv[d_argi++]; // more d_argv's avail...
            }
         d_pOptarg = d_pAddlOpt;
         d_pAddlOpt = nullptr;
         }
      else { // ch doesn't take arg
         if( *d_pAddlOpt == '\0' ) { // at end of d_argv[d_argi]?
            d_argi++;                // next time start on next d_argv entry
            d_pAddlOpt = nullptr;          // (pointer inside d_argv[d_argi])
            }
         d_pOptarg = nullptr;              // no arg
         }
      d_chLastopt = ch;
      return ch;
      }
   // ----------------------------------------------

NOT_AN_OPTION: // end of arguments seen

   d_pOptarg = d_pAddlOpt = nullptr;
   return (d_argi < d_argc) ? ' ' : 0;
   }

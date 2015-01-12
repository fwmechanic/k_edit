//
// Copyright 1991 - 2015 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"

// std::unique_ptr<int> up( new int(42) ); these _do_ compile, as
// std::shared_ptr<int> sp( new int(42) ); PoC of availability...

GLOBAL_CONST char szSpcTab[] = " \t";
GLOBAL_CONST char szMacroTerminators[] = "#";

bool CMD::IsFnCancel()     const { return &ARG::cancel     == d_func || fn_cancel     == d_func; }
bool CMD::IsFnUnassigned() const { return &ARG::unassigned == d_func || fn_unassigned == d_func; }
bool CMD::IsFnGraphic()    const { return &ARG::graphic    == d_func || fn_graphic    == d_func; }

GLOBAL_VAR PFBUF g_pFBufAssignLog;

GLOBAL_CONST char szAssignLog[] = "<a!>";

bool ARG::assignlog() { // toggle function
   0 && DBG( "%s %p", __func__, g_pFBufAssignLog );
   if( !g_pFBufAssignLog ) {
      // semantic: resume using any existing instance of szAssignLog, or create new instance
      FBOP::FindOrAddFBuf( szAssignLog, &g_pFBufAssignLog );
      AssignLogTag( __func__ );
      }
   else {
      // semantic: shutdown use of szAssignLog
      AssignLogTag( "manual shutdown" );
      g_pFBufAssignLog->UnsetGlobalPtr();
      }
   return true;
   }

void AssignLogTag( PCChar tag ) {
   0 && DBG( "===== %s ====================", tag );
   if( g_pFBufAssignLog )
       g_pFBufAssignLog->FmtLastLine( "===== %s ====================", tag );
   }

// ALLOCA_STRDUP( pszStringToAssign, slen, src.data(), src.length() )

bool AssignStrOk_( stref src, CPCChar __function__ ) { enum {DB=0};
                                                                DB && DBG( "%s 0(%" PR_BSR ")", __function__, BSR(src) );
   const auto ixNonb( FirstNonBlankOrEnd( src ) );
   src.remove_prefix( ixNonb );                                 DB && DBG( "%s 1(%" PR_BSR ")", __function__, BSR(src) );
   if( src.length() == 0 ) { return Msg( "(from %s) entirely blank", __function__ ); }
   const auto ixColon( src.find( ':' ) );
   if( stref::npos == ixColon ) return Msg( "(from %s) missing ':' in %" PR_BSR, __function__, BSR(src) );
   auto name( src.substr( 0, ixColon ) );                       0 && DB && DBG( "%s 2 %" PR_BSR "->%" PR_BSR "'", __function__, BSR(name), BSR(src) );
   rmv_trail_blanks( name );                                    0 && DB && DBG( "%s 3 %" PR_BSR "->%" PR_BSR "'", __function__, BSR(name), BSR(src) );
   src.remove_prefix( FirstNonBlankOrEnd( src, ixColon+1 ) );   DB && DBG( "%s 4 %" PR_BSR "->%" PR_BSR "'", __function__, BSR(name), BSR(src) );
   rmv_trail_blanks( src );                                     DB && DBG( "%s 5 %" PR_BSR "->%" PR_BSR "'", __function__, BSR(name), BSR(src) );
   if( '=' == src[0] ) {
      src.remove_prefix( FirstNonBlankOrEnd( src, 1 ) );        DB && DBG( "%s 6 %" PR_BSR "->%" PR_BSR "'", __function__, BSR(name), BSR(src) );
      const auto rv( DefineMacro( name, src ) );  DB && DBG( "DefineMacro(%" PR_BSR ")->%" PR_BSR " %s", BSR(name), BSR(src), rv?"true":"false" );
      return rv;
      }
   if( !CmdFromName( name ) ) {
      auto rv( SetSwitch( name, src ) );          DB && DBG( "SetSwitch(%" PR_BSR ")->%" PR_BSR " %s", BSR(name), BSR(src), rv?"true":"false" );
      return rv;
      }
   const auto BK_rv( BindKeyToCMD( name, src ) );
   switch( BK_rv ) {
      case SetKeyRV_OK    : DB && DBG( "key %" PR_BSR " ->CMD %" PR_BSR, BSR(src), BSR(name) );  return true;
      case SetKeyRV_BADKEY: return Msg( "%" PR_BSR " is an unknown key", BSR(src) );
      case SetKeyRV_BADCMD: return Msg( "%" PR_BSR " is an unknown CMD", BSR(name) );
      }
   return Msg( "should not get here" );
   }

// note that RSRCFILE_COMMENT_DELIM is allowed within a macro identifier (i.e.
// 'my#foobar' is a valid macro identifier), which this code handles, making it
// non-generic
STATIC_FXN sridx ToAssignCommentDelimOrEndSkipQuoted( stref src, sridx start ) {
   if( start < src.length() ) {
      char quoteCh( '\0' );
      for( auto it( src.cbegin() + start ) ; it != src.cend() ; ++it ) {
         if( RSRCFILE_COMMENT_DELIM == *it && (it == src.cbegin() || isBlank( *(it-1) )) ) {
            return std::distance( src.cbegin(), it );
            }
         SKIP_QUOTED_STR( quoteCh, it, src, NO_MATCH )
         }
      }
NO_MATCH:
   return std::distance( src.cbegin(), src.cend() );
   }

bool TruncComment_AssignStrOk_( stref src, CPCChar __function__ ) { enum {DB=1};
   const auto ixCD( ToAssignCommentDelimOrEndSkipQuoted( src, 0 ) );
   if( !atEnd( src, ixCD ) ) {
      src.remove_suffix( src.length() - ixCD );
      }
   return AssignStrOk_( src, __function__ );
   }

void CMD::RedefMacro( stref newDefn ) {
   FreeUp( d_argData.pszMacroDef, Strdup( newDefn ) );
   }

bool DefineMacro( stref pszMacroName, stref pszMacroCode ) { 0 && DBG( "%s '%" PR_BSR "'='%" PR_BSR "'", __func__, BSR(pszMacroName), BSR(pszMacroCode) );
   const auto pCmd( CmdFromName( pszMacroName ) );
   if( pCmd ) {
      if( !pCmd->IsRealMacro() )
         return Msg( "'%" PR_BSR "' is a non-macro function; cannot redefine as macro", BSR(pszMacroName) );

      // replace old macro-code string with new one
      pCmd->RedefMacro( pszMacroCode );
      if( g_pFBufAssignLog )  g_pFBufAssignLog->FmtLastLine( "REDEF %" PR_BSR "='%" PR_BSR "'", BSR(pszMacroName), BSR(pszMacroCode) );
      return true;
      }

   CmdIdxAddMacro( pszMacroName, pszMacroCode );
   if( g_pFBufAssignLog )  g_pFBufAssignLog->FmtLastLine( "NWDEF %" PR_BSR "='%" PR_BSR "'", BSR(pszMacroName), BSR(pszMacroCode) );
   return true;
   }

bool PushVariableMacro( PCChar pText ) { 0 && DBG( "%s+ '%s'", __func__, pText );
#if MACRO_BACKSLASH_ESCAPES
   const auto len( (2*Strlen( pText )) + 3 );
#else
   const auto len(    Strlen( pText )  + 3 );
#endif
   const auto pBuf( PChar( alloca( len ) ) );
   pBuf[0] = '"';
#if MACRO_BACKSLASH_ESCAPES
   // NB!: escapes every backslash in pText !!!
   //      This is a kludgy fix for the annoying requirement to escape '\' with
   //      '\' in macro text strings...
   //
   const auto newlen( DoubleBackslashes( pBuf+1, len-3, pText ) );
   pBuf[newlen  ] = '"';
   pBuf[newlen+1] = '\0';
#else
   memcpy( pBuf+1, pText, len-3 );
   pBuf[len-2] = '"';
   pBuf[len-1] = '\0';
#endif
   0 && DBG( "%s- '%s'", __func__, pBuf );
   return Interpreter::PushMacroStringOk( pBuf, Interpreter::variableMacro );
   }

bool ARG::RunMacro() { 0 && DBG( "%s '%s':='%s'", __func__, CmdName(), d_pCmd->d_argData.pszMacroDef );
   return Interpreter::PushMacroStringOk( d_pCmd->d_argData.pszMacroDef, 0 );
   }

#if 0 // see PushVariableMacro( os.date(fmtst) ) in k.luaedit
#ifdef fn_strftime

bool ARG::strftime() {
   switch( d_argType ) {
    default:      return BadArg();
    case TEXTARG: {
                  linebuf lbuf;
                  const auto now( time( nullptr ) );
                  ::strftime( BSOB(lbuf), d_textarg.pText, localtime( &now ) );
                  return PushVariableMacro( lbuf );
                  }
    }
   }
/*

 following editor macros use ARG::strftime

cur_dow      := arg "%A"       strftime
cur_hhmm     := arg "%H%M"     strftime
cur_hhmmss   := arg "%H%M%S"   strftime
cur_hh_mm_ss := arg "%H:%M:%S" strftime
cur_woy      := arg "%W"       strftime
cur_yy       := arg "%y"       strftime
cur_yymmdd   := arg "%y%m%d"   strftime
cur_yyyy     := arg "%Y"       strftime
cur_yyyymmdd := arg "%Y%m%d"   strftime

unfortunately, these cannot be used to for the arg string (textarg) for another function, since arg strings are not scoped to a macro

EX: xxx:= meta arg cur_yyyy "_wk_" cur_woy ".log" message
    when this executes, the strftime ending cur_yyyy "eats" both arg's (the one
    almost-leading xxx, and the one leading cur_yyyy; at this point, the text
    literals from the cur_ macros, combined with those in xxx, are typed into
    the current buffer, and NOARG message executes to end execution of xxx.

    Which is NOT the desired outcome!

 */

#else

/*
    The constructs below (e.g.  ARG::cur_min()) "solve" this problem by
    precluding need to use arg _within_ cur_woy.

EX: cur_yyyy:=cur_y4  # alias since cur_y4 is new name
    xxx:= meta arg cur_now cur_yyyy "_wk_" cur_woy ".log" message

    this is obviously less flexible (strftime format strings are cast into C++
    code vs.  macros)

    ; I added some sort of
    "auto-refresh" mechanism:

       if *s_cur_now was more than 2S in the past, refresh it

    under the assumption that the below functions will be invoked in direct
    sequence to form a filename or timestamp text.

 */

  // tslog:=endfile emacsnewl begline cur_s
  // tslog:alt+1
  // xe:=arg yyyymmdd_hhmmss cancel yyyymmdd_hhmmss:alt+6  arg curfile cancel

STATIC_FXN bool PushStrftimeMacro( PCChar fmtstr ) {
   STATIC_VAR struct tm* s_now_lt;
   { // hack to avoid a cur_now prefix edfxn (which updates the shared time datum (s_now))
   STATIC_VAR time_t s_now;
   const auto now( time( nullptr ) );
   if( nullptr==s_now_lt || (now-s_now)>1 ) {
      s_now = now;
      s_now_lt = localtime( &s_now );
      }
   }

   linebuf lbuf;
   strftime( BSOB(lbuf), fmtstr, s_now_lt );
   return PushVariableMacro( lbuf );
   }

bool ARG::cur_s   () { return PushStrftimeMacro( "%S" ); }
bool ARG::cur_min () { return PushStrftimeMacro( "%M" ); }
bool ARG::cur_h   () { return PushStrftimeMacro( "%H" ); }
bool ARG::cur_dow () { return PushStrftimeMacro( "%A" ); }
bool ARG::cur_woy () { return PushStrftimeMacro( "%W" ); }
bool ARG::cur_d   () { return PushStrftimeMacro( "%d" ); }
bool ARG::cur_mon () { return PushStrftimeMacro( "%m" ); }
bool ARG::cur_y2  () { return PushStrftimeMacro( "%y" ); }
bool ARG::cur_y4  () { return PushStrftimeMacro( "%Y" ); }

#endif
#endif

namespace Interpreter {
   enum { MAX_MACRO_NESTING = 32 };  // allowing infinite nesting is not a service to the programmer...

   class MacroRuntimeStkEntry {
      PCChar d_pStartOfText;
      PCChar d_pCurTxt     ;
      int    d_flags       ;

   public:
      void   clear();
      bool   ClearIsBreak();
      void   Ctor( PCChar pszMacroString, int macroFlags );

      enum   eGot { EXHAUSTED=0, GotLitCh, GotToken };
      eGot   GetNextTokenIsLiteralCh( PChar pDestBuf, int destBufLen );

      int    chGetAnyMacroPromptResponse();

      bool   BranchToLabel( PCChar pszBranchToken );
      bool   Breaks()          const { return ToBOOL(d_flags & breakOutHere ); }
      bool   IsVariableMacro() const { return ToBOOL(d_flags & variableMacro); }

   private:
      bool   Advance();
      // Advance commentary: the active pointer (d_pCurTxt) is kept pointing at the
      // next token or character.  Advance is called at the end of GetNextTokenIsLiteralCh,
      // just prior to the return.  Thus if GetNextTokenIsLiteralCh is returning the
      // last character of a literal string, d_pCurTxt will point outside that string,
      // at the next token/string in the macro stream.

      bool   NON_ESCAPED_DQUOTE() { return '"' == d_pCurTxt[0]
#if !MACRO_BACKSLASH_ESCAPES
                                                               && '"' != d_pCurTxt[1]
#endif

                                  ; }

      };

   STATIC_VAR  MacroRuntimeStkEntry  s_MacroRuntimeStack[ MAX_MACRO_NESTING ];
   STATIC_VAR  int                   s_ixPastTOS;

   STIL int                   ixPastTOS()     { return s_ixPastTOS; }
        bool                  Interpreting()  { return s_ixPastTOS > 0; }
   STIL MacroRuntimeStkEntry &TOS() { return s_MacroRuntimeStack[ s_ixPastTOS - 1 ]; }

   STATIC_FXN    bool  PopIsBreak();
   STATIC_FXN    bool  PopUntilBreak();
   STATIC_FXN    PCCMD AbortUntilBreak( PCChar emsg );
   STATIC_FXN    PCCMD CmdFromCurMacro();
   STATIC_FXN    bool  AnyStackEntryBreaksOut();
   } // namespace Interpreter

STATIC_FXN bool Interpreter::AnyStackEntryBreaksOut() {
   for( auto ix(0) ; ix < s_ixPastTOS ; ++ix )
      if( s_MacroRuntimeStack[ ix ].Breaks() )
         return true;

   return false;
   }

namespace Interpreter {
   bool  Interpreting_VariableMacro();
   }

bool Interpreter::Interpreting_VariableMacro() { return Interpreting() && TOS().IsVariableMacro(); }

// PushMacroStringOk - "enqueue" a macro string for later consumption/execution
//
// pszMacroString IS NOT EXECUTED by PushMacroStringOk!!! pszMacroString is
// Strdup'd, to be consumed "on demand".
//
bool Interpreter::PushMacroStringOk( PCChar pszMacroString, int macroFlags ) {
   if( ELEMENTS(s_MacroRuntimeStack) == ixPastTOS() )
      return ErrorDialogBeepf( "Macros nested too deep (%" PR_SIZET "u levels)! recursive macro defn?", ELEMENTS(s_MacroRuntimeStack) );

   0 && DBG( "PushMacStr[%d] '%s'", ixPastTOS(), pszMacroString );
   s_MacroRuntimeStack[ s_ixPastTOS++ ].Ctor( pszMacroString, macroFlags );
   return true;
   }

void Interpreter::MacroRuntimeStkEntry::Ctor( PCChar pszMacroString, int macroFlags ) {
   d_pStartOfText = d_pCurTxt = Strdup( StrPastAnyBlanks( pszMacroString ) );
   d_flags        = macroFlags;
   Advance();
   }

void Interpreter::MacroRuntimeStkEntry::clear() {
   0 && DBG( "Clear[%d]=%s|", s_ixPastTOS-1, d_pStartOfText );
   Free0( d_pStartOfText );
   d_pCurTxt = nullptr;
   d_flags = 0;
   }

bool Interpreter::MacroRuntimeStkEntry::ClearIsBreak() {
   const auto rv( Breaks() );
   clear();
   return rv;
   }

bool Interpreter::MacroRuntimeStkEntry::Advance() {
   if( d_flags & insideDQuotedString ) {
      if( NON_ESCAPED_DQUOTE() ) { 0 && DBG("-DQ  '%s'",d_pCurTxt);
         d_flags &= ~insideDQuotedString;
         d_pCurTxt++;
         goto TO_NXT_TOK;
         }
      }
   else {
TO_NXT_TOK:
      d_pCurTxt = StrPastAnyBlanks( d_pCurTxt );
      if( '"' == d_pCurTxt[0] ) { 0 && DBG("+DQ1 '%s'",d_pCurTxt);
         d_flags |= insideDQuotedString;
         d_pCurTxt++;
         }
      }
   return 0 != d_pCurTxt[0];
   }

int Interpreter::MacroRuntimeStkEntry::chGetAnyMacroPromptResponse() { // return int so we can return AskUser==-1
   // see: arg "Macro Prompt Directives" edhelp arg "LIMITATION" psearch
   0 && DBG("GetPrompt+ %X '%s'",d_flags,d_pCurTxt);
   if( d_flags & insideDQuotedString )    { return AskUser; }
   auto pC( d_pCurTxt ); if( *pC != '<' ) { return UseDflt; }
   ++pC;  if( 0 == *pC || ' ' == *pC )    { return AskUser; }
   const auto response( *pC );              0 && DBG( "macro prompt-response=%c!", response );
   d_pCurTxt = StrPastAnyBlanks( pC + 1 );
   return response;
   }

// if  rv, tos.d_pCurTxt points at token AFTER matching branch label
// if !rv, NO matching branch label was found!
bool Interpreter::MacroRuntimeStkEntry::BranchToLabel( PCChar pszBranchToken ) {
   // pszBranchToken[0] = ':';     // pszBranchToken[0] is branch prefix char [=+-]; change to label-DEFINITION prefix
   d_pCurTxt = d_pStartOfText;  // start from beginning
   linebuf token;  MacroRuntimeStkEntry::eGot got;
   while( EXHAUSTED != (got=GetNextTokenIsLiteralCh( BSOB(token) )) )
      if( GotToken==got && (':'==token[0]) && Stricmp( pszBranchToken+1, token+1 ) == 0 )
         return true;

   return false;
   }

Interpreter::MacroRuntimeStkEntry::eGot
Interpreter::MacroRuntimeStkEntry::GetNextTokenIsLiteralCh( PChar pDestBuf, int destBufLen ) {
                             0 && DBG("GetNxtTok+    %X '%s'",d_flags,d_pCurTxt);
   if( 0 == d_pCurTxt[0] ) { 0 && DBG("GetNxtTok-    %X EXHAUSTED",d_flags);
      return EXHAUSTED;
      }

   eGot rv;
   if( d_flags & insideDQuotedString ) {
      bool fEscaped = false;
      #if MACRO_BACKSLASH_ESCAPES
         if( '\\' == d_pCurTxt[0] ) {
            if( 0 == d_pCurTxt[1] )
               return false;

            d_pCurTxt++;     // skip escaping char
            fEscaped = true;
            }
      #else
         if( '"' == d_pCurTxt[0] && '"' == d_pCurTxt[1] ) {
            d_pCurTxt++;     // skip escaping char
            fEscaped = true;
            }
      #endif
                      0 && fEscaped && DBG( "ESCAPED '%c' !!!", d_pCurTxt[0] );  // stest:= "1 "" 2"""    " this is a test "

      // Assert( destBufLen >= 2 );
      pDestBuf[0] = *d_pCurTxt++;
      pDestBuf[1] = '\0';
      rv = GotLitCh;
      }
   else {
      while( '<' == d_pCurTxt[0] ) { // skip any Prompt Directives
         d_pCurTxt = StrPastAnyBlanks( StrToNextBlankOrEos( d_pCurTxt ) );
         }
      if( 0 == d_pCurTxt[0] ) {   0 && DBG("GetNxtTok-    %X EXHAUSTED",d_flags);
         return EXHAUSTED;
         }

      CPCChar pPastEndOfToken( StrToNextBlankOrEos( d_pCurTxt ) );
      const auto len( Min( static_cast<ptrdiff_t>(destBufLen-1), pPastEndOfToken - d_pCurTxt ) );
      memcpy( pDestBuf, d_pCurTxt, len );
      pDestBuf[len] = '\0';

      d_pCurTxt = pPastEndOfToken;
      rv = GotToken;
      }
                                  0 && DBG("GetNxtTok-%s %X '%s'",rv==GotToken?"tok":"lit",d_flags,pDestBuf);
   Advance();
   return rv;
   }


STATIC_FXN bool Interpreter::PopIsBreak() {
   const auto rv( TOS().ClearIsBreak() );
   --s_ixPastTOS;
   return rv;
   }

STATIC_FXN bool Interpreter::PopUntilBreak() {
   while( Interpreting() )
      if( PopIsBreak() )
         return true;

   return false;
   }

STATIC_VAR bool s_fRtndFrom_fExecuted_macro;

STIL PCCMD CleanupPendingMacroStream() {
   s_fRtndFrom_fExecuted_macro = Interpreter::PopUntilBreak();
   return nullptr;
   }

STATIC_FXN bool CleanupExecutionHaltRequests( const bool fCatchExecutionHaltRequests ) {
   if( Interpreter::Interpreting() ) // if the ExecutionHaltRequest was not created in the macro-decode process, any queued
      CleanupPendingMacroStream();   // macros are still there; MUST clean them up so they're not mistakenly used later

   if( fCatchExecutionHaltRequests ) { // the buck stops here?
      FlushKeyQueuePrimeScreenRedraw();
      DispRawDialogStr( FmtStr<80>( "*** Execution interrupted: %s ***", ExecutionHaltReason() ) );
      ClrExecutionHaltRequest();
      }
   return fCatchExecutionHaltRequests;
   }

void CleanupAnyExecutionHaltRequest() {
   if( ExecutionHaltRequested() ) {
      CleanupExecutionHaltRequests( true );
      }
   }

bool fExecuteSafe( PCChar str ) { // use ONLY when fExecute is _NOT_ called (even indirectly) by FetchAndExecuteCMDs(true)
   const auto rv( fExecute( str ) );
   CleanupAnyExecutionHaltRequest();
   return rv;
   }

void FetchAndExecuteCMDs( const bool fCatchExecutionHaltRequests ) {
   NOAUTO PCCMD prevPCMD( pCMD_unassigned );

   while( !s_fRtndFrom_fExecuted_macro ) {
      SelKeymapDisable();

      if(   ExecutionHaltRequested()
         && !CleanupExecutionHaltRequests( fCatchExecutionHaltRequests )
        ) {     // the buck stops _later_; exit this instance
         break; // DON'T RELY on s_fRtndFrom_fExecuted_macro being set (although it SHOULD be)
         }

      if( const auto newPCMD = CMD_reader().GetNextCMD() ) {
         if( !prevPCMD->IsFnGraphic() || !newPCMD->IsFnGraphic() )
            g_CurFBuf()->PutUndoBoundary();

         g_fFuncRetVal = newPCMD->BuildExecute();
         // _ASSERTE( _CrtCheckMemory() );
         prevPCMD = newPCMD;
         }
      }

   s_fRtndFrom_fExecuted_macro = false;
   }


STATIC_FXN PCCMD Interpreter::AbortUntilBreak( PCChar emsg ) {
   ErrorDialogBeepf( "%s", emsg );
   ClearArgAndSelection();
   FlushKeyQueuePrimeScreenRedraw();
   return CleanupPendingMacroStream();
   }


STATIC_FXN PCCMD Interpreter::CmdFromCurMacro() {
   Assert( Interpreting() );

   auto &tos( TOS() );

   linebuf token; NOAUTO MacroRuntimeStkEntry::eGot got;
   while( MacroRuntimeStkEntry::EXHAUSTED != (got=tos.GetNextTokenIsLiteralCh( BSOB(token) )) ) {
      if( ExecutionHaltRequested() )                // testme:= popd popd
         return CleanupPendingMacroStream();

      if( MacroRuntimeStkEntry::GotLitCh==got ) { 0 && DBG( "%s LIT '%c'", __func__, token[0] );
         STATIC_VAR CMD macro_graphic = { .d_name="macro_graphic", .d_func=&ARG::graphic };
         macro_graphic.d_argData.eka.Ascii    = token[0];
         macro_graphic.d_argData.eka.EdKcEnum = token[0];
         return &macro_graphic;
         }

      { 0 && DBG( "%s non-LIT '%s'", __func__, token );
      const auto pCmd( CmdFromName( token ) );
      if( pCmd ) { 0 && DBG( "%s CMD '%s'", __func__, pCmd->Name() );
         return pCmd;
         }
      }

      // OK, a token was found, but it's not a literal char, and it doesn't
      // match any known command.  Last choice is a branch command/defn:
      //
      const auto cond( token[0] );
      if( (':' == cond || '=' == cond || '+' == cond || '-' == cond) && '>' == token[1] ) {
         if(  ('=' == cond                  )  // branch always?
           || ('+' == cond &&  g_fFuncRetVal)  // branch if prev cmd true?
           || ('-' == cond && !g_fFuncRetVal)  // branch if prev cmd false?
           ) { // perform branch
            if( '\0' == token[2] ) // absence of dest-label means return from macro
               break;

            if( !tos.BranchToLabel( token ) )
               return AbortUntilBreak( FmtStr<BUFBYTES>( "Cannot find label '%s'", token+2 ) );
            }
         continue; // branch not taken or branch-label defn
         }
      return AbortUntilBreak( FmtStr<BUFBYTES>( "unknown function '%s'", token ) );
      }

   // break exited the loop: this means the macro exited normally: unnest one level
   //
   s_fRtndFrom_fExecuted_macro = PopIsBreak();
   return nullptr;
   }

int Interpreter::chGetAnyMacroPromptResponse() { // return int so we can return AskUser==-1
   return Interpreting() ? TOS().chGetAnyMacroPromptResponse() : AskUser;
   }


#ifdef fn_dispmstk

namespace Interpreter {
   STATIC_FXN void ShowStack( PFBUF pFBuf );
   };

STATIC_FXN void Interpreter::ShowStack( PFBUF pFBuf ) {
   pFBuf->PutLastLine( " " );

   auto nestLevel( ixPastTOS() );
   while( nestLevel > 0 ) {
      const auto &tos( s_MacroRuntimeStack[ --nestLevel ] );
      pFBuf->FmtLastLine( "[%d]%c%c%c%c='%s'"
          , nestLevel
          , (tos.d_flags & insideDQuotedString  ) ? '"' : '\''
          , (tos.IsVariableMacro()              ) ? 'D' : 'd'
          , (tos.Breaks()                       ) ? 'B' : 'b'
          ,  tos.d_pCurTxt
         );
      }
   }

bool ARG::dispmstk() {
   Interpreter::ShowStack( FBOP::FindOrAddFBuf( "<mstk>" ) );
   return true;
   }

#endif


PCCMD CmdFromKbdForExec() {
   const auto cmddata( ConIn::EdKC_Ascii_FromNextKey() );

   if( 0 == cmddata.EdKcEnum && ExecutionHaltRequested() ) { 0 && DBG( "CmdFromKbdForExec sending pCMD_cancel" );
      return pCMD_cancel;
      }

   if( cmddata.EdKcEnum >= EdKC_Count ) { DBG( "!!! KC=0x%X", cmddata.EdKcEnum );
      return pCMD_unassigned;
      }
#if 0 // to get every (valid) keystroke to display
   else {
      char kystr[50];
      StrFromEdkc( BSOB(kystr), cmddata.EdKcEnum );
      DBG( "EdKc=%s", kystr );
      }
#endif

   const auto pCmd( g_Key2CmdTbl[ cmddata.EdKcEnum ] );
   if( pCmd && !pCmd->IsRealMacro() )
        pCmd->d_argData.eka = cmddata;

   return pCmd;
   }


//
// How we record a command stream into a macro
//
// DON'T record macros, DO record all non-macro commands that they invoke
//
//   PROs: delivers perfect-fidelity replay; does not depend on all macros
//         being defined exactly as they were when the recording was taken.
//
//   CONs: recording does not show macros, leading to low understandability
//         of the recorded macro; it's all low-level functions.
//
//
// The alternative, to record only first level commands, including macro
// commands, removes the disadvantage of the chosen approach, but
//
// a) does not deliver perfect-fidelity replay if invoked macros are not defined
//    exactly as they were when the recording was taken,
//
// b) if a macro used to close recording, this macro is included in the recorded
//    macro, which will cause the recorded macro to start macro recording at the
//    end of the macro playback, which is nonsensical.
//

GLOBAL_VAR bool  g_fMacroRecordingActive;

STATIC_FXN int SaveCMDInMacroRecordFbuf( PCCMD pCmd );

STIL void RecordCmd( PCCMD pCmd ) {
   extern int g_fExecutingInternal;
   if(  !IsMacroRecordingActive()
      || g_fExecutingInternal      // don't record contents of fExecute's done by non-execute EdFuncs
      || pCmd->IsFnUnassigned()
      || fn_record == pCmd->d_func // don't record (closing) record function
     )
      return;

   SaveCMDInMacroRecordFbuf( pCmd );
   }


void  CMD_reader::VWritePrompt() {}
void  CMD_reader::VUnWritePrompt() {}
PCCMD CMD_reader::GetNextCMD_ExpandAnyMacros( const bool fRtnNullOnMacroRtn ) { 0 && DBG( "+%s", __func__ );
   while(1) {
      if( fRtnNullOnMacroRtn && s_fRtndFrom_fExecuted_macro ) { 0 && DBG( "-%s 0", __func__ );
         return nullptr;
         }

      0 && DBG( ":%s %s", __func__, Interpreter::Interpreting() ? "MACRO?" : "KB?" );

      // much of the complexity that follows is due to the difficulty of
      // deciding which CMD's need to be recorded-to-macro, or not
      // (see "How we record a command stream into a macro" above)
      //
      const auto fCmdFromInterpreter( Interpreter::Interpreting() );
      const auto fInterpreting_VariableMacro( fCmdFromInterpreter && Interpreter::Interpreting_VariableMacro() );
      const auto pCmd(
         fCmdFromInterpreter ?                                Interpreter::CmdFromCurMacro()
                             : ( VWritePrompt(), DispDoPendingRefreshes(), CmdFromKbdForExec() )
         );

      if( ExecutionHaltRequested() )
         return nullptr;

      d_fAnyInputFromKbd = !fCmdFromInterpreter;

      if( pCmd ) {
         if( !pCmd->ExecutesLikeMacro() ) { // not a macro?
            // The choice relating to VariableMacros is highly dependent on
            // the purpose of the macros being recorded:
            //
            // If the macro is being recorded for user-replay, then hiding the
            // expansion and recording the DeferredTextMacro command-name is the
            // correct/sensical approach, so that the expansion will be done at
            // macro runtime, yielding (uncertain yet) contemporaneous results.
            //
            // If the macro is being recorded for UNIT TEST purposes
            // (which implies a comparison between the current macro run and an
            // old macro run will ultimately be done), then recording the
            // expansion of the DeferredTextMacro, and hiding the name of the
            // DeferredTextMacro itself would be the correct/sensical approach,
            // so that old and new run output will be trivially comparable.
            //
            if( !fInterpreting_VariableMacro ) // we DON'T want to record the expansion of VariableMacros ...
               RecordCmd( pCmd );              // as these are typically situation-(time-)dependent

            0 && DBG( "-%s %s", __func__, pCmd->Name() );
            return pCmd;                       // caller will execute
            }

         if( !pCmd->IsRealMacro() && !fInterpreting_VariableMacro )
            RecordCmd( pCmd );

         //------------------------------------------------------------------------
         // pCmd->ExecutesLikeMacro():
         //
         //    BuildExecute() will call PushMacroStringOk, to enqueue the macro
         //    string for later consumption/execution, where "later" is the next
         //    iteration thru this loop, which will invoke
         //    Interpreter::CmdFromCurMacro()
         //
         g_fFuncRetVal = pCmd->BuildExecute();
         }
      }
   }


bool ARG::assign() {
   switch( d_argType ) {
    default:      return BadArg();
    case NOARG:  {const auto ok( TruncComment_AssignStrOk( g_CurFBuf()->PeekRawLine( d_noarg.cursor.lin ) ) );
                  if( !ok ) ErrorDialogBeepf( "assign failed" );
                  return ok;
                 }
    case TEXTARG:{const auto ok( TruncComment_AssignStrOk( d_textarg.pText ) );
                  if( !ok ) ErrorDialogBeepf( "assign failed" );
                  return ok;
                 }
    case LINEARG:{int assignsDone; Point errPt;
                  if( AssignLineRangeHadError( "user assign LINEARG", g_CurFBuf(), d_linearg.yMin, d_linearg.yMax, &assignsDone, &errPt ) ) {
                     errPt.ScrollTo();
                     Msg( "%d assign%s done; had error", assignsDone, Add_s( assignsDone ) );
                     return false;
                     }
                  Msg( "%d assign%s done", assignsDone, Add_s( assignsDone ) );
                  return true;
                  }

    case BOXARG:  for( ArgLineWalker aw( this ) ; !aw.Beyond() ; aw.NextLine() ) {
                     if( aw.GetLine() ) {
                        if( !TruncComment_AssignStrOk( aw.lineref() ) ) {
                           g_CurView()->MoveCursor( aw.Line(), aw.Col() );
                           return false;
                           }
                        }
                     }
                  return true;
    }
   }


void UnbindMacrosFromKeys() {
   for( auto &pCmd : g_Key2CmdTbl )
      if( pCmd->IsRealMacro() )
         pCmd = pCMD_unassigned;
   }


void FreeAllMacroDefs() {
   CmdIdxRmvCmdsByFunction( fn_runmacro() );
   }

// BUGBUG  doesn't seem to handle the "don't break line within macro string
// literal" problem!  Is this actually needed?
//
STATIC_FXN bool PutStringIntoCurfileAtCursor( PCChar pszString, std::string &tmp1, std::string &tmp2 ) {
   for( ; *pszString ; ++pszString )
      if( !PutCharIntoCurfileAtCursor( *pszString, tmp1, tmp2 ) )
         return false;

   return true;
   }

STATIC_FXN void PutMacroStringIntoCurfileAtCursor( PCChar pStr ) {
   if( g_CurFBuf()->CantModify() )
      return;

   const auto savefWordwrap( g_fWordwrap );
   g_fWordwrap = false; // we will wrap manually below to provide "  \" line continuation sequence

   std::string tmp1, tmp2;
   for( ; *pStr; ++pStr ) {
      if( ' ' == *pStr && g_CursorCol() >= g_iRmargin ) {
         PutStringIntoCurfileAtCursor( "  \\", tmp1, tmp2 );
         g_CurView()->MoveCursor( g_CursorLine() + 1, FBOP::GetSoftcrIndent( g_CurFBuf() ) );
         }
      else {
         PutCharIntoCurfileAtCursor( *pStr, tmp1, tmp2 );
         }
      }

   g_fWordwrap = savefWordwrap;
   }


bool ARG::tell() {
   linebuf keystringBuffer;

   PCCMD pCmd;
   switch( d_argType ) {
    default:      return BadArg();

    case NOARG:   Msg( "Press key to tell about:" );
                  pCmd = CmdFromKbdForInfo( BSOB(keystringBuffer) );
                  break;

    case TEXTARG: pCmd = CmdFromName( d_textarg.pText );
                  if( !pCmd )
                     return Msg( "%s is not an editor function or macro", d_textarg.pText );

                  keystringBuffer[0] = '\0';
                  break;
    }

   Linebuf outbuf;
   outbuf.Sprintf( "%s:%s", pCmd->Name(), keystringBuffer[0] ? keystringBuffer : pCmd->Name() );

   if( pCmd->IsRealMacro() )
      outbuf.SprintfCat( "  %s:=%s", pCmd->Name(), pCmd->MacroText() );
   else {
      linebuf lbuf;
      outbuf.SprintfCat( "  (%s)", ArgTypeNames( BSOB(lbuf), pCmd->d_argType ) );
      }

   if( d_fMeta )
      PutMacroStringIntoCurfileAtCursor( outbuf );
   else
      Msg( "%s", outbuf.k_str() );

   return fn_unassigned == pCmd->d_func;
   }


int StrToInt_variable_base( stref pszParam, int numberBase ) {
   if( (10 == numberBase || 16 == numberBase)
      && '0' == pszParam[0]
      && ('x' == pszParam[1] || 'X' == pszParam[1])
     ) {
      pszParam.remove_prefix( 2 );
      numberBase = 16;
      }

   if( numberBase < 2 || numberBase > 36 )
      return -1;

   auto accum(0);
   auto pC( pszParam.cbegin() );
   for( ; pC != pszParam.cend() ; ++pC ) {
      auto ch( *pC ); // cannot auto: *pC => const char, we need ch to be non-const
      if( isDecDigit( ch ) )            { ch -= '0'     ; }
      else if( ch >= 'a' && ch <= 'z' ) { ch -= 'a' - 10; }
      else if( ch >= 'A' && ch <= 'Z' ) { ch -= 'A' - 10; }
      else                              { break;          }

      if( ch >= numberBase )
         break;

      accum = (accum * numberBase) + ch;
      }

   const auto rv( std::distance( pszParam.cbegin(), pC ) ? accum : -1 );
   return rv;
   }


GLOBAL_VAR PFBUF g_pFbufRecord;

STATIC_VAR bool  s_fInRecordDQuote; // if set than output is in the middle of a '"'-delimited (literal) string

STIL bool RecordingInDQuote() { return s_fInRecordDQuote         ; }
STIL void SetInRecordDQuote() {        s_fInRecordDQuote = true  ; }
STIL void ClrInRecordDQuote() {        s_fInRecordDQuote = false ; }

STATIC_FXN int SaveCMDInMacroRecordFbuf( PCCMD pCmd ) {
   const auto lastLine( g_pFbufRecord->LastLine() );
   std::string st; g_pFbufRecord->DupRawLine( st, lastLine );
   linebuf lbufNew;
   if( pCmd->IsFnGraphic() ) {
      auto pNew( lbufNew );
      if( !RecordingInDQuote() ) {
         *pNew++ = ' ';
         *pNew++ = '"';
         SetInRecordDQuote();
         }
      const auto ch( pCmd->d_argData.chAscii() );  0 && DBG( "record ascii '%c'", ch );
#if MACRO_BACKSLASH_ESCAPES
      if( '"' == ch || '\\' == ch )
         *pNew++ = '\\'; // escape!
#else
      if( '"' == ch )
         *pNew++ = '"';  // double all literal '"'s!
#endif
      *pNew++ = ch;
      *pNew   = '\0';
      }
   else {
      if( RecordingInDQuote() ) {
         st += "\" ";
         ClrInRecordDQuote();
         }
      else {
         st += " ";
         }

      SafeStrcpy( lbufNew, pCmd->Name() );
      }

   std::string stmp;
   if( st.length() + Strlen( lbufNew ) > g_iRmargin ) { // wrap to next line
      st += " \\";
      g_pFbufRecord->PutLine( lastLine  , st     , stmp );
      g_pFbufRecord->PutLine( lastLine+1, lbufNew, stmp );
      }
   else {
      st += lbufNew;
      g_pFbufRecord->PutLine( lastLine  , st     , stmp );
      }

   MoveCursorToEofAllWindows( g_pFbufRecord );
   return 1;
   }


STATIC_FXN void PrintMacroDefToRecordFile( PCMD pCmd ) {
   STATIC_CONST char kszContinuation[] = "  \\";

   std::string sbuf;
   std::string stmp;
   auto pMacroTextChunk( pCmd->MacroText() );
   while( 1 ) {
      g_pFbufRecord->DupRawLine( sbuf, g_pFbufRecord->LastLine() );
      auto pC( pMacroTextChunk + Min( g_iRmargin - sbuf.length() + int(ELEMENTS(kszContinuation)), static_cast<sridx>(Strlen( pMacroTextChunk )) ) );
      for( ; pC > pMacroTextChunk; --pC )
         if( 0 == *pC || ' ' == *pC || HTAB == *pC )
            break;

      auto fDone( false );
      if( *pC != 0 && pC != pMacroTextChunk ) {
         sbuf.append( pMacroTextChunk, pC - pMacroTextChunk );
         sbuf.append( kszContinuation, KSTRLEN(kszContinuation) );
         pMacroTextChunk = pC + 1;
         }
      else {
         sbuf.append( pMacroTextChunk );
         fDone = true;
         }

      g_pFbufRecord->PutLine( g_pFbufRecord->LastLine(), sbuf, stmp );

      if( fDone )
         return;
      }
   }


//
//  Record
//
//  The Record function toggles macro recording.
//
//  While a macro is being recorded, the editor displays the message "Next
//  file is <record>" on the prompt line, And the letters "REC" on the
//  status bar.
//
//  When macro recording is stopped, the editor assigns the recorded
//  commands to the default macro name 'playback'. During the recording,
//  the editor writes the name of each command to the definition of
//  playback in the <record> file, which can be viewed as it is updated.
//
//  NOTE: Macro recording does not record changes in cursor position
//        accomplished by clicking the mouse. Use the keyboard if you want
//        to include cursor movements in a macro.
//
//  Record
//
//      Toggles macro recording on and off.
//
//  Arg <textarg> Record
//
//      Turns on recording if it is off and assigns the name specified in
//      the text argument to the recorded macro. Turns off recording if it
//      is turned on.
//
//  Meta Record
//
//      Toggles macro recording. While recording, no editing commands are
//      executed until recording is turned off. Use this form of the
//      function to record a macro without modifying your file.
//
//  Arg ... Record
//
//      As above but if the target macro already exists, the commands are
//      appended to the end of the macro.
//
//  Returns
//
//  True:  Recording turned on.
//  False: Recording turned off.
//

GLOBAL_CONST char szRecord[] = "<record>";

GLOBAL_VAR bool g_fCmdXeqInhibitedByRecord;

bool ARG::record() {
   STATIC_VAR auto s_macro_defn_first_line(-1);
   auto fUsersMacroName( false );

   if( IsMacroRecordingActive() ) { 0 && DBG( "###########  %s w/fMacroRecordingActive, %s", __func__, ArgTypeName() );
      g_fMacroRecordingActive    = false;
      g_fCmdXeqInhibitedByRecord = false;

      if( 0 && NOARG == d_argType ) {
         DispNeedsRedrawStatLn();
         return false; // always false
         }

      if( RecordingInDQuote() ) {
         ClrInRecordDQuote();
         std::string src, stmp;
         g_pFbufRecord->DupRawLine( src, g_pFbufRecord->LastLine() );
         src.append( "\"" );
         g_pFbufRecord->PutLine( g_pFbufRecord->LastLine(), src, stmp );
         }

      int   assignsDone;
      Point errPt;
      if( AssignLineRangeHadError( "record", g_pFbufRecord, s_macro_defn_first_line, g_pFbufRecord->LastLine(), &assignsDone, &errPt ) )
         ErrorDialogBeepf( "Error assigning <record> contents at line %d!", errPt.lin );

      Msg( "%d assign%s done", assignsDone, Add_s( assignsDone ) );
      }
   else { 0 && DBG( "###########  %s w/!fMacroRecordingActive, %s", __func__, ArgTypeName() );
      STATIC_VAR PCMD s_pCmdRecordMacro;
      PCChar pMacroName;
      if( TEXTARG == d_argType ) {
         fUsersMacroName = true;
         pMacroName = d_textarg.pText; // fall thru into NULLARG case
         }
      else {
         pMacroName = s_pCmdRecordMacro ? s_pCmdRecordMacro->Name() : "playback";
         }

      s_pCmdRecordMacro = CmdFromName( pMacroName );
      if(  !s_pCmdRecordMacro
        && (  !DefineMacro( pMacroName, "" )  // define empty macro failed
           || nullptr == (s_pCmdRecordMacro=CmdFromName( pMacroName )) // couldn't find it once defined?
           )
        ) {
         return false;
         }

      const auto f_cArg_GT_1( d_cArg > 1 );
      if( !f_cArg_GT_1 || fUsersMacroName ) {
#ifdef RECORD_CLEARS_RECORDBUF
         g_pFbufRecord->MakeEmpty();  // old default
#else
         if( g_pFbufRecord->LineCount() > 0 ) { g_pFbufRecord->PutLastLine( " " ); }
#endif
         g_pFbufRecord->PutLastLine( SprintfBuf( "%s:=", pMacroName ) );
         s_macro_defn_first_line = g_pFbufRecord->LastLine();

         if( f_cArg_GT_1 )
            PrintMacroDefToRecordFile( s_pCmdRecordMacro );
         }

      g_fMacroRecordingActive = true;
      ClrInRecordDQuote();

      g_fCmdXeqInhibitedByRecord = d_fMeta;
      if( d_fMeta ) {
         linebuf     kynmBuf;
         StrFromCmd( BSOB(kynmBuf), *pCMD_record );
         Msg( "No-Execute Record Mode - Press %s to resume normal editing", kynmBuf );
         }
      }

   DispNeedsRedrawStatLn();
   return IsMacroRecordingActive();
   }


STATIC_FXN stref ParseRawMacroText_ContinuesNextLine( stref src, bool &continues ) {
   enum states { outsideQuote, inQuote, prevCharBlank, contCharSeen };

   states stateWhereBlankLastSeen( outsideQuote );
   states state( outsideQuote );
   auto fChIsBlank( true );

   #if 0
   auto showStChange = []( int line, states statevar, states newval, PCChar pC, PCChar pC_start ) {
      DBG( "[%3d] %c: St %d -> %d L%d", pC - pC_start, *pC, statevar, newval, line );
      };
   #define  ChangeState( newval )  ( state = newval, showStChange( __LINE__, state, newval, pDest, pHeapBuf ) )
   #else
   #define  ChangeState( newval )  ( state = newval )
   #endif

   auto itEarlyTerm       ( src.cend() );
   auto itContinuationChar( src.cend() );
   for( auto it( src.cbegin() ) ; it != src.cend() ; ++it ) {
      if( RSRCFILE_COMMENT_DELIM == *it && fChIsBlank && outsideQuote == stateWhereBlankLastSeen ) {
         itEarlyTerm = it; // term string early and force a break from innerLoop
         break;
         }

      fChIsBlank = isBlank( *it );

      switch( state ) {
       default:             break;

       case outsideQuote:   if( fChIsBlank ) {
                               stateWhereBlankLastSeen = state;
                               ChangeState( prevCharBlank );
                               }
                            else if( '"' == *it ) {
                               ChangeState( inQuote );
                               }
                            break;

       case inQuote:        if( fChIsBlank ) {
                               stateWhereBlankLastSeen = state;
                               ChangeState( prevCharBlank );
                               }
                            else if( '"' == *it ) {
                               ChangeState( outsideQuote );
                               }
                            break;

       case prevCharBlank:  if( '\\' == *it ) { // continuation char seen?
                               ChangeState( contCharSeen );
                               itContinuationChar = it;
                               break;
                               }
                            //lint -fallthrough
       case contCharSeen:   if( !fChIsBlank ) {
                               if( '"' == *it )
                                  ChangeState( (inQuote == stateWhereBlankLastSeen) ? outsideQuote : inQuote );
                               else
                                  ChangeState( stateWhereBlankLastSeen );
                               }
                            break;
       }
      }
   #undef  ChangeState

   continues = contCharSeen == state;
   if( continues && itEarlyTerm == src.cend() ) {
      itEarlyTerm = itContinuationChar - 1; // the continuation char is always preceded by a space which is NOT included in the macro text
      }

   const auto rv( src.substr( 0, std::distance( src.cbegin(), itEarlyTerm ) ) );
   0 && DBG( "--> %" PR_BSR "|", BSR(rv) );
   return rv;
   }

bool AssignLineRangeHadError( PCChar title, PFBUF pFBuf, LINE yStart, LINE yEnd, int *pAssignsDone, Point *pErrorPt ) { enum {DBGEN=0};
   DBGEN && DBG( "%s: {%s} L [%d..%d]", __func__, title, yStart, yEnd );
   if( yEnd < 0 || yEnd > pFBuf->LastLine() ) {
      yEnd = pFBuf->LastLine();
      }

   enum AL2MSS { HAVE_CONTENT, FOUND_TAG, BLANK_LINE, };
   std::string d_dest;
   auto d_yMacCur = yStart;
   // reads lines until A MACRO definition is complete (IOW, an EoL w/o continuation char is reached)
   auto AppendLineToMacroSrcString = [&]() -> AL2MSS  {
      for( ; d_yMacCur <= yEnd ; ++d_yMacCur ) {
         const auto rl( pFBuf->PeekRawLine( d_yMacCur ) );
         if( !IsolateTagStr( rl ).empty() ) { DBGEN && DBG( "L %d TAG VIOLATION", d_yMacCur );
            return FOUND_TAG;
            }

         auto continues( false );
         const auto parsed( ParseRawMacroText_ContinuesNextLine( rl, continues ) );
         d_dest.append( parsed.data(), parsed.length() );  DBGEN && DBG( "%c+> %" PR_BSR "|", continues?'C':'c', BSR(d_dest) );
         if( !continues && !IsStringBlank( d_dest ) ) {
            DBGEN && DBG( "RTN HvContent" );
            return HAVE_CONTENT; // we got SOME text in the buffer, and the parser says there is no continuation to the next line
            }
         }
      DBGEN && DBG( "RTN ?" );
      return !IsStringBlank( d_dest ) ? HAVE_CONTENT : BLANK_LINE;
      };

   auto fContinueScan( true ); auto fAssignError( false ); auto assignsDone( 0 );
   for( ; fContinueScan && d_yMacCur <= yEnd ; ++d_yMacCur ) {  DBGEN && DBG( "%s L %d", __func__, d_yMacCur );
      const auto rslt( AppendLineToMacroSrcString() );
      DBGEN && DBG( "%s L %d rslt=%d", __func__, d_yMacCur, rslt );
      switch( rslt ) {
         case HAVE_CONTENT       :  DBGEN && DBG( "assigning --- |%s|", d_dest.c_str() );
                                    if( !AssignStrOk( d_dest ) ) {         DBGEN && DBG( "%s atom failed '%s'", __func__, d_dest.c_str() );
                                       if( pErrorPt ) {
                                          pErrorPt->Set( d_yMacCur, 0 );
                                          }
                                       fAssignError = true;
                                       }
                                    else {                                 DBGEN && DBG( "%s atom OKOKOK '%s'", __func__, d_dest.c_str() );
                                       ++assignsDone;
                                       }
                                    d_dest.clear();
                                    break;
         case BLANK_LINE         :  /* keep going */        break;
         case FOUND_TAG          :  fContinueScan = false;  break;
         default                 :  fContinueScan = false;  break;
         }
      }
   DBGEN && DBG( "%s: {%s} L [%d..%d/%d] = %d", __func__, title, yStart, d_yMacCur, yEnd, assignsDone );

   if( pAssignsDone )  *pAssignsDone = assignsDone;

   return fAssignError;
   }

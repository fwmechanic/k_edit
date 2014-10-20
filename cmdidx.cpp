//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"
#include "krbtree.h"

GLOBAL_VAR CMD g_CmdTable[] = { // *** THIS IS GLOBAL, NOT STATIC, BECAUSE #defines of pCMD_xxx in cmdtbl.kh refer to it
//-----------------------------------
#define CMDTBL_H_CMD_TBL_INITIALIZERS
#include "cmdtbl.h" // <-- Lua-generated array initializers and #defines
#undef  CMDTBL_H_CMD_TBL_INITIALIZERS
//-----------------------------------
   };

// s_CmdIdxBuiltins is a const tree, never pruned or added to after CmdIdxInit()
// is called (contains only statically allocated C++ CMDs, which cannot be
// freed!!!)

STATIC_VAR PRbTree s_CmdIdxBuiltins;

// s_CmdIdxAddins is a dynamic tree which is searched prior to s_CmdIdxBuiltins
// during user-function-lookups: all CMDs indexed herein (for macros and Lua
// functions) are heap-allocated, and can thus be created and destroyed at will:

GLOBAL_VAR PRbTree s_CmdIdxAddins;

STIL PCMD IdxNodeToPCMD( PRbNode pNd ) { return static_cast<PCMD>( rb_val(pNd) ); }  // type-safe conversion function

int rb_strcmpi( PCVoid p1, PCVoid p2 ) {
   return Stricmp( static_cast<PCChar>(p1), static_cast<PCChar>(p2) );
   }

STATIC_VAR RbCtrl s_CmdIdxRbCtrl = { AllocNZ_, Free_, };

void CmdIdxInit() {
   s_CmdIdxAddins   = rb_alloc_tree( &s_CmdIdxRbCtrl );
   s_CmdIdxBuiltins = rb_alloc_tree( &s_CmdIdxRbCtrl );
   for( auto &cmd : g_CmdTable ) {
      rb_insert_gen( s_CmdIdxBuiltins, cmd.Name(), rb_strcmpi, &cmd );
      }
   }

STATIC_FXN void CMD_PlacementFree_SameName( PCMD pCmd ) {
   if( pCmd->IsRealMacro() )
      Free0( pCmd->d_argData.pszMacroDef );

   AHELP( Free0( pCmd->d_HelpStr ); )
   }

STATIC_FXN void DeleteCMD( void *pData, void *pExtra ) {
   auto pCmd( static_cast<PCMD>(pData) );
   CMD_PlacementFree_SameName( pCmd );
   Free0( pCmd->d_name );
   Free0( pCmd );
   }

void CmdIdxClose() {
   cmdusage_updt();
   rb_dealloc_treev( s_CmdIdxAddins, nullptr, DeleteCMD );
   rb_dealloc_tree(  s_CmdIdxBuiltins );
   }

PCMD CmdFromName( PCChar name ) {
   auto pNd( rb_find_gen( s_CmdIdxAddins  , name, rb_strcmpi ) );
   if( !pNd )  pNd = rb_find_gen( s_CmdIdxBuiltins, name, rb_strcmpi );
   return pNd ? IdxNodeToPCMD( pNd ) : nullptr;
   }

STATIC_FXN PCMD CmdFromNameBuiltinOnly( PCChar name ) {
   auto pNd( rb_find_gen( s_CmdIdxBuiltins, name, rb_strcmpi ) );
   return pNd ? IdxNodeToPCMD( pNd ) : nullptr;
   }

STATIC_FXN void cmdIdxAdd( PCChar name, funcCmd pFxn, int argType, PCChar macroDef  _AHELP( PCChar helpStr ) ) {
   int equal;
   auto pNd( rb_find_gte_gen( &equal, s_CmdIdxAddins, name, rb_strcmpi ) );
   PCMD pCmd;
   if( equal ) {
      pCmd = IdxNodeToPCMD( pNd );
      CMD_PlacementFree_SameName( pCmd );
      }
   else {
      AllocArrayZ( pCmd );
      pCmd->d_name = Strdup( name );
      rb_insert_before( s_CmdIdxAddins, pNd, pCmd->Name(), pCmd );
      }

   pCmd->d_func    = pFxn;
   pCmd->d_argType = argType;
   if( macroDef )
      pCmd->d_argData.pszMacroDef = Strdup( macroDef );

   AHELP( pCmd->d_HelpStr = Strdup( helpStr ? helpStr : "" ); )

   // semi-hacky: replace any references to same-named builtin function
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( name ) );
   if( pCmdBuiltIn )
      EventCmdSupercede( pCmdBuiltIn, pCmd );
   }

void CmdIdxAddLuaFunc( PCChar name, funcCmd pFxn, int argType  _AHELP( PCChar helpStr ) ) {
   return cmdIdxAdd( name, pFxn, argType, nullptr  _AHELP( helpStr ) );
   }

void CmdIdxAddMacro( PCChar name, PCChar macroDef ) {
   return cmdIdxAdd( name, fn_runmacro(), (KEEPMETA + MACROFUNC), macroDef  _AHELP( nullptr ) );
   }

STATIC_FXN void DeleteCmd( PCMD pCmd ) {
   { // revert to builtin CMD of same name, if any
   const auto pCmdBuiltIn( CmdFromNameBuiltinOnly( pCmd->d_name ) );
   EventCmdSupercede( pCmd, pCmdBuiltIn ? pCmdBuiltIn : pCMD_unassigned );
   }

   Free0( pCmd->d_name );
   if( pCmd->IsRealMacro() )
      Free0( pCmd->d_argData.pszMacroDef );

   AHELP( Free0( pCmd->d_HelpStr ); )
   Free0( pCmd );
   }

int CmdIdxRmvCmdsByFunction( funcCmd pFxn ) {
   auto rv(0);
   PCChar pPrevKey(nullptr);
   for( auto pNd=rb_first( s_CmdIdxAddins ) ; pNd != rb_last( s_CmdIdxAddins ); pNd = rb_next( pNd ) ) {
      {
      const auto pCmd( IdxNodeToPCMD( pNd ) );
      if( pCmd->d_func != pFxn ) {
         pPrevKey = pCmd->Name();
         continue;
         }
      DeleteCmd( pCmd );
      }
      ++rv;

      rb_delete_node( s_CmdIdxAddins, pNd );

      // now (since cur node has been deleted), set pNd to prev node
      // locate prev node by searching for pPrevKey
      if( !pPrevKey ) {
         pNd = rb_first( s_CmdIdxAddins );
         }
      else {
         int equal;
         pNd = rb_find_gte_gen( &equal, s_CmdIdxAddins, pPrevKey, rb_strcmpi );
         Assert( equal );
         }
      }

   return rv;
   }

// iterator APIs

PCmdIdxNd CmdIdxFirst()                 { return rb_first( s_CmdIdxBuiltins ); }
PCmdIdxNd CmdIdxLast()                  { return rb_last(  s_CmdIdxBuiltins ); }
PCmdIdxNd CmdIdxAddinFirst()            { return rb_first( s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxAddinLast()             { return rb_last(  s_CmdIdxAddins ); }
PCmdIdxNd CmdIdxNext( PCmdIdxNd pNd )   { return rb_next(  pNd      ); }
PCCMD     CmdIdxToPCMD( PCmdIdxNd pNd ) { return IdxNodeToPCMD( pNd ); }

void WalkAllCMDs( void *pCtxt, CmdVisit visit ) { 0 && DBG( "%s+", __func__ );
   for( auto pNd=CmdIdxFirst() ; pNd != CmdIdxLast() ; pNd = CmdIdxNext( pNd ) )
      visit( CmdIdxToPCMD( pNd ), pCtxt );
   for( auto pNd=CmdIdxAddinFirst() ; pNd != CmdIdxAddinLast() ; pNd = CmdIdxNext( pNd ) )
      visit( CmdIdxToPCMD( pNd ), pCtxt );
   }

#include "my_fio.h"

enum { SEEN = 1<<31 };

STATIC_FXN void sv_idx( PCCMD pCmd, void *pCtxt ) {
   auto ofh( static_cast<FILE *>(pCtxt) );
   if( pCmd->d_callCount != SEEN ) {
      pCmd->d_gCallCount += pCmd->d_callCount;
      }
   pCmd->d_callCount = 0;
   if( pCmd->d_gCallCount > 0 ) {
      fprintf( ofh, "%10u %s\n", pCmd->d_gCallCount, pCmd->d_name );
      }
   }

void cmdusage_updt() {
   enum { SHOWDBG = 0 };
   SHOWDBG && DBG( "%s+", __func__ );
   pathbuf origfnm;
   StateFilename( BSOB(origfnm), "cmdusg" );
   const auto ifh( fopen( origfnm, "rt" ) );
   const auto fOrigFExists( ifh != nullptr );
   if( ifh ) {
      SHOWDBG && DBG( "%s: opened ifh = '%s'", __func__, origfnm );
      auto entries(0);
      for(;;) {
         char buf[300];
         auto p( fgets( BSOB(buf), ifh ) );
         if( !p )
            break;

         //DBG( "%s: '%s'", __func__, p );
         pathbuf  savedCmdnm;
         unsigned savedCount;
         if( (2 == sscanf( p, "%u %s", &savedCount, savedCmdnm )) && *savedCmdnm ) {
            ++entries;
            //DBG( "%s:>%u '%s'", __func__, savedCount, savedCmdnm );
            const PCCMD pCmd( CmdFromName( savedCmdnm ) );
            if( pCmd ) {
               pCmd->d_gCallCount = savedCount + pCmd->d_callCount;
               pCmd->d_callCount = SEEN;
               }
            }
         }
      fclose( ifh );
      SHOWDBG && DBG( "%s: closed ifh = '%s'; read %d entries", __func__, origfnm, entries );
      }

   pathbuf tmpfnm;
   StateFilename( BSOB(tmpfnm), "cmdusg_" );
   //DBG( "%s:%s", __func__, tmpfnm );
   const auto ofh = fopen( tmpfnm, "wt" );
   if( ofh ) {
      SHOWDBG && DBG( "%s: opened ofh = '%s'", __func__, tmpfnm );
      WalkAllCMDs( ofh, sv_idx );
      fclose( ofh );
      SHOWDBG && DBG( "%s: closed ofh = '%s'", __func__, tmpfnm );
      const auto doRename( !fOrigFExists || unlinkOk( origfnm ) );
      if( doRename ) {
         const auto renmOk( rename( tmpfnm, origfnm ) == 0 );
         SHOWDBG && DBG( "%s: did rename, %s", __func__, renmOk ? "ok" : "FAILED" );
         }
      }
   SHOWDBG && DBG( "%s-", __func__ );
   }

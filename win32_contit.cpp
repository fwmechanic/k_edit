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

#include "ed_main.h"

#include "win32_pvt.h"

//
//#############################################################################################################################
//##############################################  Win32 DllMain and handlers  #################################################
//#############################################################################################################################
//

//
// todo: keep a local copy of g_DllLoadCount which an idle handler can compare
//       vs.  g_DllLoadCount to detect a change of state (update status line)
//
#pragma data_seg("Shared")
Win32::LONG g_DllLoadCount = 0;  // VARS MUST BE INITIALIZED, or they won't go into data_seg (instead bss)
#pragma data_seg()
#pragma comment(linker, "/section:Shared,rws")

STATIC_VAR Win32::LONG local_DllLoadCount;

Win32::BOOL WINAPI DllMain( Win32::HINSTANCE hInstDll, Win32::DWORD fdwReason, Win32::LPVOID fImpLoad ) {
   PCChar type;
   switch( fdwReason ) {
    default                : type = "???"               ; break;
    case DLL_THREAD_ATTACH : type = "DLL_THREAD_ATTACH" ; break;
    case DLL_THREAD_DETACH : type = "DLL_THREAD_DETACH" ; break;
    case DLL_PROCESS_ATTACH: type = "DLL_PROCESS_ATTACH"; Win32::InterlockedIncrement( &g_DllLoadCount ); local_DllLoadCount = g_DllLoadCount; break;
    case DLL_PROCESS_DETACH: type = "DLL_PROCESS_DETACH"; Win32::InterlockedDecrement( &g_DllLoadCount ); break;
    }
   DBG( "-----------> %s %s <------------", __func__, type );
   return true;
   }

int EditorLoadCount() {
   return g_DllLoadCount;
   }

bool EditorLoadCountChanged() {
   const auto fChanged( local_DllLoadCount != g_DllLoadCount );
   if( fChanged )
       local_DllLoadCount = g_DllLoadCount;

   return fChanged;
   }

#define TBC_Virtual

class TitleBarContributor {
   protected:
   TitleBarContributor() {}

public:
   TBC_Virtual bool   Changed() ;
   TBC_Virtual PCChar Str()     ;
   };


//------------------------------------------------------------------------------

class EditorFilesStatus : public TitleBarContributor {
   int  d_DirtyFBufs;
   int  d_OpenFBufs;
   char d_buf[40];

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();

   bool    NoneDirty() { return d_DirtyFBufs == 0; }
   };

bool EditorFilesStatus::Changed() {
   auto dirtyFBufs(0);
   auto openFBufs (0);
#if FBUF_TREE
   RbNode *pNd;
   rb_traverse( pNd, g_FBufIdx )
#else
   DLINKC_FIRST_TO_LASTA(g_FBufHead, dlinkAllFBufs, pFBuf)
#endif
      {
#if FBUF_TREE
      PCFBUF pFBuf = IdxNodeToFBUF( pNd );
#endif
      if( pFBuf->HasLines() && pFBuf->FnmIsDiskWritable() ) {
         ++openFBufs;
         if( pFBuf->IsDirty() )
            ++dirtyFBufs;
         }
      }
   const auto rv(  d_DirtyFBufs != dirtyFBufs
                || d_OpenFBufs  != openFBufs
                );
   d_DirtyFBufs = dirtyFBufs;
   d_OpenFBufs  = openFBufs ;

   return rv;
   }

PCChar EditorFilesStatus::Str() {
   return safeSprintf( BSOB(d_buf), "%i/%i", d_DirtyFBufs, d_OpenFBufs );
   }

STATIC_VAR EditorFilesStatus s_edfs;

//------------------------------------------------------------------------------

// notification architecture (vs.  polling cwd every n mS) avoids needless
// Win32 API hammering, but not as clean
//
class CwdStatus : public TitleBarContributor {
   bool  d_fChanged;
   Path::str_t d_buf;

public:

   CwdStatus();
   void SetChanged( PCChar newName );

   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

CwdStatus::CwdStatus()
   : d_fChanged( true )
   , d_buf( Path::GetCwd() )
   {}

void CwdStatus::SetChanged( PCChar newName ) {
   d_fChanged = true;
   d_buf = newName;
   }

bool CwdStatus::Changed() {
   const auto rv( d_fChanged );
   d_fChanged = false;
   return rv;
   }

PCChar CwdStatus::Str() { return d_buf.c_str(); }

STATIC_VAR CwdStatus s_cwds;

void SetCwdChanged( PCChar newName ) { s_cwds.SetChanged( newName ); }

//------------------------------------------------------------------------------

class BatteryStatus : public TitleBarContributor {
   Mutex d_mtx;
   bool  d_fChanged;
   int   d_BatteryLifePercent;
   char  d_buf[40];

public:

   BatteryStatus();

   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();

private:
   STATIC_FXN int GetBatteryLifePercent();
   void BatteryStatusMonitorThread();

   STATIC_FXN Win32::DWORD K_STDCALL StartThread( Win32::LPVOID pThreadParam );
   };

typedef BatteryStatus *PBatteryStatus;

int BatteryStatus::GetBatteryLifePercent() {
   Win32::SYSTEM_POWER_STATUS sps; // the other fields in this struct are either...
   GetSystemPowerStatus( &sps );   // ...invalid (on my laptop) or of no interest
   return sps.BatteryLifePercent;
   }

void BatteryStatus::BatteryStatusMonitorThread() {
   DBG( "*** %s STARTING***", __func__ );
   while( 1 ) {
      const int newVal( GetBatteryLifePercent() );

      0 && DBG( "BatteryLifePercent now=%d%%", newVal );

      {
      AutoMutex am( d_mtx );
      if( d_BatteryLifePercent != newVal ) {
          d_BatteryLifePercent  = newVal;
          d_fChanged = true;
          0 && DBG( "BatteryLifePercent changed, now=%d%%", d_BatteryLifePercent );
          }
      }

      SleepMs( 2500 );
      }
   }

Win32::DWORD BatteryStatus::StartThread( Win32::LPVOID pThreadParam ) {
   PBatteryStatus(pThreadParam)->BatteryStatusMonitorThread();
   return 0; // equivalent to ExitThread( 0 );
   }

BatteryStatus::BatteryStatus() {
   d_buf[0]   = '\0';
   d_fChanged = false;
   const auto blp( GetBatteryLifePercent() );
   d_BatteryLifePercent = blp+100; // make sure first pass thru BatteryStatusMonitorThread() (if any) asserts fChanged

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
   if(  !(blp > 100)  // only start the monitoring thread if there's something _to_ monitor
      && (0 == Win32::CreateThread( nullptr, 1*1024, BatteryStatus::StartThread, this, 0L, nullptr ))
     ) {
#pragma GCC diagnostic pop
      DBG( "%s Win32::CreateThread FAILED!", __func__ );
      }
   }

bool BatteryStatus::Changed() {
   bool fChanged;
   auto newVal(-1); // init to rmv warning
   {
   AutoMutex am( d_mtx );
   fChanged = d_fChanged;
   if( fChanged ) {
      d_fChanged = false;
      newVal = d_BatteryLifePercent;
      }
   }

   if( fChanged )
      safeSprintf( BSOB(d_buf), "    Battery=%d%%", newVal );

   return fChanged;
   }

PCChar BatteryStatus::Str() { return d_buf; }

STATIC_VAR BatteryStatus s_bats;

//------------------------------------------------------------------------------

class MemStatus : public TitleBarContributor {
   typedef char dbuf[27];
   dbuf d_buf;

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

size_t GetProcessMem() {
   Win32::PSAPI::PROCESS_MEMORY_COUNTERS pmc;
   Win32::PSAPI::GetProcessMemoryInfo( g_hCurProc, &pmc, sizeof pmc );
   return pmc.WorkingSetSize;
   }

bool MemStatus::Changed() {
   dbuf dbnew;
   const auto showSize( GetProcessMem() / (1024*(g_fShowMemUseInK ? 1 : 1024)) );
   safeSprintf( BSOB(dbnew), "%s.ProcessMem=%" PR_SIZET "u%ci", ExecutableFormat(), showSize, (g_fShowMemUseInK ? 'K' : 'M') );

   if( 0!=strcmp( d_buf, dbnew ) ) {
      SafeStrcpy( d_buf, dbnew );
      return true;
      }
   return false;
   }

PCChar MemStatus::Str() { return d_buf; }

STATIC_VAR MemStatus s_mems;

//------------------------------------------------------------------------------
class LuaMemStatus : public TitleBarContributor {
   int  d_Size;
   char d_buf[25];

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

bool LuaMemStatus::Changed() {
   const auto newSize( LuaHeapSize() );
   const auto rv( d_Size != newSize );
   d_Size = newSize;
   if( rv ) {
      const auto lheapsz( d_Size / 1024 );
      if( d_Size )
         safeSprintf( BSOB(d_buf), ", LuaHeap=%iKi", lheapsz );
      else
         d_buf[0] = '\0';
      }
   return rv;
   }

PCChar LuaMemStatus::Str() { return d_buf; }

STATIC_VAR LuaMemStatus s_luam;

#if     DISP_LL_STATS
//------------------------------------------------------------------------------
class CursMoves : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

bool CursMoves::Changed() {
   const auto newVal( DispCursorMoves() );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) safeSprintf( BSOB(d_buf), ", CursMv=%d", newVal );
   return rv;
   }

PCChar CursMoves::Str() { return d_buf; }
STATIC_VAR CursMoves s_curw;
//------------------------------------------------------------------------------
class StatLnUpdts : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

bool StatLnUpdts::Changed() {
   const auto newVal( DispStatLnUpdates() );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) safeSprintf( BSOB(d_buf), ", StLnW=%d", newVal );
   return rv;
   }

PCChar StatLnUpdts::Str() { return d_buf; }
STATIC_VAR StatLnUpdts s_stlw;
//------------------------------------------------------------------------------
class ScreenRefreshes : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];

public:
   TBC_Virtual bool Changed();
   TBC_Virtual PCChar Str();
   };

bool ScreenRefreshes::Changed() {
   const auto newVal( g_WriteConsoleOutputLines );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) safeSprintf( BSOB(d_buf), ", ScrLnW=%d:%d", g_WriteConsoleOutputCalls, g_WriteConsoleOutputLines );
   return rv;
   }

PCChar ScreenRefreshes::Str() { return d_buf; }
STATIC_VAR ScreenRefreshes s_scrw;
//------------------------------------------------------------------------------
#endif//DISP_LL_STATS

void UpdateConsoleTitle() {
   if(  s_edfs.Changed()  // using '+' to AVOID ||-short-circuiting
      + s_cwds.Changed()  // (we want ALL changed "stati" to be shown as soon as changed)
      + s_mems.Changed()  // NB: only Changed() methods recalc Str()
      + s_luam.Changed()
#if     DISP_LL_STATS
      + s_curw.Changed()
      + s_stlw.Changed()
      + s_scrw.Changed()
#endif//DISP_LL_STATS
      + s_bats.Changed()
     ) {
      Win32::SetConsoleTitle(
         Sprintf2xBuf
            ( "%s %s %s%s"
#if     DISP_LL_STATS
              "%s%s%s"
#endif//DISP_LL_STATS
              "%s"
            , s_edfs.Str()
            , s_cwds.Str()
            , s_mems.Str()
            , s_luam.Str()
#if     DISP_LL_STATS
            , s_curw.Str()
            , s_stlw.Str()
            , s_scrw.Str()
#endif//DISP_LL_STATS
            , s_bats.Str()
            )
         );
      }
   }

bool EditorFilesystemNoneDirty() { return s_edfs.NoneDirty(); }

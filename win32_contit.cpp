//
// Copyright 2015-2022 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

#define  DBG_THREAD  DBG

Win32::BOOL WINAPI DllMain( Win32::HINSTANCE hInstDll, Win32::DWORD fdwReason, Win32::LPVOID fImpLoad ) {
   PCChar type;
   switch( fdwReason ) {
    break;default                : type = "???"               ;
    break;case DLL_THREAD_ATTACH : type = "DLL_THREAD_ATTACH" ;
    break;case DLL_THREAD_DETACH : type = "DLL_THREAD_DETACH" ;
    break;case DLL_PROCESS_ATTACH: type = "DLL_PROCESS_ATTACH"; Win32::InterlockedIncrement( &g_DllLoadCount ); local_DllLoadCount = g_DllLoadCount;
    break;case DLL_PROCESS_DETACH: type = "DLL_PROCESS_DETACH"; Win32::InterlockedDecrement( &g_DllLoadCount );
    }
   DBG_THREAD( "-----------> %s %s <------------", __func__, type );
   return true;
   }

int EditorLoadCount() {
   return g_DllLoadCount;
   }

bool EditorLoadCountChanged() {
   const auto fChanged( local_DllLoadCount != g_DllLoadCount );
   if( fChanged ) {
      local_DllLoadCount = g_DllLoadCount;
      }
   return fChanged;
   }

#define TBC_Virtual virtual

class TitleBarContributor {
protected:
   TitleBarContributor() {}
public:
   TBC_Virtual bool   Changed() = 0;
   TBC_Virtual PCChar Str()     = 0;
   virtual ~TitleBarContributor() {}
   };
//------------------------------------------------------------------------------
class EdFilesStatus : public TitleBarContributor {
   EditorFilesStatus_t d_efs;
   char d_buf[40];
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
bool EdFilesStatus::Changed() { // BUGBUG this is near-identical to EditorFilesystemNoneDirty()
   const auto now( EditorFilesStatus() );
   if( d_efs != now ) {
      d_efs = now;
      safeSprintf( BSOB(d_buf), "%" PR_SIZET "/%" PR_SIZET "", d_efs.dirtyFBufs, d_efs.openFBufs );
      return true;
      }
   return false;
   }
//------------------------------------------------------------------------------
class CwdStatus : public TitleBarContributor {
   Path::str_t d_last;
public:
   CwdStatus() {}
   TBC_Virtual bool Changed() override {
      Path::str_t now( Path::GetCwd() );
      const auto changed( now != d_last );
      if( changed ) {
         d_last = now;
         }
      return changed;
      }
   TBC_Virtual PCChar Str() override { return d_last.c_str(); }
   };
//------------------------------------------------------------------------------
class BatteryStatus : public TitleBarContributor {
   std::mutex d_mtx;
   bool  d_fChanged;
   int   d_BatteryLifePercent;
   char  d_buf[40];
public:
   BatteryStatus();
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
private:
   STATIC_FXN int GetBatteryLifePercent();
   void BatteryStatusMonitorThread();
   };
typedef BatteryStatus *PBatteryStatus;
int BatteryStatus::GetBatteryLifePercent() {
   Win32::SYSTEM_POWER_STATUS sps; // the other fields in this struct are either...
   GetSystemPowerStatus( &sps );   // ...invalid (on my laptop) or of no interest
   return sps.BatteryLifePercent;
   }
void BatteryStatus::BatteryStatusMonitorThread() {
   0 && DBG_THREAD( "*** %s STARTING***", __func__ );
   while( 1 ) {
      const int newVal( GetBatteryLifePercent() );
      0 && DBG_THREAD( "BatteryLifePercent now=%d%%", newVal );
      {
      std::scoped_lock<std::mutex> am( d_mtx );
      if( d_BatteryLifePercent != newVal ) {
          d_BatteryLifePercent  = newVal;
          d_fChanged = true;
          0 && DBG_THREAD( "BatteryLifePercent changed, now=%d%%", d_BatteryLifePercent );
          }
      }
      SleepMs( 2500 );
      }
   }
BatteryStatus::BatteryStatus() {
   d_buf[0]   = '\0';
   d_fChanged = false;
   const auto blp( GetBatteryLifePercent() );
   d_BatteryLifePercent = blp+100; // make sure first pass thru BatteryStatusMonitorThread() (if any) asserts fChanged
   if( !(blp > 100) ) { // only start the monitoring thread if there's something _to_ monitor
      std::thread bsmt( BatteryStatus::BatteryStatusMonitorThread, this ); bsmt.detach();
      }
   }
bool BatteryStatus::Changed() {
   bool fChanged;
   auto newVal(-1); // init to rmv warning
   {
   std::scoped_lock<std::mutex> am( d_mtx );
   fChanged = d_fChanged;
   if( fChanged ) {
      d_fChanged = false;
      newVal = d_BatteryLifePercent;
      }
   }
   if( fChanged ) {
      safeSprintf( BSOB(d_buf), "--- Battery=%d%%", newVal );
      }
   return fChanged;
   }
//------------------------------------------------------------------------------
class MemStatus : public TitleBarContributor {
   typedef char dbuf[27];
   dbuf d_buf;
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
size_t GetProcessMem() {
   Win32::PSAPI::PROCESS_MEMORY_COUNTERS pmc;
   Win32::PSAPI::GetProcessMemoryInfo( g_hCurProc, &pmc, sizeof pmc );
   return pmc.WorkingSetSize;
   }
bool MemStatus::Changed() {
   dbuf dbnew;
   const auto showSize( GetProcessMem() / (1024*(g_fShowMemUseInK ? 1 : 1024)) );
   safeSprintf( BSOB(dbnew), "%s.ProcessMem=%" PR_SIZET "%ci", ExecutableFormat(), showSize, (g_fShowMemUseInK ? 'K' : 'M') );
   if( 0!=strcmp( d_buf, dbnew ) ) {
      bcpy( d_buf, dbnew );
      return true;
      }
   return false;
   }
//------------------------------------------------------------------------------
class LuaMemStatus : public TitleBarContributor {
   int  d_Size;
   char d_buf[25];
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
bool LuaMemStatus::Changed() {
   const auto newSize( LuaHeapSize() );
   const auto rv( d_Size != newSize );
   d_Size = newSize;
   if( rv ) {
      const auto lheapsz( d_Size / 1024 );
      if( d_Size ) {
         safeSprintf( BSOB(d_buf), "LuaHeap=%iKi", lheapsz );
         }
      else {
         d_buf[0] = '\0';
         }
      }
   return rv;
   }
#if     DISP_LL_STATS
class CursMoves : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
bool CursMoves::Changed() {
   const auto newVal( DispCursorMoves() );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) { safeSprintf( BSOB(d_buf), ", CursMv=%d", newVal ); }
   return rv;
   }
//------------------------------------------------------------------------------
class StatLnUpdts : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
bool StatLnUpdts::Changed() {
   const auto newVal( DispStatLnUpdates() );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) { safeSprintf( BSOB(d_buf), ", StLnW=%d", newVal ); }
   return rv;
   }
//------------------------------------------------------------------------------
class ScreenRefreshes : public TitleBarContributor {
   int  d_prev;
   char d_buf[25];
public:
   TBC_Virtual bool Changed() override;
   TBC_Virtual PCChar Str() override { return d_buf; }
   };
bool ScreenRefreshes::Changed() {
   const auto newVal( g_WriteConsoleOutputLines );
   const auto rv( d_prev != newVal );
   d_prev = newVal;
   if( rv ) { safeSprintf( BSOB(d_buf), ", ScrLnW=%d:%d", g_WriteConsoleOutputCalls, g_WriteConsoleOutputLines ); }
   return rv;
   }
#endif//DISP_LL_STATS

STATIC_VAR std::vector<TitleBarContributor *> s_tbcs;

void UpdateConsoleTitle_Init() {
   s_tbcs.push_back( new EdFilesStatus()   );
   s_tbcs.push_back( new CwdStatus()       );
   s_tbcs.push_back( new MemStatus()       );
   s_tbcs.push_back( new LuaMemStatus()    );
#if     DISP_LL_STATS
   s_tbcs.push_back( new CursMoves()       );
   s_tbcs.push_back( new StatLnUpdts()     );
   s_tbcs.push_back( new ScreenRefreshes() );
#endif//DISP_LL_STATS
   s_tbcs.push_back( new BatteryStatus()   );
   }

void UpdateConsoleTitle() {
   auto changed( 0 );
   for( auto tbc : s_tbcs ) {
      changed += tbc->Changed();
      }
   if( changed ) {
      std::string buf;
      BoolOneShot first;
      for( auto tbc : s_tbcs ) {
         if( !first ) { buf += " "; }
         buf += tbc->Str();
         }
      Win32::SetConsoleTitle( buf.c_str() );
      }
   }

#include "ed_main.h"
#include "conio.h"

#ifndef _WIN32

#include <array>
#include <optional>
#include <string>

namespace ConIn {
namespace {

constexpr std::size_t to_index( BackendId id ) {
   return static_cast<std::size_t>( id );
   }

using BackendSlot = std::optional<BackendOps>;
static std::array<BackendSlot, to_index( BackendId::Count )> s_backends;
static BackendId  s_activeBackend = BackendId::Ncurses;
static bool       s_backendInitialized;

BackendOps *lookup( BackendId id ) {
   auto &entry = s_backends[ to_index( id ) ];
   return entry ? &entry.value() : nullptr;
   }

BackendOps *active_ops_noinit() {
   return lookup( s_activeBackend );
   }

BackendOps *require_active_ops() {
   auto *ops = active_ops_noinit();
   if( !ops ) {
      return nullptr;
      }
   if( !s_backendInitialized ) {
      if( ops->Initialize && !ops->Initialize() ) {
         return nullptr;
         }
      s_backendInitialized = true;
      }
   return ops;
   }

void call_shutdown( BackendOps *ops ) {
   if( ops && ops->Shutdown ) {
      ops->Shutdown();
      }
   }

}

bool RegisterBackend( BackendId id, const BackendOps &ops ) {
   s_backends[ to_index( id ) ] = ops;
   if( id == s_activeBackend ) {
      s_backendInitialized = false;
      }
   return true;
   }

bool BackendRegistered( BackendId id ) {
   return lookup( id ) != nullptr;
   }

bool SelectBackend( BackendId id ) {
   if( !BackendRegistered( id ) ) {
      return false;
      }
   if( id == s_activeBackend ) {
      return true;
      }
   if( s_backendInitialized ) {
      call_shutdown( active_ops_noinit() );
      s_backendInitialized = false;
      }
   s_activeBackend = id;
   return true;
   }

BackendId ActiveBackend() {
   return s_activeBackend;
   }

bool EnsureBackendInitialized() {
   return require_active_ops() != nullptr;
   }

void ShutdownActiveBackend() {
   if( s_backendInitialized ) {
      call_shutdown( active_ops_noinit() );
      s_backendInitialized = false;
      }
   }

void log_verbose() {
   if( auto *ops = require_active_ops(); ops && ops->log_verbose ) {
      ops->log_verbose();
      }
   }

void log_quiet() {
   if( auto *ops = require_active_ops(); ops && ops->log_quiet ) {
      ops->log_quiet();
      }
   }

int DecodeErrCount() {
   if( auto *ops = require_active_ops(); ops && ops->DecodeErrCount ) {
      return ops->DecodeErrCount();
      }
   return 0;
   }

bool FlushKeyQueueAnythingFlushed() {
   if( auto *ops = require_active_ops(); ops && ops->FlushKeyQueueAnythingFlushed ) {
      return ops->FlushKeyQueueAnythingFlushed();
      }
   return false;
   }

void WaitForKey() {
   if( auto *ops = require_active_ops(); ops && ops->WaitForKey ) {
      ops->WaitForKey();
      }
   }

bool KbHit() {
   if( auto *ops = require_active_ops(); ops && ops->KbHit ) {
      return ops->KbHit();
      }
   return false;
   }

EdKC_Ascii EdKC_Ascii_FromNextKey() {
   if( auto *ops = require_active_ops(); ops && ops->EdKC_Ascii_FromNextKey ) {
      return ops->EdKC_Ascii_FromNextKey();
      }
   return {};
   }

EdKC_Ascii EdKC_Ascii_FromNextKey_Keystr( std::string &dest ) {
   if( auto *ops = require_active_ops(); ops && ops->EdKC_Ascii_FromNextKey_Keystr ) {
      return ops->EdKC_Ascii_FromNextKey_Keystr( dest );
      }
   dest.clear();
   return {};
   }

} // namespace ConIn

#endif // !_WIN32

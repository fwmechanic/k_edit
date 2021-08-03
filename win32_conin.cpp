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
#include "win32_pvt.h"

//--------------------------------------------------------------------------------------------

// see for good explanations:
// http://www.adrianxw.dk/SoftwareSite/Consoles/Consoles5.html

STATIC_FXN bool IsInterestingKeyEvent( const Win32::KEY_EVENT_RECORD &KER );
struct conin_statics {
   enum { CIB_DFLT_ELEMENTS = 32, CIB_MIN_ELEMENTS = 64 };
   Win32::HANDLE         hStdin;
   Win32::DWORD          InitialConsoleInputMode;
   Win32::DWORD          CIB_ValidElements;
   Win32::DWORD          CIB_IdxRead;
   Mutex                 mutex;
   std::vector<Win32::INPUT_RECORD> CIB;
   conin_statics() : mutex() {};
   void ClearBuf() { CIB_IdxRead = CIB_ValidElements = 0; }
   bool ScanConinBufForKeyDowns() {
      for( auto ix(CIB_IdxRead); ix < CIB_ValidElements; ++ix ) {
         const auto pIR( &CIB[ix] );
         0 && DBG( "ConinEv=%d", pIR->EventType );
         if( (KEY_EVENT == pIR->EventType) && IsInterestingKeyEvent( pIR->Event.KeyEvent ) ) {
            return true;
            }
         }
      return false;
      }
private:
   NO_COPYCTOR(conin_statics);
   NO_ASGN_OPR(conin_statics);
   };

STATIC_VAR conin_statics s_Conin;

bool ConIn::FlushKeyQueueAnythingFlushed() {
   AutoMutex mtx( s_Conin.mutex );
   auto rv( s_Conin.ScanConinBufForKeyDowns() );
   s_Conin.ClearBuf();
   Win32::DWORD NumberOfConsoleEventsPending;
   if(  0 == Win32::GetNumberOfConsoleInputEvents( s_Conin.hStdin, &NumberOfConsoleEventsPending )
     || 0 == NumberOfConsoleEventsPending
     ) {
      return rv;
      }
   0 && DBG( "NumberOfConsoleEventsPending = %ld", NumberOfConsoleEventsPending );
   const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin, &s_Conin.CIB[0], s_Conin.CIB.size(), &s_Conin.CIB_ValidElements ) );
   0 && DBG( "s_Conin.CIB_ValidElements = %ld", s_Conin.CIB_ValidElements );
   if( !ok ) {
      linebuf oseb;
      DBG( "Win32::ReadConsoleInputA failed: %s", OsErrStr( BSOB(oseb) ) );
      Assert( ok );
      }
   rv = rv || s_Conin.ScanConinBufForKeyDowns();
   s_Conin.ClearBuf();
   return rv;
   }

STATIC_FXN void ApiFlushConsoleInputBuffer() {
   s_Conin.CIB_ValidElements = 0;
   Win32::FlushConsoleInputBuffer( s_Conin.hStdin );
   }

STATIC_FXN Win32::DWORD GetConsoleInputMode() {
   Win32::DWORD dwMode;
   if( !Win32::GetConsoleMode( s_Conin.hStdin, &dwMode ) ) {
      // arg "GetConsoleMode fails when input piped programmer.win32" google
      //
      // from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/console_handles.asp
      //
      // Initially, STDIN is a handle to the console's input buffer, and STDOUT
      // and STDERR are handles of the console's active screen buffer.  However,
      // the SetStdHandle function can redirect the standard handles by changing
      // the handle associated with STDIN, STDOUT, or STDERR.  Because the
      // parent's standard handles are inherited by any child process,
      // subsequent calls to GetStdHandle return the redirected handle.  A
      // handle returned by GetStdHandle may, therefore, refer to something
      // other than console I/O.  For example, before creating a child process,
      // a parent process can use SetStdHandle to set a pipe handle to be the
      // STDIN handle that is inherited by the child process.  When the child
      // process calls GetStdHandle, it gets the pipe handle.  This means that
      // the parent process can control the standard handles of the child
      // process.  The handles returned by GetStdHandle have GENERIC_READ |
      // GENERIC_WRITE access unless SetStdHandle has been used to set the
      // standard handle to have lesser access.
      //
      // The value of the handles returned by GetStdHandle are not 0, 1, and 2,
      // so the standard predefined stream constants in Stdio.h (STDIN, STDOUT,
      // and STDERR) cannot be used in functions that require a console handle.
      //
      // The CreateFile function enables a process to get a handle to its
      // console's input buffer and active screen buffer, even if STDIN and
      // STDOUT have been redirected.  To open a handle to a console's input
      // buffer, specify the CONIN$ value in a call to CreateFile.  Specify the
      // CONOUT$ value in a call to CreateFile to open a handle to a console's
      // active screen buffer.  CreateFile enables you to specify the read/write
      // access of the handle that it returns.
      //
      // *** Total-piece-of-shit-Accurev GUI starts up %EDITOR% with stdin and
      // *** stdout attached to pipes; FUCKING DUMBASSES!
      //
      // 20060118 klg
      //
      Win32::CloseHandle( s_Conin.hStdin ); // close their fucking pipe
      s_Conin.hStdin = Win32::CreateFile( "CONIN$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
// 20080304 kgoodwin added to debug problems with latest Accurev (POS!!!) release
// problem is the editor opens but is not visible; probably a ConsoleOutputMode issue.
#if 0
      if( s_Conin.hStdin == Win32::Invalid_Handle_Value() ) {
         linebuf oseb;
         VidInitApiError( FmtStr<120>( "Win32::GetStdHandle( STD_INPUT_HANDLE ) FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
         exit( 1 );
         }
#endif
      if( !Win32::GetConsoleMode( s_Conin.hStdin, &dwMode ) ) {
         linebuf oseb;
         VidInitApiError( FmtStr<120>( "Win32::GetConsoleMode on new CONIN$ FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
         exit( 1 );
         }
      if( !Win32::SetStdHandle( Win32::Std_Input_Handle(), s_Conin.hStdin ) ) {
         linebuf oseb;
         VidInitApiError( FmtStr<120>( "Win32::SetStdHandle on new CONIN$ FAILED: %s", OsErrStr( BSOB(oseb) ) ) );
         exit( 1 );
         }
      DBG( "%s ********************** REALLOCATED CONIN$ **********************", __func__ );
      }
   DBG( "GetConsoleMode(IN) => 0x%lX *****************************", dwMode );
   return dwMode;
   }

#define  SetConsoleInputMode( dwMode ) SetConsoleInputMode_( dwMode, __func__ )
STATIC_FXN void SetConsoleInputMode_( Win32::DWORD dwMode, PCChar caller ) {
   if( 0 == Win32::SetConsoleMode( s_Conin.hStdin, dwMode ) ) {
      linebuf oseb;
      DBG( "%s: %s FAILED <- %lX (is %lX) %s *****************************", caller, __func__, dwMode, GetConsoleInputMode(), OsErrStr( BSOB(oseb) ) );
      }
   else {
      DBG( "%s: %s Ok <- %lX *****************************", caller, __func__, dwMode );
      }
   }

//--------------------------------------------------------------------------------------------

ConsoleInputModeRestorer::ConsoleInputModeRestorer() : d_cim(GetConsoleInputMode()) {}
void ConsoleInputModeRestorer::Set() const { SetConsoleInputMode( d_cim ); }

void Conin_Init() {
   s_Conin.hStdin = Win32::CreateFile( "CONIN$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
   DBG( "INITIAL s_Conin.hStdin=%p", s_Conin.hStdin );
   s_Conin.CIB_ValidElements = 0;
   s_Conin.CIB_IdxRead       = 0;
   s_Conin.CIB.resize( conin_statics::CIB_DFLT_ELEMENTS );
   s_Conin.InitialConsoleInputMode = GetConsoleInputMode();
   ConsoleInputAcquire();
   }

GLOBAL_VAR volatile bool g_fSystemShutdownOrLogoffRequested;

STATIC_FXN Win32::BOOL K_STDCALL CtrlBreakHandler( Win32::DWORD dwCtrlType ) {
   // NB: This function is called from/on a BRAND NEW THREAD created by the OS
   //     SOLELY for the purpose of running this function!
   //
   auto type("");
   switch( dwCtrlType ) {
      default                 : type = "???"                 ;    break;
      case CTRL_C_EVENT       : type = "CTRL_C_EVENT"        ;    break;
      case CTRL_BREAK_EVENT   : type = "CTRL_BREAK_EVENT"    ; SetUserInterrupt();                        break;
      case CTRL_CLOSE_EVENT   : type = "CTRL_CLOSE_EVENT"    ; HANDLE_CTRL_CLOSE_EVENT( g_fProcessExitRequested = true; ) break;
      case CTRL_LOGOFF_EVENT  : type = "CTRL_LOGOFF_EVENT"   ; g_fSystemShutdownOrLogoffRequested = true; break;
      case CTRL_SHUTDOWN_EVENT: type = "CTRL_SHUTDOWN_EVENT" ; g_fSystemShutdownOrLogoffRequested = true; break;
      }
   DBG( "!!!!!!!! %s %s (cim=%lX) !!!!!!!!", __func__, type, GetConsoleInputMode() );
   return 1;
   }

void ConsoleInputAcquire() {
   // SetConsoleInputMode( s_Conin.InitialConsoleInputMode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) | ENABLE_MOUSE_INPUT );
   // Win32::SetConsoleCtrlHandler( 0, 0 );  // contrary to documentation, this seems to have any effect on Ctrl+C reception
   // Win32::SetConsoleCtrlHandler( 0, 1 );  // contrary to documentation, this seems to have any effect on Ctrl+C reception
   Win32::SetConsoleCtrlHandler( CtrlBreakHandler, 1 );
   ApiFlushConsoleInputBuffer();
   SetConsoleInputMode( CIM_USE_MOUSE ); // absence of ENABLE_PROCESSED_INPUT BLOCKS CTRL+C from invoking CtrlBreakHandler
   }

void ConinRelease() {
   // Strange but true: Win32 Console QuickEdit (and Insert) modes set by a
   // child process are INHERITED by the parent console when the editor exits!
   //
   // This probably only affects a certain case, where the child process
   // (editor) has taken over the parent process (shell) 's console, but we will
   // program for this case, since in any other case the settings are discarded
   // by the OS.
   //
   // Since both modes are beneficial in a shell environment (and S/B the
   // default: yes, _I_ know best!), I FORCE both to be enabled on editor exit.
   //
   // 20070129 kgoodwin
   //
   SetConsoleInputMode( s_Conin.InitialConsoleInputMode | ENABLE_QUICK_EDIT_INS_MODES );
   }

//#############################################################################################################################
//#############################################################################################################################
//
//   BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING
//   BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING
//   BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING
//   BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING  BEGIN KEY_DECODING
//

STATIC_FXN Win32::PINPUT_RECORD ReadNextUsefulConsoleInputRecord() {
   AutoMutex mtx( s_Conin.mutex ); //#########################################################
   STATIC_VAR bool fWaitingOnInput;
   if( fWaitingOnInput ) {
      return nullptr;
      }
   if( 0 == s_Conin.CIB_ValidElements ) {
      if( s_Conin.CIB.size() > conin_statics::CIB_MIN_ELEMENTS ) {
         0 && DBG( "s_Conin.CIB.size() was %" PR_SIZET ", now %d", s_Conin.CIB.size(), conin_statics::CIB_MIN_ELEMENTS );
         auto dummy(false);  GotHereDialog( &dummy );  // it's doubtful this is ever executed?
         s_Conin.CIB.resize( conin_statics::CIB_MIN_ELEMENTS );
         }
      fWaitingOnInput = true;
      mtx.unlock(); //#########################################################

      // Win32::ReadConsoleInput does not return until at least one input record has been read.

      // BUGBUG use Win32::WaitForMultipleObjects to wait on s_Conin.hStdin AND a message Q receiving messages from other threads
      //
      // this seems unlikely to work since "a message Q" seems to require a
      // mutex sheltering a semaphore, and you can't wait on the inner semaphore
      // w/o first owning the mutex, and you can't wait on the mutex, since it
      // being signaled signifies nothing.
      //
      // 20100106 kgoodwin see class MsgQ below for an apparent solution: this
      // offers an MQ that can be waited on (but ATM not with other handles like
      // conin, although this could be "hacked onto the end").  And if there is a
      // conin monitoring thread which tx's into this MQ, there is no need to
      // simultaneously wait on the MQ and conin handles explicitly.
      //
      // The next idea is to create a new thread whose job is to write/copy
      // console events into the message Q (while other threads write other
      // events into the same message Q.
      //
      // The WaitForMultipleObjects function can specify handles of any of the following object types in the lpHandles array:
      //
      // interesting
      //    * Console input
      //    * Event
      //    * Mutex
      //    * Semaphore
      //
      // UNinteresting
      //    * Change notification
      //    * Memory resource notification
      //    * Process
      //    * Thread
      //    * Waitable timer
      //
      // Worker-Thread Events
      //  * mfgrep, parallel shells
      //    - display update requests
      //    - completion: xfr ownership of output FBUF back to main thread
      //  * view syntax parsing (if I dare, someday)?
      //
      // NB: s_Conin.hStdin != GetStdHandle.Stdin  !!!!!!
      0 && DBG( "%s s_Conin.hStdin=%p, GetStdHandle.Stdin=%p", __func__, s_Conin.hStdin, Win32::GetStdHandle( Win32::Std_Input_Handle() ) );
      const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin, &s_Conin.CIB[0], s_Conin.CIB.size(), &s_Conin.CIB_ValidElements ) );
      if( !ok ) {
         1 && DBG( "%s s_Conin.hStdin=%p, GetStdHandle.Stdin=%p", __func__, s_Conin.hStdin, Win32::GetStdHandle( Win32::Std_Input_Handle() ) );
         linebuf oseb;
         DBG( "Win32::ReadConsoleInputA failed: %s", OsErrStr( BSOB(oseb) ) );
         Assert( ok );
         }
      mtx.lock(); //#########################################################
      fWaitingOnInput = false;
      s_Conin.CIB_IdxRead = 0;
      }
   const auto rv ( &s_Conin.CIB[ s_Conin.CIB_IdxRead ] );
   --s_Conin.CIB_ValidElements;
   ++s_Conin.CIB_IdxRead;
   return rv;
   }

//------------------------------------------------------------------------------
// http://my.fit.edu/~cotero/techreport/Multi-Reader.pdf
//------------------------------------------------------------------------------

class Semaphore {
   const Win32::DWORD  d_maxCount;
         Win32::HANDLE d_hSemaphore;
public:
   Semaphore( int initialCount, int maxCount )
      : d_maxCount  ( maxCount )
      , d_hSemaphore( Win32::CreateSemaphore(nullptr, initialCount, d_maxCount, nullptr) )
      {}
   ~Semaphore() { Win32::CloseHandle( d_hSemaphore ); }
   bool   take( int timeout )      { return ( Win32::Wait_Object_0() == Win32::WaitForSingleObject(d_hSemaphore, timeout) ) ? true : false; }
   bool   release()                { return ( 0 != Win32::ReleaseSemaphore( d_hSemaphore, 1, nullptr ) ) ? true : false; }
   Win32::HANDLE getHandle() const { return d_hSemaphore; }
   };

//------------------------------------------------------------------------------

template <class MsgType>
class MsgQ
   {
public:
   enum { MSG_Q_WAIT_FOREVER = INFINITE };
private:
   const Win32::DWORD  d_maxMsg        ;
         Win32::DWORD  d_msgCount      ;
   Semaphore           d_binarySem     ;
   Semaphore           d_countingSem   ;
   DLinkHead<MsgType>  d_QHead         ;  // mega-hack: MQ_dlink is a necessary/assumed hard-coded field of every MsgType class
   Win32::HANDLE       d_semHandles[2] ;
public:
   MsgQ<MsgType>( Win32::DWORD maxMsg )
      : d_maxMsg(maxMsg)
      , d_msgCount(0)
      , d_binarySem(1,1)
      , d_countingSem(0, d_maxMsg)
      {
      d_semHandles[0] = d_binarySem  .getHandle();
      d_semHandles[1] = d_countingSem.getHandle();
      }
   MsgType *Tx( MsgType *pMsg, int waitTime=MSG_Q_WAIT_FOREVER, bool fHighPriority=false ) {
      if( d_binarySem.take(waitTime) ) {
         if( d_msgCount >= d_maxMsg ) {
            DBG( "%s Tx on Full %d-el Q!", __func__, d_maxMsg );
            }
         else {
            Q_Tx( pMsg, fHighPriority );
            ++d_msgCount;
            d_countingSem.release();
            pMsg = nullptr;
            }
         d_binarySem.release();
         }
      return pMsg;
      }
   MsgType *Rx( int timeout=MSG_Q_WAIT_FOREVER ) {                                                                   //   v--- wait for ALL to be signaled
      if( Win32::WaitForMultipleObjects( ELEMENTS(d_semHandles), d_semHandles, TRUE, timeout ) == Win32::Wait_Object_0() ) {
         auto rv( Q_Rx() );
         --d_msgCount;
         d_binarySem.release();
         return rv;
         }
      return nullptr;
      }
private:
   void Q_Tx( MsgType *pMsg, bool fHighPriority ) {
      if( fHighPriority ) { DLINK_INSERT_FIRST( d_QHead, pMsg, MQ_dlink ); }
      else                { DLINK_INSERT_LAST(  d_QHead, pMsg, MQ_dlink ); }
      }
   MsgType *Q_Rx() {
      auto pMsg( d_QHead.front() );
      DLINK_REMOVE_FIRST( d_QHead, pMsg, MQ_dlink );
      Assert( pMsg );
      return pMsg;
      }
   };

//------------------------------------------------------------------------------
//
// MsgQ test case
//
#define TEST_MQ 0
#if     TEST_MQ

struct MQ_input {
   char   d_ch;
   int    d_count;
public:
   MQ_input(char ch, int count) : d_ch(ch), d_count(count) {}
   DLinkEntry<MQ_input> MQ_dlink;
   };

enum { MQ_TEST_MSGS_PER_THREAD = 500, MQ_TEST_THREADS = 6,  };

MsgQ<MQ_input> mqInput( MQ_TEST_THREADS * MQ_TEST_MSGS_PER_THREAD );

MQ_input *wrap_mqi_rx( int timeout=MsgQ<MQ_input>::MSG_Q_WAIT_FOREVER ) {
   return mqInput.Rx(timeout);
   }

void wrap_mqi_tx( MQ_input *pMsg ) {
   auto rv( mqInput.Tx( pMsg ) );
   Assert( rv==0 );
   }

   Win32::DWORD MQ_Test_Thread( Win32::LPVOID pThreadParam ) {
      const auto ch( char(pThreadParam) );
      printf( "\nTHREAD %c+\n", ch );
      for( auto ix(0); ix<MQ_TEST_MSGS_PER_THREAD; ++ix ) {
         wrap_mqi_tx( new MQ_input(ch,ix) );
         }
      printf( "\nTHREAD %c-\n", ch );
      return 0; // equivalent to ExitThread( 0 );
      }

   int MQ_Test_StartThreads() {
      char ch('a');
      for( auto ix(0); ix<MQ_TEST_THREADS; ++ix, ++ch ) {
         if( (0 == Win32::CreateThread( nullptr, 1*1024, MQ_Test_Thread, Win32::LPVOID(ch), 0, nullptr ))
           ) {
            DBG( "%s Win32::CreateThread FAILED!", __func__ );
            }
         }
      return MQ_TEST_THREADS * MQ_TEST_MSGS_PER_THREAD;
      }

void TestMQ() {
   auto rx( wrap_mqi_rx(0) );  Assert( !rx );  // verify empty state
   const auto msgs( MQ_Test_StartThreads() );
   printf( "looking for %d msgs\n", msgs );
   for( auto ix(0) ; ix < msgs ; ++ix ) {
      rx = wrap_mqi_rx();
      Assert( rx );
      printf( "%c:%d,", rx->d_ch, rx->d_count );
      Free0( rx );
      }
   rx = wrap_mqi_rx(0);  Assert( !rx );  // verify empty state
   printf( "rx'd %d msgs\n", msgs );
   DBG( "############ %s PASSED! ############", __func__ );
   fflush( stdout );
   }

#else

void TestMQ() {}

#endif

//------------------------------------------------------------------------------

struct RawWinKeydown {
   uint16_t  unicodeChar      ;
   uint16_t  wVirtualKeyCode  ;
   int       dwControlKeyState;
   };

struct EdInputEvent {
   enum { evt_KEY, evt_MOUSE, evt_FOCUS }; // evt
   bool           fIsEdKC_EVENT;
   union {
      RawWinKeydown rawkey;
      EdKC          EdKC_EVENT;
      };
   };

#if MOUSE_SUPPORT

// insert ConinRecord in next-to-be-read position; this is used for mouse
// processing, in the case of the most recently read ConinRecord _was_ the mouse record

STATIC_FXN void InsertConinRecord( const Win32::INPUT_RECORD &ir ) {
   AutoMutex mtx( s_Conin.mutex );
   if( s_Conin.CIB_IdxRead == 0 ) { // trying to insert w/no leading gap?
      s_Conin.CIB.insert( s_Conin.CIB.begin(), ir );
      s_Conin.CIB_IdxRead++;
      }
   else {
      s_Conin.CIB[--s_Conin.CIB_IdxRead] = ir;
      }
   ++s_Conin.CIB_ValidElements;
   }

// used by the Mouse-event handler to simulate keystrokes

STATIC_FXN void InsertKeyUpDownInputEventRecord( const RawWinKeydown &rawWinKeydn ) {
   Win32::INPUT_RECORD inrec;
   inrec.EventType                        = KEY_EVENT;
   inrec.Event.KeyEvent.wVirtualKeyCode   = rawWinKeydn.wVirtualKeyCode;
   inrec.Event.KeyEvent.uChar.UnicodeChar = rawWinKeydn.unicodeChar;
   inrec.Event.KeyEvent.dwControlKeyState = rawWinKeydn.dwControlKeyState;
   inrec.Event.KeyEvent.wRepeatCount      = 0;
   inrec.Event.KeyEvent.wVirtualScanCode  = 0;
   inrec.Event.KeyEvent.bKeyDown          = 0;
   InsertConinRecord( inrec );
   inrec.Event.KeyEvent.bKeyDown          = 1;
   InsertConinRecord( inrec );
   }

STATIC_FXN void InsertKeyUpDownInputEventRecord( uint16_t wVirtualKeyCode ) {
   RawWinKeydown rwkd;
   rwkd.unicodeChar       = 0;
   rwkd.wVirtualKeyCode   = wVirtualKeyCode;
   rwkd.dwControlKeyState = 0;
   InsertKeyUpDownInputEventRecord( rwkd );
   }

enum { // editor-generic flags
   edMOUSE_LEFT_BUTTON    = FROM_LEFT_1ST_BUTTON_PRESSED,
   edMOUSE_RIGHT_BUTTON   = RIGHTMOST_BUTTON_PRESSED    ,
   edMOUSE_DOUBLE_CLICKED = BIT(8)                      ,
   };

template <class T>
class AutoClr {
   T &d_t;
public:
   AutoClr( T &t, T v ) : d_t(t) { d_t = v; }
   ~AutoClr() { d_t = 0; }
   };

// key that invokes a given CMD; BUGBUG these should adapt to current key assignment!

#define  EDCMDVK_ARG   VK_CLEAR
#define  EDCMDVK_DOWN  VK_DOWN
#define  EDCMDVK_HELP  VK_F1

class OneEnterer {
   volatile Win32::LONG *d_pCount;
   const    Win32::LONG  d_myIncdVal;
public:
   OneEnterer( volatile Win32::LONG *pCount ) : d_pCount( pCount ), d_myIncdVal( Win32::InterlockedIncrement( d_pCount ) ) {}
   ~OneEnterer() { Win32::InterlockedDecrement( d_pCount ); }
   Win32::LONG Value() const { return d_myIncdVal; }
   };

class TMouseEvent {
   // 20160929 (I now realize that) some weeks/months ago (specifically, after the monster "Windows 10 Anniv Update"), mouse-wheel scrolling stopped working in Windows 10.
   //   the related code (here) has not changed in YEARS, so I suspect MS has broken this functionality...
   //   and I'm not the only one to notice...
   //   http://stackoverflow.com/questions/38887457/how-to-capture-mouse-wheeled-event
   //   https://github.com/Maximus5/ConEmu/issues/216
   //   and from https://forum.farmanager.com/viewtopic.php?t=10401
   //   -----
   //   I can confirm that mouse wheel stopped working in Windows 10 Anniv Update
   //   -----
   //   > BTW "[x] Use legacy console" in console properties resolves the issue.
   //   Yes it does
   //   -----
   //   "Console host doesn't send mouse wheel events to applications anymore.
   //   Instead wheel simply scrolls the screen bufffer (if it's bigger than the
   //   window area) or does nothing at all (if it's the same).[YYY]
   //
   //   "We CAN NOT fix this on our side."
   //
   // [YYY] this is exactly the behavior I observe.
   //
   Win32::MOUSE_EVENT_RECORD d_MouseEv;
   Point                     d_mousePosition;
   bool LeftButtonDown()  const { return ToBOOL( d_MouseEv.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED ); }
   bool RightButtonDown() const { return ToBOOL( d_MouseEv.dwButtonState & RIGHTMOST_BUTTON_PRESSED     ); }
   bool InterestingFlags() const {
      return LeftButtonDown() || RightButtonDown() || ToBOOL(d_MouseEv.dwEventFlags & (MOUSE_WHEELED|DOUBLE_CLICK));
      }
   int WheelSpin() const {
      int wheelspin;
      return ((d_MouseEv.dwEventFlags & MOUSE_WHEELED) && 0 != (wheelspin=static_cast<int16_t>( (d_MouseEv.dwButtonState >> 16) & 0xFFFF)))
             ? wheelspin
             : 0
             ;
      }
   void ReinsertMouseEvent( Point mousePosition ) const;
public:
   TMouseEvent( const Win32::MOUSE_EVENT_RECORD &MouseEv );
   void  Process();
   };

// MOUSE_EVENT_RECORD  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/mouse_event_record_str.asp

TMouseEvent::TMouseEvent( const Win32::MOUSE_EVENT_RECORD &MouseEv ) {
   d_MouseEv = MouseEv;
   d_mousePosition.lin = 1+MouseEv.dwMousePosition.Y;
   d_mousePosition.col = 1+MouseEv.dwMousePosition.X;
   }

void TMouseEvent::ReinsertMouseEvent( Point mousePosition ) const {
   Win32::INPUT_RECORD inrec;
   inrec.EventType = MOUSE_EVENT;
   inrec.Event.MouseEvent = d_MouseEv;
   inrec.Event.MouseEvent.dwMousePosition.Y = mousePosition.lin - 1;
   inrec.Event.MouseEvent.dwMousePosition.X = mousePosition.col - 1;
   inrec.Event.MouseEvent.dwControlKeyState = 0;
   InsertConinRecord( inrec );
   }

void TMouseEvent::Process() { // usemouse:yes
   STATIC_VAR volatile Win32::LONG s_noReenter;
   OneEnterer only1( &s_noReenter );
   if( 1 != only1.Value() ) {
      DBG( "%s  ***************************  reentered!", __func__ );
      return;
      }
   if( !InterestingFlags() ) {
      return;
      }
   0 && DBG( "%s (Y=%3d,X=%3d)", __func__, d_mousePosition.lin, d_mousePosition.col );
   {
   STATIC_VAR bool  s_fDragging_QQQ;
   STATIC_VAR bool  s_fInserting_EDCMDVK_ARG;
   STATIC_VAR Point s_prevMousePosition;
   if( LeftButtonDown() ) {
      0 && DBG( "Seln=%d drag=%d inserting=%d mouseMoved=%d", IsSelectionActive(), s_fDragging_QQQ, s_fInserting_EDCMDVK_ARG, d_mousePosition != s_prevMousePosition );
      if( !IsSelectionActive() && s_fDragging_QQQ && !s_fInserting_EDCMDVK_ARG && d_mousePosition != s_prevMousePosition ) {
         0 && DBG( "***** s_fInserting_EDCMDVK_ARG" );
         ReinsertMouseEvent( d_mousePosition );
         InsertKeyUpDownInputEventRecord( EDCMDVK_ARG );
         ReinsertMouseEvent( s_prevMousePosition );
         s_fInserting_EDCMDVK_ARG = true;
         return; //*********************************************************
         }
      s_prevMousePosition      = d_mousePosition;
      s_fDragging_QQQ          = true;
      }
   else {
      s_fDragging_QQQ          = false;
      s_fInserting_EDCMDVK_ARG = false;
      }
   }
   WhileHoldingGlobalVariableLock gvlock;
   if( g_WindowCount() > 1 && !SwitchToWinContainingPointOk( Point( d_mousePosition.lin-1, d_mousePosition.col-1 ) ) ) {
      return;
      }
   // was: MouseMovedCmdExec( d_mousePosition.lin - g_CurWin()->UpLeft.lin, d_mousePosition.col - g_CurWin()->UpLeft.col, d_flags );
   // now inlined
   PCWrV;
   const auto yLine( d_mousePosition.lin - pcw->d_UpLeft.lin );
   const auto xCol ( d_mousePosition.col - pcw->d_UpLeft.col );
   int wheelspin;
#ifdef SOME_STUFF
   STATIC_VAR bool s_fInserting_EDCMDVK_HELP;
   if( LeftButtonDown() ) {
      if( IsSelectionActive() && RightButtonDown() ) {
         pCMD_boxstream->BuildExecute();
         }
      const auto yMax( pcw->UpLeft.lin + pcw->Size.lin );
      if( yMax - yLine == -1 ) {
         ReinsertMouseEvent( Point( pcw->UpLeft.lin + yLine - 1, pcw->UpLeft.col + xCol ) );
         InsertKeyUpDownInputEventRecord( EDCMDVK_DOWN );
         }
      else if( yLine-1 < yMax ) {
         pcv->MoveCursor( pcv->Origin().lin + yLine - 1, pcv->Origin().col + xCol - 1 );
         if( IsSelectionActive() ) {
            ExtendSelectionHilite( pcv->Cursor() );
            }
         }
      }
   else if( RightButtonDown() ) {
      if( !IsSelectionActive() && !s_fInserting_EDCMDVK_HELP ) {
         s_fInserting_EDCMDVK_HELP = true;
         if( pcw->UpLeft.lin + pcw->Size.lin > yLine-1 ) {
            pcv->MoveCursor( pcv->Origin().lin + yLine - 1, pcv->Origin().col + xCol - 1 );
            InsertKeyUpDownInputEventRecord( EDCMDVK_HELP );
            }
         }
      }
   else
#endif
        if( (wheelspin = WheelSpin()) )  // WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL  WHEEL
      {
      const UI abs_spin( Abs( wheelspin ) );
    #if 0
      STATIC_VAR UI minPosSpin = UINT_MAX, maxPosSpin = 0;
      STATIC_VAR UI minNegSpin = UINT_MAX, maxNegSpin = 0;
      if( Sign(wheelspin) > 0 ) {
         minPosSpin = std::min( minPosSpin, abs_spin );
         maxPosSpin = std::max( maxPosSpin, abs_spin );
         }
      else {
         minNegSpin = std::min( minNegSpin, abs_spin );
         maxNegSpin = std::max( maxNegSpin, abs_spin );
         }
      DBG( "+spin-range: (%5d,%5d)  -spin-range: (%5d,%5d)", minPosSpin, maxPosSpin, minNegSpin, maxNegSpin );
    #endif
      const auto mag( abs_spin / ((3*WHEEL_DELTA)/2) );
      const int  speedTbl[] = { 1, 3, 7, 15, 25 };
      const auto stMag( ( mag < ELEMENTS(speedTbl) ) ? speedTbl[ mag ] : speedTbl[ ELEMENTS(speedTbl) - 1 ] );
      pcv->ScrollOrigin_Y_Rel( stMag * -Sign(wheelspin) );
      }
   else {
#ifdef SOME_STUFF
      s_fInserting_EDCMDVK_HELP = false;
#endif
      }
   }
#endif

bool KbHit() { // BUGBUG does this actually WORK? 20081215 kgoodwin NO it doesn't because s_Conin is not being filled unless there were leftovers from the last ReadConsoleInput call
   AutoMutex mtx( s_Conin.mutex );
   if( s_Conin.CIB_IdxRead < s_Conin.CIB_ValidElements ) {
      auto &inrec( s_Conin.CIB[ s_Conin.CIB_IdxRead ] );
      if( inrec.EventType == KEY_EVENT && inrec.Event.KeyEvent.bKeyDown ) {
         return true;
         }
      }
   return false;
   }

STATIC_FXN int XlatKeysWhenNumlockOn( uint16_t theVK ) {
   switch( theVK )
      { // index into numLockXlatTbl     unshifted maps to
      case VK_NUMPAD0  : return  0;  // EdKC_num0
      case VK_NUMPAD1  : return  1;  // EdKC_num1
      case VK_NUMPAD2  : return  2;  // EdKC_num2
      case VK_NUMPAD3  : return  3;  // EdKC_num3
      case VK_NUMPAD4  : return  4;  // EdKC_num4
      case VK_NUMPAD5  : return  5;  // EdKC_num5
      case VK_NUMPAD6  : return  6;  // EdKC_num6
      case VK_NUMPAD7  : return  7;  // EdKC_num7
      case VK_NUMPAD8  : return  8;  // EdKC_num8
      case VK_NUMPAD9  : return  9;  // EdKC_num9
      case VK_SUBTRACT : return 10;  // EdKC_numMinus
      case VK_ADD      : return 11;  // EdKC_numPlus
      case VK_MULTIPLY : return 12;  // EdKC_numStar
      case VK_DIVIDE   : return 13;  // EdKC_numSlash
      case VK_SEPARATOR: return 14;  // EdKC_numEnter
      default:           return -1;
      }
   }


STATIC_FXN bool IsInterestingKeyEvent( const Win32::KEY_EVENT_RECORD &KER ) {
   if( !KER.bKeyDown ) { // auto-repeat gen's a series of bKeyDown's so these are NOT skipped
      return false;
      }
   switch( KER.wVirtualKeyCode ) {
      case VK_MENU:    return KER.uChar.UnicodeChar != 0;
      case VK_NUMLOCK:
      case VK_CAPITAL:
      case VK_SHIFT:
      case VK_CONTROL: return false;
      default:         return true;
      }
   }

STATIC_FXN EdInputEvent ReadInputEventPrimitive() {
   while( true ) {
      if( EXPERIMENT_HANDLE_CTRL_CLOSE_EVENT HANDLE_CTRL_CLOSE_EVENT( && g_fProcessExitRequested ) ) {
          HANDLE_CTRL_CLOSE_EVENT( g_fProcessExitRequested = false; )
          EdInputEvent rv;
          rv.fIsEdKC_EVENT = true;
          rv.   EdKC_EVENT = EdKC_EVENT_ProgramExitRequested;
          return rv;
          }
      auto pIR( ReadNextUsefulConsoleInputRecord() );
      if( pIR ) { // pIR is within s_Conin.CIB[]?
         switch( pIR->EventType ) {
            default:  DBG( "*** InputEvent %d ??? ***", pIR->EventType );  break;
            case KEY_EVENT:
                 if( IsInterestingKeyEvent( pIR->Event.KeyEvent ) ) {
                    EdInputEvent rv;
                    rv.fIsEdKC_EVENT            = false;
                    rv.rawkey.unicodeChar       = pIR->Event.KeyEvent.uChar.UnicodeChar;
                    rv.rawkey.wVirtualKeyCode   = pIR->Event.KeyEvent.wVirtualKeyCode;
                    rv.rawkey.dwControlKeyState = pIR->Event.KeyEvent.dwControlKeyState;
                    return rv;
                    }
                 break;
            case FOCUS_EVENT: {
                 EdInputEvent rv;
                 rv.fIsEdKC_EVENT = true;
                 rv.   EdKC_EVENT = pIR->Event.FocusEvent.bSetFocus ? EdKC_EVENT_ProgramGotFocus : EdKC_EVENT_ProgramLostFocus;
                 0 && DBG( "*** InputEvent %s ***", pIR->Event.FocusEvent.bSetFocus ? "GotFocus" : "LostFocus" );
                 return rv;
                 }
#if MOUSE_SUPPORT
            case MOUSE_EVENT:
                 0 && DBG( "*** InputEvent Mouse ***" );
                 if( g_fUseMouse ) {
                    TMouseEvent( pIR->Event.MouseEvent ).Process();
                    }
                 break;
#endif
            case WINDOW_BUFFER_SIZE_EVENT:  break;  // I CANNOT cause this to be generated
            case MENU_EVENT:                break;  // MENU_EVENT _IS_ received, but its field(s) are reserved AND it's unclear what use these could be anyway, so we'll ignore
            }
         }
      }
   }

typedef const EdKC XlatForKey[5];
enum shiftIdx { shiftNONE, shiftALT, shiftCTRL, shiftSHIFT, shiftCTRL_SHIFT }; // index into XlatForKey[4]

STIL int VK_shift_to_EdKC( XlatForKey *XlatTbl, int theVK, shiftIdx effectiveShift ) {
   const EdKC rv( XlatTbl[ theVK ][ effectiveShift ] );
   0 && DBG( "XlatKey VK 0x%02X, s %d -> EdKC=%03X", theVK, effectiveShift, rv );
   return rv;
   }

STATIC_CONST XlatForKey normalXlatTbl[ 0xE0 ] = {

//   NORM           ALT               CTRL              SHIFT            CTRL+SHIFT                       VK_xxx
//   -------------  ----------------  ----------------  ---------------- ----------------               ----------------
   { 0,             0,                0,                0,               0,                 }, //    0
   { 0,             0,                0,                0,               0,                 }, // 0x01  VK_LBUTTON              Left mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x02  VK_RBUTTON              Right mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x03  VK_CANCEL               Control-break processing
   { 0,             0,                0,                0,               0,                 }, // 0x04  VK_MBUTTON              Middle mouse button (three-button mouse)
   { 0,             0,                0,                0,               0,                 }, // 0x05  VK_XBUTTON1             Windows 2000/XP: X1 mouse button
   { 0,             0,                0,                0,               0,                 }, // 0x06  VK_XBUTTON2             Windows 2000/XP: X2 mouse button
   { 0,             0,                0,                0,               0,                 }, //    7  - (07)                  Undefined
   { EdKC_bksp,     EdKC_a_bksp,      EdKC_c_bksp,      EdKC_s_bksp,     EdKC_cs_bksp,      }, // 0x08  VK_BACK                 BACKSPACE key
   { EdKC_tab,      EdKC_a_tab,       EdKC_c_tab,       EdKC_s_tab,      EdKC_cs_tab,       }, // 0x09  VK_TAB                  TAB key
   { 0,             0,                0,                0,               0,                 }, //    A  - (0A-0B)               Reserved
   { 0,             0,                0,                0,               0,                 }, //    B  - (0A-0B)               Reserved
   { EdKC_center,   EdKC_a_center,    EdKC_c_center,    EdKC_s_center,   EdKC_cs_center,    }, // 0x0C  VK_CLEAR                CLEAR key
   { EdKC_enter,    EdKC_a_enter,     EdKC_c_enter,     EdKC_s_enter,    EdKC_cs_enter,     }, // 0x0D  VK_RETURN               ENTER key
   { 0,             0,                0,                0,               0,                 }, //    E  - (0E-0F)               Undefined
   { 0,             0,                0,                0,               0,                 }, //    F  - (0E-0F)               Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x10  VK_SHIFT                SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0x11  VK_CONTROL              CTRL key
   { 0,             0,                0,                0,               0,                 }, // 0x12  VK_MENU                 ALT key
   { EdKC_pause,    EdKC_a_pause,     EdKC_c_numlk,     EdKC_s_pause,    EdKC_cs_pause,     }, // 0x13  VK_PAUSE                PAUSE key
   { 0,             0,                0,                0,               0,                 }, // 0x14  VK_CAPITAL              CAPS LOCK key
   { 0,             0,                0,                0,               0,                 }, // 0x15  VK_HANGUL               IME Hangul mode
   { 0,             0,                0,                0,               0,                 }, //    6  - (16)                  Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x17  VK_JUNJA                IME Junja mode
   { 0,             0,                0,                0,               0,                 }, // 0x18  VK_FINAL                IME final mode
   { 0,             0,                0,                0,               0,                 }, // 0x19  VK_KANJI                IME Kanji mode
   { 0,             0,                0,                0,               0,                 }, //    A  - (1A)                  Undefined
   { EdKC_esc,      0,                EdKC_c_esc,       EdKC_s_esc,      0,                 }, // 0x1B  VK_ESCAPE               ESC key
   { 0,             0,                0,                0,               0,                 }, // 0x1C  VK_CONVERT              IME convert
   { 0,             0,                0,                0,               0,                 }, // 0x1D  VK_NONCONVERT           IME nonconvert
   { 0,             0,                0,                0,               0,                 }, // 0x1E  VK_ACCEPT               IME accept
   { 0,             0,                0,                0,               0,                 }, // 0x1F  VK_MODECHANGE           IME mode change request
   { 0,             EdKC_a_space,     EdKC_c_space,     0,               EdKC_cs_space,     }, // 0x20  VK_SPACE                SPACEBAR
   { EdKC_pgup,     EdKC_a_pgup,      EdKC_c_pgup,      EdKC_s_pgup,     EdKC_cs_pgup,      }, // 0x21  VK_PRIOR                PAGE UP key
   { EdKC_pgdn,     EdKC_a_pgdn,      EdKC_c_pgdn,      EdKC_s_pgdn,     EdKC_cs_pgdn,      }, // 0x22  VK_NEXT                 PAGE DOWN key
   { EdKC_end,      EdKC_a_end,       EdKC_c_end,       EdKC_s_end,      EdKC_cs_end,       }, // 0x23  VK_END                  END key
   { EdKC_home,     EdKC_a_home,      EdKC_c_home,      EdKC_s_home,     EdKC_cs_home,      }, // 0x24  VK_HOME                 HOME key
   { EdKC_left,     EdKC_a_left,      EdKC_c_left,      EdKC_s_left,     EdKC_cs_left,      }, // 0x25  VK_LEFT                 LEFT ARROW key
   { EdKC_up,       EdKC_a_up,        EdKC_c_up,        EdKC_s_up,       EdKC_cs_up,        }, // 0x26  VK_UP                   UP ARROW key
   { EdKC_right,    EdKC_a_right,     EdKC_c_right,     EdKC_s_right,    EdKC_cs_right,     }, // 0x27  VK_RIGHT                RIGHT ARROW key
   { EdKC_down,     EdKC_a_down,      EdKC_c_down,      EdKC_s_down,     EdKC_cs_down,      }, // 0x28  VK_DOWN                 DOWN ARROW key
   { 0,             0,                0,                0,               0,                 }, // 0x29  VK_SELECT               SELECT key
   { 0,             0,                0,                0,               0,                 }, // 0x2A  VK_PRINT                PRINT key
   { 0,             0,                0,                0,               0,                 }, // 0x2B  VK_EXECUTE              EXECUTE key
   { 0,             0,                0,                0,               0,                 }, // 0x2C  VK_SNAPSHOT             PRINT SCREEN key
   { EdKC_ins,      EdKC_a_ins,       EdKC_c_ins,       EdKC_s_ins,      EdKC_cs_ins,       }, // 0x2D  VK_INSERT               INS key
   { EdKC_del,      EdKC_a_del,       EdKC_c_del,       EdKC_s_del,      EdKC_cs_del,       }, // 0x2E  VK_DELETE               DEL key
   { 0,             0,                0,                0,               0,                 }, // 0x2F  VK_HELP                 HELP key
   { 0,             EdKC_a_0,         EdKC_c_0,         0,               EdKC_cs_0,         }, // 0x30  VK_0                    0 key
   { 0,             EdKC_a_1,         EdKC_c_1,         0,               EdKC_cs_1,         }, // 0x31  VK_1                    1 key
   { 0,             EdKC_a_2,         EdKC_c_2,         0,               EdKC_cs_2,         }, // 0x32  VK_2                    2 key
   { 0,             EdKC_a_3,         EdKC_c_3,         0,               EdKC_cs_3,         }, // 0x33  VK_3                    3 key
   { 0,             EdKC_a_4,         EdKC_c_4,         0,               EdKC_cs_4,         }, // 0x34  VK_4                    4 key
   { 0,             EdKC_a_5,         EdKC_c_5,         0,               EdKC_cs_5,         }, // 0x35  VK_5                    5 key
   { 0,             EdKC_a_6,         EdKC_c_6,         0,               EdKC_cs_6,         }, // 0x36  VK_6                    6 key
   { 0,             EdKC_a_7,         EdKC_c_7,         0,               EdKC_cs_7,         }, // 0x37  VK_7                    7 key
   { 0,             EdKC_a_8,         EdKC_c_8,         0,               EdKC_cs_8,         }, // 0x38  VK_8                    8 key
   { 0,             EdKC_a_9,         EdKC_c_9,         0,               EdKC_cs_9,         }, // 0x39  VK_9                    9 key
   { 0,             0,                0,                0,               0,                 }, // 0x3A          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3B          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3C          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3D          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3E          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x3F          Undefined
   { 0,             0,                0,                0,               0,                 }, // 0x40          Undefined
   { 0,             EdKC_a_a,         EdKC_c_a,         0,               EdKC_cs_a,         }, // 0x41  VK_A
   { 0,             EdKC_a_b,         EdKC_c_b,         0,               EdKC_cs_b,         }, // 0x42  VK_B
   { 0,             EdKC_a_c,         EdKC_c_c,         0,               EdKC_cs_c,         }, // 0x43  VK_C
   { 0,             EdKC_a_d,         EdKC_c_d,         0,               EdKC_cs_d,         }, // 0x44  VK_D
   { 0,             EdKC_a_e,         EdKC_c_e,         0,               EdKC_cs_e,         }, // 0x45  VK_E
   { 0,             EdKC_a_f,         EdKC_c_f,         0,               EdKC_cs_f,         }, // 0x46  VK_F
   { 0,             EdKC_a_g,         EdKC_c_g,         0,               EdKC_cs_g,         }, // 0x47  VK_G
   { 0,             EdKC_a_h,         EdKC_c_h,         0,               EdKC_cs_h,         }, // 0x48  VK_H
   { 0,             EdKC_a_i,         EdKC_c_i,         0,               EdKC_cs_i,         }, // 0x49  VK_I
   { 0,             EdKC_a_j,         EdKC_c_j,         0,               EdKC_cs_j,         }, // 0x4A  VK_J
   { 0,             EdKC_a_k,         EdKC_c_k,         0,               EdKC_cs_k,         }, // 0x4B  VK_K
   { 0,             EdKC_a_l,         EdKC_c_l,         0,               EdKC_cs_l,         }, // 0x4C  VK_L
   { 0,             EdKC_a_m,         EdKC_c_m,         0,               EdKC_cs_m,         }, // 0x4D  VK_M
   { 0,             EdKC_a_n,         EdKC_c_n,         0,               EdKC_cs_n,         }, // 0x4E  VK_N
   { 0,             EdKC_a_o,         EdKC_c_o,         0,               EdKC_cs_o,         }, // 0x4F  VK_O
   { 0,             EdKC_a_p,         EdKC_c_p,         0,               EdKC_cs_p,         }, // 0x50  VK_P
   { 0,             EdKC_a_q,         EdKC_c_q,         0,               EdKC_cs_q,         }, // 0x51  VK_Q
   { 0,             EdKC_a_r,         EdKC_c_r,         0,               EdKC_cs_r,         }, // 0x52  VK_R
   { 0,             EdKC_a_s,         EdKC_c_s,         0,               EdKC_cs_s,         }, // 0x53  VK_S
   { 0,             EdKC_a_t,         EdKC_c_t,         0,               EdKC_cs_t,         }, // 0x54  VK_T
   { 0,             EdKC_a_u,         EdKC_c_u,         0,               EdKC_cs_u,         }, // 0x55  VK_U
   { 0,             EdKC_a_v,         EdKC_c_v,         0,               EdKC_cs_v,         }, // 0x56  VK_V
   { 0,             EdKC_a_w,         EdKC_c_w,         0,               EdKC_cs_w,         }, // 0x57  VK_W
   { 0,             EdKC_a_x,         EdKC_c_x,         0,               EdKC_cs_x,         }, // 0x58  VK_X
   { 0,             EdKC_a_y,         EdKC_c_y,         0,               EdKC_cs_y,         }, // 0x59  VK_Y
   { 0,             EdKC_a_z,         EdKC_c_z,         0,               EdKC_cs_z,         }, // 0x5A  VK_Z
   { 0,             0,                0,                0,               0,                 }, // 0x5B  VK_LWIN                 Left Windows key (Microsoft Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5C  VK_RWIN                 Right Windows key (Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5D  VK_APPS                 Applications key (Natural keyboard)
   { 0,             0,                0,                0,               0,                 }, // 0x5E         Reserved
   { 0,             0,                0,                0,               0,                 }, // 0x5F  VK_SLEEP                Computer Sleep key
   { 0,             0,                0,                0,               0,                 }, // 0x60  VK_NUMPAD0              Numeric keypad 0 key
   { 0,             0,                0,                0,               0,                 }, // 0x61  VK_NUMPAD1              Numeric keypad 1 key
   { 0,             0,                0,                0,               0,                 }, // 0x62  VK_NUMPAD2              Numeric keypad 2 key
   { 0,             0,                0,                0,               0,                 }, // 0x63  VK_NUMPAD3              Numeric keypad 3 key
   { 0,             0,                0,                0,               0,                 }, // 0x64  VK_NUMPAD4              Numeric keypad 4 key
   { 0,             0,                0,                0,               0,                 }, // 0x65  VK_NUMPAD5              Numeric keypad 5 key
   { 0,             0,                0,                0,               0,                 }, // 0x66  VK_NUMPAD6              Numeric keypad 6 key
   { 0,             0,                0,                0,               0,                 }, // 0x67  VK_NUMPAD7              Numeric keypad 7 key
   { 0,             0,                0,                0,               0,                 }, // 0x68  VK_NUMPAD8              Numeric keypad 8 key
   { 0,             0,                0,                0,               0,                 }, // 0x69  VK_NUMPAD9              Numeric keypad 9 key
   { EdKC_numStar , EdKC_a_numStar ,  EdKC_c_numStar ,  EdKC_s_numStar , EdKC_cs_numStar ,  }, // 0x6A  VK_MULTIPLY             Multiply key
   { EdKC_numPlus , EdKC_a_numPlus ,  EdKC_c_numPlus ,  EdKC_s_numPlus , EdKC_cs_numPlus ,  }, // 0x6B  VK_ADD                  Add key
   { EdKC_numEnter, EdKC_a_numEnter,  EdKC_c_numEnter,  EdKC_s_numEnter, EdKC_cs_numEnter,  }, // 0x6C  VK_SEPARATOR            Separator key
   { EdKC_numMinus, EdKC_a_numMinus,  EdKC_c_numMinus,  EdKC_s_numMinus, EdKC_cs_numMinus,  }, // 0x6D  VK_SUBTRACT             Subtract key
   { 0,             0,                0,                0,               0,                 }, // 0x6E  VK_DECIMAL              Decimal key
   { EdKC_numSlash, EdKC_a_numSlash,  EdKC_c_numSlash,  EdKC_s_numSlash, EdKC_cs_numSlash,  }, // 0x6F  VK_DIVIDE               Divide key
   { EdKC_f1,       EdKC_a_f1,        EdKC_c_f1,        EdKC_s_f1,       EdKC_cs_f1,        }, // 0x70  VK_F1                   F1 key
   { EdKC_f2,       EdKC_a_f2,        EdKC_c_f2,        EdKC_s_f2,       EdKC_cs_f2,        }, // 0x71  VK_F2                   F2 key
   { EdKC_f3,       EdKC_a_f3,        EdKC_c_f3,        EdKC_s_f3,       EdKC_cs_f3,        }, // 0x72  VK_F3                   F3 key
   { EdKC_f4,       EdKC_a_f4,        EdKC_c_f4,        EdKC_s_f4,       EdKC_cs_f4,        }, // 0x73  VK_F4                   F4 key
   { EdKC_f5,       EdKC_a_f5,        EdKC_c_f5,        EdKC_s_f5,       EdKC_cs_f5,        }, // 0x74  VK_F5                   F5 key
   { EdKC_f6,       EdKC_a_f6,        EdKC_c_f6,        EdKC_s_f6,       EdKC_cs_f6,        }, // 0x75  VK_F6                   F6 key
   { EdKC_f7,       EdKC_a_f7,        EdKC_c_f7,        EdKC_s_f7,       EdKC_cs_f7,        }, // 0x76  VK_F7                   F7 key
   { EdKC_f8,       EdKC_a_f8,        EdKC_c_f8,        EdKC_s_f8,       EdKC_cs_f8,        }, // 0x77  VK_F8                   F8 key
   { EdKC_f9,       EdKC_a_f9,        EdKC_c_f9,        EdKC_s_f9,       EdKC_cs_f9,        }, // 0x78  VK_F9                   F9 key
   { EdKC_f10,      EdKC_a_f10,       EdKC_c_f10,       EdKC_s_f10,      EdKC_cs_f10,       }, // 0x79  VK_F10                  F10 key
   { EdKC_f11,      EdKC_a_f11,       EdKC_c_f11,       EdKC_s_f11,      EdKC_cs_f11,       }, // 0x7A  VK_F11                  F11 key
   { EdKC_f12,      EdKC_a_f12,       EdKC_c_f12,       EdKC_s_f12,      EdKC_cs_f12,       }, // 0x7B  VK_F12                  F12 key
   { 0,             0,                0,                0,               0,                 }, // 0x7C  VK_F13                  F13 key
   { 0,             0,                0,                0,               0,                 }, // 0x7D  VK_F14                  F14 key
   { 0,             0,                0,                0,               0,                 }, // 0x7E  VK_F15                  F15 key
   { 0,             0,                0,                0,               0,                 }, // 0x7F  VK_F16                  F16 key
   { 0,             0,                0,                0,               0,                 }, // 0x80  VK_F17                  F17 key
   { 0,             0,                0,                0,               0,                 }, // 0x81  VK_F18                  F18 key
   { 0,             0,                0,                0,               0,                 }, // 0x82  VK_F19                  F19 key
   { 0,             0,                0,                0,               0,                 }, // 0x83  VK_F20                  F20 key
   { 0,             0,                0,                0,               0,                 }, // 0x84  VK_F21                  F21 key
   { 0,             0,                0,                0,               0,                 }, // 0x85  VK_F22                  F22 key
   { 0,             0,                0,                0,               0,                 }, // 0x86  VK_F23                  F23 key
   { 0,             0,                0,                0,               0,                 }, // 0x87  VK_F24                  F24 key
   { 0,             0,                0,                0,               0,                 }, // 0x88          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x89          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8A          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8B          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8C          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8D          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8E          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x8F          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x90  VK_NUMLOCK              NUM LOCK key
   { EdKC_scroll,   EdKC_a_scroll,    0,                EdKC_s_scroll,   EdKC_cs_scroll,    }, // 0x91  VK_SCROLL               SCROLL LOCK key
   { 0,             0,                0,                0,               0,                 }, // 0x92   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x93   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x94   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x95   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x96   (92-96)                OEM specific
   { 0,             0,                0,                0,               0,                 }, // 0x97  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x98  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x99  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9A  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9B  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9C  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9D  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9E  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0x9F  - (97-9F)               Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xA0  VK_LSHIFT               Left SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0xA1  VK_RSHIFT               Right SHIFT key
   { 0,             0,                0,                0,               0,                 }, // 0xA2  VK_LCONTROL             Left CONTROL key
   { 0,             0,                0,                0,               0,                 }, // 0xA3  VK_RCONTROL             Right CONTROL key
   { 0,             0,                0,                0,               0,                 }, // 0xA4  VK_LMENU                Left MENU key
   { 0,             0,                0,                0,               0,                 }, // 0xA5  VK_RMENU                Right MENU key
   { 0,             0,                0,                0,               0,                 }, // 0xA6  VK_BROWSER_BACK         Windows 2000/XP: Browser Back key
   { 0,             0,                0,                0,               0,                 }, // 0xA7  VK_BROWSER_FORWARD      Windows 2000/XP: Browser Forward key
   { 0,             0,                0,                0,               0,                 }, // 0xA8  VK_BROWSER_REFRESH      Windows 2000/XP: Browser Refresh key
   { 0,             0,                0,                0,               0,                 }, // 0xA9  VK_BROWSER_STOP         Windows 2000/XP: Browser Stop key
   { 0,             0,                0,                0,               0,                 }, // 0xAA  VK_BROWSER_SEARCH       Windows 2000/XP: Browser Search key
   { 0,             0,                0,                0,               0,                 }, // 0xAB  VK_BROWSER_FAVORITES    Windows 2000/XP: Browser Favorites key
   { 0,             0,                0,                0,               0,                 }, // 0xAC  VK_BROWSER_HOME         Windows 2000/XP: Browser Start and Home key
   { 0,             0,                0,                0,               0,                 }, // 0xAD  VK_VOLUME_MUTE          Windows 2000/XP: Volume Mute key
   { 0,             0,                0,                0,               0,                 }, // 0xAE  VK_VOLUME_DOWN          Windows 2000/XP: Volume Down key
   { 0,             0,                0,                0,               0,                 }, // 0xAF  VK_VOLUME_UP            Windows 2000/XP: Volume Up key
   { 0,             0,                0,                0,               0,                 }, // 0xB0  VK_MEDIA_NEXT_TRACK     Windows 2000/XP: Next Track key
   { 0,             0,                0,                0,               0,                 }, // 0xB1  VK_MEDIA_PREV_TRACK     Windows 2000/XP: Previous Track key
   { 0,             0,                0,                0,               0,                 }, // 0xB2  VK_MEDIA_STOP           Windows 2000/XP: Stop Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB3  VK_MEDIA_PLAY_PAUSE     Windows 2000/XP: Play/Pause Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB4  VK_LAUNCH_MAIL          Windows 2000/XP: Start Mail key
   { 0,             0,                0,                0,               0,                 }, // 0xB5  VK_LAUNCH_MEDIA_SELECT  Windows 2000/XP: Select Media key
   { 0,             0,                0,                0,               0,                 }, // 0xB6  VK_LAUNCH_APP1          Windows 2000/XP: Start Application 1 key
   { 0,             0,                0,                0,               0,                 }, // 0xB7  VK_LAUNCH_APP2          Windows 2000/XP: Start Application 2 key
   { 0,             0,                0,                0,               0,                 }, // 0xB8         Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xB9         Reserved
   { 0,             EdKC_a_SEMICOLON, EdKC_c_SEMICOLON, 0,               EdKC_cs_SEMICOLON, }, // 0xBA  VK_OEM_1                Windows 2000/XP: For the US standard keyboard, the ';:' key
   { 0,             EdKC_a_EQUAL,     EdKC_c_EQUAL,     0,               EdKC_cs_EQUAL,     }, // 0xBB  VK_OEM_PLUS             Windows 2000/XP: For any country/region, the '+' key
   { 0,             EdKC_a_COMMA,     EdKC_c_COMMA,     0,               EdKC_cs_COMMA,     }, // 0xBC  VK_OEM_COMMA            Windows 2000/XP: For any country/region, the ',' key
   { 0,             EdKC_a_MINUS,     EdKC_c_MINUS,     0,               EdKC_cs_MINUS,     }, // 0xBD  VK_OEM_MINUS            Windows 2000/XP: For any country/region, the '-' key
   { 0,             EdKC_a_DOT,       EdKC_c_DOT,       0,               EdKC_cs_DOT,       }, // 0xBE  VK_OEM_PERIOD           Windows 2000/XP: For any country/region, the '.' key
   { 0,             EdKC_a_SLASH,     EdKC_c_SLASH,     0,               EdKC_cs_SLASH,     }, // 0xBF  VK_OEM_2                Windows 2000/XP: For the US standard keyboard, the '/?' key
   { 0,             EdKC_a_BACKTICK,  EdKC_c_BACKTICK,  0,               EdKC_cs_BACKTICK,  }, // 0xC0  VK_OEM_3                Windows 2000/XP: For the US standard keyboard, the '`~' key
   { 0,             EdKC_a_TICK,      EdKC_c_TICK,      0,               EdKC_cs_TICK,      }, // 0xC1          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC2          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC3          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC4          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC5          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC6          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC7          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC8          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xC9          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCA          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCB          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCC          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCD          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCE          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xCF          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD0          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD1          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD2          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD3          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD4          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD5          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD6          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD7          Reserved
   { 0,             0,                0,                0,               0,                 }, // 0xD8          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xD9          Unassigned
   { 0,             0,                0,                0,               0,                 }, // 0xDA          Unassigned
   { 0,             EdKC_a_LEFT_SQ,   EdKC_c_LEFT_SQ,   0,               EdKC_cs_LEFT_SQ,   }, // 0xDB VK_OEM_4                 Windows 2000/XP: For the US standard keyboard, the '[{' key
   { 0,             EdKC_a_BACKSLASH, EdKC_c_BACKSLASH, 0,               EdKC_cs_BACKSLASH, }, // 0xDC VK_OEM_5                 Windows 2000/XP: For the US standard keyboard, the '\|' key
   { 0,             EdKC_a_RIGHT_SQ,  EdKC_c_RIGHT_SQ,  0,               EdKC_cs_RIGHT_SQ,  }, // 0xDD VK_OEM_6                 Windows 2000/XP: For the US standard keyboard, the ']}' key
   { 0,             EdKC_a_TICK,      EdKC_c_TICK,      0,               EdKC_cs_TICK,      }, // 0xDE VK_OEM_7                 Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
   { 0,             EdKC_a_EQUAL,     EdKC_c_EQUAL,     0,               EdKC_cs_EQUAL,     }, // 0xDF VK_OEM_8                 Used for miscellaneous characters; it can vary by keyboard.
                                                                                               // 0xE0          Reserved
                                                                                               // 0xE1          OEM specific
                                                                                               // 0xE2 VK_OEM_102               Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
                                                                                               // 0xE3          OEM specific
                                                                                               // 0xE4          OEM specific
                                                                                               // 0xE5 VK_PROCESSKEY            Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
                                                                                               // 0xE6          OEM specific
                                                                                               // 0xE7 VK_PACKET                Windows 2000/XP: Used to pass Unicode characters ... (see below)
                                                                                               // 0xE8          Unassigned
                                                                                               // 0xE9          OEM specific
                                                                                               // 0xEA          OEM specific
                                                                                               // 0xEB          OEM specific
                                                                                               // 0xEC          OEM specific
                                                                                               // 0xED          OEM specific
                                                                                               // 0xEE          OEM specific
                                                                                               // 0xEF          OEM specific
                                                                                               // 0xF0          OEM specific
                                                                                               // 0xF1          OEM specific
                                                                                               // 0xF2          OEM specific
                                                                                               // 0xF3          OEM specific
                                                                                               // 0xF4          OEM specific
                                                                                               // 0xF5          OEM specific
                                                                                               // 0xF6 VK_ATTN                  Attn key
                                                                                               // 0xF7 VK_CRSEL                 CrSel key
                                                                                               // 0xF8 VK_EXSEL                 ExSel key
                                                                                               // 0xF9 VK_EREOF                 Erase EOF key
                                                                                               // 0xFA VK_PLAY                  Play key
                                                                                               // 0xFB VK_ZOOM                  Zoom key
                                                                                               // 0xFC VK_NONAME                Reserved
                                                                                               // 0xFD VK_PA1                   PA1 key
                                                                                               // 0xFE VK_OEM_CLEAR             Clear key

   };

// VK_PACKET (E7): The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP

STATIC_CONST XlatForKey numLockXlatTbl[ 15 ] = {
   //    NORM       ALT               CTRL              SHIFT             CTRL+ALT
   //-------------  ----------------  ----------------  ----------------  ----------------
   { EdKC_num0    , EdKC_a_num0     , EdKC_c_num0     , EdKC_s_num0     , EdKC_cs_num0     , }, //  0  VK_NUMPAD0
   { EdKC_num1    , EdKC_a_num1     , EdKC_c_num1     , EdKC_s_num1     , EdKC_cs_num1     , }, //  1  VK_NUMPAD1
   { EdKC_num2    , EdKC_a_num2     , EdKC_c_num2     , EdKC_s_num2     , EdKC_cs_num2     , }, //  2  VK_NUMPAD2
   { EdKC_num3    , EdKC_a_num3     , EdKC_c_num3     , EdKC_s_num3     , EdKC_cs_num3     , }, //  3  VK_NUMPAD3
   { EdKC_num4    , EdKC_a_num4     , EdKC_c_num4     , EdKC_s_num4     , EdKC_cs_num4     , }, //  4  VK_NUMPAD4
   { EdKC_num5    , EdKC_a_num5     , EdKC_c_num5     , EdKC_s_num5     , EdKC_cs_num5     , }, //  5  VK_NUMPAD5
   { EdKC_num6    , EdKC_a_num6     , EdKC_c_num6     , EdKC_s_num6     , EdKC_cs_num6     , }, //  6  VK_NUMPAD6
   { EdKC_num7    , EdKC_a_num7     , EdKC_c_num7     , EdKC_s_num7     , EdKC_cs_num7     , }, //  7  VK_NUMPAD7
   { EdKC_num8    , EdKC_a_num8     , EdKC_c_num8     , EdKC_s_num8     , EdKC_cs_num8     , }, //  8  VK_NUMPAD8
   { EdKC_num9    , EdKC_a_num9     , EdKC_c_num9     , EdKC_s_num9     , EdKC_cs_num9     , }, //  9  VK_NUMPAD9
   { EdKC_numMinus, EdKC_a_numMinus , EdKC_c_numMinus , EdKC_s_numMinus , EdKC_cs_numMinus , }, // 10  VK_SUBTRACT
   { EdKC_numPlus , EdKC_a_numPlus  , EdKC_c_numPlus  , EdKC_s_numPlus  , EdKC_cs_numPlus  , }, // 11  VK_ADD
   { EdKC_numStar , EdKC_a_numStar  , EdKC_c_numStar  , EdKC_s_numStar  , EdKC_cs_numStar  , }, // 12  VK_MULTIPLY
   { EdKC_numSlash, EdKC_a_numSlash , EdKC_c_numSlash , EdKC_s_numSlash , EdKC_cs_numSlash , }, // 13  VK_DIVIDE
   { EdKC_numEnter, EdKC_a_numEnter , EdKC_c_numEnter , EdKC_s_numEnter , EdKC_cs_numEnter , }, // 14  VK_SEPARATOR
   };


//
// KEY_DATA - DEEP INTERNAL struct, used ONLY in LOW-LEVEL interface between Win32 and Editor.
//            NOT to be used within any Editor data structures!!!
//
struct KEY_DATA {
   uint8_t  Ascii;  // Ascii code
   uint8_t  d_VK;   // VK code
   uint8_t  kFlags; // Flags
   // values for the kFlags field
   //
   enum {
      FLAG_SHIFT   = BIT(0),
      FLAG_NUMLOCK = BIT(1),
      FLAG_ALT     = BIT(2),
      FLAG_CTRL    = BIT(3),
      };
   };

//
// KeyData_EdKC - DEEP INTERNAL struct, used ONLY in LOW-LEVEL interface between Win32 and Editor.
//                NOT to be used within any Editor data structures!!!
//
struct KeyData_EdKC {
   KEY_DATA    k_d;   // input
   EdKC        EdKC_; // output
   };

STATIC_FXN KeyData_EdKC GetInputEvent() {
   const auto eie( ReadInputEventPrimitive() );
   if( eie.fIsEdKC_EVENT ) {
      KeyData_EdKC rv;
      rv.k_d.Ascii  = '\0';
      rv.k_d.d_VK   = 0;
      rv.k_d.kFlags = 0;
      rv.EdKC_      = eie.EdKC_EVENT;  0 && DBG( "EdKC_+ %04X", rv.EdKC_ );
      return rv;
      }
   const auto &rwkd( eie.rawkey );
   auto edKC(0);
   auto allShifts(0);
   const uint8_t valAscii( 0xFF & rwkd.unicodeChar );
   const uint8_t valVK(    0xFF & rwkd.wVirtualKeyCode );  0 && DBG( "VK+ %02X", valVK );
   if( valVK < ELEMENTS(normalXlatTbl) ) {
      const auto anyAlt   ( ToBOOL(rwkd.dwControlKeyState & (RIGHT_ALT_PRESSED  | LEFT_ALT_PRESSED )) );
      const auto anyCtrl  ( ToBOOL(rwkd.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) );
      const auto anyShift ( ToBOOL(rwkd.dwControlKeyState &  SHIFT_PRESSED) );
      const auto numlockon( ToBOOL(rwkd.dwControlKeyState &  NUMLOCK_ON)    );
      shiftIdx effectiveShiftIdx = shiftNONE;
      if( anyAlt   ) { allShifts |= KEY_DATA::FLAG_ALT   ; effectiveShiftIdx = shiftALT         ; }
      if( anyShift ) { allShifts |= KEY_DATA::FLAG_SHIFT ; effectiveShiftIdx = shiftSHIFT       ; }
      if( anyCtrl  ) { allShifts |= KEY_DATA::FLAG_CTRL  ; effectiveShiftIdx = shiftCTRL        ;
                                                           // if( valAscii != 0 ) // "Ctrl+ascii" is directly reflected in the valAscii code itself
                                                           //    {                // so there's no reason to include a Ctrl-shift code
                                                           //    effectiveShiftIdx = shiftNONE;
                                                           //    allShifts = 0;
                                                           //    }
                     }
      if( anyCtrl && anyShift ) {                          effectiveShiftIdx = shiftCTRL_SHIFT; }
      //*********  translate VK into EdKC  *********
      if( numlockon ) {
         allShifts |= KEY_DATA::FLAG_NUMLOCK;
         const auto numlockVK( XlatKeysWhenNumlockOn( valVK ) );
         if( numlockVK >= 0 ) { edKC = VK_shift_to_EdKC( numLockXlatTbl, numlockVK, effectiveShiftIdx ); }
         else                 { edKC = VK_shift_to_EdKC( normalXlatTbl , valVK    , effectiveShiftIdx ); }
         }
      else                    { edKC = VK_shift_to_EdKC( normalXlatTbl , valVK    , effectiveShiftIdx ); }
      }
   if( edKC == 0 ) {
       edKC = valAscii;
       }
   KeyData_EdKC rv;
   rv.k_d.Ascii  = valAscii;
   rv.k_d.d_VK   = valVK;
   rv.k_d.kFlags = allShifts;
   rv.EdKC_      = ModalKeyRemap( edKC );
   return rv;
   }

STATIC_FXN std::string RawKeyStr( const KEY_DATA &k_d ) {
   return std::string(
      FmtStr<40>( "VK=x%02X, Flags=%s%s%s%s, ch=0x%02X (%c)"
         ,  k_d.d_VK
         , (k_d.kFlags & KEY_DATA::FLAG_CTRL   ) ? "C" : ""
         , (k_d.kFlags & KEY_DATA::FLAG_ALT    ) ? "A" : ""
         , (k_d.kFlags & KEY_DATA::FLAG_SHIFT  ) ? "S" : ""
         , (k_d.kFlags & KEY_DATA::FLAG_NUMLOCK) ? "N" : ""
         ,  k_d.Ascii
         ,  k_d.Ascii
         )
      );
   }

STATIC_FXN KeyData_EdKC GetKeyData_EdKC( bool fFreezeOtherThreads ) { // PRIMARY API for reading a key
   while( true ) {
      const auto fPassThreadBaton( !(fFreezeOtherThreads || KbHit()) );
      if( fPassThreadBaton )  MainThreadGiveUpGlobalVariableLock();
      0 && DBG( "%s %s", __func__, fPassThreadBaton ? "passing" : "holding" );
      const KeyData_EdKC rv( GetInputEvent() );
      if( fPassThreadBaton )  MainThreadWaitForGlobalVariableLock();
      if( rv.EdKC_ < EdKC_COUNT ) {
         if( rv.EdKC_ == 0 ) { // (rv.EdKC_ == 0) corresponds to keys we currently do not decode
            0 && DBG( "%s(rv.EdKC==0): %s", __func__, RawKeyStr( rv.k_d ).c_str() );
            }
         else {
            return rv;  // normal exit path
            }
         }
      else {
         0 && DBG( "%s %u (vs. %u)", __func__, rv.EdKC_, EdKC_COUNT );
         switch( rv.EdKC_ ) {
            case EdKC_EVENT_ProgramGotFocus:      RefreshCheckAllWindowsFBufs();  break;
            case EdKC_EVENT_ProgramExitRequested: EditorExit( 0, true );          break;
            }
         }
      CleanupAnyExecutionHaltRequest();
      }
   }

void ConIn::WaitForKey() {
   GetKeyData_EdKC( true );
   }

STIL EdKC_Ascii KeyData_EdKC2EdKC_Ascii( const KeyData_EdKC &ki ) {
   EdKC_Ascii rv;
   rv.Ascii    = ki.k_d.Ascii;
   rv.EdKcEnum = ki.EdKC_;
   return rv;
   }

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey() {
   return KeyData_EdKC2EdKC_Ascii( GetKeyData_EdKC( false ) );
   }

EdKC_Ascii ConIn::EdKC_Ascii_FromNextKey_Keystr( std::string &dest ) {
   const auto ki( GetKeyData_EdKC( true ) );
   dest.assign( (ki.EdKC_ == 0) ? RawKeyStr( ki.k_d ) : KeyNmOfEdkc( ki.EdKC_ ) );
   return KeyData_EdKC2EdKC_Ascii( ki );
   }

//
//   END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING
//   END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING
//   END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING
//   END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING  END KEY_DECODING
//
//#############################################################################################################################


#ifdef fn_kbfast

//////////////////////////////////////////////////////////////////////////
//
//  KBfast    11-May-1998 klg
//
//      Restore the keyboard repeat rate (speed, delay) in the Win32 environment
//      (when using a keyboard switchbox, the keyboard settings get blown away
//      whenever I switch _back_to_ my Win32 (NT 4.0) system)
//
//////////////////////////////////////////////////////////////////////////

STATIC_FXN int setFilterKeysFailed( int flags, int delayMSec, int repeatMSec ) {
   // 20071126 kgoodwin thanks to Eric Tetz!
   NoLessThan( &repeatMSec, 6 ); // smaller is faster: be careful!!! don't go below 10 or your system may become UNUSABLE!!!
   Win32::FILTERKEYS fk = { sizeof fk };
   fk.dwFlags     = flags;
   fk.iDelayMSec  = delayMSec; // smaller is faster
   fk.iRepeatMSec = repeatMSec;
   return !Win32::SystemParametersInfo( SPI_SETFILTERKEYS, 0, PVoid( &fk ), 0 );
   }

STIL int DisableFilterKeys() {
   return setFilterKeysFailed( 0, 500, 500 );
   }

#define  USE_OLD_KBFAST  1
#if      USE_OLD_KBFAST

//////////////////////////////////////////////////////////////////////////
//
//  KBfast    11-May-1998 klg
//
//      Restore the keyboard repeat rate (speed, delay) in the Win32 environment
//      (when using a keyboard switchbox, the keyboard settings get blown away
//      whenever I switch _back_to_ my Win32 (NT 4.0) system)
//
//////////////////////////////////////////////////////////////////////////

   // SPI_SETKEYBOARDSPEED
   // This value can be any number between 0 and 31.
   // 0 = 2.5 repetitions per second, 31 = 31 repetitions per second.

   // SPI_SETKEYBOARDDELAY
   // This value can be between 0 and 3.
   // 0 = 250 ms delay, 3 = 1 second (1000 ms) delay.

typedef API_UINT Win32::UINT;

STATIC_FXN int getKbSpeedDelayFailed( API_UINT *kbSpeed, API_UINT *kbDelay ) {
   return 0==Win32::SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, kbSpeed, 0 )  ||
          0==Win32::SystemParametersInfo( SPI_GETKEYBOARDDELAY, 0, kbDelay, 0 );
   }

STATIC_FXN int setKbSpeedDelayFailed( API_UINT kbSpeed, API_UINT kbDelay ) {
   return 0==Win32::SystemParametersInfo( SPI_SETKEYBOARDSPEED, kbSpeed, nullptr, 0 )  ||
          0==Win32::SystemParametersInfo( SPI_SETKEYBOARDDELAY, kbDelay, nullptr, 0 );
   }

bool ARG::kbfast() {
   DisableFilterKeys();
   const API_UINT invalid_kbval(0x7777);
   const API_UINT kbSpeedSB(IsWin7OrLater()?28:31); // Should Be
   const API_UINT kbDelaySB(IsWin7OrLater()? 1: 0); // Should Be
         API_UINT kbSpeedWas(invalid_kbval), kbDelayWas(invalid_kbval);
   if( getKbSpeedDelayFailed( &kbSpeedWas, &kbDelayWas ) ) return  Msg( "KbSpeedDelay read failed" );
   if( kbSpeedSB==kbSpeedWas && kbDelaySB==kbDelayWas    ) return !Msg( "KbSpeedDelay already set->(%d,%d)", kbSpeedWas,kbDelayWas );
   0 && DBG( "KbSpeedDelay now (%X,%X) setting (%X,%X)", kbSpeedWas,kbDelayWas , kbSpeedSB,kbDelaySB );
   if( !setKbSpeedDelayFailed( kbSpeedSB, kbDelaySB ) ) {
      API_UINT kbSpeedIs(invalid_kbval), kbDelayIs(invalid_kbval);
      if( !getKbSpeedDelayFailed( &kbSpeedIs, &kbDelayIs ) ) {
         return !Msg( "KbSpeedDelay set (%d,%d)->(%d,%d)", kbSpeedWas,kbDelayWas , kbSpeedIs,kbDelayIs );
         }
      }
   return Msg( "KbSpeedDelay write failed" );
   }

#else // !USE_OLD_KBFAST

#define MS_UNTIL_STARTS_REPEATING   375
#define MS_BETWEEN_REPEATS           15

STIL int setKbFastFailed() {
   return setFilterKeysFailed( FKF_FILTERKEYSON | FKF_AVAILABLE, MS_UNTIL_STARTS_REPEATING, MS_BETWEEN_REPEATS );
   }

bool ARG::kbfast() {
   if( d_fMeta ? DisableFilterKeys() : setKbFastFailed() ) {
      return Msg( "KB access failed" );
      }
   Msg( "KB set to %sest setting",  (d_fMeta ? "slow" : "fast") );
   return true;
   }

#endif

#endif

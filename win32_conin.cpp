// index CONSOLE_OUTPUT KEY_DECODING INIT_N_MISC

//
// Copyright 1991 - 2014 by Kevin L. Goodwin; All rights reserved
//

#include "ed_main.h"
#include "win32_pvt.h"

//--------------------------------------------------------------------------------------------

// see for good explanations:
// http://www.adrianxw.dk/SoftwareSite/Consoles/Consoles5.html


struct conin_statics {
   enum { CIB_DFLT_ELEMENTS = 32, CIB_MIN_ELEMENTS = 64 };

   Win32::HANDLE         hStdin;
   Win32::DWORD          InitialConsoleInputMode;
   int                   CIB_MaxElements;
   Win32::DWORD          CIB_ValidElements;
   Win32::DWORD          CIB_IdxRead;
   Mutex                 mutex;
   std::vector<Win32::INPUT_RECORD> vINPUT_RECORD;
   Win32::PINPUT_RECORD  paINPUT_RECORD;

   conin_statics() : mutex() {};

   void ClearBuf() { CIB_IdxRead = CIB_ValidElements = 0; }
   bool ScanConinBufForKeyDowns();

private:

   conin_statics( const conin_statics & src );
   conin_statics & operator=( const conin_statics & rhs );
   };

STATIC_VAR conin_statics s_Conin;


STATIC_FXN bool IsInterestingKeyEvent( const Win32::KEY_EVENT_RECORD &KER );

bool conin_statics::ScanConinBufForKeyDowns() {
   for( auto ix(CIB_IdxRead); ix < CIB_ValidElements; ++ix ) {
      const auto pIR( &paINPUT_RECORD[ix] );
      0 && DBG( "ConinEv=%d", pIR->EventType );
      if( (KEY_EVENT == pIR->EventType) && IsInterestingKeyEvent( pIR->Event.KeyEvent ) )
          return true;
      }
   return false;
   }


bool FlushKeyQueueAnythingFlushed() {
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
// const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin, &s_Conin.vINPUT_RECORD , s_Conin.vINPUT_RECORD.size(), &s_Conin.CIB_ValidElements ) );
   const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin,  s_Conin.paINPUT_RECORD, s_Conin.CIB_MaxElements     , &s_Conin.CIB_ValidElements ) );
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
   s_Conin.CIB_MaxElements   = conin_statics::CIB_DFLT_ELEMENTS;
   s_Conin.vINPUT_RECORD.resize( s_Conin.CIB_MaxElements );
   AllocArrayNZ( s_Conin.paINPUT_RECORD, s_Conin.CIB_MaxElements, "paINPUT_RECORD" );

   s_Conin.InitialConsoleInputMode = GetConsoleInputMode();

   ConsoleInputAcquire();
   }


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

   if( fWaitingOnInput )
      return nullptr;

   if( 0 == s_Conin.CIB_ValidElements ) {
      if( s_Conin.CIB_MaxElements > conin_statics::CIB_MIN_ELEMENTS ) {
         auto dummy(false);  GotHereDialog( &dummy );  // it's doubtful this is ever executed?
         0 && DBG( "s_Conin.CIB_MaxElements was %d, now %d", s_Conin.CIB_MaxElements, conin_statics::CIB_MIN_ELEMENTS );
         s_Conin.CIB_MaxElements = conin_statics::CIB_MIN_ELEMENTS;
         s_Conin.vINPUT_RECORD.resize( s_Conin.CIB_MaxElements );
         ReallocArray( s_Conin.paINPUT_RECORD, s_Conin.CIB_MaxElements, "paINPUT_RECORD" );
         }

      fWaitingOnInput = true;
      mtx.Release(); //#########################################################

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

//    const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin, &s_Conin.vINPUT_RECORD , s_Conin.vINPUT_RECORD.size(), &s_Conin.CIB_ValidElements ) );
      const auto ok( Win32::ReadConsoleInputA( s_Conin.hStdin,  s_Conin.paINPUT_RECORD, s_Conin.CIB_MaxElements     , &s_Conin.CIB_ValidElements ) );
      if( !ok ) {
         1 && DBG( "%s s_Conin.hStdin=%p, GetStdHandle.Stdin=%p", __func__, s_Conin.hStdin, Win32::GetStdHandle( Win32::Std_Input_Handle() ) );
         linebuf oseb;
         DBG( "Win32::ReadConsoleInputA failed: %s", OsErrStr( BSOB(oseb) ) );
         Assert( ok );
         }

      mtx.Acquire(); //#########################################################
      fWaitingOnInput = false;
      s_Conin.CIB_IdxRead = 0;
      }

   const auto rva( &s_Conin.vINPUT_RECORD[ s_Conin.CIB_IdxRead ] );
   const auto rv ( s_Conin.paINPUT_RECORD + s_Conin.CIB_IdxRead );
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
      auto pMsg( d_QHead.First() );
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
   U16  unicodeChar      ;
   U16  wVirtualKeyCode  ;
   int  dwControlKeyState;
   };
STD_TYPEDEFS( RawWinKeydown )


typedef U16 EdKC;

struct EdInputEvent {
   enum { evt_KEY, evt_MOUSE, evt_FOCUS } // evt
        ;
   bool           fIsEdKC_EVENT;
   union {
      RawWinKeydown rawkey;
      EdKC          EdKC_EVENT;
      };
   };
STD_TYPEDEFS( EdInputEvent )


#if MOUSE_SUPPORT

// insert ConinRecord in next-to-be-read position; this is used for mouse
// processing, in the case of the most recently read ConinRecord _was_ the mouse record

STATIC_FXN void InsertConinRecord( const Win32::INPUT_RECORD &ir ) {
   AutoMutex mtx( s_Conin.mutex );

   if( s_Conin.CIB_IdxRead == 0 ) { // trying to insert w/no leading gap?
      s_Conin.vINPUT_RECORD.insert( s_Conin.vINPUT_RECORD.begin(), ir );
      enum { INS_RECS = 4 };
      s_Conin.CIB_MaxElements += INS_RECS;
      ReallocArray( s_Conin.paINPUT_RECORD, s_Conin.CIB_MaxElements, "s_Conin.paINPUT_RECORD" );
      MoveArray( &s_Conin.paINPUT_RECORD[INS_RECS], &s_Conin.paINPUT_RECORD[0], s_Conin.CIB_ValidElements );
      s_Conin.CIB_IdxRead = INS_RECS;
      }
   else {
      s_Conin.vINPUT_RECORD[--s_Conin.CIB_IdxRead] = ir;
      }

   s_Conin.paINPUT_RECORD[--s_Conin.CIB_IdxRead] = ir;
   ++s_Conin.CIB_ValidElements;
   }


// This will be used by the Mouse-event handler to simulate keystrokes

STATIC_FXN void InsertKeyUpDownInputEventRecord( PRawWinKeydown pRawWinKeydn ) {
   Win32::INPUT_RECORD inrec;
   inrec.EventType                        = KEY_EVENT;
   inrec.Event.KeyEvent.wVirtualKeyCode   = pRawWinKeydn->wVirtualKeyCode;
   inrec.Event.KeyEvent.uChar.UnicodeChar = pRawWinKeydn->unicodeChar;
   inrec.Event.KeyEvent.dwControlKeyState = pRawWinKeydn->dwControlKeyState;
   inrec.Event.KeyEvent.wRepeatCount      = 0;
   inrec.Event.KeyEvent.wVirtualScanCode  = 0;
   inrec.Event.KeyEvent.bKeyDown          = 0;
   InsertConinRecord( inrec );

   inrec.Event.KeyEvent.bKeyDown          = 1;
   InsertConinRecord( inrec );
   }

STATIC_FXN void InsertKeyUpDownInputEventRecord( U16 wVirtualKeyCode ) {
   RawWinKeydown rwkd;
   rwkd.unicodeChar       = 0;
   rwkd.wVirtualKeyCode   = wVirtualKeyCode;
   rwkd.dwControlKeyState = 0;
   InsertKeyUpDownInputEventRecord( &rwkd );
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

GLOBAL_VAR bool g_fUseMouse;

class TMouseEvent {
   Win32::MOUSE_EVENT_RECORD d_MouseEv;
   Point                     d_mousePosition;

   bool LeftButtonDown()  const { return ToBOOL( d_MouseEv.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED ); }
   bool RightButtonDown() const { return ToBOOL( d_MouseEv.dwButtonState & RIGHTMOST_BUTTON_PRESSED     ); }
   bool InterestingFlags() const {
      return LeftButtonDown() || RightButtonDown() || ToBOOL(d_MouseEv.dwEventFlags & (MOUSE_WHEELED|DOUBLE_CLICK));
      }

   int WheelSpin() const {
      int wheelspin;
      return ((d_MouseEv.dwEventFlags & MOUSE_WHEELED) && 0 != (wheelspin=static_cast<S16>( (d_MouseEv.dwButtonState >> 16) & 0xFFFF)))
             ? wheelspin
             : 0
             ;
      }

   void ReinsertMouseEvent( Point mousePosition ) const;

public:
   TMouseEvent( const Win32::MOUSE_EVENT_RECORD &MouseEv );
   void  Process();
   }; STD_TYPEDEFS( TMouseEvent )


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

   if( !InterestingFlags() )
      return;

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

   if( g_iWindowCount() > 1 && !SwitchToWinContainingPointOk( Point( d_mousePosition.lin-1, d_mousePosition.col-1 ) ) )
      return;

   // was: MouseMovedCmdExec( d_mousePosition.lin - g_CurWin()->UpLeft.lin, d_mousePosition.col - g_CurWin()->UpLeft.col, d_flags );
   // now inlined

   PCWV;
   const auto yLine( d_mousePosition.lin - pcw->d_UpLeft.lin );
   const auto xCol ( d_mousePosition.col - pcw->d_UpLeft.col );

   int wheelspin;
#ifdef SOME_STUFF
   STATIC_VAR bool s_fInserting_EDCMDVK_HELP;
   if( LeftButtonDown() ) {
      if( IsSelectionActive() && RightButtonDown() )
         pCMD_boxstream->BuildExecute();

      const auto yMax( pcw->UpLeft.lin + pcw->Size.lin );

      if( yMax - yLine == -1 ) {
         ReinsertMouseEvent( Point( pcw->UpLeft.lin + yLine - 1, pcw->UpLeft.col + xCol ) );
         InsertKeyUpDownInputEventRecord( EDCMDVK_DOWN );
         }
      else if( yLine-1 < yMax ) {
         pcv->MoveCursor( pcv->Origin().lin + yLine - 1, pcv->Origin().col + xCol - 1 );
         if( IsSelectionActive() )
            ExtendSelectionHilite( pcv->Cursor() );
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
         minPosSpin = SmallerOf( minPosSpin, abs_spin );
         maxPosSpin = LargerOf ( maxPosSpin, abs_spin );
         }
      else {
         minNegSpin = SmallerOf( minNegSpin, abs_spin );
         maxNegSpin = LargerOf ( maxNegSpin, abs_spin );
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
#endif
   }


bool KbHit() { // BUGBUG does this actually WORK? 20081215 kgoodwin NO it doesn't because s_Conin is not being filled unless there were leftovers from the last ReadConsoleInput call
   AutoMutex mtx( s_Conin.mutex );

   if( s_Conin.CIB_IdxRead < s_Conin.CIB_ValidElements ) {
      auto &inre_( s_Conin.vINPUT_RECORD[ s_Conin.CIB_IdxRead ] );
      auto &inrec( s_Conin.paINPUT_RECORD[ s_Conin.CIB_IdxRead ] );
      if( inrec.EventType == KEY_EVENT && inrec.Event.KeyEvent.bKeyDown )
         return true;
      }

   return false;
   }


STATIC_FXN int XlatKeysWhenNumlockOn( U16 theVK ) {
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


// 20070309 kgoodwin added EdKC_EVENT_xxx
//
// "normal" EdKC's are
//
//    executed via a table lookup mechanism that the user is in control of.
//
// EdKC_EVENT's are:
//
//    always executed immediately; their execution does not alter any
//    in-progress user command sequence.
//

#define  EdKC_EVENT_ProgramGotFocus       (EdKC_COUNT+1)
#define  EdKC_EVENT_ProgramLostFocus      (EdKC_COUNT+2)
#define  EdKC_EVENT_ProgramExitRequested  (EdKC_COUNT+3)

STATIC_FXN bool IsInterestingKeyEvent( const Win32::KEY_EVENT_RECORD &KER ) {
   if( !KER.bKeyDown ) // auto-repeat gen's a series of bKeyDown's so these are NOT skipped
      return false;

   switch( KER.wVirtualKeyCode ) {
      case VK_MENU:    return KER.uChar.UnicodeChar != 0;
      case VK_NUMLOCK:
      case VK_CAPITAL:
      case VK_SHIFT:
      case VK_CONTROL: return false;
      default:         return true;
      }
   }

STATIC_FXN void ReadInputEventPrimitive( PEdInputEvent rv ) {
   while( true ) {
      if( EXPERIMENT_HANDLE_CTRL_CLOSE_EVENT HANDLE_CTRL_CLOSE_EVENT( && g_fProcessExitRequested ) ) {
          HANDLE_CTRL_CLOSE_EVENT( g_fProcessExitRequested = false; )
          rv->fIsEdKC_EVENT = true;
          rv->   EdKC_EVENT = EdKC_EVENT_ProgramExitRequested;
          return;
          }

      auto pIR( ReadNextUsefulConsoleInputRecord() );
      if( pIR ) { // pIR is within s_Conin.paINPUT_RECORD[]?
         switch( pIR->EventType ) {
            default:  DBG( "*** InputEvent %d ??? ***", pIR->EventType );  break;

            case KEY_EVENT:
                 if( IsInterestingKeyEvent( pIR->Event.KeyEvent ) ) {
                    rv->fIsEdKC_EVENT            = false;
                    rv->rawkey.unicodeChar       = pIR->Event.KeyEvent.uChar.UnicodeChar;
                    rv->rawkey.wVirtualKeyCode   = pIR->Event.KeyEvent.wVirtualKeyCode;
                    rv->rawkey.dwControlKeyState = pIR->Event.KeyEvent.dwControlKeyState;
                    return;
                    }
                 break;

            case FOCUS_EVENT:
                 rv->fIsEdKC_EVENT = true;
                 rv->   EdKC_EVENT = pIR->Event.FocusEvent.bSetFocus ? EdKC_EVENT_ProgramGotFocus : EdKC_EVENT_ProgramLostFocus;
                 0 && DBG( "*** InputEvent %s ***", pIR->Event.FocusEvent.bSetFocus ? "GotFocus" : "LostFocus" );
                 return;

#if MOUSE_SUPPORT
            case MOUSE_EVENT:
                 0 && DBG( "*** InputEvent Mouse ***" );
                 if( g_fUseMouse )
                    TMouseEvent( pIR->Event.MouseEvent ).Process();

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
   { EdKC_del,      EdKC_a_del,       EdKC_c_del,       EdKC_s_del,      0,                 }, // 0x2E  VK_DELETE               DEL key
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
   U8  Ascii;  // Ascii code
   U8  d_VK;   // VK code
   U8  kFlags; // Flags

   // values for the kFlags field
   //
   enum {
      FLAG_SHIFT   = BIT(0),
      FLAG_NUMLOCK = BIT(1),
      FLAG_ALT     = BIT(2),
      FLAG_CTRL    = BIT(3),
      };

   void RawKeyStr( PChar buf, int bufbytes ) const;
   };

//
// KeyBytesEnum - DEEP INTERNAL struct, used ONLY in LOW-LEVEL interface between Win32 and Editor.
//                NOT to be used within any Editor data structures!!!
//
struct KeyBytesEnum {
   KEY_DATA    k_d;
   EdKC        EdKC_;
   };
typedef KeyBytesEnum *PKeyBytesEnum;

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

STATIC_FXN KeyBytesEnum GetInputEvent() {
   EdInputEvent eie;
   ReadInputEventPrimitive( &eie );
   if( eie.fIsEdKC_EVENT ) {
      KeyBytesEnum rv;
      rv.k_d.Ascii  = '\0';
      rv.k_d.d_VK   = 0;
      rv.k_d.kFlags = 0;
      rv.EdKC_      = eie.EdKC_EVENT;  0 && DBG( "EdKC_+ %04X", rv.EdKC_ );
      return rv;
      }

   const auto &rwkd( eie.rawkey );
   auto edKC(0);
   auto allShifts(0);;
   const U8 valAscii( 0xFF & rwkd.unicodeChar );
   const U8 valVK(    0xFF & rwkd.wVirtualKeyCode );  0 && DBG( "VK+ %02X", valVK );
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
      if( anyCtrl && anyShift ) {                            effectiveShiftIdx = shiftCTRL_SHIFT; }

      //*********  translate VK into EdKC  *********

      if( numlockon ) {
         allShifts |= KEY_DATA::FLAG_NUMLOCK;

         const auto numlockVK( XlatKeysWhenNumlockOn( valVK ) );

         if( numlockVK >= 0 )  edKC = VK_shift_to_EdKC( numLockXlatTbl, numlockVK, effectiveShiftIdx );
         else                  edKC = VK_shift_to_EdKC( normalXlatTbl , valVK    , effectiveShiftIdx );
         }
      else                     edKC = VK_shift_to_EdKC( normalXlatTbl , valVK    , effectiveShiftIdx );
      }
#pragma GCC diagnostic pop

   if( edKC == 0 )
       edKC = valAscii;

  #if SEL_KEYMAP
   if( SEL_KEYMAP && SelKeymapEnabled() ) { 0 && DBG( "-> %03X", edKC );
      if( edKC >= EdKC_a && edKC <= EdKC_z ) { // map to EdKC_sela..EdKC_selz
         edKC += EdKC_sela - EdKC_a;  0 && DBG( "-> EdKC_sela" );
         }
      if( edKC >= EdKC_A && edKC <= EdKC_Z ) { // map to EdKC_selA..EdKC_selZ
         edKC += EdKC_selA - EdKC_A;  0 && DBG( "-> EdKC_selA" );
         }
      }
  #endif

   KeyBytesEnum rv;
   rv.k_d.Ascii  = valAscii;
   rv.k_d.d_VK   = valVK;
   rv.k_d.kFlags = allShifts;
   rv.EdKC_      = edKC;
   return rv;
   }


void KEY_DATA::RawKeyStr( PChar buf, int bufbytes ) const {
   safeSprintf( buf, bufbytes, "VK=x%02X, Flags=%s%s%s%s, ch=0x%02X (%c)"
      ,  d_VK
      , (kFlags & FLAG_CTRL   ) ? "C" : ""
      , (kFlags & FLAG_ALT    ) ? "A" : ""
      , (kFlags & FLAG_SHIFT  ) ? "S" : ""
      , (kFlags & FLAG_NUMLOCK) ? "N" : ""
      ,  Ascii
      ,  Ascii
      );
   }


STATIC_FXN KeyBytesEnum GetKeyBytesEnum( bool fFreezeOtherThreads ) { // PRIMARY API for reading a key
   while( true ) {
      const auto fPassThreadBaton( !(fFreezeOtherThreads || KbHit()) );
      if( fPassThreadBaton )  MainThreadGiveUpGlobalVariableLock();
      0 && DBG( "%s %s", __func__, fPassThreadBaton ? "passing" : "holding" );

      const KeyBytesEnum rv( GetInputEvent() );

      if( fPassThreadBaton )  MainThreadWaitForGlobalVariableLock();

      if( rv.EdKC_ < EdKC_COUNT ) {
         if( rv.EdKC_ == 0 ) { // (rv.EdKC_ == 0) corresponds to keys we currently do not decode
            char ktmp[55];  rv.k_d.RawKeyStr( BSOB(ktmp) );  0 && DBG( "%s(rv.EdKC==0): %s", __func__, ktmp );
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


void WaitForKey() {
   GetKeyBytesEnum( true );
   }

STIL CmdData KeyBytesEnum2CmdData( KeyBytesEnum ki ) {
   CmdData rv;
   rv.EdKcEnum = ki.EdKC_;
   rv.Ascii    = ki.k_d.Ascii;
   return rv;
   }

CmdData CmdDataFromNextKey() {
   return KeyBytesEnum2CmdData( GetKeyBytesEnum( false ) );
   }

CmdData CmdDataFromNextKey_Keystr( PChar pKeyStringBuffer, size_t pKeyStringBufferBytes ) {
   const auto ki( GetKeyBytesEnum( true ) );
   if( ki.EdKC_ == 0 )
      ki.k_d.RawKeyStr( pKeyStringBuffer, pKeyStringBufferBytes );
   else
      StrFromEdkc( pKeyStringBuffer, pKeyStringBufferBytes, ki.EdKC_ );

   return KeyBytesEnum2CmdData( ki );
   }


struct TKyCd2KyNameTbl {
   EdKC   EdKC_;
   PCChar name;
   };

// typedef const TKyCd2KyNameTbl *PKKyCd2KyNameTbl;

STATIC_CONST TKyCd2KyNameTbl KyCd2KyNameTbl[] = {
#if SEL_KEYMAP
      { EdKC_sela            , "sel_a"          },
      { EdKC_selb            , "sel_b"          },
      { EdKC_selc            , "sel_c"          },
      { EdKC_seld            , "sel_d"          },
      { EdKC_sele            , "sel_e"          },
      { EdKC_self            , "sel_f"          },
      { EdKC_selg            , "sel_g"          },
      { EdKC_selh            , "sel_h"          },
      { EdKC_seli            , "sel_i"          },
      { EdKC_selj            , "sel_j"          },
      { EdKC_selk            , "sel_k"          },
      { EdKC_sell            , "sel_l"          },
      { EdKC_selm            , "sel_m"          },
      { EdKC_seln            , "sel_n"          },
      { EdKC_selo            , "sel_o"          },
      { EdKC_selp            , "sel_p"          },
      { EdKC_selq            , "sel_q"          },
      { EdKC_selr            , "sel_r"          },
      { EdKC_sels            , "sel_s"          },
      { EdKC_selt            , "sel_t"          },
      { EdKC_selu            , "sel_u"          },
      { EdKC_selv            , "sel_v"          },
      { EdKC_selw            , "sel_w"          },
      { EdKC_selx            , "sel_x"          },
      { EdKC_sely            , "sel_y"          },
      { EdKC_selz            , "sel_z"          },
      { EdKC_selA            , "shift+sel_a"    },
      { EdKC_selB            , "shift+sel_b"    },
      { EdKC_selC            , "shift+sel_c"    },
      { EdKC_selD            , "shift+sel_d"    },
      { EdKC_selE            , "shift+sel_e"    },
      { EdKC_selF            , "shift+sel_f"    },
      { EdKC_selG            , "shift+sel_g"    },
      { EdKC_selH            , "shift+sel_h"    },
      { EdKC_selI            , "shift+sel_i"    },
      { EdKC_selJ            , "shift+sel_j"    },
      { EdKC_selK            , "shift+sel_k"    },
      { EdKC_selL            , "shift+sel_l"    },
      { EdKC_selM            , "shift+sel_m"    },
      { EdKC_selN            , "shift+sel_n"    },
      { EdKC_selO            , "shift+sel_o"    },
      { EdKC_selP            , "shift+sel_p"    },
      { EdKC_selQ            , "shift+sel_q"    },
      { EdKC_selR            , "shift+sel_r"    },
      { EdKC_selS            , "shift+sel_s"    },
      { EdKC_selT            , "shift+sel_t"    },
      { EdKC_selU            , "shift+sel_u"    },
      { EdKC_selV            , "shift+sel_v"    },
      { EdKC_selW            , "shift+sel_w"    },
      { EdKC_selX            , "shift+sel_x"    },
      { EdKC_selY            , "shift+sel_y"    },
      { EdKC_selZ            , "shift+sel_z"    },
#endif // SEL_KEYMAP
      { EdKC_f1              , "f1"             },
      { EdKC_f2              , "f2"             },
      { EdKC_f3              , "f3"             },
      { EdKC_f4              , "f4"             },
      { EdKC_f5              , "f5"             },
      { EdKC_f6              , "f6"             },
      { EdKC_f7              , "f7"             },
      { EdKC_f8              , "f8"             },
      { EdKC_f9              , "f9"             },
      { EdKC_f10             , "f10"            },
      { EdKC_f11             , "f11"            },
      { EdKC_f12             , "f12"            },
      { EdKC_home            , "home"           },
      { EdKC_end             , "end"            },
      { EdKC_left            , "left"           },
      { EdKC_right           , "right"          },
      { EdKC_up              , "up"             },
      { EdKC_down            , "down"           },
      { EdKC_pgup            , "pgup"           },
      { EdKC_pgdn            , "pgdn"           },
      { EdKC_ins             , "ins"            },
      { EdKC_del             , "del"            },
      { EdKC_center          , "center"         },
      { EdKC_num0            , "num+0"          },
      { EdKC_num1            , "num+1"          },
      { EdKC_num2            , "num+2"          },
      { EdKC_num3            , "num+3"          },
      { EdKC_num4            , "num+4"          },
      { EdKC_num5            , "num+5"          },
      { EdKC_num6            , "num+6"          },
      { EdKC_num7            , "num+7"          },
      { EdKC_num8            , "num+8"          },
      { EdKC_num9            , "num+9"          },
      { EdKC_numMinus        , "num-"           },
      { EdKC_numPlus         , "num+"           },
      { EdKC_numStar         , "num*"           },
      { EdKC_numSlash        , "num/"           },
      { EdKC_numEnter        , "num+enter"      },
      { EdKC_space           , "space"          },
      { EdKC_bksp            , "bksp"           },
      { EdKC_tab             , "tab"            },
      { EdKC_esc             , "esc"            },
      { EdKC_enter           , "enter"          },
      { EdKC_a_0             , "alt+0"          },
      { EdKC_a_1             , "alt+1"          },
      { EdKC_a_2             , "alt+2"          },
      { EdKC_a_3             , "alt+3"          },
      { EdKC_a_4             , "alt+4"          },
      { EdKC_a_5             , "alt+5"          },
      { EdKC_a_6             , "alt+6"          },
      { EdKC_a_7             , "alt+7"          },
      { EdKC_a_8             , "alt+8"          },
      { EdKC_a_9             , "alt+9"          },
      { EdKC_a_a             , "alt+a"          },
      { EdKC_a_b             , "alt+b"          },
      { EdKC_a_c             , "alt+c"          },
      { EdKC_a_d             , "alt+d"          },
      { EdKC_a_e             , "alt+e"          },
      { EdKC_a_f             , "alt+f"          },
      { EdKC_a_g             , "alt+g"          },
      { EdKC_a_h             , "alt+h"          },
      { EdKC_a_i             , "alt+i"          },
      { EdKC_a_j             , "alt+j"          },
      { EdKC_a_k             , "alt+k"          },
      { EdKC_a_l             , "alt+l"          },
      { EdKC_a_m             , "alt+m"          },
      { EdKC_a_n             , "alt+n"          },
      { EdKC_a_o             , "alt+o"          },
      { EdKC_a_p             , "alt+p"          },
      { EdKC_a_q             , "alt+q"          },
      { EdKC_a_r             , "alt+r"          },
      { EdKC_a_s             , "alt+s"          },
      { EdKC_a_t             , "alt+t"          },
      { EdKC_a_u             , "alt+u"          },
      { EdKC_a_v             , "alt+v"          },
      { EdKC_a_w             , "alt+w"          },
      { EdKC_a_x             , "alt+x"          },
      { EdKC_a_y             , "alt+y"          },
      { EdKC_a_z             , "alt+z"          },
      { EdKC_a_f1            , "alt+f1"         },
      { EdKC_a_f2            , "alt+f2"         },
      { EdKC_a_f3            , "alt+f3"         },
      { EdKC_a_f4            , "alt+f4"         },
      { EdKC_a_f5            , "alt+f5"         },
      { EdKC_a_f6            , "alt+f6"         },
      { EdKC_a_f7            , "alt+f7"         },
      { EdKC_a_f8            , "alt+f8"         },
      { EdKC_a_f9            , "alt+f9"         },
      { EdKC_a_f10           , "alt+f10"        },
      { EdKC_a_f11           , "alt+f11"        },
      { EdKC_a_f12           , "alt+f12"        },
      { EdKC_a_BACKTICK      , "alt+`"          },
      { EdKC_a_MINUS         , "alt+-"          },
      { EdKC_a_EQUAL         , "alt+="          },
      { EdKC_a_LEFT_SQ       , "alt+["          },
      { EdKC_a_RIGHT_SQ      , "alt+]"          },
      { EdKC_a_BACKSLASH     , "alt+\\"         },
      { EdKC_a_SEMICOLON     , "alt+;"          },
      { EdKC_a_TICK          , "alt+'"          },
      { EdKC_a_COMMA         , "alt+,"          },
      { EdKC_a_DOT           , "alt+."          },
      { EdKC_a_SLASH         , "alt+/"          },
      { EdKC_a_home          , "alt+home"       },
      { EdKC_a_end           , "alt+end"        },
      { EdKC_a_left          , "alt+left"       },
      { EdKC_a_right         , "alt+right"      },
      { EdKC_a_up            , "alt+up"         },
      { EdKC_a_down          , "alt+down"       },
      { EdKC_a_pgup          , "alt+pgup"       },
      { EdKC_a_pgdn          , "alt+pgdn"       },
      { EdKC_a_ins           , "alt+ins"        },
      { EdKC_a_del           , "alt+del"        },
      { EdKC_a_center        , "alt+center"     },
      { EdKC_a_num0          , "alt+num0"       },
      { EdKC_a_num1          , "alt+num1"       },
      { EdKC_a_num2          , "alt+num2"       },
      { EdKC_a_num3          , "alt+num3"       },
      { EdKC_a_num4          , "alt+num4"       },
      { EdKC_a_num5          , "alt+num5"       },
      { EdKC_a_num6          , "alt+num6"       },
      { EdKC_a_num7          , "alt+num7"       },
      { EdKC_a_num8          , "alt+num8"       },
      { EdKC_a_num9          , "alt+num9"       },
      { EdKC_a_numMinus      , "alt+num-"       },
      { EdKC_a_numPlus       , "alt+num+"       },
      { EdKC_a_numStar       , "alt+num*"       },
      { EdKC_a_numSlash      , "alt+num/"       },
      { EdKC_a_numEnter      , "alt+numenter"   },
      { EdKC_a_space         , "alt+space"      },
      { EdKC_a_bksp          , "alt+bksp"       },
      { EdKC_a_tab           , "alt+tab"        },
      { EdKC_a_esc           , "alt+esc"        },
      { EdKC_a_enter         , "alt+enter"      },
      { EdKC_c_0             , "ctrl+0"         },
      { EdKC_c_1             , "ctrl+1"         },
      { EdKC_c_2             , "ctrl+2"         },
      { EdKC_c_3             , "ctrl+3"         },
      { EdKC_c_4             , "ctrl+4"         },
      { EdKC_c_5             , "ctrl+5"         },
      { EdKC_c_6             , "ctrl+6"         },
      { EdKC_c_7             , "ctrl+7"         },
      { EdKC_c_8             , "ctrl+8"         },
      { EdKC_c_9             , "ctrl+9"         },
      { EdKC_c_a             , "ctrl+a"         },
      { EdKC_c_b             , "ctrl+b"         },
      { EdKC_c_c             , "ctrl+c"         },
      { EdKC_c_d             , "ctrl+d"         },
      { EdKC_c_e             , "ctrl+e"         },
      { EdKC_c_f             , "ctrl+f"         },
      { EdKC_c_g             , "ctrl+g"         },
      { EdKC_c_h             , "ctrl+h"         },
      { EdKC_c_i             , "ctrl+i"         },
      { EdKC_c_j             , "ctrl+j"         },
      { EdKC_c_k             , "ctrl+k"         },
      { EdKC_c_l             , "ctrl+l"         },
      { EdKC_c_m             , "ctrl+m"         },
      { EdKC_c_n             , "ctrl+n"         },
      { EdKC_c_o             , "ctrl+o"         },
      { EdKC_c_p             , "ctrl+p"         },
      { EdKC_c_q             , "ctrl+q"         },
      { EdKC_c_r             , "ctrl+r"         },
      { EdKC_c_s             , "ctrl+s"         },
      { EdKC_c_t             , "ctrl+t"         },
      { EdKC_c_u             , "ctrl+u"         },
      { EdKC_c_v             , "ctrl+v"         },
      { EdKC_c_w             , "ctrl+w"         },
      { EdKC_c_x             , "ctrl+x"         },
      { EdKC_c_y             , "ctrl+y"         },
      { EdKC_c_z             , "ctrl+z"         },
      { EdKC_c_f1            , "ctrl+f1"        },
      { EdKC_c_f2            , "ctrl+f2"        },
      { EdKC_c_f3            , "ctrl+f3"        },
      { EdKC_c_f4            , "ctrl+f4"        },
      { EdKC_c_f5            , "ctrl+f5"        },
      { EdKC_c_f6            , "ctrl+f6"        },
      { EdKC_c_f7            , "ctrl+f7"        },
      { EdKC_c_f8            , "ctrl+f8"        },
      { EdKC_c_f9            , "ctrl+f9"        },
      { EdKC_c_f10           , "ctrl+f10"       },
      { EdKC_c_f11           , "ctrl+f11"       },
      { EdKC_c_f12           , "ctrl+f12"       },
      { EdKC_c_BACKTICK      , "ctrl+`"         },
      { EdKC_c_MINUS         , "ctrl+-"         },
      { EdKC_c_EQUAL         , "ctrl+="         },
      { EdKC_c_LEFT_SQ       , "ctrl+["         },
      { EdKC_c_RIGHT_SQ      , "ctrl+]"         },
      { EdKC_c_BACKSLASH     , "ctrl+\\"        },
      { EdKC_c_SEMICOLON     , "ctrl+;"         },
      { EdKC_c_TICK          , "ctrl+'"         },
      { EdKC_c_COMMA         , "ctrl+,"         },
      { EdKC_c_DOT           , "ctrl+."         },
      { EdKC_c_SLASH         , "ctrl+/"         },
      { EdKC_c_home          , "ctrl+home"      },
      { EdKC_c_end           , "ctrl+end"       },
      { EdKC_c_left          , "ctrl+left"      },
      { EdKC_c_right         , "ctrl+right"     },
      { EdKC_c_up            , "ctrl+up"        },
      { EdKC_c_down          , "ctrl+down"      },
      { EdKC_c_pgup          , "ctrl+pgup"      },
      { EdKC_c_pgdn          , "ctrl+pgdn"      },
      { EdKC_c_ins           , "ctrl+ins"       },
      { EdKC_c_del           , "ctrl+del"       },
      { EdKC_c_center        , "ctrl+center"    },
      { EdKC_c_num0          , "ctrl+num0"      },
      { EdKC_c_num1          , "ctrl+num1"      },
      { EdKC_c_num2          , "ctrl+num2"      },
      { EdKC_c_num3          , "ctrl+num3"      },
      { EdKC_c_num4          , "ctrl+num4"      },
      { EdKC_c_num5          , "ctrl+num5"      },
      { EdKC_c_num6          , "ctrl+num6"      },
      { EdKC_c_num7          , "ctrl+num7"      },
      { EdKC_c_num8          , "ctrl+num8"      },
      { EdKC_c_num9          , "ctrl+num9"      },
      { EdKC_c_numMinus      , "ctrl+num-"      },
      { EdKC_c_numPlus       , "ctrl+num+"      },
      { EdKC_c_numStar       , "ctrl+num*"      },
      { EdKC_c_numSlash      , "ctrl+num/"      },
      { EdKC_c_numEnter      , "ctrl+numenter"  },
      { EdKC_c_space         , "ctrl+space"     },
      { EdKC_c_bksp          , "ctrl+bksp"      },
      { EdKC_c_tab           , "ctrl+tab"       },
      { EdKC_c_esc           , "ctrl+esc"       },
      { EdKC_c_enter         , "ctrl+enter"     },
      { EdKC_s_f1            , "shift+f1"       },
      { EdKC_s_f2            , "shift+f2"       },
      { EdKC_s_f3            , "shift+f3"       },
      { EdKC_s_f4            , "shift+f4"       },
      { EdKC_s_f5            , "shift+f5"       },
      { EdKC_s_f6            , "shift+f6"       },
      { EdKC_s_f7            , "shift+f7"       },
      { EdKC_s_f8            , "shift+f8"       },
      { EdKC_s_f9            , "shift+f9"       },
      { EdKC_s_f10           , "shift+f10"      },
      { EdKC_s_f11           , "shift+f11"      },
      { EdKC_s_f12           , "shift+f12"      },
      { EdKC_s_home          , "shift+home"     },
      { EdKC_s_end           , "shift+end"      },
      { EdKC_s_left          , "shift+left"     },
      { EdKC_s_right         , "shift+right"    },
      { EdKC_s_up            , "shift+up"       },
      { EdKC_s_down          , "shift+down"     },
      { EdKC_s_pgup          , "shift+pgup"     },
      { EdKC_s_pgdn          , "shift+pgdn"     },
      { EdKC_s_ins           , "shift+ins"      },
      { EdKC_s_del           , "shift+del"      },
      { EdKC_s_center        , "shift+center"   },
      { EdKC_s_num0          , "shift+num0"     },
      { EdKC_s_num1          , "shift+num1"     },
      { EdKC_s_num2          , "shift+num2"     },
      { EdKC_s_num3          , "shift+num3"     },
      { EdKC_s_num4          , "shift+num4"     },
      { EdKC_s_num5          , "shift+num5"     },
      { EdKC_s_num6          , "shift+num6"     },
      { EdKC_s_num7          , "shift+num7"     },
      { EdKC_s_num8          , "shift+num8"     },
      { EdKC_s_num9          , "shift+num9"     },
      { EdKC_s_numMinus      , "shift+num-"     },
      { EdKC_s_numPlus       , "shift+num+"     },
      { EdKC_s_numStar       , "shift+num*"     },
      { EdKC_s_numSlash      , "shift+num/"     },
      { EdKC_s_numEnter      , "shift+numenter" },
      { EdKC_s_space         , "shift+space"    },
      { EdKC_s_bksp          , "shift+bksp"     },
      { EdKC_s_tab           , "shift+tab"      },
      { EdKC_s_esc           , "shift+esc"      },
      { EdKC_s_enter         , "shift+enter"    },

      { EdKC_cs_bksp         , "ctrl+shift+bksp"     },
      { EdKC_cs_tab          , "ctrl+shift+tab"      },
      { EdKC_cs_center       , "ctrl+shift+center"   },
      { EdKC_cs_enter        , "ctrl+shift+enter"    },
      { EdKC_cs_pause        , "ctrl+shift+pause"    },
      { EdKC_cs_space        , "ctrl+shift+space"    },
      { EdKC_cs_pgup         , "ctrl+shift+pgup"     },
      { EdKC_cs_pgdn         , "ctrl+shift+pgdn"     },
      { EdKC_cs_end          , "ctrl+shift+end"      },
      { EdKC_cs_home         , "ctrl+shift+home"     },
      { EdKC_cs_left         , "ctrl+shift+left"     },
      { EdKC_cs_up           , "ctrl+shift+up"       },
      { EdKC_cs_right        , "ctrl+shift+right"    },
      { EdKC_cs_down         , "ctrl+shift+down"     },
      { EdKC_cs_ins          , "ctrl+shift+ins"      },
      { EdKC_cs_0            , "ctrl+shift+0"        },
      { EdKC_cs_1            , "ctrl+shift+1"        },
      { EdKC_cs_2            , "ctrl+shift+2"        },
      { EdKC_cs_3            , "ctrl+shift+3"        },
      { EdKC_cs_4            , "ctrl+shift+4"        },
      { EdKC_cs_5            , "ctrl+shift+5"        },
      { EdKC_cs_6            , "ctrl+shift+6"        },
      { EdKC_cs_7            , "ctrl+shift+7"        },
      { EdKC_cs_8            , "ctrl+shift+8"        },
      { EdKC_cs_9            , "ctrl+shift+9"        },
      { EdKC_cs_a            , "ctrl+shift+a"        },
      { EdKC_cs_b            , "ctrl+shift+b"        },
      { EdKC_cs_c            , "ctrl+shift+c"        },
      { EdKC_cs_d            , "ctrl+shift+d"        },
      { EdKC_cs_e            , "ctrl+shift+e"        },
      { EdKC_cs_f            , "ctrl+shift+f"        },
      { EdKC_cs_g            , "ctrl+shift+g"        },
      { EdKC_cs_h            , "ctrl+shift+h"        },
      { EdKC_cs_i            , "ctrl+shift+i"        },
      { EdKC_cs_j            , "ctrl+shift+j"        },
      { EdKC_cs_k            , "ctrl+shift+k"        },
      { EdKC_cs_l            , "ctrl+shift+l"        },
      { EdKC_cs_m            , "ctrl+shift+m"        },
      { EdKC_cs_n            , "ctrl+shift+n"        },
      { EdKC_cs_o            , "ctrl+shift+o"        },
      { EdKC_cs_p            , "ctrl+shift+p"        },
      { EdKC_cs_q            , "ctrl+shift+q"        },
      { EdKC_cs_r            , "ctrl+shift+r"        },
      { EdKC_cs_s            , "ctrl+shift+s"        },
      { EdKC_cs_t            , "ctrl+shift+t"        },
      { EdKC_cs_u            , "ctrl+shift+u"        },
      { EdKC_cs_v            , "ctrl+shift+v"        },
      { EdKC_cs_w            , "ctrl+shift+w"        },
      { EdKC_cs_x            , "ctrl+shift+x"        },
      { EdKC_cs_y            , "ctrl+shift+y"        },
      { EdKC_cs_z            , "ctrl+shift+z"        },
      { EdKC_cs_numStar      , "ctrl+shift+num*"     },
      { EdKC_cs_numPlus      , "ctrl+shift+num+"     },
      { EdKC_cs_numEnter     , "ctrl+shift+numenter" },
      { EdKC_cs_numMinus     , "ctrl+shift+num-"     },
      { EdKC_cs_numSlash     , "ctrl+shift+num/"     },
      { EdKC_cs_f1           , "ctrl+shift+f1"       },
      { EdKC_cs_f2           , "ctrl+shift+f2"       },
      { EdKC_cs_f3           , "ctrl+shift+f3"       },
      { EdKC_cs_f4           , "ctrl+shift+f4"       },
      { EdKC_cs_f5           , "ctrl+shift+f5"       },
      { EdKC_cs_f6           , "ctrl+shift+f6"       },
      { EdKC_cs_f7           , "ctrl+shift+f7"       },
      { EdKC_cs_f8           , "ctrl+shift+f8"       },
      { EdKC_cs_f9           , "ctrl+shift+f9"       },
      { EdKC_cs_f10          , "ctrl+shift+f10"      },
      { EdKC_cs_f11          , "ctrl+shift+f11"      },
      { EdKC_cs_f12          , "ctrl+shift+f12"      },
      { EdKC_cs_scroll       , "ctrl+shift+scroll"   },
      { EdKC_cs_SEMICOLON    , "ctrl+shift+;"        },
      { EdKC_cs_EQUAL        , "ctrl+shift+="        },
      { EdKC_cs_COMMA        , "ctrl+shift+,"        },
      { EdKC_cs_MINUS        , "ctrl+shift+-"        },
      { EdKC_cs_DOT          , "ctrl+shift+."        },
      { EdKC_cs_SLASH        , "ctrl+shift+/"        },
      { EdKC_cs_BACKTICK     , "ctrl+shift+`"        },
      { EdKC_cs_TICK         , "ctrl+shift+'"        },
      { EdKC_cs_LEFT_SQ      , "ctrl+shift+["        },
      { EdKC_cs_BACKSLASH    , "ctrl+shift+\\"       },
      { EdKC_cs_RIGHT_SQ     , "ctrl+shift+]"        },
      { EdKC_cs_num0         , "ctrl+shift+num0"     },
      { EdKC_cs_num1         , "ctrl+shift+num1"     },
      { EdKC_cs_num2         , "ctrl+shift+num2"     },
      { EdKC_cs_num3         , "ctrl+shift+num3"     },
      { EdKC_cs_num4         , "ctrl+shift+num4"     },
      { EdKC_cs_num5         , "ctrl+shift+num5"     },
      { EdKC_cs_num6         , "ctrl+shift+num6"     },
      { EdKC_cs_num7         , "ctrl+shift+num7"     },
      { EdKC_cs_num8         , "ctrl+shift+num8"     },
      { EdKC_cs_num9         , "ctrl+shift+num9"     },

      { EdKC_pause           , "pause"               },
      { EdKC_a_pause         , "alt+pause"           },
      { EdKC_s_pause         , "shift+pause"         },
      { EdKC_c_numlk         , "ctrl+numlk"          },
      { EdKC_scroll          , "scroll"              },
      { EdKC_a_scroll        , "alt+scroll"          },
      { EdKC_s_scroll        , "shift+scroll"        },
   };

static_assert( ELEMENTS( KyCd2KyNameTbl ) == (EdKC_COUNT - 256), "KyCd2KyNameTbl ) == (EdKC_COUNT - 256)" );


#ifdef GCC
#define  IDX_EQ( idx_val )  [ idx_val ] =
#else
#define  IDX_EQ( idx_val )
#endif


// nearly identical declarations:
#if 1
GLOBAL_VAR PCMD     g_Key2CmdTbl[] =         // use this so assert @ end of initializer will work
#else
GLOBAL_VAR AKey2Cmd g_Key2CmdTbl   =         // use this to prove it (still) works
#endif
   {
   // first 256 [0x00..0xFF] are for ASCII codes
   #define  gfc   pCMD_graphic
   // 0..3
     gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 4..7
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 8..11
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   // 12..15
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc
   , gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc, gfc

   #undef  gfc

#if SEL_KEYMAP
   , IDX_EQ( EdKC_sela         )  pCMD_graphic   // a
   , IDX_EQ( EdKC_selb         )  pCMD_graphic   // b
   , IDX_EQ( EdKC_selc         )  pCMD_graphic   // c
   , IDX_EQ( EdKC_seld         )  pCMD_graphic   // d
   , IDX_EQ( EdKC_sele         )  pCMD_graphic   // e
   , IDX_EQ( EdKC_self         )  pCMD_graphic   // f
   , IDX_EQ( EdKC_selg         )  pCMD_graphic   // g
   , IDX_EQ( EdKC_selh         )  pCMD_left      // h (left)
   , IDX_EQ( EdKC_seli         )  pCMD_graphic   // i
   , IDX_EQ( EdKC_selj         )  pCMD_down      // j (down)
   , IDX_EQ( EdKC_selk         )  pCMD_up        // k (up)
   , IDX_EQ( EdKC_sell         )  pCMD_right     // l (right)
   , IDX_EQ( EdKC_selm         )  pCMD_graphic   // m
   , IDX_EQ( EdKC_seln         )  pCMD_graphic   // n
   , IDX_EQ( EdKC_selo         )  pCMD_graphic   // o
   , IDX_EQ( EdKC_selp         )  pCMD_graphic   // p
   , IDX_EQ( EdKC_selq         )  pCMD_graphic   // q
   , IDX_EQ( EdKC_selr         )  pCMD_graphic   // r
   , IDX_EQ( EdKC_sels         )  pCMD_graphic   // s
   , IDX_EQ( EdKC_selt         )  pCMD_graphic   // t
   , IDX_EQ( EdKC_selu         )  pCMD_graphic   // u
   , IDX_EQ( EdKC_selv         )  pCMD_graphic   // v
   , IDX_EQ( EdKC_selw         )  pCMD_graphic   // w
   , IDX_EQ( EdKC_selx         )  pCMD_graphic   // x
   , IDX_EQ( EdKC_sely         )  pCMD_graphic   // y
   , IDX_EQ( EdKC_selz         )  pCMD_graphic   // z
   , IDX_EQ( EdKC_selA         )  pCMD_graphic   // A
   , IDX_EQ( EdKC_selB         )  pCMD_graphic   // B
   , IDX_EQ( EdKC_selC         )  pCMD_graphic   // C
   , IDX_EQ( EdKC_selD         )  pCMD_graphic   // D
   , IDX_EQ( EdKC_selE         )  pCMD_graphic   // E
   , IDX_EQ( EdKC_selF         )  pCMD_graphic   // F
   , IDX_EQ( EdKC_selG         )  pCMD_graphic   // G
   , IDX_EQ( EdKC_selH         )  pCMD_graphic   // H
   , IDX_EQ( EdKC_selI         )  pCMD_graphic   // I
   , IDX_EQ( EdKC_selJ         )  pCMD_graphic   // J
   , IDX_EQ( EdKC_selK         )  pCMD_graphic   // K
   , IDX_EQ( EdKC_selL         )  pCMD_graphic   // L
   , IDX_EQ( EdKC_selM         )  pCMD_graphic   // M
   , IDX_EQ( EdKC_selN         )  pCMD_graphic   // N
   , IDX_EQ( EdKC_selO         )  pCMD_graphic   // O
   , IDX_EQ( EdKC_selP         )  pCMD_graphic   // P
   , IDX_EQ( EdKC_selQ         )  pCMD_graphic   // Q
   , IDX_EQ( EdKC_selR         )  pCMD_graphic   // R
   , IDX_EQ( EdKC_selS         )  pCMD_graphic   // S
   , IDX_EQ( EdKC_selT         )  pCMD_graphic   // T
   , IDX_EQ( EdKC_selU         )  pCMD_graphic   // U
   , IDX_EQ( EdKC_selV         )  pCMD_graphic   // V
   , IDX_EQ( EdKC_selW         )  pCMD_graphic   // W
   , IDX_EQ( EdKC_selX         )  pCMD_graphic   // X
   , IDX_EQ( EdKC_selY         )  pCMD_graphic   // Y
   , IDX_EQ( EdKC_selZ         )  pCMD_graphic   // Z
#endif // SEL_KEYMAP

   , IDX_EQ( EdKC_f1           )  pCMD_unassigned
   , IDX_EQ( EdKC_f2           )  pCMD_setfile
   , IDX_EQ( EdKC_f3           )  pCMD_psearch
   , IDX_EQ( EdKC_f4           )  pCMD_msearch
   , IDX_EQ( EdKC_f5           )  pCMD_unassigned
   , IDX_EQ( EdKC_f6           )  pCMD_window
   , IDX_EQ( EdKC_f7           )  pCMD_execute
   , IDX_EQ( EdKC_f8           )  pCMD_unassigned
   , IDX_EQ( EdKC_f9           )  pCMD_meta
   , IDX_EQ( EdKC_f10          )  pCMD_arg            //            LAPTOP
   , IDX_EQ( EdKC_f11          )  pCMD_mfreplace
   , IDX_EQ( EdKC_f12          )  pCMD_unassigned
   , IDX_EQ( EdKC_home         )  pCMD_begline
   , IDX_EQ( EdKC_end          )  pCMD_endline
   , IDX_EQ( EdKC_left         )  pCMD_left
   , IDX_EQ( EdKC_right        )  pCMD_right
   , IDX_EQ( EdKC_up           )  pCMD_up
   , IDX_EQ( EdKC_down         )  pCMD_down
   , IDX_EQ( EdKC_pgup         )  pCMD_mpage
   , IDX_EQ( EdKC_pgdn         )  pCMD_ppage
   , IDX_EQ( EdKC_ins          )  pCMD_paste
   , IDX_EQ( EdKC_del          )  pCMD_delete
   , IDX_EQ( EdKC_center       )  pCMD_arg
   , IDX_EQ( EdKC_num0         )  pCMD_graphic
   , IDX_EQ( EdKC_num1         )  pCMD_graphic
   , IDX_EQ( EdKC_num2         )  pCMD_graphic
   , IDX_EQ( EdKC_num3         )  pCMD_graphic
   , IDX_EQ( EdKC_num4         )  pCMD_graphic
   , IDX_EQ( EdKC_num5         )  pCMD_graphic
   , IDX_EQ( EdKC_num6         )  pCMD_graphic
   , IDX_EQ( EdKC_num7         )  pCMD_graphic
   , IDX_EQ( EdKC_num8         )  pCMD_graphic
   , IDX_EQ( EdKC_num9         )  pCMD_graphic
   , IDX_EQ( EdKC_numMinus     )  pCMD_udelete
   , IDX_EQ( EdKC_numPlus      )  pCMD_copy
   , IDX_EQ( EdKC_numStar      )  pCMD_graphic
   , IDX_EQ( EdKC_numSlash     )  pCMD_graphic
   , IDX_EQ( EdKC_numEnter     )  pCMD_emacsnewl
   , IDX_EQ( EdKC_space        )  pCMD_unassigned
   , IDX_EQ( EdKC_bksp         )  pCMD_emacscdel
   , IDX_EQ( EdKC_tab          )  pCMD_tab
   , IDX_EQ( EdKC_esc          )  pCMD_cancel
   , IDX_EQ( EdKC_enter        )  pCMD_emacsnewl
   , IDX_EQ( EdKC_a_0          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_1          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_2          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_3          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_4          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_5          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_6          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_7          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_8          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_9          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_a          )  pCMD_arg
   , IDX_EQ( EdKC_a_b          )  pCMD_boxstream
   , IDX_EQ( EdKC_a_c          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_d          )  pCMD_lasttext
   , IDX_EQ( EdKC_a_e          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_g          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_h          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_i          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_j          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_k          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_l          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_m          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_n          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_o          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_p          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_q          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_r          )  pCMD_unassigned     //        CMD_record
   , IDX_EQ( EdKC_a_s          )  pCMD_lastselect
   , IDX_EQ( EdKC_a_t          )  pCMD_tell
   , IDX_EQ( EdKC_a_u          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_v          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_w          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_x          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_y          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_z          )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f2         )  pCMD_files          //       CMD_print
   , IDX_EQ( EdKC_a_f3         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f4         )  pCMD_exit
   , IDX_EQ( EdKC_a_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f8         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f10        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_BACKTICK   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_MINUS      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_EQUAL      )  pCMD_assign
   , IDX_EQ( EdKC_a_LEFT_SQ    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_RIGHT_SQ   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_BACKSLASH  )  pCMD_unassigned
   , IDX_EQ( EdKC_a_SEMICOLON  )  pCMD_ldelete        //            LAPTOP
   , IDX_EQ( EdKC_a_TICK       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_COMMA      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_DOT        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_SLASH      )  pCMD_copy           //            LAPTOP
   , IDX_EQ( EdKC_a_home       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_left       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_right      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_up         )  pCMD_unassigned
   , IDX_EQ( EdKC_a_down       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pgup       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pgdn       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_center     )  pCMD_restcur
   , IDX_EQ( EdKC_a_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_a_space      )  pCMD_unassigned
   , IDX_EQ( EdKC_a_bksp       )  pCMD_undo
   , IDX_EQ( EdKC_a_tab        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_esc        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_enter      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_0          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_1          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_2          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_3          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_4          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_5          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_6          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_7          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_8          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_9          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_a          )  pCMD_mword
   , IDX_EQ( EdKC_c_b          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_c          )  pCMD_towinclip
   , IDX_EQ( EdKC_c_d          )  pCMD_right
   , IDX_EQ( EdKC_c_e          )  pCMD_up
   , IDX_EQ( EdKC_c_f          )  pCMD_pword
   , IDX_EQ( EdKC_c_g          )  pCMD_cdelete
   , IDX_EQ( EdKC_c_h          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_i          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_j          )  pCMD_sinsert
   , IDX_EQ( EdKC_c_k          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_l          )  pCMD_replace
   , IDX_EQ( EdKC_c_m          )  pCMD_mark
   , IDX_EQ( EdKC_c_n          )  pCMD_linsert
   , IDX_EQ( EdKC_c_o          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_p          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_q          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_r          )  pCMD_refresh
   , IDX_EQ( EdKC_c_s          )  pCMD_left
   , IDX_EQ( EdKC_c_t          )  pCMD_tell
   , IDX_EQ( EdKC_c_u          )  pCMD_unassigned
   , IDX_EQ( EdKC_c_v          )  pCMD_fromwinclip
   , IDX_EQ( EdKC_c_w          )  pCMD_mlines
   , IDX_EQ( EdKC_c_x          )  pCMD_execute
   , IDX_EQ( EdKC_c_y          )  pCMD_ldelete
   , IDX_EQ( EdKC_c_z          )  pCMD_plines         //   // gap
   , IDX_EQ( EdKC_c_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f2         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f3         )  pCMD_grep           //       CMD_compile
   , IDX_EQ( EdKC_c_f4         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f8         )  pCMD_unassigned     //       CMD_print
   , IDX_EQ( EdKC_c_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f10        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_BACKTICK   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_MINUS      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_EQUAL      )  pCMD_false
   , IDX_EQ( EdKC_c_LEFT_SQ    )  pCMD_unassigned     //       CMD_pbal
   , IDX_EQ( EdKC_c_RIGHT_SQ   )  pCMD_setwindow
   , IDX_EQ( EdKC_c_BACKSLASH  )  pCMD_qreplace
   , IDX_EQ( EdKC_c_SEMICOLON  )  pCMD_unassigned
   , IDX_EQ( EdKC_c_TICK       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_COMMA      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_DOT        )  pCMD_false
   , IDX_EQ( EdKC_c_SLASH      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_home       )  pCMD_home
   , IDX_EQ( EdKC_c_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_left       )  pCMD_mword
   , IDX_EQ( EdKC_c_right      )  pCMD_pword
   , IDX_EQ( EdKC_c_up         )  pCMD_mpara
   , IDX_EQ( EdKC_c_down       )  pCMD_ppara
   , IDX_EQ( EdKC_c_pgup       )  pCMD_begfile
   , IDX_EQ( EdKC_c_pgdn       )  pCMD_endfile
   , IDX_EQ( EdKC_c_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_center     )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_c_space      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_bksp       )  pCMD_redo
   , IDX_EQ( EdKC_c_tab        )  pCMD_unassigned
   , IDX_EQ( EdKC_c_esc        )  pCMD_unassigned     //   // WINDOWS SYSTEM KC
   , IDX_EQ( EdKC_c_enter      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f1         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f2         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f3         )  pCMD_unassigned     //       CMD_nextmsg
   , IDX_EQ( EdKC_s_f4         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f5         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f6         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f7         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f8         )  pCMD_initialize
   , IDX_EQ( EdKC_s_f9         )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f10        )  pCMD_files
   , IDX_EQ( EdKC_s_f11        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_f12        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_home       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_end        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_left       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_right      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_up         )  pCMD_mlines
   , IDX_EQ( EdKC_s_down       )  pCMD_plines
   , IDX_EQ( EdKC_s_pgup       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_pgdn       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_ins        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_del        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_center     )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num0       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num1       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num2       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num3       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num4       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num5       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num6       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num7       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num8       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_num9       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numMinus   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numPlus    )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numStar    )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numSlash   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_numEnter   )  pCMD_unassigned
   , IDX_EQ( EdKC_s_space      )  pCMD_graphic
   , IDX_EQ( EdKC_s_bksp       )  pCMD_unassigned
   , IDX_EQ( EdKC_s_tab        )  pCMD_backtab
   , IDX_EQ( EdKC_s_esc        )  pCMD_unassigned
   , IDX_EQ( EdKC_s_enter      )  pCMD_emacsnewl

   , IDX_EQ( EdKC_cs_bksp      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_tab       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_center    )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_enter     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pause     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_space     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pgup      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_pgdn      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_end       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_home      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_left      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_up        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_right     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_down      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_ins       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_0         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_1         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_2         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_3         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_4         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_5         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_6         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_7         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_8         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_9         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_a         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_b         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_c         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_d         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_e         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_g         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_h         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_i         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_j         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_k         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_l         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_m         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_n         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_o         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_p         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_q         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_r         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_s         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_t         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_u         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_v         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_w         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_x         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_y         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_z         )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numStar   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numPlus   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numEnter  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numMinus  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_numSlash  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f1        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f2        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f3        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f4        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f5        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f6        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f7        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f8        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f9        )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f10       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f11       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_f12       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_scroll    )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_SEMICOLON )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_EQUAL     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_COMMA     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_MINUS     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_DOT       )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_SLASH     )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_BACKTICK  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_TICK      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_LEFT_SQ   )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_BACKSLASH )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_RIGHT_SQ  )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num0      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num1      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num2      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num3      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num4      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num5      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num6      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num7      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num8      )  pCMD_unassigned
   , IDX_EQ( EdKC_cs_num9      )  pCMD_unassigned

   , IDX_EQ( EdKC_pause        )  pCMD_unassigned
   , IDX_EQ( EdKC_a_pause      )  pCMD_unassigned
   , IDX_EQ( EdKC_s_pause      )  pCMD_unassigned
   , IDX_EQ( EdKC_c_numlk      )  pCMD_savecur
   , IDX_EQ( EdKC_scroll       )  pCMD_unassigned
   , IDX_EQ( EdKC_a_scroll     )  pCMD_unassigned
   , IDX_EQ( EdKC_s_scroll     )  pCMD_unassigned
   };

static_assert( ELEMENTS( g_Key2CmdTbl ) == EdKC_COUNT, "ELEMENTS( g_Key2CmdTbl ) == EdKC_COUNT" );

STATIC_FXN int MaxKeyNameLen_() {
   auto maxLen(0);
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      const auto len( Strlen( ky2Nm.name ) );
      if( maxLen < len )
          maxLen = len;
      }
   return maxLen;
   }

// GLOBAL_CONST int g_MaxKeyNameLen( MaxKeyNameLen_() ); // <-- Exuberant Ctags DOES NOT tag g_MaxKeyNameLen
//        const int g_MaxKeyNameLen( MaxKeyNameLen_() ); // <-- Exuberant Ctags DOES NOT tag g_MaxKeyNameLen
   GLOBAL_CONST int g_MaxKeyNameLen = MaxKeyNameLen_();  // <-- Exuberant Ctags DOES     tag g_MaxKeyNameLen

int edkcFromKeyname( PCChar pszKeyStr ) {
   const auto len( Strlen( pszKeyStr ) );
   if( len == 1 )
      return pszKeyStr[0];

   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      if( Stricmp( ky2Nm.name, pszKeyStr ) == 0 ) {
         return ky2Nm.EdKC_;
         }
      }

   return 0;
   }

int KeyStr_full( PPChar ppDestBuf, size_t *bufBytesLeft, int keyNum_word ) {
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      if( ky2Nm.EdKC_ == keyNum_word )
         return snprintf_full( ppDestBuf, bufBytesLeft, "%s", ky2Nm.name );
      }

   return 0;
   }


void StrFromEdkc( PChar pKeyStringBuf, size_t pKeyStringBufBytes, int edKC ) {
   Assert( pKeyStringBufBytes > 2 );
   for( const auto &ky2Nm : KyCd2KyNameTbl ) {
      if( ky2Nm.EdKC_ == edKC ) {
         safeStrcpy( pKeyStringBuf, pKeyStringBufBytes, ky2Nm.name );
         return;
         }
      }

   if( edKC < 0x100 && isprint( edKC ) ) {
      *pKeyStringBuf++ = edKC;
      *pKeyStringBuf = '\0';
      }
   else {
      safeSprintf( pKeyStringBuf, pKeyStringBufBytes, "edKC=0x%X", edKC );
      }
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
   if( d_fMeta ? DisableFilterKeys() : setKbFastFailed() )
      return Msg( "KB access failed" );

   Msg( "KB set to %sest setting",  (d_fMeta ? "slow" : "fast") );
   return true;
   }

#endif

#endif

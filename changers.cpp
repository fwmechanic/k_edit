//
// Copyright 1991 - 2013 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//

#include "ed_main.h"


//************************************************************************************************************

class KeyChanger {

 #define  VV  virtual void

   NO_COPYCTOR(KeyChanger);
   NO_ASGN_OPR(KeyChanger);

   void doRedraw();

   public:

   KeyChanger() {} // must supply dflt ctor if we even _declared_ any other ctor

   bool Run();

   VV updtMsg      () {};
   VV actionEsc    () {};
   VV actionC_Enter() {}; // GLOBAL save-exit

   VV actionDown () {};  VV actionCtrlRight() {};  VV actionAltRight() {};
   VV actionUp   () {};  VV actionCtrlLeft () {};  VV actionAltLeft () {};
   VV actionRight() {};  VV actionCtrlDown () {};  VV actionAltDown () {};
   VV actionLeft () {};  VV actionCtrlUp   () {};  VV actionAltUp   () {};

   VV actionPlus () {};
   VV actionMinus() {};
   VV actionStar () {};

 #undef  VV

   protected:

   FixedCharArray<90> d_msgbuf;
   };

void KeyChanger::doRedraw() {
   updtMsg();
   Msg( "%s", d_msgbuf.k_str() );
   DispRefreshWholeScreenNow();
   }

bool KeyChanger::Run() {
   while(1) {
      doRedraw();
      char keyNm[30];
      const auto pCmd( CmdFromKbdForInfo( BSOB(keyNm) ) );
      if( pCmd && *keyNm ) {
         if(      0==strcmp( keyNm, "enter"      ) ) {                    break; }
         else if( 0==strcmp( keyNm, "ctrl+enter" ) ) { actionC_Enter  (); break; }
         else if( 0==strcmp( keyNm, "esc"        ) ) { actionEsc      (); break; }
         else if( 0==strcmp( keyNm, "down"       ) ) { actionDown     (); }
         else if( 0==strcmp( keyNm, "up"         ) ) { actionUp       (); }
         else if( 0==strcmp( keyNm, "right"      ) ) { actionRight    (); }
         else if( 0==strcmp( keyNm, "left"       ) ) { actionLeft     (); }
         else if( 0==strcmp( keyNm, "num+"       ) ) { actionPlus     (); }
         else if( 0==strcmp( keyNm, "num-"       ) ) { actionMinus    (); }
         else if( 0==strcmp( keyNm, "num*"       ) ) { actionStar     (); }
         else if( 0==strcmp( keyNm, "ctrl+right" ) ) { actionCtrlRight(); }
         else if( 0==strcmp( keyNm, "ctrl+left"  ) ) { actionCtrlLeft (); }
         else if( 0==strcmp( keyNm, "ctrl+down"  ) ) { actionCtrlDown (); }
         else if( 0==strcmp( keyNm, "ctrl+up"    ) ) { actionCtrlUp   (); }
         else if( 0==strcmp( keyNm, "alt+right"  ) ) { actionAltRight (); }
         else if( 0==strcmp( keyNm, "alt+left"   ) ) { actionAltLeft  (); }
         else if( 0==strcmp( keyNm, "alt+down"   ) ) { actionAltDown  (); }
         else if( 0==strcmp( keyNm, "alt+up"     ) ) { actionAltUp    (); }
         }
      }

   doRedraw();
   return true;
   }

//************************************************************************************************************
#if defined(_WIN32)

#ifdef fn_cc

class ColorChanger : public KeyChanger
   {
   NO_COPYCTOR(ColorChanger);
   NO_ASGN_OPR(ColorChanger);

         U8 &d_colorVar;
   const U8  d_origColor;

   void SetColor( int val ) { d_colorVar = val; }

   void IncDecColor( bool fg, int incr ) {
      const int mask ( fg ? 0xF0 : 0x0F );
      const int shift( fg ?    4 :    0 );
      d_colorVar = (((((d_colorVar & mask) >> shift) + incr) << shift) & mask) | (d_colorVar & ~mask);
      }

   public:

   ColorChanger( U8 &colorVar ) : d_colorVar(colorVar), d_origColor(colorVar) {}

   void updtMsg    () override { d_msgbuf.Sprintf( "colorFG=%02X", d_colorVar ); }
   void actionEsc  () override { SetColor( d_origColor  ); }
   void actionDown () override { IncDecColor( false, +1 ); }
   void actionUp   () override { IncDecColor( false, -1 ); }
   void actionRight() override { IncDecColor( true , +1 ); }
   void actionLeft () override { IncDecColor( true , -1 ); }
   };

bool ARG::cc() {
   ColorChanger ccc( *g_CurView()->ColorIdx2Var( COLOR::FG ) );
   return ccc.Run();
   }

#endif

//************************************************************************************************************

class ConsoleSizeChanger : public KeyChanger
   {
   NO_COPYCTOR(ConsoleSizeChanger);
   NO_ASGN_OPR(ConsoleSizeChanger);

   Win32ConsoleFontChanger d_wcfc;

   const int d_origWidth ;
   const int d_origHeight;
   int       d_fontNow;
   int       d_numFonts;

   void Resize_Screen( int newX, int newY ) {
      Point newSize; newSize.lin=newY; newSize.col=newX;
      if( VideoSwitchModeToXYOk( newSize ) && newSize.lin==newY && newSize.col==newX ) {
         d_wcfc.GetFontInfo();
         }
      }

   bool SetFont( int idx ) {
      if( d_wcfc.GetFontAspectRatio( idx ) <= 1.0 ) {
         d_fontNow = idx;
         d_wcfc.SetFont( idx );
         return true;
         }
      return false;
      }

   public:

   ConsoleSizeChanger() : d_origWidth(EditScreenCols()), d_origHeight(ScreenLines())
      {
      d_fontNow  = d_wcfc.OrigFont();
      d_numFonts = d_wcfc.NumFonts();
      }

   void updtMsg    () { d_msgbuf.Sprintf( "ConsoleSizeChanger: %d x %d, font %d = %dx%d (%4.2f)"
                           , EditScreenCols()     , ScreenLines()
                           , d_fontNow
                           , d_wcfc.GetFontSizeX(), d_wcfc.GetFontSizeY()
                           , d_wcfc.GetFontAspectRatio( d_fontNow )
                           );
                      }
   void actionEsc  () { d_wcfc.SetOrigFont();   Resize_Screen( d_origWidth        , d_origHeight    ); }
   void actionDown () {                         Resize_Screen( EditScreenCols()   , ScreenLines()+1 ); }
   void actionUp   () {                         Resize_Screen( EditScreenCols()   , ScreenLines()-1 ); }
   void actionRight() {                         Resize_Screen( EditScreenCols()+1 , ScreenLines()   ); }
   void actionLeft () {                         Resize_Screen( EditScreenCols()-1 , ScreenLines()   ); }
   void actionPlus () { for(auto ix(d_fontNow+1); ix <= d_numFonts-1; ++ix) if( SetFont( ix ) ) return; }
   void actionMinus() { for(auto ix(d_fontNow-1); ix >= 0           ; --ix) if( SetFont( ix ) ) return; }
// void actionStar     () { Resize_Screen( 10000, 10000 ); }  // will get resized downward
   void actionStar     () { const auto maxSize( ConOut::GetMaxConsoleSize() );  Resize_Screen( maxSize.col-1   , maxSize.lin-1 ); }
   // maxSize.? -1 cuz GetMaxConsoleSize (Win32 API) doesn't seem to account for sizes of window borders
// void actionCtrlRight() { Resize_Screen( ((EditScreenCols()*4)/3),   ScreenLines()       ); }
   void actionCtrlRight() { const auto maxSize( ConOut::GetMaxConsoleSize() );  Resize_Screen( maxSize.col-1   , ScreenLines() ); }
   void actionCtrlDown () { const auto maxSize( ConOut::GetMaxConsoleSize() );  Resize_Screen( EditScreenCols(), maxSize.lin-1 ); }
// void actionCtrlDown () { Resize_Screen(   EditScreenCols(),       ((ScreenLines()*4)/3) ); }
   void actionCtrlLeft () { Resize_Screen( ((EditScreenCols()*3)/4),   ScreenLines()       ); }
   void actionCtrlUp   () { Resize_Screen(   EditScreenCols(),       ((ScreenLines()*3)/4) ); }
   };

bool ARG::resize() {
   ConsoleSizeChanger ssc;
   return ssc.Run();
   }

#endif
//************************************************************************************************************

class TabWidthChanger : public KeyChanger
   {
   NO_COPYCTOR(TabWidthChanger);
   NO_ASGN_OPR(TabWidthChanger);

   const int  d_origTabWidth;
   const bool d_orig_fTabDisp;

   void IncDecTabWidth( int incr ) { g_CurFBuf()->SetTabWidthOk( g_CurFBuf()->TabWidth() + incr ); }

   public:

   TabWidthChanger() : d_origTabWidth(  g_CurFBuf()->TabWidth() )
                     , d_orig_fTabDisp( g_CurFBuf()->fTabDisp() )
                     {}

   void updtMsg    () { d_msgbuf.Sprintf( "tabwidth=%d, arrows change: LEFT--, RIGHT++, UP:visible DOWN:hide", g_CurFBuf()->TabWidth() ); }
   void actionEsc  () { g_CurFBuf()->SetTabWidthOk( d_origTabWidth );
                        g_CurFBuf()->SetTabDisp( d_orig_fTabDisp );
                      }
   void actionDown () { g_CurFBuf()->BlankAnnoDispSrcEdge( BlankDispSrc_USER_ALWAYS, false ); }
   void actionUp   () { g_CurFBuf()->BlankAnnoDispSrcEdge( BlankDispSrc_USER_ALWAYS, true  ); }
   void actionRight() { IncDecTabWidth( +1 ); }
   void actionLeft () { IncDecTabWidth( -1 ); }
   void actionC_Enter()
        { // GLOBAL save-exit
        g_CurFBuf()->FreezeTabSettings(); // any user-edit of these settings locks out auto-setting of same
        g_iTabWidth = g_CurFBuf()->TabWidth();
        // g_CurFBuf()->CalcIndent();
        }

   };

bool ARG::ftab() {
   TabWidthChanger twc;
   return twc.Run();
   }

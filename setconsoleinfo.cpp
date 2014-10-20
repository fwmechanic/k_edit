//*****************************************************************
// SETCONSOLEINFO.CPP
//
// Undocumented method to set console attributes
//  at runtime including console palette (NT4, 2000, XP).
//
// VOID WINAPI SetConsolePalette(COLORREF palette[16])
//
// For Vista use the newly documented SetConsoleScreenBufferEx API
//
// www.catch22.net
//
//*****************************************************************
//  DDM notes (02/24/07 10:18)
//  If I compile this with VC6, this all works fine!!
//  cl /W3 /O2 /G4 /Feconsattr.exe setconsoleinfo.c kernel32.lib user32.lib
//
//  However, if I try to compile it with mingw,
//  it crashes on the first or second run.
//  gcc -Wall -O3 -s $< -o $@ -lcomctl32
//*****************************************************************

static char *VerStr = "Set Console Attributes, Version 1.02" ;

#define WIN32_LEAN_AND_MEAN // this will assume smaller exe... NOT ...
#define  _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>

// #include "wfontinfo.h"

typedef unsigned char u8;

typedef union {
   unsigned int   ul ;
   unsigned char  uc[4] ;
} ul2uc_t ;

#define  STD_FILE_LEN   1024

static char tempstr[81] ;
static char my_path[STD_FILE_LEN] = "" ;
static char orig_name[STD_FILE_LEN] = "" ;
static char bkup_name[STD_FILE_LEN] = "" ;

//**************************************************************
//  dos.pal
// 000   00 00 00 00 00 2A 00 2A  00 00 2A 2A 2A 00 00 2A
// 010   00 2A 2A 15 00 2A 2A 2A  15 15 15 15 15 3F 15 3F
// 020   15 15 3F 3F 3F 15 15 3F  15 3F 3F 3F 15 3F 3F 3F
// 030   81 C7 D9
//**************************************************************
COLORREF curr_attr[16] ;

// Undocumented console message
#define WM_SETCONSOLEINFO        (WM_USER+201)

//
// Structure to send console via WM_SETCONSOLEINFO
//
#pragma pack(push, 1)
typedef struct _CONSOLE_INFO
{
   ULONG    Length;
   COORD    ScreenBufferSize;
   COORD    WindowSize;
   ULONG    WindowPosX;
   ULONG    WindowPosY;

   COORD    FontSize;
   ULONG    FontFamily;
   ULONG    FontWeight;
   WCHAR    FaceName[32];

   ULONG    CursorSize;
   ULONG    FullScreen;
   ULONG    QuickEdit;
   ULONG    AutoPosition;
   ULONG    InsertMode;

   USHORT   ScreenColors;
   USHORT   PopupColors;
   ULONG    HistoryNoDup;
   ULONG    HistoryBufferSize;
   ULONG    NumberOfHistoryBuffers;

   COLORREF ColorTable[16];

   ULONG    CodePage;
   HWND     Hwnd;

   WCHAR    ConsoleTitle[0x100];

} CONSOLE_INFO;
#pragma pack(pop)

//*************************************************************
//  each subsequent call to this function overwrites
//  the previous report.
//*************************************************************
char *get_system_message(void)
{
   static char msg[261] ;
   int slen ;

   LPVOID lpMsgBuf;
   FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   strncpy(msg, (char *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = strlen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 13) {
      msg[slen-1] = 0 ;
   }

   return msg;
}

//
//  Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
   DWORD   dwConsoleOwnerPid;
   HANDLE  hProcess;
   HANDLE   hSection, hDupSection;
   PVOID   ptrView = 0;
   HANDLE  hThread;

   //
   // Open the process which "owns" the console
   //
   GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);

   hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwConsoleOwnerPid);
   if (hProcess == NULL) {
      wsprintf(tempstr, "OpenProcess: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   //
   // Create a SECTION object backed by page-file, then map a view of
   // this section into the owner process so we can write the contents
   // of the CONSOLE_INFO buffer into it
   //
   hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, pci->Length, 0);
   if (hSection == NULL) {
      wsprintf(tempstr, "CreateFileMapping: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   //
   // Copy our console structure into the section-object
   //
   ptrView = MapViewOfFile(hSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, pci->Length);
   if (ptrView == NULL) {
      wsprintf(tempstr, "MapViewOfFile: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   memcpy(ptrView, pci, pci->Length);

   if (!UnmapViewOfFile(ptrView)) {
      wsprintf(tempstr, "UnmapViewOfFile: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   //
   // Map the memory into owner process
	//
   if (!DuplicateHandle(GetCurrentProcess(), hSection, hProcess, &hDupSection, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
      wsprintf(tempstr, "DuplicateHandle: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   //  Send console window the "update" message
   SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)hDupSection, 0);

   //
   // clean up
   //
   hThread = CreateRemoteThread(hProcess, 0, 0,
      (LPTHREAD_START_ROUTINE)CloseHandle, hDupSection, 0, 0);
   if (hThread == NULL) {
      wsprintf(tempstr, "CreateRemoteThread: %s\n", get_system_message()) ;
      OutputDebugString(tempstr) ;
      return FALSE;
   }

   CloseHandle(hThread);
   CloseHandle(hSection);
   CloseHandle(hProcess);

   return TRUE;
}

//
// Fill the CONSOLE_INFO structure with information
//  about the current console window
//
static void GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
   CONSOLE_SCREEN_BUFFER_INFO csbi;

   HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

   GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

   pci->ScreenBufferSize = csbi.dwSize;
   pci->WindowSize.X     = csbi.srWindow.Right - csbi.srWindow.Left + 1;
   pci->WindowSize.Y     = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
   pci->WindowPosX       = csbi.srWindow.Left;
   pci->WindowPosY       = csbi.srWindow.Top;
}

//
// Fill the CONSOLE_INFO structure with information
//  about the current console font
//
//

static void GetConsoleFontInfo(CONSOLE_INFO *pci)
{
   CONSOLE_FONT_INFO cfi ;
   COORD fsize ;
   HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

   GetCurrentConsoleFont(hConsoleOut, FALSE, &cfi) ;
   fsize = GetConsoleFontSize(hConsoleOut, cfi.nFont) ;

   pci->FontSize.X = fsize.X ;
   pci->FontSize.Y = fsize.Y ;

   // wsprintf(tempstr, "Font Size= X%u Y%u\n", fsize.X, fsize.Y) ;
   // OutputDebugString(tempstr) ;

   // set these to zero to keep current settings
   // pci->FontFamily = 0x30;//FF_MODERN|FIXED_PITCH;//0x30;
   // pci->FontWeight = 0x400;
   // lstrcpyW(pci->FaceName, L"Terminal");
   pci->FontFamily = 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
   pci->FontWeight = 0;//0x400;
   pci->FaceName[0]          = L'\0';
}

// Set palette of current console
//
// palette should be of the form:
//
// COLORREF DefaultColors[16] =
// {
// 0x00000000, 0x00800000, 0x00008000, 0x00808000,
// 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
// 0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00,
// 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
// };
//
VOID WINAPI SetConsolePalette(COLORREF palette[16])
{
   CONSOLE_INFO ci = { sizeof(ci) };
   int i;
   HWND hwndConsole = GetConsoleWindow();

   // get current size/position settings rather than using defaults..
   GetConsoleSizeInfo(&ci);

   GetConsoleFontInfo(&ci);

   // set these to zero to keep current settings
   ci.CursorSize           = 25;
   ci.FullScreen           = FALSE;
   ci.QuickEdit            = TRUE;
   ci.AutoPosition         = 0x10000;
   ci.InsertMode           = TRUE;
   ci.ScreenColors         = MAKEWORD(0x7, 0x0);
   ci.PopupColors          = MAKEWORD(0x5, 0xf);

   ci.HistoryNoDup         = FALSE;
   ci.HistoryBufferSize    = 50;
   ci.NumberOfHistoryBuffers  = 4;

   // colour table
   for(i = 0; i < 16; i++)
      ci.ColorTable[i] = palette[i];

   ci.CodePage             = 0;//0x352;
   ci.Hwnd                 = hwndConsole;

   lstrcpyW(ci.ConsoleTitle, L"");

   SetConsoleInfo(hwndConsole, &ci);
}

//**************************************************************
static unsigned make_brighter(unsigned utemp, double brighten)
{
   A uvalue = utemp * brighten;
   if (uvalue > 255)
       uvalue = 255;
   return uvalue ;
}

//**************************************************************
int read_palette_file(char *palette_name, double brighten)
{
   int rdbytes ;
   ul2uc_t uconv ;
   u8 pdata[65] ;
   u8 *uptr ;
   unsigned utemp, j ;
   int hdl = _open(palette_name, O_BINARY | O_RDONLY) ;
   if (hdl < 0) {
      wsprintf(tempstr, "open (read): %s: %s\n", palette_name, get_system_message()) ;
      OutputDebugString(tempstr) ;
      return errno ;
   }
   rdbytes = _read(hdl, pdata, sizeof(pdata)+1) ;
   _close(hdl) ;
   if (rdbytes != 64) {
      // return -1;
      wsprintf(tempstr, "read: %s: %d vs %d\n", palette_name, rdbytes, sizeof(pdata)) ;
      OutputDebugString(tempstr) ;
      return EINVAL ;
   }

   uptr = &pdata[0] ;
   for (j=0; j<16; j++) {
      //  bytes are reached in r,g,b sequence in binary file.
      //  Also, don't brighten index 0 (background color)
      utemp = (unsigned) *uptr++ ;
      if (brighten)
         utemp = make_brighter(utemp, brighten) ;
      uconv.uc[0] = (u8) utemp ;
      utemp = (unsigned) *uptr++ ;
      if (brighten)
         utemp = make_brighter(utemp, brighten) ;
      uconv.uc[1] = (u8) utemp ;
      utemp = (unsigned) *uptr++ ;
      if (brighten)
         utemp = make_brighter(utemp, brighten) ;
      uconv.uc[2] = (u8) utemp ;
      uconv.uc[3] = 0 ;
      uptr++ ; //  skip fourth byte
      curr_attr[j] = uconv.ul ;
   }
   return 0;
}

//************************************************************************
//  If no relative or absolute path is specified with palette_file,
//  the files will be assumed to be stored in a 'palette' directory
//  beneath the location of the executable file.
//************************************************************************
static void update_palette_file(char *palette_file)
{
   char tbfr[STD_FILE_LEN] ;
   char *sptr ;
   int slen ;
   my_path[0] = 0 ;  //  just in case...

   //  append .plt if needed
   slen = strlen(palette_file) ;
   if (slen <= 4) {
      strcat(palette_file, ".plt") ;
   } else {
      // printf("slen=%d\n", slen) ;
      slen -= 4 ; //  point to extension
      if (strnicmp(palette_file+slen, ".plt", 4) != 0)
         strcat(palette_file, ".plt") ;
   }

   //  if backslash is present, this is a path specification
   if (strchr(palette_file, '\\') != 0)
      return ;

   //  this shouldn't fail.  If it does, just blow this all off...
   if (GetModuleFileName(NULL, tbfr, sizeof(tbfr)) == 0)
      return ;
   printf("found [%s]\n", tbfr) ;

   sptr = strrchr(tbfr, '\\') ;
   if (sptr == 0)
      return ;

   sptr++ ; //  retain the backslash
   *sptr = 0 ; //  set up for copying to my_path
   //  save this for finding console2 path (-x option only)
   strcpy(my_path, tbfr) ;

   sprintf(sptr, "palettes\\%s", palette_file) ;
   strcpy(palette_file, tbfr) ;
}

//************************************************************************
static int file_exists(char *fname)
{
   struct stat st ;
   if (stat(fname, &st) == 0)
      return 1;
   return 0;
}

//************************************************************************
static char *skip_spaces(char *inpstr)
{
   while (*inpstr == ' '  ||  *inpstr == '\t')
      inpstr++ ;
   return inpstr;
}

//************************************************************************
static char *next_field(char *inpstr)
{
   while (*inpstr != ' '  &&  *inpstr != 0)
      inpstr++ ;
   while (*inpstr == ' ')
      inpstr++ ;
   return inpstr;
}

//************************************************************************
static void UpdateConsole2File(void)
{
   static char inpstr[1024] ;
   static char pstr[60] ;
   ul2uc_t uconv ;
   //  let's change the default from "current directory" to
   //  <consattr.exe directory>\\console2
   //  so I can put console2 directory under utility.
   if (orig_name[0] == 0) {
      if (my_path[0] == 0)
         strcpy(orig_name, ".\\") ;
      else {
         strcpy(orig_name, my_path) ;
         strcat(orig_name, "console2\\") ;
      }
   }
   //  make sure there's a backslash at the end
   int slen = strlen(orig_name) ;
   slen-- ; //  change from length to end-index
   if (orig_name[slen] == '\\') {
      puts("trailing backslash is present") ;
   } else {
      puts("adding trailing backslash") ;
      strcat(orig_name, "\\") ;
   }
   strcpy(bkup_name, orig_name) ;
   strcat(orig_name, "console.xml") ;
   strcat(bkup_name, "console.xml~") ;

   if (!file_exists(orig_name)) {
      printf("Could not find %s !!\n", orig_name) ;
      // puts("For now, it needs to be in the current directory.") ;
      return ;
   }
   unlink(bkup_name) ;
   if (rename(orig_name, bkup_name) != 0) {
      perror(orig_name) ;
      puts("could not create backup file, aborting...") ;
      return ;
   }

   FILE *fdin = fopen(bkup_name, "rt") ;
   if (fdin == 0) {
      perror(bkup_name) ;
      return ;
   }
   FILE *fdout = fopen(orig_name, "wt") ;
   if (fdout == 0) {
      fclose(fdin) ;
      perror(orig_name) ;
      return ;
   }
   char *hd ;
   unsigned id ;
   int pstate = 0 ;
   while (fgets(inpstr, sizeof(inpstr), fdin) != 0) {
      char *dptr = skip_spaces(inpstr) ;
      switch (pstate) {
      case 0:  //  looking for <colors> label
         if (strnicmp(dptr, "<colors>", 8) == 0) {
            // printf("found %s", dptr) ;
            pstate = 1 ;
         }
         // else {
         //    printf("passing %s", dptr) ;
         // }
         fputs(inpstr, fdout) ;
         break;

      case 1:  //  parsing color entries
         if (strnicmp(dptr, "</colors>", 9) == 0) {
            // printf("found %s", dptr) ;
            pstate = 2 ;
            fputs(inpstr, fdout) ;
            break;
         }
         //  process color entry
         // fputs(inpstr, fdout) ;  //@@@ debug
         // printf("color entry %s", dptr) ;
         // <color id="0" r="0" g="0" b="0"/>
         if (strnicmp(dptr, "<color", 6) != 0) {
            printf("invalid color entry (c): %s", dptr) ;
            return ;
         }
         hd = next_field(dptr) ;
         if (strnicmp(hd, "id=", 3) != 0) {
            printf("invalid color entry (i): %s", hd) ;
            return ;
         }
         hd += 4 ;
         id = atoi(hd) ;

         //  read new color entry
         uconv.ul = curr_attr[id] ;
         // printf("color %u= r%u, g%u, b%u (0x%08X)\n", id,
         //    (unsigned) uconv.uc[2],
         //    (unsigned) uconv.uc[1],
         //    (unsigned) uconv.uc[0],
         //    uconv.ul) ;
         fprintf(fdout, "\t\t\t<color id=%c%u%c r=%c%u%c g=%c%u%c b=%c%u%c/>\n",
            34, id, 34,
            34, (unsigned) uconv.uc[0], 34,
            34, (unsigned) uconv.uc[1], 34,
            34, (unsigned) uconv.uc[2], 34) ;

         break;

      default: //  echo everything else to output
         fputs(inpstr, fdout) ;
         break;
      }

   }
   fclose(fdin) ;
   fclose(fdout) ;
   printf("Updated %s successfully\n", orig_name) ;
}

//************************************************************************
static void usage(void)
{
   puts("Usage: consattr [options] palette_file") ;
   puts("Options:") ;
   puts(" -bB.B - set brightness level (default is 3.1)") ;
   puts("") ;
   puts(" -xcpath - update a Console2 xml config file in directory [cpath].") ;
   puts("   If cpath is omitted, current directory is used.") ;
   puts("") ;
   puts("If palette_file does not contain a path, I will look for") ;
   puts("the file in the 'palettes' directory below the location") ;
   puts("where the executable file is found.") ;
   puts("Also, if the .plt extension is not included in palette_file,") ;
   puts(".plt will be appended.") ;
}

//************************************************************************
int main(int argc, char** argv)
{
   char palette_file[STD_FILE_LEN] = "" ;
   double brightness = 3.1 ;
   int xml_update = 0 ;
   int j, result ;
   char *p ;

   puts(VerStr) ;
   for (j=1; j<argc; j++) {
      p = argv[j] ;
      if (*p == '-') {
         p++ ;
         switch (*p) {
         case 'b':
            p++ ;
            brightness = strtod(p, 0) ;
            break;

         case 'x':
            p++ ;
            if (*p == 0) {
               orig_name[0] = 0 ;
            } else {
               strncpy(orig_name, p, sizeof(orig_name)) ;
            }
            xml_update = 1 ;
            break;

         default:
            usage() ;
            return 1;
         }

      } else {
         strncpy(palette_file, p, sizeof(palette_file)) ;
      }
   }

   if (palette_file[0] == 0) {
      usage() ;
      return 1;
   }
   update_palette_file(palette_file) ;

   printf("palette file: %s\n", palette_file) ;
   result = read_palette_file(palette_file, brightness) ;
   if (result != 0) {
      printf("%s: %s\n", palette_file, strerror(result)) ;
      return 1;
   }
   if (xml_update)
      UpdateConsole2File() ;
   else
      SetConsolePalette(curr_attr) ;

   return 0;
}

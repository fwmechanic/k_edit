//
// Copyright 1991 - 2014 by Kevin L. Goodwin [fwmechanic@yahoo.com]; All rights reserved
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp
//

#include "ed_main.h"
// #include <boost/filesystem/operations.hpp>

#define  CALLING_GetVolumeInformation  0
#if      CALLING_GetVolumeInformation
bool GetRootpathOk( PChar pDestBuf, size_t sizeofDest, PCChar pSrcFullname ) {
   if( sizeofDest < 4 ) return false;

   const auto ps1( Path::StrToNextPathSepOrEos( pSrcFullname ) );
   if( !*ps1 ) {
      DBG( "%s '%s' !1", __func__, pSrcFullname );
      return false;
      }

   if(  ps1 - pSrcFullname == 2
  // && is_alpha( pSrcFullname[0] )
     && pSrcFullname[1] == ':'
     ) {
      pDestBuf[0] = pSrcFullname[0];
      pDestBuf[1] = ':';
      pDestBuf[2] = Path::PATH_SEP_CH;
      DBG( "%s '%s' -> '%s'", __func__, pSrcFullname, pDestBuf );
      return true;
      }

   if( ps1 - pSrcFullname == 1 ) { // "\\..."
      const auto ps2( Path::StrToNextPathSepOrEos( ps1 ) ); // "\\...\..."
      if( !*ps2 ) {
         DBG( "%s '%s' !2", __func__, pSrcFullname );
         return false;
         }
      const auto ps3( Path::StrToNextPathSepOrEos( ps2 ) ); // "\\...\...\..."
      const auto len( safeStrcpy( pDestBuf, sizeofDest, pSrcFullname, ps3 - pSrcFullname ) );
      if( !*ps3 )
         safeStrcat( pDestBuf, sizeofDest, len, PATH_SEP_STR, Strlen( PATH_SEP_STR ) );

      DBG( "%s '%s' -> '%s'", __func__, pSrcFullname, pDestBuf );
      return true;
      }

   DBG( "%s '%s' !3", __func__, pSrcFullname );
   return false;
   }
#endif

/*
   The PIECE OF SHIT Win32 API GetVolumeInformation flag FS_CASE_SENSITIVE is
   MEANINGLESS, since NTFS, while being CASE-SENSITIVE ***CAPABLE***, IS NOT
   actually BEHAVING in CASE-SENSITIVE mode when accessed via the Win32 API.
   Since what is needed is the ACTUAL MODE, not what the FS is CAPABLE of, this
   flag is useless for this purpose.  (What purpose _IS_ it useful for???)

   Second attempt was to _strcmp the "name of file system" field returned by Win32
   API GetVolumeInformation, and if "NTFS", consider the filesystem to be NOT
   CS.  Except that SAMBA mounts/volumes at work return "name of file system" ==
   "NTFS" too, and they can be either CS or CI (depending on Samba settings).

   Third attempt was to use GetFileInformationByHandle, which SUPPOSEDLY
   returns a GUID for a file, by opening the file twice using different case,
   and obtaining the GUIDs of each case's instance, we should be able to tell if
   the different-cased names refer to the same file or not.  Should!
   But, while a Samba volume is set to case sensitive mode (which I can prove
   from CMD.EXE), GUIDs returned from GetFileInformationByHandle for the
   diff-cased file are different!  (Anyway, if the FS were truly acting in CS
   mode, the open of the wrong-cased file should FAIL).

   <040609> klg

   Gave up on the whole fscking thing and rewrote things so the string returned
   from Win32::FindNextFile is used, per path component, to create the
   canonical/authoritative filename.  I DO force the drive letter to lowercase,
   which doesn't seem to cause any adverse effects!

   <041006> klg
*/

STATIC_FXN void LowerDriveLetter( PChar pbuf ) {
   if( pbuf[0] && pbuf[1] == ':' )
       pbuf[0] = tolower( pbuf[0] );
   }

std::string Path::GetCwd() {
   pathbuf pbuf;
   if( !_getcwd( BSOB( pbuf ) ) )
      pbuf[0] = '\0';
   else
      LowerDriveLetter( pbuf );

   _strlwr( pbuf );
   return std::string( pbuf );
   }

std::string Path::Absolutize( PCChar pszFilename ) {  enum { DEBUG_FXN = 0 };
#ifdef BOOST_LIB_VERSION
   boost::filesystem::path src( pszFilename );
   boost::system::error_code ec;
   auto boost_dest( canonical( src, ec ) );
   // std::string destgs( boost_dest.generic_string() );
   // DBG( "%s Boost '%s' -> '%s'", __func__, pszFilename, destgs.c_str() );
   std::string dests( boost_dest.string() );
   DBG( "%s Boost '%s' -> '%s'", __func__, pszFilename, dests.c_str() );
#endif
   /* old code, works fine but uses Win32 API explicitly
      GetFullPathName combines the FILENAME with the current drive and directory
      name and returns a fully qualified (aka, absolute) path name.  Note that
      no attempt is made to convert 8.3 components in the supplied FILENAME to
      longnames or vice-versa.
   */
   pathbuf pbuf;
   {
   PChar dummy;
   if( !Win32::GetFullPathName( pszFilename, sizeof pbuf, pbuf, &dummy ) ) {
      // DEBUG_FXN &&
      DBG( "%s- '%s' -> '%s'", __func__, pszFilename, pbuf );
      return std::string("");
      }
   }
   // site:microsoft.com Win32 File Namespaces
   //
   STATIC_CONST char kszDevicePrefix[] = "\\\\.\\";
   if( memcmp( pbuf, kszDevicePrefix, KSTRLEN(kszDevicePrefix) ) == 0 ) { // Win32::GetFullPathName returned a DEVICE-space name?
      // DEBUG_FXN &&
      DBG( "%s- '%s' -> '%s' is a DEVICE so FAILS", __func__, pszFilename, pbuf );
      return std::string("");
      }

   DEBUG_FXN && DBG( "%s+ '%s' -> '%s'", __func__, pszFilename, pbuf );

   auto rv( Path::CanonizeCase( pbuf ) );

 #if CALLING_GetVolumeInformation
   pathbuf root;
   GetRootpathOk( BSOB(root), pDestBuf );
   Win32::DWORD maxCompLen;
   Win32::DWORD fsFlags;
   pathbuf volname;
   pathbuf fsname;
   if( Win32::GetVolumeInformation(
         root,
         BSOB(volname),
         NULL,
         &maxCompLen,
         &fsFlags,
         BSOB(fsname)
         )
      )
       1 && DBG( "GetVolumeInformation: %08X '%s'", fsFlags, pDestBuf );
 #endif

   DEBUG_FXN && DBG( "%s- '%s' -> '%s'", __func__, pszFilename, rv.c_str() );
   return rv;
   }

//-----------------------------------------------------------------------------------

STIL bool KeepMatch( const WildCardMatchMode want, const unsigned have ) {
   // A directory usually DOES have FILE_ATTRIBUTE_xxx's _besides_
   // FILE_ATTRIBUTE_DIRECTORY (ie.  they look like files), so if the
   // FILE_ATTRIBUTE_DIRECTORY is used, it must be examined alone
   //

   const auto fFileOk( ToBOOL(want & ONLY_FILES) );
   const auto fDirOk ( ToBOOL(want & ONLY_DIRS ) );
   const auto fIsDir ( ToBOOL(have & FILE_ATTRIBUTE_DIRECTORY) );
   const auto fIsFile( !fIsDir && (have & (FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY)) );

   if( fDirOk  && fIsDir  ) return true;
   if( fFileOk && fIsFile ) return true;
   return                          false;
   }

bool DirMatches::KeepMatch() const {
   const auto rv( ::KeepMatch( d_wcMode, d_Win32_FindData.dwFileAttributes ) );

   0 && DBG( "want %c, have %c '%s' rv=%d"
           , (d_wcMode & ONLY_DIRS) ? 'D':'d'
           , (d_Win32_FindData.dwFileAttributes) ? 'D':'d'
           , d_Win32_FindData.cFileName
           , rv
           );

   return rv;
   }

bool DirMatches::FoundNext() {
#if defined(_WIN32)
   if( !d_fTriedFirst ) {
      d_fTriedFirst = true;
      d_hFindFile = Win32::FindFirstFile( d_buf.c_str(), &d_Win32_FindData );
      if( d_hFindFile == Win32::Invalid_Handle_Value() ) {
         d_ixDest = std::string::npos; // flag no more matches
         return false; // failed!
         }

      return true;
      }

   const auto rv( Win32::FindNextFile( d_hFindFile, &d_Win32_FindData ) != 0 );
   if( !rv )
      d_ixDest = std::string::npos; // flag no more matches

   return rv;
#else
   return false;
#endif
   }

const std::string DirMatches::GetNext() {
   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return std::string("");

   while( FoundNext() && !KeepMatch() )
      continue;

   if( d_ixDest == std::string::npos ) // already hit no-more-matches condition?
      return std::string("");

   d_buf.replace( d_ixDest, std::string::npos, d_Win32_FindData.cFileName );
   if( ToBOOL(d_Win32_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !Path::IsDotOrDotDot( d_buf.c_str() ) )
      d_buf.append( PATH_SEP_STR );

   0 && DBG( "DirMatches::GetNext: '%s' (%lX)", d_buf.c_str(), d_Win32_FindData.dwFileAttributes );
   return d_buf;
   }

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

std::string Path::CanonizeCase( const PCChar fnmBuf ) { enum { DBG_ABS_PATH = 0 };
   /* If a filesystem is NOT case sensitive, we could, by typing different
      casings of the filename, open the same file into multiple edit buffers,
      and accidentally make parallel edits; leading to A DISASTER!

      Similarly, there are numerous ways to relatively name _THE SAME file_.

      To preclude this situation, we need to adopt throughout all code use of a
      "canonizied" path+filename.  This function provides that conversion.

      For Win32: use FindFirstFile API, which returns the OS-/Filesys-understood
      cased version of each non-wildcard path-tail-component, to do this same
      conversion to EVERY component of the path.  Any drive-letter component is
      lower-cased, simply because I find that least annoying.
   */
   DBG_ABS_PATH && DBG( "%s+ '%s'", __func__, fnmBuf );

   PCChar pNxtComponent = fnmBuf;
   const bool hasDriveLetter( isalpha( fnmBuf[0] ) && fnmBuf[1] == ':' ); // leading c: ?
   if( hasDriveLetter ) {
      pNxtComponent += 2;
      }
   while( Path::IsPathSepCh( *pNxtComponent ) ) {
      pNxtComponent++;
      }

   std::string pbs( fnmBuf, pNxtComponent-fnmBuf ); // since path may grow due to (8.3-equivalent) -> longname expansion, we accumulate in pb
   if( hasDriveLetter ) {
      pbs[0] = tolower( pbs[0] );
      }
   DBG_ABS_PATH && DBG( "%s startX '%s'", __func__, pNxtComponent );
   DBG_ABS_PATH && DBG( "0 %s|", pbs.c_str() );

   while(1) {
      const auto pNxtComponentEnd( Path::StrToNextPathSepOrEos( pNxtComponent ) );
      const auto fLastComponent( *pNxtComponentEnd == '\0' );
      const auto nxtComponentLen( pNxtComponentEnd - pNxtComponent );  DBG_ABS_PATH && DBG( "%s NXT='%*.*s'", __func__, pd2Int(nxtComponentLen), pd2Int(nxtComponentLen), pNxtComponent );

      const auto segStartIx( pbs.length() );  // where EXPANDED version of pNxtComponent will be copied
      pbs.append( pNxtComponent, nxtComponentLen ); // copy INPUT version of pNxtComponent
                                                                       DBG_ABS_PATH && DBG( "1 %s|", pbs.c_str() );
      // Even though we are NOT expanding wildcards here, the length of wfd.cFileName
      // MAY NOT EQUAL nxtComponentLen due to shortname (8.3-equivalent) -> longname expansion
      const auto segStart( pbs.c_str() + segStartIx );
      Win32::WIN32_FIND_DATA wfd;
      Win32::HANDLE          hFF;                                      DBG_ABS_PATH && DBG( "segStart:%s|", segStart );
      if(   !HasWildcard(   segStart )
 //      && !Path::isDotOrDotDot( segStart )
         && Win32::Invalid_Handle_Value() != (hFF=Win32::FindFirstFile( pbs.c_str(), &wfd ))
        ) { Win32::FindClose( hFF );
         pbs.replace( segStartIx, std::string::npos, wfd.cFileName );  DBG_ABS_PATH && DBG( "2 %s|", pbs.c_str() );
         }

      if( fLastComponent )
         break;

      pbs += PATH_SEP_STR;                                             DBG_ABS_PATH && DBG( "%s :::: '%s'", __func__, pbs.c_str() );
      pNxtComponent = pNxtComponentEnd + 1;
      }
                                                                       DBG_ABS_PATH && DBG( "%s- rv='%s'", __func__, pbs.c_str() );
   return pbs;
   }

bool FileOrDirExists( PCChar pszFileName ) {
   FileAttribs fa( pszFileName );
   return fa.Exists();
   }

bool IsFileReadonly( PCChar pszFileName ) {
   FileAttribs fa( pszFileName );
   return fa.Exists() && fa.IsReadonly();
   }

bool Path::IsLegalFnm( PCChar name ) {
   return StrToNextOrEos( name, Path::InvalidFnmChars() )[0] == '\0';
   }

#ifdef fn_glds

bool ARG::glds() {
   linebuf lbuf;
   auto rv( Win32::GetLogicalDriveStrings( sizeof lbuf, lbuf ) );
   if( rv == 0 || rv > sizeof lbuf )
      return Msg( "Win32::GetLogicalDriveStrings FAILED" );

   auto pFBuf( g_CurFBuf() );

   auto ix(0);
   for( auto px(lbuf); *px != '\0'; ++ix, px=Eos( px )+1 ) {
      PCChar pdt;
      switch( Win32::GetDriveType( px ) ) {
         default:pdt = "???"               ; break;
         case 0: pdt = "DRIVE_UNKNOWN"     ; break;
         case 1: pdt = "DRIVE_NO_ROOT_DIR" ; break;
         case 2: pdt = "DRIVE_REMOVABLE"   ; break;
         case 3: pdt = "DRIVE_FIXED"       ; break;
         case 4: pdt = "DRIVE_REMOTE"      ; break;
         case 5: pdt = "DRIVE_CDROM"       ; break;
         case 6: pdt = "DRIVE_RAMDISK"     ; break;
         }
      pFBuf->FmtLastLine( "%s = %s", px, pdt );
      }

   Msg( "Win32::GetLogicalDriveStrings returned %d strings", ix );
   return true;
   }

#endif

#ifdef fn_findfile

// this is a development-assist fn; not useful to users
//
//  findfile:alt+f
//
//  .svn
//
bool ARG::findfile() {
   STATIC_FXN auto hFindFile( Win32::Invalid_Handle_Value() );
   STATIC_VAR Win32::WIN32_FIND_DATA Win32_FindData;

   switch( d_argType ) {
      case TEXTARG:
           hFindFile = Win32::FindFirstFile( d_textarg.pText, &Win32_FindData );
           if( hFindFile == Win32::Invalid_Handle_Value() )
              return Msg( "FindFirstFile => INVALID_HANDLE_VALUE" );

           {
           linebuf buf; buf[0] = '\0';
           auto pB(buf);  auto cbB(sizeof(buf));
           if( FILE_ATTRIBUTE_ARCHIVE   ) snprintf_full( &pB, &cbB, ",ARCH" );
           if( FILE_ATTRIBUTE_NORMAL    ) snprintf_full( &pB, &cbB, ",NORM" );
           if( FILE_ATTRIBUTE_READONLY  ) snprintf_full( &pB, &cbB, ",RO"   );
           if( FILE_ATTRIBUTE_DIRECTORY ) snprintf_full( &pB, &cbB, ",DIR"  );

           Msg( "FindFirstFile => '%s' (%X=%s)", Win32_FindData.cFileName, Win32_FindData.dwFileAttributes, buf+1 );
           }
           return true;

      case NOARG:
           if( Win32::FindNextFile( hFindFile, &Win32_FindData ) == 0 ) {
              Win32::FindClose( hFindFile );
              hFindFile = Win32::Invalid_Handle_Value();
              return Msg( "FindNextFile => 0 (no more matches)" );
              }

           Msg( "FindNextFile => '%s'", Win32_FindData.cFileName );
           return true;

      default:
           return BadArg();
      }
   }

#endif // fn_findfile

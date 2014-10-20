#include <windows.h>
#include <stdio.h>
#include <assert.h>

#pragma hdrstop

#pragma comment(lib, "advapi32.lib")

void doerr( const char *file, int line )
{
	DWORD gle = GetLastError();
	if( gle == 0 )
		return;

	printf( "%s(%d): GetLastError => 0x%X\n", file, line, gle );
	exit( 2 );
}

#define err doerr( __FILE__, __LINE__ )

void enableprivs()
{
	HANDLE hToken;
	if( ! OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		err;

	// enable SeBackupPrivilege, SeRestorePrivilege

	byte buf[sizeof TOKEN_PRIVILEGES * 2];
	TOKEN_PRIVILEGES & tkp = *( (TOKEN_PRIVILEGES *) buf );

	if( ! LookupPrivilegeValue( NULL, SE_BACKUP_NAME , &tkp.Privileges[0].Luid ) )
		err;

	if( ! LookupPrivilegeValue( NULL, SE_RESTORE_NAME, &tkp.Privileges[1].Luid ) )
		err;

	tkp.PrivilegeCount = 2;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tkp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

	if( ! AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof tkp, NULL, NULL ) )
		err;
}

void dumphdr( WIN32_STREAM_ID & wsi )
{
	const char *pid;
	switch ( wsi.dwStreamId )
	{
		case BACKUP_DATA           : pid = "BACKUP_DATA"           ; break;
		case BACKUP_EA_DATA        : pid = "BACKUP_EA_DATA"        ; break;
		case BACKUP_SECURITY_DATA  : pid = "BACKUP_SECURITY_DATA"  ; break;
		case BACKUP_ALTERNATE_DATA : pid = "BACKUP_ALTERNATE_DATA" ; break;
		case BACKUP_LINK           : pid = "BACKUP_LINK"           ; break;
		case BACKUP_OBJECT_ID      : pid = "BACKUP_OBJECT_ID"      ; break;
		case BACKUP_PROPERTY_DATA  : pid = "BACKUP_PROPERTY_DATA"  ; break;
		case BACKUP_REPARSE_DATA   : pid = "BACKUP_REPARSE_DATA"   ; break;
		case BACKUP_SPARSE_BLOCK   : pid = "BACKUP_SPARSE_BLOCK"   ; break;
		default:                     {
			static char stbuf[32];
			_snprintf( stbuf, sizeof stbuf, "StreamID=0x%08X", wsi.dwStreamId );
			pid = stbuf;
			break;
			}
	}
	printf( "%-22s %c%c %8I64d bytes"
	   , pid
	   , (wsi.dwStreamAttributes & STREAM_MODIFIED_WHEN_READ) ? 'M':'m'
	   , (wsi.dwStreamAttributes & STREAM_CONTAINS_SECURITY ) ? 'S':'s'
	   , wsi.Size.QuadPart
	   );

	if( wsi.dwStreamNameSize ) {
		// wsi.cStreamName is a UNICODE string that is NOT NUL-TERMINATED
		// AND we did not read beyond the unterminated UNICODE string
		// "UNICODE string" apparently equates to "UTF-16"
		// so we add the UNICODE string NUL-TERMINATION now:
		((char *)wsi.cStreamName)[wsi.dwStreamNameSize+0] = 0;  // UNICODE/UTF-16 NUL char is 2 bytes of 0?
		((char *)wsi.cStreamName)[wsi.dwStreamNameSize+1] = 0;

		const int wlen( wcslen( wsi.cStreamName ) );
		printf( ", StreamName(%d)='%S'", wlen, wsi.cStreamName );
		// printf( ", StreamName='%S'", wsi.cStreamName );
		enum { StreamNamechars = 1024 };
		if( wsi.dwStreamNameSize <= StreamNamechars ) {
			char dest[ StreamNamechars + 1 ];
			WideCharToMultiByte( CP_OEMCP, 0, wsi.cStreamName,
				  wlen  // WORKS
			    //    -1    // WORKS
				, dest,
				  StreamNamechars  // WORKS
			    //    -1               // does NOT work
				, 0, 0 );
			const int slen( strlen( dest ) );
			assert( slen == wlen );
			// 20091129 kgoodwin works, commented out since duplicate info (until I actually NEED a
			//                   standard C string rep of the stream name)
			// printf( " OEM(%d)='%s'", slen, dest );
			}
		}
	printf( "\n" );
}

int main( int argc, char *argv[] )
{

	if( argc != 2 )
	{
		printf( "usage: dump_ntfs_streams {file}\n" );
		return 1;
	}

	// SeBackupPrivilege is not necessary to enumerate streams --
	// but it helps if you are an admin/backup-operator and need
	// to scan files to which you have no permissions
	enableprivs();

	HANDLE fh = CreateFile( argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL );
	if( fh == INVALID_HANDLE_VALUE || fh == NULL )
		err;

	byte buf[4096];
	DWORD numread, numtoskip;
	void *ctx = NULL;
	WIN32_STREAM_ID & wsi = *( (WIN32_STREAM_ID *) buf );
	enum { SIZEOF_WIN32_STREAM_ID = 20 };

	numtoskip = 0;
	while ( 1 )
	{
		// we are at the start of a stream header. read it.
		if( ! BackupRead( fh, buf, SIZEOF_WIN32_STREAM_ID, &numread, FALSE, TRUE, &ctx ) )
			err;

		if( numread != SIZEOF_WIN32_STREAM_ID )
			break;

		if( wsi.dwStreamNameSize > 0 )
		{
			if( wsi.dwStreamNameSize > sizeof buf - SIZEOF_WIN32_STREAM_ID )
			{
				printf( "wsi.dwStreamNameSize (%d) is too big for my buffer (%d)\n", wsi.dwStreamNameSize, sizeof buf - SIZEOF_WIN32_STREAM_ID );
				err;
			}

			if( ! BackupRead( fh, buf + SIZEOF_WIN32_STREAM_ID, wsi.dwStreamNameSize, &numread, FALSE, TRUE, &ctx ) )
				err;

			if( numread != wsi.dwStreamNameSize )
				break;
		}

		dumphdr( wsi );

		// skip stream data
		if( wsi.Size.QuadPart > 0 )
		{
			DWORD lo, hi;
			BackupSeek( fh, 0xffffffffL, 0x7fffffffL, &lo, &hi, &ctx );
		}
	}

	// make NT release the context
	BackupRead( fh, buf, 0, &numread, TRUE, FALSE, &ctx );

	CloseHandle( fh );

	return 0;
}

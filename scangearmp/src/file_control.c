/*
 *  ScanGear MP for Linux
 *  Copyright CANON INC. 2007-2010
 *  All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * NOTE:
 *  - As a special exception, this program is permissible to link with the
 *    libraries released as the binary modules.
 *  - If you write modifications of your own for these programs, it is your
 *    choice whether to permit this exception to apply to your modifications.
 *    If you do not wish that, delete this exception.
*/

#ifndef	_FILE_CONTROL_C_
#define	_FILE_CONTROL_C_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "cnmstype.h"
#include "errors.h"
#include "cnmsfunc.h"
#include "file_control.h"

/*	#define	__CNMS_DEBUG_FILE_CONTROL__	*/

#define	TEMP_FILE_FULL_NAME		"/tmp/cnms_tmp_file_XXXXXX\0"


/* for FileControlReadFileEx... */
typedef struct {
	CNMSFd		fd;
	CNMSBool	rotate;
	CNMSInt32	colors;
	CNMSInt32	width;
	CNMSInt32	height;
	CNMSInt32	width_bytes;	/* width * colors */
} FILECONTROLEX;


CNMSInt32 FileControlGetStatus(
		CNMSLPSTR		path,
		CNMSInt32		pathLen )
{
	struct stat		buf;
	CNMSLPSTR		path_dir = CNMSNULL, path_temp;
	CNMSInt32		ret = FILECONTROL_STATUS_OTHER_ERROR;
	
	if( path == CNMSNULL ){
		DBGMSG( "[FileControlGetStatus]Parameter is error.\n" );
		goto	EXIT;
	}

	if( ( path_dir = strdup( path ) ) == CNMSNULL ){
		DBGMSG( "[FileControlGetStatus]Error is occured in strdup.\n" );
		goto	EXIT;
	}
	path_temp = path_dir + strlen( path_dir );
	
	while( ( *path_temp != '/' ) && ( path_dir != path_temp ) ){
		path_temp--;
	}
	if( path_dir == path_temp ){
		if( *path_temp == '/' ){	/* ext. path = "/xxxx.png" -> path_dir = "/" */
			*( path_temp + 1 ) = '\0';
		}
		else{	/* not found "/" */
			goto	EXIT;
		}
	}
	else {	/* ext. path = "/yyyy/xxxx.png" -> path_dir = "/yyyy" */
		*path_temp = '\0';
	}
	
	if( lstat( path_dir, &buf ) != 0 ){
		/* No such directory. */
		if( errno == ENOENT ) {
			ret = FILECONTROL_STATUS_INVALID_DIR;
		}
	}
	else { 
		/* directory exists. */
		if ( access( path_dir, W_OK ) != 0 ) {
			/* cannot write to directory. */
			ret = FILECONTROL_STATUS_INVALID_DIR;
		}
		else {
			if( lstat( path, &buf ) != 0 ){
				/* No such file or directory. */
				if ( errno == ENOENT ) {
					ret = FILECONTROL_STATUS_NOT_EXIST;
				}
			}
			else {
				/* file exists. */
				if( S_ISREG( buf.st_mode ) != 0 ) { /* is regular file ? */
					if( access ( path, W_OK ) == 0 ) { /* write ok ? */
						ret = FILECONTROL_STATUS_WRITE_OK;
					}
					else {
						ret = FILECONTROL_STATUS_WRITE_NG;
					}
				}
				else {
					ret = FILECONTROL_STATUS_ISNOT_FILE;
				}
			}
		}
	}
EXIT:
	if( path_dir ){
		free( path_dir );
	}
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlGetStatus(path:%s,pathLen:%d)]=%d.\n", path, pathLen, ret );
#endif
	return	ret;
}

CNMSFd FileControlMakeTempFile(
		CNMSLPSTR		lpPath,
		CNMSInt32		pathLen )
{
	CNMSFd			retFd = CNMS_FILE_ERR;

	if( ( lpPath == CNMSNULL ) || ( pathLen < CnmsStrLen( TEMP_FILE_FULL_NAME ) ) ){
		DBGMSG( "[FileControlMakeTempFile]Parameter is error.\n" );
		goto	EXIT;
	}
	CnmsStrCopy( TEMP_FILE_FULL_NAME, lpPath, pathLen );
	if( ( retFd = mkstemp( lpPath ) ) == CNMS_FILE_ERR ){
		set_errno();
		DBGMSG( "[FileControlMakeTempFile]Can't make temp file( %s )!\n", lpPath );
	}
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlMakeTempFile(path:%s,pathLen:%d)]=%d.\n", lpPath, pathLen, retFd );
#endif
	return	retFd;
}

CNMSFd FileControlOpenFile(
		CNMSInt32		type,
		CNMSLPSTR		lpPath )
{
	CNMSFd			retFd = CNMS_FILE_ERR;
	int				flags[ FILECONTROL_OPEN_TYPE_MAX ] = {	O_RDONLY,
															O_WRONLY | O_CREAT | O_TRUNC,
															O_WRONLY | O_CREAT | O_TRUNC };
	mode_t			mode[ FILECONTROL_OPEN_TYPE_MAX ] = {	S_IRUSR,
															S_IRUSR | S_IWUSR,
															S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH };

	if( ( lpPath == CNMSNULL ) || ( type < 0 ) || ( FILECONTROL_OPEN_TYPE_MAX <= type ) ){
		DBGMSG( "[FileControlOpenFile]Parameter is error.\n" );
		goto	EXIT;
	}

	/* make dst file */
	if( ( retFd = open( lpPath, flags[ type ], mode[ type ] ) ) == CNMS_FILE_ERR ){
		set_errno();
		DBGMSG( "[FileControlOpenFile]Can't open file( %s )!\n", lpPath );
	}
	if( type != FILECONTROL_OPEN_TYPE_READ ){
		chmod( lpPath, mode[ type ] );
	}
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlOpenFile(lpPath:%s)]=%d.\n", lpPath, retFd );
#endif
	return	retFd;
}


CNMSVoid FileControlCloseFile(
		CNMSFd		fd )
{
	if( fd != CNMS_FILE_ERR ){
		close( fd );
	}
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlCloseFile(fd:%d)].\n", fd );
#endif
	return;
}

CNMSVoid FileControlDeleteFile(
		CNMSLPSTR		lpPath,
		CNMSFd			fd )
{
	if( lpPath != CNMSNULL ){
		unlink( lpPath );
	}
	FileControlCloseFile( fd );
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlDeleteFile(lpPath:%s,fd:%d)].\n", lpPath, fd );
#endif
	return;
}

CNMSInt32 FileControlReadFile(
		CNMSFd			fd,
		CNMSLPSTR		lpDst,
		CNMSInt32		readSize )
{
	CNMSInt32	ret = CNMS_ERR, ldata;

	if( ( fd == CNMS_FILE_ERR ) || ( lpDst == CNMSNULL ) || ( readSize <= 0 ) ) {
		DBGMSG( "[FileControlReadFile]Parameter is error.\n" );
		goto	EXIT;
	}
	else if( ( ldata = read( fd, lpDst, readSize ) ) < 0 ){
		set_errno();
		DBGMSG( "[FileControlReadFile]Can't read file.\n" );
		goto	EXIT;
	}
	ret = ldata;
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
/*	DBGMSG( "[FileControlReadFile(fd:%d,readSize:%d)]=%d.\n", fd, readSize, ret );	*/
#endif
	return	ret;
}

CNMSInt32 FileControlWriteFile(
		CNMSFd			fd,
		CNMSLPSTR		lpSrc,
		CNMSInt32		writeSize )
{
	CNMSInt32	ret = CNMS_ERR, ldata_1st, ldata_2nd;

	if( ( fd == CNMS_FILE_ERR ) || ( lpSrc == CNMSNULL ) || ( writeSize <= 0 ) ) {
		DBGMSG( "[FileControlWriteFile]Parameter is error.\n" );
		goto	EXIT;
	}
	else if( ( ldata_1st = write( fd, lpSrc, writeSize ) ) != writeSize ){
		set_errno();
		DBGMSG( "[FileControlWriteFile]Can't write file(1st request:%d -> write:%d).\n", writeSize, ldata_1st );
		if( ( ldata_2nd = write( fd, lpSrc+ldata_1st, writeSize-ldata_1st ) ) != writeSize-ldata_1st ){ /* For detect write() error */
			set_errno();
			DBGMSG( "[FileControlWriteFile]Can't write file(2nd request:%d -> write:%d).\n", writeSize-ldata_1st, ldata_2nd );
			goto	EXIT;
		}
	}
	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
/*	DBGMSG( "[FileControlWriteFile(fd:%d,writeSize:%d)]=%d.\n", fd, writeSize, ret );	*/
#endif
	return	ret;
}

CNMSInt32 FileControlSeekFile(
		CNMSFd			fd,
		CNMSInt32		offset,
		CNMSInt32		seekPoint )
{
	CNMSInt32	ret = CNMS_ERR, ldata, flag[ FILECONTROL_SEEK_MAX ] = { SEEK_SET, SEEK_CUR, SEEK_END };

	if( ( fd == CNMS_FILE_ERR ) || ( seekPoint < FILECONTROL_SEEK_FROM_TOP ) || ( FILECONTROL_SEEK_MAX <= seekPoint ) ) {
		DBGMSG( "[FileControlSeekFile]Parameter is error.\n" );
		goto	EXIT;
	}
	else if( ( ldata = lseek( fd, offset, flag[ seekPoint ] ) ) < 0 ){
		set_errno();
		DBGMSG( "[FileControlSeekFile]Error is occured in lseek.\n" );
		goto	EXIT;
	}
	ret = ldata;
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
/*	DBGMSG( "[FileControlSeekFile(fd:%d,offset:%d,point:%d)]=%d.\n", fd, offset, seekPoint, ret );	*/
#endif
	return	ret;
}

off_t FileControlSeekFileOFF_T(
		CNMSFd			fd,
		off_t			offset,
		CNMSInt32		seekPoint )
{
	CNMSInt32	flag[ FILECONTROL_SEEK_MAX ] = { SEEK_SET, SEEK_CUR, SEEK_END };
	off_t		ret = CNMS_ERR, ldata;

	if( ( fd == CNMS_FILE_ERR ) || ( seekPoint < FILECONTROL_SEEK_FROM_TOP ) || ( FILECONTROL_SEEK_MAX <= seekPoint ) ) {
		DBGMSG( "[FileControlSeekFile]Parameter is error.\n" );
		goto	EXIT;
	}
	else if( ( ldata = lseek( fd, offset, flag[ seekPoint ] ) ) < 0 ){
		set_errno();
		DBGMSG( "[FileControlSeekFile]Error is occured in lseek.\n" );
		goto	EXIT;
	}
	ret = ldata;
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
/*	DBGMSG( "[FileControlSeekFileOFF_T(fd:%d,offset:%d,point:%d)]=%d.\n", fd, offset, seekPoint, ret );	*/
#endif
	return	ret;
}

CNMSInt32 FileControlReadRasterString(
		CNMSFd			fd,
		CNMSLPSTR		lpDst,
		CNMSInt32		dstSize )
{
	CNMSInt32		ret = CNMS_ERR, lastOffset, readSize, i, ldata;

	if( ( fd == CNMS_FILE_ERR ) || ( lpDst == CNMSNULL ) || ( dstSize <= 0 ) ) {
		DBGMSG( "[FileControlReadRasterString]Parameter is error.\n" );
		goto	EXIT;
	}
	else if( ( lastOffset = FileControlSeekFile( fd, 0, FILECONTROL_SEEK_FROM_CURRENT ) ) < 0 ){
		DBGMSG( "[FileControlReadRasterString]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}
	else if( ( readSize = FileControlReadFile( fd, lpDst, dstSize ) ) == CNMS_ERR ){
		DBGMSG( "[FileControlReadRasterString]Error is occured in FileControlReadFile.\n" );
		goto	EXIT;
	}
	else if( readSize == 0 ){
		/* file end */
		lpDst[ 0 ] = '\0';
		ret = 0;
		goto	EXIT;
	}
	/* seek LF */
	for( i = 0 ; i < readSize ; i ++ ){
		if( lpDst[ i ] == '\n' ){
			lpDst[ i ] = '\0';
			break;
		}
	}
	if( i == readSize ){
		DBGMSG( "[FileControlReadRasterString]Raster size is over buffer.\n" );
		goto	EXIT_ERR;
	}
	else if( ( ldata = FileControlSeekFile( fd, lastOffset + i + 1, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[FileControlReadRasterString]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT_ERR;
	}
	ret = i;
EXIT:
#ifdef	__CNMS_DEBUG_FILE_CONTROL__
	DBGMSG( "[FileControlReadRasterString(fd:%d,lpDst:%s)]=%d.\n", fd, lpDst, ret );
#endif
	return	ret;

EXIT_ERR:
	FileControlSeekFile( fd, lastOffset, FILECONTROL_SEEK_FROM_TOP );
	goto	EXIT;
}


static CNMSVoid CopyMirrorData( CNMSLPSTR lpSrc, CNMSLPSTR lpDst, FILECONTROLEX *lpFCEx )
{
	CNMSInt32	i;
	
	if( lpSrc == CNMSNULL || lpDst == CNMSNULL || lpFCEx == CNMSNULL ) {
		DBGMSG( "[CopyMirrorData]Parameter is error.\n" );
		goto EXIT;
	}
	lpSrc += lpFCEx->width_bytes;
	
	if( lpFCEx->colors == 1 ) {
		for(i = 0;i < lpFCEx->width;i++ ) {
			*lpDst++ = *(--lpSrc);
		}
	}
	else if ( lpFCEx->colors == 3 ) {
		for(i = 0;i < lpFCEx->width;i++, lpDst += 3, lpSrc -= 3) {
			*(lpDst    ) = *(lpSrc - 3);
			*(lpDst + 1) = *(lpSrc - 2);
			*(lpDst + 2) = *(lpSrc - 1);
		}
	}
	
EXIT:
	return;
}

CNMSVoid *FileControlReadFileExOpen( CNMSFd fd, CNMSBool rotate, CNMSInt32 width, CNMSInt32 height, CNMSInt32 colors )
{
	FILECONTROLEX	*work = CNMSNULL;
	
	if ( fd == CNMS_FILE_ERR || width < 0 || height < 0 || ( colors != 1 && colors != 3 ) ) {
		DBGMSG( "[FileControlReadFileExOpen]Parameter is error.\n" );
		goto EXIT;
	}
	else if ( ( work = (FILECONTROLEX *)CnmsGetMem( sizeof(FILECONTROLEX) ) ) == CNMSNULL ) {
		DBGMSG( "[FileControlReadFileExOpen]Can't get work memory!\n" );
		goto EXIT;
	}
	
	work->fd			= fd;
	work->rotate		= rotate;
	work->colors		= colors;
	work->width			= width;
	work->height		= height;
	work->width_bytes	= width * colors;
	
EXIT:
	return (CNMSVoid *)work;
}

CNMSVoid FileControlReadFileExClose( CNMSVoid **work )
{
	if ( work == CNMSNULL ) {
		return;
	}
	else if ( *work != CNMSNULL ) {
		CnmsFreeMem( (CNMSLPSTR)*work );
		*work = CNMSNULL;
	}
}

CNMSInt32 FileControlReadFileExRead( CNMSVoid *work, CNMSLPSTR lpDst, CNMSInt32 start, CNMSInt32 lines )
{
	FILECONTROLEX	*lpFCEx = work;
	CNMSInt32		i, j, ldata, ret = CNMS_ERR, read_bytes, start_read;
	CNMSLPSTR		buf = CNMSNULL, lpSrc = CNMSNULL, lpDstOrg = lpDst;
	
	if ( lpFCEx == CNMSNULL || lpDst == CNMSNULL || start < 0 || lines < 0 ) {
		DBGMSG( "[FileControlReadFileExOpen]Parameter is error.\n" );
		goto EXIT;
	}
	else if ( start > lpFCEx->height - 1 ) {
		DBGMSG( "[FileControlReadFileExOpen]Parameter is error.( start > height - 1 )\n" );
		goto EXIT;
	}
	else if ( start + lines > lpFCEx->height ) {
		lines = lpFCEx->height - start;
	}
	
	read_bytes = lines * lpFCEx->width_bytes;
	
	if ( lpFCEx->rotate ) {
		if ( ( buf = CnmsGetMem( read_bytes ) ) == CNMSNULL ) {
			DBGMSG( "[FileControlReadFileExOpen]Can't get work memory!\n" );
			goto EXIT;
		}
		start_read = ( ( lpFCEx->height - 1 ) - start - ( lines - 1 ) ) * lpFCEx->width_bytes;
		
		if( ( ldata = FileControlSeekFile( lpFCEx->fd, start_read, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
			DBGMSG( "[FileControlReadFileExRead]Error is occured in FileControlSeekFile.\n" );
			goto EXIT;
		}
		else if ( ( ldata = FileControlReadFile( lpFCEx->fd, buf, read_bytes ) ) == CNMS_ERR ){
			DBGMSG( "[FileControlReadFileExRead]Error is occured in FileControlReadFile.\n" );
			goto EXIT;
		}
		else if ( ldata != read_bytes ) {
			DBGMSG( "[FileControlReadFileExRead]read bytes = %d.\n", ldata );
			goto EXIT;
		}
		
		for( i = 0, j = lines; i < lines; i++, j-- ) {
			lpSrc = buf + i * lpFCEx->width_bytes;
			lpDst = lpDstOrg + ( j - 1 ) * lpFCEx->width_bytes;
			
			CopyMirrorData( lpSrc, lpDst, lpFCEx );
		}
	}
	else {
		start_read = start * lpFCEx->width_bytes;
		
		if( ( ldata = FileControlSeekFile( lpFCEx->fd, start_read, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
			DBGMSG( "[FileControlReadFileExRead]Error is occured in FileControlSeekFile.\n" );
			goto EXIT;
		}
		else if( ( ldata = FileControlReadFile( lpFCEx->fd, lpDst, read_bytes ) ) == CNMS_ERR ){
			DBGMSG( "[FileControlReadFileExRead]Error is occured in FileControlReadFile.\n" );
			goto EXIT;
		}
		else if ( ldata != read_bytes ) {
			DBGMSG( "[FileControlReadFileExRead]read bytes = %d.\n", ldata );
			goto EXIT;
		}
	}
	
	ret = read_bytes;
EXIT:
	if( buf ) {
		CnmsFreeMem( buf );
	}
	return ret;
}


#endif	/* _FILE_CONTROL_C_ */

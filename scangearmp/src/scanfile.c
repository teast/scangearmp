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

#ifndef _SCANFILE_C_
#define _SCANFILE_C_

//#define	__CNMS_DEBUG_SCANFILE__

//#include <stdio.h>
//#include <string.h>
//#include <sys/types.h>
//#include <unistd.h>
#include <limits.h>

#include "cnmsfunc.h"
#include "png.h"
#include "errors.h"
#include "w1.h"
#include "file_control.h"
#include "save_dialog.h"
#include "progress_bar.h"
#include "raw2pdf.h"
#include "scanmsg.h"

#define GRAY2MONO_BK		(0)
#define GRAY2MONO_GRAY		(1)

#define DPI_TO_DPMETER(x) ((x)*3938/100)

#define	STATUS_SAVE_CANCELED	( -1 )

static CNMSInt32 Change_RAW_to_PNG( CNMSInt32 width, CNMSInt32 height, CNMSInt32 bpp, CNMSInt32 outRes, LPCNMS_ROOT root, CNMSFd dstFd );
static CNMSInt32 Change_RAW_to_PNM( CNMSInt32 width, CNMSInt32 height, CNMSInt32 bpp, LPCNMS_ROOT root, CNMSFd dstFd );
static CNMSInt32 Change_RAW_to_PDF( CNMSInt32 width, CNMSInt32 height, CNMSInt32 bpp, CNMSInt32 outRes, LPCNMS_ROOT root, CNMSFd dstFd );

static CNMSInt32 WritePnmHeader( CNMSInt32 width, CNMSInt32 height, CNMSInt32 max, CNMSFd fd );
static CNMSVoid Gray8ToMono1( CNMSInt32 srcsize, CNMSInt32 dstsize, CNMSUInt8* srcimg, CNMSUInt8* dstimg, CNMSInt32 type );

static CNMSInt32 SaveStatusFlag;

CNMSInt32 CnmsScanFileExec( CNMSVoid )
{
	CNMSInt32		ret = CNMS_ERR, ldata, i, outRes, bpp, file_format, errorQuit = ERROR_QUIT_NO_ERR;
	CNMSFd			scanFd = CNMS_FILE_ERR, dstFd = CNMS_FILE_ERR;
	CNMSLPSTR		lpStr = CNMSNULL;
	CNMSUIReturn	uiRtn;
	LPCNMS_ROOT		root = CNMSNULL;
	
	if( lpW1Comp == CNMSNULL ){
		goto	EXIT;
	}
	/* get param */
	else if( ( outRes = CnmsUiGetRealValue( CNMS_OBJ_A_OUTPUT_RESOLUTION, lpW1Comp->lpLink, lpW1Comp->linkNum ) ) <= 0 ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in CnmsUiGetRealValue( CNMS_OBJ_A_OUTPUT_RESOLUTION ).\n" );
		goto	EXIT;
	}
	else if( ( bpp = CnmsUiGetRealValue( CNMS_OBJ_A_COLOR_MODE, lpW1Comp->lpLink, lpW1Comp->linkNum ) ) <= 0 ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in CnmsUiGetRealValue( CNMS_OBJ_A_COLOR_MODE ).\n" );
		goto	EXIT;
	}
	/* make scan file */
	if( ( root = CnmsCreateRoot() ) == CNMSNULL ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in FileControlMakeTempFile.\n" );
		goto	EXIT_ERR;
	}
	else if( ( ldata = CnmsScanChangeStatus( CNMSSCPROC_ACTION_SCAN, CNMS_FILE_ERR, root, &uiRtn ) ) != CNMS_NO_ERR ){
		if( root->head != CNMSNULL ){
			errorQuit = ShowErrorDialog();
		}
		else{
			goto	EXIT_ERR;
		}
	}
	/* make dst file */
	if( ( lpStr = SaveDialog_GetPath() ) == CNMSNULL ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in SaveDialog_GetPath.\n" );
		goto	EXIT_ERR;
	}
	else if( ( dstFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_NEW, lpStr ) ) == CNMS_FILE_ERR ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in FileControlOpenFile.\n" );
		goto	EXIT_ERR;
	}
	else if( ( file_format = SaveDialog_GetFileFormat() ) == CNMS_ERR ){
		DBGMSG( "[CnmsScanFileExec]Error is occured in SaveDialog_GetFileFormat.\n" );
		goto	EXIT_ERR;
	}

	switch( file_format ){
		case	CNMS_SAVE_FORMAT_PNG:
			ldata = Change_RAW_to_PNG( uiRtn.ResultImgSize.Width, uiRtn.ResultImgSize.Height, bpp, outRes, root, dstFd );
			break;
		case	CNMS_SAVE_FORMAT_PNM:
			ldata = Change_RAW_to_PNM( uiRtn.ResultImgSize.Width, uiRtn.ResultImgSize.Height, bpp, root, dstFd );
			break;
		case	CNMS_SAVE_FORMAT_PDF:
			ldata = Change_RAW_to_PDF( uiRtn.ResultImgSize.Width, uiRtn.ResultImgSize.Height, bpp, outRes, root, dstFd );
			break;
		default:
			ldata = CNMS_ERR;
			break;
	}
	if( ldata != CNMS_NO_ERR ){
		if( ldata != STATUS_SAVE_CANCELED ){
			set_module_error();
		}
		DBGMSG( "[CnmsScanFileExec]Can't convert file format!\n" );
		goto	EXIT_ERR;
	}

	ret = CNMS_NO_ERR;
EXIT:
	if( ret != CNMS_NO_ERR ){
		FileControlDeleteFile( lpStr, dstFd );
	}
	else{
		FileControlCloseFile( dstFd );
	}
	CnmsAllScanDataDispose( root );

	W1_MainWindowSetSensitiveTrue();

	if( errorQuit == ERROR_QUIT_TRUE ){
		W1_Close();
	}
#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "[CnmsScanFileExec]= %d.\n", ret );
#endif
	return	ret;

EXIT_ERR:
	if( ldata != STATUS_SAVE_CANCELED ){
		errorQuit = ShowErrorDialog();
	}

	goto	EXIT;
}


/********************************************************************************************/
/*							Portable aNyMap File Format										*/
/********************************************************************************************/

static CNMSInt32 Change_RAW_to_PNM(
		CNMSInt32	width,
		CNMSInt32	height,
		CNMSInt32	bpp,
		LPCNMS_ROOT	root,
		CNMSFd		dstFd )
{
	CNMSInt32	ret = CNMS_ERR, srcSize, dstSize, monoSize, ldata;
	CNMSUInt8	*image = CNMSNULL, *image_mono = CNMSNULL;
	CNMSInt32	i;
	CNMSFd		srcFd = CNMS_FILE_ERR;
	
	if( root == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNM]Initialize parameter is error! root[%p]\n",root );
		goto	EXIT;
	}
	else if( root->head == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNM]Initialize parameter is error! root->head[%p]\n",root->head );
		goto	EXIT;
	}
	if( ( srcFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_READ, root->head->file_path ) ) == CNMS_FILE_ERR ){
		DBGMSG( "[Change_RAW_to_PNM]Can't open file( %s )!\n", root->head->file_path );
		goto	EXIT;
	}
	if( ( dstFd == CNMS_FILE_ERR ) || ( width <= 0 ) || ( height <= 0 ) ){
		DBGMSG( "[Change_RAW_to_PNM]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	else if( ( ldata = FileControlSeekFile( srcFd, 0, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[Change_RAW_to_PNM]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}
	else if( ( ldata = FileControlSeekFile( dstFd, 0, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[Change_RAW_to_PNM]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}

	if( bpp == 1 ) {
		srcSize = width;
		monoSize = ( width + 7 ) / 8;
	}
	else {
		srcSize = width * ( bpp / 8 );
	}
	
	if( ( image = (CNMSUInt8 *)CnmsGetMem( srcSize ) ) == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNM]Can't get work memory!\n" );
		goto	EXIT;
	}
	else if( bpp == 1 ){
		if( ( image_mono = (CNMSUInt8 *)CnmsGetMem( monoSize ) ) == CNMSNULL ){
			DBGMSG( "[Change_RAW_to_PNM]Can't get work memory!\n" );
			goto	EXIT;
		}
	}

	if( ( ldata = WritePnmHeader( width, height, bpp, dstFd ) ) <= 0 ){
		DBGMSG( "[Change_RAW_to_PNM]Error is occured in WritePnmHeader.\n" );
		goto	EXIT;
	}
	else if( ( ldata = ProgressBarStart( PROGRESSBAR_ID_SAVE, 0 ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Change_RAW_to_PNM]Error is occured in ProgressBarStart.\n" );
		goto	EXIT;
	}
	SaveStatusFlag = CNMS_NO_ERR;

	for( i = 0 ;i < height ; i ++ ){
		if( ( ldata = FileControlReadFile( srcFd, (CNMSLPSTR)image, srcSize ) ) == CNMS_ERR ){
			DBGMSG( "[Change_RAW_to_PNM]Error is occured in FileControlReadFile.\n" );
			goto	EXIT_2;
		}
		if ( bpp == 1 ) {
			Gray8ToMono1( srcSize, monoSize, image, image_mono, GRAY2MONO_BK );
			if( ( ldata = FileControlWriteFile( dstFd, (CNMSLPSTR)image_mono, monoSize ) ) != CNMS_NO_ERR ){
				DBGMSG( "[Change_RAW_to_PNM]Error is occured in FileControlWriteFile.\n" );
				goto	EXIT_2;
			}
		}
		else {
			if( ( ldata = FileControlWriteFile( dstFd, (CNMSLPSTR)image, srcSize ) ) != CNMS_NO_ERR ){
				DBGMSG( "[Change_RAW_to_PNM]Error is occured in FileControlWriteFile.\n" );
				goto	EXIT_2;
			}
		}
		
		if( ( ldata = ProgressBarUpdate( ( 100 * i ) / height, 0 ) ) != CNMS_NO_ERR ){
			DBGMSG( "[Change_RAW_to_PNM]Error is occured in ProgressBarUpdate!\n" );
			goto	EXIT_2;
		}
		else if( SaveStatusFlag != CNMS_NO_ERR ){	/* cancel */
			ret = STATUS_SAVE_CANCELED;
			goto	EXIT_2;
		}
	}

	ret = CNMS_NO_ERR;
EXIT_2:
	ProgressBarEnd();
EXIT:
	if( image != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)image );
	}
	FileControlDeleteFile( root->head->file_path, srcFd );
#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "\t[Change_RAW_to_PNM(width:%d,height:%d,bpp:%d]= %d.\n", width, height, bpp, ret );
#endif
	return	ret;
}

static CNMSInt32 WritePnmHeader(
		CNMSInt32		width,
		CNMSInt32		height,
		CNMSInt32		bpp,
		CNMSFd			fd )
{
	CNMSInt32		ret = CNMS_ERR, 				/* Return value */
					ldata,							/* temporary data */
					wSize,							/* Error detect use */
					max = 255;						/* Constant (Max value on 1 pixel) */
	CNMSInt8		buf[ 256 ],						/* Header string buff */
					str_type[ 8 ]  , len_type,		/* Format (mono/gray/color) */
					str_width[ 8 ] , len_width,		/* Width */
					str_height[ 8 ], len_height,	/* Height */
					str_max[ 8 ]   , len_max,		/* Max value on 1 pixel */
					ptr = 0;						/* Counter */

	strncpy( buf, "\0", 1 );	/* Buff Init */

	/* Format add */
	switch( bpp )
	{
		case  1:	/* Monochrome */
					if( (snprintf(str_type, sizeof(str_type), "P4\0")) == EOF ){
						DBGMSG( "[WritePnmHeader]Header write error!\n" );
						goto	EXIT;
					}
					break;

		case  8:	/* Grayscale */
					if( (snprintf(str_type, sizeof(str_type), "P5\0")) == EOF ){
						DBGMSG( "[WritePnmHeader]Header write error!\n" );
						goto	EXIT;
					}
					break;

		case 24:	/* Color */
					if( (snprintf(str_type, sizeof(str_type), "P6\0")) == EOF ){
						DBGMSG( "[WritePnmHeader]Header write error!\n" );
						goto	EXIT;
					}
					break;
		default:
					DBGMSG( "[WritePnmHeader]Bpp input value error!\n" );
					goto	EXIT;
					break;
	}
	if( (len_type = strlen( str_type )) == 0 )
		goto	EXIT;
	strncat( buf+ptr, str_type, len_type);
	ptr = ptr + len_type;

	/* Width add */
	if( (snprintf(str_width, sizeof(str_width), " %d\0",width)) == EOF ){
		DBGMSG( "[WritePnmHeader]Header write error!\n" );
		goto	EXIT;
	}
	if( (len_width = strlen( str_width )) == 0 ){
		DBGMSG( "[WritePnmHeader]Header write error!\n" );
		goto	EXIT;
	}
	strncat( buf+ptr, str_width, len_width);
	ptr = ptr + len_width;

	/* Height add */
	if( (snprintf(str_height, sizeof(str_height), " %d\0",height)) == EOF ){
		DBGMSG( "[WritePnmHeader]Header write error!\n" );
		goto	EXIT;
	}
	if( (len_height = strlen( str_height )) == 0 ){
		DBGMSG( "[WritePnmHeader]Header write error!\n" );
		goto	EXIT;
	}
	strncat( buf+ptr, str_height, len_height);
	ptr = ptr + len_height;

	/* Max value add */
	if( ( bpp == 8 ) || ( bpp == 24 ) ){
		if( (snprintf(str_max, sizeof(str_max), " %d\n\0",max)) == EOF ){
			DBGMSG( "[WritePnmHeader]Header write error!\n" );
			goto	EXIT;
		}
	}
	else{
		if( (snprintf(str_max, sizeof(str_max), "\n\0")) == EOF ){
			DBGMSG( "[WritePnmHeader]Header write error!\n" );
			goto	EXIT;
		}
	}
	if( (len_max = strlen( str_max )) == 0 )
		goto	EXIT;
	strncat( buf+ptr, str_max, len_max);

	/* Header write */
	wSize = len_type + len_width + len_height + len_max;
	if( ( ldata = FileControlWriteFile( fd, buf, wSize ) ) != CNMS_NO_ERR ){
		DBGMSG( "[WritePnmHeader]Error is occured in FileControlWriteFile.\n" );
		goto	EXIT;
	}
	ret = wSize;
EXIT:
#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "\t[WritePnmHeader(width:%d,height:%d,bpp:%d]= %d.\n", width, height, bpp, ret );
#endif
	return	ret;
}


/********************************************************************************************/
/*							Portable Network Graphics File Format							*/
/********************************************************************************************/

void write_data_for_png( png_structp png_ptr, png_bytep data, png_size_t length )
{
	FileControlWriteFile( *( (int *)png_ptr->io_ptr ), (CNMSLPSTR)data, length );

	return;
}

static CNMSInt32 Change_RAW_to_PNG(
		CNMSInt32	width,
		CNMSInt32	height,
		CNMSInt32	bpp,
		CNMSInt32	outRes,
		LPCNMS_ROOT root,
		CNMSFd		dstFd )
{
	CNMSInt32	ret = CNMS_ERR, i, srcSize, monoSize, ldata, pack_size = 0;
	CNMSUInt8	*image = CNMSNULL, *image_mono = CNMSNULL;
	png_structp	write_ptr;
	png_infop	info_ptr;
	CNMSFd		srcFd = CNMS_FILE_ERR;

	if( root == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNG]Initialize parameter is error! root[%p]\n",root );
		goto	EXIT;
	}
	else if( root->head == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNG]Initialize parameter is error! root->head[%p]\n",root->head );
		goto	EXIT;
	}
	if( ( srcFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_READ, root->head->file_path ) ) == CNMS_FILE_ERR ){
		DBGMSG( "[Change_RAW_to_PNG]Can't open file( %s )!\n", root->head->file_path );
		goto	EXIT;
	}
	if( ( dstFd == CNMS_FILE_ERR ) || ( width <= 0 ) || ( height <= 0 ) ){
		DBGMSG( "[Change_RAW_to_PNG]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	else if( ( ldata = FileControlSeekFile( srcFd, 0, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[Change_RAW_to_PNG]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}
	else if( ( ldata = FileControlSeekFile( dstFd, 0, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[Change_RAW_to_PNG]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}

	if( bpp == 1 ) {
		srcSize = width;
		monoSize = ( width + 7 ) / 8;
	}
	else {
		srcSize = width * ( bpp / 8 );
	}
	
	if( ( image = (CNMSUInt8 *)CnmsGetMem( srcSize ) ) == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNG]Can't get work memory!\n" );
		goto	EXIT;
	}
	else if( bpp == 1 ){
		if( ( image_mono = (CNMSUInt8 *)CnmsGetMem( monoSize ) ) == CNMSNULL ){
			DBGMSG( "[Change_RAW_to_PNG]Can't get work memory!\n" );
			goto	EXIT;
		}
	}
	
	if( ( write_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, CNMSNULL, CNMSNULL, CNMSNULL ) ) == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNG]Can't png_create_write_struct!\n" );
		goto	EXIT;
	}
	else if( ( info_ptr = png_create_info_struct( write_ptr ) ) == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PNG]png_create_info_struct!\n" );
		png_destroy_write_struct( &write_ptr, CNMSNULL );
		goto	EXIT;
	}
	
	png_set_write_fn( write_ptr, (png_voidp)&dstFd, write_data_for_png, NULL );
	png_set_compression_level( write_ptr, Z_BEST_SPEED );	/* maximum speed */
	switch( bpp )
	{
		case  1:
				png_set_IHDR(write_ptr, info_ptr, width, height, 1, PNG_COLOR_TYPE_GRAY, 0, 0, 0);
				break;

		case  8:
				png_set_IHDR(write_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, 0, 0, 0);
				break;

		case 24:
				png_set_IHDR(write_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, 0, 0, 0);
				break;

		default:
				DBGMSG( "[Change_RAW_to_PNG]Bpp input value error!\n" );
				goto	EXIT;
	}	
	
	png_set_pHYs(write_ptr, info_ptr, DPI_TO_DPMETER(outRes), DPI_TO_DPMETER(outRes), PNG_RESOLUTION_METER);
	
	png_write_info(write_ptr, info_ptr);
	if ( lastIOErrCode ) {
		goto EXIT;
	}
	if( ( ldata = ProgressBarStart( PROGRESSBAR_ID_SAVE, 0 ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Change_RAW_to_PNG]Error is occured in ProgressBarStart.\n" );
		goto	EXIT;
	}
	SaveStatusFlag = CNMS_NO_ERR;

	for( i = 0;i < height;i++ ) {
		if( ( ldata = FileControlReadFile( srcFd, (CNMSLPSTR)image, srcSize ) ) == CNMS_ERR ){
			DBGMSG( "[Change_RAW_to_PNG]Error is occured in FileControlReadFile.\n" );
			goto	EXIT_2;
		}
		if ( bpp == 1 ) {
			Gray8ToMono1( srcSize, monoSize, image, image_mono, GRAY2MONO_GRAY );
			png_write_row( write_ptr, image_mono );
		}
		else {
			png_write_row( write_ptr, image );
		}
		if ( lastIOErrCode ) {
			goto	EXIT_2;
		}
		
		if( ( ldata = ProgressBarUpdate( ( 100 * i ) / height, 0 ) ) != CNMS_NO_ERR ){
			DBGMSG( "[Change_RAW_to_PNG]Error is occured in ProgressBarUpdate!\n" );
			goto	EXIT_2;
		}
		else if( SaveStatusFlag != CNMS_NO_ERR ){	/* cancel */
			ret = STATUS_SAVE_CANCELED;
			goto	EXIT_2;
		}
	}
	png_write_end(write_ptr, info_ptr);
	if ( lastIOErrCode ) {
		goto	EXIT_2;
	}
	png_destroy_write_struct(&write_ptr, &info_ptr);

	ret = CNMS_NO_ERR;
EXIT_2:
	ProgressBarEnd();
EXIT:
	if( image != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)image );
	}
	if( image_mono != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)image_mono );
	}
	FileControlDeleteFile( root->head->file_path, srcFd );
#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "\t[Change_RAW_to_PNG(width:%d,height:%d,bpp:%d,outRes:%d)]= %d.\n", width, height, bpp, outRes, ret );
#endif
	return	ret;
}


/********************************************************************************************/
/*								Portable Document Format									*/
/********************************************************************************************/

static CNMSInt32 Change_RAW_to_PDF(
		CNMSInt32	width,
		CNMSInt32	height,
		CNMSInt32	bpp,
		CNMSInt32	outRes,
		LPCNMS_ROOT	root,
		CNMSFd		dstFd )
{
	CNMSInt32	ret = CNMS_ERR, i, srcSize, monoSize, ldata, pack_size = 0, pageMax, page, page_progress;;
	CNMSLPSTR	image = CNMSNULL;
	CNMSUInt8	*image_mono = CNMSNULL;
	CNMSInt32	type;
	CNMSVoid	*pdf = CNMSNULL;
	CNMSFd		srcFd = CNMS_FILE_ERR;
	CNMSVoid	*fcex = CNMSNULL;
	CNMSInt32	colors;
	
	if( root == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PDF]Initialize parameter is error! root[%p]\n",root );
		goto	EXIT;
	}
	else if( root->head == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PDF]Initialize parameter is error! root->head[%p]\n",root->head );
		goto	EXIT;
	}
	if( ( dstFd == CNMS_FILE_ERR ) || ( width <= 0 ) || ( height <= 0 ) ){
		DBGMSG( "[Change_RAW_to_PDF]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	else if( ( ldata = FileControlSeekFile( dstFd, 0, FILECONTROL_SEEK_FROM_TOP ) ) < 0 ){
		DBGMSG( "[Change_RAW_to_PDF]Error is occured in FileControlSeekFile.\n" );
		goto	EXIT;
	}

	if( bpp == 1 ) {
		srcSize = width;
		monoSize = ( width + 7 ) / 8;
	}
	else {
		srcSize = width * ( bpp / 8 );
	}
	
	switch( bpp )
	{
		case  1:
				type = CNMS_PDF_IMAGE_MONO;
				colors = 1;
				break;

		case  8:
				type = CNMS_PDF_IMAGE_GRAY;
				colors = 1;
				break;

		case 24:
				type = CNMS_PDF_IMAGE_COLOR;
				colors = 3;
				break;

		default:
				DBGMSG( "[Change_RAW_to_PDF]Bpp input value error!\n" );
				goto	EXIT;
	}	
	
	if( ( ldata = CnmsPDF_Open( &pdf, dstFd ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Change_RAW_to_PDF]Error is occured in CnmsPDF_Open.\n" );
		goto	EXIT;
	}
	else if( ( ldata = CnmsPDF_StartDoc( pdf ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Change_RAW_to_PDF]Error is occured in CnmsPDF_StartDoc.\n" );
		goto	EXIT_1;
	}

	if( ( ldata = ProgressBarStart( PROGRESSBAR_ID_SAVE, 0 ) ) != CNMS_NO_ERR ){	/* <- page */
		DBGMSG( "[Change_RAW_to_PDF]Error is occured in ProgressBarStart.\n" );
		goto	EXIT_1;
	}

	pageMax = root->page_num;
	if( ( image = (CNMSLPSTR)CnmsGetMem( srcSize ) ) == CNMSNULL ){
		DBGMSG( "[Change_RAW_to_PDF]Can't get work memory!\n" );
		goto	EXIT_2;
	}
	else if( bpp == 1 ){
		if( ( image_mono = (CNMSUInt8 *)CnmsGetMem( monoSize ) ) == CNMSNULL ){
			DBGMSG( "[Change_RAW_to_PDF]Can't get work memory!\n" );
			goto	EXIT_2;
		}
	}
		
	while( root->head != CNMSNULL ){
		page = root->head->page;
		page_progress = root->head->show_page ? page : 0;
		
		if( ( srcFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_READ, root->head->file_path ) ) == CNMS_FILE_ERR ){
			DBGMSG( "[Change_RAW_to_PNM]Can't open file( %s )!\n", root->head->file_path );
			goto	EXIT_2;
		}
		else if( ( fcex = FileControlReadFileExOpen( srcFd, root->head->rotate, width, height, colors ) ) == CNMSNULL ) {
			DBGMSG( "[Change_RAW_to_PDF]Error is occured in FileControlReadFileExOpen.\n" );
			goto	EXIT;
		}

		if( ( ldata = CnmsPDF_StartPage( pdf, width, height, outRes, type ) ) != CNMS_NO_ERR ){
			DBGMSG( "[Change_RAW_to_PDF]Error is occured in CnmsPDF_StartDoc.\n" );
			goto	EXIT_2;
		}
		SaveStatusFlag = CNMS_NO_ERR;

		for( i = 0;i < height;i++ ) {
			if( ( ldata = FileControlReadFileExRead( fcex, image, i, 1 ) ) == CNMS_ERR ){
				DBGMSG( "[Change_RAW_to_PDF]Error is occured in FileControlReadFile.\n" );
				goto	EXIT_2;
			}
			if ( bpp == 1 ) {
				Gray8ToMono1( srcSize, monoSize, (CNMSUInt8 *)image, image_mono, GRAY2MONO_GRAY );
				if( ( ldata = CnmsPDF_WriteRowData( pdf, (CNMSLPSTR)image_mono ) ) != CNMS_NO_ERR ){
					DBGMSG( "[Change_RAW_to_PDF]Error is occured in FileControlReadFile.\n" );
					goto	EXIT_2;
				}
			}
			else {
				if( ( ldata = CnmsPDF_WriteRowData( pdf, image ) ) != CNMS_NO_ERR ){
					DBGMSG( "[Change_RAW_to_PDF]Error is occured in FileControlReadFile.\n" );
					goto	EXIT_2;
				}
			}
			
			if( ( ldata = ProgressBarUpdate( ( ( 100 * i ) / ( height * pageMax ) ) + ( ( ( page - 1 ) * 100 ) / pageMax ), page_progress ) ) != CNMS_NO_ERR ){	/* <- page */
				DBGMSG( "[Change_RAW_to_PDF]Error is occured in ProgressBarUpdate!\n" );
				goto	EXIT_2;
			}
			else if( SaveStatusFlag != CNMS_NO_ERR ){	/* cancel */
				ret = STATUS_SAVE_CANCELED;
				goto	EXIT_2;
			}
		}
		if( ( ldata = CnmsPDF_EndPage( pdf ) ) != CNMS_NO_ERR ){
			DBGMSG( "[Change_RAW_to_PDF]Error is occured in CnmsPDF_EndPage.\n" );
			goto	EXIT_2;
		}
		FileControlDeleteFile( root->head->file_path, srcFd );
		CnmsDisposeQueue( root, CNMS_NODE_HEAD );
		FileControlReadFileExClose( &fcex );
	}

	if( ( ldata = CnmsPDF_EndDoc( pdf ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Change_RAW_to_PDF]Error is occured in CnmsPDF_EndDoc.\n" );
		goto	EXIT_2;
	}
	
	ret = CNMS_NO_ERR;
EXIT_2:
	ProgressBarEnd();
EXIT_1:
	CnmsPDF_Close( pdf );
EXIT:
	if( image != CNMSNULL ){
		CnmsFreeMem( image );
	}
	if( image_mono != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)image_mono );
	}
	if( fcex ) {
		FileControlReadFileExClose( &fcex );
	}
#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "\t[Change_RAW_to_PDF(width:%d,height:%d,bpp:%d,outRes:%d)]= %d.\n", width, height, bpp, outRes, ret );
#endif
	return	ret;
}


static CNMSVoid Gray8ToMono1(
		CNMSInt32		srcsize,
		CNMSInt32		dstsize,
		CNMSUInt8*		srcimg,
		CNMSUInt8*		dstimg,
		CNMSInt32		type )
{
	CNMSInt32	i, j, remain;
	CNMSUInt8	buf;
	
	switch ( type ) {
		case GRAY2MONO_BK :
			for( i = 0; i < (srcsize / 8); i++ ) {
				for( j = 0, buf = 0; j < 7; j++, buf <<= 1 ) {
					if( ! srcimg[i * 8 + j] ) {
						buf++;
					}
				}
				if ( ! srcimg[i * 8 + j] ) {
					buf++;
				}
				dstimg[i] = buf;
			}
			remain = dstsize - (srcsize / 8);
			if ( remain ) {
				for( j = 0, buf = 0; j < remain; j++, buf <<= 1 ) {
					if( ! srcimg[i * 8 + j] ) {
						buf++;
					}
				}
				buf <<= 7 - remain;
				dstimg[i] = buf;
			}
			break;
			
		case GRAY2MONO_GRAY :
			for( i = 0; i < (srcsize / 8); i++ ) {
				for( j = 0, buf = 0; j < 7; j++, buf <<= 1 ) {
					if( srcimg[i * 8 + j] ) {
						buf++;
					}
				}
				if ( srcimg[i * 8 + j] ) {
					buf++;
				}
				dstimg[i] = buf;
			}
			remain = dstsize - (srcsize / 8);
			if ( remain ) {
				for( j = 0, buf = 0; j < remain; j++, buf <<= 1 ) {
					if( srcimg[i * 8 + j] ) {
						buf++;
					}
				}
				buf <<= 7 - remain;
				dstimg[i] = buf;
			}
			break;
			
		default :
			break;
	}
}

CNMSVoid CnmsScanFileCancel( CNMSVoid )
{
	SaveStatusFlag = CNMS_ERR;

#ifdef	__CNMS_DEBUG_SCANFILE__
	DBGMSG( "[CnmsScanFileCancel].\n" );
#endif
	return;
}

#endif	/* _SCANFILE_C_ */


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

#ifndef _RAW2PDF_C_
#define _RAW2PDF_C_

#include <stdio.h>
#include <time.h>
#include <zlib.h>

#include "cnmstype.h"
#include "cnmsfunc.h"
#include "file_control.h"
#include "raw2pdf.h"


/* PDF File Header */
#define CNMS_PDF_HEADER "%%PDF-1.3\n"

/* trailer format */
#define CNMS_PDF_TRAILER_OBJ "trailer\n<<\n/Size %d\n/Root 1 0 R\n/Info 3 0 R\n>>\nstartxref\n%lld\n%%%%EOF\n"

/* xref format */
#define CNMS_PDF_XREF_OBJ1 "xref\n0 %d\n0000000000 65535 f \n"
#define CNMS_PDF_XREF_OBJ2 "%010lld 00000 n \n"

/* Catalog format */
#define CNMS_PDF_CATALOG_OBJ "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"

/* Pages format */
#define CNMS_PDF_PAGES_OBJ1 "2 0 obj\n<<\n/Type /Pages\n/Kids [ "
#define CNMS_PDF_PAGES_OBJ2 "%d 0 R "
#define CNMS_PDF_PAGES_OBJ3 "]\n/Count %d\n>>\nendobj\n"

/* Info format */
#define CNMS_PDF_INFO_OBJ "3 0 obj\n<<\n/Creator (scangearmp)\n/Producer (scangearmp)\n/CreationDate %s\n/ModDate %s\n>>\nendobj\n"
#define CNMS_PDF_INFO_DATES "(D:%4d%02d%02d%02d%02d%02d%c%02d'%02d')"

/* Page format */
#define CNMS_PDF_PAGE_OBJ1 "%d 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n"
#define CNMS_PDF_PAGE_OBJ2 "/Resources\n<<\n/XObject << /Im%d %d 0 R >>\n/ProcSet [ /PDF /%s ]\n>>\n"
#define CNMS_PDF_PAGE_OBJ3 "/MediaBox [ 0 0 %d %d ]\n/Contents %d 0 R\n>>\nendobj\n"
#define CNMS_PDF_PAGE_OBJ	CNMS_PDF_PAGE_OBJ1 CNMS_PDF_PAGE_OBJ2 CNMS_PDF_PAGE_OBJ3

/* Contents format */
#define CNMS_PDF_CONTENTS_OBJ1 "%d 0 obj\n<< /Length %d 0 R >>\nstream\n"
#define CNMS_PDF_CONTENTS_OBJ2 "q\n%d 0 0 %d 0 0 cm\n/Im%d Do\nQ\n"

/* XObject(Image) format */
#define CNMS_PDF_IMAGE_OBJ1 "%d 0 obj\n<<\n/Length %d 0 R\n/Type /XObject\n/Subtype /Image\n"
#define CNMS_PDF_IMAGE_OBJ2 "/Width %d /Height %d\n/ColorSpace /%s\n/BitsPerComponent %d\n"
#define CNMS_PDF_IMAGE_OBJ3 "/Filter /FlateDecode\n>>\nstream\n"
#define CNMS_PDF_IMAGE_OBJ	CNMS_PDF_IMAGE_OBJ1 CNMS_PDF_IMAGE_OBJ2 CNMS_PDF_IMAGE_OBJ3

/* Length format */
#define CNMS_PDF_LENGTH_OBJ "%d 0 obj\n%d\nendobj\n"

/* end of stream/object */
#define CNMS_PDF_END_ST_OBJ "endstream\nendobj\n"


/* object id of first page */
#define CNMS_PDF_FIRST_PAGE_ID (4)

/* xref max value */
#define CNMS_PDF_XREF_MAX (9999999999LL)

/* pdfwork->offset_table */
enum {
	CNMS_PDF_ENDDOC_XREF = 0,
	CNMS_PDF_ENDDOC_CATALOG,
	CNMS_PDF_ENDDOC_PAGES,
	CNMS_PDF_ENDDOC_INFO,
	CNMS_PDF_ENDDOC_NUM,
};

/* pdfpage->offset_table */
enum {
	CNMS_PDF_PAGE_OBJ_PAGE = 0,
	CNMS_PDF_PAGE_OBJ_IMAGE,
	CNMS_PDF_PAGE_OBJ_IMAGE_LEN,
	CNMS_PDF_PAGE_OBJ_CONTENTS,
	CNMS_PDF_PAGE_OBJ_CONTENTS_LEN,
	CNMS_PDF_PAGE_OBJ_NUM,
};

/* Page object info */
typedef struct cnms_pdf_page {
	CNMSInt32		page;			/* page No. */
	CNMSInt32		obj_id;			/* Page object id */
	CNMSInt32		image_type;		/* ColorSpace, BitsPerComponent */
	CNMSInt32		res;			/* image resolution */
	CNMSInt32		w;				/* width (image res) */
	CNMSInt32		h;				/* height (image res) */
	CNMSInt32		w_72;			/* width (72dpi) */
	CNMSInt32		h_72;			/* height (72dpi) */
	CNMSInt64		offset_table[CNMS_PDF_PAGE_OBJ_NUM];	/* xref table */
	CNMSInt32		stream_len;		/* stream object length */
	CNMSInt32		w_bytes;		/* bytes of row data */
	CNMSInt32		zlib_init;		/* zlib initialized flag */
	z_stream		zs;				/* zlib stream */
	CNMSInt32		buf_size;		/* zlib output buffer size */
	CNMSLPSTR		buf;			/* zlib output buffer (stream object) */
	CNMSInt32		line_count;		/* line counter (image res) */
	CNMSInt32		status;			/* page object status */
	struct cnms_pdf_page	*prev;	/* previous page data */
	struct cnms_pdf_page	*next;	/* next page data */
} CNMSPdfPage;


/* PDF Work */
typedef struct {
	CNMSInt32		obj_num;		/* xref - num, trailer - Size */
	CNMSInt32		page_num;		/* Pages - Count */
	CNMSInt64		offset_table[CNMS_PDF_ENDDOC_NUM];	/* xref table */
	CNMSPdfPage		*first;			/* first page data */
	CNMSPdfPage		*last;			/* last page data */
	CNMSFd			fd;				/* destination file */
} CNMSPdfWork;


static CNMSInt64 GetCurrentOffset( CNMSInt32 fd )
{
	CNMSInt64	offset64 = (CNMSInt64)FileControlSeekFileOFF_T( fd, 0, FILECONTROL_SEEK_FROM_CURRENT );
	
	if ( offset64 > CNMS_PDF_XREF_MAX ) offset64 = -1;
	
	return offset64;
}

static CNMSInt32 GetCurrentTime( struct tm *pt, CNMSByte *sign_c, CNMSInt32 *ptz_h, CNMSInt32 *ptz_m )
{
	CNMSInt32		ret = CNMS_ERR;
	time_t			t;
	long			tz;
	
	if ( pt == CNMSNULL || sign_c == CNMSNULL || ptz_h == CNMSNULL || ptz_m == CNMSNULL ) {
		goto EXIT;
	}
	
	CnmsSetMem( (CNMSLPSTR)pt, 0, sizeof(struct tm) );
	/* get time */
	if( ( t = time( NULL ) ) < 0 ) {
		DBGMSG( "[GetCurrentTime]Can't get time.\n" );
		goto EXIT;
	}
	/* get localtime */
	if ( localtime_r( &t, pt ) == NULL ) {
		DBGMSG( "[GetCurrentTime]Can't get localtime.\n" );
		goto EXIT;
	}
	/* get time difference ( OHH'mm' ) */
	tz = timezone;
	if ( tz > 0 ) {
		*sign_c = '-';
	}
	else {
		tz = -tz;
		*sign_c = '+';
	}
	*ptz_h = tz / 60 / 60;
	*ptz_m = ( tz / 60 ) % 60;

	ret = CNMS_NO_ERR;
EXIT:
	return ret;
}

CNMSInt32 CnmsPDF_Open( CNMSVoid **ppw, CNMSFd fd )
{
	CNMSInt32		ret = CNMS_ERR, i;
	CNMSPdfWork		*p = CNMSNULL;
	
	if ( fd == CNMS_FILE_ERR ) {
		DBGMSG( "[CnmsPDF_Open]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	else if ( ( p = (CNMSPdfWork *)CnmsGetMem( sizeof(CNMSPdfPage) ) ) == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_Open]Can't get work memory!\n" );
		goto	EXIT;
	}
	
	p->fd = fd;
	p->obj_num = CNMS_PDF_FIRST_PAGE_ID - 1;	/* Catalog, Pages, Info */
	p->page_num = 0;
	p->first = CNMSNULL;
	p->last = CNMSNULL;
	
	*ppw = (CNMSVoid *)p;
	
	ret = CNMS_NO_ERR;
EXIT:
	return ret;
}

CNMSVoid CnmsPDF_Close( CNMSVoid *pw )
{
	CNMSPdfPage		*cur, *next;
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;
	
	if ( pwork == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_Close]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	
	cur = pwork->first;
	while ( cur != CNMSNULL ) {
		next = cur->next;
		CnmsFreeMem( cur->buf );
		CnmsFreeMem( (CNMSLPSTR)cur );
		cur = next;
	}
	
	CnmsFreeMem( (CNMSLPSTR)pwork );
	
EXIT:
	return ;
}

CNMSInt32 CnmsPDF_StartDoc( CNMSVoid *pw )
{
	CNMSInt32		ret = CNMS_ERR, ldata;
	CNMSByte		str[32];
	CNMSInt			len;
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;
	
	if ( pwork == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_StartDoc]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	
	len = snprintf( str, sizeof(str), CNMS_PDF_HEADER );
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	ret = CNMS_NO_ERR;
EXIT:
	return ret;
}

CNMSInt32 CnmsPDF_EndDoc( CNMSVoid *pw )
{
	CNMSInt32		ret = CNMS_ERR, ldata, i, size, w_count;
	CNMSPdfPage		*p = CNMSNULL;
	CNMSByte		str[1024], str_t[64];
	CNMSInt			len;
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;

	struct tm		tm;
	CNMSByte		sign_c;
	CNMSInt32		tz_h = 0, tz_m = 0;

	if ( pwork == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_EndDoc]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	
	size = pwork->obj_num + 1;
	w_count = 1;
	
	/* <1> Pages */
	if ( ( pwork->offset_table[ CNMS_PDF_ENDDOC_PAGES ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Pages(1) */
	len = snprintf( str, sizeof(str), CNMS_PDF_PAGES_OBJ1 );
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* write Pages(2) ... Kids array */
	p = pwork->first;
	i = 0;
	while ( p != CNMSNULL ) {
		i++;
		if ( p->status != CNMS_NO_ERR ) {
			DBGMSG( "[CnmsPDF_EndDoc]page(%d) is NG!\n", i );
			goto EXIT;
		}
	
		len = snprintf( str, sizeof(str), CNMS_PDF_PAGES_OBJ2, p->obj_id );	/* Page object id */
		if ( len >= sizeof(str) || len < 0 ) {
			DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
			goto EXIT;
		}
		if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
			DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
			goto EXIT;
		}
		
		p = p->next;
	}
	
	/* write Pages(3) */
	len = snprintf( str, sizeof(str), CNMS_PDF_PAGES_OBJ3, pwork->page_num );	/* Count */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}

	/* <2> Catalog */
	if ( ( pwork->offset_table[ CNMS_PDF_ENDDOC_CATALOG ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Catalog */
	len = snprintf( str, sizeof(str), CNMS_PDF_CATALOG_OBJ );
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}

	/* <3> Info */
	if ( ( pwork->offset_table[ CNMS_PDF_ENDDOC_INFO ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	if ( GetCurrentTime( &tm, &sign_c, &tz_h, &tz_m ) == CNMS_ERR ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in GetCurrentTime.\n" );
		goto EXIT;
	}
	/* Dates format */
	len = snprintf(str_t, sizeof(str_t), CNMS_PDF_INFO_DATES,
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, sign_c, tz_h, tz_m );
	if ( len >= sizeof(str_t) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	/* write Info */
	len = snprintf( str, sizeof(str), CNMS_PDF_INFO_OBJ, str_t, str_t);		/* CreationDate, ModDate */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}

	/* <4> xref */
	if ( ( pwork->offset_table[ CNMS_PDF_ENDDOC_XREF ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write xref(1) */
	len = snprintf( str, sizeof(str), CNMS_PDF_XREF_OBJ1, size );	/* object num */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* write xref(2) */
	len = snprintf( str, sizeof(str), CNMS_PDF_XREF_OBJ2 CNMS_PDF_XREF_OBJ2 CNMS_PDF_XREF_OBJ2,
			pwork->offset_table[ CNMS_PDF_ENDDOC_CATALOG ],			/* object id = 1 : Catalog */
			pwork->offset_table[ CNMS_PDF_ENDDOC_PAGES ],			/* object id = 2 : Pages */
			pwork->offset_table[ CNMS_PDF_ENDDOC_INFO ] );			/* object id = 3 : Info */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	w_count += CNMS_PDF_FIRST_PAGE_ID - 1; 
	
	/* write xref(3) */
	p = pwork->first;
	while ( p != CNMSNULL ) {
		/* write offset : CNMS_PDF_PAGE_OBJ_PAGE -> CNMS_PDF_PAGE_OBJ_CONTENTS_LEN */
		for ( i = 0; i < CNMS_PDF_PAGE_OBJ_NUM; i++ ) {
			len = snprintf( str, sizeof(str), CNMS_PDF_XREF_OBJ2, p->offset_table[ i ] );	/* object id = 3 ~ */
			if ( len >= sizeof(str) || len < 0 ) {
				DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
				goto EXIT;
			}
			if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
				DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
				goto EXIT;
			}
			w_count ++;
		}
		p = p->next;
	}
	/* check object number */
	if ( w_count != size ) {
		DBGMSG( "[CnmsPDF_EndDoc]object number is wrong.\n" );
		goto EXIT;
	}
	
	/* <4> trailer */
	len = snprintf( str, sizeof(str), CNMS_PDF_TRAILER_OBJ,
			size,												/* object num */
			pwork->offset_table[ CNMS_PDF_ENDDOC_XREF ] );		/* xref offset */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndDoc]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	
	ret = CNMS_NO_ERR;
EXIT:
	return ret;
}

CNMSInt32 CnmsPDF_StartPage( 
	CNMSVoid		*pw,
	CNMSInt32		w,
	CNMSInt32		h,
	CNMSInt32		res,
	CNMSInt32		type )
{
	CNMSInt32		ret = CNMS_ERR, ldata, buf_size;
	CNMSPdfPage		*p = CNMSNULL, first_page;
	CNMSByte		str[1024];
	CNMSInt			len, len_c;
	CNMSByte		*ProcSetImage[CNMS_PDF_IMAGE_NUM]		= { "ImageC", "ImageG", "ImageG" };
	CNMSByte		*ColorSpace[CNMS_PDF_IMAGE_NUM]			= { "DeviceRGB", "DeviceGray", "DeviceGray" };
	CNMSInt32		BitsPerComponent[CNMS_PDF_IMAGE_NUM]	= { 8, 8, 1 };
	CNMSInt32		ComponentNum[CNMS_PDF_IMAGE_NUM]		= { 3, 1, 1 };
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;
	
	if ( pwork == CNMSNULL || w <= 0 || h <= 0 || res <= 0 ||
			!( type == CNMS_PDF_IMAGE_COLOR || type == CNMS_PDF_IMAGE_GRAY || type == CNMS_PDF_IMAGE_MONO ) ) {
		DBGMSG( "[CnmsPDF_StartPage]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	else if ( ( p = (CNMSPdfPage *)CnmsGetMem( sizeof(CNMSPdfPage) ) ) == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_StartPage]Can't get work memory!\n" );
		goto	EXIT;
	}
	
	/* p->buf : zlib dest buf */
	p->w_bytes = ( w * ComponentNum[ type ] + 8 - BitsPerComponent[ type ] ) * BitsPerComponent[ type ] / 8;
	p->buf_size = p->w_bytes * 4;
	
	if ( ( p->buf = (CNMSLPSTR)CnmsGetMem( p->buf_size ) ) == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_StartPage]Can't get work memory!\n" );
		goto	EXIT;
	}
	
	pwork->obj_num += CNMS_PDF_PAGE_OBJ_NUM;
	pwork->page_num ++;
	
	p->prev = p->next = CNMSNULL;
	if ( pwork->first == CNMSNULL ) {
		/* append first page */
		pwork->first = p;
	}
	if ( pwork->last == CNMSNULL ) {
		/* append first page */
		pwork->last = p;
	}
	else {
		/* append page */
		pwork->last->next = p;
		p->prev = pwork->last;
		pwork->last = p;
	}
	
	p->page = pwork->page_num;
	/* page obj id : page1=4, page2=4+5=9, page3=4+5*2=14, ... */
	p->obj_id = CNMS_PDF_FIRST_PAGE_ID + ( p->page - 1 ) * CNMS_PDF_PAGE_OBJ_NUM;
	p->image_type = type;
	p->res = res;
	p->w = w; p->h = h;
	p->w_72 = w * 72 / res; p->h_72 = h * 72 / res;
	p->stream_len = 0;
	p->zlib_init = CNMS_FALSE;
	p->line_count = 0;
	p->status = CNMS_ERR;
	
	/* <1> Page */
	if ( ( p->offset_table[ CNMS_PDF_PAGE_OBJ_PAGE ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Page */
	len = snprintf( str, sizeof(str), CNMS_PDF_PAGE_OBJ,
			p->obj_id + CNMS_PDF_PAGE_OBJ_PAGE,			/* object id ( Page ) */
			p->page,									/* ImX (X = page number) ... XObject/Image Name */
			p->obj_id + CNMS_PDF_PAGE_OBJ_IMAGE,		/* object id ( XObject/Image ) */
			ProcSetImage[ type ],						/* ProcSet */
			p->w_72, p->h_72,							/* MediaBox */
			p->obj_id + CNMS_PDF_PAGE_OBJ_CONTENTS );	/* object id ( Contents ) */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* <2> Contents */
	if ( ( p->offset_table[ CNMS_PDF_PAGE_OBJ_CONTENTS ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Contents(1) */
	len = snprintf( str, sizeof(str), CNMS_PDF_CONTENTS_OBJ1,
			p->obj_id + CNMS_PDF_PAGE_OBJ_CONTENTS,			/* object id ( Contents ) */
			p->obj_id + CNMS_PDF_PAGE_OBJ_CONTENTS_LEN );	/* object id ( Length of Contents ) */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	/* write Contents(2) */
	len_c = len = snprintf( str, sizeof(str), CNMS_PDF_CONTENTS_OBJ2,
			p->w_72, p->h_72,							/* CTM ( scaling ) */
			p->page );									/* ImX (X = page number) ... XObject/Image Name */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* write Contents(3) */
	len = snprintf( str, sizeof(str), CNMS_PDF_END_ST_OBJ );
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* <3> Length of Contents - stream */
	if ( ( p->offset_table[ CNMS_PDF_PAGE_OBJ_CONTENTS_LEN ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Length */
	len = snprintf( str, sizeof(str), CNMS_PDF_LENGTH_OBJ,
			p->obj_id + CNMS_PDF_PAGE_OBJ_CONTENTS_LEN,		/* object id ( Length of Contents ) */
			len_c );										/* length value */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* <4> XObject(Image) */
	if ( ( p->offset_table[ CNMS_PDF_PAGE_OBJ_IMAGE ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write XObject */
	len = snprintf( str, sizeof(str), CNMS_PDF_IMAGE_OBJ,
			p->obj_id + CNMS_PDF_PAGE_OBJ_IMAGE,		/* object id ( XObject(Image) ) */
			p->obj_id + CNMS_PDF_PAGE_OBJ_IMAGE_LEN,	/* object id ( Length of XObject ) */
			p->w, p->h,									/* Width/Height */
			ColorSpace[ type ],							/* ColorSpace */
			BitsPerComponent[ type ] );					/* BitsPerComponent */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_StartPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	ret = CNMS_NO_ERR;
EXIT:
	return ret;

}

CNMSInt32 CnmsPDF_EndPage( CNMSVoid *pw )
{
	CNMSInt32		ret = CNMS_ERR, ldata, f_size;
	CNMSPdfPage		*p = CNMSNULL;
	CNMSByte		str[1024];
	CNMSInt			len;
	int				z_ret;
	z_stream		*z;
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;

	if ( pwork == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_EndPage]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	
	p = pwork->last;
	
	if ( p->line_count != p->h ) {
		DBGMSG( "[CnmsPDF_EndPage]p->line_count != p->h\n" );
		goto	EXIT;
	}
	
	if ( p->zlib_init ) {
		z = &p->zs;
		/* flush zlib buffer */
		z_ret = Z_OK;
		while( z_ret != Z_STREAM_END ) {
			if ( ( z_ret = deflate( z, Z_FINISH ) ) != Z_OK && z_ret != Z_STREAM_END ) {
				DBGMSG( "[CnmsPDF_EndPage]Error is occured in deflate.\n" );
				goto ZLIB_ERROR;
			}
			if ( ( f_size = p->buf_size - z->avail_out ) > 0  ) {
				/* write output buffer */
				if ( ( ldata = FileControlWriteFile( pwork->fd, p->buf, f_size ) ) < 0 ) {
					DBGMSG( "[CnmsPDF_EndPage]Error is occured in FileControlWriteFile.\n" );
					goto ZLIB_ERROR;
				}
				p->stream_len += f_size;
				z->avail_out = p->buf_size;
				z->next_out = (Bytef *)p->buf;
			}
		}
		deflateEnd( z );
		p->zlib_init = CNMS_FALSE;
	}
	CnmsFreeMem( p->buf );
	p->buf = CNMSNULL;
	
	/* <1> endstream, endobj (XObject) */
	len = snprintf( str, sizeof(str), CNMS_PDF_END_ST_OBJ );
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	/* <2> Length of XObject - stream */
	if ( ( p->offset_table[ CNMS_PDF_PAGE_OBJ_IMAGE_LEN ] = GetCurrentOffset( pwork->fd ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndPage]offset > %lld\n", CNMS_PDF_XREF_MAX );
		goto EXIT;
	}
	/* write Length */
	len = snprintf( str, sizeof(str), CNMS_PDF_LENGTH_OBJ,
			p->obj_id + CNMS_PDF_PAGE_OBJ_IMAGE_LEN,		/* object id ( Length of XObject stream ) */
			p->stream_len );								/* length value */
	if ( len >= sizeof(str) || len < 0 ) {
		DBGMSG( "[CnmsPDF_EndPage]string is too long!\n" );
		goto EXIT;
	}
	if ( ( ldata = FileControlWriteFile( pwork->fd, str, len ) ) < 0 ) {
		DBGMSG( "[CnmsPDF_EndPage]Error is occured in FileControlWriteFile.\n" );
		goto EXIT;
	}
	
	ret = CNMS_NO_ERR;
	p->status = CNMS_NO_ERR;
EXIT:
	return ret;
	
ZLIB_ERROR:
	deflateEnd( z );
	goto EXIT;
}


CNMSInt32 CnmsPDF_WriteRowData( CNMSVoid *pw, CNMSLPSTR buf )
{
	CNMSInt32		ret = CNMS_ERR, ldata;
	CNMSPdfPage		*p = CNMSNULL;
	int				z_ret;
	z_stream		*z;
	CNMSPdfWork		*pwork = (CNMSPdfWork *)pw;

	if ( pwork == CNMSNULL || buf == CNMSNULL ) {
		DBGMSG( "[CnmsPDF_WriteRowData]Initialize parameter is error!\n" );
		goto	EXIT;
	}
	
	p = pwork->last;
	z = &p->zs;

	/* initialize zlib */
	if ( !p->zlib_init ) {
		CnmsSetMem( (CNMSLPSTR)z, 0, sizeof( z_stream ) );
		if ( ( z_ret = deflateInit( z, Z_BEST_SPEED ) ) != Z_OK ) {	/* maximum speed */
			DBGMSG( "[CnmsPDF_WriteRowData]Error is occured in deflateInit.\n" );
			goto	EXIT;
		}
		z->avail_out = p->buf_size;
		z->next_out = (Bytef *)p->buf;
		
		p->zlib_init = CNMS_TRUE;
	}
	
	z->avail_in = p->w_bytes;
	z->next_in = (Bytef *)buf;
	
	while ( z->avail_in > 0 ) {
		z_ret = deflate( z, Z_NO_FLUSH );
		if ( z_ret != Z_OK && z_ret != Z_STREAM_END ) {
			DBGMSG( "[CnmsPDF_WriteRowData]Error is occured in deflate.\n" );
			goto ZLIB_ERROR;
		}
		if ( z->avail_out == 0 ) {
			/* write output buffer */
			if ( ( ldata = FileControlWriteFile( pwork->fd, p->buf, p->buf_size ) ) < 0 ) {
				DBGMSG( "[CnmsPDF_WriteRowData]Error is occured in FileControlWriteFile.\n" );
				goto ZLIB_ERROR;
			}
			p->stream_len += p->buf_size;
			z->avail_out = p->buf_size;
			z->next_out = (Bytef *)p->buf;
		}
	}
	
	ret = CNMS_NO_ERR;
	p->line_count++;
EXIT:
	return ret;
	
ZLIB_ERROR:
	deflateEnd( z );
	goto EXIT;
}

#endif /* _RAW2PDF_C_ */

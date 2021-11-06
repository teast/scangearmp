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


#ifndef	_CNMSABILITY_H_
#define	_CNMSABILITY_H_

#define CANON_ABILITY_SENSOR_TYPE_CCD		(1)
#define CANON_ABILITY_SENSOR_TYPE_CIS		(2)

#define CANON_ABILITY_SCANMETHOD_FLATBED	(1)
#define CANON_ABILITY_SCANMETHOD_ADF		(2) /* ADF(one-side) */
#define CANON_ABILITY_SCANMETHOD_ADF2		(4) /* ADF(double-side) */

#define CANON_ABILITY_REF_UPPERLEFT			(0)
#define CANON_ABILITY_REF_CENTER			(1)

#define CANON_ABILITY_SUB_INFO_FLATBED		(0)
#define CANON_ABILITY_SUB_INFO_ADF			(1)
#define CANON_ABILITY_SUB_INFO_ADF2			(2) /* CANON_ABILITY_SUB_INFO_RESERVED */
#define CANON_ABILITY_SUB_INFO_MAX			(3)

#define CANON_AB_VER_MINOR_MASK (0xFF)
#define CANON_AB_VER_MEJOR_MASK (~CANON_AB_VER_MINOR_MASK)
#define CANON_AB_VER_MINOR(x) ((x) & CANON_AB_VER_MINOR_MASK)
#define CANON_AB_VER_MEJOR(x) (((x) & CANON_AB_VER_MEJOR_MASK) >> 8 )

#define CANON_ABILITY_ENABLE				(1)
#define CANON_ABILITY_DISABLE				(0)

/* device ablilty sub info struct */
typedef struct {
	int		support;			/* support = 1, not support 0 */
	int		max_width;			/* 1 pixel(opt_res) */
	int		max_length;
	int		res_index_preview;	/* index of resolution for preview. */
	int		res_index_max;		/* index of max resolution. */
	int		res_index_min;		/* index of max resolution. */
	int		ref;				/* upper left = 0, center = 1 */
	int		reserved[16];
} CANON_ABILITY_SUB_INFO;

/* device ability struct */
typedef struct {
	int		version;
	int		sensor_type;		/* CIS or CCD */
	int		scan_method;		/* support scanning method(Bit Definition: 0x01:Flatbed, 0x02:ADF(one-side), 0x04:ADF(double-side) */
	int		wait_calibration;	/* maximun wait time for Calibration */
	int		wait_warmup;		/* maximun wait time for Warmup */
	int		res_num;			/* number of resolution list element */
	int		xres[16];			/* x-resolution list */
	int		yres[16];			/* y-resolution list */
	int		opt_res;			/* optical resolution */
	int		res_max;			/* maximum resolution */
	CANON_ABILITY_SUB_INFO	info[ CANON_ABILITY_SUB_INFO_MAX ];	/* scanning info. 0 : flatbed, 1 : adf, 2 : reserved */
/* Ver.1.20 : ABILITY_VER = 1.01 */
	int		silent_mode;
	int		every_calibration;
	int		reserved[30];
} CANON_ABILITY_INFO;

#endif	/* _CNMSABILITY_H_ */

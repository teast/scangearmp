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

#ifndef	_PREFERENCE_H_
#define	_PREFERENCE_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "cnmstype.h"
#include "cnmsui.h"

#ifdef	_GLOBALS_
	#define	GLOBAL
#else
	#define	GLOBAL	extern
#endif

enum{
	PREF_TAB_SCANNER = 0,
	PREF_TAB_COLORSETTING,
	PREF_TAB_MAX,
};

typedef struct{
	CNMSInt32		color_setting;
	CNMSDec32		monitor_gamma;
	CNMSInt32		silent_mode;
	CNMSInt32		every_calibration;
}CNMSPREFCOMP, *LPCNMSPREFCOMP;

CNMSInt32 Preference_Open( CNMSVoid );
CNMSVoid  Preference_Close( CNMSVoid );

CNMSInt32 Preference_Show( LPCNMSUILINKCOMP lpLink, CNMSInt32 linkNum );
CNMSVoid  Preference_Hide( CNMSVoid );

CNMSInt32 Preference_Link( LPCNMSUILINKCOMP lpLink, CNMSInt32 linkNum );
CNMSInt32 Preference_Save( CNMSVoid );
CNMSInt32 Preference_GammaReset( CNMSVoid );

CNMSInt32 Preference_Get( LPCNMSUILINKCOMP lpLink, CNMSInt32 linkNum, LPCNMSPREFCOMP lpPref );

CNMSVoid  Preference_HideTab( CNMSInt32 tabID );

#endif	/* _PREFERENCE_H_ */

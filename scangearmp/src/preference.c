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

#ifndef	_PREFERENCE_C_
#define	_PREFERENCE_C_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"

#include "cnmstype.h"
#include "cnmsdef.h"
#include "cnmsfunc.h"
#include "prev_main.h"
#include "w1.h"
#include "preference.h"

enum{
	PREF_COLOR_24 = 0,
	PREF_COLOR_8,
	PREF_COLOR_1,
	PREF_COLOR_NUM,
};

enum{
	PREF_OBJ_COLORSET = 0,
	PREF_OBJ_MONGAMMA,
	PREF_OBJ_QUIETMODE,
	PREF_OBJ_EVERYCALIB,
	PREF_OBJ_MAX,
};

/* #define	__CNMS_DEBUG_PREFERENCE__ */

#define	PREF_WIDGET_NUM				(2)
#define	PREF_GAMMA_RESET_VALUE		(2.20)

typedef struct{
	CNMSInt32		obj;
	CNMSInt32		value;
	CNMSLPSTR		str;
}PREFWIDGETCOMP, *LPPREFWIDGETCOMP;

typedef struct{
	GtkWidget		*widget;		/* widget					*/
	CNMSInt32		valInt;			/* Value of Integer type	*/
	CNMSDec32		valDouble;		/* Value of Double type		*/
}PREFOBJCOMP, *LPPREFOBJCOMP;

typedef struct{
	GtkWidget		*preference_window;
	CNMSInt32		current_color;
	PREFOBJCOMP		obj[PREF_OBJ_MAX];
	CNMSDec32		monitor_gamma;
	CNMSInt32		widgetMode[ PREF_COLOR_NUM ][ PREF_WIDGET_NUM ];
}PREFERENCECOMP, *LPPREFERENCECOMP;

static PREFWIDGETCOMP	PrefWidgetComp[ PREF_WIDGET_NUM ] = {
	{ CNMS_OBJ_A_COLOR_CONFIG,	CNMS_A_COLOR_CONFIG_RECOM,	"preference_color_recommended_radio" },
/*	{ CNMS_OBJ_A_COLOR_CONFIG,	CNMS_A_COLOR_CONFIG_MATCH,	"preference_color_matching_radio"    }, */
	{ CNMS_OBJ_A_COLOR_CONFIG,	CNMS_A_COLOR_CONFIG_NON,	"preference_color_none_radio"        },
};

static LPPREFERENCECOMP lpPrefComp = CNMSNULL;

static CNMSInt32 ColorConv(
		CNMSInt32		colorMode )
{
	const CNMSInt32 prefColor[][ 2 ] = {
						{ CNMS_A_COLOR_MODE_COLOR,				PREF_COLOR_24 },
						{ CNMS_A_COLOR_MODE_COLOR_DOCUMENTS,	PREF_COLOR_24 },
						{ CNMS_A_COLOR_MODE_GRAY,				PREF_COLOR_8  },
						{ CNMS_A_COLOR_MODE_MONO,				PREF_COLOR_1  },
						{ CNMS_A_COLOR_MODE_TEXT_ENHANCED,		PREF_COLOR_1  } };
	CNMSInt32		ret = CNMS_ERR, i;

	for( i = 0 ; i < sizeof( prefColor ) / sizeof( prefColor[ 0 ] ) ; i ++ ){
		if( colorMode == prefColor[ i ][ 0 ] ){
			ret = prefColor[ i ][ 1 ];
			break;
		}
	}

	return	ret;
};

static CNMSVoid Preference_SetDefaultTab( CNMSVoid )
{
	GtkWidget	*notebook = CNMSNULL;

	if( lpPrefComp->preference_window == CNMSNULL ){
		DBGMSG( "[Preference_SetDefaultTab]Parameter lpPrefComp->preference_window[%p] is Invalid.\n",lpPrefComp->preference_window );
		goto	EXIT;
	}
	if( ( notebook = lookup_widget( lpPrefComp->preference_window, "notebook2" ) ) == CNMSNULL ){
		DBGMSG( "[Preference_SetDefaultTab]Parameter notebook[%p] is Invalid.\n",notebook );
		goto	EXIT;
	}
	
	gtk_notebook_set_current_page( GTK_NOTEBOOK( notebook ), 0 );

EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_HideTab()]=%d.\n", tabID );
#endif
	return;
}

CNMSInt32 Preference_Open( CNMSVoid )
{
	CNMSInt32			ret		= CNMS_ERR,
						i,
						j;
	LPPREFOBJCOMP		obj		= CNMSNULL;
	GtkAdjustment		*adjust_ment;

	if( lpPrefComp != CNMSNULL ){
		Preference_Close();
	}
	if( ( lpPrefComp = (LPPREFERENCECOMP)CnmsGetMem( sizeof( PREFERENCECOMP ) ) ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Error is occured in CnmsGetMem.\n" );
		goto	EXIT;
	}

	for( i = 0 ; i < PREF_COLOR_NUM ; i ++ ){
		for( j = 0 ; j < PREF_WIDGET_NUM ; j ++ ){
			lpPrefComp->widgetMode[ i ][ j ] = CNMS_ERR;
		}
	}
	lpPrefComp->current_color = CNMS_ERR;

	/* Preference Window */
	if( ( lpPrefComp->preference_window = create_preference_window() ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Error is occured in create_preference_window.\n" );
		goto	EXIT;
	}

	/* Color Setting */
	obj = &lpPrefComp->obj[PREF_OBJ_COLORSET];
	obj->widget		= CNMSNULL;
	obj->valInt		= CNMS_ERR;
	obj->valDouble	= CNMS_ERR;

	/* Monitor Gamma */
	obj = &lpPrefComp->obj[PREF_OBJ_MONGAMMA];	
	if( ( obj->widget = lookup_widget( lpPrefComp->preference_window, "preference_gamma_spin" ) ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Can't look up widget(preference_gamma_spin).\n" );
		goto	EXIT;
	}
	obj->valInt		= CNMS_ERR;
	obj->valDouble	= PREF_GAMMA_RESET_VALUE;
	if( ( adjust_ment = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( obj->widget ) ) ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Can't get adjustment of spin button at scale.\n" );
		goto	EXIT;
	}
	adjust_ment->lower = 0.1;
	adjust_ment->upper = 10.0;
	gtk_spin_button_set_adjustment( GTK_SPIN_BUTTON( obj->widget ), adjust_ment );

	/* Quiet Mode */
	obj = &lpPrefComp->obj[PREF_OBJ_QUIETMODE];	
	if( ( obj->widget = lookup_widget( lpPrefComp->preference_window, "preference_scanner_quietmode_check" ) ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Can't look up widget(preference_scanner_quietmode_check).\n" );
		goto	EXIT;
	}
	obj->valInt		= CNMS_FALSE;
	obj->valDouble	= CNMS_ERR;

	/* Every Calibration */
	obj = &lpPrefComp->obj[PREF_OBJ_EVERYCALIB];	
	if( ( obj->widget = lookup_widget( lpPrefComp->preference_window, "preference_scanner_calibset_every_combo" ) ) == CNMSNULL ){
		DBGMSG( "[Preference_Open]Can't look up widget(preference_scanner_calibset_every_combo).\n" );
		goto	EXIT;
	}
	obj->valInt		= 0;
	obj->valDouble	= CNMS_ERR;
	if( W1Ui_SetComboDefVal( obj->widget, CNMS_OBJ_P_EVERY_CALIBRATION ) == CNMS_ERR ){
		DBGMSG( "[Preference_Open]Func [CnmsSetComboDefVal] is Error.\n" );
		goto	EXIT;
	}

	ret = CNMS_NO_ERR;
EXIT:
	if( ret != CNMS_NO_ERR ){
		Preference_Close();
	}
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Open()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSVoid Preference_Close( CNMSVoid )
{
	if( lpPrefComp != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)lpPrefComp );
	}
	lpPrefComp = CNMSNULL;

#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Close()].\n" );
#endif
	return;
}

CNMSInt32 Preference_Link(
		LPCNMSUILINKCOMP	lpLink,
		CNMSInt32			linkNum )
{
	CNMSInt32		ret		= CNMS_ERR,
					i,
					j;
	GtkWidget		*widget;

	if( lpPrefComp == CNMSNULL ){
		DBGMSG( "[Preference_Link]Status is error.\n" );
		goto	EXIT;
	}
	else if( ( lpLink == CNMSNULL ) || ( linkNum <= 0 ) ){
		DBGMSG( "[Preference_Link]Parameter is error.\n" );
		goto	EXIT;
	}
	for( i = 0 ; i < linkNum ; i ++ ){
		if( ( lpLink[ i ].object == CNMS_OBJ_A_COLOR_MODE ) && ( lpLink[ i ].mode == CNMS_MODE_SELECT ) ){
			break;
		}
	}
	
	if( ( i == linkNum ) || ( ( lpPrefComp->current_color = ColorConv( lpLink[ i ].value ) ) == CNMS_ERR ) ){
		DBGMSG( "[Preference_Link]Color mode is error(%d).\n", lpLink[ i ].value );
		goto	EXIT;
	}
	
	if( lpPrefComp->widgetMode[ lpPrefComp->current_color ][ 0 ] != CNMS_ERR ){
		goto	EXIT_NO_ERR;
	}
	for( i = 0 ; i < PREF_WIDGET_NUM ; i ++ ){
		for( j = 0 ; j < linkNum ; j ++ ){
			if( ( lpLink[ j ].object == PrefWidgetComp[ i ].obj ) && ( lpLink[ j ].value == PrefWidgetComp[ i ].value ) ){
				lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] = lpLink[ j ].mode;
				break;
			}
		}
	}
EXIT_NO_ERR:
	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Link()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSInt32 Preference_Show(
		LPCNMSUILINKCOMP	lpLink,
		CNMSInt32			linkNum )
{
	CNMSInt32		ret		= CNMS_ERR,
					i,
					ldata;
	GtkWidget		*widget	= CNMSNULL;
	LPPREFOBJCOMP	obj		= CNMSNULL;

	if( ( ldata = Preference_Link( lpLink, linkNum ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Preference_Show]Error is occured in Preference_Link.\n" );
		goto	EXIT;
	}
	
	/* Color Setting */
	for( i = 0 ; i < PREF_WIDGET_NUM ; i ++ ){
		if( ( widget = lookup_widget( lpPrefComp->preference_window, PrefWidgetComp[ i ].str ) ) == CNMSNULL ){
			DBGMSG( "[Preference_Show]Can't look up widget(%s).\n", PrefWidgetComp[ i ].str );
			goto	EXIT;
		}
		switch( lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] ){
			case	CNMS_MODE_SELECT:
				if( GTK_WIDGET_SENSITIVE( widget ) == 0 ){
					gtk_widget_set_sensitive( widget, TRUE );
				}
				gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );
				break;
			case	CNMS_MODE_ADD:
				if( GTK_WIDGET_SENSITIVE( widget ) == 0 ){
					gtk_widget_set_sensitive( widget, TRUE );
				}
				break;
			case	CNMS_MODE_HIDE:
				if( GTK_WIDGET_SENSITIVE( widget ) != 0 ){
					gtk_widget_set_sensitive( widget, FALSE );
				}
				if( GTK_WIDGET_VISIBLE( widget ) == 0 ){
					gtk_widget_show( widget );
				}
				break;
		}
	}

	/* Monitor Gamma */
	obj = &lpPrefComp->obj[PREF_OBJ_MONGAMMA];	
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( obj->widget ), obj->valDouble );

	/* Quiet Mode */
	if( W1_JudgeFormatType( CNMS_OBJ_P_SILENT_MODE ) == CNMS_TRUE ){
		obj = &lpPrefComp->obj[PREF_OBJ_QUIETMODE];	
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( obj->widget ), obj->valInt );
	}

	/* Every Calibration */
	if( W1_JudgeFormatType( CNMS_OBJ_P_EVERY_CALIBRATION ) == CNMS_TRUE ){
		obj = &lpPrefComp->obj[PREF_OBJ_EVERYCALIB];	
		gtk_combo_box_set_active( GTK_COMBO_BOX( obj->widget ), obj->valInt );
	}

	Preference_SetDefaultTab();
	/* set focus -> OK */
	W1_WidgetGrabFocus( lpPrefComp->preference_window, "preference_ok_button" );
	
	W1_ModalDialogShowAction( lpPrefComp->preference_window );

	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Show()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSVoid  Preference_Hide( CNMSVoid )
{
	if( lpPrefComp == CNMSNULL ){
		DBGMSG( "[Preference_Hide]Status is error.\n" );
		goto	EXIT;
	}

	W1_ModalDialogHideAction( lpPrefComp->preference_window );

EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Hide()].\n" );
#endif
	return;
}

CNMSInt32 Preference_Save( CNMSVoid )
{
	CNMSInt32		ret		= CNMS_ERR,
					i;
	CNMSBool		flag;
	GtkWidget		*widget	= CNMSNULL;
	LPPREFOBJCOMP	obj		= CNMSNULL;

	if( lpPrefComp == CNMSNULL ){
		DBGMSG( "[Preference_Save]Status is error.\n" );
		goto	EXIT;
	}
	/* color setting */
	for( i = 0 ; i < PREF_WIDGET_NUM ; i ++ ){
		if( ( widget = lookup_widget( lpPrefComp->preference_window, PrefWidgetComp[ i ].str ) ) == CNMSNULL ){
			DBGMSG( "[Preference_Save]Can't look up widget(%s).\n", PrefWidgetComp[ i ].str );
			goto	EXIT;
		}
		if( ( flag = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) ) != CNMS_TRUE ){
			/* toggle is OFF or HIDE */
			if( lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] != CNMS_MODE_HIDE ){
				lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] = CNMS_MODE_ADD;
			}
		}
		else{
			/* toggle is ON */
			lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] = CNMS_MODE_SELECT;
		}
	}

	/* monitor gamma */
	obj = &lpPrefComp->obj[PREF_OBJ_MONGAMMA];	
	obj->valDouble = gtk_spin_button_get_value( GTK_SPIN_BUTTON( obj->widget ) );

	/* Quiet Mode */
	if( W1_JudgeFormatType( CNMS_OBJ_P_SILENT_MODE ) == CNMS_TRUE ){
		obj = &lpPrefComp->obj[PREF_OBJ_QUIETMODE];	
		obj->valInt = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( obj->widget ) );
	}
	
	/* Every Calibration */
	if( W1_JudgeFormatType( CNMS_OBJ_P_EVERY_CALIBRATION ) == CNMS_TRUE ){
		obj = &lpPrefComp->obj[PREF_OBJ_EVERYCALIB];	
		obj->valInt = gtk_combo_box_get_active( GTK_COMBO_BOX( obj->widget ) );
	}

	if( ( ret = Preview_ChangeStatus( CNMSSCPROC_ACTION_CROP_CORRECT ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Preference_Save]Error is occured in Preview_ChangeStatus( CNMSSCPROC_ACTION_CROP_CORRECT )!\n" );
		goto	EXIT;
	}

	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Save()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSInt32 Preference_GammaReset( CNMSVoid )
{
	CNMSInt32		ret = CNMS_ERR;
	LPPREFOBJCOMP	obj = CNMSNULL;
	
	if( lpPrefComp == CNMSNULL ){
		DBGMSG( "[Preference_GammaReset]Status is error.\n" );
		goto	EXIT;
	}
	obj = &lpPrefComp->obj[PREF_OBJ_MONGAMMA];	
	gtk_spin_button_set_value( GTK_SPIN_BUTTON( obj->widget ), PREF_GAMMA_RESET_VALUE );

	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_GammaReset()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSInt32 Preference_Get(
		LPCNMSUILINKCOMP	lpLink,
		CNMSInt32			linkNum,
		LPCNMSPREFCOMP		lpPref )
{
	CNMSInt32		ret = CNMS_ERR, i;

	if( ( lpPref == CNMSNULL ) || ( lpPrefComp == CNMSNULL ) ){
		DBGMSG( "[Preference_Get]Initialize parameter is error.\n" );
		goto	EXIT;
	}

	else if( ( i = Preference_Link( lpLink, linkNum ) ) != CNMS_NO_ERR ){
		DBGMSG( "[Preference_Get]Error is occured in Preference_Link.\n" );
		goto	EXIT;
	}

	/* color setting */
	for( i = 0 ; i < PREF_WIDGET_NUM ; i ++ ){
		if( lpPrefComp->widgetMode[ lpPrefComp->current_color ][ i ] == CNMS_MODE_SELECT ){
			lpPref->color_setting = PrefWidgetComp[ i ].value;
			break;
		}
	}

	/* monitor gamma */
	lpPref->monitor_gamma = lpPrefComp->obj[PREF_OBJ_MONGAMMA].valDouble;

	/* silent mode */
	lpPref->silent_mode = lpPrefComp->obj[PREF_OBJ_QUIETMODE].valInt;

	/* every calibration */
	lpPref->every_calibration = lpPrefComp->obj[PREF_OBJ_EVERYCALIB].valInt;

	ret = CNMS_NO_ERR;
EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_Get()]=%d.\n", ret );
#endif
	return	ret;
}

CNMSVoid Preference_HideTab(
		CNMSInt32			tabID )
{
	GtkWidget	*notebook = CNMSNULL;

	if( ( tabID < 0 ) || ( PREF_TAB_MAX <= tabID ) ){
		DBGMSG( "[Preference_HideTab]Arg parameter tabID[%d] is Invalid.\n",tabID );
		goto	EXIT;
	}
	if( lpPrefComp->preference_window == CNMSNULL ){
		DBGMSG( "[Preference_HideTab]Parameter lpPrefComp->preference_window[%p] is Invalid.\n",lpPrefComp->preference_window );
		goto	EXIT;
	}
	if( ( notebook = lookup_widget( lpPrefComp->preference_window, "notebook2" ) ) == CNMSNULL ){
		DBGMSG( "[Preference_HideTab]Parameter notebook[%p] is Invalid.\n",notebook );
		goto	EXIT;
	}
	
	gtk_notebook_remove_page( GTK_NOTEBOOK( notebook ), tabID );

EXIT:
#ifdef	__CNMS_DEBUG_PREFERENCE__
	DBGMSG( "[Preference_HideTab()]=%d.\n", tabID );
#endif
	return;
}

#endif	/* _PREFERENCE_C_ */


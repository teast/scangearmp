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

#ifndef	_SCANGIMP_H_
#define	_SCANGIMP_H_

#ifdef	__GIMP_PLUGIN_ENABLE__

#include "cnmstype.h"

#ifndef	_SCANGIMP_C_
extern gboolean	plug_in;
#endif

CNMSVoid  ScanGimpOpen( CNMSVoid );
CNMSInt32 ScanGimpMain( CNMSInt32 argc, CNMSLPSTR argv[] );
CNMSInt32 ScanGimpInit( CNMSVoid );
CNMSVoid  ScanGimpClose( CNMSVoid );
CNMSBool  ScanGimpGetStatus( CNMSVoid );
CNMSInt32 ScanGimpExec( CNMSVoid );
CNMSVoid  ScanGimpCancel( CNMSVoid );

#endif	/* __GIMP_PLUGIN_ENABLE__ */

#endif	/* _SCANGIMP_H_ */

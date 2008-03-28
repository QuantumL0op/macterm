/*!	\file VectorToBitmap.h
	\brief Routines for creating Macintosh pictures (QuickDraw
	PICT format) out of TEK vector graphics.
	
	Only one real device is available.  This means that the
	correct graphics port must be set when making all
	manipulations.
*/
/*###############################################################

	MacTelnet
		� 1998-2008 by Kevin Grant.
		� 2001-2003 by Ian Anderson.
		� 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __VECTORTOBITMAP__
#define __VECTORTOBITMAP__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

//!\name Initialization
//@{

void
	VectorToBitmap_Init					();

//@}

//!\name Creating and Destroying Vector Graphics Contexts
//@{

SInt16
	VectorToBitmap_New					();

SInt16
	VectorToBitmap_Dispose				(SInt16			inWhichDevice);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	VectorToBitmap_DataLine				(SInt16			inDevice,
										 SInt16			inData,
										 SInt16			inCount);

SInt16
	VectorToBitmap_DrawDot				(SInt16			inDevice,
										 SInt16			inX,
										 SInt16			inY);

SInt16
	VectorToBitmap_DrawLine				(SInt16			inDevice,
										 SInt16			inStartX,
										 SInt16			inStartY,
										 SInt16			inEndX,
										 SInt16			inEndY);

char const*
	VectorToBitmap_ReturnDeviceName		();

SInt16
	VectorToBitmap_SetBounds			(Rect const*	inBoundsPtr);

void
	VectorToBitmap_SetCallbackData		(SInt16			inDevice,
										 SInt16			inTektronixVirtualGraphicsRef,
										 SInt16			inData2,
										 SInt16			inData3,
										 SInt16			inData4,
										 SInt16			inData5);

void
	VectorToBitmap_SetCharacterMode		(SInt16			inDevice,
										 SInt16			inRotation,
										 SInt16			inSize);

SInt16
	VectorToBitmap_SetPenColor			(SInt16			inDevice,
										 SInt16			inColorIndex);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE

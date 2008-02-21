/*!	\file FileSelectionDialogs.h
	\brief Convenient front ends to Navigation Services.
*/
/*###############################################################

	Interface Library 1.3
	� 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __FILESELECTIONDIALOGS__
#define __FILESELECTIONDIALOGS__



// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

//!\name Displaying Navigation Services Dialog Boxes
//@{

// DEPRECATED IN FAVOR OF FileSelectionDialogs_GetFiles()
OSStatus
	FileSelectionDialogs_GetFile		(ConstStr255Param		inPromptMessage,
										 ConstStr255Param		inDialogTitle,
										 OSType					inApplicationSignature,
										 UInt32					inPrefKey,
										 NavDialogOptionFlags	inFlags,
										 UInt32					inNumTypes,
										 OSType					inTypeList[],
										 NavEventProcPtr		inEventProc,
										 FSSpec*				inoutFileSpecPtr, // non-nullptr for single-select
										 OSType*				inoutFileType, // non-nullptr for single-select
										 FileInfo*				outFileInfo); // non-nullptr for single-select

// PRESENTS AN APPLICATION-MODAL DIALOG
OSStatus
	FileSelectionDialogs_GetFiles		(CFStringRef			inPromptMessage,
										 CFStringRef			inDialogTitle,
										 OSType					inApplicationSignature,
										 UInt32					inPrefKey,
										 NavDialogOptionFlags	inFlags,
										 UInt32					inNumTypes,
										 OSType					inTypeList[],
										 NavEventProcPtr		inEventProc);

// PRESENTS AN APPLICATION-MODAL DIALOG
OSStatus
	FileSelectionDialogs_GetDirectory	(ConstStringPtr			inPromptMessage,
										 ConstStringPtr			inDialogTitle,
										 UInt32					inPrefKey,
										 NavDialogOptionFlags	inFlags,
										 NavEventProcPtr		inEventProc,
										 FSSpec*				outFileSpecPtr);

// PRESENTS AN APPLICATION-MODAL DIALOG
OSStatus
	FileSelectionDialogs_GetFile		(ConstStringPtr			inPromptMessage,
										 ConstStringPtr			inDialogTitle,
										 OSType					inApplicationSignature,
										 UInt32					inPrefKey,
										 NavDialogOptionFlags	inFlags,
										 UInt32					inNumTypes,
										 OSType					inTypeList[],
										 NavEventProcPtr		inEventProc,
										 FSSpec*				inoutFileSpecPtr, // non-nullptr for single-select
										 OSType*				inoutFileType, // non-nullptr for single-select
										 FileInfo*				outFileInfo); // non-nullptr for single-select

// PRESENTS AN APPLICATION-MODAL DIALOG
OSStatus
	FileSelectionDialogs_PutFile		(ConstStringPtr			inPromptMessage,
										 ConstStringPtr			inDialogTitle,
										 ConstStringPtr			inDefaultFileNameOrNull,
										 OSType					inApplicationSignature,
										 OSType					inFileSignature,
										 UInt32					inPrefKey,
										 NavDialogOptionFlags	inFlags,
										 NavEventProcPtr		inEventProc,
										 NavReplyRecord*		outReplyPtr,
										 FSSpec*				outFileSpecPtr);

//@}

//!\name Utility Routines For Use With Navigation Services
//@{

OSStatus
	FileSelectionDialogs_CompleteSave				(NavReplyRecord*		inoutReplyPtr);

OSStatus
	FileSelectionDialogs_CreateOrFindUserSaveFile	(NavReplyRecord const&	inReply,
													 OSType					inNewFileCreator,
													 OSType					inNewFileType,
													 FSRef&					outUserSaveFile,
													 FSRef&					outTemporaryFile);

OSStatus
	FileSelectionDialogs_GetFSSpecArrayFromAEDesc	(AEDesc const*			inAEDescPtr,
													 void*					outFSSpecArray,
													 long*					inoutFSSpecArrayLengthPtr);

OSStatus
	FileSelectionDialogs_GetFSSpecFromAEDesc		(AEDesc const*			inAEDescPtr,
													 FSSpec*				outFSSpecPtr);

pascal Boolean
	FileSelectionDialogs_NothingFilterProc			(AEDesc*				inItem,
													 void*					inNavFileOrFolderInfoPtr,
													 NavCallBackUserData	inContext,
													 NavFilterModes			inFilter);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
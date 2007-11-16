/*###############################################################

	PrefsWindow.cp
	
	MacTelnet
		� 1998-2007 by Kevin Grant.
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

// standard-C++ includes
#include <algorithm>
#include <functional>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "StringResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelFormats.h"
#include "PrefPanelGeneral.h"
#include "PrefPanelKiosk.h"
#include "PrefPanelMacros.h"
#include "PrefPanelScripts.h"
#include "PrefPanelSessions.h"
#include "PrefPanelTerminals.h"
#include "PrefPanelTranslations.h"
#include "PrefsWindow.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

namespace
{
	/*!
	IMPORTANT

	The following values MUST agree with the control IDs in
	the "Window" NIB from the package "PrefsWindow.nib".
	*/
	HIViewID const		idMyUserPaneAnyPrefPanel		= { FOUR_CHAR_CODE('Panl'),			0/* ID */ };
	
	/*!
	IMPORTANT

	The following values MUST agree with the control IDs in
	the "Drawer" NIB from the package "PrefsWindow.nib".
	*/
	HIViewID const		idMyStaticTextListTitle			= { FOUR_CHAR_CODE('Titl'),			0/* ID */ };
	HIViewID const		idMyDataBrowserCollections		= { FOUR_CHAR_CODE('Favs'),			0/* ID */ };
	HIViewID const		idMyButtonAddCollection			= { FOUR_CHAR_CODE('AddC'),			0/* ID */ };
	HIViewID const		idMyButtonRemoveCollection		= { FOUR_CHAR_CODE('DelC'),			0/* ID */ };
	FourCharCode const	kMyDataBrowserPropertyIDSets	= FOUR_CHAR_CODE('Sets');
}

#pragma mark Types

struct MyPanelData
{
	inline MyPanelData		(Panel_Ref, SInt16);
	
	Panel_Ref	panel;					//!< the panel that will be displayed when selected by the user
	SInt16		listIndex;				//!< row index of panel in the panel choice list
};
typedef MyPanelData*		MyPanelDataPtr;

typedef std::vector< MyPanelDataPtr >		MyPanelDataList;
typedef std::vector< HIToolbarItemRef >		CategoryToolbarItems;
typedef std::map< UInt32, SInt16 >			IndexByCommandID;

#pragma mark Internal Method Prototypes

static pascal OSStatus		accessDataBrowserItemData		(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
																DataBrowserItemDataRef, Boolean);
static void					chooseContext					(Preferences_ContextRef);
static void					choosePanel						(UInt16);
static void					findBestPanelSize				(HISize const&, HISize&);
static void					handleNewDrawerWindowSize		(WindowRef, Float32, Float32, void*);
static void					handleNewMainWindowSize			(WindowRef, Float32, Float32, void*);
static void					init							();
static void					installPanel					(Panel_Ref);
static pascal void			monitorDataBrowserItems			(ControlRef, DataBrowserItemID, DataBrowserItemNotification);
static void					newPanelSelector				(Panel_Ref);
static void					preferenceChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void					rebuildList						();
static pascal OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowClosing			(EventHandlerCallRef, EventRef, void*);
static void					refreshDisplay					();
static Preferences_Class	returnCurrentPreferencesClass	();
static void					sizePanels						(HISize const&);

// declare the LDEF entry point (it�s only referred to here, and is implemented in IconListDef.c)
pascal void IconListDef(SInt16, Boolean, Rect*, Cell, SInt16, SInt16, ListHandle);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Panel_Ref								gCurrentPanel = nullptr;
	WindowInfoRef							gPreferencesWindowInfo = nullptr;
	WindowRef								gPreferencesWindow = nullptr; // the Mac OS window pointer
	WindowRef								gDrawerWindow = nullptr; // the Mac OS window pointer
	CommonEventHandlers_WindowResizer		gPreferencesWindowResizeHandler;
	CommonEventHandlers_WindowResizer		gDrawerWindowResizeHandler;
	CarbonEventHandlerWrap					gPrefsCommandHandler(GetApplicationEventTarget(),
																	receiveHICommand,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassCommand),
																			kEventCommandProcess),
																	nullptr/* user data */);
	Console_Assertion						_1(gPrefsCommandHandler.isInstalled(), __FILE__, __LINE__);
	EventHandlerUPP							gPreferencesWindowClosingUPP = nullptr;
	EventHandlerRef							gPreferencesWindowClosingHandler = nullptr;
	ListenerModel_ListenerRef				gPreferenceChangeEventListener = nullptr;
	HISize									gMaximumWindowSize = CGSizeMake(600, 400); // arbitrary; overridden later
	HIViewRef								gDataBrowserTitle = nullptr;
	HIViewRef								gDataBrowserForCollections = nullptr;
	HIViewRef								gCollectionAddButton = nullptr;
	HIViewRef								gCollectionRemoveButton = nullptr;
	SInt16									gPanelChoiceListLastRowIndex = -1;
	MyPanelDataList&						gPanelList ()	{ static MyPanelDataList x; return x; }
	CategoryToolbarItems&					gCategoryToolbarItems ()	{ static CategoryToolbarItems x; return x; }
	IndexByCommandID&						gIndicesByCommandID ()		{ static IndexByCommandID x; return x; }
	Preferences_ContextRef					gCurrentDataSet = nullptr;
}


#pragma mark Functors

/*!
This is a functor that determines if a given panel
data structure is intended for a particular panel.

Model of STL Predicate.

(3.0)
*/
class isPanelDataForSpecificPanel:
public std::unary_function< MyPanelDataPtr/* argument */, bool/* return */ >
{
public:
	isPanelDataForSpecificPanel	(Panel_Ref	inPanel)
	: _panel(inPanel)
	{
	}
	
	bool
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		return (inPanelDataPtr->panel == _panel);
	}

protected:

private:
	Panel_Ref	_panel;
};

/*!
Finds a panel�s ideal size.  If invoked on several
panels successively, the cumulative maximum is stored.

Model of STL Unary Function.

(3.1)
*/
class panelFindIdealSize:
public std::unary_function< MyPanelDataPtr/* argument */, void/* return */ >
{
public:
	panelFindIdealSize ()
	: idealSize(CGSizeMake(0, 0))
	{
	}
	
	void
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		HISize		panelIdealSize = CGSizeMake(0, 0);
		
		
		if (kPanel_ResponseSizeProvided == Panel_SendMessageGetIdealSize(inPanelDataPtr->panel, panelIdealSize))
		{
			this->idealSize.width = std::max(panelIdealSize.width, idealSize.width);
			this->idealSize.height = std::max(panelIdealSize.height, idealSize.height);
		}
	}
	
	HISize		idealSize;

protected:

private:
};

/*!
Resizes a panel, given its internal data structure.

Model of STL Unary Function.

(3.1)
*/
class panelResizer:
public std::unary_function< MyPanelDataPtr/* argument */, void/* return */ >
{
public:
	panelResizer	(Float32	inHorizontal,
					 Float32	inVertical,
					 Boolean	inIsDelta)
	: _horizontal(inHorizontal), _vertical(inVertical), _delta(inIsDelta)
	{
	}
	
	void
	operator ()	(MyPanelDataPtr		inPanelDataPtr)
	{
		Panel_Resizer	resizer(_horizontal, _vertical, _delta);
		
		
		resizer(inPanelDataPtr->panel);
	}

protected:

private:
	Float32		_horizontal;
	Float32		_vertical;
	Boolean		_delta;
};


#pragma mark Public Methods

/*!
Call this method in the exit routine of the program
to ensure that this window�s memory allocations
get destroyed.  You can also call this method after
you are done using this window, but then a call to
PrefsWindow_Display() will re-initialize the window
before redisplaying it.

(3.0)
*/
void
PrefsWindow_Done ()
{
	PrefsWindow_Remove(); // saves preferences
	
	if (nullptr != gPreferencesWindow)
	{
		MyPanelDataList::iterator		panelDataIterator;
		CategoryToolbarItems::iterator	toolbarItemIterator;
		
		
		// clean up the Help System
		HelpSystem_SetWindowKeyPhrase(gPreferencesWindow, kHelpSystem_KeyPhraseDefault);
		
		// disable preferences callbacks
		Preferences_StopListeningForChanges(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts);
		Preferences_StopListeningForChanges(gPreferenceChangeEventListener, kPreferences_ChangeContextName);
		ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
		
		// disable event callbacks and destroy the window
		RemoveEventHandler(gPreferencesWindowClosingHandler), gPreferencesWindowClosingHandler = nullptr;
		DisposeEventHandlerUPP(gPreferencesWindowClosingUPP), gPreferencesWindowClosingUPP = nullptr;
		for (panelDataIterator = gPanelList().begin(); panelDataIterator != gPanelList().end(); ++panelDataIterator)
		{
			// dispose each panel
			MyPanelDataPtr		dataPtr = *panelDataIterator;
			if (nullptr != dataPtr)
			{
				Panel_Dispose(&dataPtr->panel);
				delete dataPtr, dataPtr = nullptr;
			}
		}
		for (toolbarItemIterator = gCategoryToolbarItems().begin();
				toolbarItemIterator != gCategoryToolbarItems().end(); ++toolbarItemIterator)
		{
			// release each item
			if (nullptr != *toolbarItemIterator) CFRelease(*toolbarItemIterator);
		}
		(OSStatus)SetDrawerParent(gDrawerWindow, nullptr/* parent */);
		DisposeWindow(gDrawerWindow), gDrawerWindow = nullptr;
		DisposeWindow(gPreferencesWindow), gPreferencesWindow = nullptr; // automatically destroys all controls
		WindowInfo_Dispose(gPreferencesWindowInfo);
	}
}// Done


/*!
Use this method to display the preferences window.
If the window is not yet in memory, it is created.

(3.0)
*/
void
PrefsWindow_Display ()
{
	if (nullptr == gPreferencesWindow) init();
	if (nullptr == gPreferencesWindow) Alert_ReportOSStatus(memPCErr);
	else
	{
		// define this only for debugging; this lets you dump the window�s
		// control embedding hierarchy to a file, before it is displayed
	#if 0
		DebugSelectControlHierarchyDumpFile(gPreferencesWindow);
	#endif
		
		// display the window and handle events
		unless (IsWindowVisible(gPreferencesWindow)) ShowWindow(gPreferencesWindow);
		EventLoop_SelectBehindDialogWindows(gPreferencesWindow);
		Cursors_UseArrow();
		
		// if no panel has ever been highlighted, choose the first one
		assert(false == gPanelList().empty());
		if (nullptr == gCurrentPanel)
		{
			choosePanel(0);
		}
	}
}// Display


/*!
Use this method to hide the preferences window, which
will cause all changes to be saved.

The window remains in memory and can be re-displayed
using PrefsWindow_Display().  To destroy the window,
use the method PrefsWindow_Done().

(3.0)
*/
void
PrefsWindow_Remove ()
{
	if (nullptr != gPreferencesWindow)
	{
		HIViewRef	focusControl = nullptr;
		
		
		if (GetKeyboardFocus(gPreferencesWindow, &focusControl) == noErr)
		{
			// if the user is editing a text field, this makes sure the changes �stick�
			if (nullptr != focusControl) Panel_SendMessageFocusLost(gCurrentPanel, focusControl);
		}
	}
	
	// write all the preference data in memory to disk
	(Preferences_ResultCode)Preferences_Save();
	
	// save window size and location in preferences
	if (nullptr != gPreferencesWindow)
	{
		Preferences_SetWindowArrangementData(gPreferencesWindow, kPreferences_WindowTagPreferences);
		HideWindow(gPreferencesWindow);
	}
}// Remove


#pragma mark Internal Methods

/*!
Initializes a MyPanelData structure.

(3.1)
*/
MyPanelData::
MyPanelData		(Panel_Ref	inPanel,
				 SInt16		inListIndex)
:
panel(inPanel),
listIndex(inListIndex)
{
}// MyPanelData 2-argument constructor


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
static pascal OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus				result = noErr;
	Preferences_ContextRef	context = REINTERPRET_CAST(inItemID, Preferences_ContextRef);
	
	
	assert(gDataBrowserForCollections == inDataBrowser);
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			// TEMPORARY - the Default context should not be editable, all others are
			result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
			break;
		
		case kMyDataBrowserPropertyIDSets:
			// return the name of the collection
			{
				Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
				CFStringRef					contextName = nullptr;
				
				
				prefsResult = Preferences_ContextGetName(context, contextName);
				if (kPreferences_ResultCodeSuccess != prefsResult)
				{
					result = paramErr;
				}
				else
				{
					result = SetDataBrowserItemDataText(inItemData, contextName);
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		switch (inPropertyID)
		{
		case kMyDataBrowserPropertyIDSets:
			// user has changed the collection name; update preferences
			{
				CFStringRef		newName = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newName);
				if (noErr == result)
				{
					Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
					
					
					prefsResult = Preferences_ContextRename(context, newName);
					if (kPreferences_ResultCodeSuccess != prefsResult)
					{
						result = paramErr;
					}
					else
					{
						result = noErr;
					}
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// accessDataBrowserItemData


/*!
Sends a panel event indicating that the current
global context (if any) is being swapped out for
the specified context.  Then, sets the global
context to the given context.

Pass nullptr to cause a full reset, a �select
nothing� event.

(3.1)
*/
static void
chooseContext	(Preferences_ContextRef		inContext)
{
	Panel_DataSetTransition		setChange;
	
	
	bzero(&setChange, sizeof(setChange));
	setChange.oldDataSet = gCurrentDataSet;
	setChange.newDataSet = inContext;
	Panel_SendMessageNewDataSet(gCurrentPanel, setChange);
	gCurrentDataSet = inContext;
}// chooseContext


/*!
Displays the panel with the specified index in the
main list.  0 indicates the first panel, etc.

WARNING:	No boundary checking is done.  Pass in
			a valid index, which should be between
			0 and the size of the global panel list.

(3.1)
*/
static void
choosePanel		(UInt16		inZeroBasedPanelNumber)
{
	MyPanelDataPtr		newPanelDataPtr = nullptr;
	
	
	// unhighlight all, then highlight the new one; only possible on Tiger or later
	if (FlagManager_Test(kFlagOS10_4API))
	{
		CategoryToolbarItems::const_iterator	toItem = gCategoryToolbarItems().begin();
		CategoryToolbarItems::const_iterator	itemEnd = gCategoryToolbarItems().end();
		
		
		assert(false == gCategoryToolbarItems().empty());
		for (; toItem != itemEnd; ++toItem)
		{
			(OSStatus)HIToolbarItemChangeAttributes(*toItem, 0/* attributes to set */,
													FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected)/* attributes to clear */);
		}
		(OSStatus)HIToolbarItemChangeAttributes(gCategoryToolbarItems()[inZeroBasedPanelNumber],
												FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected)/* attributes to set */,
												0/* attributes to clear */);
	}
	
	// get the selected item�s data (which ought to always be defined)
	newPanelDataPtr = gPanelList()[inZeroBasedPanelNumber];
	if ((nullptr != newPanelDataPtr) && (newPanelDataPtr->panel != gCurrentPanel))
	{
		HIViewRef	nowVisibleContainer = nullptr;
		HIViewRef	nowInvisibleContainer = nullptr;
		
		
		// note which panel was displayed before (if any)
		{
			// this panel�s button is currently selected, so this is the panel that will disappear
			if (nullptr != gCurrentPanel)
			{
				Panel_GetContainerView(gCurrentPanel, nowInvisibleContainer);
				
				// remember the size of this panel, so it can be restored later
				{
					HIRect		preferredBounds;
					
					
					if (noErr == HIViewGetBounds(nowInvisibleContainer, &preferredBounds))
					{
						Panel_SetPreferredSize(gCurrentPanel, preferredBounds.size);
					}
				}
				
				// notify the panel
				Panel_SendMessageNewVisibility(newPanelDataPtr->panel, false/* visible */);
			}
			
			// set the title of the collections drawer�s list to match the panel name
			{
				CFStringRef		newListTitleCFString = nullptr;
				
				
				Panel_GetName(newPanelDataPtr->panel, newListTitleCFString);
				SetControlTextWithCFString(gDataBrowserTitle, newListTitleCFString);
			}
			
			// change the current panel
			gCurrentPanel = newPanelDataPtr->panel;
			
			// tell the new panel that it is becoming visible; also adapt the window size,
			// and perform any related actions (like showing or hiding a drawer)
			if (nullptr != gCurrentPanel)
			{
				HISize	idealSize;
				
				
				// determine the minimum allowed size for the panel
				if (kPanel_ResponseSizeProvided == Panel_SendMessageGetIdealSize(newPanelDataPtr->panel, idealSize))
				{
					// the new absolute minimum window size is the size required to fit all panel views
					gPreferencesWindowResizeHandler.setWindowMinimumSize(idealSize.width, idealSize.height);
				}
				else
				{
					idealSize.width = 0;
					idealSize.height = 0;
				}
				
				// perhaps the user prefers the panel to be larger...
				{
					HISize	preferredSize;
					
					
					Panel_GetPreferredSize(gCurrentPanel, preferredSize);
					
					// determine which is bigger...the width the user has chosen, or the ideal width;
					// ditto for height; this becomes the most ideal size (that is, stay as close to
					// the user�s preferred size as possible, while still leaving room for all subviews)
					idealSize.width = std::max(idealSize.width, preferredSize.width);
					idealSize.height = std::max(idealSize.height, preferredSize.height);
				}
				
				Panel_GetContainerView(gCurrentPanel, nowVisibleContainer);
				Panel_SendMessageNewVisibility(newPanelDataPtr->panel, true/* visible */);
				
				// animate a resize of the window if the new panel size is different
				{
					Rect	windowContentBounds;
					
					
					assert_noerr(GetWindowBounds(gPreferencesWindow, kWindowContentRgn, &windowContentBounds));
					
					// if the ideal size is empty, force it to match the window size
					if ((idealSize.width < 100/* arbitrary */) || (idealSize.height < 100/* arbitrary */))
					{
						idealSize.width = windowContentBounds.right - windowContentBounds.left;
						idealSize.height = windowContentBounds.bottom - windowContentBounds.top;
					}
					
					// size container according to limits; due to event handlers,
					// a resize of the window changes the panel size too
					{
						Rect		windowStructureBounds;
						Float32		structureAdditionX = 0;
						Float32		structureAdditionY = 0;
						HIRect		newBounds;
						OSStatus	error = noErr;
						
						
						// determine window structure location
						assert_noerr(GetWindowBounds(gPreferencesWindow, kWindowStructureRgn, &windowStructureBounds));
						
						// determine the extra width and height contributed by the frame, versus the content area
						structureAdditionX = (windowStructureBounds.right - windowStructureBounds.left) -
												(windowContentBounds.right - windowContentBounds.left);
						structureAdditionY = (windowStructureBounds.bottom - windowStructureBounds.top) -
												(windowContentBounds.bottom - windowContentBounds.top);
						
						// define the new structure boundaries in terms of the desired content width
						// and the extra width/height contributed by the non-content regions
						newBounds = CGRectMake(windowStructureBounds.left, windowStructureBounds.top,
												idealSize.width + structureAdditionX,
												idealSize.height + structureAdditionY);
						
						// finally, reshape the window!
						error = TransitionWindowWithOptions(gPreferencesWindow, kWindowSlideTransitionEffect,
															kWindowResizeTransitionAction, &newBounds,
															false/* asynchronously */, nullptr/* options */);
						if (noErr != error)
						{
							// if the transition fails, just resize the window normally
							SizeWindow(gPreferencesWindow, STATIC_CAST(idealSize.width, SInt16),
										STATIC_CAST(idealSize.height, SInt16), true/* update */);
						}
					}
				}
				
				// modify the data displayed in the list drawer
				rebuildList();
				
				// if the panel is an inspector type, show the collections drawer
				if (kPanel_ResponseEditTypeInspector == Panel_SendMessageGetEditType(newPanelDataPtr->panel))
				{
					(OSStatus)OpenDrawer(gDrawerWindow, kWindowEdgeDefault, true/* asynchronously */);
				}
				else
				{
					(OSStatus)CloseDrawer(gDrawerWindow, true/* asynchronously */);
				}
			}
		}
		
		// swap panels
		Embedding_OffscreenSwapOverlappingControls(gPreferencesWindow, nowInvisibleContainer, nowVisibleContainer);
		(OSStatus)HIViewSetVisible(nowVisibleContainer, true/* visible */);
	}
}// choosePanel


/*!
Determines the ideal sizes of all installed panels, and
finds the best compromise between them all.

This routine presumes a panel cannot become any smaller
than its ideal size or the given initial size.

IMPORTANT:	Invoke this only after all desired panels
			have been installed.

(3.1)
*/
static void
findBestPanelSize	(HISize const&		inInitialSize,
					 HISize&			outIdealSize)
{
	panelFindIdealSize	idealSizeFinder;
	
	
	// find the best size, accounting for the ideal sizes of all panels
	idealSizeFinder = std::for_each(gPanelList().begin(), gPanelList().end(), idealSizeFinder);
	outIdealSize.width = std::max(inInitialSize.width, idealSizeFinder.idealSize.width);
	outIdealSize.height = std::max(inInitialSize.height, idealSizeFinder.idealSize.height);
}// findBestPanelSize


/*!
This routine is called whenever the drawer is
resized, to make sure the drawer does not become
too large.

(3.1)
*/
static void
handleNewDrawerWindowSize	(WindowRef		inWindowRef,
							 Float32		UNUSED_ARGUMENT(inDeltaX),
							 Float32		UNUSED_ARGUMENT(inDeltaY),
							 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing really needs to be done here; the routine exists only
	// to ensure the initial constraints (at routine install time)
	// are always enforced, otherwise the drawer size cannot shrink
	assert(inWindowRef == gDrawerWindow);
}// handleNewDrawerWindowSize


/*!
This routine is called whenever the window is
resized, to make sure panels match its new size.

(3.0)
*/
static void
handleNewMainWindowSize	(WindowRef		inWindow,
						 Float32		UNUSED_ARGUMENT(inDeltaX),
						 Float32		UNUSED_ARGUMENT(inDeltaY),
						 void*			UNUSED_ARGUMENT(inContext))
{
	Rect	windowContentBounds;
	
	
	assert(inWindow == gPreferencesWindow);
	assert_noerr(GetWindowBounds(inWindow, kWindowContentRgn, &windowContentBounds));
	
	// resize panels
	panelResizer	resizer(windowContentBounds.right - windowContentBounds.left,
							windowContentBounds.bottom - windowContentBounds.top,
							false/* is delta */);
	std::for_each(gPanelList().begin(), gPanelList().end(), resizer);
	
	Cursors_UseArrow();
}// handleNewMainWindowSize


/*!
Initializes the preferences window.  The window is
created invisibly, and then global preference data is
used to set up the corresponding controls in the
window panes.

You should generally call PrefsWindow_Display(), which
will automatically invoke init() if necessary.  This
allows you to create the window only the first time it
is necessary to display it, and from then on to retain
it in memory.

Be sure to call PrefsWindow_Done() at the end of the
program to dispose of memory that this module
allocates.

(3.0)
*/
static void
init ()
{
	NIBWindow	mainWindow(AppResources_ReturnBundleForNIBs(), CFSTR("PrefsWindow"), CFSTR("Window"));
	NIBWindow	drawerWindow(AppResources_ReturnBundleForNIBs(), CFSTR("PrefsWindow"), CFSTR("Drawer"));
	
	
	mainWindow << NIBLoader_AssertWindowExists;
	drawerWindow << NIBLoader_AssertWindowExists;
	gPreferencesWindow = mainWindow;
	gDrawerWindow = drawerWindow;
	(OSStatus)SetDrawerParent(drawerWindow, mainWindow);
	
	if (nullptr != gPreferencesWindow)
	{
		HIViewRef	panelUserPane = nullptr;
		Rect		panelBounds;
		
		
		// set up the Help System
		HelpSystem_SetWindowKeyPhrase(drawerWindow, kHelpSystem_KeyPhrasePreferences);
		
		// enable window drag tracking, so the data browser column moves and toolbar item drags work
		(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gPreferencesWindow, true);
		
		// set up the WindowInfo stuff
		gPreferencesWindowInfo = WindowInfo_New();
		WindowInfo_SetWindowDescriptor(gPreferencesWindowInfo, kConstantsRegistry_WindowDescriptorPreferences);
		WindowInfo_SetForWindow(gPreferencesWindow, gPreferencesWindowInfo);
		
		// set the item text for the Dock�s window menu
		{
			CFStringRef		titleCFString = nullptr;
			
			
			(UIStrings_ResultCode)UIStrings_Copy(kUIStrings_PreferencesWindowIconName, titleCFString);
			SetWindowAlternateTitle(gPreferencesWindow, titleCFString);
			CFRelease(titleCFString), titleCFString = nullptr;
		}
		
		// find references to all NIB-based controls that are needed for any operation
		// (button clicks, dealing with text or responding to window resizing)
		panelUserPane = (mainWindow.returnHIViewWithID(idMyUserPaneAnyPrefPanel) << HIViewWrap_AssertExists);
		gDataBrowserTitle = (drawerWindow.returnHIViewWithID(idMyStaticTextListTitle) << HIViewWrap_AssertExists);
		gDataBrowserForCollections = (drawerWindow.returnHIViewWithID(idMyDataBrowserCollections) << HIViewWrap_AssertExists);
		gCollectionAddButton = (drawerWindow.returnHIViewWithID(idMyButtonAddCollection) << HIViewWrap_AssertExists);
		gCollectionRemoveButton = (drawerWindow.returnHIViewWithID(idMyButtonRemoveCollection) << HIViewWrap_AssertExists);
		
		// set up the data browser
		{
			DataBrowserCallbacks	callbacks;
			OSStatus				error = noErr;
			
			
			// define a callback for specifying what data belongs in the list
			callbacks.version = kDataBrowserLatestCallbacks;
			error = InitDataBrowserCallbacks(&callbacks);
			assert_noerr(error);
			callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
			assert(nullptr != callbacks.u.v1.itemDataCallback);
			callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
			assert(nullptr != callbacks.u.v1.itemNotificationCallback);
			
			// attach data not specified in NIB
			error = SetDataBrowserCallbacks(gDataBrowserForCollections, &callbacks);
			assert_noerr(error);
			
			// insert data - TEMPORARY
			{
				Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
				Preferences_ContextRef		defaultContext = nullptr;
				
				
				prefsResult = Preferences_GetDefaultContext(&defaultContext, returnCurrentPreferencesClass());
				if (kPreferences_ResultCodeSuccess == prefsResult)
				{
					DataBrowserItemID	ids[] = { REINTERPRET_CAST(defaultContext, DataBrowserItemID) };
					
					
					(OSStatus)AddDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
													sizeof(ids) / sizeof(DataBrowserItemID), ids,
													kMyDataBrowserPropertyIDSets/* pre-sort property */);
				}
			}
		}
		
		// add "+" and "-" icons to the add and remove buttons
		{
			IconManagerIconRef		buttonIcon = nullptr;
			
			
			buttonIcon = IconManager_NewIcon();
			if (nullptr != buttonIcon)
			{
				if (noErr == IconManager_MakeIconRefFromBundleFile
								(buttonIcon, AppResources_ReturnItemAddIconFilenameNoExtension(),
									kConstantsRegistry_ApplicationCreatorSignature,
									kConstantsRegistry_IconServicesIconItemAdd))
				{
					if (noErr == IconManager_SetButtonIcon(gCollectionAddButton, buttonIcon))
					{
						// once the icon is set successfully, the equivalent text title can be removed
						(OSStatus)SetControlTitleWithCFString(gCollectionAddButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
			
			buttonIcon = IconManager_NewIcon();
			if (nullptr != buttonIcon)
			{
				if (noErr == IconManager_MakeIconRefFromBundleFile
								(buttonIcon, AppResources_ReturnItemRemoveIconFilenameNoExtension(),
									kConstantsRegistry_ApplicationCreatorSignature,
									kConstantsRegistry_IconServicesIconItemRemove))
				{
					if (noErr == IconManager_SetButtonIcon(gCollectionRemoveButton, buttonIcon))
					{
						// once the icon is set successfully, the equivalent text title can be removed
						(OSStatus)SetControlTitleWithCFString(gCollectionRemoveButton, CFSTR(""));
					}
				}
				IconManager_DisposeIcon(&buttonIcon);
			}
		}
		
		//
		// create base controls
		//
		
		// create preferences icons
		{
			HIToolbarRef	categoryIcons = nullptr;
			
			
			if (noErr == HIToolbarCreate(kConstantsRegistry_HIToolbarIDPreferences, kHIToolbarNoAttributes,
											&categoryIcons))
			{
				// add standard items to display the collections drawer
			#if 0
				HIToolbarItemRef	drawerDisplayItem = nullptr;
				HIToolbarItemRef	drawerSeparatorItem = nullptr;
				
				
				if (noErr == HIToolbarItemCreate
								(kMyHIToolbarItemIDCollections,
									kHIToolbarItemCantBeRemoved | kHIToolbarItemAnchoredLeft/* item options */,
									&drawerDisplayItem))
				{
					CFStringRef		labelCFString = nullptr;
					CFStringRef		descriptionCFString = nullptr;
					FSRef			iconFile;
					
					
					// set the icon, label and tooltip
					(OSStatus)HIToolbarItemSetCommandID(drawerDisplayItem, kCommandShowHidePrefCollectionsDrawer);
					if (AppResources_GetArbitraryResourceFileFSRef
						(AppResources_ReturnPreferenceCollectionsIconFilenameNoExtension(),
							CFSTR("icns")/* type */, iconFile))
					{
						IconRef		iconRef = nullptr;
						
						
						if (noErr == RegisterIconRefFromFSRef(kConstantsRegistry_ApplicationCreatorSignature,
																kMyIconServicesIconPreferenceCollections,
																&iconFile, &iconRef))
						{
							(OSStatus)HIToolbarItemSetIconRef(drawerDisplayItem, iconRef);
						}
					}
					if (UIStrings_Copy(kUIStrings_PreferencesWindowCollectionsDrawerShowHideName, labelCFString).ok())
					{
						(OSStatus)HIToolbarItemSetLabel(drawerDisplayItem, labelCFString);
						CFRelease(labelCFString), labelCFString = nullptr;
					}
					if (UIStrings_Copy(kUIStrings_PreferencesWindowCollectionsDrawerDescription, descriptionCFString).ok())
					{
						(OSStatus)HIToolbarItemSetHelpText(drawerDisplayItem, descriptionCFString, nullptr/* long version */);
						CFRelease(descriptionCFString), descriptionCFString = nullptr;
					}
					
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, drawerDisplayItem);
				}
				if (noErr == HIToolbarCreateItemWithIdentifier(categoryIcons, kHIToolbarSeparatorIdentifier,
																nullptr/* configuration data */, &drawerSeparatorItem))
				{
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, drawerSeparatorItem);
				}
			#endif
				
				(OSStatus)SetWindowToolbar(gPreferencesWindow, categoryIcons);
				(OSStatus)ShowHideWindowToolbar(gPreferencesWindow, true/* show */, false/* animate */);
				CFRelease(categoryIcons);
			}
		}
		
		// initialize the panel rectangle based on the NIB definition
		GetControlBounds(panelUserPane, &panelBounds);
		
		// since the user pane is only used to define these boundaries, it can be destroyed now
		CFRelease(panelUserPane), panelUserPane = nullptr;
		
		// create category panels - call these routines in the order you want their category buttons to appear
		installPanel(PrefPanelGeneral_New());
		installPanel(PrefPanelSessions_New());
		installPanel(PrefPanelMacros_New());
		installPanel(PrefPanelTranslations_New());
		installPanel(PrefPanelTerminals_New());
		installPanel(PrefPanelFormats_New());
		installPanel(PrefPanelKiosk_New());
		installPanel(PrefPanelScripts_New());
		
		// install a callback that responds as the main window is resized
		{
			HISize		initialSize = CGSizeMake(panelBounds.right - panelBounds.left,
													panelBounds.bottom - panelBounds.top);
			HISize		idealWindowContentSize = CGSizeMake(0, 0);
			Point		deltaSize;
			
			
			findBestPanelSize(initialSize, idealWindowContentSize);
			sizePanels(idealWindowContentSize);
			
			gPreferencesWindowResizeHandler.install(gPreferencesWindow, handleNewMainWindowSize, nullptr/* user data */,
													idealWindowContentSize.width/* minimum width */,
													idealWindowContentSize.height/* minimum height */,
													idealWindowContentSize.width + 500/* arbitrary maximum width */,
													idealWindowContentSize.height + 350/* arbitrary maximum height */);
			assert(gPreferencesWindowResizeHandler.isInstalled());
			
			// remember this maximum width, it is used to set the window size
			// whenever a new panel is chosen
			gPreferencesWindowResizeHandler.getWindowMaximumSize(gMaximumWindowSize.width, gMaximumWindowSize.height);
			
			// use the preferred rectangle, if any; since a resize handler was
			// installed above, simply resizing the window will cause all
			// controls to be adjusted automatically by the right amount
			SetPt(&deltaSize, STATIC_CAST(idealWindowContentSize.width, SInt16),
					STATIC_CAST(idealWindowContentSize.height, SInt16)); // initially...
			(Preferences_ResultCode)Preferences_ArrangeWindow(gPreferencesWindow, kPreferences_WindowTagPreferences,
																&deltaSize);
		}
		
		// install a callback that responds as the drawer window is resized; this is used
		// primarily to enforce a maximum drawer height, not to allow a resizable drawer
		{
			Rect		currentBounds;
			OSStatus	error = noErr;
			
			
			error = GetWindowBounds(gDrawerWindow, kWindowContentRgn, &currentBounds);
			assert_noerr(error);
			gDrawerWindowResizeHandler.install(gDrawerWindow, handleNewDrawerWindowSize, nullptr/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												currentBounds.right - currentBounds.left/* maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
			assert(gDrawerWindowResizeHandler.isInstalled());
		}
		
		// set the initial offset of the drawer
		{
			float		leadingOffset = 52/* arbitrary; the current height of a standard toolbar */;
			float		trailingOffset = kWindowOffsetUnchanged;
			OSStatus	error = noErr;
			
			
			if (FlagManager_Test(kFlagOS10_5API))
			{
				// on Leopard, the aesthetically-pleasing spot for the drawer
				// is slightly higher
				--leadingOffset;
			}
			error = SetDrawerOffsets(gDrawerWindow, leadingOffset, trailingOffset);
		}
		
		// install a callback that disposes of the window properly when it should be closed
		{
			EventTypeSpec const		whenWindowClosing[] =
									{
										{ kEventClassWindow, kEventWindowClose }
									};
			OSStatus				error = noErr;
			
			
			gPreferencesWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
			error = InstallWindowEventHandler(gPreferencesWindow, gPreferencesWindowClosingUPP, GetEventTypeCount(whenWindowClosing),
												whenWindowClosing, nullptr/* user data */,
												&gPreferencesWindowClosingHandler/* event handler reference */);
			assert(error == noErr);
		}
		
		// install a callback that finds out about changes to available preferences collections
		gPreferenceChangeEventListener = ListenerModel_NewStandardListener(preferenceChanged);
		{
			Preferences_ResultCode		error = kPreferences_ResultCodeSuccess;
			
			
			error = Preferences_ListenForChanges(gPreferenceChangeEventListener, kPreferences_ChangeContextName,
													false/* call immediately to get initial value */);
			assert(kPreferences_ResultCodeSuccess == error);
			error = Preferences_ListenForChanges(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts,
													true/* call immediately to get initial value */);
			assert(kPreferences_ResultCodeSuccess == error);
		}
	}
}// init


/*!
To add a panel to the preferences window from
an abstract Panel object, use this method.  A
preference panel should define a name and an
icon (as recommended by the Panel module anyway),
which is used to render a bevel button for the
panel.  The preferences window automatically
adapts itself for the panel, creating a new icon
to represent it and firing notifications to the
panel whenever necessary.  If you respond to the
standard Panel interface, the panel will function
perfectly in the preferences window.

There is no way to arrange preference panels
except by invoking PrefsWindow_InstallPanel() for
each panel, in the order you want the panels to
appear in the toolbar.

(3.0)
*/
static void
installPanel	(Panel_Ref		inPanel)
{
	newPanelSelector(inPanel);
	
	// allow the panel to create its controls by providing a pointer to the containing window
	Panel_SendMessageCreateViews(inPanel, gPreferencesWindow);
}// installPanel


/*!
Responds to changes in the data browser.  Currently,
most of the possible messages are ignored, but this
is used to determine when to update the panel views.

(3.1)
*/
static pascal void
monitorDataBrowserItems		(ControlRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	Preferences_ContextRef	prefsContext = REINTERPRET_CAST(inItemID, Preferences_ContextRef);
	
	
	assert(gDataBrowserForCollections == inDataBrowser);
	switch (inMessage)
	{
	case kDataBrowserItemSelected:
		// update the panel views to match the newly-selected Favorite
		chooseContext(prefsContext);
		break;
	
	default:
		// not all messages are supported
		break;
	}
}// monitorDataBrowserItems


/*!
To create a new list item for the preferences window that
represents the specified panel, use this method.  The
selector�s title and icon are acquired from the specified
panel.

(3.0)
*/
static void
newPanelSelector	(Panel_Ref		inPanel)
{
	MyPanelDataPtr		dataPtr = new MyPanelData(inPanel, ++gPanelChoiceListLastRowIndex);
	
	
	if (nullptr != dataPtr)
	{
		// store the panel
		dataPtr->panel = inPanel;
		gPanelList().push_back(dataPtr);
		
		// remember panel position and command ID, for later event handling
		gIndicesByCommandID()[Panel_ReturnShowCommandID(inPanel)] = dataPtr->listIndex;
		
		// create a toolbar item for this panel
		{
			HIToolbarRef	categoryIcons = nullptr;
			
			
			if (noErr == GetWindowToolbar(gPreferencesWindow, &categoryIcons))
			{
				HIToolbarItemRef	newItem = nullptr;
				OptionBits			itemOptions = kHIToolbarItemCantBeRemoved | kHIToolbarItemAnchoredLeft;
				
				
				if ((0 == dataPtr->listIndex) &&
					(FlagManager_Test(kFlagOS10_4API)))
				{
					itemOptions |= FUTURE_SYMBOL(1 << 7, kHIToolbarItemSelected);
				}
				if (noErr == HIToolbarItemCreate(Panel_ReturnKind(inPanel), itemOptions, &newItem))
				{
					CFStringRef		descriptionCFString = nullptr;
					
					
					// PrefsWindow_Done() should clean up this retain
					CFRetain(newItem), gCategoryToolbarItems().push_back(newItem);
					
					// set the icon, label and tooltip
					(OSStatus)Panel_SetToolbarItemIconAndLabel(newItem, inPanel);
					Panel_GetDescription(inPanel, descriptionCFString);
					if (nullptr != descriptionCFString)
					{
						(OSStatus)HIToolbarItemSetHelpText(newItem, descriptionCFString, nullptr/* long version */);
					}
					
					// add the item to the toolbar
					(OSStatus)HIToolbarAppendItem(categoryIcons, newItem);
				}
			}
		}
	}
}// newPanelSelector


/*!
Invoked whenever a monitored preference value is changed
(see TerminalView_Init() to see which preferences are
monitored).  This routine responds by ensuring that internal
variables are up to date for the changed preference.

(3.0)
*/
static void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeContextName:
		// a context has been renamed; refresh the list
		refreshDisplay();
		break;
	
	case kPreferences_ChangeNumberOfContexts:
		// contexts were added or removed; destroy and rebuild the list
		rebuildList();
		refreshDisplay();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Destroys all the items in the data browser and reconstructs
them based on the list of contexts for the current preferences
class.  It is appropriate to call this whenever a change to
the set of items is made (choosing a new panel, adding or
removing an item, etc.).

FUTURE: Callbacks could send specific context references, so
that changes do not necessarily have to target the entire list
in this way.

(3.1)
*/
static void
rebuildList ()
{
	Preferences_Class const		kCurrentPreferencesClass = returnCurrentPreferencesClass();
	typedef std::vector< Preferences_ContextRef >	ContextsList;
	ContextsList				contextList;
	
	
	if (Preferences_GetContextsInClass(kCurrentPreferencesClass, contextList))
	{
		// start by destroying all items in the list
		(OSStatus)RemoveDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
											0/* size of array */, nullptr/* items array; nullptr = destroy all */,
											kDataBrowserItemNoProperty/* pre-sort property */);
		
		// now acquire contexts for all available names in this class,
		// and add data browser items for each of them
		for (ContextsList::const_iterator toContextRef = contextList.begin();
				toContextRef != contextList.end(); ++toContextRef)
		{
			DataBrowserItemID	ids[] = { REINTERPRET_CAST(*toContextRef, DataBrowserItemID) };
			
			
			(OSStatus)AddDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
											sizeof(ids) / sizeof(DataBrowserItemID), ids,
											kDataBrowserItemNoProperty/* pre-sort property */);
		}
	}
}// rebuildList


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the preferences toolbar.  Responds by changing
the currently-displayed panel.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// don�t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command selected from the toolbar
				switch (received.commandID)
				{
				case kHICommandNew:
					{
						// create a data browser item; this is accomplished by
						// creating and immediately saving a new named context
						// (a preferences callback elsewhere in this file is
						// then notified by the Preferences module of the change)
						Preferences_ContextRef		newContext = Preferences_NewContext
																	(returnCurrentPreferencesClass(),
																		nullptr/* name, or nullptr for automatic name */);
						Boolean						isError = false;
						
						
						if (nullptr == newContext) isError = true;
						else
						{
							Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
							
							
							prefsResult = Preferences_ContextSave(newContext);
							if (kPreferences_ResultCodeSuccess != prefsResult) isError = true;
							else
							{
								DataBrowserItemID const		kNewItemID = REINTERPRET_CAST(newContext,
																							DataBrowserItemID);
								
								
								// success!
								Preferences_ReleaseContext(&newContext);
								result = noErr;
								
								// automatically switch to editing the new item; ignore
								// errors for now because this is not critical to creating
								// the actual item
								{
									DataBrowserItemID	itemList[] = { kNewItemID };
									
									
									(OSStatus)SetDataBrowserSelectedItems
												(gDataBrowserForCollections, 1/* number of items */, itemList,
													kDataBrowserItemsAssign);
								}
								
								// attempt to help the user by automatically highlighting
								// the new item for editing; but if this fails, no big deal
								{
									HIViewRef const		kDrawerRoot = HIViewGetRoot(gDrawerWindow);
									
									
									// focus the drawer
									(OSStatus)SetUserFocusWindow(gDrawerWindow);
									
									// focus the data browser itself
									(OSStatus)HIViewSetNextFocus(kDrawerRoot, gDataBrowserForCollections);
									(OSStatus)HIViewAdvanceFocus(kDrawerRoot, 0L/* event modifiers */);
									
									// open the new item for editing
									(OSStatus)SetDataBrowserEditItem(gDataBrowserForCollections, kNewItemID,
																		kMyDataBrowserPropertyIDSets);
								}
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kHICommandClear:
					{
						// delete a data browser item; this is accomplished by
						// destroying the underlying context (a preferences callback
						// elsewhere in this file is then notified by the Preferences
						// module of the change)
						DataBrowserItemID	selectedID = 0;
						OSStatus			error = noErr;
						Boolean				isError = false;
						
						
						error = GetDataBrowserSelectionAnchor(gDataBrowserForCollections,
																&selectedID, &selectedID);
						if (noErr != error) isError = true;
						else
						{
							Preferences_ContextRef		deletedContext = REINTERPRET_CAST(selectedID,
																							Preferences_ContextRef);
							
							
							if (nullptr == deletedContext) isError = true;
							else
							{
								Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
								
								
								prefsResult = Preferences_ContextDeleteSaved(deletedContext);
								if (kPreferences_ResultCodeSuccess != prefsResult) isError = true;
								else
								{
									// success!
									result = noErr;
								}
							}
						}
						
						if (isError)
						{
							Sound_StandardAlert();
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kCommandShowHidePrefCollectionsDrawer:
					result = ToggleDrawer(gDrawerWindow);
					break;
				
				case kCommandDisplayPrefPanelFormats:
				case kCommandDisplayPrefPanelGeneral:
				case kCommandDisplayPrefPanelKiosk:
				case kCommandDisplayPrefPanelMacros:
				case kCommandDisplayPrefPanelScripts:
				case kCommandDisplayPrefPanelSessions:
				case kCommandDisplayPrefPanelTerminals:
					assert(false == gIndicesByCommandID().empty());
					choosePanel(gIndicesByCommandID()[received.commandID]);
					result = noErr;
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the preferences window.

(3.1)
*/
static pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			PrefsWindow_Remove();
			result = noErr; // event is completely handled
		}
	}
	
	return result;
}// receiveWindowClosing


/*!
Redraws the list display.  Use this when you have caused
it to change in some way (by changing a preference elsewhere
in the application, for example).

(3.1)
*/
static void
refreshDisplay ()
{
	(OSStatus)UpdateDataBrowserItems(gDataBrowserForCollections, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDSets);
}// refreshDisplay


/*!
Returns the preference class to use for settings,
based on the panel that is showing.  Most of the
time the class will be for general preferences,
however if a collection drawer is showing then a
collection class will be returned.

(3.1)
*/
static Preferences_Class
returnCurrentPreferencesClass ()
{
	Preferences_Class	result = kPreferences_ClassGeneral;
	Panel_Kind			currentPanelKind = Panel_ReturnKind(gCurrentPanel);
	
	
	assert(nullptr != currentPanelKind);
	if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorFormats, currentPanelKind,
												kCFCompareBackwards))
	{
		result = kPreferences_ClassFormat;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorMacros, currentPanelKind,
													kCFCompareBackwards))
	{
		result = kPreferences_ClassMacroSet;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorSessions, currentPanelKind,
													kCFCompareBackwards))
	{
		result = kPreferences_ClassSession;
	}
	else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_PrefPanelDescriptorTerminals, currentPanelKind,
													kCFCompareBackwards))
	{
		result = kPreferences_ClassTerminal;
	}
	return result;
}// returnCurrentPreferencesClass


/*!
Resizes every installed panel to match the given size.
This is only intended to be used once, initially.

(3.1)
*/
static void
sizePanels	(HISize const&		inInitialSize)
{
	std::for_each(gPanelList().begin(), gPanelList().end(),
					panelResizer(inInitialSize.width, inInitialSize.height, false/* is delta */));
}// sizePanels

// BELOW IS REQUIRED NEWLINE TO END FILE

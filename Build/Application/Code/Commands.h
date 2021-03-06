/*!	\file Commands.h
	\brief A list of all command IDs, and a mechanism for
	invoking the application’s main features.
	
	A command is a series of primitive actions that leads to a
	result; usually, there is a menu item for each command (but
	this is not required; for example, a command might be used
	to operate a toolbar item).
	
	This TEMPORARILY serves as a single point of binding for
	Cocoa-based menu commands and other interface elements.  It
	is necessary because Carbon-based windows and Cocoa-based
	windows DO NOT mix all that well, and commands generally
	will not work in the usual Cocoa first-responder way.  The
	temporary work-around is to implement most commands here,
	and bind only to these classes (not the first responder),
	so that they can handle Carbon/Cocoa details properly.
	When ALL Carbon-based windows are finally gone, it will
	then be possible to migrate the various methods into the
	most appropriate classes/files, e.g. for specific windows.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
typedef void* SEL;
class NSMenu;
class NSMenuItem;
#endif
#include <CoreServices/CoreServices.h>

// library includes
#include <ResultCode.template.h>

// application includes
#include <QuillsPrefs.h>



#pragma mark Constants

typedef ResultCode< UInt16 >	Commands_Result;
Commands_Result const	kCommands_ResultOK(0);				//!< no error
Commands_Result const	kCommands_ResultParameterError(1);	//!< bad input - for example, invalid listener type

enum Commands_NameType
{
	kCommands_NameTypeDefault		= 0,	//!< the name of the command in normal context (such as in a menu item)
	kCommands_NameTypeShort			= 1		//!< a short version of the name (such as in a toolbar item)
};

/*!
Command IDs

These are all deprecated and are being gradually replaced
by methods on objects in the Cocoa responder chain.
*/

// Terminal menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandBellEnabled						'Bell'
#define kCommandEcho							'Echo'
#define kCommandWrapMode						'Wrap'
#define kCommandClearScreenSavesLines			'CSSL'
#define kCommandClearEntireScrollback			'ClSB'
#define kCommandResetTerminal					'RTrm'

// terminal view page control
#define kCommandTerminalViewPageUp				'TVPU'
#define kCommandTerminalViewPageDown			'TVPD'
#define kCommandTerminalViewHome				'TVPH'
#define kCommandTerminalViewEnd					'TVPE'

// commands currently used only in dialogs
#define kCommandDisplayPrefPanelFormats			'SPrF'		// “Preferences“ window
#define kCommandDisplayPrefPanelFormatsANSI		'SPFA'		// multiple interfaces
#define kCommandDisplayPrefPanelFormatsNormal	'SPFN'		// multiple interfaces
#define kCommandDisplayPrefPanelGeneral			'SPrG'		// “Preferences“ window
#define kCommandDisplayPrefPanelMacros			'SPrM'		// “Preferences“ window
#define kCommandDisplayPrefPanelSessions		'SPrS'		// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsDataFlow	'SPSD'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsGraphics	'SPSG'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsKeyboard	'SPSK'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsResource	'SPSR'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminals		'SPrT'		// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsEmulation	'SPTE'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsHacks	'SPTH'		// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsOptions	'SPTO'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsScreen	'SPTS'		// “Preferences“ window
#define kCommandDisplayPrefPanelTranslations	'SPrX'		// “Preferences“ window
#define kCommandDisplayPrefPanelWorkspaces		'SPrW'		// “Preferences“ window

/*!
These MUST agree with "MainMenuCocoa.xib".  In the Carbon days,
these were menu IDs.  For Cocoa, they are the "tag" values on
each of the menus in the main menu.  So you could ask NSApp for
"mainMenu", and then use "itemWithTag" with an ID below to find
an NSMenuItem for the title, whose "submenu" is the NSMenu
containing all of the items.
*/
enum
{
	kCommands_MenuIDApplication = 512,
	kCommands_MenuIDFile = 513,
	kCommands_MenuIDEdit = 514,
	kCommands_MenuIDView = 515,
	kCommands_MenuIDTerminal = 516,
	kCommands_MenuIDKeys = 517,
	kCommands_MenuIDMacros = 518,
	kCommands_MenuIDWindow = 519,
		kCommands_MenuItemIDPrecedingWindowList = 123,
	kCommands_MenuIDHelp = 520,
	kCommands_MenuIDDebug = 521
};

#pragma mark Types

struct Commands_ExecutionEventContext
{
	UInt32		commandID;		//!< which command the event is for
};
typedef Commands_ExecutionEventContext*		Commands_ExecutionEventContextPtr;

#ifdef __OBJC__

/*!
Implements an interface for menu commands to target.  See
"MainMenuCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Commands_Executor : NSObject< NSUserInterfaceValidations > //{
{
@private
	NSString*	_fullScreenCommandName;
}

// class methods
	+ (instancetype)
	sharedExecutor;

// accessors
	@property (strong) NSString*
	fullScreenCommandName;

// new methods: explicit validation (rarely needed)
	- (BOOL)
	defaultValidationForAction:(SEL)_
	sender:(id)_;
	- (BOOL)
	validateAction:(SEL)_
	sender:(id)_;

// new methods: menu items
	- (NSMenuItem*)
	newMenuItemForAction:(SEL)_
	itemTitle:(NSString*)_
	ifEnabled:(BOOL)_;

@end //}

/*!
Implements NSApplicationDelegate and NSApplicationNotifications;
see Commands.mm.
*/
@interface Commands_Executor (Commands_ApplicationCoreEvents) @end

/*!
Actions typically associated with the Edit menu.
*/
@interface Commands_Executor (Commands_Editing) //{

// actions
	- (IBAction)
	performUndo:(id)_;
	- (IBAction)
	performRedo:(id)_;

@end //}

/*!
Actions that create new terminal-based sessions.
*/
@interface Commands_Executor (Commands_OpeningSessions) //{

// actions
	- (IBAction)
	performNewDefault:(id)_;
	- (IBAction)
	performNewByFavoriteName:(id)_;
	- (IBAction)
	performNewLogInShell:(id)_;
	- (IBAction)
	performNewShell:(id)_;
	- (IBAction)
	performNewCustom:(id)_;
	- (IBAction)
	performRestoreWorkspaceDefault:(id)_;
	- (IBAction)
	performRestoreWorkspaceByFavoriteName:(id)_;
	- (IBAction)
	performOpen:(id)_;
	- (IBAction)
	performDuplicate:(id)_;
	- (IBAction)
	performSaveAs:(id)_;

// methods of the form required by setEventHandler:andSelector:forEventClass:andEventID:
	- (void)
	receiveGetURLEvent:(NSAppleEventDescriptor*)_
	replyEvent:(NSAppleEventDescriptor*)_;

@end //}

/*!
Actions related to vector graphics windows.
*/
@interface Commands_Executor (Commands_OpeningVectorGraphics) //{

// actions
	- (IBAction)
	performNewTEKPage:(id)_;
	- (IBAction)
	performPageClearToggle:(id)_;

@end //}

/*!
Actions that cause Internet addresses to be accessed.
*/
@interface Commands_Executor (Commands_OpeningWebPages) //{

// actions
	- (IBAction)
	performCheckForUpdates:(id)_;
	- (IBAction)
	performGoToMainWebSite:(id)_;
	- (IBAction)
	performProvideFeedback:(id)_;

@end //}

/*!
Actions related to macros.
*/
@interface Commands_Executor (Commands_ManagingMacros) //{

// actions
	- (IBAction)
	performActionForMacro:(id)_;
	- (IBAction)
	performMacroSwitchNone:(id)_;
	- (IBAction)
	performMacroSwitchDefault:(id)_;
	- (IBAction)
	performMacroSwitchByFavoriteName:(id)_;
	- (IBAction)
	performMacroSwitchNext:(id)_;
	- (IBAction)
	performMacroSwitchPrevious:(id)_;

@end //}

/*!
Actions to configure terminal event handlers.
*/
@interface Commands_Executor (Commands_ManagingTerminalEvents) //{

// actions
	- (IBAction)
	performBellToggle:(id)_;
	- (IBAction)
	performSetActivityHandlerNone:(id)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnNext:(id)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnIdle:(id)_;
	- (IBAction)
	performSetActivityHandlerSendKeepAliveOnIdle:(id)_;

@end //}

/*!
Actions affecting keyboard behavior in terminal windows.
*/
@interface Commands_Executor (Commands_ManagingTerminalKeyMappings) //{

// actions
	- (IBAction)
	performDeleteMapToBackspace:(id)_;
	- (IBAction)
	performDeleteMapToDelete:(id)_;
	- (IBAction)
	performEmacsCursorModeToggle:(id)_;
	- (IBAction)
	performLocalPageKeysToggle:(id)_;
	- (IBAction)
	performMappingCustom:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutRxvt:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutVT220:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermX11:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermXFree86:(id)_;

@end //}

/*!
Actions to change various terminal behaviors.
*/
@interface Commands_Executor (Commands_ManagingTerminalSettings) //{

// actions
	- (IBAction)
	performLineWrapToggle:(id)_;
	- (IBAction)
	performLocalEchoToggle:(id)_;
	- (IBAction)
	performReset:(id)_;
	- (IBAction)
	performSaveOnClearToggle:(id)_;
	- (IBAction)
	performScrollbackClear:(id)_;

@end //}

/*!
Actions that affect a window’s properties, placement and size.
*/
@interface Commands_Executor (Commands_ModifyingWindows) //{

// actions
	- (IBAction)
	performCloseAll:(id)_;
	- (IBAction)
	performMiniaturizeAll:(id)_;
	- (IBAction)
	performZoomAll:(id)_;
	- (IBAction)
	mergeAllWindows:(id)_; // match OS name of selector (available only in later OS versions)
	- (IBAction)
	moveTabToNewWindow:(id)_; // match OS name of selector (available only in later OS versions)
	- (IBAction)
	performArrangeInFront:(id)_;
	- (IBAction)
	performHideWindow:(id)_;
	- (IBAction)
	performHideOtherWindows:(id)_;
	- (IBAction)
	performMaximize:(id)_;
	- (IBAction)
	performMoveWindowRight:(id)_;
	- (IBAction)
	performMoveWindowLeft:(id)_;
	- (IBAction)
	performMoveWindowDown:(id)_;
	- (IBAction)
	performMoveWindowUp:(id)_;
	- (IBAction)
	performRename:(id)_;
	- (IBAction)
	performShowHiddenWindows:(id)_;

@end //}

/*!
Actions that display specific windows.
*/
@interface Commands_Executor (Commands_ShowingPanels) //{

// actions
	- (IBAction)
	orderFrontAbout:(id)_;
	- (IBAction)
	orderFrontCommandLine:(id)_;
	- (IBAction)
	orderFrontContextualHelp:(id)_;
	- (IBAction)
	orderFrontControlKeys:(id)_;
	- (IBAction)
	orderFrontDebuggingOptions:(id)_;
	- (IBAction)
	orderFrontIPAddresses:(id)_;
	- (IBAction)
	orderFrontPreferences:(id)_;
	- (IBAction)
	orderFrontSessionInfo:(id)_;
	- (IBAction)
	orderFrontVT220FunctionKeys:(id)_;
	- (IBAction)
	orderFrontVT220Keypad:(id)_;
	- (IBAction)
	toggleClipboard:(id)_;

@end //}

/*!
Actions to enter or exit Full Screen or tab Exposé.
*/
@interface Commands_Executor (Commands_SwitchingModes) //{

// actions
	- (IBAction)
	toggleFullScreen:(id)_;

@end //}

/*!
Actions to cycle through windows.
*/
@interface Commands_Executor (Commands_SwitchingWindows) //{

// actions
	- (IBAction)
	orderFrontNextWindow:(id)_;
	- (IBAction)
	orderFrontNextWindowHidingPrevious:(id)_;
	- (IBAction)
	orderFrontPreviousWindow:(id)_;
	- (IBAction)
	orderFrontSpecificWindow:(id)_;

@end //}

/*!
Actions related to printing.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_Printing //{

// actions
	- (IBAction)
	performPrintScreen:(id)_;
	- (IBAction)
	performPrintSelection:(id)_;

@end //}

/*!
Actions for controlling data flow of a session, such as
suspend and resume.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_SessionThrottling //{

@optional

// actions
	- (IBAction)
	performInterruptProcess:(id)_;
	- (IBAction)
	performJumpScrolling:(id)_;
	- (IBAction)
	performSuspendToggle:(id)_;

@end //}

/*!
Actions for accessing text via standard system commands.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardEditing //{

@required

// actions
	- (IBAction)
	copy:(id)_;

@optional

// actions
	- (IBAction)
	paste:(id)_;
	- (IBAction)
	selectAll:(id)_;
	- (IBAction)
	selectNone:(id)_;

@end //}

/*!
Actions based on finding locations within a text buffer.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardSearching //{

@required

// actions
	- (IBAction)
	performFind:(id)_;

@optional

// actions
	- (IBAction)
	performFindCursor:(id)_;
	- (IBAction)
	performFindNext:(id)_;
	- (IBAction)
	performFindPrevious:(id)_;
	- (IBAction)
	performSendMenuItemText:(id)_; // from completions menu; “types” the menu item title text
	- (IBAction)
	performShowCompletions:(id)_;

@end //}

/*!
Actions for speech control via standard system commands.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardSpeechHandling //{

// actions
	- (IBAction)
	performSpeechToggle:(id)_;
	- (IBAction)
	startSpeaking:(id)_;
	- (IBAction)
	stopSpeaking:(id)_;

@end //}

/*!
Actions to change number of rows/columns in terminal views.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalScreenResizing //{

@optional

// actions
	- (IBAction)
	performScreenResizeCustom:(id)_;
	- (IBAction)
	performScreenResizeNarrower:(id)_;
	- (IBAction)
	performScreenResizeShorter:(id)_;
	- (IBAction)
	performScreenResizeStandard:(id)_;
	- (IBAction)
	performScreenResizeTall:(id)_;
	- (IBAction)
	performScreenResizeTaller:(id)_;
	- (IBAction)
	performScreenResizeWide:(id)_;
	- (IBAction)
	performScreenResizeWider:(id)_;

@end //}

/*!
Actions that affect fonts, colors and other format settings.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TextFormatting //{

@optional

// actions
	- (IBAction)
	performFormatByFavoriteName:(id)_;
	- (IBAction)
	performFormatCustom:(id)_;
	- (IBAction)
	performFormatDefault:(id)_;
	- (IBAction)
	performFormatTextBigger:(id)_;
	- (IBAction)
	performFormatTextMaximum:(id)_;
	- (IBAction)
	performFormatTextSmaller:(id)_;

@end //}

/*!
Actions for opening a selected URL.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_URLSelectionHandling //{

// actions
	- (IBAction)
	performOpenURL:(id)_;

@end //}

#endif



#pragma mark Public Methods

//!\name Initialization
//@{

void
	Commands_Init							();

void
	Commands_Done							();

//@}

//!\name Executing Commands
//@{

// WARNING: NOT THREAD SAFE, USE Commands_ExecuteByIDUsingEvent() TO INSERT A COMMAND INTO THE MAIN THREAD’S QUEUE
Boolean
	Commands_ExecuteByID					(UInt32						inCommandID);

Boolean
	Commands_ExecuteByIDUsingEvent			(UInt32						inCommandID,
											 void*						inUnusedLegacyPtr = nullptr);

#ifdef __OBJC__
Boolean
	Commands_ViaFirstResponderPerformSelector	(SEL					inSelector,
											 id							inObjectOrNil = nil);
#endif

//@}

//!\name Cocoa Menu Utilities
//@{

Commands_Result
	Commands_InsertPrefNamesIntoMenu		(Quills::Prefs::Class		inClass,
											 NSMenu*					inoutMenu,
											 SInt64						inAtItemIndex,
											 SInt16						inInitialIndent,
											 SEL						inAction);

#ifdef __OBJC__
NSMenuItem*
	Commands_NewMenuItemForAction			(SEL						inActionSelector,
											 CFStringRef				inPreferredTitle,
											 Boolean					inMustBeEnabled = false);
#endif

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE

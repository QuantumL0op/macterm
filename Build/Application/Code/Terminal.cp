/*###############################################################

	Terminal.cp
	
	MacTelnet
		© 1998-2011 by Kevin Grant.
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

#include "UniversalDefines.h"

// standard-C includes
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

// standard-C++ includes
#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// GNU compiler includes
#include <ext/algorithm>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <Console.h>
#include <Cursors.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "Commands.h"
#include "DebugInterface.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "Preferences.h"
#include "PrintTerminal.h"
#include "Session.h"
#include "StreamCapture.h"
#include "Terminal.h"
#include "TerminalSpeaker.h"
#include "TerminalView.h"
#include "TextTranslation.h"
#include "UIStrings.h"
#include "VTKeys.h"



#pragma mark Constants
namespace {

/*!
A parser state represents a recent history of input that limits
what can happen next (based on future input).

The list below contains generic names, however the same values
are often used to define aliases in specific terminal classes
(like My_VT100).  This ties a terminal-specific state to a
generic one, allowing the default parser to do most of the state
determination/transition work on behalf of all terminals!
*/
typedef UInt32 My_ParserState;
typedef std::pair< My_ParserState, My_ParserState >		My_ParserStatePair;
enum
{
	// key states
	kMy_ParserStateInitial						= 'init',	//!< the very first state, no characters have yet been entered
	kMy_ParserStateAccumulateForEcho			= 'echo',	//!< no sense could be made of the input, so gather it for later translation and display
	
	// generic states - these are automatically handled by the default state determinant based on the data stream
	kMy_ParserStateSeenNull						= 'Ctl@',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlA					= 'CtlA',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlB					= 'CtlB',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlC					= 'CtlC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlD					= 'CtlD',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlE					= 'CtlE',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlF					= 'CtlF',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlG					= 'CtlG',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlH					= 'CtlH',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlI					= 'CtlI',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlJ					= 'CtlJ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlK					= 'CtlK',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlL					= 'CtlL',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlM					= 'CtlM',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlN					= 'CtlN',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlO					= 'CtlO',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlP					= 'CtlP',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlQ					= 'CtlQ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlR					= 'CtlR',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlS					= 'CtlS',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlT					= 'CtlT',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlU					= 'CtlU',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlV					= 'CtlV',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlW					= 'CtlW',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlX					= 'CtlX',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlY					= 'CtlY',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenControlZ					= 'CtlZ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC						= 'cESC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracket			= 'ESC[',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParams	= 'E[;;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsA	= 'E[;A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsB	= 'E[;B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsc	= 'E[;c',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsC	= 'E[;C',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsd	= 'E[;d',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsD	= 'E[;D',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsE	= 'E[;E',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsf	= 'E[;f',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsF	= 'E[;F',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsg	= 'E[;g',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsG	= 'E[;G',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsh	= 'E[;h',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsH	= 'E[;H',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsi	= 'E[;i',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsI	= 'E[;I',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsJ	= 'E[;J',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsK	= 'E[;K',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsl	= 'E[;l',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsL	= 'E[;L',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsm	= 'E[;m',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsM	= 'E[;M',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsn	= 'E[;n',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsP	= 'E[;P',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsq	= 'E[;q',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsr	= 'E[;r',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamss	= 'E[;s',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsS	= 'E[;S',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsT	= 'E[;T',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsu	= 'E[;u',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsx	= 'E[;x',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsZ	= 'E[;Z',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsAt	= 'E[;@',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftSqBracketParamsBackquote	= 'E[;`',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen				= 'ESC(',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenA			= 'ES(A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParenB			= 'ES(B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen0			= 'ES(0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen1			= 'ES(1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLeftParen2			= 'ES(2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen			= 'ESC)',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenA			= 'ES)A',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParenB			= 'ES)B',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen0			= 'ES)0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen1			= 'ES)1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightParen2			= 'ES)2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket		= 'ESC]',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket0		= 'ES]0',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1		= 'ES]1',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket2		= 'ES]2',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket3		= 'ES]3',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket4		= 'ES]4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket0Semi	= 'E]0;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket1Semi	= 'E]1;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket2Semi	= 'E]2;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket3Semi	= 'E]3;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCRightSqBracket4Semi	= 'E]4;',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCA						= 'ESCA',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCB						= 'ESCB',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCC						= 'ESCC',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCc						= 'ESCc',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCD						= 'ESCD',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCE						= 'ESCE',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCF						= 'ESCF',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCG						= 'ESCG',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCH						= 'ESCH',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCI						= 'ESCI',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCJ						= 'ESCJ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCK						= 'ESCK',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCM						= 'ESCM',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCY						= 'ESCY',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCZ						= 'ESCZ',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC7						= 'ESC7',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESC8						= 'ESC8',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound					= 'ESC#',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound3				= 'ES#3',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound4				= 'ES#4',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound5				= 'ES#5',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound6				= 'ES#6',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCPound8				= 'ES#8',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCEquals				= 'ESC=',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCLessThan				= 'ESC<',	//!< generic state used to define emulator-specific states, below
	kMy_ParserStateSeenESCGreaterThan			= 'ESC>',	//!< generic state used to define emulator-specific states, below
	// note that a real backslash is a very common escape character, and
	// since state codes tend to be printed, it would screw up the output;
	// so the convention is broken in this case, and "B/" is used instead
	kMy_ParserStateSeenESCBackslash				= 'ESB/',	//!< generic state used to define emulator-specific states, below
};

UInt16 const					kNumberOfScrollbackRowsToAllocateAtOnce = 100;
TerminalTextAttributes const	kTerminalTextAttributesAllOff = 0L;

char const		kMy_TabSet		= 'x';	//!< in "tabSettings" field of terminal structure, characters with this value mark tab stops
char const		kMy_TabClear	= ' ';	//!< in "tabSettings" field of terminal structure, all characters not marking tab stops have this value
UInt8 const		kMy_TabStop		= 8;	//!< number of characters between normal tab stops

enum
{
	kMy_NumberOfCharactersPerLineMaximum = 256	//!< maximum number of columns allowed; must be a multiple of "kMy_TabStop"
};

enum My_AttributeRule
{
	kMy_AttributeRuleInitialize		= 0,	//!< newly-created lines have cleared attributes
	kMy_AttributeRuleCopyLastLine	= 1		//!< newly-created lines copy all of the attributes of the line that used to be at the end
};

enum My_CharacterSet
{
	kMy_CharacterSetVT100UnitedKingdom		= 0,	//!< ASCII character set except '#' is '£' (British pounds currency symbol)
	kMy_CharacterSetVT100UnitedStates		= 1		//!< ASCII character set
};

enum My_CharacterROM
{
	kMy_CharacterROMNormal		= 0,	//!< regular source
	kMy_CharacterROMAlternate	= 1		//!< alternate ROM (e.g. Unix programs like "top" will try to switch out)
};

enum My_GraphicsMode
{
	kMy_GraphicsModeOff			= 0,	//!< no graphics glyphs will appear
	kMy_GraphicsModeOn			= 1		//!< non-printable ASCII will become VT graphics characters
};

typedef UInt32 My_LEDBits;
enum
{
	kMy_LEDBitsLight1			= (1 << 0),		//!< set if LED 1 is on
	kMy_LEDBitsLight2			= (1 << 1),		//!< set if LED 2 is on
	kMy_LEDBitsLight3			= (1 << 2),		//!< set if LED 3 is on
	kMy_LEDBitsLight4			= (1 << 3),		//!< set if LED 4 is on
	kMy_LEDBitsAllOff			= 0L			//!< cleared if all LEDs are off
};

/*!
In VT102 terminals and beyond, there are multiple
printing modes that can be active.  In order to
ensure that as few printing dialogs appear as
possible, all possible modes must be off before
printing will occur.
*/
typedef UInt8 My_PrintingMode;
enum
{
	kMy_PrintingModeAutoPrint			= (1 << 0),		//!< MC private (VT102): true only if terminal-rendered lines are also sent to the printer
	kMy_PrintingModePrintController		= (1 << 1),		//!< MC (VT102): true only if received data is being copied to the printer, verbatim
};

enum
{
	kMy_MaximumANSIParameters = 16		// when <ESC>'['<param>[;<param>][;<param>]... sequences are encountered,
										// this will be the maximum <param>s allowed
};

} // anonymous namespace

#pragma mark Callbacks
namespace {

struct My_Emulator;			// declared here because the callback declarations use it (defined later)
struct My_ScreenBuffer;		// declared here because the callback declarations use it (defined later)

/*!
Emulator Echo-Data Routine

Translates the given data from the specified screen’s input
text encoding, and sends it to the screen (probably using the
utility function echoCFString()).

This is a separate emulator function because it is occasionally
important for emulators to customize it, e.g. a “dumb” terminal.

WARNING:	The specified buffer is a limited slice of the
			overall data stream.  It is not guaranteed to
			terminate at the end of a full sequence of
			characters that you are interested in, so naïve
			translation will not work.  You must be prepared
			to “backtrack” and ignore zero or more bytes at
			the end of the buffer, to find a valid segment.
			TextTranslation_PersistentCFStringCreate() is
			very useful for this!
*/
typedef UInt32 (*My_EmulatorEchoDataProcPtr)	(My_ScreenBuffer*	inDataPtr,
												 UInt8 const*		inBuffer,
												 UInt32				inLength);
inline UInt32
invokeEmulatorEchoDataProc	(My_EmulatorEchoDataProcPtr		inProc,
							 My_ScreenBuffer*				inDataPtr,
							 UInt8 const*					inBuffer,
							 UInt32							inLength)
{
	return (*inProc)(inDataPtr, inBuffer, inLength);
}

/*!
Emulator State Determinant Routine

Specifies the next state for the parser, based on the given
data and/or current state.  The number of bytes read is
returned; this is generally 1, but may be zero or more if
you used look-ahead to set a more precise next state or do
not wish to consume any bytes.

The specified buffer should use the default text encoding of
the given emulator.

WARNING:	The specified buffer is a limited slice of the
			overall data stream.  It is not guaranteed to
			terminate at the end of a full sequence of
			characters that you are interested in, so naïve
			look-ahead generally does not work.  Instead,
			move the parser to a custom scanner state until
			you have found the complete sequence you want
			(you can then continue accumulating characters
			as long as the current state is your scanner
			state, even across multiple calls).  The only
			time look-ahead is reliable in a single call is
			when you do not care about the specific data
			in the sequence (e.g. the standard echo state).

Sometimes, a state is an interrupt: entered and exited
immediately and restoring the parser to a state as if the
interrupt never occurred.  When this applies to your
suggested new state, also set "outInterrupt" to true (the
default is false).  This causes the caller to perform the
state transition, but then restore the current state and
seek another next state (with a buffer pointer that has
since advanced).

You do not need to set a new state; the default is the
current state.  However, for clarity it is recommended
that you always set the state precisely on each call.

The "outHandled" flag is used to indicate that a handler
is taking full responsibility for determining the state
for this input (even if the state is unchanged).  When
invokeEmulatorStateDeterminantProc() is invoked, the flag
is ALWAYS initialized to true, but can be cleared by the
callback.
*/
typedef UInt32 (*My_EmulatorStateDeterminantProcPtr)	(My_Emulator*			inDataPtr,
														 UInt8 const*			inBuffer,
														 UInt32					inLength,
														 My_ParserStatePair&	inNowOutNext,
														 Boolean&				outInterrupt,
														 Boolean&				outHandled);
inline UInt32
invokeEmulatorStateDeterminantProc	(My_EmulatorStateDeterminantProcPtr		inProc,
									 My_Emulator*							inEmulatorPtr,
									 UInt8 const*							inBuffer,
									 UInt32									inLength,
									 My_ParserStatePair&					inNowOutNext,
									 Boolean&								outInterrupt,
									 Boolean&								outHandled)
{
	outHandled = true; // initially...
	return (*inProc)(inEmulatorPtr, inBuffer, inLength, inNowOutNext, outInterrupt, outHandled);
}

/*!
Emulator State Transition Routine

Unlike a "My_EmulatorStateDeterminantProcPtr", which determines
what the next state should be, this routine *performs an
action* based on the *current* state.  Some context is given
(like the previous state), in case this matters to you.

The specified buffer should use the default text encoding of
the given screen’s current emulator.

The number of bytes read is returned; this is generally 0, but
may be more if you use look-ahead to absorb data (e.g. the echo
state).

The "outHandled" flag is used to indicate that a handler is
taking full responsibility for transitioning between the given
states.  When invokeEmulatorStateTransitionProc() is invoked,
the flag is ALWAYS initialized to true, but can be cleared by
the callback.
*/
typedef UInt32 (*My_EmulatorStateTransitionProcPtr)	(My_ScreenBuffer*			inDataPtr,
													 UInt8 const*				inBuffer,
													 UInt32						inLength,
													 My_ParserStatePair const&	inOldNew,
													 Boolean&					outHandled);
inline UInt32
invokeEmulatorStateTransitionProc	(My_EmulatorStateTransitionProcPtr	inProc,
									 My_ScreenBuffer*					inDataPtr,
									 UInt8 const*						inBuffer,
									 UInt32								inLength,
									 My_ParserStatePair const&			inOldNew,
									 Boolean&							outHandled)
{
	outHandled = true; // initially...
	return (*inProc)(inDataPtr, inBuffer, inLength, inOldNew, outHandled);
}

/*!
Screen Line Operation Routine

This defines a function that can be used as an iterator
over all of the rows of a virtual screen buffer (either
the main screen or the scrollback).  The specified text
buffer (which is read/write) includes the contents of
the current row.

The row number passed into the routine is relative to the
iteration currently being done; for example, if someone
iterates over lines 5 to 8 of a buffer, the line number
is 0 when starting at line 5, and maximizes at 3 when
line 8 is hit.

If your operation modifies the buffer (which is allowed),
return "true" to notify the virtual screen module that
you changed the text.  Otherwise, return "false" if you
leave the text alone.
*/
typedef void (*My_ScreenLineOperationProcPtr)	(My_ScreenBuffer*		inScreen,
												 CFMutableStringRef		inLineTextBuffer,
												 UInt16					inZeroBasedRowNumberOrNegativeForScrollbackRow,
												 void*					inContextPtr);
inline void
invokeScreenLineOperationProc	(My_ScreenLineOperationProcPtr	inUserRoutine,
								 My_ScreenBuffer*				inScreen,
								 CFMutableStringRef				inLineTextBuffer,
								 UInt16							inZeroBasedRowNumberRelativeToStartOfIterationRange,
								 void*							inContextPtr)
{
	(*inUserRoutine)(inScreen, inLineTextBuffer, inZeroBasedRowNumberRelativeToStartOfIterationRange, inContextPtr);
}

} // anonymous namespace

#pragma mark Types
namespace {

typedef UniChar*								My_TextIterator;
typedef std::map< UniChar, CFRetainRelease >	My_PrintableByUniChar;

typedef std::vector< char >						My_TabStopList;
typedef std::vector< TerminalTextAttributes >   My_TextAttributesList;

/*!
All the information associated with either the G0 or G1
character sets in a VT terminal.
*/
struct My_CharacterSetInfo
{
public:
	My_CharacterSetInfo		(My_CharacterSet	inTranslationTable,
							 My_CharacterROM	inSource,
							 My_GraphicsMode	inGraphicsMode)
	: translationTable(inTranslationTable), source(inSource), graphicsMode(inGraphicsMode)
	{
	}
	
	My_CharacterSet		translationTable;	//!< what character code translation rules are used?
	My_CharacterROM		source;				//!< which ROM are characters coming from?
	My_GraphicsMode		graphicsMode;		//!< can graphics glyphs appear?
};
typedef My_CharacterSetInfo*			My_CharacterSetInfoPtr;
typedef My_CharacterSetInfo const*		My_CharacterSetInfoConstPtr;

/*!
Represents a single line of the screen buffer of a
terminal, as well as attributes of its contents
(special styles, colors, highlighting, double-sized
text, etc.).  Since text and attributes are allocated
many lines at a time, a given line may or may not
point to the start of an allocated block, so flags
are also present to indicate which pointers should be
used to dispose memory blocks later.

The line list is an std::list (as opposed to some other
standard container like a vector or deque) because it
requires quick access to the start, end and middle, and
it requires iterators that remain valid after lines are
mucked with.  It also makes use of the splice() method
which is specific to a linked list.

NOTE:	Traditionally NCSA Telnet has used bits to
		represent the style of every single terminal
		cell.  This is memory-inefficient (albeit
		convenient at times), and also worsens linearly
		as the size of the screen increases.  It may be
		nice to implement a “style run”-based approach
		that sets attributes for ranges of text (which
		is pretty much how they’re defined anyway, when
		VT sequences arrive).  That would greatly
		reduce the number of attribute words in memory!
		The first part of this is implemented, in the
		sense that Terminal Views only see terminal data
		in terms of style runs (see the new routine
		Terminal_ForEachLikeAttributeRunDo()).
*/
struct My_ScreenBufferLine
{
	My_TextIterator				textVectorBegin;	//!< where characters exist
	My_TextIterator				textVectorEnd;		//!< for convenience; past-the-end of this buffer
	size_t						textVectorSize;		//!< for convenience; size of buffer
	CFRetainRelease				textCFString;		//!< mutable string object for which "textVectorBegin" is the storage,
													//!  so the buffer can be manipulated directly if desired
	My_TextAttributesList		attributeVector;	//!< where character attributes exist
	TerminalTextAttributes		globalAttributes;   //!< attributes that apply to every character (e.g. double-sized text)
	
	My_ScreenBufferLine ();
	~My_ScreenBufferLine ();
	
	My_ScreenBufferLine (My_ScreenBufferLine const&);
	
	My_ScreenBufferLine&
	operator = (My_ScreenBufferLine const&);
	
	bool
	operator == (My_ScreenBufferLine const&  inLine) const;
	
	void
	structureInitialize ();

private:
};
typedef std::list< My_ScreenBufferLine >		My_ScreenBufferLineList;
typedef My_ScreenBufferLineList::size_type		My_ScreenRowIndex;

/*!
A pair of rows that define the start and end rows,
inclusive, of a range in the buffer.
*/
struct My_RowBoundary
{
public:
	My_RowBoundary	(My_ScreenRowIndex		inFirstRow,
					 My_ScreenRowIndex		inLastRow)
	: firstRow(inFirstRow), lastRow(inLastRow)
	{
	}
	
	bool
	operator ==		(My_RowBoundary const&		inOther)
	{
		return ((inOther.firstRow == this->firstRow) &&
				(inOther.lastRow == this->lastRow));
	}
	
	My_ScreenRowIndex	firstRow;   //!< zero-based row number where range occurs
	My_ScreenRowIndex	lastRow;	//!< zero-based row number of the final row included in the range; 0 is topmost
									//!  main screen line, a negative line number is in the scrollback buffer
};

struct My_RowColumnBoundary
{
public:
	My_RowColumnBoundary	(UInt16				inFirstColumn,
							 My_ScreenRowIndex	inFirstRow,
							 UInt16				inLastColumn,
							 My_ScreenRowIndex	inLastRow)
	: firstColumn(inFirstColumn), lastColumn(inLastColumn), rows(inFirstRow, inLastRow)
	{
	}
	
	UInt16				firstColumn;	//!< zero-based column number where range begins
	UInt16				lastColumn;		//!< zero-based column number of the final column included in the range
	My_RowBoundary		rows;			//!< zero-based row numbers where range occurs (inclusive)
};

/*!
Represents a line of the terminal buffer, either in the
scrollback or one of the main screen (“visible”) lines.

A line iterator can traverse from the oldest scrollback
line (the beginning) all the way to the bottommost main
screen line (the end), as if all terminal lines were
stored sequentially in memory.

IMPORTANT:	Since the scrollback size is cached, it is
			important that the scrollback buffer NOT be
			changed without updating the cache.  The
			iterator holds a non-constant reference to
			the buffer, but should have no legitimate
			reason to resize the buffer.
*/
struct My_LineIterator
{
public:
	My_LineIterator		(My_ScreenBufferLineList&			inScreenBuffer,
						 My_ScreenBufferLineList&			inScrollbackBuffer,
						 My_ScreenBufferLineList&			inInitialIteratorTarget,
						 My_ScreenBufferLineList::iterator	inRowIterator)
	:
	screenBuffer(inScreenBuffer),
	scrollbackBuffer(inScrollbackBuffer),
	currentListPtr(&inInitialIteratorTarget),
	rowIterator(inRowIterator)
	{
		assert((inInitialIteratorTarget == inScreenBuffer) || (inInitialIteratorTarget == inScrollbackBuffer));
	}
	
	My_ScreenBufferLineList const&
	currentBuffer ()
	{
		return *currentListPtr;
	}
	
	My_ScreenBufferLine&
	currentLine ()
	{
		return *rowIterator; // crashes if isEnd(), which is expected (STL-like) behavior
	}
	
	//! Returns either the oldest scrollback line, or the topmost
	//! main screen line when the scrollback is empty.
	My_ScreenBufferLine&
	firstLine ()
	{
		// the back of the scrollback is used because its first line is the one
		// that is closest to the main screen, and the front (from the caller’s
		// perspective) must be the oldest scrollback line
		return (scrollbackBuffer.empty() || (currentBuffer() == screenBuffer)) ? screenBuffer.front() : scrollbackBuffer.back();
	}
	
	//! Increments the internal line pointer so that currentLine() now
	//! refers to the next line in the scrollback or screen buffer, as
	//! appropriate.  Returns a reference to the new currentLine().
	//! If the returned line is the last valid line, "isEnd" is set to
	//! true; otherwise, it is set to false.  The initial value of
	//! "isEnd" is ignored.
	My_ScreenBufferLine&
	goToNextLine	(Boolean&	isEnd)
	{
		// IMPORTANT:	The scrollback buffer is stored in the opposite
		//			sense that it is returned by this iterator: the
		//			beginning of the scrollback is the line nearest
		//			the main screen.  So, the scrollback buffer is
		//			traversed backward to reach “next lines”.
		isEnd = false;
		if ((currentBuffer() == scrollbackBuffer) &&
			(currentBuffer().empty() || (&currentBuffer().front() == &currentLine())))
		{
			// change the iterator to look at the screen buffer instead
			currentListPtr = &screenBuffer;
			rowIterator = currentListPtr->begin();
		}
		else
		{
			if ((currentBuffer() == scrollbackBuffer) && (scrollbackBuffer.begin() != rowIterator))
			{
				--rowIterator; // scrollback is inverted
			}
			else if ((currentBuffer() == screenBuffer) && (&lastLine() != &*rowIterator))
			{
				++rowIterator;
			}
			else
			{
				isEnd = true;
			}
		}
		return currentLine();
	}
	
	//! Decrements the internal line pointer so that currentLine() now
	//! refers to the previous line in the scrollback or screen buffer,
	//! as appropriate.  Returns a reference to the new currentLine().
	//! If the returned line is the first valid line, "isEnd" is set to
	//! true; otherwise, it is set to false.  The initial value of
	//! "isEnd" is ignored.
	My_ScreenBufferLine&
	goToPreviousLine	(Boolean&	isEnd)
	{
		// IMPORTANT:	The scrollback buffer is stored in the opposite
		//			sense that it is returned by this iterator: the
		//			beginning of the scrollback is the line nearest
		//			the main screen.  So, the scrollback buffer is
		//			traversed forward to reach “previous lines”.
		isEnd = false;
		if ((currentBuffer() == screenBuffer) &&
			(&currentBuffer().front() == &currentLine()))
		{
			// change the iterator to look at the scrollback buffer instead
			currentListPtr = &scrollbackBuffer;
			rowIterator = currentListPtr->begin();
		}
		else
		{
			if ((currentBuffer() == screenBuffer) && (screenBuffer.begin() != rowIterator))
			{
				--rowIterator;
			}
			else if ((currentBuffer() == scrollbackBuffer) && (&firstLine() != &*rowIterator))
			{
				++rowIterator; // scrollback is inverted
			}
			else
			{
				isEnd = true;
			}
		}
		return currentLine();
	}
	
	//! Returns the bottommost main screen line.
	My_ScreenBufferLine&
	lastLine ()
	{
		return screenBuffer.back();
	}

private:
	My_ScreenBufferLineList&			screenBuffer;			//!< the other possible target for "currentListPtr"
	My_ScreenBufferLineList&			scrollbackBuffer;		//!< one possible target for "currentListPtr"
	My_ScreenBufferLineList*			currentListPtr;			//!< the list that this iterator is pointing into
	My_ScreenBufferLineList::iterator	rowIterator;			//!< points into one of the buffer queues
};
typedef My_LineIterator*	My_LineIteratorPtr;

/*!
Represents the state of a terminal emulator, such as any
parameters collected and any pending operations.
*/
struct My_Emulator
{
public:
	struct Callbacks
	{
	public:
		Callbacks	();
		Callbacks	(My_EmulatorEchoDataProcPtr,
					 My_EmulatorStateDeterminantProcPtr,
					 My_EmulatorStateTransitionProcPtr);
		
		Boolean
		exist () const;
		
		My_EmulatorEchoDataProcPtr			dataWriter;				//!< translates data and dumps it to the screen
		My_EmulatorStateDeterminantProcPtr	stateDeterminant;		//!< figures out what the next state should be
		My_EmulatorStateTransitionProcPtr	transitionHandler;		//!< handles new parser states, driving the terminal; varies based on the emulator
	};
	
	typedef std::vector< Callbacks >		VariantChain;
	typedef std::vector< SInt16 >			ParameterList;
	typedef std::set< Preferences_Tag >		TagSet;
	
	My_Emulator		(Terminal_Emulator, CFStringRef, CFStringEncoding);
	
	Boolean
	changeTo	(Terminal_Emulator);
	
	Boolean
	supportsVariant		(Preferences_Tag);
	
	Terminal_Emulator					primaryType;			//!< VT100, VT220, etc.
	CFStringEncoding					inputTextEncoding;		//!< specifies the encoding used by the input data stream
	CFRetainRelease						answerBackCFString;		//!< similar to "primaryType", but can be an arbitrary string
	My_ParserState						currentState;			//!< state the terminal input parser is in now
	My_ParserState						stringAccumulatorState;	//!< state that was in effect when the "stringAccumulator" was recently cleared
	std::string							stringAccumulator;		//!< used to gather characters for such things as XTerm window changes
	UInt16								stateRepetitions;		//!< to guard against looping; counts repetitions of same state
	SInt16								parameterEndIndex;		//!< zero-based last parameter position in the "values" array
	ParameterList						parameterValues;		//!< all values provided for the current escape sequence
	VariantChain						preCallbackSet;			//!< callbacks invoked prior to ordinary callbacks, to allow tweaks (e.g. XTerm)
	Callbacks							currentCallbacks;		//!< emulator-type-specific handlers to drive the state machine
	Callbacks							pushedCallbacks;		//!< for emulators that can switch modes, the previous set of callbacks
	TagSet								supportedVariants;		//!< tags identifying minor features, e.g. "kPreferences_TagXTerm256ColorsEnabled"
	Boolean								addedXTerm;				//!< since multiple variants reuse these callbacks, only insert them once

protected:
	My_EmulatorEchoDataProcPtr
	returnDataWriter	(Terminal_Emulator);
	
	My_EmulatorStateDeterminantProcPtr
	returnStateDeterminant		(Terminal_Emulator);
	
	My_EmulatorStateTransitionProcPtr
	returnStateTransitionHandler	(Terminal_Emulator);
};
typedef My_Emulator*	My_EmulatorPtr;

struct My_ScreenBuffer
{
public:
	My_ScreenBuffer	(Preferences_ContextRef, Preferences_ContextRef);
	~My_ScreenBuffer ();
	
	static void
	preferenceChanged	(ListenerModel_Ref, ListenerModel_Event, void*, void*);
	
	void
	printingEnd		(Boolean = true);
	
	void
	printingReset ();
	
	CFStringRef
	returnAnswerBackMessage		(Preferences_ContextRef);
	
	Terminal_Emulator
	returnEmulator		(Preferences_ContextRef);
	
	Boolean
	returnForceSave		(Preferences_ContextRef);
	
	Session_LineEnding
	returnLineEndings ();
	
	UInt16
	returnScreenColumns		(Preferences_ContextRef);
	
	UInt16
	returnScreenRows		(Preferences_ContextRef);
	
	UInt32
	returnScrollbackRows	(Preferences_ContextRef);
	
	CFStringEncoding
	returnTextEncoding		(Preferences_ContextRef);
	
	Boolean
	returnXTerm256	(Preferences_ContextRef);
	
	Boolean
	returnXTermWindowAlteration		(Preferences_ContextRef);
	
	Preferences_ContextRef				configuration;
	My_Emulator							emulator;					//!< handles all parsing of the data stream
	SessionRef							listeningSession;			//!< may be nullptr; the currently attached session, where certain terminal reports are sent
	
	TerminalSpeaker_Ref					speaker;					//!< object that emits sound based on this terminal data;
																	//!  TEMPORARY: the speaker REALLY shouldn’t be part of the terminal data model!
	CFRetainRelease						windowTitleCFString;		//!< stores the string that the terminal considers its window title
	CFRetainRelease						iconTitleCFString;			//!< stores the string that the terminal considers its icon title
	
	ListenerModel_Ref					changeListenerModel;		//!< registry of listeners for various terminal events
	ListenerModel_ListenerRef			preferenceMonitor;			//!< listener for changes to preferences that affect a particular screen
	
	My_ScreenBufferLineList::size_type	scrollbackBufferCachedSize;	//!< linked list size is sometimes needed, but is VERY expensive to calculate;
																	//!  therefore, it is cached, and ALL code that changes "scrollbackBuffer" must
																	//!  be aware of this cached size and update it accordingly!
	My_ScreenBufferLineList				scrollbackBuffer;			//!< a double-ended queue containing all the scrollback text for the terminal;
																	//!  IMPORTANT: the FRONT of this queue is the scrollback line CLOSEST to the
																	//!  top (FRONT) of the screen buffer line queue; imagine both queues starting
																	//!  at the home line and growing outwards from one another
	My_ScreenBufferLineList				screenBuffer;				//!< a double-ended queue containing all the visible text for the terminal;
																	//!  insertion or deletion from either end is fast, other operations are slow;
																	//!  NOTE you should ONLY modify this using screen...() routines!
	std::basic_string<UInt8>			bytesToEcho;				//!< captures contiguous blocks of text to be translated and echoed
	
	// Error Counts
	//
	// Certain classes of error are exceptional, but when they occur
	// they may occur very often (e.g. extremely bad input data that
	// continues to cause errors until it is cleared out).
	//
	// To avoid the spam and resulting slowdown of logging these
	// cases, error counts are used: incremented as errors occur,
	// and decremented when a periodic handler discovers new errors
	// (except for the total, which is never reset).
	UInt32								echoErrorCount;				//!< echo errors occur when a stream of exceptionally bad data arrives (e.g.
																	//!  someone dumps binary data)
	UInt32								translationErrorCount;		//!< translation errors typically occur when the text encoding assumed either
																	//!  by the user or the running process is not actually used by some data; if
																	//!  enough of these are accumulated, the user could actually be prompted to
																	//!  reconsider the chosen translation table for the session
	UInt32								errorCountTotal;			//!< used to eventually fire "kTerminal_ChangeExcessiveErrors", if things have
																	//!  become just ridiculous
	
	My_TabStopList						tabSettings;				//!< array of characters representing tab stops; values are either kMy_TabClear
																	//!  (for most columns), or kMy_TabSet at tab columns
	
	StreamCapture_Ref					captureStream;				//!< used to stream data to a file chosen by the user
	StreamCapture_Ref					printingStream;				//!< used to stream data to a temporary file for later printing
	FSRef								printingFile;				//!< used to delete the temporary printing file when finished
	UInt8								printingModes;				//!< MC private (VT102): true only if terminal-rendered lines are also sent to the printer
	
	Boolean								bellDisabled;				//!< if true, all bell signals are completely ignored (no audio or visual)
	Boolean								cursorVisible;				//!< if true, cursor state is visible (as opposed to invisible)
	Boolean								reverseVideo;				//!< if true, foreground and background colors are swapped when rendering
	Boolean								windowMinimized;			//!< if true, the window has been *flagged* as being iconified; since this is only
																	//!  a data model, this only means that a terminal sequence has told the window to
																	//!  become iconified; synchronizing this flag with the actual window state is the
																	//!  responsibility of the Terminal Window module
	
	My_CharacterSetInfo					vtG0;						//!< the G0 character set
	My_CharacterSetInfo					vtG1;						//!< the G1 character set
	
	My_RowColumnBoundary				visibleBoundary;			//!< rows and columns forming the perimeter of the buffer’s “visible” region
	My_RowBoundary						customScrollingRegion;		//!< region that is in effect when margin set sequence is received; updated only
																	//!  when a terminal sequence to change margins is received
	
	struct
	{
		struct
		{
			My_ScreenRowIndex	numberOfRowsPermitted;  //!< maximum lines of scrollback specified by the user
														//!  (NOTE: this is inflexible; cooler scrollback handling schemes are limited by it...)
			Boolean				enabled;				//!< true if scrolled lines should be appended to the buffer automatically
		} scrollback;
		
		struct
		{
			UInt16				numberOfColumnsAllocated;	//!< how many characters per line are currently taking up memory; it follows
															//!  that this is the maximum possible value for "numberOfColumnsPermitted"
			UInt16				numberOfColumnsPermitted;	//!< maximum columns per line specified by the user; but see current.returnNumberOfColumnsPermitted()
		} visibleScreen;
	} text;
	
	My_LEDBits							litLEDs;					//!< highlighted states of terminal LEDs (lights)
	
	Boolean								mayNeedToSaveToScrollback;	//!< if true, the cursor has moved to the home position, and therefore
																	//!  a subsequent attempt to erase to the end of the line or screen
																	//!  should cause the data to be copied to the scrollback first (that is,
																	//!  of course, if the force-save flag is also set)
	Boolean								saveToScrollbackOnClear;	//!< if true, the entire screen contents are saved to the scrollback
																	//!  buffer just before being explicitly erased with a clear command
	
	Boolean								reportOnlyOnRequest;		//!< DECREQTPARM mode: determines response format and timing
	Boolean								wrapPending;				//!< set only when a character is echoed in final column
	
	Boolean								modeANSIEnabled;			//!< DECANM mode: true only if in ANSI mode (as opposed to VT52 compatibility
																	//!  mode); if the former, parameters are only recognized following the CSI
																	//!  sequence, but if the latter, ESC-character sequences are allowed instead
	Boolean								modeApplicationKeys;		//!< DECKPAM mode: true only if the keypad is in application mode
	Boolean								modeAutoWrap;				//!< DECAWM mode: true only if line wrapping is automatic
	Boolean								modeCursorKeysForApp;		//!< DECCKM mode: true only if the keypad should not act as cursor movement arrows
																	//!  (note also that the VT100 manual states this setting has no effect unless
																	//!  the terminal is also in ANSI mode and the keypad is in application mode
	Boolean								modeInsertNotReplace;		//!< IRM mode: if true, new characters overwrite the cursor position, otherwise
																	//!  they first shift existing text over by one before appearing
	Boolean								modeNewLineOption;			//!< LNM mode: if true, a line feed causes the cursor to move to the start of
																	//!  the next line, and a Return generates a CR LF sequence; if false, the
																	//!  line feed is just vertical movement of the cursor, and Return is just CR
	Boolean								modeOriginRedefined;		//!< DECOM mode: true only if the origin has been moved from its default place
																	//!  (in which case, various subsequent terminal operations must be relative to
																	//!  the origin instead of the home position)
	My_RowBoundary*						originRegionPtr;			//!< automatically set to the boundaries appropriate for the current origin mode;
																	//!  this should always be preferred when restricting the cursor, and as an offset
																	//!  when returning column or row numbers
	
    struct
	{
		Terminal_SpeechMode		mode;	//!< restrictions on when the computer may speak
	} speech;
	
	struct CurrentLineCache
	{
		CurrentLineCache	(struct My_ScreenBuffer const&		inParent)
		:
		parent(inParent),
		cursorAttributes(kNoTerminalTextAttributes),
		drawingAttributes(kNoTerminalTextAttributes),
		cursorX(0),
		cursorY(0),
		characterSetInfoPtr(nullptr)
		{
		}
		
		UInt16
		returnNumberOfColumnsPermitted ()
		const
		{
			UInt16		result = parent.text.visibleScreen.numberOfColumnsPermitted;
			
			
			if (STYLE_IS_DOUBLE_ANY(this->cursorAttributes)) result = INTEGER_HALVED(result);
			return result;
		}
		
		struct My_ScreenBuffer const&	parent;
		TerminalTextAttributes			cursorAttributes;		//!< text attributes for the position under the cursor; useful for rendering the cursor
																//!  in a way that does not clash with these attributes (e.g. by choosing opposite colors)
		TerminalTextAttributes			drawingAttributes;		//!< text attributes for the line the cursor is on; these should be updated as the cursor
																//!  moves, or as terminal sequences are processed by the emulator
		SInt16							cursorX;				//!< column number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_ScreenRowIndex				cursorY;				//!< row number of current cursor position;
																//!  WARNING: do not change this value except with a moveCursor...() routine
		My_CharacterSetInfoPtr			characterSetInfoPtr;	//!< pointer to either G0 or G1 character rules, whichever is active
	} current;
	
	struct
	{
		TerminalTextAttributes		drawingAttributes;	//!< previous bits of corresponding bits in "current" structure
		SInt16						cursorX;			//!< previous value of corresponding value in "current" structure
		SInt16						cursorY;			//!< previous value of corresponding value in "current" structure
	} previous;
	
	TerminalScreenRef		selfRef;					//!< opaque reference that would resolve to a pointer to this structure
};
typedef My_ScreenBuffer*			My_ScreenBufferPtr;
typedef My_ScreenBuffer const*		My_ScreenBufferConstPtr;

/*!
Manages state determination and transition for conditions
that no emulator knows how to deal with.  Also used to
handle extremely common things like echoing printable
characters.
*/
class My_DefaultEmulator
{
public:
	static UInt32	echoData			(My_ScreenBufferPtr, UInt8 const*, UInt32);
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
};

/*!
Manages state determination and transition for a terminal
that does nothing but echo human-readable versions of every
character it receives.
*/
class My_DumbTerminal
{
public:
	static UInt32	echoData			(My_ScreenBufferPtr, UInt8 const*, UInt32);
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
};

/*!
Manages state determination and transition for the VT100
terminal emulator while in ANSI mode.  The VT52 subclass
handles sequences specific to VT52 mode.
*/
class My_VT100
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
	
	static void		alignmentDisplay			(My_ScreenBufferPtr);
	static void		ansiMode					(My_ScreenBufferPtr);
	static void		cursorBackward				(My_ScreenBufferPtr);
	static void		cursorDown					(My_ScreenBufferPtr);
	static void		cursorForward				(My_ScreenBufferPtr);
	static void		cursorUp					(My_ScreenBufferPtr);
	static void		deviceAttributes			(My_ScreenBufferPtr);
	static void		deviceStatusReport			(My_ScreenBufferPtr);
	static void		eraseInDisplay				(My_ScreenBufferPtr);
	static void		eraseInLine					(My_ScreenBufferPtr);
	static void		loadLEDs					(My_ScreenBufferPtr);
	static void		modeSetReset				(My_ScreenBufferPtr, Boolean);
	static UInt32	readCSIParameters			(My_ScreenBufferPtr, UInt8 const*, UInt32);
	static void		reportTerminalParameters	(My_ScreenBufferPtr);
	static void		setTopAndBottomMargins		(My_ScreenBufferPtr);
	static void		vt52Mode					(My_ScreenBufferPtr);
	
	class VT52
	{
		friend class My_VT100;
		
	public:
		static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
		static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
		
		static void		cursorBackward		(My_ScreenBufferPtr);
		static void		cursorDown			(My_ScreenBufferPtr);
		static void		cursorForward		(My_ScreenBufferPtr);
		static void		cursorUp			(My_ScreenBufferPtr);
		static void		identify			(My_ScreenBufferPtr);
	
	protected:
		// The names of these constants use the same mnemonics from
		// the programming manual of the original terminal.
		enum State
		{
			// VT100 sequences in VT52 compatibility mode (in the order they appear in the manual) - see VT100 manual for full details
			kStateCU		= kMy_ParserStateSeenESCA,				//!< cursor up
			kStateCD		= kMy_ParserStateSeenESCB,				//!< cursor down
			kStateCR 		= kMy_ParserStateSeenESCC,				//!< cursor right
			kStateCL 		= kMy_ParserStateSeenESCD,				//!< cursor left
			kStateNGM		= kMy_ParserStateSeenESCF,				//!< enter graphics mode
			kStateXGM		= kMy_ParserStateSeenESCG,				//!< exit graphics mode
			kStateCH		= kMy_ParserStateSeenESCH,				//!< cursor home
			kStateRLF		= kMy_ParserStateSeenESCI,				//!< reverse line feed
			kStateEES		= kMy_ParserStateSeenESCJ,				//!< erase to end of screen
			kStateEEL		= kMy_ParserStateSeenESCK,				//!< erase to end of line
			kStateDCA		= kMy_ParserStateSeenESCY,				//!< direct cursor address
			kStateDCAY		= 'DCAY',								//!< direct cursor address, seen first follow-up character (Y)
			kStateDCAX		= 'DCAX',								//!< direct cursor address, seen second follow-up character (X)
			kStateID		= kMy_ParserStateSeenESCZ,				//!< identify terminal
			kStateNAKM		= kMy_ParserStateSeenESCEquals,			//!< enter alternate keypad mode
			kStateXAKM		= kMy_ParserStateSeenESCGreaterThan,	//!< exit alternate keypad mode
			kStateANSI		= kMy_ParserStateSeenESCLessThan,		//!< enter ANSI mode
		};
	};
	
	// The names of these constants use the same mnemonics from
	// the programming manual of the original terminal.
	enum State
	{
		// VT100 immediates (in order of the corresponding control character’s ASCII code)
		kStateControlENQ		= 'VAns',				//!< transmit answerback message
		kStateControlBEL		= 'VBel',				//!< audio event
		kStateControlBS			= 'VBks',				//!< move cursor left if possible
		kStateControlHT			= 'VTab',				//!< move cursor right to tab stop, or margin
		kStateControlLFVTFF		= 'VLnF',				//!< newline or line feed, depending on LNM
		kStateControlCR			= 'VCRt',				//!< move cursor down and to left margin
		kStateControlSO			= 'VShO',				//!< shift out to G1 character set
		kStateControlSI			= 'VShI',				//!< shift in to G0 character set
		kStateControlXON		= 'VXON',				//!< resume transmission
		kStateControlXOFF		= 'VXOF',				//!< suspend transmission
		kStateControlCANSUB		= 'VCAN',				//!< terminate control seq. with error char.
		
		// VT100 sequences (in the order they appear in the manual) - see VT100 manual for full details
		kStateCSI				= kMy_ParserStateSeenESCLeftSqBracket,	//!< control sequence inducer
		kStateCSIParamScan		= kMy_ParserStateSeenESCLeftSqBracketParams,	//!< state of accumulating parameters
		kStateCUB				= kMy_ParserStateSeenESCLeftSqBracketParamsD,	//!< cursor backward
		kStateCUD				= kMy_ParserStateSeenESCLeftSqBracketParamsB,	//!< cursor down
		kStateCUF				= kMy_ParserStateSeenESCLeftSqBracketParamsC,	//!< cursor forward
		kStateCUP				= kMy_ParserStateSeenESCLeftSqBracketParamsH,	//!< cursor position
		kStateCUU				= kMy_ParserStateSeenESCLeftSqBracketParamsA,	//!< cursor up
		kStateDA				= kMy_ParserStateSeenESCLeftSqBracketParamsc,	//!< device attributes
		kStateDECALN			= kMy_ParserStateSeenESCPound8,			//!< screen alignment display
		kStateDECANM			= 'VANM',				//!< ANSI/VT52 mode
		kStateDECARM			= 'VARM',				//!< auto repeat mode
		kStateDECAWM			= 'VAWM',				//!< auto wrap mode
		kStateDECCKM			= 'VCKM',				//!< cursor keys mode
		kStateDECCOLM			= 'VCLM',				//!< column mode
		kStateDECDHLT			= kMy_ParserStateSeenESCPound3,			//!< double height line, top half
		kStateDECDHLB			= kMy_ParserStateSeenESCPound4,			//!< double height line, bottom half
		kStateDECDWL			= kMy_ParserStateSeenESCPound6,			//!< double width line
		kStateDECID				= kMy_ParserStateSeenESCZ,				//!< identify terminal
		kStateDECKPAM			= kMy_ParserStateSeenESCEquals,			//!< keypad application mode
		kStateDECKPNM			= kMy_ParserStateSeenESCGreaterThan,	//!< keypad numeric mode
		kStateDECLL				= kMy_ParserStateSeenESCLeftSqBracketParamsq,	//!< load LEDs (keyboard lights)
		kStateDECOM				= 'VOM ',				//!< origin mode
		kStateDECRC				= kMy_ParserStateSeenESC8,				//!< restore cursor
		kStateDECREPTPARM		= 'VRPM',				//!< report terminal parameters
		kStateDECREQTPARM		= kMy_ParserStateSeenESCLeftSqBracketParamsx,	//!< request terminal parameters
		kStateDECSC				= kMy_ParserStateSeenESC7,				//!< save cursor
		kStateDECSTBM			= kMy_ParserStateSeenESCLeftSqBracketParamsr,	//!< set top and bottom margins
		kStateDECSWL			= kMy_ParserStateSeenESCPound5,			//!< single width line
		kStateDECTST			= 'VTST',				//!< invoke confidence test
		kStateDSR				= kMy_ParserStateSeenESCLeftSqBracketParamsn,	//!< device status report
		kStateED 				= kMy_ParserStateSeenESCLeftSqBracketParamsJ,	//!< erase in display
		kStateEL 				= kMy_ParserStateSeenESCLeftSqBracketParamsK,	//!< erase in line
		kStateHTS				= kMy_ParserStateSeenESCH,				//!< horizontal tabulation set
		kStateHVP				= kMy_ParserStateSeenESCLeftSqBracketParamsf,	//!< horizontal and vertical position
		kStateIND				= kMy_ParserStateSeenESCD,				//!< index
		kStateNEL				= kMy_ParserStateSeenESCE,				//!< next line
		kStateRI				= kMy_ParserStateSeenESCM,				//!< reverse index
		kStateRIS				= kMy_ParserStateSeenESCc,				//!< reset to initial state
		kStateRM				= kMy_ParserStateSeenESCLeftSqBracketParamsl,	//!< reset mode
		kStateSCSG0UK			= kMy_ParserStateSeenESCLeftParenA,		//!< select character set for G0, U.K.
		kStateSCSG0ASCII		= kMy_ParserStateSeenESCLeftParenB,		//!< select character set for G0, ASCII
		kStateSCSG0SG			= kMy_ParserStateSeenESCLeftParen0,		//!< select character set for G0, special graphics
		kStateSCSG0ACRStd		= kMy_ParserStateSeenESCLeftParen1,		//!< select character set for G0, alternate character ROM standard set
		kStateSCSG0ACRSG		= kMy_ParserStateSeenESCLeftParen2,		//!< select character set for G0, alternate character ROM special graphics
		kStateSCSG1UK			= kMy_ParserStateSeenESCRightParenA,	//!< select character set for G0, U.K.
		kStateSCSG1ASCII		= kMy_ParserStateSeenESCRightParenB,	//!< select character set for G0, ASCII
		kStateSCSG1SG			= kMy_ParserStateSeenESCRightParen0,	//!< select character set for G0, special graphics
		kStateSCSG1ACRStd		= kMy_ParserStateSeenESCRightParen1,	//!< select character set for G0, alternate character ROM standard set
		kStateSCSG1ACRSG		= kMy_ParserStateSeenESCRightParen2,	//!< select character set for G0, alternate character ROM special graphics
		kStateSGR				= kMy_ParserStateSeenESCLeftSqBracketParamsm,	//!< select graphic rendition
		kStateSM				= kMy_ParserStateSeenESCLeftSqBracketParamsh,	//!< set mode
		kStateTBC				= kMy_ParserStateSeenESCLeftSqBracketParamsg,	//!< tabulation clear
		
		// ANSI hacks that should be in their own emulator, but for now are not
		kStateANSISC			= kMy_ParserStateSeenESCLeftSqBracketParamss,	//!< save cursor
		kStateANSIRC			= kMy_ParserStateSeenESCLeftSqBracketParamsu,	//!< restore cursor
	};
};

/*!
Manages state determination and transition for the VT102
terminal emulator.
*/
class My_VT102:
public My_VT100
{
public:
	static void		deleteCharacters	(My_ScreenBufferPtr);
	static void		deleteLines			(My_ScreenBufferPtr);
	static void		insertLines			(My_ScreenBufferPtr);
	static void		loadLEDs			(My_ScreenBufferPtr);
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);

protected:
	// The names of these constants use the same mnemonics from
	// the programming manual of the original terminal.
	enum State
	{
		kStateDCH		= kMy_ParserStateSeenESCLeftSqBracketParamsP,	//!< delete characters on the cursor line
		kStateDL		= kMy_ParserStateSeenESCLeftSqBracketParamsM,	//!< delete lines, including cursor line
		kStateIL		= kMy_ParserStateSeenESCLeftSqBracketParamsL,	//!< insert lines below cursor line
		kStateMC		= kMy_ParserStateSeenESCLeftSqBracketParamsi,	//!< media copy (printer access)
	};
};

/*!
Manages state determination and transition for the VT220
terminal emulator.
*/
class My_VT220
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
	
	static void		deviceAttributes	(My_ScreenBufferPtr);
};

/*!
Manages state determination and transition for the XTerm
terminal emulator.
*/
class My_XTerm
{
public:
	static UInt32	stateDeterminant	(My_EmulatorPtr, UInt8 const*, UInt32, My_ParserStatePair&, Boolean&, Boolean&);
	static UInt32	stateTransition		(My_ScreenBufferPtr, UInt8 const*, UInt32, My_ParserStatePair const&, Boolean&);
	
	static void		cursorBackwardTabulation		(My_ScreenBufferPtr);
	static void		cursorCharacterAbsolute			(My_ScreenBufferPtr);
	static void		cursorForwardTabulation			(My_ScreenBufferPtr);
	static void		cursorNextLine					(My_ScreenBufferPtr);
	static void		cursorPreviousLine				(My_ScreenBufferPtr);
	static void		horizontalPositionAbsolute		(My_ScreenBufferPtr);
	static void		insertBlankCharacters			(My_ScreenBufferPtr);
	static void		scrollDown						(My_ScreenBufferPtr);
	static void		scrollUp						(My_ScreenBufferPtr);
	static void		verticalPositionAbsolute		(My_ScreenBufferPtr);
	
	enum State
	{
		// Ideally these are "protected", but loop evasion code requires them.
		kStateCBT				= kMy_ParserStateSeenESCLeftSqBracketParamsZ,			//!< cursor backward tabulation
		kStateCHA				= kMy_ParserStateSeenESCLeftSqBracketParamsG,			//!< cursor character absolute
		kStateCHT				= kMy_ParserStateSeenESCLeftSqBracketParamsI,			//!< cursor forward tabulation
		kStateCNL				= kMy_ParserStateSeenESCLeftSqBracketParamsE,			//!< cursor next line
		kStateCPL				= kMy_ParserStateSeenESCLeftSqBracketParamsF,			//!< cursor previous line
		kStateHPA				= kMy_ParserStateSeenESCLeftSqBracketParamsBackquote,	//!< horizontal (character) position absolute
		kStateICH				= kMy_ParserStateSeenESCLeftSqBracketParamsAt,			//!< insert blank characters
		kStateSD				= kMy_ParserStateSeenESCLeftSqBracketParamsT,			//!< scroll down
		kStateSU				= kMy_ParserStateSeenESCLeftSqBracketParamsS,			//!< scroll up
		kStateVPA				= kMy_ParserStateSeenESCLeftSqBracketParamsd,			//!< vertical position absolute
		kStateSWIT				= kMy_ParserStateSeenESCRightSqBracket0,				//!< subsequent string is for window title and icon title
		kStateSWITAcquireStr	= kMy_ParserStateSeenESCRightSqBracket0Semi,			//!< seen ESC]0, gathering characters of string
		kStateSIT				= kMy_ParserStateSeenESCRightSqBracket1,				//!< subsequent string is for icon title only
		kStateSITAcquireStr		= kMy_ParserStateSeenESCRightSqBracket1Semi,			//!< seen ESC]1, gathering characters of string
		kStateSWT				= kMy_ParserStateSeenESCRightSqBracket2,				//!< subsequent string is for window title only
		kStateSWTAcquireStr		= kMy_ParserStateSeenESCRightSqBracket2Semi,			//!< seen ESC]2, gathering characters of string
		kStateSetColor			= kMy_ParserStateSeenESCRightSqBracket4,				//!< subsequent string is a color specification
		kStateColorAcquireStr	= kMy_ParserStateSeenESCRightSqBracket4Semi,			//!< seen ESC]4, gathering characters of string
		kStateStringTerminator	= kMy_ParserStateSeenESCBackslash,						//!< perform action according to accumulated string
	};
};

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void						addScreenLineLength						(My_ScreenBufferPtr, CFMutableStringRef, UInt16, void*);
void						appendScreenLinePtrToList				(My_ScreenBufferPtr, CFMutableStringRef, UInt16, void*);
void						appendScreenLineRawToCFString			(My_ScreenBufferPtr, CFMutableStringRef, UInt16, void*);
void						assertScrollingRegion					(My_ScreenBufferPtr);
void						bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr);
void						bufferEraseFromCursorToEnd				(My_ScreenBufferPtr);
void						bufferEraseFromHomeToCursor				(My_ScreenBufferPtr);
void						bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr);
void						bufferEraseLineWithoutUpdate			(My_ScreenBufferPtr, My_ScreenBufferLine&);
void						bufferEraseLineWithUpdate				(My_ScreenBufferPtr, SInt16);
void						bufferEraseVisibleScreenWithUpdate		(My_ScreenBufferPtr);
void						bufferEraseWithoutUpdate				(My_ScreenBufferPtr);
void						bufferInsertBlanksAtCursorColumnWithoutUpdate	(My_ScreenBufferPtr, SInt16);
void						bufferInsertBlankLines					(My_ScreenBufferPtr, UInt16,
																	 My_ScreenBufferLineList::iterator&,
																	 My_AttributeRule = kMy_AttributeRuleInitialize);
void						bufferLineFill							(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt8,
																	 TerminalTextAttributes =
																		kTerminalTextAttributesAllOff,
																	 Boolean = true);
void						bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr, SInt16);
void						bufferRemoveLines						(My_ScreenBufferPtr, UInt16,
																	 My_ScreenBufferLineList::iterator&,
																	 My_AttributeRule = kMy_AttributeRuleInitialize);
void						changeLineAttributes					(My_ScreenBufferPtr, My_ScreenBufferLine&,
																	 TerminalTextAttributes, TerminalTextAttributes);
void						changeLineGlobalAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&,
																	 TerminalTextAttributes, TerminalTextAttributes);
void						changeLineRangeAttributes				(My_ScreenBufferPtr, My_ScreenBufferLine&, UInt16,
																	 SInt16, TerminalTextAttributes,
																	 TerminalTextAttributes);
void						changeNotifyForTerminal					(My_ScreenBufferConstPtr, Terminal_Change, void*);
void						clearEscapeSequenceParameters			(My_ScreenBufferPtr);
void						cursorRestore							(My_ScreenBufferPtr);
void						cursorSave								(My_ScreenBufferPtr);
void						cursorWrapIfNecessaryGetLocation		(My_ScreenBufferPtr, SInt16*, My_ScreenRowIndex*);
void						echoCFString							(My_ScreenBufferPtr, CFStringRef);
void						emulatorFrontEndOld						(My_ScreenBufferPtr, UInt8 const*, SInt32);
void						eraseRightHalfOfLine					(My_ScreenBufferPtr, My_ScreenBufferLine&);
Terminal_Result				forEachLineDo							(TerminalScreenRef, Terminal_LineRef, UInt32,
																	 My_ScreenLineOperationProcPtr, void*);
void						getBufferOffsetCell						(My_ScreenBufferPtr, size_t, UInt16, UInt16&, SInt32&);
inline My_LineIteratorPtr	getLineIterator							(Terminal_LineRef);
My_ScreenBufferPtr			getVirtualScreenData					(TerminalScreenRef);
void						highlightLED							(My_ScreenBufferPtr, SInt16);
void						initializeParserStateStack				(My_EmulatorPtr);
void						locateCursorLine						(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
void						locateScrollingRegion					(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&,
																	 My_ScreenBufferLineList::iterator&);
void						locateScrollingRegionTop				(My_ScreenBufferPtr, My_ScreenBufferLineList::iterator&);
void						moveCursor								(My_ScreenBufferPtr, SInt16, My_ScreenRowIndex);
void						moveCursorDown							(My_ScreenBufferPtr);
void						moveCursorDownOrScroll					(My_ScreenBufferPtr);
void						moveCursorDownToEdge					(My_ScreenBufferPtr);
void						moveCursorLeft							(My_ScreenBufferPtr);
void						moveCursorLeftToEdge					(My_ScreenBufferPtr);
void						moveCursorLeftToHalf					(My_ScreenBufferPtr);
void						moveCursorLeftToNextTabStop				(My_ScreenBufferPtr);
void						moveCursorRight							(My_ScreenBufferPtr);
void						moveCursorRightToEdge					(My_ScreenBufferPtr);
void						moveCursorRightToNextTabStop			(My_ScreenBufferPtr);
void						moveCursorUpToEdge						(My_ScreenBufferPtr);
void						moveCursorUp							(My_ScreenBufferPtr);
void						moveCursorUpOrScroll					(My_ScreenBufferPtr);
void						moveCursorX								(My_ScreenBufferPtr, SInt16);
void						moveCursorY								(My_ScreenBufferPtr, My_ScreenRowIndex);
void						resetTerminal							(My_ScreenBufferPtr);
SessionRef					returnListeningSession					(My_ScreenBufferPtr);
Terminal_EmulatorType		returnTerminalType						(Terminal_Emulator);
Terminal_EmulatorVariant	returnTerminalVariant					(Terminal_Emulator);
Boolean						screenCopyLinesToScrollback				(My_ScreenBufferPtr);
Boolean						screenInsertNewLines					(My_ScreenBufferPtr, My_ScreenBufferLineList::size_type);
Boolean						screenMoveLinesToScrollback				(My_ScreenBufferPtr, My_ScreenBufferLineList::size_type);
void						screenScroll							(My_ScreenBufferPtr, SInt16 = 1);
void						setCursorVisible						(My_ScreenBufferPtr, Boolean);
void						setScrollbackSize						(My_ScreenBufferPtr, UInt32);
Terminal_Result				setVisibleColumnCount					(My_ScreenBufferPtr, UInt16);
Terminal_Result				setVisibleRowCount						(My_ScreenBufferPtr, UInt16);
// IMPORTANT: Attribute bit manipulation is fully described in "TerminalTextAttributes.typedef.h".
//            Changes must be kept consistent everywhere.  See below, for usage.
inline TerminalTextAttributes	styleOfVTParameter					(UInt8	inPs)
{
	return (1 << (inPs - 1));
}
void						tabStopClearAll							(My_ScreenBufferPtr);
UInt16						tabStopGetDistanceFromCursor			(My_ScreenBufferConstPtr, Boolean);
void						tabStopInitialize						(My_ScreenBufferPtr);
UniChar						translateCharacter						(My_ScreenBufferPtr, UniChar, TerminalTextAttributes,
																	 TerminalTextAttributes&);
template < typename src_char_seq_const_iter, typename src_char_seq_size_t,
			typename dest_char_seq_iter, typename dest_char_seq_size_t >
dest_char_seq_iter			whitespaceSensitiveCopy					(src_char_seq_const_iter, src_char_seq_size_t,
																	 dest_char_seq_iter, dest_char_seq_size_t,
																	 dest_char_seq_size_t*, src_char_seq_size_t);

} // anonymous namespace

#pragma mark Variables
namespace {

My_PrintableByUniChar&		gDumbTerminalRenderings ()	{ static My_PrintableByUniChar x; return x; }
My_ScreenBufferLine&		gEmptyScreenBufferLine ()	{ static My_ScreenBufferLine x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new terminal screen and initial view according
to the given specifications.

MacTelnet 3.0 is designed so that a terminal screen buffer
can have multiple views, or vice-versa (a view can display
the contents of multiple buffers).  Therefore, although a
single buffer and view is created by this routine, more
could be constructed later.

\retval kTerminal_ResultOK
if the screen was created successfully

\retval kTerminal_ResultParameterError
if any of the given storage pointers are nullptr

\retval kTerminal_ResultNotEnoughMemory
if there is a serious problem creating the screen

(3.0)
*/
Terminal_Result
Terminal_NewScreen	(Preferences_ContextRef		inTerminalConfig,
					 Preferences_ContextRef		inTranslationConfig,
					 TerminalScreenRef*			outScreenPtr)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (outScreenPtr == nullptr) result = kTerminal_ResultParameterError;
	else
	{
		try
		{
			*outScreenPtr = REINTERPRET_CAST(new My_ScreenBuffer(inTerminalConfig, inTranslationConfig), TerminalScreenRef);
		}
		catch (std::bad_alloc)
		{
			*outScreenPtr = nullptr;
		}
		catch (Terminal_Result		inConstructionError)
		{
			result = inConstructionError;
		}
	}
	return result;
}// NewScreen


/*!
Disposes of a screen previously created with
Terminal_NewScreen().  On return, the specified
screen ID is invalid.

For legacy reasons, returns nonzero if there is
an error while disposing of the screen.  In
reality, this only happens if you pass in a
nullptr reference.

(2.6)
*/
SInt16
Terminal_DisposeScreen	(TerminalScreenRef		inRef)
{
	SInt16		result = 0;
	
	
	if (inRef == nullptr) result = -3/* legacy return value from NCSA Telnet; can probably ditch this eventually */;
	else
	{
		My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
		
		
		// dispose screen lines - UNIMPLEMENTED!!!
		delete ptr;
	}
	return result;
}// DisposeScreen


/*!
Creates a special reference to the given line of the given
terminal screen’s VISIBLE SCREEN buffer, or returns nullptr
if the line is out of range or the iterator cannot be created
for some other reason.

Pass 0 to indicate you want the very topmost line (that is,
the one that would be added to the scrollback buffer first if
the screen scrolls), or larger values to ask for lines below
it.  Currently, this routine takes linearly more time to
figure out where the bottommost lines are.

A main line iterator can be advanced using negative numbers
to go backwards, and if this is done often enough, it will
automatically enter the scrollback buffer (as if the iterator
were constructed with Terminal_NewScrollbackLineIterator()).

IMPORTANT:	An iterator is completely invalid once the
			screen it was created for has been destroyed.

(3.0)
*/
Terminal_LineRef
Terminal_NewMainScreenLineIterator	(TerminalScreenRef		inRef,
									 UInt16					inLineNumberZeroForTop)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if (nullptr != ptr)
	{
		// ensure the specified row is in range
		if (inLineNumberZeroForTop < ptr->screenBuffer.size())
		{
			try
			{
				My_ScreenBufferLineList::iterator	startIterator = ptr->screenBuffer.begin();
				My_LineIteratorPtr					iteratorPtr = nullptr;
				
				
				std::advance(startIterator, STATIC_CAST(inLineNumberZeroForTop, My_ScreenBufferLineList::difference_type));
				iteratorPtr = new My_LineIterator(ptr->screenBuffer, ptr->scrollbackBuffer,
													ptr->screenBuffer/* iterator target */, startIterator);
				if (nullptr != iteratorPtr)
				{
					result = REINTERPRET_CAST(iteratorPtr, Terminal_LineRef);
				}
			}
			catch (std::bad_alloc)
			{
				result = nullptr;
			}
		}
	}
	return result;
}// NewMainScreenLineIterator


/*!
Creates a special reference to the given line of the given
terminal screen’s SCROLLBACK buffer, or returns nullptr if
the line is out of range or the iterator cannot be created
for some other reason.

Pass 0 to indicate you want the very newest line (that is,
the one that most recently scrolled off the top of the main
screen), or larger values to ask for older lines.  Currently,
this routine takes linearly more time to figure out where
old scrollback lines are.

A scrollback line iterator can be advanced often enough to
automatically enter the main screen buffer (as if the iterator
were constructed with NewMainScreenLineIterator()).

IMPORTANT:	An iterator is completely invalid once the screen
			it was created for has been destroyed.  It can
			also be invalidated in other ways, e.g. a call to
			Terminal_DeleteAllSavedLines().

(3.0)
*/
Terminal_LineRef
Terminal_NewScrollbackLineIterator	(TerminalScreenRef	inRef,
									 UInt16				inLineNumberZeroForNewest)
{
	Terminal_LineRef		result = nullptr;
	My_ScreenBufferPtr		ptr = getVirtualScreenData(inRef);
	
	
	if ((nullptr != ptr) && (ptr->scrollbackBuffer.begin() != ptr->scrollbackBuffer.end()))
	{
		try
		{
			My_ScreenBufferLineList::iterator	startIterator = ptr->scrollbackBuffer.begin();
			My_LineIteratorPtr					iteratorPtr = nullptr;
			Boolean								validIterator = true;
			
			
			// ensure the specified row is in range; since the scrollback buffer is
			// a linked list, it would be prohibitively expensive to ask for its size,
			// so instead the iterator is incremented while watching for the end
			for (My_ScreenBufferLineList::difference_type i = 0; i < inLineNumberZeroForNewest; ++i)
			{
				++startIterator;
				if (startIterator == ptr->scrollbackBuffer.end())
				{
					validIterator = false;
					break;
				}
			}
			
			if (validIterator)
			{
				iteratorPtr = new My_LineIterator(ptr->screenBuffer, ptr->scrollbackBuffer,
													ptr->scrollbackBuffer/* iterator target */, startIterator);
				if (nullptr != iteratorPtr)
				{
					result = REINTERPRET_CAST(iteratorPtr, Terminal_LineRef);
				}
			}
		}
		catch (std::bad_alloc)
		{
			result = nullptr;
		}
	}
	return result;
}// NewScrollbackLineIterator


/*!
Destroys an iterator created with Terminal_NewLineIterator(),
and sets your copy of the reference to nullptr.

(3.0)
*/
void
Terminal_DisposeLineIterator	(Terminal_LineRef*		inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		My_LineIteratorPtr		ptr = getLineIterator(*inoutRefPtr);
		
		
		if (nullptr != ptr)
		{
			delete ptr;
			*inoutRefPtr = nullptr;
		}
	}
}// DisposeLineIterator


/*!
Returns "true" only if the given terminal’s bell is
active.  An inactive bell completely ignores all
bell signals - without giving any audible or visible
indication that a bell has occurred.  This can be
useful if you know you’ve just triggered a long string
of bells and don’t want to be annoyed by a series of
beeps or flashes.

(3.0)
*/
Boolean
Terminal_BellIsEnabled		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = !dataPtr->bellDisabled;
	}
	return result;
}// BellIsEnabled


/*!
Applies the specified changes to every single attribute
for a single line of the screen buffer (no effect on
the display until the next redraw). Also updates the
“current” attributes, if the cursor happens to be on
the specified line.

The row value may be negative, signifying a scrollback
buffer row.

Equivalent to invoking Terminal_ChangeLineRangeAttributes()
with a range from first column to last column.

You can use this to apply effects intended to be global
to a line, like double-sized text.

\retval kTerminal_ResultOK
if the line attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeLineAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inRow,
								 TerminalTextAttributes		inSetTheseAttributes,
								 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		changeLineAttributes(dataPtr, iteratorPtr->currentLine(), inSetTheseAttributes, inClearTheseAttributes);
	}
	
	return result;
}// ChangeLineAttributes


/*!
Applies the specified changes to every single attribute
in the specified column range of the specified line
screen buffer (no effect on the display until the next
redraw).  Also updates the “current” attributes, if the
cursor happens to be in the specified range.

The row value may be negative, signifying a scrollback
buffer row.

Equivalent to invoking Terminal_ChangeRangeAttributes()
with the same start and end rows.

You can use this to apply effects intended to be applied
to a series of characters on a line, like text coloring.

\retval kTerminal_ResultOK
if the line range attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeLineRangeAttributes	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inRow,
									 UInt16						inZeroBasedStartColumn,
									 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
									 TerminalTextAttributes		inSetTheseAttributes,
									 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn,
									inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
									inSetTheseAttributes, inClearTheseAttributes);
	}
	
	return result;
}// ChangeLineRangeAttributes


/*!
Applies the specified changes to every single attribute
within the specified range of the screen buffer (no
effect on the display until the next redraw).  Also
updates the “current” attributes, if the cursor happens
to be in the specified range.

The anchor points are automatically “sorted”, so despite
their names it does not matter which point really marks
the start or end of the range.

The row values may be negative, signifying that they
apply to scrollback buffer rows.

If "inConstrainToRectangle" is true, the range is
considered to be exact so no columns outside the range
are filled in (normally, all lines in between the start
and end points are filled completely, from column 0 to
the last column).

You can use this to apply effects to a large number of
characters at once (e.g. when highlighting).

\retval kTerminal_ResultOK
if the range attributes were changed successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.0)
*/
Terminal_Result
Terminal_ChangeRangeAttributes	(TerminalScreenRef			inRef,
								 Terminal_LineRef			inStartRow,
								 UInt16						inNumberOfRowsToConsider,
								 UInt16						inZeroBasedStartColumn,
								 UInt16						inZeroBasedPastTheEndColumn,
								 Boolean					inConstrainToRectangle,
								 TerminalTextAttributes		inSetTheseAttributes,
								 TerminalTextAttributes		inClearTheseAttributes)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	// force anchors to be within valid screen boundaries
	if (inZeroBasedStartColumn >= dataPtr->text.visibleScreen.numberOfColumnsPermitted)
	{
		inZeroBasedStartColumn = dataPtr->text.visibleScreen.numberOfColumnsPermitted - 1;
	}
	if (inZeroBasedPastTheEndColumn > dataPtr->text.visibleScreen.numberOfColumnsPermitted)
	{
		inZeroBasedPastTheEndColumn = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
	}
	if (inZeroBasedPastTheEndColumn < 1)
	{
		inZeroBasedPastTheEndColumn = 1;
	}
	
	if (inNumberOfRowsToConsider > 0)
	{
		// now apply changes to attributes of all specified cells, and to
		// the “current” attributes (if the cursor is anywhere in the range)
		if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
		else if (nullptr == iteratorPtr) result = kTerminal_ResultInvalidIterator;
		else
		{
			Boolean const		kSingleLineRange = (1 == inNumberOfRowsToConsider);
			
			
			// figure out how to treat the first line
			if (kSingleLineRange)
			{
				// same line - iterate only over certain columns
				changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn, inZeroBasedPastTheEndColumn,
											inSetTheseAttributes, inClearTheseAttributes);
			}
			else
			{
				// multi-line range; start by creating some convenient anchors that
				// are set correctly based on the rectangular/non-rectangular mode
				UInt16		lineStartColumn = (inConstrainToRectangle)
												? inZeroBasedStartColumn
												: 0;
				SInt16		linePastTheEndColumn = (inConstrainToRectangle)
													? inZeroBasedPastTheEndColumn
													: dataPtr->text.visibleScreen.numberOfColumnsPermitted;
				Boolean		isEnd = false;
				
				
				// fill in first line
				changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), inZeroBasedStartColumn, linePastTheEndColumn,
											inSetTheseAttributes, inClearTheseAttributes);
				iteratorPtr->goToNextLine(isEnd);
				
				if (false == isEnd)
				{
					// fill in remaining lines; the last line is special
					// because it will end “early” at the end anchor
					{
						register SInt16		i = 0;
						SInt16 const		kLineEnd = (inNumberOfRowsToConsider - 1);
						
						
						for (i = 0; i < kLineEnd; ++i, iteratorPtr->goToNextLine(isEnd))
						{
							if (isEnd)
							{
								Console_Warning(Console_WriteLine, "exceeded row boundaries when changing attributes of a range");
								break;
							}
							
							// intermediate lines occupy the whole range; the last line
							// only occupies up to the final anchor point
							if (i < (kLineEnd - 1))
							{
								// fill in intermediate line
								changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), lineStartColumn, linePastTheEndColumn,
															inSetTheseAttributes, inClearTheseAttributes);
							}
							else
							{
								// fill in last line
								changeLineRangeAttributes(dataPtr, iteratorPtr->currentLine(), lineStartColumn, inZeroBasedPastTheEndColumn,
															inSetTheseAttributes, inClearTheseAttributes);
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}// ChangeRangeAttributes


/*!
Copies a portion of a single line of text not exceeding the
current visible width.  Trailing whitespace is skipped.

DEPRECATED.  Use Terminal_GetLineRangePossibleCopy().

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultParameterError
if the given line number is too large or the given column
numbers are out of range

\retval kTerminal_ResultNotEnoughMemory
if the specified buffer is too small; a truncated version of
the text will be copied into the buffer

(3.0)
*/
Terminal_Result
Terminal_CopyLineRange	(TerminalScreenRef		inScreen,
						 Terminal_LineRef		inRow,
						 UInt16					inZeroBasedStartColumn,
						 SInt16					inZeroBasedEndColumnOrNegativeForLastColumn,
						 char*					outBuffer,
						 SInt32					inBufferLength,
						 SInt32*				outActualLengthPtrOrNull,
						 UInt16					inNumberOfSpacesPerTabOrZeroForNoSubstitution)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		CFIndex const	kTextVectorSize = CFStringGetLength(iteratorPtr->currentLine().textCFString.returnCFMutableStringRef());
		UInt16			endColumn = (inZeroBasedEndColumnOrNegativeForLastColumn < 0)
										? kTextVectorSize - 1
										: inZeroBasedEndColumnOrNegativeForLastColumn;
		
		
		if (endColumn >= kTextVectorSize)
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			My_TextIterator		textStartIter = nullptr;
			SInt32				copyLength = INTEGER_MINIMUM(inBufferLength, endColumn - inZeroBasedStartColumn + 1);
			
			
			textStartIter = iteratorPtr->currentLine().textVectorBegin;
			std::advance(textStartIter, inZeroBasedStartColumn);
			(char*)whitespaceSensitiveCopy(textStartIter, copyLength, outBuffer, copyLength,
											&copyLength/* returned actual length */,
											STATIC_CAST(inNumberOfSpacesPerTabOrZeroForNoSubstitution, SInt32));
			if (outActualLengthPtrOrNull != nullptr) *outActualLengthPtrOrNull = copyLength;
		}
	}
	
	return result;
}// CopyLineRange


/*!
Copies a portion of text from the specified screen into the
given buffer - excluding trailing whitespace on each line.

DEPRECATED.  See Terminal_GetLine() and Terminal_GetLineRange(),
which are relatively efficient routines that also return
ranges of Unicode characters.

IMPORTANT:	This concept is under evaluation.  Probably,
			most interactions with terminal data are not
			necessary or, at a minimum, do not require
			copying data - in the future MacTelnet will
			attempt to avoid duplicating large text blocks.

The end-of-line sequence is inserted at line boundaries.
This is currently assumed to be exactly ONE character long.

NOTE:	You can set the first character of the sequence to
		0, to specify that no character should be used to
		separate lines.  You might do this if you want to
		copy multiple lines of terminal text that “really
		are one line”.

In MacTelnet 3.0, this method now copies text differently
if the coordinates are to define a rectangular selection.
That is, while normal Macintosh text selection includes all
text in the rows between the start and end anchors, a text
selection that is “rectangular” will only include text in
the in-between rows that are part of the columns between
the columns of the two anchor points.

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultNotEnoughMemory
if the specified buffer is too small; a truncated version
of the text will be copied into the buffer

(3.0)
*/
Terminal_Result
Terminal_CopyRange	(TerminalScreenRef			inScreen,
					 Terminal_LineRef			inStartRow,
					 UInt16						inNumberOfRowsToConsider,
					 UInt16						inZeroBasedStartColumnOnFirstRow,
					 UInt16						inZeroBasedEndColumnOnLastRow,
					 char*						outBuffer,
					 SInt32						inBufferLength,
					 SInt32*					outActualLengthPtrOrNull,
					 char const*				inEndOfLineSequence,
					 SInt16						inNumberOfSpacesPerTabOrZeroForNoSubstitution,
					 Terminal_TextCopyFlags		inFlags)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else if (iteratorPtr == nullptr) result = kTerminal_ResultInvalidIterator;
	else
	{
		char const* const	kOutBufferPastTheEndPtr = outBuffer + inBufferLength;
		SInt32				copiedLength = 0L;
		
		
		// copy text; the exact strategy depends on the range - if
		// a range is fairly small, it may be possible to copy the
		// data more efficiently
		if (1 == inNumberOfRowsToConsider)
		{
			// same line - very simple, just copy a few characters
			result = Terminal_CopyLineRange(inScreen, inStartRow, inZeroBasedStartColumnOnFirstRow, inZeroBasedEndColumnOnLastRow,
											outBuffer, inBufferLength, &copiedLength, inNumberOfSpacesPerTabOrZeroForNoSubstitution);
		}
		else
		{
			// there are multiple lines of text
			UInt16 const	kEOLLength = CPP_STD::strlen(inEndOfLineSequence);
			Boolean			isEnd = false;
			UInt16			lineCount = 0;
			SInt16			currentX = 0;
			SInt16			actualLineLength = 0L;
			char*			destPtr = outBuffer;
			
			
			for (; ((lineCount < inNumberOfRowsToConsider) && (false == isEnd)); iteratorPtr->goToNextLine(isEnd), ++lineCount)
			{
				char const* const	kDestLineStartPtr = destPtr; // pointer into destination buffer for start of current line
				char const*			destLinePastTheEndPtr = nullptr;
				SInt16				lineLength = 0;
				
				
				// set up the extreme values
				if ((inFlags & kTerminal_TextCopyFlagsRectangular) == 0)
				{
					// non-rectangular mode; the first and last lines are irregularly-shaped, but
					// the remainder is rectangular, flushed to the left and right screen edges
					if (0 == lineCount)
					{
						// first row in range
						currentX = inZeroBasedStartColumnOnFirstRow;
						lineLength = dataPtr->text.visibleScreen.numberOfColumnsPermitted - inZeroBasedStartColumnOnFirstRow;
					}
					else if ((lineCount == (inNumberOfRowsToConsider - 1)) || (&(iteratorPtr->currentLine()) == &(iteratorPtr->lastLine())))
					{
						// last row in range
						currentX = 0;
						lineLength = inZeroBasedEndColumnOnLastRow + 1;
					}
					else
					{
						// middle row
						currentX = 0;
						lineLength = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
					}
				}
				else
				{
					// rectangular mode; all lines are uniform, and the left and right boundaries
					// exactly match those of the anchor points, not flushing to the screen edges
					currentX = inZeroBasedStartColumnOnFirstRow;
					lineLength = INTEGER_ABSOLUTE(inZeroBasedEndColumnOnLastRow - inZeroBasedStartColumnOnFirstRow) + 1;
				}
				destLinePastTheEndPtr = destPtr + lineLength;
				
				// constrain to boundaries of output buffer, in case there
				// really isn’t enough room for all the lines in the range
				if (destLinePastTheEndPtr > kOutBufferPastTheEndPtr)
				{
					destLinePastTheEndPtr = kOutBufferPastTheEndPtr;
				}
				
				// copy another line into the output buffer, replacing
				// spaces with tabs as needed, omitting trailing whitespace
				{
					My_TextIterator		textStartIter = nullptr;
					
					
					textStartIter = iteratorPtr->currentLine().textVectorBegin;
					std::advance(textStartIter, currentX);
					destPtr = whitespaceSensitiveCopy(textStartIter, lineLength/* source length */,
														destPtr/* destination start */,
														STATIC_CAST(destLinePastTheEndPtr - kDestLineStartPtr, SInt16)/* destination length */,
														&actualLineLength,
														inNumberOfSpacesPerTabOrZeroForNoSubstitution/* number of spaces that equal one tab */);
				}
				
				// if necessary, add the end-of-line sequence; skip it for the last line, and always
				// skip it for rectangular selections; for regular selections, unless told otherwise
				// skip it for any line where the copy range includes the right margin and the right
				// margin character is non-whitespace (this is an XTerm-style way of allowing long
				// pathnames to join when they are broken at the right margin)
				if ((kEOLLength) && (&(iteratorPtr->currentLine()) != &(iteratorPtr->lastLine())))
				{
					if ((inFlags & kTerminal_TextCopyFlagsAlwaysNewLineAtRightMargin)/* force new-line? */ ||
						(inFlags & kTerminal_TextCopyFlagsRectangular)/* rectangular selections always have new-lines */ ||
						((currentX + actualLineLength) < dataPtr->text.visibleScreen.numberOfColumnsPermitted/* not at edge? */) ||
						CPP_STD::isspace(*destPtr)/* new-line would always follow a line ending in spaces */)
					{
						*destPtr++ = *inEndOfLineSequence; // assumes it’s only one character!
					}
				}
			}
			copiedLength = (destPtr - outBuffer);
		}
		
		if (outActualLengthPtrOrNull != nullptr) *outActualLengthPtrOrNull = copiedLength;
	}
	return result;
}// CopyRange


/*!
Returns the title assigned to the iconified version of this
terminal.  In MacTelnet this is symbolic, as no assumption is
made that a terminal corresponds to a single minimized window;
the exact usage of the “icon title” is not decided by this
module.

IMPORTANT:	You must eventually use CFRelease() on the returned
			title string.

(3.0)
*/
void
Terminal_CopyTitleForIcon	(TerminalScreenRef	inRef,
							 CFStringRef&		outTitle)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	outTitle = dataPtr->iconTitleCFString.returnCFStringRef();
	if (outTitle != nullptr)
	{
		CFRetain(outTitle);
	}
}// CopyTitleForIcon


/*!
Returns the title assigned to the window of this terminal.  In
MacTelnet this is symbolic, as no assumption is made that a
terminal corresponds to a single window; the exact usage of the
“window title” is not decided by this module.

IMPORTANT:	You must eventually use CFRelease() on the returned
			title string.

(3.0)
*/
void
Terminal_CopyTitleForWindow		(TerminalScreenRef	inRef,
								 CFStringRef&		outTitle)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	outTitle = dataPtr->windowTitleCFString.returnCFStringRef();
	if (outTitle != nullptr)
	{
		CFRetain(outTitle);
	}
}// CopyTitleForWindow


/*!
Creates a data descriptor or object specifier that describes
the given range of text in the given terminal screen.

The ranges are zero-based, but can be negative; 0 is the
first line of the main screen area, and -1 is the first
scrollback line.  The oldest scrollback row is a larger
negative number, the bottommost line of the visible screen
is one less than the number of rows high that the terminal
screen is.

IMPORTANT:	Currently this performs text duplication, when in
			fact a smarter approach would probably be to “copy
			on write” and otherwise return a light-weight
			reference to the required data.

(3.0)
*/
OSStatus
Terminal_CreateContentsAEDesc	(TerminalScreenRef		inRef,
								 Terminal_LineRef		inStartRow,
								 UInt16					inNumberOfRowsToConsider,
								 AEDesc*				outDescPtr)
{
	OSStatus	result = noErr;
	CFIndex		totalLength = 0L;
	
	
	if (kTerminal_ResultOK != forEachLineDo(inRef, inStartRow, inNumberOfRowsToConsider, addScreenLineLength, &totalLength))
	{
		result = memFullErr;
	}
	else
	{
		UniChar*			buffer = new UniChar[totalLength];
		CFRetainRelease		mutableCFString(CFStringCreateMutableWithExternalCharactersNoCopy
											(kCFAllocatorDefault, buffer, totalLength, totalLength/* capacity */,
												kCFAllocatorNull/* reallocator */),
											true/* is retained */);
		
		
		if (false == mutableCFString.exists()) result = memFullErr;
		else
		{
			if (kTerminal_ResultOK != forEachLineDo(inRef, inStartRow, inNumberOfRowsToConsider,
													appendScreenLineRawToCFString, mutableCFString.returnCFMutableStringRef()))
			{
				// No data?  Out of memory!
				result = memFullErr;
			}
			else result = AECreateDesc(typeUnicodeText, buffer,
										CFStringGetLength(mutableCFString.returnCFMutableStringRef()) * sizeof(UniChar),
										outDescPtr);
		}
		delete [] buffer;
	}
	
	return result;
}// CreateContentsAEDesc


/*!
Returns the zero-based column and line numbers of the
terminal cursor in the given buffer.  Since the cursor
is not allowed to enter the scrollback buffer, negative
return values are not possible.

Pass nullptr for one of the values if you do not want it.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_CursorGetLocation	(TerminalScreenRef		inRef,
							 UInt16*				outZeroBasedColumnPtr,
							 UInt16*				outZeroBasedRowPtr)
{
	Terminal_Result				result = kTerminal_ResultOK;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
	else
	{
		if (outZeroBasedColumnPtr != nullptr) *outZeroBasedColumnPtr = dataPtr->current.cursorX;
		if (outZeroBasedRowPtr != nullptr) *outZeroBasedRowPtr = dataPtr->current.cursorY;
	}
	
	return result;
}// CursorGetLocation


/*!
Returns "true" only if the terminal cursor is supposed
to be visible in the specified screen buffer.  This is
a logical state ONLY, and is set by interaction with the
terminal emulator.  It is only guaranteed to correspond
to a rendered cursor by virtue of the callback framework
that links Terminal Views to screen buffers.

(3.0)
*/
Boolean
Terminal_CursorIsVisible	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->cursorVisible;
	}
	return result;
}// CursorIsVisible


/*!
Provides all the text attributes of the position currently
occupied by the cursor.

NOTE:	Admittedly this is a bit of a weird exception, since
		all other style information is returned as part of the
		Terminal_ForEachLikeAttributeRunDo() loop.  However,
		the cursor is rendered at unusual times, and has a
		special appearance, that make this necessary.

(4.0)
*/
TerminalTextAttributes
Terminal_CursorReturnAttributes		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr			dataPtr = getVirtualScreenData(inRef);
	TerminalTextAttributes		result = dataPtr->current.cursorAttributes;
	
	
	return result;
}// CursorReturnAttributes


/*!
Writes arbitrary debugging information to the console for the
specified terminal screen.

(4.0)
*/
void
Terminal_DebugDumpDetailedSnapshot		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	Console_WriteValuePair("Visible boundary first and last columns", dataPtr->visibleBoundary.firstColumn,
																		dataPtr->visibleBoundary.lastColumn);
	Console_WriteValuePair("Visible boundary first and last rows", dataPtr->visibleBoundary.rows.firstRow,
																	dataPtr->visibleBoundary.rows.lastRow);
	Console_WriteValuePair("Defined scrolling region first and last rows", dataPtr->customScrollingRegion.firstRow,
																			dataPtr->customScrollingRegion.lastRow);
	Console_WriteValuePair("Current scrolling region first and last rows", dataPtr->originRegionPtr->firstRow,
																			dataPtr->originRegionPtr->lastRow);
	Console_WriteValue("Mode: auto-wrap", dataPtr->modeAutoWrap);
	Console_WriteValue("Mode: cursor keys for application", dataPtr->modeCursorKeysForApp);
	Console_WriteValue("Mode: application keys", dataPtr->modeApplicationKeys);
	Console_WriteValue("Mode: origin redefined", dataPtr->modeOriginRedefined);
	Console_WriteValue("Mode: insert, not replace", dataPtr->modeInsertNotReplace);
	Console_WriteValue("Mode: new-line option", dataPtr->modeNewLineOption);
	// INCOMPLETE - could put just about anything here, whatever is interesting to know
}// DebugDumpDetailedSnapshot


/*!
Destroys all scrollback buffer lines.  The “visible”
lines are not affected.  This obviously invalidates any
iterators pointing to scrollback lines.

It follows that after invoking this routine, the return
value of Terminal_ReturnInvisibleRowCount() should be zero.

This triggers two events: "kTerminal_ChangeScrollActivity"
to indicate that data has been removed, and also
"kkTerminal_ChangeTextRemoved" (given a range consisting of
all previous scrollback lines).

(3.1)
*/
void
Terminal_DeleteAllSavedLines	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		SInt16 const	kPreviousScrollbackCount = dataPtr->scrollbackBufferCachedSize;
		
		
		dataPtr->scrollbackBuffer.clear();
		dataPtr->scrollbackBufferCachedSize = 0;
		
		// notify listeners of the range of text that has gone away
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = dataPtr->selfRef;
			range.firstRow = -kPreviousScrollbackCount;
			range.firstColumn = 0;
			range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kPreviousScrollbackCount;
			
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeTextRemoved, &range/* context */);
		}
		
		// notify listeners that scroll activity has taken place,
		// though technically no remaining lines have been affected
		{
			Terminal_ScrollDescription	scrollInfo;
			
			
			bzero(&scrollInfo, sizeof(scrollInfo));
			scrollInfo.screen = dataPtr->selfRef;
			scrollInfo.rowDelta = 0;
			changeNotifyForTerminal(dataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
		}
	}
}// DeleteAllSavedLines


/*!
Scans the given data for telltale signs that it may be
“expecting” a certain type of terminal emulator, setting
"outApparentEmulator" to the emulator that seems most
appropriate.

Use this to help avoid getting the emulator into unknown
states (the parser can recover, but gobbletygook doesn’t
help the user much).

\retval kTerminal_ResultOK
if the text is processed without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(3.1)
*/
Terminal_Result
Terminal_EmulatorDeriveFromCString	(TerminalScreenRef		inRef,
									 char const*			inCString,
									 Terminal_Emulator&		outApparentEmulator)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	outApparentEmulator = kTerminal_EmulatorVT100;
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else
	{
		// INCOMPLETE; besides, this is essentially a heuristic
	}
	
	return result;
}// EmulatorDeriveFromCString


/*!
Returns whether the terminal emulator for the given
terminal is exactly VT100 (and NOT a compatible terminal
such as VT102).

WARNING:	Try not to rely on a routine like this.  It
			is far better to query this module generically
			for terminal features, than to check for a
			specific terminal.  This routine may go away
			in the future.

(3.0)
*/
Boolean
Terminal_EmulatorIsVT100	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = (dataPtr->emulator.primaryType == kTerminal_EmulatorVT100);
	}
	return result;
}// EmulatorIsVT100


/*!
Returns whether the terminal emulator for the given
terminal is exactly VT220 (and NOT a somewhat compatible
terminal such as VT100).

WARNING:	Try not to rely on a routine like this.  It
			is far better to query this module generically
			for terminal features, than to check for a
			specific terminal.  This routine may go away
			in the future.

(3.0)
*/
Boolean
Terminal_EmulatorIsVT220	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = (dataPtr->emulator.primaryType == kTerminal_EmulatorVT220);
	}
	return result;
}// EmulatorIsVT220


/*!
Sends a stream of characters originating in a
C-style string to the specified screen’s terminal
emulator, interpreting escape sequences, etc.
which may affect the terminal display.

USE WITH CARE.  Generally, you send data to this
routine that comes directly from a program that
knows what it’s doing.  There are more elegant
ways to have specific effects on terminal screens
in an emulator-independent fashion; you should
use those routines before hacking up a string as
input to this routine.

\retval kTerminal_ResultOK
if the text is processed without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_EmulatorProcessCString		(TerminalScreenRef	inRef,
									 char const*		inCString)
{
	return Terminal_EmulatorProcessData(inRef, REINTERPRET_CAST(inCString, UInt8 const*), CPP_STD::strlen(inCString));
}// EmulatorProcessCString


/*!
Sends a stream of characters to the specified
screen’s terminal emulator, interpreting escape
sequences, etc. which may affect the terminal
display.

USE WITH CARE.  Generally, you send data to this
routine that comes directly from a program that
knows what it’s doing.  There are more elegant
ways to have specific effects on terminal screens
in an emulator-independent fashion; you should
use those routines before hacking up a string as
input to this routine.

\retval kTerminal_ResultOK
if the text is processed without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_EmulatorProcessData	(TerminalScreenRef	inRef,
								 UInt8 const*		inBuffer,
								 UInt32				inLength)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (inLength != 0)
	{
		My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
		
		
		if (dataPtr == nullptr) result = kTerminal_ResultInvalidID;
		else
		{
			UInt8 const*	ptr = inBuffer;
			UInt32			countRead = 0;
			
			
			// hide cursor momentarily
			setCursorVisible(dataPtr, false);
			
			// interpret the character stream, one character at a time; NOTE that
			// since this is just a slice of a continuous and infinite stream,
			// all state information is kept in the data structure (e.g. the last
			// character in this particular buffer may form the first character
			// in a sequence, and this scan must be continued the next time this
			// function is called)
			for (register UInt32 i = inLength; i > 0; )
			{
				My_ParserStatePair	states = std::make_pair(dataPtr->emulator.currentState,
															dataPtr->emulator.currentState);
				Boolean				isHandled = false;
				Boolean				isInterrupt = false;
				
				
				// debugging only
				if (DebugInterface_LogsTerminalInputChar())
				{
					Console_WriteValueCharacter("terminal loop: received character", *ptr);
				}
				
				// find a new state, which may or may not interrupt the
				// state that is currently forming
				isHandled = false;
				for (My_Emulator::VariantChain::const_iterator toCallbacks = dataPtr->emulator.preCallbackSet.begin();
						((false == isHandled) && (toCallbacks != dataPtr->emulator.preCallbackSet.end())); ++toCallbacks)
				{
					countRead = invokeEmulatorStateDeterminantProc
								(toCallbacks->stateDeterminant, &dataPtr->emulator,
									ptr, i, states, isInterrupt, isHandled);
				}
				unless (isHandled)
				{
					// most of the time, the ordinary emulator is used
					countRead = invokeEmulatorStateDeterminantProc
								(dataPtr->emulator.currentCallbacks.stateDeterminant, &dataPtr->emulator,
									ptr, i, states, isInterrupt, isHandled);
				}
				assert(countRead <= i);
				//Console_WriteValue("number of characters absorbed by state determinant", countRead); // debug
				i -= countRead; // could be zero (no-op)
				ptr += countRead; // could be zero (no-op)
				
				// DEBUG:
				//Console_WriteValueFourChars("transition to state", states.second);
				
				// LOOPING GUARD: whenever the proposed next state matches the
				// current state, a counter is incremented; if this count
				// exceeds an arbitrary value, the next state is FORCED to
				// return to the initial state (flagging a console error) so
				// that this does not hang the application in an infinite loop
				if (states.first == states.second)
				{
					// exclude the echo data, because this is the most likely state
					// to last awhile (e.g. long strings of printable text)
					if (states.second != kMy_ParserStateAccumulateForEcho)
					{
						Boolean		interrupt = (dataPtr->emulator.stateRepetitions > 100/* arbitrary */);
						
						
						++dataPtr->emulator.stateRepetitions;
						
						if (interrupt)
						{
							// some states allow a bit more leeway before panicking,
							// because there could be good reasons for long data streams
							// TEMPORARY; it may make the most sense to also defer loop
							// evasion to an emulator method, so that each emulator type
							// can handle its own custom states (and only when that
							// emulator is actually in use!)
							if ((states.second == My_XTerm::kStateSITAcquireStr) ||
								(states.second == My_XTerm::kStateSWTAcquireStr) ||
								(states.second == My_XTerm::kStateSWITAcquireStr))
							{
								interrupt = (dataPtr->emulator.stateRepetitions > 255/* arbitrary */);
							}
						}
						
						if (interrupt)
						{
							Boolean const	kLogThis = DebugInterface_LogsTerminalState();
							
							
							if (kLogThis)
							{
								Console_WriteHorizontalRule();
								Console_WriteValueFourChars("SERIOUS PARSER ERROR: appears to be stuck, state", states.first);
							}
							
							if (kMy_ParserStateInitial == states.first)
							{
								// if somehow stuck oddly in the initial state, assume
								// the trigger character is responsible and simply
								// ignore the troublesome sequence of characters
								if (kLogThis)
								{
									Console_WriteValueCharacter("FORCING step-over of trigger character", *ptr);
								}
								--i;
								++ptr;
							}
							else
							{
								if (kLogThis)
								{
									Console_WriteLine("FORCING a return to the initial state");
								}
								states.second = kMy_ParserStateInitial;
							}
							
							if (kLogThis)
							{
								Console_WriteHorizontalRule();
							}
							
							dataPtr->emulator.stateRepetitions = 0;
						}
					}
				}
				else
				{
					dataPtr->emulator.stateRepetitions = 0;
				}
				
				if (kMy_ParserStateAccumulateForEcho == states.second)
				{
					// gather a byte for later use in display, but do not display yet;
					// while it would be nice to feed the raw data stream into the
					// translation APIs, translators might not stop when they see
					// special characters such as control characters; so, the terminal
					// emulator (above) has the first crack at the byte to see if there
					// is any special meaning, and the byte is cached only if the
					// terminal allows the byte to be echoed; the echo does not actually
					// occur until a future byte triggers a non-echo state; at that
					// time, the cached array of bytes is sent to the translator and
					// converted into human-readable text
					dataPtr->bytesToEcho.push_back(*ptr);
					--i;
					++ptr;
				}
				
				// if the new state is no longer echo accumulation, or this chunk of the
				// infinite stream ended with echo-ready data, flush as much as possible
				if ((kMy_ParserStateAccumulateForEcho != states.second) || (0 == i))
				{
					if (false == dataPtr->bytesToEcho.empty())
					{
						std::string::size_type const	kOldSize = dataPtr->bytesToEcho.size();
						UInt32							bytesUsed = invokeEmulatorEchoDataProc
																	(dataPtr->emulator.currentCallbacks.dataWriter, dataPtr,
																		dataPtr->bytesToEcho.data(), kOldSize);
						
						
						// it is possible for this chunk of the stream to terminate with
						// an incomplete encoding of bytes, so preserve anything that
						// could not be echoed this time around
						assert(bytesUsed <= kOldSize);
						if (0 == bytesUsed)
						{
							// special case...some kind of error, no bytes were translated;
							// dump the buffer, which LOSES DATA, but this is a spin control
							++(dataPtr->echoErrorCount);
							dataPtr->bytesToEcho.clear();
						}
						else if (bytesUsed > 0)
						{
							dataPtr->bytesToEcho.erase(0/* starting position */, bytesUsed/* count */);
						}
					}
				}
				
				// perform whatever action is appropriate to enter this state
				isHandled = false;
				for (My_Emulator::VariantChain::const_iterator toCallbacks = dataPtr->emulator.preCallbackSet.begin();
						((false == isHandled) && (toCallbacks != dataPtr->emulator.preCallbackSet.end())); ++toCallbacks)
				{
					countRead = invokeEmulatorStateTransitionProc
								(toCallbacks->transitionHandler, dataPtr, ptr, i, states, isHandled);
				}
				unless (isHandled)
				{
					countRead = invokeEmulatorStateTransitionProc
								(dataPtr->emulator.currentCallbacks.transitionHandler,
									dataPtr, ptr, i, states, isHandled);
				}
				assert(countRead <= i);
				//Console_WriteValue("number of characters absorbed by transition handler", countRead); // debug
				i -= countRead; // could be zero (no-op)
				ptr += countRead; // could be zero (no-op)
				
				if (false == isInterrupt)
				{
					// remember this new state
					dataPtr->emulator.currentState = states.second;
				}
			}
			
			// restore cursor
			setCursorVisible(dataPtr, true);
		}
		
		// to minimize spam, count certain classes of data error in
		// the loop above, and only report them at the end
		if ((dataPtr->echoErrorCount != 0) ||
			(dataPtr->translationErrorCount != 0))
		{
			Boolean const	kDebugExcessiveErrors = false;
			
			
			++(dataPtr->errorCountTotal);
			if (dataPtr->errorCountTotal == 10/* arbitrary; equality is used to ensure that this event can only fire once */)
			{
				changeNotifyForTerminal(dataPtr, kTerminal_ChangeExcessiveErrors, inRef);
			}
			
			if (kDebugExcessiveErrors)
			{
				Console_Warning(Console_WriteValue, "at least some characters were SKIPPED due to the following errors; original buffer length", inLength);
			}
			if (dataPtr->echoErrorCount != 0)
			{
				if (kDebugExcessiveErrors)
				{
					Console_Warning(Console_WriteValue, "number of times that echoing unexpectedly failed", dataPtr->echoErrorCount);
				}
				dataPtr->echoErrorCount = 0;
			}
			if (dataPtr->translationErrorCount != 0)
			{
				if (kDebugExcessiveErrors)
				{
					Console_WriteValueCharacter("current terminal text encoding", dataPtr->emulator.inputTextEncoding);
					Console_Warning(Console_WriteValue, "number of times that translation unexpectedly failed", dataPtr->translationErrorCount);
				}
				dataPtr->translationErrorCount = 0;
			}
		}
	}
	
	return result;
}// EmulatorProcessData


/*!
Returns the default name for the given emulation type,
suitable for use in a TERM environment variable or
answer-back message.  For example, "vt100" is the name
of kTerminal_EmulatorVT100.

The string is not retained, so do not release it.

See also Terminal_EmulatorReturnForName(), the reverse
of this routine, and Terminal_EmulatorReturnName(), which
returns whatever a screen is using.

(3.1)
*/
CFStringRef
Terminal_EmulatorReturnDefaultName		(Terminal_Emulator	inEmulationType)
{
	CFStringRef		result = nullptr;
	
	
	// IMPORTANT: This should be the inverse of Terminal_EmulatorReturnForName().
	switch (inEmulationType)
	{
	case kTerminal_EmulatorANSIBBS:
		result = CFSTR("ansi-bbs");
		break;
	
	case kTerminal_EmulatorANSISCO:
		result = CFSTR("ansi-sco");
		break;
	
	case kTerminal_EmulatorDumb:
		result = CFSTR("dumb");
		break;
	
	case kTerminal_EmulatorVT100:
		result = CFSTR("vt100");
		break;
	
	case kTerminal_EmulatorVT102:
		result = CFSTR("vt102");
		break;
	
	case kTerminal_EmulatorVT220:
		result = CFSTR("vt220");
		break;
	
	case kTerminal_EmulatorVT320:
		result = CFSTR("vt320");
		break;
	
	case kTerminal_EmulatorVT420:
		result = CFSTR("vt420");
		break;
	
	case kTerminal_EmulatorXTermColor:
		result = CFSTR("xterm-color");
		break;
	
	case kTerminal_EmulatorXTerm256Color:
		result = CFSTR("xterm-256color");
		break;
	
	case kTerminal_EmulatorXTermOriginal:
		result = CFSTR("xterm");
		break;
	
	default:
		// ???
		assert(false);
		result = CFSTR("UNKNOWN");
		break;
	}
	
	return result;
}// EmulatorReturnDefaultName


/*!
Returns the emulation type for the given name, if any
(otherwise, chooses a reasonable value).  For example,
"vt100" corresponds to kTerminal_EmulatorVT100.

See also EmulatorReturnDefaultName().

(3.1)
*/
Terminal_Emulator
Terminal_EmulatorReturnForName		(CFStringRef	inName)
{
	Terminal_Emulator	result = kTerminal_EmulatorVT100;
	
	
	// IMPORTANT: This should be the inverse of EmulatorReturnDefaultName().
	if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("ansi-bbs"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorANSIBBS;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("ansi-sco"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorANSISCO;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("dumb"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorDumb;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt100"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorVT100;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt102"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorVT102;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt220"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorVT220;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt320"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorVT320;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("vt420"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorVT420;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorXTermOriginal;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm-color"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorXTermColor;
	}
	else if (kCFCompareEqualTo == CFStringCompare(inName, CFSTR("xterm-256color"), kCFCompareCaseInsensitive | kCFCompareBackwards))
	{
		result = kTerminal_EmulatorXTerm256Color;
	}
	else
	{
		// ???
		assert(false);
	}
	
	return result;
}// EmulatorReturnForName


/*!
Returns the current name (often called the answer-back
message) for the emulator being used by the given screen.
This value really could be anything; it is typically used
to set a TERM environment variable for communication
about terminal type with a running process.  If the
process does not directly support a MacTelnet emulator,
but supports something mostly compatible, this routine
may return the name of the compatible terminal.

The string is not retained, so do not release it.

See also EmulatorReturnDefaultName(), which always
returns a specific name for a supported emulator.

(3.1)
*/
CFStringRef
Terminal_EmulatorReturnName		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	CFStringRef				result = nullptr;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->emulator.answerBackCFString.returnCFStringRef();
	}
	
	return result;
}// EmulatorReturnName


/*!
Changes the kind of terminal a virtual terminal
will emulate.

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

(3.0)
*/
Terminal_Result
Terminal_EmulatorSet	(TerminalScreenRef	inRef,
						 Terminal_Emulator	inEmulationType)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else dataPtr->emulator.changeTo(inEmulationType);
	
	return result;
}// EmulatorSet


/*!
Initiates a capture to a file for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureBegun"
events if successful.  The callbacks are invoked after
internal file capture state has been set up.

If "inAutoClose" is set to true, then you NO LONGER OWN
the given File Manager file, and you should not close it
yourself; it will be closed for you, whenever the capture
ends (through Terminal_FileCaptureEnd(), or the destruction
of this terminal screen).  Otherwise, YOU must close the
file, after you have finished with the terminal that is
writing to it.  See the "kTerminal_ChangeFileCaptureEnding"
event, which will tell you exactly when the capture is
finished.

If the capture begins successfully, "true" is returned;
otherwise, "false" is returned.

(3.0)
*/
Boolean
Terminal_FileCaptureBegin	(TerminalScreenRef	inRef,
							 SInt16				inOpenWritableFile,
							 Boolean			inAutoClose)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Boolean					result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = StreamCapture_Begin(dataPtr->captureStream, inOpenWritableFile, inAutoClose);
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureBegun, inRef);
	}
	return result;
}// FileCaptureBegin


/*!
Terminates the file capture for the specified screen,
notifying listeners of "kTerminal_ChangeFileCaptureEnding"
events just prior to clearing internal file capture state.

Since the Terminal module does not open the capture file,
you must normally close it yourself using FSClose() after
this routine returns.  HOWEVER, if you set the auto-close
flag in Terminal_FileCaptureBegin(), then FSClose() WILL
be called for you, and you lose ownership of the file.

(3.0)
*/
void
Terminal_FileCaptureEnd		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeFileCaptureEnding, inRef);
		StreamCapture_End(dataPtr->captureStream);
	}
}// FileCaptureEnd


/*!
Returns "true" only if there is a file capture in
progress for the specified screen.

(2.6)
*/
Boolean
Terminal_FileCaptureInProgress	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = StreamCapture_InProgress(dataPtr->captureStream);
	}
	return result;
}// FileCaptureInProgress


/*!
Iterates over the given row of the specified terminal screen,
executing a function on each chunk of text for which the
associated attributes are all IDENTICAL.  The context is
defined by you, and is passed directly to the specified
function each time it is invoked.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultParameterError
if the screen run operation function is invalid

\retval kTerminal_ResultNotEnoughMemory
if any line buffers are unexpectedly empty

(3.0)
*/
Terminal_Result
Terminal_ForEachLikeAttributeRunDo	(TerminalScreenRef			inRef,
									 Terminal_LineRef			inStartRow,
									 Terminal_ScreenRunProcPtr	inDoWhat,
									 void*						inContextPtr)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		screenPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if ((nullptr == inDoWhat) || (nullptr == screenPtr) || (nullptr == iteratorPtr))
	{
		result = kTerminal_ResultParameterError;
	}
	else
	{
		// need to search line for style chunks
		My_ScreenBufferLine&					currentLine = iteratorPtr->currentLine();
		My_TextIterator							textIterator = nullptr;
		My_TextAttributesList::const_iterator	attrIterator = currentLine.attributeVector.begin();
		TerminalTextAttributes					previousAttributes = 0;
		TerminalTextAttributes					currentAttributes = 0;
		SInt16									runStartCharacterIndex = 0;
		SInt16									characterIndex = 0;
		size_t									styleRunLength = 0;
		
		
	#if 0
		// DEBUGGING ONLY: if you suspect a bug in the incremental loop below,
		// try asking the entire line to be drawn without formatting, first
		InvokeScreenRunOperationProc(inDoWhat, inRef, currentLine.textVectorBegin/* starting point */,
										currentLine.textVectorSize/* length */,
										inStartRow, 0/* zero-based start column */,
										currentLine.globalAttributes, inContextPtr);
	#endif
		
		// TEMPORARY - HIGHLY inefficient to search here, need to change this into a cache
		//             (in fact, attribute bit arrays can probably be completely replaced by
		//             style runs at some point in the future)
		assert(nullptr != currentLine.textVectorBegin);
		for (textIterator = currentLine.textVectorBegin,
					attrIterator = currentLine.attributeVector.begin();
				(textIterator != currentLine.textVectorEnd) &&
					(attrIterator != currentLine.attributeVector.end());
				++textIterator, ++attrIterator, ++characterIndex)
		{
			currentAttributes = *attrIterator;
			if ((currentAttributes != previousAttributes) ||
				(characterIndex == STATIC_CAST(currentLine.textVectorSize - 1, SInt16)) ||
				(characterIndex == STATIC_CAST(currentLine.attributeVector.size() - 1, SInt16)))
			{
				styleRunLength = characterIndex - runStartCharacterIndex;
				
				// found new style run; so handle the previous run
				if (styleRunLength > 0)
				{
					TerminalTextAttributes		rangeAttributes = previousAttributes;
					
					
					STYLE_ADD(rangeAttributes, currentLine.globalAttributes);
					Terminal_InvokeScreenRunProc(inDoWhat, inRef, currentLine.textVectorBegin + runStartCharacterIndex/* starting point */,
													styleRunLength/* length */, inStartRow,
													runStartCharacterIndex/* zero-based start column */,
													rangeAttributes, inContextPtr);
				}
				
				// reset for next run
				previousAttributes = currentAttributes;
				runStartCharacterIndex = characterIndex;
			}
		}
		
		// ask that the remainder of the line be handled as if it were blank
		if (textIterator != currentLine.textVectorEnd)
		{
			styleRunLength = std::distance(textIterator, currentLine.textVectorEnd);
			
			// found new style run; so handle the previous run
			if (styleRunLength > 0)
			{
				TerminalTextAttributes		attributesForRemainder = currentLine.globalAttributes;
				
				
				// the “selected” attribute is special; it persists regardless
				if (STYLE_SELECTED(previousAttributes))
				{
					STYLE_ADD(attributesForRemainder, kTerminalTextAttributeSelected);
				}
				
				Terminal_InvokeScreenRunProc(inDoWhat, inRef, nullptr/* starting point */, styleRunLength/* length */,
												inStartRow, runStartCharacterIndex/* zero-based start column */,
												attributesForRemainder, inContextPtr);
			}
		}
	}
	return result;
}// ForEachLikeAttributeRunDo


/*!
Returns ONLY the attributes that were assigned to the
specified line in a global manner.  For example, double-sized
text is applied to an entire line, never to a single column.

IMPORTANT:	To properly render a line, its global attributes
			must be mixed with the attributes of the current
			cursor location.

(3.0)
*/
Terminal_Result
Terminal_GetLineGlobalAttributes	(TerminalScreenRef			UNUSED_ARGUMENT(inScreen),
									 Terminal_LineRef			inRow,
									 TerminalTextAttributes*	outAttributesPtr)
{
	Terminal_Result			result = kTerminal_ResultOK;
	//My_ScreenBufferConstPtr	dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	if ((iteratorPtr == nullptr) || (outAttributesPtr == nullptr)) result = kTerminal_ResultParameterError;
	else
	{
		*outAttributesPtr = iteratorPtr->currentLine().globalAttributes;
	}
	
	return result;
}// GetLineGlobalAttributes


/*!
Like Terminal_GetLineRange(), but automatically pulls in the
entire line (from the first column to past the end column).

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

(3.1)
*/
Terminal_Result
Terminal_GetLine	(TerminalScreenRef			inScreen,
					 Terminal_LineRef			inRow,
					 UniChar const*&			outPossibleReferenceStart,
					 UniChar const*&			outPossibleReferencePastEnd,
					 Terminal_TextFilterFlags	inFlags)
{
	return Terminal_GetLineRange(inScreen, inRow, 0/* first column */, -1/* last column; negative means “very end” */,
									outPossibleReferenceStart, outPossibleReferencePastEnd, inFlags);
}// GetLine


/*!
Allows read-only access to a single line of text - everything
from the specified start column, inclusive, of the given row to
the specified end column, exclusive.

Pass -1 for the end column to conveniently refer to the end of
the line.  Otherwise, pass a nonnegative number to index a
column, where 0 is the first column.

The pointers must be immediately used to access data; they could
become invalid (for instance, if the line is about to be
scrolled into oblivion from the oldest part of the scrollback
buffer).

As their names imply, the range parameters are inclusive at the
beginning and exclusive at the end, such that the pointer
difference is zero if the range is empty.  (Like the STL.)

NOTE:	This API is somewhat implementation dependent.  So this
		API could change in the future, and any code that calls
		it would have to change too.

\retval kTerminal_ResultOK
if the data was copied successfully

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultInvalidIterator
if the specified row reference is invalid

\retval kTerminal_ResultParameterError
if the specified column is out of range and nonnegative

(3.1)
*/
Terminal_Result
Terminal_GetLineRange	(TerminalScreenRef			inScreen,
						 Terminal_LineRef			inRow,
						 UInt16						inZeroBasedStartColumn,
						 SInt16						inZeroBasedPastEndColumnOrNegativeForLastColumn,
						 UniChar const*&			outReferenceStart,
						 UniChar const*&			outReferencePastEnd,
						 Terminal_TextFilterFlags	inFlags)
{
	Terminal_Result			result = kTerminal_ResultParameterError;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inScreen);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	
	
	outReferenceStart = nullptr;
	outReferencePastEnd = nullptr;
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == iteratorPtr) result = kTerminal_ResultInvalidIterator;
	else
	{
		UInt16 const	kPastEndColumn = (inZeroBasedPastEndColumnOrNegativeForLastColumn < 0)
											? dataPtr->text.visibleScreen.numberOfColumnsPermitted
											: inZeroBasedPastEndColumnOrNegativeForLastColumn;
		
		
		if (kPastEndColumn > iteratorPtr->currentLine().textVectorSize)
		{
			result = kTerminal_ResultParameterError;
		}
		else
		{
			outReferenceStart = iteratorPtr->currentLine().textVectorBegin + inZeroBasedStartColumn;
			outReferencePastEnd = iteratorPtr->currentLine().textVectorBegin + kPastEndColumn;
			if (inFlags & kTerminal_TextFilterFlagsNoEndWhitespace)
			{
				UniChar const*		lastCharPtr = outReferencePastEnd - 1;
				
				
				// LOCALIZE THIS
				while ((lastCharPtr != outReferenceStart) && std::isspace(*lastCharPtr))
				{
					--lastCharPtr;
				}
				outReferencePastEnd = lastCharPtr + 1;
			}
			result = kTerminal_ResultOK;
		}
	}
	
	return result;
}// GetLineRange


/*!
Returns "true" only if the LED with the specified
number is currently on.  The meaning of an LED with
a specific number is left to the caller.

Currently, only 4 LEDs are defined.  Providing an
LED number of 5 or greater will result in a "false"
return value.

Numbers less than zero are reserved.  Probably, they
will one day be used to determine if specific LEDs
are OFF.

(3.0)
*/
Boolean
Terminal_LEDIsOn	(TerminalScreenRef	inRef,
					 SInt16				inOneBasedLEDNumber)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		switch (inOneBasedLEDNumber)
		{
		case 1:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight1));
			break;
		
		case 2:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight2));
			break;
		
		case 3:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight3));
			break;
		
		case 4:
			result = (0 != (dataPtr->litLEDs & kMy_LEDBitsLight4));
			break;
		
		case 0:
		default:
			// ???
			break;
		}
	}
	return result;
}// LEDIsOn


/*!
Sets an LED to on or off.  Values less than or equal to
zero are reserved.

Note that LEDs are ordinarily entirely under the control
of the terminal, and the terminal could later change the
LED state anyway.

(3.1)
*/
void
Terminal_LEDSetState	(TerminalScreenRef	inRef,
						 SInt16				inOneBasedLEDNumber,
						 Boolean			inIsHighlighted)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		switch (inOneBasedLEDNumber)
		{
		case 1:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight1;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight1;
			break;
		
		case 2:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight2;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight2;
			break;
		
		case 3:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight3;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight3;
			break;
		
		case 4:
			if (inIsHighlighted) dataPtr->litLEDs |= kMy_LEDBitsLight4;
			else dataPtr->litLEDs &= ~kMy_LEDBitsLight4;
			break;
		
		case 0:
		default:
			// ???
			break;
		}
	}
	changeNotifyForTerminal(dataPtr, kTerminal_ChangeNewLEDState, dataPtr->selfRef/* context */);
}// LEDSetState


/*!
Changes an iterator to point to a different line, one that is
the specified number of rows later than or earlier than the
row that the iterator currently points to.  Pass a value less
than zero to find a previous row, otherwise positive values
find following rows.

\retval kTerminal_ResultOK
if the iterator is advanced or backed-up successfully

\retval kTerminal_ResultParameterError
if the iterator is completely invalid

\retval kTerminal_ResultIteratorCannotAdvance
if the iterator cannot move in the requested direction (already
at oldest scrollback or screen line, or already at bottommost
screen line)

(3.0)
*/
Terminal_Result
Terminal_LineIteratorAdvance	(TerminalScreenRef		UNUSED_ARGUMENT(inRef),
								 Terminal_LineRef		inRow,
								 SInt16					inHowManyRowsForwardOrNegativeForBackward)
{
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inRow);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (iteratorPtr == nullptr) result = kTerminal_ResultParameterError;
	else
	{
		Boolean		isEnd = false;
		
		
		if (inHowManyRowsForwardOrNegativeForBackward > 0)
		{
			SInt16		i = 0;
			
			
			for (i = 0; i < inHowManyRowsForwardOrNegativeForBackward; ++i)
			{
				iteratorPtr->goToNextLine(isEnd);
				if (isEnd) break;
			}
			if (inHowManyRowsForwardOrNegativeForBackward != i)
			{
				result = kTerminal_ResultIteratorCannotAdvance;
			}
		}
		else
		{
			SInt16		i = 0;
			
			
			for (i = inHowManyRowsForwardOrNegativeForBackward; i < 0; ++i)
			{
				iteratorPtr->goToPreviousLine(isEnd);
				if (isEnd) break;
			}
			if (0 != i)
			{
				result = kTerminal_ResultIteratorCannotAdvance;
			}
		}
	}
	return result;
}// LineIteratorAdvance


/*!
Returns "true" only if the given terminal has mapped Return
key presses and line feeds to the “carriage return, line feed”
behavior.  If "false", a Return sends only a carriage return
and a line feed only moves the cursor vertically.

Valid in VT terminals; may not be defined for others.

(3.1)
*/
Boolean
Terminal_LineFeedNewLineMode	(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->modeNewLineOption;
	}
	return result;
}// LineFeedNewLineMode


/*!
Returns "true" only if the given terminal automatically moves
the cursor to the beginning of the next line (and inserts
text there) when an attempt to write past the limit of the
current line is made.

(3.0)
*/
Boolean
Terminal_LineWrapIsEnabled		(TerminalScreenRef	inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->modeAutoWrap;
	}
	return result;
}// LineWrapIsEnabled


/*!
Resets the indicated terminal settings to their default
states.  Pass "kTerminal_ResetFlagsAll" to do a standard
terminal reset, which among other things clears the screen
and homes the cursor.

(3.0)
*/
void
Terminal_Reset		(TerminalScreenRef		inRef,
					 Terminal_ResetFlags	inFlags)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		if (inFlags & kTerminal_ResetFlagsGraphicsCharacters)
		{
			// “rescue” the user by forcing graphics not to be used
			{
				Terminal_RangeDescription	range;
				Terminal_LineRef			lineIterator = nullptr;
				
				
				range.screen = dataPtr->selfRef;
				range.firstRow = 0;
				range.firstColumn = 0;
				range.columnCount = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = dataPtr->screenBuffer.size();
				
				lineIterator = Terminal_NewMainScreenLineIterator(inRef, range.firstRow);
				if (lineIterator != nullptr)
				{
					// remove this mode attribute from the current character set
					dataPtr->current.characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
					
					// force all visible characters to no longer use graphics
					(Terminal_Result)Terminal_ChangeRangeAttributes
										(dataPtr->selfRef, lineIterator/* first row */, range.rowCount,
											range.firstColumn/* start */, range.firstColumn + range.columnCount/* past the end */,
											false/* constrain to rectangle */, 0/* attributes to set */,
											kTerminalTextAttributeVTGraphics/* attributes to clear */);
					
					// add the entire visible buffer to the text-change region;
					// this should trigger things like Terminal View updates
					//Console_WriteLine("text changed event: reset terminal");
					changeNotifyForTerminal(dataPtr, kTerminal_ChangeTextEdited, &range);
					
					// destroy the iterator
					Terminal_DisposeLineIterator(&lineIterator);
				}
			}
		}
		
		// TEMPORARY - currently, this is not broken down into parts,
		//             but if more flag bits are defined in the future
		//             this should probably do exactly what is requested
		//             and nothing more
		if (inFlags == kTerminal_ResetFlagsAll)
		{
			setCursorVisible(dataPtr, false);
			resetTerminal(dataPtr); // homes cursor, among other things
			setCursorVisible(dataPtr, true);
			// ensure cursor is in visible screen area in all views - UNIMPLEMENTED
		}
	}
}// Reset


/*!
Returns the maximum number of columns allowed
(useful in order to limit a text field in a
dialog box, for example).

(3.0)
*/
UInt16
Terminal_ReturnAllocatedColumnCount ()
{
	return kMy_NumberOfCharactersPerLineMaximum;
}// ReturnAllocatedColumnCount


/*!
Returns the number of characters wide the specified
terminal screen is (regardless of however many may
be visible in the window that contains it).

(3.0)
*/
UInt16
Terminal_ReturnColumnCount		(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
	}
	return result;
}// ReturnColumnCount


/*!
Returns a variety of preferences unique to this screen.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Terminal Screen and used
to automatically update the emulator and internal caches.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one screen may be absent in another.

(3.1)
*/
Preferences_ContextRef
Terminal_ReturnConfiguration	(TerminalScreenRef		inRef)
{
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef		result = dataPtr->configuration;
	
	
	// since many settings are represented internally, this context
	// will not contain the latest information; update the context
	// based on current settings
	
	// INCOMPLETE
	
	{
		UInt16		dimension = dataPtr->text.visibleScreen.numberOfColumnsPermitted;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenColumns,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		UInt16		dimension = dataPtr->screenBuffer.size();
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenRows,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		UInt32		dimension = dataPtr->text.scrollback.numberOfRowsPermitted;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalScreenScrollbackRows,
													sizeof(dimension), &dimension);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	return result;
}// ReturnConfiguration


/*!
Returns the number of saved lines that have scrolled
off the top of the screen.

(3.0)
*/
UInt32
Terminal_ReturnInvisibleRowCount	(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->scrollbackBufferCachedSize;
	}
	return result;
}// ReturnInvisibleRowCount


/*!
Returns the number of lines long the specified
terminal screen’s main screen area is (minus any
scrollback lines).

(3.0)
*/
UInt16
Terminal_ReturnRowCount		(TerminalScreenRef		inRef)
{
	UInt16						result = 0;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->screenBuffer.size();
	}
	return result;
}// ReturnRowCount


/*!
Returns the Terminal Speaker object that handles
audio for the given terminal.

IMPORTANT:	This API is under evaluation.  Returning
			a delegate object is usually a sign of
			design weakness, maybe what should happen
			is the speaker should be controlled by a
			larger entity such as Terminal Window.
			Hmmm...

(3.0)
*/
TerminalSpeaker_Ref
Terminal_ReturnSpeaker		(TerminalScreenRef	inRef)
{
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	TerminalSpeaker_Ref			result = nullptr;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->speaker;
	}
	return result;
}// ReturnSpeaker


/*!
Returns the encoding of text streams read by the terminal
emulator.  This does *not* indicate the internal buffer
encoding, which might be Unicode regardless.

Returns "kCFStringEncodingInvalidId" if for any reason the
encoding cannot be found.

(4.0)
*/
CFStringEncoding
Terminal_ReturnTextEncoding		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	CFStringEncoding	result = kCFStringEncodingInvalidId;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->emulator.inputTextEncoding;
	}
	return result;
}// ReturnTextEncoding


/*!
Returns "true" only if the specified screen should
be rendered in reverse video mode - that is, with
the foreground and background colors swapped prior
to rendering.

(3.0)
*/
Boolean
Terminal_ReverseVideoIsEnabled	(TerminalScreenRef		inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->reverseVideo;
	}
	return result;
}// ReverseVideoIsEnabled


/*!
Returns "true" only if the lines of the terminal
screen are scrolled prior to a clearing of the
visible screen area.

(3.0)
*/
Boolean
Terminal_SaveLinesOnClearIsEnabled		(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = dataPtr->saveToScrollbackOnClear;
	}
	return result;
}// SaveLinesOnClearIsEnabled


/*!
Searches the specified terminal screen buffer using the given
query and flags as a guide, and returns zero or more matches.
All matching ranges are returned.

This is not guaranteed to be perfectly efficient; in particular,
currently the internal implementation needs to copy data in order
to pass it to the scanner.  In the future, if the internal
storage format is made to match that of the search engine, the
search may be faster.

\retval kTerminal_ResultOK
if no error occurs

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the query string is invalid or an unrecognized flag is given

\retval kTerminal_ResultNotEnoughMemory
if the buffer is too large to search

(3.1)
*/
Terminal_Result
Terminal_Search		(TerminalScreenRef							inRef,
					 CFStringRef								inQuery,
					 Terminal_SearchFlags						inFlags,
					 std::vector< Terminal_RangeDescription >&	outMatches)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == inQuery) result = kTerminal_ResultParameterError;
	else
	{
		typedef std::vector< My_ScreenBufferLineList* >		BufferList;
		BufferList		buffersToSearch;
		CFOptionFlags	searchFlags = 0;
		UInt16			bufferIndex = 0;
		
		
		// translate given flags to Core Foundation String search flags
		if (0 == (inFlags & kTerminal_SearchFlagsCaseSensitive)) searchFlags |= kCFCompareCaseInsensitive;
		if (inFlags & kTerminal_SearchFlagsSearchBackwards) searchFlags |= kCFCompareBackwards;
		
		// search the screen first, then the scrollback (backwards) if it exists
		buffersToSearch.push_back(&dataPtr->screenBuffer);
		buffersToSearch.push_back(&dataPtr->scrollbackBuffer);
		for (BufferList::const_iterator toBuffer = buffersToSearch.begin();
				toBuffer != buffersToSearch.end(); ++toBuffer, ++bufferIndex)
		{
			Boolean const				kIsScreen = (0 == bufferIndex); // else scrollback
			My_ScreenBufferLineList&	kBuffer = *(*toBuffer);
			SInt32						rowIndex = 0; // reset per buffer
			
			
			for (My_ScreenBufferLineList::const_iterator toLine = kBuffer.begin();
					toLine != kBuffer.end(); ++toLine, ++rowIndex)
			{
				// find ALL matches; NOTE that this technically will not find words
				// that begin at the end of one line and continue at the start of
				// the next, but that is a known limitation right now (TEMPORARY)
				My_ScreenBufferLine const&	kLine = *toLine;
				CFStringRef const			kCFStringToSearch = kLine.textCFString.returnCFStringRef();
				CFRetainRelease				resultsArray(CFStringCreateArrayWithFindResults
															(kCFAllocatorDefault, kCFStringToSearch, inQuery,
																CFRangeMake(0, CFStringGetLength(kCFStringToSearch)),
																searchFlags),
															true/* already retained */);
				
				
				if (resultsArray.exists())
				{
					CFArrayRef const	kResultsArrayRef = resultsArray.returnCFArrayRef();
					CFIndex const		kNumberOfMatches = CFArrayGetCount(kResultsArrayRef);
					
					
					// return the range of text that was found
					outMatches.reserve(outMatches.size() + kNumberOfMatches);
					for (CFIndex i = 0; i < kNumberOfMatches; ++i)
					{
						CFRange const*				toRange = REINTERPRET_CAST(CFArrayGetValueAtIndex(kResultsArrayRef, i),
																				CFRange const*);
						SInt32						firstRow = rowIndex;
						UInt16						firstColumn = 0;
						UInt16						secondColumn = 0;
						Terminal_RangeDescription	textRegion;
						
						
						// translate all results ranges into external form; the
						// caller understands rows and columns, etc. not offsets
						// into a giant buffer
						//getBufferOffsetCell(dataPtr, toRange->location, kEndOfLinePad, firstColumn, firstRow);
						//getBufferOffsetCell(dataPtr, toRange->location + toRange->length, kEndOfLinePad, secondColumn, firstRow);
						firstColumn = toRange->location;
						secondColumn = toRange->location + toRange->length;
						if (false == kIsScreen)
						{
							// translate scrollback into negative coordinates (zero-based)
							firstRow = -firstRow - 1;
						}
						bzero(&textRegion, sizeof(textRegion));
						textRegion.screen = inRef;
						textRegion.firstRow = firstRow;
						textRegion.firstColumn = firstColumn;
						textRegion.columnCount = toRange->length;
						textRegion.rowCount = 1;
						outMatches.push_back(textRegion);
					}
				}
				else
				{
					// text was not found
					//Console_WriteLine("string not found");
				}
			}
		}
	}
	
	return result;
}// SearchForPhrase


/*!
Specifies whether the given terminal’s bell is active.
An inactive bell completely ignores all bell signals -
without giving any audible or visible indication that
a bell has occurred.  This can be useful if you know
you’ve just triggered a long string of bells and don’t
want to be annoyed by a series of beeps or flashes.

(3.0)
*/
void
Terminal_SetBellEnabled		(TerminalScreenRef	inRef,
							 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		dataPtr->bellDisabled = !inIsEnabled;
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeAudioState, dataPtr->selfRef/* context */);
	}
}// SetBellEnabled


/*!
Sets the string that will be printed by a dumb terminal
(kTerminal_EmulatorDumb) when the specified character is
to be displayed.  The description must be in UTF-8 encoding.

Normally, any character that is considered “printable”
should be echoed as-is, so this is the default behavior
if no mapping has been given for a printable character.

(3.1)
*/
void
Terminal_SetDumbTerminalRendering	(UniChar		inCharacter,
									 char const*	inDescription)
{
	CFStringRef		descriptionCFString = CFStringCreateWithCString(kCFAllocatorDefault, inDescription, kCFStringEncodingUTF8);
	
	
	if (nullptr == descriptionCFString) Console_Warning(Console_WriteLine, "unexpected error creating UTF-8 string for description");
	else
	{
		gDumbTerminalRenderings()[inCharacter] = descriptionCFString;
		CFRelease(descriptionCFString), descriptionCFString = nullptr;
	}
}// SetDumbTerminalRendering


/*!
Specifies whether the given terminal automatically moves
the cursor to the beginning of the next line (and inserts
text there) when an attempt to write past the limit of the
current line is made.

(3.0)
*/
void
Terminal_SetLineWrapEnabled		(TerminalScreenRef	inRef,
								 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		dataPtr->modeAutoWrap = inIsEnabled;
	}
}// SetLineWrapEnabled


/*!
Attaches a session to this terminal, so that certain
features (such as VT100 Device Attributes) can return
their report data somewhere.  If you want to disable
this, pass a nullptr session.

(3.1)
*/
Terminal_Result
Terminal_SetListeningSession	(TerminalScreenRef	inRef,
								 SessionRef			inSession)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else
	{
		dataPtr->listeningSession = inSession;
	}
	
	return result;
}// SetListeningSession


/*!
Specifies whether or not the lines of the terminal
screen are scrolled prior to a clearing of the
visible screen area.

(2.6)
*/
void
Terminal_SetSaveLinesOnClear	(TerminalScreenRef	inRef,
								 Boolean			inClearScreenSavesLines)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr) dataPtr->saveToScrollbackOnClear = inClearScreenSavesLines;
}// SetSaveLinesOnClear


/*!
Specifies whether the given terminal’s text may be spoken
by the computer.  If you disable speech, any remaining text
in the buffer will continue to be spoken; you may want to
invoke Terminal_SpeechPause() ahead of time to force speech
to end immediately, as well.

(3.0)
*/
void
Terminal_SetSpeechEnabled	(TerminalScreenRef	inRef,
							 Boolean			inIsEnabled)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr != dataPtr)
	{
		// TEMPORARY; other speech modes are not implemented yet
		dataPtr->speech.mode = (inIsEnabled) ? kTerminal_SpeechModeSpeakAlways : kTerminal_SpeechModeSpeakNever;
	}
}// SetSpeechEnabled


/*!
Specifies the encoding of text streams read by the terminal
emulator.  This does *not* indicate the internal buffer
encoding, which might be Unicode regardless.

\retval kTerminal_ResultOK
if the encoding is set successfully

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

(4.0)
*/
Terminal_Result
Terminal_SetTextEncoding	(TerminalScreenRef		inRef,
							 CFStringEncoding		inNewEncoding)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Terminal_Result		result = kTerminal_ResultInvalidID;
	
	
	if (nullptr != dataPtr)
	{
		dataPtr->emulator.inputTextEncoding = inNewEncoding;
		result = kTerminal_ResultOK;
	}
	return result;
}// SetTextEncoding


/*!
Calls both of the internal setVisibleColumnCount() and
setVisibleRowCount() routines, but generates only a single
notification to observers of screen size changes.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of columns or rows is too small or too large

\retval kTerminal_ResultNotEnoughMemory
not currently returned because this routine does no memory
reallocation; however a future implementation might decide to
reallocate, and if such reallocation fails, this error should be
returned

(4.0)
*/
Terminal_Result
Terminal_SetVisibleScreenDimensions		(TerminalScreenRef	inRef,
										 UInt16				inNewNumberOfCharactersWide,
										 UInt16				inNewNumberOfLinesHigh)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else
	{
		(Terminal_Result)setVisibleColumnCount(dataPtr, inNewNumberOfCharactersWide);
		result = setVisibleRowCount(dataPtr, inNewNumberOfLinesHigh);
		
		changeNotifyForTerminal(dataPtr, kTerminal_ChangeScreenSize, dataPtr->selfRef/* context */);
	}
	return result;
}// SetVisibleScreenDimensions


/*!
Returns "true" only if speech is enabled for the specified
session.  Use the SpeechBusy() system call to determine if
the computer is actually speaking something at the moment.

(3.0)
*/
Boolean
Terminal_SpeechIsEnabled	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	Boolean				result = false;
	
	
	if (nullptr != dataPtr)
	{
		result = (kTerminal_SpeechModeSpeakNever != dataPtr->speech.mode);
	}
	return result;
}// SpeechIsEnabled


/*!
Immediately interrupts any speaking the computer is doing
on behalf of the specified terminal, or does nothing if
no speech is in progress.

You can resume speaking with Terminal_SpeechResume().

(3.0)
*/
void
Terminal_SpeechPause	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		TerminalSpeaker_SetPaused(dataPtr->speaker, true);
	}
}// SpeechPause


/*!
Immediately interrupts any speaking the computer is doing
on behalf of the specified terminal, or does nothing if
no speech is in progress.

You can resume speaking with Terminal_SpeechResume().

(3.0)
*/
void
Terminal_SpeechResume	(TerminalScreenRef		inRef)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		TerminalSpeaker_SetPaused(dataPtr->speaker, false);
	}
}// SpeechResume


/*!
Arranges for a callback to be invoked whenever a setting
changes for a terminal (such as the lit state of one or
more terminal LEDs).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "Terminal.h" for comments
			on what the context means for each type of
			change.

(3.0)
*/
void
Terminal_StartMonitoring	(TerminalScreenRef			inRef,
							 Terminal_Change			inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		OSStatus	error = noErr;
		
		
		// add a listener to the specified target’s listener model for the given setting change
		error = ListenerModel_AddListenerForEvent(dataPtr->changeListenerModel, inForWhatChange, inListener);
	}
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
a setting changes for a terminal (such as the lit state
of one or more terminal LEDs).

IMPORTANT:	This routine cancels the effects of a previous
			call to Terminal_StartMonitoring() - your
			parameters must match the previous start-call,
			or the stop will fail.

(3.0)
*/
void
Terminal_StopMonitoring		(TerminalScreenRef			inRef,
							 Terminal_Change			inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	My_ScreenBufferPtr	dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		// remove a listener from the specified target’s listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(dataPtr->changeListenerModel, inForWhatChange, inListener);
	}
}// StopMonitoring


/*!
Attempts to reposition the cursor by sending enough arrow key
sequences in the specified directions.  (A delta of zero in
either direction means the cursor does not move on that axis.)

This could fail, if for instance there is no session listening
to input for this terminal.

IMPORTANT:	This could fail in ways that cannot be detected by
			this function; for instance, if the user is currently
			running a process that does not interpret arrow keys
			properly.

IMPORTANT:	User input routines at the terminal level are rare,
			and generally only used to handle sequences that
			depend very directly on terminal state.  Look in the
			Session module for preferred user input routines.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultNoListeningSession
if keys cannot be sent because no session is listening

(3.1)
*/
Terminal_Result
Terminal_UserInputOffsetCursor	(TerminalScreenRef		inRef,
								 SInt16					inColumnDelta,
								 SInt16					inRowDelta)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == dataPtr->listeningSession) result = kTerminal_ResultNoListeningSession;
	else
	{
		Session_Result		keyResult = kSession_ResultOK;
		
		
		// horizontal offset
		if (inColumnDelta < 0)
		{
			for (SInt16 i = 0; i > inColumnDelta; --i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSLT);
			}
		}
		else
		{
			for (SInt16 i = 0; i < inColumnDelta; ++i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSRT);
			}
		}
		
		// vertical offset
		if (inRowDelta < 0)
		{
			for (SInt16 i = 0; i > inRowDelta; --i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSUP);
			}
		}
		else
		{
			for (SInt16 i = 0; i < inRowDelta; ++i)
			{
				keyResult = Session_UserInputKey(dataPtr->listeningSession, VSDN);
			}
		}
	}
	return result;
}// UserInputOffsetCursor


/*!
Attempts to reposition the cursor by sending enough arrow key
sequences in the specified directions.  (A delta of zero in
either direction means the cursor does not move on that axis.)

This could fail, if for instance there is no session listening
to input for this terminal.

IMPORTANT:	This could fail in ways that cannot be detected by
			this function; for instance, if the user is currently
			running a process that does not interpret arrow keys
			properly.

IMPORTANT:	User input routines at the terminal level are rare,
			and generally only used to handle sequences that
			depend very directly on terminal state.  Look in the
			Session module for preferred user input routines.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultInvalidID
if the specified screen reference is invalid

\retval kTerminal_ResultNoListeningSession
if keys cannot be sent because no session is listening

(3.1)
*/
Terminal_Result
Terminal_UserInputVTKey		(TerminalScreenRef		inRef,
							 UInt8					inVTKey,
							 Boolean				inLocalEcho)
{
	My_ScreenBufferPtr		dataPtr = getVirtualScreenData(inRef);
	Terminal_Result			result = kTerminal_ResultOK;
	
	
	if (nullptr == dataPtr) result = kTerminal_ResultInvalidID;
	else if (nullptr == dataPtr->listeningSession) result = kTerminal_ResultNoListeningSession;
	else
	{
		if ((inVTKey >= VSK0) && (inVTKey <= VSKE) && (false == dataPtr->modeApplicationKeys))
		{
			// VT SPECIFIC:
			// keypad key in numeric mode (as opposed to application key mode)
			UInt8 const		kVTNumericModeTranslation[] =
			{
				// numbers, symbols, Enter, PF1-PF4
				"0123456789,-.\015"
			};
			
			
			Session_SendData(dataPtr->listeningSession, &kVTNumericModeTranslation[inVTKey - VSK0], 1);
			if (inLocalEcho)
			{
				Terminal_EmulatorProcessData(dataPtr->selfRef, &kVTNumericModeTranslation[inVTKey - VSUP], 1);
			}
		}
		else
		{
			// VT SPECIFIC:
			// keypad key in application mode (as opposed to numeric mode),
			// or an arrow key or PF-key in either mode
			char const		kVTApplicationModeTranslation[] =
			{
				// arrows, numbers, symbols, Enter, PF1-PF4
				"ABCDpqrstuvwxylmnMPQRS"
			};
			char*		seqPtr = nullptr;
			size_t		seqLength = 0;
			
			
			if (inVTKey < VSUP)
			{
				// construct key code sequences starting from VSF10 (see VTKeys.h);
				// each sequence is defined starting with the template, below, and
				// then substituting 1 or 2 characters into the sequence template;
				// the order must exactly match what is in VTKeys.h, because of the
				// subtraction used to derive the array index
				static char			seqVT220Keys[] = "\033[  ~";
				static char const	kArrayIndex2Translation[] = "222122?2?3?3?2?3?3123425161";
				static char const	kArrayIndex3Translation[] = "134956?9?2?3?8?1?4~~~~0~8~7";
				
				
				seqVT220Keys[2] = kArrayIndex2Translation[inVTKey - VSF10];
				seqVT220Keys[3] = kArrayIndex3Translation[inVTKey - VSF10];
				seqPtr = seqVT220Keys;
				seqLength = CPP_STD::strlen(seqVT220Keys);
				if ('~' == seqPtr[3]) --seqLength; // a few of the sequences are shorter
			}
			else if (inVTKey < VSF1)
			{
				// arrows or most keypad keys
				if (dataPtr->modeANSIEnabled)
				{
					// non-VT52
					static char		seqKeypadApp[] = "\033O ";
					static char		seqKeypadCursor[] = "\033[ ";
					
					
					if (inVTKey < VSK0)
					{
						// arrows
						if (dataPtr->modeCursorKeysForApp)
						{
							seqPtr = seqKeypadApp;
							seqLength = CPP_STD::strlen(seqKeypadApp);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
						else
						{
							seqPtr = seqKeypadCursor;
							seqLength = CPP_STD::strlen(seqKeypadCursor);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
					}
					else
					{
						if (dataPtr->modeApplicationKeys)
						{
							// keypad keys have special application behavior, unless
							// the cursor keys are hit and cursor mode is enabled
							seqPtr = seqKeypadApp;
							seqLength = CPP_STD::strlen(seqKeypadApp);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
						else
						{
							// numerical mode
							seqPtr = seqKeypadCursor;
							seqLength = CPP_STD::strlen(seqKeypadCursor);
							seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						}
					}
				}
				else
				{
					// VT52
					static char		seqKeypadVT52[] = "\033? ";
					static char		seqArrowsVT52[] = "\033 ";
					
					
					if (inVTKey > VSLT)
					{
						// non-arrows
						seqPtr = seqKeypadVT52;
						seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
						seqLength = CPP_STD::strlen(seqKeypadVT52);
					}
					else
					{
						// arrows
						seqPtr = seqArrowsVT52;
						seqLength = CPP_STD::strlen(seqArrowsVT52);
						seqPtr[1] = kVTApplicationModeTranslation[inVTKey - VSUP];
					}
				}
			}
			else
			{
				// PF1 through PF4
				static char		seqFunctionKeysNormal[] = "\033O ";
				static char		seqFunctionKeysVT52[] = "\033 ";
				
				
				if (dataPtr->modeANSIEnabled)
				{
					seqPtr = seqFunctionKeysNormal;
					seqPtr[2] = kVTApplicationModeTranslation[inVTKey - VSUP];
					seqLength = CPP_STD::strlen(seqFunctionKeysNormal);
				}
				else
				{
					seqPtr = seqFunctionKeysVT52;
					seqPtr[1] = kVTApplicationModeTranslation[inVTKey - VSUP];
					seqLength = CPP_STD::strlen(seqFunctionKeysVT52);
				}
			}
			
			// finally, send the key sequence, optionally handling it immediately in the terminal
			Session_SendData(dataPtr->listeningSession, seqPtr, seqLength);
			if (inLocalEcho)
			{
				Terminal_EmulatorProcessData(dataPtr->selfRef, REINTERPRET_CAST(seqPtr, UInt8*), seqLength);
			}
		}
	}
	return result;
}// UserInputVTKey


/*!
Returns "true" only if the specified terminal has been
*told* to iconify its window.  In order for the state of
a real window to be in sync with this, a window handler
(such as the Terminal Window module) must register a
handler for the "kTerminal_ChangeWindowMinimization"
event using Terminal_StartMonitoring().

(3.0)
*/
Boolean
Terminal_WindowIsToBeMinimized	(TerminalScreenRef	inRef)
{
	Boolean						result = false;
	My_ScreenBufferConstPtr		dataPtr = getVirtualScreenData(inRef);
	
	
	if (dataPtr != nullptr)
	{
		result = dataPtr->windowMinimized;
	}
	return result;
}// WindowIsToBeMinimized


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_Emulator class instance.

(3.1)
*/
My_Emulator::
My_Emulator		(Terminal_Emulator		inPrimaryEmulation,
				 CFStringRef			inAnswerBack,
				 CFStringEncoding		inInputTextEncoding)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
primaryType(inPrimaryEmulation),
inputTextEncoding(inInputTextEncoding),
answerBackCFString(inAnswerBack),
currentState(kMy_ParserStateInitial),
stringAccumulatorState(kMy_ParserStateInitial),
stringAccumulator(),
stateRepetitions(0),
parameterEndIndex(0),
parameterValues(kMy_MaximumANSIParameters),
preCallbackSet(),
currentCallbacks(returnDataWriter(inPrimaryEmulation),
					returnStateDeterminant(inPrimaryEmulation),
					returnStateTransitionHandler(inPrimaryEmulation)),
pushedCallbacks(),
supportedVariants(),
addedXTerm(false)
{
	initializeParserStateStack(this);
}// My_Emulator default constructor


/*!
Changes the callbacks used to drive the emulator state machine,
based on the desired emulation type.

Returns true only if successful.

(4.0)
*/
Boolean
My_Emulator::
changeTo	(Terminal_Emulator		inPrimaryEmulation)
{
	My_EmulatorEchoDataProcPtr const			kNewDataWriter = returnDataWriter(inPrimaryEmulation);
	My_EmulatorStateDeterminantProcPtr const	kNewDeterminant = returnStateDeterminant(inPrimaryEmulation);
	My_EmulatorStateTransitionProcPtr const		kNewTransitionHandler = returnStateTransitionHandler(inPrimaryEmulation);
	Boolean										result = ((nullptr != kNewDataWriter) &&
															(nullptr != kNewDeterminant) &&
															(nullptr != kNewTransitionHandler));
	
	
	if (result)
	{
		this->currentCallbacks = My_Emulator::Callbacks(kNewDataWriter, kNewDeterminant, kNewTransitionHandler);
	}
	return result;
}// changeTo


/*!
Returns the entry point for determining how to echo data,
for the specified terminal type.

(4.0)
*/
My_EmulatorEchoDataProcPtr
My_Emulator::
returnDataWriter	(Terminal_Emulator		inPrimaryEmulation)
{
	My_EmulatorEchoDataProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kTerminal_EmulatorDumb:
		result = My_DumbTerminal::echoData;
		break;
	
	case kTerminal_EmulatorVT100:
	case kTerminal_EmulatorXTermOriginal:
	case kTerminal_EmulatorXTermColor:
	case kTerminal_EmulatorXTerm256Color:
	case kTerminal_EmulatorANSIBBS:
	case kTerminal_EmulatorANSISCO:
	case kTerminal_EmulatorVT102:
	case kTerminal_EmulatorVT220:
	case kTerminal_EmulatorVT320:
	case kTerminal_EmulatorVT420:
	default:
		// Echoing data with correct translation, etc. is not trivial and
		// it is not recommended that most emulators try to do this any
		// differently than the default emulator.
		result = My_DefaultEmulator::echoData;
		break;
	}
	return result;
}// returnDataWriter


/*!
Returns the entry point for determining emulator state,
for the specified terminal type.

(4.0)
*/
My_EmulatorStateDeterminantProcPtr
My_Emulator::
returnStateDeterminant		(Terminal_Emulator		inPrimaryEmulation)
{
	My_EmulatorStateDeterminantProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kTerminal_EmulatorVT100:
	case kTerminal_EmulatorXTermOriginal: // TEMPORARY
	case kTerminal_EmulatorXTermColor: // TEMPORARY
	case kTerminal_EmulatorXTerm256Color: // TEMPORARY
	case kTerminal_EmulatorANSIBBS: // TEMPORARY
	case kTerminal_EmulatorANSISCO: // TEMPORARY
		result = My_VT100::stateDeterminant;
		break;
	
	case kTerminal_EmulatorVT102:
		result = My_VT102::stateDeterminant;
		break;
	
	case kTerminal_EmulatorVT220:
	case kTerminal_EmulatorVT320: // TEMPORARY
	case kTerminal_EmulatorVT420: // TEMPORARY
		result = My_VT220::stateDeterminant;
		break;
	
	case kTerminal_EmulatorDumb:
		result = My_DumbTerminal::stateDeterminant;
		break;
	
	default:
		// ???
		result = My_DefaultEmulator::stateDeterminant;
		break;
	}
	return result;
}// returnStateDeterminant


/*!
Returns the entry point for moving between emulator states,
for the specified terminal type.

(4.0)
*/
My_EmulatorStateTransitionProcPtr
My_Emulator::
returnStateTransitionHandler	(Terminal_Emulator		inPrimaryEmulation)
{
	My_EmulatorStateTransitionProcPtr		result = nullptr;
	
	
	switch (inPrimaryEmulation)
	{
	case kTerminal_EmulatorVT100:
	case kTerminal_EmulatorXTermOriginal: // TEMPORARY
	case kTerminal_EmulatorXTermColor: // TEMPORARY
	case kTerminal_EmulatorXTerm256Color: // TEMPORARY
	case kTerminal_EmulatorANSIBBS: // TEMPORARY
	case kTerminal_EmulatorANSISCO: // TEMPORARY
		result = My_VT100::stateTransition;
		break;
	
	case kTerminal_EmulatorVT102:
		result = My_VT102::stateTransition;
		break;
	
	case kTerminal_EmulatorVT220:
	case kTerminal_EmulatorVT320: // TEMPORARY
	case kTerminal_EmulatorVT420: // TEMPORARY
		result = My_VT220::stateTransition;
		break;
	
	case kTerminal_EmulatorDumb:
		result = My_DumbTerminal::stateTransition;
		break;
	
	default:
		// ???
		result = My_DefaultEmulator::stateTransition;
		break;
	}
	return result;
}// returnStateTransitionHandler


/*!
Returns true only if this terminal emulator has been configured
to support the specified variant.

Currently, the only expected tags are those identifying special
terminal features, e.g. "kPreferences_TagXTerm256ColorsEnabled",
"kPreferences_TagVT100FixLineWrappingBug".

(4.0)
*/
Boolean
My_Emulator::
supportsVariant		(Preferences_Tag	inTag)
{
	Boolean		result = (this->supportedVariants.end() != this->supportedVariants.find(inTag));
	
	
	return result;
}// supportsVariant


/*!
Initializes a My_Emulator::Callbacks class instance with
null pointers.

(4.0)
*/
My_Emulator::Callbacks::
Callbacks ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
dataWriter(nullptr),
stateDeterminant(nullptr),
transitionHandler(nullptr)
{
}// My_Emulator::Callbacks default constructor


/*!
Initializes a My_Emulator::Callbacks class instance.

(4.0)
*/
My_Emulator::Callbacks::
Callbacks	(My_EmulatorEchoDataProcPtr				inDataWriter,
			 My_EmulatorStateDeterminantProcPtr		inStateDeterminant,
			 My_EmulatorStateTransitionProcPtr		inTransitionHandler)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
dataWriter(inDataWriter),
stateDeterminant(inStateDeterminant),
transitionHandler(inTransitionHandler)
{
}// My_Emulator::Callbacks 2-argument constructor


/*!
Returns true only if the callbacks are all defined.

(4.0)
*/
Boolean
My_Emulator::Callbacks::
exist ()
const
{
	return ((nullptr != dataWriter) && (nullptr != stateDeterminant) && (nullptr != transitionHandler));
}// My_Emulator::Callbacks::exist


/*!
Constructor.  See Terminal_NewScreen().

Throws a Terminal_Result if any problems occur.

(3.0)
*/
My_ScreenBuffer::
My_ScreenBuffer	(Preferences_ContextRef		inTerminalConfig,
				 Preferences_ContextRef		inTranslationConfig)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
configuration(Preferences_NewCloneContext(inTerminalConfig, true/* detach */)),
emulator(returnEmulator(inTerminalConfig), returnAnswerBackMessage(inTerminalConfig), returnTextEncoding(inTranslationConfig)),
listeningSession(nullptr),
speaker(nullptr),
windowTitleCFString(),
iconTitleCFString(),
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard, kConstantsRegistry_ListenerModelDescriptorTerminalChanges)),
preferenceMonitor(ListenerModel_NewStandardListener(preferenceChanged, this/* context */)),
scrollbackBufferCachedSize(0),
scrollbackBuffer(),
screenBuffer(),
bytesToEcho(),
echoErrorCount(0),
translationErrorCount(0),
errorCountTotal(0),
tabSettings(),
captureStream(StreamCapture_New(returnLineEndings())),
printingStream(nullptr),
printingFile(),
printingModes(0),
bellDisabled(false),
cursorVisible(true),
reverseVideo(false),
windowMinimized(false),
vtG0(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOff),
vtG1(kMy_CharacterSetVT100UnitedStates, kMy_CharacterROMNormal, kMy_GraphicsModeOn),
visibleBoundary(0, 0, returnScreenColumns(inTerminalConfig) - 1, returnScreenRows(inTerminalConfig) - 1),
customScrollingRegion(0, 0), // reset below...
// text elements - not initialized
litLEDs(kMy_LEDBitsAllOff),
mayNeedToSaveToScrollback(false),
saveToScrollbackOnClear(true),
reportOnlyOnRequest(false),
wrapPending(false),
modeANSIEnabled(true),
modeApplicationKeys(false),
modeAutoWrap(false),
modeCursorKeysForApp(false),
modeInsertNotReplace(false),
modeNewLineOption(false),
modeOriginRedefined(false),
originRegionPtr(&visibleBoundary.rows),
// speech elements - not initialized
current(*this),
// previous elements - not initialized
selfRef(REINTERPRET_CAST(this, TerminalScreenRef))
// TEMPORARY: initialize other members here...
{
	this->text.visibleScreen.numberOfColumnsAllocated = Terminal_ReturnAllocatedColumnCount(); // always allocate max columns
	
	this->current.cursorX = 0; // initialized because moveCursor() depends on prior values...
	this->current.cursorY = 0; // initialized because moveCursor() depends on prior values...
	
	// now “append” the desired number of main screen lines, which will have
	// the effect of allocating a screen buffer of the right size
	unless (screenInsertNewLines(this, returnScreenRows(inTerminalConfig)))
	{
		throw kTerminal_ResultNotEnoughMemory;
	}
	assert(!this->screenBuffer.empty());
	
	// arbitrary pre-allocation to avoid dynamic allocation;
	// the destructor can log the actual size in debug mode,
	// which will help to tune this for optimal performance
	this->bytesToEcho.reserve(256);
	
	try
	{
		// it is important to make the list a multiple of the tab stop distance;
		// see tabStopInitialize() to see why this is the case
		this->tabSettings.resize(this->text.visibleScreen.numberOfColumnsAllocated +
									(this->text.visibleScreen.numberOfColumnsAllocated % kMy_TabStop));
		tabStopInitialize(this);
	}
	catch (std::bad_alloc)
 	{
		throw kTerminal_ResultNotEnoughMemory;
	}
	
	this->current.characterSetInfoPtr = &this->vtG0; // by definition, G0 is active initially
	setScrollbackSize(this, returnScrollbackRows(inTerminalConfig));
	this->text.scrollback.enabled = (this->text.scrollback.enabled && returnForceSave(inTerminalConfig));
	this->text.visibleScreen.numberOfColumnsPermitted = returnScreenColumns(inTerminalConfig);
	this->current.cursorAttributes = kNoTerminalTextAttributes;
	this->current.drawingAttributes = kNoTerminalTextAttributes;
	this->previous.drawingAttributes = kInvalidTerminalTextAttributes; // initially no saved attribute
	
	// speech setup
	this->speech.mode = kTerminal_SpeechModeSpeakNever;
	
	if (returnXTerm256(inTerminalConfig))
	{
		this->emulator.supportedVariants.insert(kPreferences_TagXTerm256ColorsEnabled);
		if (false == this->emulator.addedXTerm)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(My_DefaultEmulator::echoData,
																			My_XTerm::stateDeterminant,
																			My_XTerm::stateTransition));
			this->emulator.addedXTerm = true;
		}
	}
	if (returnXTermWindowAlteration(inTerminalConfig))
	{
		this->emulator.supportedVariants.insert(kPreferences_TagXTermWindowAlterationEnabled);
		if (false == this->emulator.addedXTerm)
		{
			this->emulator.preCallbackSet.insert(this->emulator.preCallbackSet.begin(),
													My_Emulator::Callbacks(My_DefaultEmulator::echoData,
																			My_XTerm::stateDeterminant,
																			My_XTerm::stateTransition));
			this->emulator.addedXTerm = true;
		}
	}
	
	// IMPORTANT: Within constructors, calls to routines expecting a *self reference* should be
	//            *last*; otherwise, there’s no telling whether or not the data that the routine
	//            requires will have been properly initialized yet.
	
	moveCursor(this, 0, 0);
	
	this->customScrollingRegion = this->visibleBoundary.rows; // initially...
	assertScrollingRegion(this);
	
	this->speaker = TerminalSpeaker_New(REINTERPRET_CAST(this, TerminalScreenRef));
	
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextStartMonitoring(this->configuration, this->preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
	}
}// My_ScreenBuffer 1-argument constructor


/*!
Destructor.  See Terminal_DisposeScreen().

(3.1)
*/
My_ScreenBuffer::
~My_ScreenBuffer ()
{
	if (DebugInterface_LogsTerminalState())
	{
		// write some statistics on the terminal when it is finished,
		// for things like memory profiling; by knowing how large
		// dynamic structures typically grow, the pre-allocation
		// arbitrations can be more appropriate
		Console_WriteLine("statistics for disposed screen buffer object:");
		Console_WriteValue("final echo buffer size", this->bytesToEcho.capacity());
	}
	
	this->printingModes = 0; // clear so that printingEnd() will clean up
	printingEnd();
	StreamCapture_Release(&this->captureStream);
	(Preferences_Result)Preferences_ContextStopMonitoring(this->configuration, this->preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
	ListenerModel_ReleaseListener(&this->preferenceMonitor);
	Preferences_ReleaseContext(&this->configuration);
	TerminalSpeaker_Dispose(&this->speaker);
	ListenerModel_Dispose(&this->changeListenerModel);
}// My_ScreenBuffer destructor


/*!
Invoked whenever a monitored preference value is changed for
a particular screen (see the constructor for the calls that
arrange to monitor preferences).  This routine responds by
updating internal caches.

(4.0)
*/
void
My_ScreenBuffer::
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inPreferencesContext,
					 void*					inMyScreenBufferPtr)
{
	// WARNING: The context is only defined for the preferences monitored in a
	// context-specific way through Preferences_ContextStartMonitoring() calls.
	// Otherwise, the data type of the input is "Preferences_ChangeContext*".
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	My_ScreenBufferPtr			ptr = REINTERPRET_CAST(inMyScreenBufferPtr, My_ScreenBufferPtr);
	
	
	if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
	{
		// batch mode; multiple things have changed; this should basically mirror
		// what is copied by Terminal_ReturnConfiguration()
		Terminal_SetVisibleScreenDimensions(ptr->selfRef, ptr->returnScreenColumns(prefsContext),
											ptr->returnScreenRows(prefsContext));
		setScrollbackSize(ptr, ptr->returnScrollbackRows(prefsContext));
	}
	else
	{
		// ???
	}
}// preferenceChanged


/*!
Conditionally terminates a printout for the specified terminal,
closing the temporary file used for print data.  If requested,
all the text cached since the last printingReset() is then sent
to the printer (displaying a preview dialog first).  The cache
file is deleted, so this is your only chance to print.

This has no effect if ANY printing mode is still active.  The
typical approach is to first disable the desired printing mode
bit (in the "printingModes" member) when a mode ends, and to
then call this routine.  If the disabled mode was the last
active mode, then there will be no active mode, and printing
will occur automatically.

(4.0)
*/
void
My_ScreenBuffer::
printingEnd		(Boolean	inSendRemainderToPrinter)
{
	if ((nullptr != this->printingStream) && (0 == this->printingModes))
	{
		FSClose(StreamCapture_ReturnReferenceNumber(this->printingStream));
		StreamCapture_End(this->printingStream);
		StreamCapture_Release(&this->printingStream);
		
		// to print the outstanding text, determine the location of the temporary file
		// and then print from that file
		if (inSendRemainderToPrinter)
		{
			CFRetainRelease		fileURL(CFURLCreateFromFSRef(kCFAllocatorDefault, &this->printingFile),
										true/* is retained */);
			
			
			if (false == fileURL.exists())
			{
				Console_Warning(Console_WriteLine, "unable to find the URL of the temporary file that was used to print");
			}
			else
			{
				// print the captured text using the print dialog
				CFStringRef			jobTitle = nullptr;
				UIStrings_Result	stringResult = UIStrings_Copy(kUIStrings_TerminalPrintFromTerminalJobTitle,
																	jobTitle);
				
				
				if (nullptr != jobTitle)
				{
					TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(this->listeningSession);
					PrintTerminal_JobRef	printJob = PrintTerminal_NewJobFromFile
														(CFUtilities_URLCast(fileURL.returnCFTypeRef()),
															TerminalWindow_ReturnViewWithFocus(terminalWindow),
															jobTitle);
					
					
					if (nullptr != printJob)
					{
						(PrintTerminal_Result)PrintTerminal_JobSendToPrinter
												(printJob, TerminalWindow_ReturnWindow(terminalWindow));
						PrintTerminal_ReleaseJob(&printJob);
					}
					CFRelease(jobTitle), jobTitle = nullptr;
				}
			}
		}
		
		(OSStatus)FSDeleteObject(&this->printingFile);
	}
}// printingEnd


/*!
Conditionally starts printing by opening a new temporary file
for streaming.  If ANY printing mode is currently active (such
as auto-print or print controller), this has no effect.

Call this whenever you start a new printing mode.

To terminate all active prints, use printingEnd() (also called
automatically by the destructor of the class).

(4.0)
*/
void
My_ScreenBuffer::
printingReset ()
{
	if (PrintTerminal_IsPrintingSupported() && (0 == this->printingModes))
	{
		SInt16		openedFile = 0;
		
		
		// create a new, empty file that is open for write
		openedFile = FileUtilities_OpenTemporaryFile(this->printingFile);
		this->printingStream = StreamCapture_New(kSession_LineEndingLF);
		(Boolean)StreamCapture_Begin(this->printingStream, openedFile);
		// TEMPORARY - if this fails, need to report to the user somehow
	}
}// printingReset


/*!
Reads "kPreferences_TagTerminalAnswerBackMessage" from a
Preferences context, and returns either that value or the
default answer-back for returnEmulatorType().

(3.1)
*/
CFStringRef
My_ScreenBuffer::
returnAnswerBackMessage		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				result = nullptr;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalAnswerBackMessage,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = Terminal_EmulatorReturnDefaultName(returnEmulator(inTerminalConfig));
	
	return result;
}// returnAnswerBackMessage


/*!
Reads "kPreferences_TagTerminalEmulatorType" from a Preferences
context, and returns either that value or the default VT100
type if none was found.

(3.1)
*/
Terminal_Emulator
My_ScreenBuffer::
returnEmulator	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Terminal_Emulator		result = kTerminal_EmulatorVT100;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalEmulatorType,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = kTerminal_EmulatorVT100;
	
	return result;
}// returnEmulator


/*!
Reads "kPreferences_TagTerminalClearSavesLines" from a
Preferences context, and returns either that value or the
default of true if none was found.

(3.1)
*/
Boolean
My_ScreenBuffer::
returnForceSave		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalClearSavesLines,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnForceSave


/*!
Reads "kPreferences_TagCaptureFileLineEndings" from a Preferences
context, and returns either that value or the default value if
none was found.

(4.0)
*/
Session_LineEnding
My_ScreenBuffer::
returnLineEndings ()
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Session_LineEnding		result = kSession_LineEndingLF; // arbitrary default
	
	
	// TEMPORARY - perhaps this routine should take a specific preferences context
	prefsResult = Preferences_GetData(kPreferences_TagCaptureFileLineEndings,
										sizeof(result), &result);
	return result;
}// returnLineEndings


/*!
Reads "kPreferences_TagTerminalScreenColumns" from a Preferences
context, and returns either that value or the default value of
80 if none was found.

(3.1)
*/
UInt16
My_ScreenBuffer::
returnScreenColumns		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					result = 80; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenColumns,
												sizeof(result), &result);
	return result;
}// returnScreenColumns


/*!
Reads "kPreferences_TagTerminalScreenRows" from a Preferences
context, and returns either that value or the default value of
24 if none was found.

(3.1)
*/
UInt16
My_ScreenBuffer::
returnScreenRows	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					result = 24; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenRows,
												sizeof(result), &result);
	return result;
}// returnScreenRows


/*!
Reads all scrollback-related settings from a Preferences context,
and returns an appropriate value for scrollback size.

(3.1)
*/
UInt32
My_ScreenBuffer::
returnScrollbackRows	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Terminal_ScrollbackType		scrollbackType = kTerminal_ScrollbackTypeFixed;
	UInt32						result = 200; // arbitrary default
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenScrollbackType,
												sizeof(scrollbackType), &scrollbackType);
	if (kPreferences_ResultOK == prefsResult)
	{
		if (kTerminal_ScrollbackTypeDisabled == scrollbackType) result = 0;
		else if (kTerminal_ScrollbackTypeUnlimited == scrollbackType) result = USHRT_MAX; // TEMPORARY
		else if (kTerminal_ScrollbackTypeDistributed == scrollbackType) ; // UNIMPLEMENTED
	}
	
	if (kTerminal_ScrollbackTypeFixed == scrollbackType)
	{
		prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagTerminalScreenScrollbackRows,
													sizeof(result), &result);
	}
	
	return result;
}// returnScrollbackRows


/*!
Reads "kPreferences_TagTextEncodingIANAName" or
"kPreferences_TagTextEncodingID" from a Preferences context,
and returns either that value or the default of UTF-8 if none
was found.

(4.0)
*/
CFStringEncoding
My_ScreenBuffer::
returnTextEncoding		(Preferences_ContextRef		inTranslationConfig)
{
	CFStringEncoding	result = kCFStringEncodingUTF8;
	
	
	result = TextTranslation_ContextReturnEncoding(inTranslationConfig, result/* default */);
	return result;
}// returnTextEncoding


/*!
Reads "kPreferences_TagXTerm256ColorsEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
returnXTerm256	(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTerm256ColorsEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnXTerm256


/*!
Reads "kPreferences_TagXTermWindowAlterationEnabled" from a
Preferences context, and returns either that value or the
default of true if none was found.

(4.0)
*/
Boolean
My_ScreenBuffer::
returnXTermWindowAlteration		(Preferences_ContextRef		inTerminalConfig)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	
	prefsResult = Preferences_ContextGetData(inTerminalConfig, kPreferences_TagXTermWindowAlterationEnabled,
												sizeof(result), &result);
	if (kPreferences_ResultOK != prefsResult) result = true; // arbitrary
	
	return result;
}// returnXTermWindowAlteration


/*!
Creates a new screen buffer line.

(3.1)
*/
My_ScreenBufferLine::
My_ScreenBufferLine ()
:
textVectorBegin(REINTERPRET_CAST(std::malloc(kMy_NumberOfCharactersPerLineMaximum * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kMy_NumberOfCharactersPerLineMaximum),
textVectorSize(textVectorEnd - textVectorBegin),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kMy_NumberOfCharactersPerLineMaximum,
					kMy_NumberOfCharactersPerLineMaximum/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				true/* is retained */),
attributeVector(kMy_NumberOfCharactersPerLineMaximum),
globalAttributes(kTerminalTextAttributesAllOff)
{
	assert(textCFString.exists());
	structureInitialize();
}// My_ScreenBufferLine constructor


/*!
Creates a new screen buffer line by copying an
existing one.

(3.1)
*/
My_ScreenBufferLine::
My_ScreenBufferLine	(My_ScreenBufferLine const&		inCopy)
:
textVectorBegin(REINTERPRET_CAST(std::malloc(kMy_NumberOfCharactersPerLineMaximum * sizeof(UniChar)), UniChar*)),
textVectorEnd(textVectorBegin + kMy_NumberOfCharactersPerLineMaximum),
textVectorSize(textVectorEnd - textVectorBegin),
textCFString(CFStringCreateMutableWithExternalCharactersNoCopy
				(kCFAllocatorDefault, textVectorBegin, kMy_NumberOfCharactersPerLineMaximum,
					kMy_NumberOfCharactersPerLineMaximum/* capacity */, kCFAllocatorMalloc/* reallocator/deallocator */),
				true/* is retained */),
attributeVector(inCopy.attributeVector),
globalAttributes(inCopy.globalAttributes)
{
	assert(textCFString.exists());
	
	// it is important for the local CFMutableStringRef to have its own
	// internal buffer, which is why it was allocated separately and
	// is being copied directly below
	std::copy(inCopy.textVectorBegin, inCopy.textVectorEnd, this->textVectorBegin);
}// My_ScreenBufferLine copy constructor


/*!
Disposes of a screen buffer line.

(3.1)
*/
My_ScreenBufferLine::
~My_ScreenBufferLine ()
{
}// My_ScreenBufferLine destructor


/*!
Reinitializes a screen buffer line from a
different one.

(3.1)
*/
My_ScreenBufferLine&
My_ScreenBufferLine::
operator =	(My_ScreenBufferLine const&		inCopy)
{
	if (this != &inCopy)
	{
		this->attributeVector = inCopy.attributeVector;
		this->globalAttributes = inCopy.globalAttributes;
		
		// since the CFMutableStringRef uses the internal buffer, overwriting
		// the buffer contents will implicitly update the CFStringRef as well;
		// also, since the lines are all the same size, there is no need to
		// copy the start/end and size information
		std::copy(inCopy.textVectorBegin, inCopy.textVectorEnd, this->textVectorBegin);
	}
	return *this;
}// My_ScreenBufferLine::operator =


/*!
Returns true only if the specified line is considered
equal to this line.

(3.1)
*/
bool
My_ScreenBufferLine::
operator == (My_ScreenBufferLine const&  inLine)
const
{
	return (&inLine == this);
}// My_ScreenBufferLine::operator ==


/*!
Resets a line to its initial state (clearing all text and
removing attribute bits).

(3.1)
*/
void
My_ScreenBufferLine::
structureInitialize ()
{
	std::fill(textVectorBegin, textVectorEnd, ' ');
	std::fill(attributeVector.begin(), attributeVector.end(), kTerminalTextAttributesAllOff);
	globalAttributes = kTerminalTextAttributesAllOff;
}// My_ScreenBufferLine::structureInitialize


/*!
Translates the specified buffer into Unicode (from the input
text encoding of the terminal), and echoes it to the screen.
Also pipes the data to additional targets, such as spoken voice,
if so enabled in the terminal.

This implementation expects to be used with terminals whose
state determinants will pre-filter the data stream to not have
control characters, etc.  It will ignore text that it cannot
translate correctly!

Returns the number of characters successfully echoed.

(4.0)
*/
UInt32
My_DefaultEmulator::
echoData	(My_ScreenBufferPtr		inDataPtr,
			 UInt8 const*			inBuffer,
			 UInt32					inLength)
{
	UInt32		result = inLength;
	
	
	if (inLength > 0)
	{
		CFIndex				bytesRequired = 0;
		CFRetainRelease		bufferAsCFString(TextTranslation_PersistentCFStringCreate
												(kCFAllocatorDefault, inBuffer, inLength, inDataPtr->emulator.inputTextEncoding,
													false/* is external representation */, bytesRequired, inLength/* maximum trim/repeat */),
												true/* is retained */);
		
		
		if (false == bufferAsCFString.exists())
		{
			// TEMPORARY: this should probably be handled better
			++(inDataPtr->translationErrorCount);
			result = 0;
		}
		else
		{
			// send the data wherever it needs to go
			echoCFString(inDataPtr, bufferAsCFString.returnCFStringRef());
			
			// speech implementation; TEMPORARY: handled here because the spoken text must
			// have access to a post-translation string, free of any meta-characters that
			// might have been in the original text stream; but text typically arrives too
			// irregularly and too quickly to allow a useful sentence to be spoken, so it
			// may be necessary to capture lines and speak them via a timer in a more
			// orderly fashion
			unless (TerminalSpeaker_IsMuted(inDataPtr->speaker) || TerminalSpeaker_IsGloballyMuted())
			{
				Boolean		doSpeak = false;
				
				
				switch (inDataPtr->speech.mode)
				{
				case kTerminal_SpeechModeSpeakAlways:
					doSpeak = true;
					break;
				
				case kTerminal_SpeechModeSpeakWhenActive:
					//doSpeak = IsWindowHilited(the screen window);
					break;
				
				case kTerminal_SpeechModeSpeakWhenInactive:
					//doSpeak = !IsWindowHilited(the screen window);
					break;
				
				case kTerminal_SpeechModeSpeakNever:
				default:
					doSpeak = false;
					break;
				}
				
				if (doSpeak)
				{
					TerminalSpeaker_Result		speakerResult = kTerminalSpeaker_ResultOK;
					
					
					// TEMPORARY - queue this, to keep asynchronous speech from jumbling or interrupting multi-line text
					// (and to improve performance as a result)
					speakerResult = TerminalSpeaker_SynthesizeSpeechFromCFString(inDataPtr->speaker, bufferAsCFString.returnCFStringRef());
					if (kTerminalSpeaker_ResultSpeechSynthesisTryAgain == speakerResult)
					{
						// error...
					}
				}
			}
		}
	}
	return result;
}// My_DefaultEmulator::echoData


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
default states based on the characters of the given buffer.

This routine can be used to automatically handle a huge
variety of state transitions for your specific terminal
type.  It understands a lot of common encoding schemes,
so you need only define your emulator-specific states to
have generic values (like "kMy_ParserStateSeenESCA") and
let your emulator-specific state determinant use this
routine as a fallback for MOST input values!

When this routine - based on the data stream alone - sees
a pattern it recognizes, it chooses the next generic
state that matches the pattern.  If your emulator-specific
state matches that generic state, you will see this in
your code as a transition to your emulator-specific state
and can therefore react in an emulator-specific way within
your state transition callback.

IMPORTANT:	Even if this routine can handle a sequence
			applicable to your terminal type, it will do
			so entirely on a raw data basis.  This will
			sometimes not be enough.  Ensure this routine
			is the *fallback*, not the primary, in your
			emulator-specific state determinant so that
			you can override how any state is handled.

(3.1)
*/
UInt32
My_DefaultEmulator::
stateDeterminant	(My_EmulatorPtr			UNUSED_ARGUMENT(inEmulatorPtr),
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				UNUSED_ARGUMENT(outHandled))
{
	assert(inLength > 0);
	UInt8 const				kTriggerChar = *inBuffer; // for convenience; usually only first character matters
	// if no specific next state seems appropriate, the character will either
	// be printed (if possible) or be re-evaluated from the initial state
	My_ParserState const	kDefaultNextState = kMy_ParserStateAccumulateForEcho;
	UInt32					result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// by default, the state does not change
	inNowOutNext.second = inNowOutNext.first;
	
	switch (inNowOutNext.first)
	{
	case kMy_ParserStateInitial:
	case kMy_ParserStateAccumulateForEcho:
		inNowOutNext.second = kDefaultNextState;
		result = 0; // do not absorb the unknown
		break;
	
	case kMy_ParserStateSeenESC:
		switch (kTriggerChar)
		{
		case '[':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracket;
			break;
		
		case ']':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket;
			break;
		
		case '(':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParen;
			break;
		
		case ')':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParen;
			break;
		
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCB;
			break;
		
		case 'C':
			inNowOutNext.second = kMy_ParserStateSeenESCC;
			break;
		
		case 'c':
			inNowOutNext.second = kMy_ParserStateSeenESCc;
			break;
		
		case 'D':
			inNowOutNext.second = kMy_ParserStateSeenESCD;
			break;
		
		case 'E':
			inNowOutNext.second = kMy_ParserStateSeenESCE;
			break;
		
		case 'F':
			inNowOutNext.second = kMy_ParserStateSeenESCF;
			break;
		
		case 'G':
			inNowOutNext.second = kMy_ParserStateSeenESCG;
			break;
		
		case 'H':
			inNowOutNext.second = kMy_ParserStateSeenESCH;
			break;
		
		case 'I':
			inNowOutNext.second = kMy_ParserStateSeenESCI;
			break;
		
		case 'J':
			inNowOutNext.second = kMy_ParserStateSeenESCJ;
			break;
		
		case 'K':
			inNowOutNext.second = kMy_ParserStateSeenESCK;
			break;
		
		case 'M':
			inNowOutNext.second = kMy_ParserStateSeenESCM;
			break;
		
		case 'Y':
			inNowOutNext.second = kMy_ParserStateSeenESCY;
			break;
		
		case 'Z':
			inNowOutNext.second = kMy_ParserStateSeenESCZ;
			break;
		
		case '7':
			inNowOutNext.second = kMy_ParserStateSeenESC7;
			break;
		
		case '8':
			inNowOutNext.second = kMy_ParserStateSeenESC8;
			break;
		
		case '#':
			inNowOutNext.second = kMy_ParserStateSeenESCPound;
			break;
		
		case '=':
			inNowOutNext.second = kMy_ParserStateSeenESCEquals;
			break;
		
		case '<':
			inNowOutNext.second = kMy_ParserStateSeenESCLessThan;
			break;
		
		case '>':
			inNowOutNext.second = kMy_ParserStateSeenESCGreaterThan;
			break;
		
		case '\\':
			inNowOutNext.second = kMy_ParserStateSeenESCBackslash;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape", kTriggerChar);
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCLeftSqBracket:
		// immediately begin parsing parameters, but do not absorb these characters
		inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParams;
		result = 0; // do not absorb the unknown
		break;
	
	case kMy_ParserStateSeenESCLeftSqBracketParams:
		switch (kTriggerChar)
		{
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsB;
			break;
		
		case 'c':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsc;
			break;
		
		case 'C':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsC;
			break;
		
		case 'd':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsd;
			break;
		
		case 'D':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsD;
			break;
		
		case 'E':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsE;
			break;
		
		case 'f':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsf;
			break;
		
		case 'F':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsF;
			break;
		
		case 'g':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsg;
			break;
		
		case 'G':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsG;
			break;
		
		case 'h':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsh;
			break;
		
		case 'H':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsH;
			break;
		
		case 'i':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsi;
			break;
		
		case 'I':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsI;
			break;
		
		case 'J':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsJ;
			break;
		
		case 'K':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsK;
			break;
		
		case 'l':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsl;
			break;
		
		case 'L':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsL;
			break;
		
		case 'm':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsm;
			break;
		
		case 'M':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsM;
			break;
		
		case 'n':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsn;
			break;
		
		case 'P':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsP;
			break;
		
		case 'q':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsq;
			break;
		
		case 'r':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsr;
			break;
		
		case 's':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamss;
			break;
		
		case 'u':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsu;
			break;
		
		case 'x':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsx;
			break;
		
		case 'Z':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsZ;
			break;
		
		case '@':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsAt;
			break;
		
		case '`':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsBackquote;
			break;
		
		default:
			// continue looking for parameters until a known terminator is found
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParams;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCLeftParen:
		switch (kTriggerChar)
		{
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParenA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParenB;
			break;
		
		case '0':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParen0;
			break;
		
		case '1':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParen1;
			break;
		
		case '2':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftParen2;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-(", kTriggerChar);
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightParen:
		switch (kTriggerChar)
		{
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParenA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParenB;
			break;
		
		case '0':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParen0;
			break;
		
		case '1':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParen1;
			break;
		
		case '2':
			inNowOutNext.second = kMy_ParserStateSeenESCRightParen2;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-)", kTriggerChar);
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket:
		switch (kTriggerChar)
		{
		case '0':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket0;
			break;
		
		case '1':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1;
			break;
		
		case '2':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket2;
			break;
		
		case '3':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket3;
			break;
		
		case '4':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket4;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket0:
		switch (kTriggerChar)
		{
		case ';':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket0Semi;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket1:
		switch (kTriggerChar)
		{
		case ';':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket1Semi;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket2:
		switch (kTriggerChar)
		{
		case ';':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket2Semi;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket3:
		switch (kTriggerChar)
		{
		case ';':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket3Semi;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCRightSqBracket4:
		switch (kTriggerChar)
		{
		case ';':
			inNowOutNext.second = kMy_ParserStateSeenESCRightSqBracket4Semi;
			break;
		
		default:
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	case kMy_ParserStateSeenESCPound:
		switch (kTriggerChar)
		{
		case '3':
			inNowOutNext.second = kMy_ParserStateSeenESCPound3;
			break;
		
		case '4':
			inNowOutNext.second = kMy_ParserStateSeenESCPound4;
			break;
		
		case '5':
			inNowOutNext.second = kMy_ParserStateSeenESCPound5;
			break;
		
		case '6':
			inNowOutNext.second = kMy_ParserStateSeenESCPound6;
			break;
		
		case '8':
			inNowOutNext.second = kMy_ParserStateSeenESCPound8;
			break;
		
		default:
			//Console_WriteValueCharacter("WARNING, terminal received unknown character following escape-#", kTriggerChar);
			inNowOutNext.second = kDefaultNextState;
			result = 0; // do not absorb the unknown
			break;
		}
		break;
	
	default:
		// unknown state!
		//Console_WriteValueCharacter("WARNING, terminal entered unknown state; choosing a valid state based on character", kTriggerChar);
		inNowOutNext.second = kDefaultNextState;
		result = 0; // do not absorb the unknown
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< default in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     default proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        default bases this at least on character", *inBuffer);
	
	return result;
}// My_DefaultEmulator::stateDeterminant


/*!
Every "My_EmulatorStateTransitionProcPtr" callback should
default to the result of invoking this routine with its
arguments.

(3.1)
*/
UInt32
My_DefaultEmulator::
stateTransition		(My_ScreenBufferPtr			UNUSED_ARGUMENT(inDataPtr),
					 UInt8 const*				UNUSED_ARGUMENT(inBuffer),
					 UInt32						UNUSED_ARGUMENT(inLength),
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					UNUSED_ARGUMENT(outHandled))
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< standard handler transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     standard handler transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	//Console_WriteValueFourChars("WARNING, no known actions associated with new terminal state", inOldNew.second);
	// the trigger character would also be skipped in this case
	
	return result;
}// My_DefaultEmulator::stateTransition


/*!
Translates the specified buffer into Unicode (from the input
text encoding of the terminal), and echoes a representation of
those bytes to the stream.

By definition, a dumb terminal can render any byte, as nothing
is “special” or invisible.

Returns the number of characters successfully echoed.

(4.0)
*/
UInt32
My_DumbTerminal::
echoData	(My_ScreenBufferPtr		inDataPtr,
			 UInt8 const*			inBuffer,
			 UInt32					inLength)
{
	UInt32		result = inLength;
	
	
	if (inLength > 0)
	{
		CFIndex				bytesRequired = 0;
		CFRetainRelease		bufferAsCFString(TextTranslation_PersistentCFStringCreate
												(kCFAllocatorDefault, inBuffer, inLength, inDataPtr->emulator.inputTextEncoding,
													false/* is external representation */, bytesRequired, inLength/* maximum trim/repeat */),
												true/* is retained */);
		CFRetainRelease		humanReadableCFString(CFStringCreateMutable(kCFAllocatorDefault, 0/* maximum length or 0 for no limit */),
													true/* is retained */);
		UniChar*			deletedBufferPtr = nullptr;
		
		
		if (false == bufferAsCFString.exists())
		{
			// TEMPORARY: this should probably be handled better
			++(inDataPtr->translationErrorCount);
			
			// echo a single byte so that it will be skipped next time
			CFStringAppendFormat(humanReadableCFString.returnCFMutableStringRef(), nullptr/* format options */,
									CFSTR("<!%u>"), STATIC_CAST(*inBuffer, unsigned int));
			result = 1;
		}
		else
		{
			CFIndex const		kLength = CFStringGetLength(bufferAsCFString.returnCFStringRef());
			UniChar const*		bufferIterator = nullptr;
			UniChar const*		bufferPtr = CFStringGetCharactersPtr(bufferAsCFString.returnCFStringRef());
			CFIndex				characterIndex = 0;
			
			
			if (nullptr == bufferPtr)
			{
				// not ideal, but if the internal buffer is not a Unicode array,
				// it must be copied before it can be interpreted that way
				deletedBufferPtr = new UniChar[kLength];
				CFStringGetCharacters(bufferAsCFString.returnCFStringRef(), CFRangeMake(0, kLength), deletedBufferPtr);
				bufferPtr = deletedBufferPtr;
			}
			
			// create a printable interpretation of every character
			bufferIterator = bufferPtr;
			while (characterIndex < kLength)
			{
				if (gDumbTerminalRenderings().end() != gDumbTerminalRenderings().find(*bufferIterator))
				{
					// print whatever was registered as the proper rendering
					CFStringAppend(humanReadableCFString.returnCFMutableStringRef(),
									gDumbTerminalRenderings()[*bufferIterator].returnCFStringRef());
				}
				else
				{
					// print the numerical value, e.g. 200 becomes "<200>"
					CFStringAppendFormat(humanReadableCFString.returnCFMutableStringRef(), nullptr/* format options */,
											CFSTR("<%u>"), STATIC_CAST(*bufferIterator, unsigned int));
				}
				++bufferIterator;
				++characterIndex;
			}
		}
		
		// send the data wherever it needs to go
		echoCFString(inDataPtr, humanReadableCFString.returnCFStringRef());
		
		if (nullptr != deletedBufferPtr)
		{
			delete [] deletedBufferPtr, deletedBufferPtr = nullptr;
		}
	}
	return result;
}// My_DumbTerminal::echoData


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
dumb-terminal states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_DumbTerminal::
stateDeterminant	(My_EmulatorPtr			UNUSED_ARGUMENT(inEmulatorPtr),
					 UInt8 const*			UNUSED_ARGUMENT(inBuffer),
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				UNUSED_ARGUMENT(outInterrupt),
					 Boolean&				UNUSED_ARGUMENT(outHandled))
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// dumb terminals echo everything
	inNowOutNext.second = kMy_ParserStateAccumulateForEcho;
	result = 0; // do not absorb, it will be handled by the emulator loop
	
	// debug
	//Console_WriteValueFourChars("    <<< dumb terminal in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     dumb terminal proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        dumb terminal bases this at least on character", *inBuffer);
	
	return result;
}// My_DumbTerminal::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds
to dumb terminal state changes.

(3.1)
*/
UInt32
My_DumbTerminal::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						inLength,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< dumb terminal transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     dumb terminal transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	result = invokeEmulatorStateTransitionProc
				(My_DefaultEmulator::stateTransition, inDataPtr, inBuffer, inLength,
					inOldNew, outHandled);
	
	return result;
}// My_DumbTerminal::stateTransition


/*!
Handles the VT100 'DECALN' sequence.  See the VT100
manual for complete details.

(3.0)
*/
void
My_VT100::
alignmentDisplay	(My_ScreenBufferPtr		inDataPtr)
{
	// first clear the buffer, saving it to scrollback if appropriate
	bufferEraseVisibleScreenWithUpdate(inDataPtr);
	
	// now fill the lines with letter-E characters; technically this
	// also will reset all attributes and this may not be part of the
	// VT100 specification (but it seems reasonable to get rid of any
	// special colors or oversized text when doing screen alignment)
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferLineFill(inDataPtr, *lineIterator, 'E', kTerminalTextAttributesAllOff, true/* change line global attributes to match */);
		}
	}
	
	// update the display - UNIMPLEMENTED
}// My_VT100::alignmentDisplay


/*!
Switches a VT100-compatible terminal to ANSI mode, which means
it no longer accepts VT52 sequences.

(3.1)
*/
void
My_VT100::
ansiMode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = true;
	if (inDataPtr->emulator.pushedCallbacks.exist())
	{
		inDataPtr->emulator.currentCallbacks = inDataPtr->emulator.pushedCallbacks;
		inDataPtr->emulator.pushedCallbacks = My_Emulator::Callbacks();
	}
	initializeParserStateStack(&inDataPtr->emulator);
}// My_VT100::ansiMode


/*!
Handles the VT100 'CUB' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorBackward	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.parameterValues[0] < 1)
	{
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX - inDataPtr->emulator.parameterValues[0];
		
		
		if (newValue < 0) newValue = 0;
		moveCursorX(inDataPtr, newValue);
	}
}// My_VT100::cursorBackward


/*!
Handles the VT100 'CUD' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorDown	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.parameterValues[0] < 1)
	{
		if (inDataPtr->current.cursorY < inDataPtr->originRegionPtr->lastRow) moveCursorDown(inDataPtr);
		else moveCursorDownToEdge(inDataPtr);
	}
	else
	{
		My_ScreenRowIndex	newValue = inDataPtr->current.cursorY +
										inDataPtr->emulator.parameterValues[0];
		
		
		if (newValue > inDataPtr->originRegionPtr->lastRow)
		{
			newValue = inDataPtr->originRegionPtr->lastRow;
		}
		// NOTE: the check below may not be necessary
		if (newValue >= inDataPtr->screenBuffer.size())
		{
			newValue = inDataPtr->screenBuffer.size() - 1;
		}
		moveCursorY(inDataPtr, newValue);
	}
}// My_VT100::cursorDown


/*!
Handles the VT100 'CUF' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorForward	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->current.returnNumberOfColumnsPermitted() - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.parameterValues[0] < 1)
	{
		if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
		else moveCursorRightToEdge(inDataPtr);
	}
	else
	{
		SInt16		newValue = inDataPtr->current.cursorX + inDataPtr->emulator.parameterValues[0];
		
		
		if (newValue > rightLimit) newValue = rightLimit;
		moveCursorX(inDataPtr, newValue);
	}
}// My_VT100::cursorForward


/*!
Handles the VT100 'CUU' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
cursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	// the default value is 1 if there are no parameters
	if (inDataPtr->emulator.parameterValues[0] < 1)
	{
		if (inDataPtr->current.cursorY > inDataPtr->originRegionPtr->firstRow)
		{
			moveCursorUp(inDataPtr);
		}
		else
		{
			moveCursorUpToEdge(inDataPtr);
		}
	}
	else
	{
		SInt16				newValue = inDataPtr->current.cursorY - inDataPtr->emulator.parameterValues[0];
		My_ScreenRowIndex	rowIndex = 0;
		
		
		if (newValue < 0)
		{
			newValue = 0;
		}
		
		rowIndex = STATIC_CAST(newValue, My_ScreenRowIndex);
		if (rowIndex < inDataPtr->originRegionPtr->firstRow)
		{
			rowIndex = inDataPtr->originRegionPtr->firstRow;
		}
		moveCursorY(inDataPtr, rowIndex);
	}
}// My_VT100::cursorUp


/*!
Handles the VT100 'DA' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
deviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// support GPO (graphics processor option) and AVO (advanced video option)
		Session_SendData(session, "\033[?1;6c", 7);
	}
}// My_VT100::deviceAttributes


/*!
Handles the VT100 'DSR' sequence.  See the VT100
manual for complete details.

(3.0)
*/
void
My_VT100::
deviceStatusReport		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.parameterValues[0])
	{
	case 5:
		// report status using a DSR control sequence
		{
			SessionRef	session = returnListeningSession(inDataPtr);
			
			
			if (nullptr != session)
			{
				Session_SendData(session, "\033[0n"/* 0 means “ready, no malfunctions detected”; see VT100 manual on DSR for details */,
									4/* length of the string */);
			}
		}
		break;
	
	case 6:
		// report active (cursor) position using a CPR control sequence
		{
			SInt16				reportedCursorX = inDataPtr->current.cursorX;
			My_ScreenRowIndex	reportedCursorY = inDataPtr->current.cursorY;
			
			
			// determine imminent cursor position
			if (reportedCursorX >= inDataPtr->text.visibleScreen.numberOfColumnsPermitted)
			{
				// auto-wrap pending
				reportedCursorX = 0;
				++reportedCursorY;
			}
			if (reportedCursorY >= inDataPtr->screenBuffer.size())
			{
				// scroll pending (because of auto-wrap)
				reportedCursorY = inDataPtr->screenBuffer.size() - 1;
			}
			
			// report relative to the scroll region if in origin mode
			reportedCursorY -= inDataPtr->originRegionPtr->firstRow;
			
			// the reported numbers are one-based, not zero-based
			++reportedCursorX;
			++reportedCursorY;
			
			// send response
			{
				SessionRef	session = returnListeningSession(inDataPtr);
				
				
				if (nullptr != session)
				{
					std::ostringstream		reportBuffer;
					
					
					reportBuffer
					<< "\033["
					<< reportedCursorY
					<< ";"
					<< reportedCursorX
					<< "R"
					;
					std::string		reportBufferString = reportBuffer.str();
					Session_SendData(session, reportBufferString.c_str(), reportBufferString.size());
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::deviceStatusReport


/*!
Handles the VT100 'ED' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
eraseInDisplay		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.parameterValues[0])
	{
	case -1: // -1 means no parameter was given; the default value is 0
	case 0:
		bufferEraseFromCursorToEnd(inDataPtr);
		break;
	
	case 1:
		bufferEraseFromHomeToCursor(inDataPtr);
		break;
	
	case 2:
		bufferEraseVisibleScreenWithUpdate(inDataPtr);
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::eraseInDisplay


/*!
Handles the VT100 'EL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
eraseInLine		(My_ScreenBufferPtr		inDataPtr)
{
	switch (inDataPtr->emulator.parameterValues[0])
	{
	case -1: // -1 means no parameter was given; the default value is 0
	case 0:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr);
		break;
	
	case 1:
		bufferEraseFromLineBeginToCursorColumn(inDataPtr);
		break;
	
	case 2:
		bufferEraseLineWithUpdate(inDataPtr, -1/* line number, or negative for cursor line */);
		break;
	
	default:
		// ???
		break;
	}
}// My_VT100::eraseInLine


/*!
Handles the VT100 'DECLL' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
loadLEDs	(My_ScreenBufferPtr		inDataPtr)
{
	register SInt16		i = 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.parameterEndIndex; ++i)
	{
		if (inDataPtr->emulator.parameterValues[i] == -1)
		{
			// no value; default is “all off”
			highlightLED(inDataPtr, 0);
		}
		else if (inDataPtr->emulator.parameterValues[i] == 137)
		{
			// could decide to emulate the “ludicrous repeat rate” bug
			// of the VT100, here; in combination with key click, this
			// should basically make each key press play a musical note :)
		}
		else
		{
			// a parameter of 1 means “LED 1 on”, 2 means “LED 2 on”,
			// 3 means “LED 3 on”, 4 means “LED 4 on”; 0 means “all off”
			highlightLED(inDataPtr, inDataPtr->emulator.parameterValues[i]/* LED # */);
		}
	}
}// My_VT100::loadLEDs


/*!
Handles the VT100 'SM' sequence (if "inIsModeEnabled" is true)
or the VT100 'RM' sequence (if "inIsModeEnabled" is false).
See the VT100 manual for complete details.

(2.6)
*/
void
My_VT100::
modeSetReset	(My_ScreenBufferPtr		inDataPtr,
				 Boolean				inIsModeEnabled)
{
	switch (inDataPtr->emulator.parameterValues[0])
	{
	case -2: // DEC-private control sequence
		{
			register SInt16		i = 0;
			Boolean				emulateDECOMBug = false;
			
			
			for (i = 1/* skip the -2 parameter */; i <= inDataPtr->emulator.parameterEndIndex; ++i)
			{
				switch (inDataPtr->emulator.parameterValues[i])
				{
				case 1:
					// DECCKM (cursor-key mode)
					inDataPtr->modeCursorKeysForApp = inIsModeEnabled;
					break;
				
				case 2:
					// DECANM (ANSI/VT52 mode); this is only possible to reset, not set
					// (the set is accomplished in a different way)
					if (false == inIsModeEnabled)
					{
						My_VT100::vt52Mode(inDataPtr);
					}
					break;
				
				case 3:
					// DECCOLM (80/132 column switch)
					{
						(Boolean)Commands_ExecuteByIDUsingEvent((inIsModeEnabled)
																? kCommandLargeScreen
																: kCommandSmallScreen);
					}
					break;
				
				case 5:
					// DECSCNM (screen mode)
					inDataPtr->reverseVideo = inIsModeEnabled;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeVideoMode, inDataPtr->selfRef);
					break;
				
				case 6: // DECOM (origin mode)
					{
					#if 0
						// the original VT100 has a bug where as soon as DECOM is set or cleared,
						// ALL subsequent mode changes are ignored; since technically MacTelnet
						// is emulating a *real* VT100 and not the ideal manual, you could enable
						// this flag to replicate VT100 behavior instead of what the manual says
						emulateDECOMBug = true;
					#endif
						
						inDataPtr->modeOriginRedefined = inIsModeEnabled;
						if (inIsModeEnabled)
						{
							// restrict cursor movements to the defined margins, and
							// ensure that reported cursor row/column use these offsets
							inDataPtr->originRegionPtr = &inDataPtr->customScrollingRegion;
						}
						else
						{
							// no restrictions
							inDataPtr->originRegionPtr = &inDataPtr->visibleBoundary.rows;
						}
						
						// home the cursor, but relative to the new top margin
						// (automatically restricted by cursor movement routines)
						moveCursor(inDataPtr, 0, 0);
					}
					break;
				
				case 7: // DECAWM (auto-wrap mode)
					inDataPtr->modeAutoWrap = inIsModeEnabled;
					if (false == inIsModeEnabled) inDataPtr->wrapPending = false;
					break;
				
				case 4: // DECSCLM (if enabled, scrolling is smooth at 6 lines per second; otherwise, instantaneous)
				case 8: // DECARM (auto-repeating)
				case 9: // DECINLM (interlace)
				case 0: // error, ignored
				case -1: // no value given (set and reset do not have default values)
				default:
					// ???
					break;
				}
				
				// see above, where this is defined, for more information on this break
				if (emulateDECOMBug)
				{
					break;
				}
			}
		}
		break;
	
	case 4: // insert/replace character writing mode
		inDataPtr->modeInsertNotReplace = inIsModeEnabled;
		break;
	
	case 20: // LNM (line feed / newline mode)
		inDataPtr->modeNewLineOption = inIsModeEnabled;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeLineFeedNewLineMode, inDataPtr->selfRef);
		break;
	
	default:
		break;
	}
}// My_VT100::modeSetReset


/*!
Scans the specified buffer for parameter-like data
(e.g. ;0;05;21;2) and saves all the parameters it
finds.  The number of characters “used” is returned
(you can use this to offset your original buffer
pointer appropriately).

Typically this is done immediately after a VT100
control sequence inducer (CSI, a.k.a. ESC-[) is
received.  That way, any terminal sequence which
has a CSI in it (i.e. anything that needs parameters)
will have all defined parameters available to it.

(3.1)
*/
UInt32
My_VT100::
readCSIParameters	(My_ScreenBufferPtr		inDataPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength)
{
	UInt32			result = 0;
	UInt8 const*	bufferIterator = inBuffer;
	Boolean			done = false;
	SInt16&			terminalEndIndexRef = inDataPtr->emulator.parameterEndIndex;
	
	
	for (; (false == done) && (bufferIterator != (inBuffer + inLength)); ++bufferIterator, ++result)
	{
		//Console_WriteValueCharacter("scan", *bufferIterator);
		switch (*bufferIterator)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			// parse numeric parameter
			{
				// rename this incredibly long expression, since it’s needed a lot here!
				SInt16&		valueRef = inDataPtr->emulator.parameterValues[terminalEndIndexRef];
				
				
				if (valueRef < 0) valueRef = 0;
				valueRef *= 10;
				valueRef += *bufferIterator - '0';
			}
			break;
		
		case ';':
			// parameter separator
			if (terminalEndIndexRef < kMy_MaximumANSIParameters) ++terminalEndIndexRef;
			break;
		
		case '?':
			// manual says the FIRST character must be this to enter
			// DEC-private parameter mode; otherwise, ignore (stop here)
			if (0 == result)
			{
				// DEC-private parameters
				inDataPtr->emulator.parameterValues[terminalEndIndexRef++] = -2;
			}
			else
			{
				done = true;
			}
			break;
		
		default:
			// not part of a parameter sequence
			done = true;
			break;
		}
	}
	
	// ignore final character if the loop was broken prematurely
	if (done) --result;
	
	// debug - write results to console
	//Console_WriteValue("parameters found", terminalEndIndexRef + 1);
	//for (register SInt16 i = 0; i <= terminalEndIndexRef; ++i)
	//{
	//	Console_WriteValue("found post-CSI parameter", inDataPtr->emulator.parameterValues[i]);
	//}
	
	return result;
}// My_VT100::readCSIParameters


/*!
Handles the VT100 'DECREQT' sequence.  See the VT100
manual for complete details.

(3.1)
*/
void
My_VT100::
reportTerminalParameters	(My_ScreenBufferPtr		inDataPtr)
{
	UInt16 const	kRequestType = (inDataPtr->emulator.parameterValues[0] != -1)
									? inDataPtr->emulator.parameterValues[0]
									: 0/* default is a request, unsolicited reports mode */;
	// these variable names are copied directly from the VT100 manual
	// in the DECREQT section, to make it crystal clear what each one is;
	// see the VT100 manual for full descriptions of their values
	UInt16			sol = 0;		// solicitation; what to do
	UInt16			par = 0;		// parity
	UInt16			nbits = 0;		// bits per character
	UInt16			xspeed = 0;		// transmission speed
	UInt16			rspeed = 0;		// reception speed
	UInt16			clkmul = 0;		// clock multiplier
	UInt16			flags = 0;		// four switch values from SETUP-B
	
	
	// determine the type of response based on the request;
	// also remember this mode for future reports
	if (kRequestType == 1)
	{
		inDataPtr->reportOnlyOnRequest = true;
	}
	else
	{
		inDataPtr->reportOnlyOnRequest = false;
	}
	
	// set the report parameters appropriately
	sol = (inDataPtr->reportOnlyOnRequest) ? 3 : 2;
	par = 1; // 1 = no parity is set
	// TEMPORARY: bits setting is fixed, should be fluid
	nbits = 1; // 1 = 8 bits per character, 2 = 7 bits
	// NOTE: These speeds are a hack...technically, this is set in
	// the terminal control code in "Local.cp", but even that is a hack!
	xspeed = 112; // 112 = 9600 baud
	rspeed = 112; // 112 = 9600 baud
	clkmul = 1; // set to 1 if the bit rate multiplier is 16
	flags = 0; // TEMPORARY: these switches are not stored anywhere
	
	// send response
	{
		SessionRef	session = returnListeningSession(inDataPtr);
		
		
		if (nullptr != session)
		{
			std::ostringstream	reportBuffer;
			
			
			reportBuffer
			<< "\033["
			<< sol << ";"
			<< par << ";"
			<< nbits << ";"
			<< xspeed << ";"
			<< rspeed << ";"
			<< clkmul << ";"
			<< flags
			<< "x"
			;
			std::string		reportBufferString = reportBuffer.str();
			Session_SendData(session, reportBufferString.c_str(), reportBufferString.size());
		}
	}
}// My_VT100::reportTerminalParameters


/*!
Handles the VT100 'DECSTBM' sequence.  See the VT100
manual for complete details.

(3.0)
*/
inline void
My_VT100::
setTopAndBottomMargins		(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->emulator.parameterValues[0] < 0)
	{
		// no top parameter given; default is top of screen
		inDataPtr->customScrollingRegion.firstRow = inDataPtr->visibleBoundary.rows.firstRow;
	}
	else
	{
		// parameter is the line number of the new first line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		if (0 == inDataPtr->emulator.parameterValues[0]) inDataPtr->emulator.parameterValues[0] = 1;
		inDataPtr->customScrollingRegion.firstRow = inDataPtr->emulator.parameterValues[0] - 1;
	}
	
	if (inDataPtr->emulator.parameterValues[1] < 0)
	{
		// no bottom parameter given; default is bottom of screen
		inDataPtr->customScrollingRegion.lastRow = inDataPtr->visibleBoundary.rows.lastRow;
	}
	else
	{
		// parameter is the line number of the new last line of the scrolling region; the
		// input is 1-based but internally it is a zero-based array index, so subtract one
		UInt16		newValue = 0;
		
		
		if (0 == inDataPtr->emulator.parameterValues[1]) inDataPtr->emulator.parameterValues[1] = 1;
		
		newValue = inDataPtr->emulator.parameterValues[1] - 1;
		if (newValue > inDataPtr->visibleBoundary.rows.lastRow)
		{
			Console_Warning(Console_WriteLine, "emulator was given a scrolling region bottom row that is too large; truncating");
			newValue = inDataPtr->visibleBoundary.rows.lastRow;
		}
		inDataPtr->customScrollingRegion.lastRow = newValue;
	}
	
	// VT100 requires that the range be 2 lines minimum
	if (inDataPtr->customScrollingRegion.firstRow >= inDataPtr->customScrollingRegion.lastRow)
	{
		Console_Warning(Console_WriteLine, "emulator was given a scrolling region bottom row that is less than the top; resetting");
		inDataPtr->customScrollingRegion.lastRow = inDataPtr->customScrollingRegion.firstRow + 1;
		if (inDataPtr->customScrollingRegion.lastRow > inDataPtr->visibleBoundary.rows.lastRow)
		{
			inDataPtr->customScrollingRegion.lastRow = inDataPtr->visibleBoundary.rows.lastRow;
		}
		if (inDataPtr->customScrollingRegion.firstRow >= inDataPtr->customScrollingRegion.lastRow)
		{
			inDataPtr->customScrollingRegion.firstRow = inDataPtr->customScrollingRegion.lastRow - 1;
		}
		assertScrollingRegion(inDataPtr);
	}
	
	// home the cursor, but relative to any current top margin
	// (this limit is enforced by moveCursorY())
	moveCursor(inDataPtr, 0, 0);
	
	//Console_WriteValuePair("scrolling region rows are now", inDataPtr->inDataPtr->customScrollingRegion.firstRow,
	//							inDataPtr->customScrollingRegion.lastRow); // debug
	//Console_WriteValuePair("origin mode enable flag and cursor row", inDataPtr->modeOriginRedefined, inDataPtr->current.cursorY); // debug
}// My_VT100::setTopAndBottomMargins


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT100-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT100::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	Boolean		isControlCharacter = true;
	
	
	// see if the given character is a control character; if so,
	// it will not contribute to the current sequence and may
	// even reset the parser
	switch (*inBuffer)
	{
	case '\000':
		// ignore this character for the purposes of sequencing
		inNowOutNext.second = inNowOutNext.first;
		break;
	
	case '\005':
		// send answer-back message
		inNowOutNext.second = kStateControlENQ;
		break;
	
	case '\007':
		// audio event
		inNowOutNext.second = kStateControlBEL;
		break;
	
	case '\010':
		// backspace
		inNowOutNext.second = kStateControlBS;
		break;
	
	case '\011':
		// horizontal tab
		inNowOutNext.second = kStateControlHT;
		break;
	
	case '\012':
	case '\013':
	case '\014':
		// line feed
		// all of these are interpreted the same for VT100
		inNowOutNext.second = kStateControlLFVTFF;
		break;
	
	case '\015':
		// carriage return
		inNowOutNext.second = kStateControlCR;
		break;
	
	case '\016':
		// shift out
		inNowOutNext.second = kStateControlSO;
		break;
	
	case '\017':
		// shift in
		inNowOutNext.second = kStateControlSI;
		break;
	
	case '\021':
		// resume transmission
		inNowOutNext.second = kStateControlXON;
		break;
	
	case '\023':
		// suspend transmission
		inNowOutNext.second = kStateControlXOFF;
		break;
	
	case '\030':
	case '\032':
		// abort control sequence (if any) and emit error character
		inNowOutNext.second = kStateControlCANSUB;
		break;
	
	case '\177': // DEL
		// ignore this character for the purposes of sequencing
		inNowOutNext.second = inNowOutNext.first;
		break;
	
	default:
		isControlCharacter = false;
		break;
	}
	
	// all control characters are interrupt-class: they should
	// cause actions, but not “corrupt” any partially completed
	// sequence that may have come before them, i.e. the caller
	// should revert to the state preceding the control character
	if (isControlCharacter)
	{
		outInterrupt = true;
	}
	
	// if no interrupt has occurred, use the current state and
	// the available data to determine the next logical state
	if (false == isControlCharacter)
	{
		switch (inNowOutNext.first)
		{
		case kStateCSI:
			inNowOutNext.second = kStateCSIParamScan;
			result = 0; // absorb nothing
			break;
		
		case kStateCSIParamScan:
			// look for a terminating character (anything not legal in a parameter)
			switch (*inBuffer)
			{
			case 'A':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsA;
				break;
			
			case 'B':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsB;
				break;
			
			case 'c':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsc;
				break;
			
			case 'C':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsC;
				break;
			
			case 'D':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsD;
				break;
			
			case 'f':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsf;
				break;
			
			case 'g':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsg;
				break;
			
			case 'h':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsh;
				break;
			
			case 'H':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsH;
				break;
			
			case 'J':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsJ;
				break;
			
			case 'K':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsK;
				break;
			
			case 'l':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsl;
				break;
			
			case 'm':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsm;
				break;
			
			case 'n':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsn;
				break;
			
			case 'q':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsq;
				break;
			
			case 'r':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsr;
				break;
			
			case 's':
				// TEMPORARY - ANSI compatibility hack, should be a separate terminal type
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamss;
				break;
			
			case 'u':
				// TEMPORARY - ANSI compatibility hack, should be a separate terminal type
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsu;
				break;
			
			case 'x':
				inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsx;
				break;
			
			// continue scanning as long as characters are LEGAL in a parameter sequence
			// (the set below should be consistent with My_VT100::readCSIParameters())
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case ';':
			case '?':
				inNowOutNext.second = kStateCSIParamScan;
				result = 0; // do not absorb this character
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// this is unexpected data; choose a new state
				if (DebugInterface_LogsTerminalInputChar())
				{
					Console_Warning(Console_WriteValueCharacter, "VT100 in CSI parameter mode did not expect character", *inBuffer);
				}
				outHandled = false;
				break;
			}
			break;
		
		default:
			if (*inBuffer == '\033')
			{
				// this character forces any partial sequence that came before it to be ignored
				inNowOutNext.second = kMy_ParserStateSeenESC;
			}
			else
			{
				outHandled = false;
			}
			break;
		}
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_DefaultEmulator::stateDeterminant, inEmulatorPtr, inBuffer, inLength,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT100 proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        VT100 bases this at least on character", *inBuffer);
	
	return result;
}// My_VT100::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds
to VT100-specific (ANSI mode) state changes.

See also My_VT100::VT52::stateTransition(), which should
ONLY be called when the emulator is in VT52 mode.

IMPORTANT:	This emulator should ONLY be called when the
			emulator is in ANSI mode, *or* if the VT52
			emulator has been called first (which,
			incidentally, defaults to calling this one).
			This code filters out VT52 codes, however
			some codes overlap and do DIFFERENT THINGS
			in ANSI mode.

(3.1)
*/
UInt32
My_VT100::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						inLength,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 ANSI transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     VT100 ANSI transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateControlENQ:
		// send answer-back message
		// UNIMPLEMENTED
		//Console_WriteLine("request to send answer-back message; unimplemented");
		break;
	
	case kStateControlBEL:
		// audio event
		unless (inDataPtr->bellDisabled)
		{
			static CFAbsoluteTime	gLastBeep = 0;
			CFAbsoluteTime			now = CFAbsoluteTimeGetCurrent();
			
			
			// do not allow repeating beeps in a short period of time to
			// take over the terminal; automatically ignore events that
			// occur at an arbitrarily high frequency
			if ((now - gLastBeep) > 2/* arbitrary; in seconds */)
			{
				changeNotifyForTerminal(inDataPtr, kTerminal_ChangeAudioEvent, inDataPtr->selfRef/* context */);
			}
			gLastBeep = now;
		}
		break;
	
	case kStateControlBS:
		// backspace
		if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
		else moveCursorLeftToEdge(inDataPtr); // do not extend past margin
		break;
	
	case kStateControlHT:
		// horizontal tab
		moveCursorRightToNextTabStop(inDataPtr);
		StreamCapture_WriteUTF8Data(inDataPtr->captureStream, inBuffer, 1);
		break;
	
	case kStateControlLFVTFF:
		// line feed
		// all of these are interpreted the same for VT100;
		// if LNM was received, this is a regular line feed,
		// otherwise it is actually a new-line operation
		moveCursorDownOrScroll(inDataPtr);
	#if 0
		if (inDataPtr->modeNewLineOption)
		{
			moveCursorLeftToEdge(inDataPtr);
		}
	#endif
		break;
	
	case kStateControlCR:
		// carriage return
		moveCursorLeftToEdge(inDataPtr);
	#if 0
		if (inDataPtr->modeNewLineOption)
		{
			moveCursorDownOrScroll(inDataPtr);
		}
	#endif
		StreamCapture_WriteUTF8Data(inDataPtr->captureStream, inBuffer, 1);
		break;
	
	case kStateControlSO:
		// shift out
		inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG1;
		if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
		{
			// set attribute
			STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
		}
		else
		{
			// clear attribute
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
		}
		break;
	
	case kStateControlSI:
		// shift in
		inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
		if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
		{
			// set attribute
			STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
		}
		else
		{
			// clear attribute
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
		}
		break;
	
	case kStateControlXON:
		// resume transmission
		// UNIMPLEMENTED
		//Console_WriteLine("request to resume transmission; unimplemented");
		break;
	
	case kStateControlXOFF:
		// suspend transmission
		// UNIMPLEMENTED
		//Console_WriteLine("request to suspend transmission (except for XON/XOFF); unimplemented");
		break;
	
	case kStateControlCANSUB:
		// abort control sequence (if any) and emit error character
		echoCFString(inDataPtr, CFSTR("?"));
		break;
	
	case kStateCSI:
		// each new CSI means a blank slate for parameters
		clearEscapeSequenceParameters(inDataPtr);
		break;
	
	case kStateCSIParamScan:
		// continue to accumulate parameters (this could require multiple passes)
		result += My_VT100::readCSIParameters(inDataPtr, inBuffer, inLength);
		break;
	
	case kStateCUB:
		My_VT100::cursorBackward(inDataPtr);
		break;
	
	case kStateCUD:
		My_VT100::cursorDown(inDataPtr);
		break;
	
	case kStateCUF:
		My_VT100::cursorForward(inDataPtr);
		break;
	
	case kStateCUU:
		My_VT100::cursorUp(inDataPtr);
		break;
	
	case kStateCUP:
	case kStateHVP:
		// absolute cursor positioning
		{
			// both 0 and 1 are considered first row/column
			SInt16				newX = (0 == inDataPtr->emulator.parameterValues[1])
										? 0
										: (inDataPtr->emulator.parameterValues[1] != -1)
											? inDataPtr->emulator.parameterValues[1] - 1
											: 0/* default is home */;
			My_ScreenRowIndex	newY = (0 == inDataPtr->emulator.parameterValues[0])
										? 0
										: (inDataPtr->emulator.parameterValues[0] != -1)
											? inDataPtr->emulator.parameterValues[0] - 1
											: 0/* default is home */;
			
			
			// offset according to the origin mode
			newY += inDataPtr->originRegionPtr->firstRow;
			
			// the new values are not checked for violation of constraints
			// because constraints (including current origin mode) are
			// automatically enforced by moveCursor...() routines
			moveCursor(inDataPtr, newX, newY);
		}
		break;
	
	case kStateDA:
		My_VT100::deviceAttributes(inDataPtr);
		break;
	
	case kStateDECALN:
		My_VT100::alignmentDisplay(inDataPtr);
		break;
	
	case kStateDECANM:
		break;
	
	case kStateDECARM:
		break;
	
	case kStateDECAWM:
		break;
	
	case kStateDECCKM:
		break;
	
	case kStateDECCOLM:
		break;
	
	case kStateDECDHLT:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleHeightTop/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECDHLB:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleHeightBottom/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECDWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// check line-global attributes; if this line was single-width,
			// clear out the entire right half of the line
			eraseRightHalfOfLine(inDataPtr, *cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, kTerminalTextAttributeDoubleWidth/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
			
			// VT100 manual specifies that a cursor in the right half of
			// the normal screen width should be stuck at the half-way point
			moveCursorLeftToHalf(inDataPtr);
		}
		break;
	
	case kStateDECID:
		My_VT100::deviceAttributes(inDataPtr);
		break;
	
	case kStateDECKPAM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) of VT100
		break;
	
	case kStateDECKPNM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) of VT100
		break;
	
	case kStateDECLL:
		My_VT100::loadLEDs(inDataPtr);
		break;
	
	case kStateDECOM:
		break;
	
	case kStateANSIRC:
	case kStateDECRC:
		cursorRestore(inDataPtr);
		break;
	
	case kStateDECREPTPARM:
		// a parameter report has been received
		// IGNORED
		break;
	
	case kStateDECREQTPARM:
		// a request for parameters has been made; send a response
		My_VT100::reportTerminalParameters(inDataPtr);
		break;
	
	case kStateANSISC:
	case kStateDECSC:
		cursorSave(inDataPtr);
		break;
	
	case kStateDECSTBM:
		My_VT100::setTopAndBottomMargins(inDataPtr);
		break;
	
	case kStateDECSWL:
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			// set attributes global to the line, which means that there is
			// no option for any character to lack the attribute on this line
			changeLineGlobalAttributes(inDataPtr, *cursorLineIterator, 0/* set */,
										kMaskTerminalTextAttributeDoubleText/* clear */);
		}
		break;
	
	case kStateDECTST:
		break;
	
	case kStateDSR:
		My_VT100::deviceStatusReport(inDataPtr);
		break;
	
	case kStateED:
		My_VT100::eraseInDisplay(inDataPtr);
		break;
	
	case kStateEL:
		My_VT100::eraseInLine(inDataPtr);
		break;
	
	case kStateHTS:
		// set tab at current position
		inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;
		break;
	
	//case kStateHVP:
	//see above
	
	case kStateIND:
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateNEL:
		moveCursorLeftToEdge(inDataPtr);
		moveCursorDownOrScroll(inDataPtr);
		break;
	
	case kStateRI:
		moveCursorUpOrScroll(inDataPtr);
		break;
	
	case kStateRIS:
		resetTerminal(inDataPtr);
		break;
	
	case kStateRM:
		My_VT100::modeSetReset(inDataPtr, false/* set */);
		break;
	
	case kStateSCSG0UK:
	case kStateSCSG1UK:
		{
			// U.K. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1UK == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0ASCII:
	case kStateSCSG1ASCII:
		{
			// U.S. character set, normal ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ASCII == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0SG:
	case kStateSCSG1SG:
		{
			// normal ROM, graphics mode
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1SG == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMNormal;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // set graphics attribute
		}
		break;
	
	case kStateSCSG0ACRStd:
	case kStateSCSG1ACRStd:
		{
			// alternate ROM, no graphics
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ACRStd == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOff;
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
		}
		break;
	
	case kStateSCSG0ACRSG:
	case kStateSCSG1ACRSG:
		{
			// normal ROM, graphics mode
			My_CharacterSetInfoPtr		targetCharacterSetPtr = &inDataPtr->vtG0;
			
			
			if (kStateSCSG1ACRSG == inOldNew.second) targetCharacterSetPtr = &inDataPtr->vtG1;
			targetCharacterSetPtr->source = kMy_CharacterROMAlternate;
			targetCharacterSetPtr->graphicsMode = kMy_GraphicsModeOn;
			STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // set graphics attribute
		}
		break;
	
	case kStateSGR:
		// ANSI colors and other character attributes
		{
			SInt16		i = 0;
			
			
			while (i <= inDataPtr->emulator.parameterEndIndex)
			{
				SInt16		p = 0;
				
				
				if (inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex] < 0)
				{
					inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex] = 0;
				}
				
				p = inDataPtr->emulator.parameterValues[i];
				
				// Note that a real VT100 will only understand 0-7 here.
				// Other values are basically recognized because they are
				// compatible with VT100 and are very often used (ANSI
				// colors in particular).
				if (p == 0) STYLE_REMOVE(inDataPtr->current.drawingAttributes, kAllStyleOrColorTerminalTextAttributes); // all style bits off
				else if (p < 9) STYLE_ADD(inDataPtr->current.drawingAttributes, styleOfVTParameter(p)); // set attribute
				else if (p == 10) { /* set normal font - unsupported */ }
				else if (p == 11) { /* set alternate font - unsupported */ }
				else if (p == 12) { /* set alternate font, shifting by 128 - unsupported */ }
				else if (p == 22) STYLE_REMOVE(inDataPtr->current.drawingAttributes, styleOfVTParameter(1)); // clear bold (oddball - 22, not 21)
				else if ((p > 22) && (p < 29)) STYLE_REMOVE(inDataPtr->current.drawingAttributes, styleOfVTParameter(p - 20)); // clear attribute
				else
				{
					if ((p >= 30) && (p < 38))
					{
						STYLE_SET_FOREGROUND_INDEX(inDataPtr->current.drawingAttributes, p - 30);
					}
					else if ((p >= 40) && (p < 48))
					{
						STYLE_SET_BACKGROUND_INDEX(inDataPtr->current.drawingAttributes, p - 40);
					}
					else if ((38 == p) || (48 == p))
					{
						if (inDataPtr->emulator.supportsVariant(kPreferences_TagXTerm256ColorsEnabled))
						{
							//Console_WriteLine("request to set one of 256 background or foreground colors");
							if (2 != (inDataPtr->emulator.parameterEndIndex - i))
							{
								if (DebugInterface_LogsTerminalInputChar())
								{
									Console_Warning(Console_WriteLine, "expected exactly 3 parameters for 256-color-mode request");
								}
							}
							else
							{
								Boolean const	kSetForeground = (38 == p);
								SInt16 const	kParam2 = inDataPtr->emulator.parameterValues[i + 1];
								SInt16 const	kParam3 = inDataPtr->emulator.parameterValues[i + 2];
								SInt16 const	kColorParam = kParam3;
								
								
								if (5 != kParam2)
								{
									if (DebugInterface_LogsTerminalInputChar())
									{
										Console_Warning(Console_WriteValue, "unrecognized parameter for type of color (expected 5)",
														kParam2);
									}
								}
								else
								{
									if (kSetForeground)
									{
										STYLE_SET_FOREGROUND_INDEX(inDataPtr->current.drawingAttributes, kColorParam);
									}
									else
									{
										STYLE_SET_BACKGROUND_INDEX(inDataPtr->current.drawingAttributes, kColorParam);
									}
								}
								++i; // skip next parameter (2)
								++i; // skip next parameter (3)
							}
						}
					}
					else if (39 == p)
					{
						// generally means “reset foreground”
						STYLE_CLEAR_FOREGROUND_INDEX(inDataPtr->current.drawingAttributes);
					}
					else if (49 == p)
					{
						// generally means “reset background”
						STYLE_CLEAR_BACKGROUND_INDEX(inDataPtr->current.drawingAttributes);
					}
					else
					{
						Console_WriteValue("current terminal in SGR mode does not support parameter", p);
					}
				}
				++i;
			}
		}
		break;
	
	case kStateSM:
		My_VT100::modeSetReset(inDataPtr, true/* set */);
		break;
	
	case kStateTBC:
		if (3 == inDataPtr->emulator.parameterValues[0])
		{
			// clear all tabs
			tabStopClearAll(inDataPtr);
		}
		else if (0 >= inDataPtr->emulator.parameterValues[0])
		{
			// clear tab at current position
			inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabClear;
		}
		else
		{
			// invalid (do nothing)
		}
		break;
	
	// ignore all VT100/VT52 sequences that are invalid within this ANSI-mode parser
	case My_VT100::VT52::kStateCU:
	case My_VT100::VT52::kStateCD:
	case My_VT100::VT52::kStateCR:
	//case My_VT100::VT52::kStateCL: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateNGM:
	case My_VT100::VT52::kStateXGM:
	//case My_VT100::VT52::kStateCH: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateRLF:
	case My_VT100::VT52::kStateEES:
	case My_VT100::VT52::kStateEEL:
	case My_VT100::VT52::kStateDCA:
	//case My_VT100::VT52::kStateID: // this conflicts with a valid VT100 ANSI mode value above
	//case My_VT100::VT52::kStateNAKM: // this conflicts with a valid VT100 ANSI mode value above
	//case My_VT100::VT52::kStateXAKM: // this conflicts with a valid VT100 ANSI mode value above
	case My_VT100::VT52::kStateANSI:
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_DefaultEmulator::stateTransition, inDataPtr, inBuffer, inLength,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT100::stateTransition


/*!
Switches a VT100 terminal to VT52 mode, which means it
starts to accept VT52 sequences.

(3.1)
*/
void
My_VT100::
vt52Mode	(My_ScreenBufferPtr		inDataPtr)
{
	inDataPtr->modeANSIEnabled = false;
	inDataPtr->emulator.pushedCallbacks = inDataPtr->emulator.currentCallbacks;
	inDataPtr->emulator.currentCallbacks = My_Emulator::Callbacks
											(My_DefaultEmulator::echoData,
												My_VT100::VT52::stateDeterminant,
												My_VT100::VT52::stateTransition);
	initializeParserStateStack(&inDataPtr->emulator);
}// My_VT100::vt52Mode


/*!
Handles the VT100 VT52-compatibility sequence 'ESC D'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorBackward	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
	else moveCursorLeftToEdge(inDataPtr);
}// My_VT100::VT52::cursorBackward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC B'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorDown	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY < inDataPtr->originRegionPtr->lastRow) moveCursorDown(inDataPtr);
	else moveCursorDownToEdge(inDataPtr);
}// My_VT100::VT52:cursorDown


/*!
Handles the VT100 VT52-compatibility sequence 'ESC C'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorForward	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		rightLimit = inDataPtr->current.returnNumberOfColumnsPermitted() - ((inDataPtr->modeAutoWrap) ? 0 : 1);
	
	
	if (inDataPtr->current.cursorX < rightLimit) moveCursorRight(inDataPtr);
	else moveCursorRightToEdge(inDataPtr);
}// My_VT100::VT52:cursorForward


/*!
Handles the VT100 VT52-compatibility sequence 'ESC A'.
See the VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
cursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY > inDataPtr->originRegionPtr->firstRow)
	{
		moveCursorUp(inDataPtr);
	}
	else
	{
		moveCursorUpToEdge(inDataPtr);
	}
}// My_VT100::VT52:cursorUp


/*!
Handles the VT100 'ESC Z' sequence, which should only
be recognized in VT52 compatibility mode.  See the
VT100 manual for complete details.

(3.0)
*/
inline void
My_VT100::VT52::
identify	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		Session_SendData(session, "\033/Z", 3);
	}
}// My_VT100::VT52:identify


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT52-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT100::VT52::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	switch (inNowOutNext.first)
	{
	case kMy_ParserStateSeenESC:
		switch (*inBuffer)
		{
		case 'A':
			inNowOutNext.second = kMy_ParserStateSeenESCA;
			break;
		
		case 'B':
			inNowOutNext.second = kMy_ParserStateSeenESCB;
			break;
		
		case 'C':
			inNowOutNext.second = kMy_ParserStateSeenESCC;
			break;
		
		case 'D':
			inNowOutNext.second = kMy_ParserStateSeenESCD;
			break;
		
		case 'F':
			inNowOutNext.second = kMy_ParserStateSeenESCF;
			break;
		
		case 'G':
			inNowOutNext.second = kMy_ParserStateSeenESCG;
			break;
		
		case 'H':
			inNowOutNext.second = kMy_ParserStateSeenESCH;
			break;
		
		case 'I':
			inNowOutNext.second = kMy_ParserStateSeenESCI;
			break;
		
		case 'J':
			inNowOutNext.second = kMy_ParserStateSeenESCJ;
			break;
		
		case 'K':
			inNowOutNext.second = kMy_ParserStateSeenESCK;
			break;
		
		case 'Y':
			inNowOutNext.second = kMy_ParserStateSeenESCY;
			break;
		
		case 'Z':
			inNowOutNext.second = kMy_ParserStateSeenESCZ;
			break;
		
		case '=':
			inNowOutNext.second = kMy_ParserStateSeenESCEquals;
			break;
		
		case '>':
			inNowOutNext.second = kMy_ParserStateSeenESCGreaterThan;
			break;
		
		case '<':
			inNowOutNext.second = kMy_ParserStateSeenESCLessThan;
			break;
		
		default:
			// this is unexpected data; choose a new state
			if (DebugInterface_LogsTerminalInputChar())
			{
				Console_Warning(Console_WriteValueCharacter, "VT52 did not expect an ESC to be followed by character", *inBuffer);
			}
			outHandled = false;
			break;
		}
		break;
	
	case kStateDCA:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		inNowOutNext.second = kStateDCAY;
		result = 0; // absorb nothing
		break;
	
	case kStateDCAY:
		// the 2 characters after a VT52 DCA are the coordinates (Y first)
		inNowOutNext.second = kStateDCAX;
		result = 0; // absorb nothing
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT100::stateDeterminant, inEmulatorPtr, inBuffer, inLength,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 in VT52 mode in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT100 in VT52 mode proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        VT100 in VT52 mode bases this at least on character", *inBuffer);
	
	return result;
}// My_VT100::VT52::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT100-specific (VT52 mode) state changes.

(3.1)
*/
UInt32
My_VT100::VT52::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						inLength,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT100 VT52 transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     VT100 VT52 transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateCU:
		My_VT100::VT52::cursorUp(inDataPtr);
		break;
	
	case kStateCD:
		My_VT100::VT52::cursorDown(inDataPtr);
		break;
	
	case kStateCR:
		My_VT100::VT52::cursorForward(inDataPtr);
		break;
	
	case kStateCL:
		My_VT100::VT52::cursorBackward(inDataPtr);
		break;
	
	case kStateNGM:
		// enter graphics mode - unimplemented
		break;
	
	case kStateXGM:
		// exit graphics mode - unimplemented
		break;
	
	case kStateCH:
		moveCursor(inDataPtr, 0, 0); // home cursor in VT52 compatibility mode of VT100
		break;
	
	case kStateRLF:
		moveCursorUpOrScroll(inDataPtr); // reverse line feed in VT52 compatibility mode of VT100
		break;
	
	case kStateEES:
		bufferEraseFromCursorToEnd(inDataPtr); // erase to end of screen, in VT52 compatibility mode of VT100
		break;
	
	case kStateEEL:
		bufferEraseFromCursorColumnToLineEnd(inDataPtr); // erase to end of line, in VT52 compatibility mode of VT100
		break;
	
	case kStateDCA:
		// direct cursor address in VT52 compatibility mode of VT100;
		// new cursor position is encoded as the next two characters
		// (vertical first, then horizontal) offset by the octal
		// value 037 (equal to decimal 31); this is handled by 2
		// other states, "DCAY" and "DCAX" (below)
		break;
	
	case kStateDCAY:
		// VT52 DCA, first character (Y + 31)
		{
			My_ScreenRowIndex	newY = 0;
			
			
			newY = *inBuffer - 32/* - 31 - 1 to convert from one-based to zero-based */;
			++result;
			
			// constrain the value and then change it safely
			//if (newY < 0) newY = 0;
			if (newY >= inDataPtr->screenBuffer.size())
			{
				newY = inDataPtr->screenBuffer.size() - 1;
			}
			moveCursorY(inDataPtr, newY);
		}
		break;
	
	case kStateDCAX:
		// VT52 DCA, second character (X + 31)
		{
			SInt16		newX = 0;
			
			
			newX = *inBuffer - 32/* - 31 - 1 to convert from one-based to zero-based */;
			++result;
			
			// constrain the value and then change it safely
			if (newX < 0) newX = 0;
			if (newX >= inDataPtr->current.returnNumberOfColumnsPermitted())
			{
				newX = inDataPtr->current.returnNumberOfColumnsPermitted() - 1;
			}
			moveCursorX(inDataPtr, newX);
		}
		break;
	
	case kStateID:
		My_VT100::VT52::identify(inDataPtr);
		break;
	
	case kStateNAKM:
		inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (use application key sequences) in VT52 compatibility mode of VT100
		break;
	
	case kStateXAKM:
		inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (restore regular keypad characters) in VT52 compatibility mode of VT100
		break;
	
	case kStateANSI:
		My_VT100::ansiMode(inDataPtr);
		break;
	
	// ignore all VT100 sequences that are invalid in VT52 mode
	// NONE?
	//case :
	//	break;
	
	default:
		// other state transitions should still basically be handled as if in VT100 ANSI
		// (NOTE: this switch should filter out any ANSI mode states that are supposed to
		// be ignored while in VT52 mode!)
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_VT100::stateTransition, inDataPtr, inBuffer, inLength,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT100::VT52::stateTransition


/*!
Handles the VT102 'DCH' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
deleteCharacters	(My_ScreenBufferPtr		inDataPtr)
{
	// “one character” is assumed if a parameter is zero, or there are no parameters,
	// even though this is not explicitly stated in VT102 documentation
	SInt16		i = 0;
	UInt16		totalChars = (inDataPtr->emulator.parameterEndIndex < 0) ? 1 : 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.parameterEndIndex; ++i)
	{
		if (inDataPtr->emulator.parameterValues[i] > 0)
		{
			totalChars += inDataPtr->emulator.parameterValues[i];
		}
		else
		{
			++totalChars;
		}
	}
	bufferRemoveCharactersAtCursorColumn(inDataPtr, totalChars);
}// deleteCharacters


/*!
Handles the VT102 'DL' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
deleteLines		(My_ScreenBufferPtr		inDataPtr)
{
	// do nothing if the cursor is outside the scrolling region
	if ((inDataPtr->customScrollingRegion.lastRow >= inDataPtr->current.cursorY) &&
		(inDataPtr->customScrollingRegion.firstRow <= inDataPtr->current.cursorY))
	{
		// “one line” is assumed if a parameter is zero, or there are no parameters,
		// even though this is not explicitly stated in VT102 documentation
		My_ScreenBufferLineList::iterator	lineIterator;
		SInt16								i = 0;
		UInt16								totalLines = (inDataPtr->emulator.parameterEndIndex < 0) ? 1 : 0;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		for (i = 0; i <= inDataPtr->emulator.parameterEndIndex; ++i)
		{
			if (inDataPtr->emulator.parameterValues[i] > 0)
			{
				totalLines += inDataPtr->emulator.parameterValues[i];
			}
			else
			{
				++totalLines;
			}
		}
		bufferRemoveLines(inDataPtr, totalLines, lineIterator, kMy_AttributeRuleCopyLastLine);
	}
}// deleteLines


/*!
Handles the VT102 'IL' sequence.  See the VT102
manual for complete details.

(3.1)
*/
void
My_VT102::
insertLines		(My_ScreenBufferPtr		inDataPtr)
{
	// do nothing if the cursor is outside the scrolling region
	if ((inDataPtr->customScrollingRegion.lastRow >= inDataPtr->current.cursorY) &&
		(inDataPtr->customScrollingRegion.firstRow <= inDataPtr->current.cursorY))
	{
		// “one line” is assumed if a parameter is zero, or there are no parameters,
		// even though this is not explicitly stated in VT102 documentation
		My_ScreenBufferLineList::iterator	lineIterator;
		SInt16								i = 0;
		UInt16								totalLines = (inDataPtr->emulator.parameterEndIndex < 0) ? 1 : 0;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		for (i = 0; i <= inDataPtr->emulator.parameterEndIndex; ++i)
		{
			if (inDataPtr->emulator.parameterValues[i] > 0)
			{
				totalLines += inDataPtr->emulator.parameterValues[i];
			}
			else
			{
				++totalLines;
			}
		}
		bufferInsertBlankLines(inDataPtr, totalLines, lineIterator, kMy_AttributeRuleCopyLastLine);
	}
}// insertLines


/*!
Handles the VT102 'DECLL' sequence.  See the VT102
manual for complete details.  Unlike VT100, the VT102
has only a single LED.

(3.1)
*/
inline void
My_VT102::
loadLEDs	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		i = 0;
	
	
	for (i = 0; i <= inDataPtr->emulator.parameterEndIndex; ++i)
	{
		// a parameter of 1 means “LED 1 on”, 0 means “LED 1 off”
		if (0 == inDataPtr->emulator.parameterValues[i])
		{
			highlightLED(inDataPtr, 0/* 0 means “all off” */);
		}
		else if (1 == inDataPtr->emulator.parameterValues[i])
		{
			highlightLED(inDataPtr, 1/* LED # */);
		}
	}
}// loadLEDs


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT102-specific states based on the characters of the given
buffer.

Since a VT102 is very similar to a VT100, the vast majority
of state analysis is done by the VT100 routine.

(3.1)
*/
UInt32
My_VT102::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// if no interrupt has occurred, use the current state and
	// the available data to determine the next logical state
	switch (inNowOutNext.first)
	{
	case My_VT100::kStateCSIParamScan:
		// look for a terminating character (anything not legal in a parameter)
		switch (*inBuffer)
		{
		case 'i':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsi;
			break;
		
		case 'L':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsL;
			break;
		
		case 'M':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsM;
			break;
		
		case 'P':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsP;
			break;
		
		default:
			outHandled = false;
			break;
		}
		break;
	
	default:
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT100::stateDeterminant, inEmulatorPtr, inBuffer, inLength,
						inNowOutNext, outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT102 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT102 proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        VT102 bases this at least on character", *inBuffer);
	
	return result;
}// My_VT102::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT102-specific state changes.

(3.1)
*/
UInt32
My_VT102::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						inLength,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT102 transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     VT102 transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateControlLFVTFF:
		// line feed
		// all of these are interpreted the same for VT102;
		// when printing, this also forces a line to print
		// (for auto-print, but not printer controller mode)
		moveCursorDownOrScroll(inDataPtr);
		if (0 != inDataPtr->printingModes)
		{
			UInt8 const		kReturn = '\015';
			
			
			// the implementation is such that sending any new-line-like character
			// (return, whatever) will translate to the actual new-line sequence
			// defined for the stream
			StreamCapture_WriteUTF8Data(inDataPtr->printingStream, &kReturn, sizeof(kReturn));
		}
		break;
	
	case kStateDCH:
		deleteCharacters(inDataPtr);
		break;
	
	case kStateDECLL:
		loadLEDs(inDataPtr);
		break;
	
	case kStateDL:
		deleteLines(inDataPtr);
		break;
	
	case kStateIL:
		insertLines(inDataPtr);
		break;
	
	case kStateMC:
		// media copy (automatic printing)
		{
			Boolean		printScreen = false;
			
			
			if (inDataPtr->emulator.parameterEndIndex >= 0)
			{
				switch (inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex])
				{
				case 5:
					// printer controller (print lines without necessarily displaying them);
					// this can be enabled or disabled while auto-print remains in effect
					inDataPtr->printingReset();
					inDataPtr->printingModes |= kMy_PrintingModePrintController;
					break;
				
				case 4:
					// printer controller off
					inDataPtr->printingModes &= ~kMy_PrintingModePrintController;
					inDataPtr->printingEnd();
					break;
				
				case 0:
					printScreen = true;
					break;
				
				case -2:
					// private parameters (e.g. ESC [ ? 5 i)
					switch (inDataPtr->emulator.parameterValues[0])
					{
					case 5:
						// auto-print (print lines that are also displayed)
						inDataPtr->printingReset();
						inDataPtr->printingModes |= kMy_PrintingModeAutoPrint;
						break;
					
					case 4:
						// auto-print off
						inDataPtr->printingModes &= ~kMy_PrintingModeAutoPrint;
						inDataPtr->printingEnd();
						break;
					
					case 1:
						// print cursor line
						// UNIMPLEMENTED
						break;
					
					default:
						// ???
						if (DebugInterface_LogsTerminalInputChar())
						{
							Console_Warning(Console_WriteLine, "VT102 media copy did not recognize the given private parameters");
						}
						break;
					}
					break;
				
				default:
					if (DebugInterface_LogsTerminalInputChar())
					{
						Console_Warning(Console_WriteLine, "VT102 media copy did not recognize the given parameters");
					}
					break;
				}
			}
			else
			{
				// no parameters; defaults to “print screen”
				printScreen = true;
			}
			
			if (printScreen)
			{
				// print the screen or the scrolling region, based on
				// the most recent use of the DECEXT sequence
				// UNIMPLEMENTED
			}
		}
		break;
	
	default:
		// other state transitions should still basically be handled as if in VT100 ANSI
		outHandled = false;
		break;
	}
	
	if (false == outHandled)
	{
		result = invokeEmulatorStateTransitionProc
					(My_VT100::stateTransition, inDataPtr, inBuffer, inLength,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT102::stateTransition


/*!
Handles the VT220 'DA' sequence.  See the VT220
manual for complete details.

(3.0)
*/
inline void
My_VT220::
deviceAttributes	(My_ScreenBufferPtr		inDataPtr)
{
	SessionRef		session = returnListeningSession(inDataPtr);
	
	
	if (nullptr != session)
	{
		// support GPO (graphics processor option) and AVO (advanced video option)
		Session_SendData(session, "\033[?62;1;6c", 10);
	}
}// My_VT220::deviceAttributes


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
VT220-specific states based on the characters of the given
buffer.

(3.1)
*/
UInt32
My_VT220::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	assert(inLength > 0);
	UInt32		result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	// INCOMPLETE
	outHandled = false;
	if (false == outHandled)
	{
		result = invokeEmulatorStateDeterminantProc
					(My_VT102::stateDeterminant, inEmulatorPtr, inBuffer, inLength, inNowOutNext,
						outInterrupt, outHandled);
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     VT220 proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        VT220 bases this at least on character", *inBuffer);
	
	return result;
}// My_VT220::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that
responds to VT220-specific state changes.

(3.1)
*/
UInt32
My_VT220::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						inLength,
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< VT220 transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     VT220 transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	outHandled = false;
	if (false == outHandled)
	{
		// other state transitions should still basically be handled as if in VT100
		result = invokeEmulatorStateTransitionProc
					(My_VT102::stateTransition, inDataPtr, inBuffer, inLength,
						inOldNew, outHandled);
	}
	
	return result;
}// My_VT220::stateTransition


/*!
Handles the XTerm 'CBT' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved backwards to the first tab stop on the
current line.  Otherwise, the parameter refers to the number of
tab stops to move.

See also the handling of the tab character in the VT terminal,
which tabs in the opposite direction.

(4.0)
*/
void
My_XTerm::
cursorBackwardTabulation	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		tabCount = 1;
	
	
	if (-1 != inDataPtr->emulator.parameterValues[0])
	{
		tabCount = inDataPtr->emulator.parameterValues[0];
	}
	
	for (SInt16 i = 0; i < tabCount; ++i)
	{
		moveCursorLeftToNextTabStop(inDataPtr);
	}
}// My_XTerm::cursorBackwardTabulation


/*!
Handles the XTerm 'CHA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first position on the current line.  If
there is just one parameter, it is a one-based index for the new
cursor column on the current line.  And with two parameters, the
order changes and the parameters are the one-based indices of the
new cursor row and column, in that order.

NOTE:	It is not clear yet if this should be any different from
		the implementation of horizontalPositionAbsolute().
		Currently they are the same.

(4.0)
*/
void
My_XTerm::
cursorCharacterAbsolute		(My_ScreenBufferPtr		inDataPtr)
{
	horizontalPositionAbsolute(inDataPtr);
}// My_XTerm::cursorCharacterAbsolute


/*!
Handles the XTerm 'CHT' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved forwards to the first tab stop on the
current line.  Otherwise, the parameter refers to the number of
tab stops to move.

See also the handling of the tab character in the VT terminal.

(4.0)
*/
void
My_XTerm::
cursorForwardTabulation		(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		tabCount = 1;
	
	
	if (-1 != inDataPtr->emulator.parameterValues[0])
	{
		tabCount = inDataPtr->emulator.parameterValues[0];
	}
	
	for (SInt16 i = 0; i < tabCount; ++i)
	{
		moveCursorRightToNextTabStop(inDataPtr);
	}
}// My_XTerm::cursorForwardTabulation


/*!
Handles the XTerm 'CNL' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved to the next line.  Otherwise, it is moved
the specified number of lines downward, limited by the visible
screen area.  In any case, the cursor is also reset to the
first column of the display.

(4.0)
*/
void
My_XTerm::
cursorNextLine		(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	if (-1 == inDataPtr->emulator.parameterValues[0])
	{
		++newY;
	}
	else
	{
		newY += inDataPtr->emulator.parameterValues[0];
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, 0, newY);
}// My_XTerm::cursorNextLine


/*!
Handles the XTerm 'CPL' sequence.

This should accept zero or one parameters.  With no parameters,
the cursor is moved to the previous line.  Otherwise, it is
moved the specified number of lines upward, limited by the
visible screen area.  In any case, the cursor is also reset to
the first column of the display.

(4.0)
*/
void
My_XTerm::
cursorPreviousLine		(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	if (-1 == inDataPtr->emulator.parameterValues[0])
	{
		--newY;
	}
	else
	{
		newY -= inDataPtr->emulator.parameterValues[0];
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, 0, newY);
}// My_XTerm::cursorPreviousLine


/*!
Handles the XTerm 'HPA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first position on the current line.  If
there is just one parameter, it is a one-based index for the new
cursor column on the current line.  And with two parameters, the
order changes and the parameters are the one-based indices of the
new cursor row and column, in that order.

See also the 'CUP' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
horizontalPositionAbsolute	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16 const		kParam0 = inDataPtr->emulator.parameterValues[0];
	SInt16 const		kParam1 = inDataPtr->emulator.parameterValues[1];
	SInt16				newX = 0;
	My_ScreenRowIndex	newY = inDataPtr->current.cursorY;
	
	
	// in the following definitions, 0 and 1 are considered the same number
	if (-1 == kParam1)
	{
		// only one parameter was given, so it is the column and
		// the cursor row does not change
		newX = (0 == kParam0)
				? 0
				: (-1 != kParam0)
					? kParam0 - 1
					: 0/* default is column 1 in current row */;
	}
	else
	{
		// two parameters, so the order changes to (row, column)
		newY = (0 == kParam0)
				? 0
				: (-1 != kParam0)
					? kParam0 - 1
					: inDataPtr->current.cursorY/* default is current cursor row */;
		newX = (0 == kParam1)
				? 0
				: kParam1 - 1;
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, newX, newY);
}// My_XTerm::horizontalPositionAbsolute


/*!
Handles the XTerm 'ICH' sequence.

This should accept zero or one parameters.  With no parameters,
a single blank character is inserted at the character position,
moving the rest of the line over by one column.  Otherwise, the
parameter refers to the number of blank characters to insert
(shifting the line by that amount).

(4.0)
*/
void
My_XTerm::
insertBlankCharacters	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16				preWriteCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex	preWriteCursorY = inDataPtr->current.cursorY;
	SInt16				characterCount = inDataPtr->emulator.parameterValues[0];
	
	
	if (-1 == characterCount)
	{
		characterCount = 1;
	}
	else if (0 == characterCount)
	{
		characterCount = 1;
	}
	
	bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, characterCount);
	
	// add the effects of the insert to the text-change region;
	// this should trigger things like Terminal View updates
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = preWriteCursorY;
		if (preWriteCursorY != inDataPtr->current.cursorY)
		{
			// more than one line; just draw all lines completely
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = inDataPtr->current.cursorY - preWriteCursorY + 1;
		}
		else
		{
			// invalidate the entire line starting from the original cursor column
			range.firstColumn = preWriteCursorX;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - preWriteCursorX;
			range.rowCount = 1;
		}
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// My_XTerm::insertBlankCharacters


/*!
Handles the XTerm 'SD' sequence.

This should accept zero or one parameters.  With no parameters,
the screen content moves down by one line (which from a user’s
point of view is like clicking an up-arrow in a scroll bar).
If a parameter is given, it is the number of rows to scroll by.

See also the 'IND' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
scrollDown	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		lineCount = inDataPtr->emulator.parameterValues[0];
	
	
	if (-1 == lineCount)
	{
		lineCount = 1;
	}
	
	// the following function handles the scrolling region boundaries automatically
	screenScroll(inDataPtr, -lineCount);
}// My_XTerm::scrollDown


/*!
Handles the XTerm 'SU' sequence.

This should accept zero or one parameters.  With no parameters,
the screen content moves up by one line (which from a user’s
point of view is like clicking a down-arrow in a scroll bar).
If a parameter is given, it is the number of rows to scroll by.

See also the 'RI' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
scrollUp	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		lineCount = inDataPtr->emulator.parameterValues[0];
	
	
	if (-1 == lineCount)
	{
		lineCount = 1;
	}
	
	// the following function handles the scrolling region boundaries automatically
	screenScroll(inDataPtr, lineCount);
}// My_XTerm::scrollUp


/*!
A standard "My_EmulatorStateDeterminantProcPtr" that sets
XTerm-specific window states based on the characters of the
given buffer.

(4.0)
*/
UInt32
My_XTerm::
stateDeterminant	(My_EmulatorPtr			inEmulatorPtr,
					 UInt8 const*			inBuffer,
					 UInt32					inLength,
					 My_ParserStatePair&	inNowOutNext,
					 Boolean&				outInterrupt,
					 Boolean&				outHandled)
{
	assert(inLength > 0);
	UInt8 const		kTriggerChar = *inBuffer; // for convenience; usually only first character matters
	UInt32			result = 1; // the first character is *usually* “used”, so 1 is the default (it may change)
	
	
	switch (inNowOutNext.first)
	{
	case My_VT100::kStateCSIParamScan:
		// look for a terminating character (anything not legal in a parameter)
		switch (*inBuffer)
		{
		case 'd':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsd;
			break;
		
		case 'E':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsE;
			break;
		
		case 'F':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsF;
			break;
		
		case 'G':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsG;
			break;
		
		case 'I':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsI;
			break;
		
		case 'S':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsS;
			break;
		
		case 'T':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsT;
			break;
		
		case 'Z':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsZ;
			break;
		
		case '@':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsAt;
			break;
		
		case '`':
			inNowOutNext.second = kMy_ParserStateSeenESCLeftSqBracketParamsBackquote;
			break;
		
		default:
			outHandled = false;
			break;
		}
		break;
	
	case kMy_ParserStateSeenESC:
	case kMy_ParserStateSeenESCRightSqBracket:
		result = inEmulatorPtr->currentCallbacks.stateDeterminant(inEmulatorPtr, inBuffer, inLength,
																	inNowOutNext, outInterrupt, outHandled);
		break;
	
	case kStateSWITAcquireStr:
		if (inEmulatorPtr->supportsVariant(kPreferences_TagXTermWindowAlterationEnabled))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = kStateStringTerminator;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSWITAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateSITAcquireStr:
		if (inEmulatorPtr->supportsVariant(kPreferences_TagXTermWindowAlterationEnabled))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = kStateStringTerminator;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSITAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateSWTAcquireStr:
		if (inEmulatorPtr->supportsVariant(kPreferences_TagXTermWindowAlterationEnabled))
		{
			switch (kTriggerChar)
			{
			case '\007':
				inNowOutNext.second = kStateStringTerminator;
				break;
			
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string until a known terminator is found
				inNowOutNext.second = kStateSWTAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateColorAcquireStr:
		if (inEmulatorPtr->supportsVariant(kPreferences_TagXTerm256ColorsEnabled))
		{
			switch (kTriggerChar)
			{
			case '\033':
				inNowOutNext.second = kMy_ParserStateSeenESC;
				break;
			
			default:
				// continue extending the string; termination is an escape sequence (a different state)
				inNowOutNext.second = kStateColorAcquireStr;
				result = 0; // do not absorb the unknown
				break;
			}
		}
		else
		{
			// ignore
			outHandled = false;
		}
		break;
	
	case kStateStringTerminator:
		// ignore
		outHandled = false;
		break;
	
	default:
		// other states are not handled at all
		outHandled = false;
		break;
	}
	
	// debug
	//Console_WriteValueFourChars("    <<< XTerm in state", inNowOutNext.first);
	//Console_WriteValueFourChars(">>>     XTerm proposes state", inNowOutNext.second);
	//Console_WriteValueCharacter("        XTerm bases this at least on character", *inBuffer);
	
	return result;
}// My_XTerm::stateDeterminant


/*!
A standard "My_EmulatorStateTransitionProcPtr" that responds to
XTerm-specific window state changes.

(4.0)
*/
UInt32
My_XTerm::
stateTransition		(My_ScreenBufferPtr			inDataPtr,
					 UInt8 const*				inBuffer,
					 UInt32						UNUSED_ARGUMENT(inLength),
					 My_ParserStatePair const&	inOldNew,
					 Boolean&					outHandled)
{
	UInt32		result = 0; // usually, no characters are consumed at the transition stage
	
	
	// debug
	//Console_WriteValueFourChars("    <<< XTerm transition from state", inOldNew.first);
	if (DebugInterface_LogsTerminalState())
	{
		Console_WriteValueFourChars(">>>     XTerm transition to state  ", inOldNew.second);
	}
	
	// decide what to do based on the proposed transition
	// INCOMPLETE
	switch (inOldNew.second)
	{
	case kStateCBT:
		cursorBackwardTabulation(inDataPtr);
		break;
	
	case kStateCHA:
		cursorCharacterAbsolute(inDataPtr);
		break;
	
	case kStateCHT:
		cursorForwardTabulation(inDataPtr);
		break;
	
	case kStateCNL:
		cursorNextLine(inDataPtr);
		break;
	
	case kStateCPL:
		cursorPreviousLine(inDataPtr);
		break;
	
	case kStateHPA:
		horizontalPositionAbsolute(inDataPtr);
		break;
	
	case kStateICH:
		insertBlankCharacters(inDataPtr);
		break;
	
	case kStateSD:
		scrollDown(inDataPtr);
		break;
	
	case kStateSU:
		scrollUp(inDataPtr);
		break;
	
	case kStateVPA:
		verticalPositionAbsolute(inDataPtr);
		break;
	
	case kStateSWIT:
	case kStateSIT:
	case kStateSWT:
	case kStateSetColor:
		inDataPtr->emulator.stringAccumulator.clear();
		inDataPtr->emulator.stringAccumulatorState = inOldNew.second;
		break;
	
	case kStateSWITAcquireStr:
	case kStateSITAcquireStr:
	case kStateSWTAcquireStr:
	case kStateColorAcquireStr:
		inDataPtr->emulator.stringAccumulator += *inBuffer;
		result = 1;
		break;
	
	case kStateStringTerminator:
		// a window and/or icon title can actually be terminated in multiple
		// ways; if a new-style XTerm string terminator is seen, then the
		// action must be chosen based on the state that most recently used
		// the string buffer
		switch (inDataPtr->emulator.stringAccumulatorState)
		{
		case kStateSWIT:
		case kStateSIT:
		case kStateSWT:
			if (inDataPtr->emulator.supportsVariant(kPreferences_TagXTermWindowAlterationEnabled))
			{
				CFStringRef		titleString = CFStringCreateWithCString(kCFAllocatorDefault,
																		inDataPtr->emulator.stringAccumulator.c_str(),
																		inDataPtr->emulator.inputTextEncoding);
				
				
				if (nullptr != titleString)
				{
					if ((kStateSWIT == inDataPtr->emulator.stringAccumulatorState) ||
						(kStateSWT == inDataPtr->emulator.stringAccumulatorState))
					{
						inDataPtr->windowTitleCFString.setCFTypeRef(titleString);
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
					}
					if ((kStateSWIT == inDataPtr->emulator.stringAccumulatorState) ||
						(kStateSIT == inDataPtr->emulator.stringAccumulatorState))
					{
						inDataPtr->iconTitleCFString.setCFTypeRef(titleString);
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowIconTitle, inDataPtr->selfRef/* context */);
					}
				}
				CFRelease(titleString), titleString = nullptr;
			}
			else
			{
				// ignore
				outHandled = false;
			}
			break;
		
		case kStateSetColor:
			if (inDataPtr->emulator.supportsVariant(kPreferences_TagXTerm256ColorsEnabled))
			{
				// the accumulated string up to this point actually needs to be parsed; currently,
				// only strings of this form are allowed:
				//		%d;rgb:%x/%x/%x - set color at index, to red, green, blue components in hex
				char const*		stringPtr = inDataPtr->emulator.stringAccumulator.c_str();
				int				i = 0;
				int				r = 0;
				int				g = 0;
				int				b = 0;
				int				scanResult = sscanf(stringPtr, "%d;rgb:%x/%x/%x", &i, &r, &g, &b);
				
				
				if (4 == scanResult)
				{
					if ((i > 255) || (r > 255) || (g > 255) || (b > 255) ||
						(i < 16/* cannot overwrite base ANSI colors */) || (r < 0) || (g < 0) || (b < 0))
					{
						Console_Warning(Console_WriteValueFloat4, "one or more illegal indices found in request to set XTerm color: index, red, green, blue", i, r, g, b);
					}
					else
					{
						// success!
						Terminal_XTermColorDescription		colorInfo;
						
						
						bzero(&colorInfo, sizeof(colorInfo));
						colorInfo.screen = inDataPtr->selfRef;
						colorInfo.index = i;
						colorInfo.redComponent = r;
						colorInfo.greenComponent = g;
						colorInfo.blueComponent = b;
						
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeXTermColor, &colorInfo/* context */);
						
						//Console_WriteValueFloat4("set color at index to red, green, blue", i, r, g, b);
					}
				}
				else
				{
					Console_Warning(Console_WriteValue, "failed to parse XTerm color string; sscanf() result", scanResult);
					Console_Warning(Console_WriteValueCString, "discarding unrecognized syntax for XTerm color string", stringPtr);
				}
			}
			else
			{
				// ignore
				outHandled = false;
			}
			break;
		
		default:
			// ignore
			outHandled = false;
			break;
		}
		break;
	
	default:
		// other state transitions are not handled at all
		outHandled = false;
		break;
	}
	
	return result;
}// My_XTerm::stateTransition


/*!
Handles the XTerm 'VPA' sequence.

This should accept up to 2 parameters.  With no parameters, the
cursor is moved to the first row, but the same cursor column.  If
there is just one parameter, it is a one-based index for the new
cursor row, using the same column.  And with two parameters, the
parameters are the one-based indices of the new cursor row and
column, in that order.

See also the 'CUP' sequence in the VT terminal.

(4.0)
*/
void
My_XTerm::
verticalPositionAbsolute	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16 const		kParam0 = inDataPtr->emulator.parameterValues[0];
	SInt16 const		kParam1 = inDataPtr->emulator.parameterValues[1];
	SInt16				newX = inDataPtr->current.cursorX;
	My_ScreenRowIndex	newY = 0;
	
	
	// in the following definitions, 0 and 1 are considered the same number
	if (-1 == kParam1)
	{
		// only one parameter was given, so it is the row and
		// the cursor column does not change
		newY = (0 == kParam0)
				? 0
				: (-1 != kParam0)
					? kParam0 - 1
					: 0/* default is row 1 in current column */;
	}
	else
	{
		// two parameters, giving (row, column)
		newY = (0 == kParam0)
				? 0
				: (-1 != kParam0)
					? kParam0 - 1
					: 0/* default is the home cursor row */;
		newX = (0 == kParam1)
				? 0
				: kParam1 - 1;
	}
	
	// the new values are not checked for violation of constraints
	// because constraints (including current origin mode) are
	// automatically enforced by moveCursor...() routines
	moveCursor(inDataPtr, newX, newY);
}// My_XTerm::verticalPositionAbsolute



/*!
A method of standard My_ScreenLineOperationProcPtr form,
this routine adds the length of the specified line of
text to an overall sum, pointed to by "inoutLengthPtr"
(assumed to be a CFIndex*).

Since this operation never modifies the buffer, "false"
is always returned.

(3.0)
*/
void
addScreenLineLength		(My_ScreenBufferPtr		UNUSED_ARGUMENT(inRef),
						 CFMutableStringRef		inLineTextBuffer,
						 UInt16					UNUSED_ARGUMENT(inOneBasedLineNumber),
						 void*					inoutLengthPtr)
{
	CFIndex*	sumPtr = REINTERPRET_CAST(inoutLengthPtr, CFIndex*);
	
	
	(*sumPtr) += (CFStringGetLength(inLineTextBuffer) + 1/* for newline */);
}// addScreenLineLength


/*!
A method of standard My_ScreenLineOperationProcPtr form,
this routine adds the data from the specified line text
buffer to a CFMutableStringRef, pointed to by
"inoutCFMutableStringRef".

This is a “raw” append, there is no new-line or other
delimiter between appended lines.

Currently, this routine is somewhat naïve, but in the
future it will do its best to express the actual rendered
content of the line in Unicode.

(3.1)
*/
void
appendScreenLineRawToCFString	(My_ScreenBufferPtr		UNUSED_ARGUMENT(inRef),
								 CFMutableStringRef		inLineTextBuffer,
								 UInt16					UNUSED_ARGUMENT(inOneBasedLineNumber),
								 void*					inoutCFMutableStringRef)
{
	CFMutableStringRef	mutableCFString = REINTERPRET_CAST(inoutCFMutableStringRef, CFMutableStringRef);
	
	
	CFStringAppend(mutableCFString, inLineTextBuffer);
}// appendScreenLineRawToCFString


/*!
Performs various assertions on the current custom scrolling
region range, to make sure all values are valid.

This should be called if the range changes for any reason.

(4.0)
*/
inline void
assertScrollingRegion	(My_ScreenBufferPtr		inDataPtr)
{
	assert(inDataPtr->customScrollingRegion.lastRow >= inDataPtr->customScrollingRegion.firstRow);
	assert(inDataPtr->customScrollingRegion.lastRow < inDataPtr->screenBuffer.size());
	assert(inDataPtr->screenBuffer.size() >= (inDataPtr->customScrollingRegion.lastRow - inDataPtr->customScrollingRegion.firstRow));
}// assertScrollingRegion


/*!
Erases characters on the cursor row, from the cursor
position to the end of the line.

(2.6)
*/
void
bufferEraseFromCursorColumnToLineEnd	(My_ScreenBufferPtr		inDataPtr)
{
	My_TextIterator						textIterator = nullptr;
	My_TextAttributesList::iterator		attrIterator;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	
	
	// if the cursor is positioned so as to trigger an erase
	// of the entire top line, and forced saving is enabled,
	// be sure to dump the screen contents into the scrollback
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->mayNeedToSaveToScrollback))
	{
		inDataPtr->mayNeedToSaveToScrollback = false;
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, cursorLineIterator);
	
	// clear out screen line
	attrIterator = cursorLineIterator->attributeVector.begin();
	std::advance(attrIterator, postWrapCursorX);
	textIterator = cursorLineIterator->textVectorBegin;
	std::advance(textIterator, postWrapCursorX);
	std::fill(attrIterator, cursorLineIterator->attributeVector.end(),
				cursorLineIterator->globalAttributes);
	std::fill(textIterator, cursorLineIterator->textVectorEnd, ' ');
	
	// add the remainder of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor column to line end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromCursorColumnToLineEnd


/*!
Erases characters from the current cursor position
up to the end position (last line, last column).

(2.6)
*/
void
bufferEraseFromCursorToEnd  (My_ScreenBufferPtr  inDataPtr)
{
	My_ScreenRowIndex	postWrapCursorY = inDataPtr->current.cursorY + 1;
	
	
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// if the cursor is in the home position, this action will
	// erase the entire visible screen; so, check the preference
	// for saving to scrollback, and copy the data if necessary
 	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->mayNeedToSaveToScrollback))
	{
		inDataPtr->mayNeedToSaveToScrollback = false;
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// blank out current line from cursor to end, and update
	bufferEraseFromCursorColumnToLineEnd(inDataPtr);
	
	// blank out following rows
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		locateCursorLine(inDataPtr, lineIterator);
		
		// skip cursor line
		++postWrapCursorY;
		++lineIterator;
		for (; lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
		}
	}
	
	// add all the remaining rows to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from cursor to screen end");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->screenBuffer.size() - postWrapCursorY + 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromCursorToEnd


/*!
Erases characters from the beginning of the screen
up to and including the current cursor position.
For example, if the cursor is at line 3, column 6,
lines 1 and 2 are erased as well as the first 6
columns of line 3.

(2.6)
*/
void
bufferEraseFromHomeToCursor		(My_ScreenBufferPtr  inDataPtr)
{
	My_ScreenRowIndex	postWrapCursorY = 0;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// blank out current line from beginning to cursor
	bufferEraseFromLineBeginToCursorColumn(inDataPtr);
	
	// blank out previous rows
	{
		My_ScreenBufferLineList::iterator	lineIterator = inDataPtr->screenBuffer.begin();
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (; lineIterator != cursorLineIterator; ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
		}
	}
	
	// add all the pre-cursor rows to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from home to cursor");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = postWrapCursorY - 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromHomeToCursor


/*!
Erases characters on the cursor row, from the start of
the line up to and including the cursor position.

(2.6)
*/
void
bufferEraseFromLineBeginToCursorColumn  (My_ScreenBufferPtr  inDataPtr)
{
	SInt32								fillLength = 0;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	// figure out where the cursor is, but first force it to
	// wrap to the next line if a wrap is pending
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	fillLength = 1 + postWrapCursorX;
	
	// clear from beginning of line to cursor
	locateCursorLine(inDataPtr, cursorLineIterator);
	std::fill_n(cursorLineIterator->attributeVector.begin(), fillLength,
				cursorLineIterator->globalAttributes);
	std::fill_n(cursorLineIterator->textVectorBegin, fillLength, ' ');
	
	// add the first part of the row to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase from line begin to cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = fillLength;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseFromLineBeginToCursorColumn


/*!
Erases an entire line in the screen buffer, triggering
redraws.  See also bufferEraseLineWithoutUpdate(), which
modifies the data buffer but does not notify about the
change.

Unlike bufferEraseLineWithoutUpdate(), this routine will
respect pending cursor wraps when asked to erase the
cursor line, and will leave line-global attributes
untouched.  (Its only real purpose is to handle the
VT100 "EL" sequence correctly.)

(2.6)
*/
void
bufferEraseLineWithUpdate		(My_ScreenBufferPtr		inDataPtr,
								 SInt16					inLineToEraseOrNegativeForCursorLine)
{
	My_ScreenRowIndex					postWrapCursorY = inLineToEraseOrNegativeForCursorLine;
	My_ScreenBufferLineList::iterator	cursorLineIterator;
	
	
	if (inLineToEraseOrNegativeForCursorLine < 0)
	{
		SInt16		postWrapCursorX = 0;
		
		
		cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	}
	
	// clear out line
	// 3.0 - the VT100 spec does not say that EL should erase double-width,
	//       so leave the line-global attributes unchanged in this routine
	locateCursorLine(inDataPtr, cursorLineIterator);
	std::fill(cursorLineIterator->attributeVector.begin(),
				cursorLineIterator->attributeVector.end(),
				cursorLineIterator->globalAttributes);
	std::fill(cursorLineIterator->textVectorBegin, cursorLineIterator->textVectorEnd, ' ');
	
	// add the entire row contents to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase line");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted();
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseLineWithUpdate


/*!
Erases an entire line in the screen buffer without
triggering a redraw.  See also bufferEraseLineWithUpdate().

(2.6)
*/
void
bufferEraseLineWithoutUpdate	(My_ScreenBufferPtr  	inDataPtr,
								 My_ScreenBufferLine&	inRow)
{
	// 3.0 - line-global attributes are cleared only if clearing an entire line
	inRow.globalAttributes = kTerminalTextAttributesAllOff;
	
	// fill in line
	bufferLineFill(inDataPtr, inRow, ' ', inRow.globalAttributes, false/* update global attributes */);
}// bufferEraseLineWithoutUpdate


/*!
Clears the screen, first saving its contents in the scrollback
buffer if that flag is turned on.  A screen redraw is triggered.

(2.6)
*/
void
bufferEraseVisibleScreenWithUpdate		(My_ScreenBufferPtr		inDataPtr)
{
	//Console_WriteLine("bufferEraseVisibleScreenWithUpdate");
	
	// save screen contents in scrollback buffer, if appropriate
	if (inDataPtr->saveToScrollbackOnClear)
	{
		screenCopyLinesToScrollback(inDataPtr);
	}
	
	// clear buffer
	{
		My_ScreenBufferLineList::iterator	lineIterator;
		
		
		for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
		{
			bufferEraseLineWithoutUpdate(inDataPtr, *lineIterator);
		}
	}
	
	// add the entire visible buffer to the text-change region;
	// this should trigger things like Terminal View updates
	//Console_WriteLine("text changed event: erase visible screen");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = 0;
		range.firstColumn = 0;
		range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
		range.rowCount = inDataPtr->screenBuffer.size();
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferEraseVisibleScreenWithUpdate


/*!
Clears out the text area and attributes buffers for
the specified screen.  All text characters are set to
blanks, and all attribute bytes become 0.  The display
is NOT updated.

(2.6)
*/
void
bufferEraseWithoutUpdate	(My_ScreenBufferPtr		inDataPtr)
{
	My_ScreenBufferLineList::iterator	lineIterator;
	
	
	for (lineIterator = inDataPtr->screenBuffer.begin(); lineIterator != inDataPtr->screenBuffer.end(); ++lineIterator)
	{
		std::fill(lineIterator->attributeVector.begin(), lineIterator->attributeVector.end(),
					kTerminalTextAttributesAllOff);
		std::fill(lineIterator->textVectorBegin, lineIterator->textVectorEnd, ' ');
	}
}// bufferEraseWithoutUpdate


/*!
Inserts the specified number of blank characters at the
current cursor position, truncating that many characters
from the end of the line when the line is shifted.  The
display is NOT updated.

(2.6)
*/
void
bufferInsertBlanksAtCursorColumnWithoutUpdate	(My_ScreenBufferPtr		inDataPtr,
												 SInt16					inNumberOfBlankCharactersToInsert)
{
	UInt16								numBlanksToAdd = inNumberOfBlankCharactersToInsert;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	toCursorLine;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, toCursorLine);
	
	// since the caller cannot know for sure if the cursor wrapped,
	// do bounds-checking between the screen edge and new location
	if ((postWrapCursorX + numBlanksToAdd) >= inDataPtr->current.returnNumberOfColumnsPermitted())
	{
		numBlanksToAdd = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
	}
	
	// update attributes
	{
		My_TextAttributesList::iterator		pastVisibleEnd = toCursorLine->attributeVector.begin();
		My_TextAttributesList::iterator		toCursorAttr = toCursorLine->attributeVector.begin();
		My_TextAttributesList::iterator		toFirstRelocatedAttr;
		My_TextAttributesList::iterator		pastLastPreservedAttr;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastPreservedAttr = pastVisibleEnd;
		
		std::advance(toCursorAttr, postWrapCursorX);
		toFirstRelocatedAttr = toCursorAttr;
		std::advance(toFirstRelocatedAttr, numBlanksToAdd);
		std::advance(pastLastPreservedAttr, -numBlanksToAdd);
		
		std::copy_backward(toCursorAttr, pastLastPreservedAttr, pastVisibleEnd);
		std::fill(toCursorAttr, toFirstRelocatedAttr, toCursorLine->globalAttributes);
	}
	
	// update text
	{
		My_TextIterator		pastVisibleEnd = toCursorLine->textVectorBegin;
		My_TextIterator		toCursorChar = toCursorLine->textVectorBegin;
		My_TextIterator		toFirstRelocatedChar;
		My_TextIterator		pastLastPreservedChar;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastPreservedChar = pastVisibleEnd;
		
		std::advance(toCursorChar, postWrapCursorX);
		toFirstRelocatedChar = toCursorChar;
		std::advance(toFirstRelocatedChar, numBlanksToAdd);
		std::advance(pastLastPreservedChar, -numBlanksToAdd);
		
		std::copy_backward(toCursorChar, pastLastPreservedChar, pastVisibleEnd);
		std::fill(toCursorChar, toFirstRelocatedChar, ' ');
	}
}// bufferInsertBlanksAtCursorColumnWithoutUpdate


/*!
Inserts the specified number of blank lines, scrolling the
remaining ones down and dropping any that fall off the end
of the scrolling region.  The display is updated.

See also bufferRemoveLines().

NOTE:   You cannot use this to alter the scrollback.

WARNING:	The specified line MUST be part of the terminal
			scrolling region.

(3.0)
*/
void
bufferInsertBlankLines	(My_ScreenBufferPtr						inDataPtr,
						 UInt16									inNumberOfLines,
						 My_ScreenBufferLineList::iterator&		inInsertionLine,
						 My_AttributeRule						inAttributeRule)
{
	My_ScreenBufferLineList::iterator	scrollingRegionBegin;
	My_ScreenBufferLineList::iterator	scrollingRegionEnd;
	
	
	locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
	if (0 != inNumberOfLines)
	{
		// the row index MUST be calculated immediately, since inserting lines into the
		// buffer might make it impossible to find this at the end (where this index is
		// actually needed)
		My_ScreenRowIndex const						kFirstInsertedRow = std::distance(inDataPtr->screenBuffer.begin(), inInsertionLine);
		My_ScreenRowIndex const						kLinesUntilEnd = std::distance(inInsertionLine, scrollingRegionEnd);
		My_ScreenRowIndex const						kMostLines = std::min(STATIC_CAST(inNumberOfLines, My_ScreenRowIndex), kLinesUntilEnd);
		My_ScreenBufferLineList::size_type const	kBufferSize = inDataPtr->screenBuffer.size();
		My_ScreenBufferLineList::iterator			pastLastKeptLine;
		
		
		// insert blank lines
		if ((kMy_AttributeRuleCopyLastLine == inAttributeRule) && (scrollingRegionEnd != scrollingRegionBegin))
		{
			My_ScreenBufferLine		lineTemplate = gEmptyScreenBufferLine();
			
			
			lineTemplate.attributeVector = (*inInsertionLine).attributeVector;
			lineTemplate.globalAttributes = (*inInsertionLine).globalAttributes;
			inDataPtr->screenBuffer.insert(inInsertionLine, kMostLines, lineTemplate);
		}
		else
		{
			inDataPtr->screenBuffer.insert(inInsertionLine, kMostLines, gEmptyScreenBufferLine());
		}
		
		// delete last lines
		pastLastKeptLine = scrollingRegionEnd;
		std::advance(pastLastKeptLine, -STATIC_CAST(kMostLines, SInt16));
		inDataPtr->screenBuffer.erase(pastLastKeptLine, scrollingRegionEnd);
		
		assert(kBufferSize == inDataPtr->screenBuffer.size());
		
		// redraw the area
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = kFirstInsertedRow;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = inDataPtr->customScrollingRegion.lastRow - range.firstRow + 1;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// bufferInsertBlankLines


/*!
Efficiently overwrites the entire buffer for the given
line, such that every character on the line becomes the
given fill character and every character’s attributes
match the given fill attributes.  You may also update
the line global attributes, which define the default
attributes effective regardless of individual character
attributes on the line.

(3.0)
*/
void
bufferLineFill	(My_ScreenBufferPtr			UNUSED_ARGUMENT(inDataPtr),
				 My_ScreenBufferLine&		inRow,
				 UInt8						inFillCharacter,
				 TerminalTextAttributes		inFillAttributes,
				 Boolean					inUpdateLineGlobalAttributesAlso)
{
	std::fill(inRow.textVectorBegin, inRow.textVectorEnd, inFillCharacter);
	std::fill(inRow.attributeVector.begin(), inRow.attributeVector.end(), inFillAttributes);
	if (inUpdateLineGlobalAttributesAlso)
	{
		inRow.globalAttributes = inFillAttributes;
	}
}// bufferLineFill


/*!
Deletes characters starting at the current cursor
position and moving forwards.  The rest of the line
is shifted to the left as a result.

(3.0)
*/
void
bufferRemoveCharactersAtCursorColumn	(My_ScreenBufferPtr		inDataPtr,
										 SInt16					inNumberOfCharactersToDelete)
{
	UInt16								numCharsToRemove = inNumberOfCharactersToDelete;
	SInt16								postWrapCursorX = inDataPtr->current.cursorX;
	My_ScreenRowIndex					postWrapCursorY = inDataPtr->current.cursorY;
	My_ScreenBufferLineList::iterator	toCursorLine;
	TerminalTextAttributes				copiedAttributes = kNoTerminalTextAttributes;
	
	
	// wrap cursor
	cursorWrapIfNecessaryGetLocation(inDataPtr, &postWrapCursorX, &postWrapCursorY);
	locateCursorLine(inDataPtr, toCursorLine);
	
	// since the caller cannot know for sure if the cursor wrapped,
	// do bounds-checking between the screen edge and new location
	if ((postWrapCursorX + numCharsToRemove) >= inDataPtr->current.returnNumberOfColumnsPermitted())
	{
		numCharsToRemove = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
	}
	
	// the VT102 specification says that the blank attributes are copied from the last character
	copiedAttributes = toCursorLine->attributeVector[inDataPtr->current.returnNumberOfColumnsPermitted() - 1];
	
	// update attributes
	{
		My_TextAttributesList::iterator		pastVisibleEnd = toCursorLine->attributeVector.begin();
		My_TextAttributesList::iterator		toCursorAttr = toCursorLine->attributeVector.begin();
		My_TextAttributesList::iterator		toFirstPreservedAttr;
		My_TextAttributesList::iterator		pastLastRelocatedAttr;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastRelocatedAttr = pastVisibleEnd;
		
		std::advance(toCursorAttr, postWrapCursorX);
		toFirstPreservedAttr = toCursorAttr;
		std::advance(toFirstPreservedAttr, numCharsToRemove);
		std::advance(pastLastRelocatedAttr, -numCharsToRemove);
		
		std::copy(toFirstPreservedAttr, pastVisibleEnd, toCursorAttr);
		std::fill(pastLastRelocatedAttr, pastVisibleEnd, copiedAttributes);
	}
	
	// update text
	{
		My_TextIterator		pastVisibleEnd = toCursorLine->textVectorBegin;
		My_TextIterator		toCursorChar = toCursorLine->textVectorBegin;
		My_TextIterator		toFirstPreservedChar;
		My_TextIterator		pastLastRelocatedChar;
		
		
		std::advance(pastVisibleEnd, inDataPtr->current.returnNumberOfColumnsPermitted());
		
		pastLastRelocatedChar = pastVisibleEnd;
		
		std::advance(toCursorChar, postWrapCursorX);
		toFirstPreservedChar = toCursorChar;
		std::advance(toFirstPreservedChar, numCharsToRemove);
		std::advance(pastLastRelocatedChar, -numCharsToRemove);
		
		std::copy(toFirstPreservedChar, pastVisibleEnd, toCursorChar);
		std::fill(pastLastRelocatedChar, pastVisibleEnd, ' ');
	}
	
	// add the entire line from the cursor to the end
	// to the text-change region; this would trigger
	// things like Terminal View updates
	//Console_WriteLine("text changed event: remove characters at cursor column");
	{
		Terminal_RangeDescription	range;
		
		
		range.screen = inDataPtr->selfRef;
		range.firstRow = postWrapCursorY;
		range.firstColumn = postWrapCursorX;
		range.columnCount = inDataPtr->current.returnNumberOfColumnsPermitted() - postWrapCursorX;
		range.rowCount = 1;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
	}
}// bufferRemoveCharactersAtCursorColumn


/*!
Deletes lines from the screen buffer, scrolling up the
remainder and inserting new blank lines at the bottom of the
scrolling region.  The display is updated.

New blank lines normally have cleared attributes.  However, if
"inAttributeRule" is set to "kMy_AttributeRuleCopyLastLine",
they will instead copy the attributes of the line that was at
the end prior to new lines being inserted.

See also bufferInsertBlankLines().

NOTE:   You cannot use this to alter the scrollback.

WARNING:	The specified line MUST be part of the terminal
			scrolling region.

(3.0)
*/
void
bufferRemoveLines	(My_ScreenBufferPtr						inDataPtr,
					 UInt16									inNumberOfLines,
					 My_ScreenBufferLineList::iterator&		inFirstDeletionLine,
					 My_AttributeRule						inAttributeRule)
{
	My_ScreenBufferLineList::iterator	scrollingRegionBegin;
	My_ScreenBufferLineList::iterator	scrollingRegionEnd;
	
	
	locateScrollingRegion(inDataPtr, scrollingRegionBegin, scrollingRegionEnd);
	if (0 != inNumberOfLines)
	{
		// the row index MUST be calculated immediately, since removing lines from the
		// buffer might make it impossible to find this at the end (where this index
		// is actually needed)
		My_ScreenRowIndex const						kFirstDeletedRow = std::distance(inDataPtr->screenBuffer.begin(), inFirstDeletionLine);
		My_ScreenRowIndex const						kLinesUntilEnd = std::distance(inFirstDeletionLine, scrollingRegionEnd);
		My_ScreenRowIndex const						kMostLines = std::min(STATIC_CAST(inNumberOfLines, My_ScreenRowIndex), kLinesUntilEnd);
		My_ScreenBufferLineList::size_type const	kBufferSize = inDataPtr->screenBuffer.size();
		My_ScreenBufferLineList::iterator			pastLastDeletionLine;
		
		
		// insert blank lines
		if ((kMy_AttributeRuleCopyLastLine == inAttributeRule) && (scrollingRegionEnd != scrollingRegionBegin))
		{
			My_ScreenBufferLine					lineTemplate = gEmptyScreenBufferLine();
			My_ScreenBufferLineList::iterator	toCopiedLine = scrollingRegionEnd;
			
			
			std::advance(toCopiedLine, -1);
			lineTemplate.attributeVector = (*toCopiedLine).attributeVector;
			lineTemplate.globalAttributes = (*toCopiedLine).globalAttributes;
			inDataPtr->screenBuffer.insert(scrollingRegionEnd, kMostLines, lineTemplate);
		}
		else
		{
			inDataPtr->screenBuffer.insert(scrollingRegionEnd, kMostLines, gEmptyScreenBufferLine());
		}
		
		// delete first lines
		pastLastDeletionLine = inFirstDeletionLine;
		std::advance(pastLastDeletionLine, kMostLines);
		inDataPtr->screenBuffer.erase(inFirstDeletionLine, pastLastDeletionLine);
		
		assert(kBufferSize == inDataPtr->screenBuffer.size());
		
		// redraw the area
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = kFirstDeletedRow;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = inDataPtr->customScrollingRegion.lastRow - range.firstRow + 1;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// bufferRemoveLines


/*!
Internal version of Terminal_ChangeLineAttributes().

(3.0)
*/
void
changeLineAttributes	(My_ScreenBufferPtr			inDataPtr,
						 My_ScreenBufferLine&		inRow,
						 TerminalTextAttributes		inSetTheseAttributes,
						 TerminalTextAttributes		inClearTheseAttributes)
{
	changeLineRangeAttributes(inDataPtr, inRow, 0/* first column */, -1/* last column; negative means “very end” */,
								inSetTheseAttributes, inClearTheseAttributes);
}// changeLineAttributes


/*!
Sets attributes that are global to a line.  Unlike
changeLineAttributes() which merely copies attributes
to each character, this routine also remembers the
specified changes as attributes for the entire line
so that every time the cursor enters the line, the
cursor attributes inherit the line’s global attributes.

(3.0)
*/
void
changeLineGlobalAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 TerminalTextAttributes		inSetTheseAttributes,
							 TerminalTextAttributes		inClearTheseAttributes)
{
	// first copy the attributes to every character in the line
	changeLineAttributes(inDataPtr, inRow, inSetTheseAttributes, inClearTheseAttributes);
	
	// now remember these changes so that the cursor can inherit them automatically;
	// currently attributes for scrollback lines do not exist, but negative indices
	// are allowed; to avoid array overflow, check for nonnegativity here
	STYLE_REMOVE(inRow.globalAttributes, inClearTheseAttributes);
	STYLE_ADD(inRow.globalAttributes, inSetTheseAttributes);
}// changeLineGlobalAttributes


/*!
Internal version of Terminal_ChangeLineRangeAttributes().

(3.0)
*/
void
changeLineRangeAttributes	(My_ScreenBufferPtr			inDataPtr,
							 My_ScreenBufferLine&		inRow,
							 UInt16						inZeroBasedStartColumn,
							 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
							 TerminalTextAttributes		inSetTheseAttributes,
							 TerminalTextAttributes		inClearTheseAttributes)
{
	SInt16				pastTheEndColumn = (inZeroBasedPastTheEndColumnOrNegativeForLastColumn < 0)
											? inDataPtr->text.visibleScreen.numberOfColumnsAllocated
											: inZeroBasedPastTheEndColumnOrNegativeForLastColumn;
	register SInt16		i = 0;
	
	
	// update attributes for the specified columns of the given line
	for (i = inZeroBasedStartColumn; i < pastTheEndColumn; ++i)
	{
		STYLE_REMOVE(inRow.attributeVector[i], inClearTheseAttributes);
		STYLE_ADD(inRow.attributeVector[i], inSetTheseAttributes);
	}
	
	// update current attributes too, if the cursor is in the given range
	if ((inZeroBasedStartColumn <= inDataPtr->current.cursorX) && (inDataPtr->current.cursorX < pastTheEndColumn))
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		if (inRow == *cursorLineIterator)
		{
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, inClearTheseAttributes);
			STYLE_ADD(inDataPtr->current.drawingAttributes, inSetTheseAttributes);
			
			// ...however, do not propagate text highlighting to text rendered from now on
			STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeSelected);
		}
	}
	
	// finally, if the end column is beyond the last character in the line,
	// apply the attributes to EVERY column that does not have a valid character
	// UNIMPLEMENTED
}// changeLineRangeAttributes


/*!
Notifies all listeners for the specified Terminal
state change, passing the given context to the
listener.

IMPORTANT:	The context must make sense for the
			type of change; see "Terminal.h" for
			the type of context associated with
			each terminal change.

(3.0)
*/
void
changeNotifyForTerminal		(My_ScreenBufferConstPtr	inPtr,
							 Terminal_Change			inWhatChanged,
							 void*						inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified terminal’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForTerminal


/*!
Responds to a CSI (control sequence inducer) by reinitializing
all parameter values and resetting the current parameter index
to 0.

(3.0)
*/
void
clearEscapeSequenceParameters	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		parameterIndex = kMy_MaximumANSIParameters;
	
	
	while (--parameterIndex >= 0)
	{
		inDataPtr->emulator.parameterValues[parameterIndex] = -1;
	}
	inDataPtr->emulator.parameterEndIndex = 0;
}// clearEscapeSequenceParameters


/*!
Restores the last saved cursor position and
attribute settings.  See cursorSave() to ensure
this function does the opposite.

(2.6)
*/
void
cursorRestore  (My_ScreenBufferPtr		inPtr)
{
	moveCursor(inPtr, inPtr->previous.cursorX, inPtr->previous.cursorY);
	
	// ??? this will clear the current graphics character set too, is that supposed to happen ???
	inPtr->current.drawingAttributes = inPtr->previous.drawingAttributes;
}// cursorRestore


/*!
Saves the current cursor position and attribute
settings.  See cursorRestore() to ensure this
function does the opposite.

(2.6)
*/
void
cursorSave  (My_ScreenBufferPtr  inPtr)
{
	inPtr->previous.cursorX = inPtr->current.cursorX;
	inPtr->previous.cursorY = inPtr->current.cursorY;
	inPtr->previous.drawingAttributes = inPtr->current.drawingAttributes;
}// cursorSave


/*!
Returns the cursor location, first wrapping it to the
following line if it is out of bounds (if a wrap is
pending).

(3.0)
*/
void
cursorWrapIfNecessaryGetLocation	(My_ScreenBufferPtr		inDataPtr,
									 SInt16*				outCursorXPtr,
									 My_ScreenRowIndex*		outCursorYPtr)
{
	if (inDataPtr->current.cursorX >= inDataPtr->current.returnNumberOfColumnsPermitted()) 
	{
		moveCursorX(inDataPtr, 0);
		moveCursorDownOrScroll(inDataPtr);
	}
	*outCursorXPtr = inDataPtr->current.cursorX;
	*outCursorYPtr = inDataPtr->current.cursorY;
}// cursorWrapIfNecessaryGetLocation


/*!
Treats the specified string as “verbatim”, sending the
characters wherever they need to go (any open print jobs
or capture files, the terminal, etc.).

(4.0)
*/
void
echoCFString	(My_ScreenBufferPtr		inDataPtr,
				 CFStringRef			inString)
{
	CFIndex const	kLength = CFStringGetLength(inString);
	Boolean const	kPrinterOnly = (0 != (inDataPtr->printingModes & kMy_PrintingModePrintController));
	
	
	// append to capture file, if one is open; try to avoid conversion,
	// but if necessary convert the bytes into a Unicode format
	if ((nullptr != inDataPtr->captureStream) || (nullptr != inDataPtr->printingStream))
	{
		CFStringEncoding const		kDesiredEncoding = kCFStringEncodingUTF8;
		CFIndex						bytesNeeded = 0;
		UInt8 const*				bufferReadOnly = REINTERPRET_CAST(CFStringGetCStringPtr(inString, kDesiredEncoding),
																		UInt8 const*);
		UInt8*						buffer = nullptr;
		Boolean						freeBuffer = false;
		
		
		if (nullptr != bufferReadOnly)
		{
			bytesNeeded = CPP_STD::strlen(REINTERPRET_CAST(bufferReadOnly, char const*));
		}
		else
		{
			CFIndex		conversionResult = CFStringGetBytes(inString, CFRangeMake(0, kLength),
															kDesiredEncoding, '?'/* loss byte */,
															true/* is external representation */,
															nullptr/* buffer; do not use, just find size */, 0/* buffer size, ignored */,
															&bytesNeeded);
			
			
			if (conversionResult > 0)
			{
				buffer = new UInt8[bytesNeeded];
				freeBuffer = true;
				
				conversionResult = CFStringGetBytes(inString, CFRangeMake(0, kLength),
													kDesiredEncoding, '?'/* loss byte */,
													true/* is external representation */,
													buffer, bytesNeeded, &bytesNeeded);
				assert(conversionResult > 0);
				
				bufferReadOnly = buffer;
			}
		}
		
		if (nullptr != bufferReadOnly)
		{
			if ((false == kPrinterOnly) && (nullptr != inDataPtr->captureStream))
			{
				StreamCapture_WriteUTF8Data(inDataPtr->captureStream, bufferReadOnly, bytesNeeded);
			}
			if (nullptr != inDataPtr->printingStream)
			{
				StreamCapture_WriteUTF8Data(inDataPtr->printingStream, bufferReadOnly, bytesNeeded);
			}
		}
		
		if (freeBuffer)
		{
			delete [] buffer, buffer = nullptr;
		}
	}
	
	// add each character to the terminal at the current cursor position, advancing
	// the cursor each time (and therefore being mindful of wrap settings, etc.)
	if (false == kPrinterOnly)
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		register SInt16						preWriteCursorX = inDataPtr->current.cursorX;
		register My_ScreenRowIndex			preWriteCursorY = inDataPtr->current.cursorY;
		TerminalTextAttributes				temporaryAttributes = 0;
		CFStringInlineBuffer				inlineBuffer;
		
		
		CFStringInitInlineBuffer(inString, &inlineBuffer, CFRangeMake(0, kLength));
		
		// WARNING: This is done once here, for efficiency, and is only
		//          repeated below if the cursor actually moves vertically
		//          (as evidenced by some moveCursor...() call that would
		//          affect the cursor row).  Keep this in sync!!!
		locateCursorLine(inDataPtr, cursorLineIterator);
		for (CFIndex i = 0; i < kLength; ++i)
		{
			UniChar			thisCharacter = CFStringGetCharacterFromInlineBuffer(&inlineBuffer, i);
			CFIndex const	kCharacterCountToCompose = CFStringGetRangeOfComposedCharactersAtIndex(inString, i).length;
			
			
			// compose the character for display purposes
			// IMPORTANT: this is a bit of a hack, as it is technically possible
			// for Unicode combinations to have no single character equivalent
			// (i.e. they can only be described in decomposed form); however, this
			// is rare; for now composition is considered an acceptable work-around
			if (kCharacterCountToCompose > 1)
			{
				CFRetainRelease		composedCharacter(CFStringCreateMutable(kCFAllocatorDefault, kCharacterCountToCompose),
														true/* is retained */);
				
				
				for (CFIndex j = i; j < (i + kCharacterCountToCompose); ++j)
				{
					UniChar const	kNextChar = CFStringGetCharacterFromInlineBuffer(&inlineBuffer, j);
					
					
					CFStringAppendCharacters(composedCharacter.returnCFMutableStringRef(), &kNextChar, 1);
				}
				CFStringNormalize(composedCharacter.returnCFMutableStringRef(), kCFStringNormalizationFormC);
				thisCharacter = CFStringGetCharacterAtIndex(composedCharacter.returnCFStringRef(), 0);
			}
			
		#if 0
			// debug
			{
				CFRetainRelease		s(CFStringCreateWithCharacters(kCFAllocatorDefault, &thisCharacter, 1), true);
				
				
				Console_WriteValueCFString("echo character: glyph", s.returnCFStringRef());
				Console_WriteValue("echo character: value", thisCharacter);
				Console_WriteValue("echo character: count", kCharacterCountToCompose);
			}
		#endif
			
			// if the cursor was about to wrap on the previous
			// write, perform that wrap now
			if (inDataPtr->wrapPending)
			{
				// autowrap to start of next line
				moveCursorLeftToEdge(inDataPtr);
				moveCursorDownOrScroll(inDataPtr);
				locateCursorLine(inDataPtr, cursorLineIterator); // cursor changed rows...
				
				// reset column tracker
				preWriteCursorX = 0;
			}
			
			// write characters on a single line
			if (inDataPtr->modeInsertNotReplace)
			{
				bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, 1/* number of blank characters */);
			}
			cursorLineIterator->textVectorBegin[inDataPtr->current.cursorX] = translateCharacter(inDataPtr, thisCharacter,
																									inDataPtr->current.drawingAttributes,
																									temporaryAttributes);
			cursorLineIterator->attributeVector[inDataPtr->current.cursorX] = temporaryAttributes;
			
			if (false == inDataPtr->wrapPending)
			{
				if (inDataPtr->current.cursorX < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1))
				{
					// advance the cursor position
					moveCursorRight(inDataPtr);
				}
				else
				{
					// hit right margin
					if (inDataPtr->modeAutoWrap)
					{
						// the cursor just arrived here, so set up a pending
						// wrap-and-scroll; it will only occur the next time
						// data is actually written
						inDataPtr->wrapPending = true;
					}
					else
					{
						// stay at right margin
						moveCursorRightToEdge(inDataPtr);
					}
				}
			}
			
			// when characters are composed (e.g. a letter followed by its accent),
			// ALL of the values used to produce the single, visible glyph should
			// be skipped in the buffer, while still corresponding to a single
			// position from the user’s point of view, e.g. cursor only moves once
			i += (kCharacterCountToCompose - 1/* loop has a ++i by default */);
		}
		
		// end of data; notify of a change (this will cause things like Terminal View updates)
		{
			// add the new line of text to the text-change region;
			// this should trigger things like Terminal View updates
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = preWriteCursorY;
			if (preWriteCursorY != inDataPtr->current.cursorY)
			{
				// more than one line; just draw all lines completely
				range.firstColumn = 0;
				range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
				range.rowCount = inDataPtr->current.cursorY - preWriteCursorY + 1;
			}
			else
			{
				range.firstColumn = preWriteCursorX;
				if (inDataPtr->modeInsertNotReplace)
				{
					// invalidate the rest of the line
					range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted - preWriteCursorX;
				}
				else
				{
					range.columnCount = inDataPtr->current.cursorX - preWriteCursorX + 1;
				}
				range.rowCount = 1;
			}
			//Console_WriteValuePair("text changed event: add data starting at row, column", range.firstRow, range.firstColumn);
			//Console_WriteValuePair("text changed event: add data for #rows, #columns", range.rowCount, range.columnCount);
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
}// echoCFString


/*!
Old terminal emulator implementation.  (Ugly, huh?)
MacTelnet 3.1 finally scraps this implementation and
moves to the fast, flexible and correct-by-construction
callback-based mechanism.  For more information, see
Terminal_EmulatorProcessData().

(2.6)
*/
void
emulatorFrontEndOld	(My_ScreenBufferPtr		inDataPtr,
					 UInt8 const*			inBuffer,
					 SInt32					inLength)
{
	enum
	{
		ASCII_ESC	= 0x1B		//!< escape character
	};
	register SInt16				escflg = 0;//inDataPtr->current.escapeSequence.level;
    SInt32						ctr = inLength; // 3.0 - use a COPY so string length parameter’s original value isn’t lost!!!
	UInt8 const*				c = inBuffer;
	My_CharacterSetInfoPtr		characterSetInfoPtr = nullptr; // which character set (G0 or G1) is being modified; used only for shift-in, -out
	
	
	while (ctr > 0)
	{
		//if (inDataPtr->printing.enabled)
		//{
		//	TelnetPrinting_Spool(&inDataPtr->printing, c, &ctr/* count can decrease, depending on what is printed */);
		//}
		
		while ((escflg == 0) && (ctr > 0) && (*c < 32))
		{
			switch (*c)
			{
			case 0x07: // control-G; sound bell
				unless (inDataPtr->bellDisabled)
				{
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeAudioEvent, inDataPtr->selfRef/* context */);
				}
				break;
			
			case 0x08: // control-H; backspace
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr); // do not extend past margin
				break;
			
			case 0x09: // control-I; horizontal tab
				moveCursorRightToNextTabStop(inDataPtr);
				StreamCapture_WriteUTF8Data(inDataPtr->captureStream, c, 1);
				break;
			
			case 0x0A: // control-J; line feed
			case 0x0B: // control-K; vertical tab
			case 0x0C: // control-L; form feed
				// all of these are interpreted the same for VT100;
				// if LNM was received, this is a regular line feed,
				// otherwise it is actually a new-line operation
				moveCursorDownOrScroll(inDataPtr);
				if (inDataPtr->modeNewLineOption) moveCursorLeftToEdge(inDataPtr);
				break;
			
			case 0x0D: // control-M; carriage return
				moveCursorLeftToEdge(inDataPtr);
				StreamCapture_WriteUTF8Data(inDataPtr->captureStream, c, 1);
				break;
			
			case 0x0E: // control-N; shift-out
				inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG1;
				if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
				{
					// set attribute
					STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
				}
				else
				{
					// clear attribute
					STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
				}
				break;
			
			case 0x0F: // control-O; shift-in
				inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
				if (inDataPtr->current.characterSetInfoPtr->graphicsMode == kMy_GraphicsModeOn)
				{
					// set attribute
					STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
				}
				else
				{
					// clear attribute
					STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics);
				}
				break;
			
			case 0x13: // control-S; resume transmission (XON)
				break;
			
			case 0x15: // control-U; stop transmission (XOFF) except for XON and XOFF
				break;
			
			case ASCII_ESC: // escapes are significant
				++escflg;
				break;
			
			default:
				// ignore
				break;
			} 
			++c;
			--ctr;
		} 
		if ((escflg == 0) &&
			(ctr > 0) &&
			(*c & 0x80) &&
			(*c < 0xA0) &&
			(inDataPtr->emulator.primaryType == kTerminal_EmulatorVT220)) // VT220 eightbit starts here
		{											
			switch (*c)								
			{									
			case 0x84: /* ind */		//same as ESC D
				moveCursorDownOrScroll(inDataPtr);		
				goto ShortCut;			
			
			case 0x85: /* nel */		//same as ESC E
				moveCursorLeftToEdge(inDataPtr);
				moveCursorDownOrScroll(inDataPtr);				
				goto ShortCut;			
			
			case 0x88: /* hts */		//same as ESC H 
				inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;	
				goto ShortCut;	
			
			case 0x8D: /* ri */			//same as ESC M
				moveCursorUpOrScroll(inDataPtr);
				goto ShortCut;
			
			case 0x9B: /* csi */		//same as ESC [ 
				clearEscapeSequenceParameters(inDataPtr);			
				escflg = 2;			
				++c;			//CCP			
				--ctr;					
				break;						
			
			case 0x86: /* ssa */			// - same as ESC F */
			case 0x87: /* esa */			// - same as ESC G */
			case 0x8E: /* ss2 */			// - same as ESC N */
			case 0x8F: /* ss3 */			// - same as ESC O */
			case 0x90: /* dcs */			// - same as ESC P */
			case 0x93: /* sts */			// - same as ESC S */
			case 0x96: /* spa */			// - same as ESC V */
			case 0x97: /* epa */			// - same as ESC W */
			case 0x9D: /* osc */			// - same as ESC ] */
			case 0x9E: /* pm */				// - same as ESC ^ */
			case 0x9F: /* apc */			// - same as ESC _ */
			default:
				goto ShortCut;				
			} 					
		}// end if vt220
		
		{
			UniChar*							startPtr = nullptr;
			My_TextIterator						startIterator = nullptr;
			TerminalTextAttributes				attrib = 0;
			TerminalTextAttributes				temporaryAttributes = 0;
			register SInt16						preWriteCursorX = 0;
			SInt16								extra = 0;
			Boolean								wrapped = false;
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			// WARNING: This is done once here, for efficiency, and is only
			//			repeated below if the cursor actually moves vertically
			//			(as evidenced by some moveCursor...() call that would
			//			affect the cursor row).  Keep this in sync!!!
			locateCursorLine(inDataPtr, cursorLineIterator);
			
			while ((escflg == 0) &&
					(ctr > 0) &&
					(*c >= 32) &&
					!((*c & 0x80) && (*c < 0xA0) && (inDataPtr->emulator.primaryType == kTerminal_EmulatorVT220)))
			{
				// print lines of text one at a time
				attrib = inDataPtr->current.drawingAttributes; /* current writing attribute */
				wrapped = false; /* wrapped to next line (boolean) */
				extra = 0; /* overwriting last character of line  */
				
				if (inDataPtr->current.cursorX >= inDataPtr->current.returnNumberOfColumnsPermitted())
				{
					if (inDataPtr->modeAutoWrap)
					{
						// wrap to next line 
						moveCursorLeftToEdge(inDataPtr);
						moveCursorDownOrScroll(inDataPtr);
						locateCursorLine(inDataPtr, cursorLineIterator); // cursor changed rows...
					}
					else
					{
						// stay at right margin
						moveCursorX(inDataPtr, inDataPtr->current.returnNumberOfColumnsPermitted() - 1);
					}
				}
				
				startPtr = cursorLineIterator->textVectorBegin + inDataPtr->current.cursorX;
				startIterator = cursorLineIterator->textVectorBegin;
				std::advance(startIterator, inDataPtr->current.cursorX);
				preWriteCursorX = inDataPtr->current.cursorX;
				while ((ctr > 0) &&
						(*c >= 32) &&
						(!wrapped) &&
						!((*c & 0x80) && (*c < 0xA0) && (inDataPtr->emulator.primaryType == kTerminal_EmulatorVT220)))
				{
					// write characters on a single line
					if (inDataPtr->modeInsertNotReplace)
					{
						bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, 1/* number of blank characters */);
					}
					cursorLineIterator->textVectorBegin[inDataPtr->current.cursorX] = translateCharacter(inDataPtr, *c, attrib,
																											temporaryAttributes);
					cursorLineIterator->attributeVector[inDataPtr->current.cursorX] = temporaryAttributes;
					++c;
					--ctr;
					if (inDataPtr->current.cursorX < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1))
					{
						// advance the cursor position
						moveCursorRight(inDataPtr);
					}
					else
					{
						// hit right margin
						if (inDataPtr->modeAutoWrap)
						{
							// autowrap to start of next line (past-the-end cursor position triggers this)
							moveCursorRight(inDataPtr);
							wrapped = true; // terminate inner loop 
						}
						else
						{
							// stay at right margin
							moveCursorRightToEdge(inDataPtr);
							extra = 1; // cursor position doesn’t advance 
						} 
					} 
				}
				
				// line has terminated, or invisible character arrived - update the terminal view(s)
				// to display the printable characters received in the preceding loop
				extra += inDataPtr->current.cursorX - preWriteCursorX;
				if (*c == '\015')
				{
					// add the new line of text to the text-change region;
					// this should trigger things like Terminal View updates
					Terminal_RangeDescription	range;
					
					
					//Console_WriteValue("text changed event: add new line, terminated by character", *c);
					range.screen = inDataPtr->selfRef;
					range.firstRow = inDataPtr->current.cursorY;
					range.firstColumn = preWriteCursorX;
					range.columnCount = extra;
					range.rowCount = 1;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
				}
				
				StreamCapture_WriteUTF8Data(inDataPtr->captureStream, (UInt8*)startPtr/* data to write */, extra/* buffer length */);
			}/* while */
		}
		
		// 3.0 - absorb any number of consecutive escape characters, they are meaningless
		while ((escflg == 1) && (*c == ASCII_ESC) && (ctr > 0))
		{
			++c;
			--ctr;
		}
		
		while ((escflg == 1) && (ctr > 0))
		{
			// Basic escape sequence processing.  This indicates that an isolated
			// ESC character (for a new sequence) has been read, but nothing else.
			//
			// IMPORTANT: Be sure to consult "inDataPtr->modeANSIEnabled" for each
			//            of the following, because ANSI and VT52-compatible modes
			//            are combined in the switch below.  Do not respond to ANSI
			//            sequences while in VT52 mode, and vice-versa.
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '[':
				if (inDataPtr->modeANSIEnabled)
				{
					// CSI (control sequence inducer)
					clearEscapeSequenceParameters(inDataPtr);
					++escflg;
				}
				break;
			
			case ']':
				if (inDataPtr->modeANSIEnabled)
				{
					//if (ScreenFactory_GetConnectionDataOf(inDataPtr->selfRef)->Xterm) escflg = 6;
					escflg = 6; // TEMP
				}
				else
				{
					// print screen
					Commands_ExecuteByID(kCommandPrintScreen);
				}
				break;
			
			case '7':
				if (inDataPtr->modeANSIEnabled) cursorSave(inDataPtr);
				goto ShortCut;				
			
			case '8':
				if (inDataPtr->modeANSIEnabled) cursorRestore(inDataPtr);
				goto ShortCut;				
			
			case 'c':
				if (inDataPtr->modeANSIEnabled) resetTerminal(inDataPtr);
				break;
			
			case 'A':
				unless (inDataPtr->modeANSIEnabled) My_VT100::VT52::cursorUp(inDataPtr);
				goto ShortCut;
			
			case 'B':
				unless (inDataPtr->modeANSIEnabled) My_VT100::VT52::cursorDown(inDataPtr);
				goto ShortCut;
			
			case 'C':
				unless (inDataPtr->modeANSIEnabled) My_VT100::VT52::cursorForward(inDataPtr);
				goto ShortCut;
			
			case 'D':
				if (inDataPtr->modeANSIEnabled) moveCursorDownOrScroll(inDataPtr);
				else My_VT100::VT52::cursorBackward(inDataPtr);
				goto ShortCut;				
			
			case 'E':
				if (inDataPtr->modeANSIEnabled)
				{
					moveCursorLeftToEdge(inDataPtr);
					moveCursorDownOrScroll(inDataPtr);
				}
				goto ShortCut;
			
			case 'F':
				unless (inDataPtr->modeANSIEnabled) { /* enter graphics mode; unimplemented */ }
				goto ShortCut;
			
			case 'G':
				unless (inDataPtr->modeANSIEnabled) { /* exit graphics mode; unimplemented */ }
				goto ShortCut;
			
			case 'H':
				if (inDataPtr->modeANSIEnabled) inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabSet;
				else moveCursor(inDataPtr, 0, 0); // home cursor in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'I':
				unless (inDataPtr->modeANSIEnabled) moveCursorUpOrScroll(inDataPtr); // reverse line feed in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'J':
				unless (inDataPtr->modeANSIEnabled) bufferEraseFromCursorToEnd(inDataPtr); // erase to end of screen, in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'K':
				unless (inDataPtr->modeANSIEnabled) bufferEraseFromCursorColumnToLineEnd(inDataPtr); // erase to end of line, in VT52 compatibility mode of VT100
				goto ShortCut;
			
			case 'M':
				if (inDataPtr->modeANSIEnabled) moveCursorUpOrScroll(inDataPtr);
				goto ShortCut;
			
			case 'V':
				// print cursor line
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'W':
				// enter printer controller mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'X':
				// exit printer controller mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case 'Y':
				unless (inDataPtr->modeANSIEnabled)
				{
					// direct cursor address in VT52 compatibility mode of VT100;
					// new cursor position is encoded as the next two characters
					// (vertical first, then horizontal) offset by the octal
					// value 037 (equal to decimal 31)
					SInt16				newX = 0;
					My_ScreenRowIndex	newY = 0;
					
					
					newY = *(++c) - 32/* - 31 - 1 to convert from one-based to zero-based */;
					if (--ctr > 0)
					{
						newX = *(++c) - 32/* - 31 - 1 to convert from one-based to zero-based */;
						--ctr;
					}
					
					// constrain the value and then change it safely
					if (newX < 0) newX = 0;
					if (newX >= inDataPtr->current.returnNumberOfColumnsPermitted())
					{
						newX = inDataPtr->current.returnNumberOfColumnsPermitted() - 1;
					}
					//if (newY < 0) newY = 0;
					if (newY >= inDataPtr->screenBuffer.size())
					{
						newY = inDataPtr->screenBuffer.size() - 1;
					}
					moveCursor(inDataPtr, newX, newY);
				}
				goto ShortCut;
			
			case 'Z':
				if (inDataPtr->modeANSIEnabled) My_VT100::deviceAttributes(inDataPtr);
				else My_VT100::VT52::identify(inDataPtr);
				goto ShortCut;
			
			case '=':
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeApplicationKeys = true; // enter alternate keypad mode (application key sequences)
				goto ShortCut;
			
			case '>':
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeApplicationKeys = false; // exit alternate keypad mode (regular keypad characters)
				goto ShortCut;
			
			case '<':
				// enter ANSI mode (from VT52-compatible mode)
				unless (inDataPtr->modeANSIEnabled) inDataPtr->modeANSIEnabled = true;
				goto ShortCut;
			
			case '^':
				// enter auto-print mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case '_':
				// exit auto-print mode
				unless (inDataPtr->modeANSIEnabled) { /* unimplemented */ }
				goto ShortCut;
			
			case ' ':
			case '*':
			case '#':
				if (inDataPtr->modeANSIEnabled) escflg = 3;
				break;
			
			case '(':
				if (inDataPtr->modeANSIEnabled)
				{
					characterSetInfoPtr = &inDataPtr->vtG0;
					escflg = 4;
				}
				break;
			
			case ')':
				if (inDataPtr->modeANSIEnabled)
				{
					characterSetInfoPtr = &inDataPtr->vtG1;
					escflg = 4;
				}
				break;
			
			default:
				goto ShortCut;				
			}
			++c;
			--ctr;
		}/* while */
		
		while ((escflg == 2) && (ctr > 0))
		{
			// “control sequence” processing
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				// parse numeric parameter
				{
					// rename this incredibly long expression, since it’s needed a lot here!
					SInt16&		valueRef = inDataPtr->emulator.parameterValues
											[inDataPtr->emulator.parameterEndIndex];
					
					
					if (valueRef < 0) valueRef = 0;
					valueRef *= 10;
					valueRef += *c - '0';
				}
				break;
			
		#if 0
			case '=':
				// SCO-ANSI
				escflg = 9;
				break;
		#endif
			
			case '?':
				// DEC-private control sequence (see 'h' and 'l', that respectively do mode set/reset)
				inDataPtr->emulator.parameterValues[(inDataPtr->emulator.parameterEndIndex)++] = -2;
				break;
			
			case ';':
				// parameter separator
				++(inDataPtr->emulator.parameterEndIndex);
				break;
			
			case 'A':
				My_VT100::cursorUp(inDataPtr);
				goto ShortCut;				
			
			case 'B':
				My_VT100::cursorDown(inDataPtr);
				goto ShortCut;				
			
			case 'C':
				My_VT100::cursorForward(inDataPtr);
				goto ShortCut;
			
			case 'D':
				My_VT100::cursorBackward(inDataPtr);
				goto ShortCut;
			
			case 'f':
			case 'H':
				// absolute cursor positioning
				{
					SInt16				newX = (0 == inDataPtr->emulator.parameterValues[1])
												? 0
												: (inDataPtr->emulator.parameterValues[1] != -1)
													? inDataPtr->emulator.parameterValues[1] - 1
													: 0/* default is home */;
					My_ScreenRowIndex	newY = (0 == inDataPtr->emulator.parameterValues[0])
												? 0
												: (inDataPtr->emulator.parameterValues[0] != -1)
													? inDataPtr->emulator.parameterValues[0] - 1
													: 0/* default is home */;
					
					
					// offset according to the scrolling region (if any)
					newY += inDataPtr->originRegionPtr->firstRow;
					
					// the new values are not checked for violation of constraints
					// because constraints (including current origin mode) are
					// automatically enforced by moveCursor...() routines
					moveCursor(inDataPtr, newX, newY);
				}
				goto ShortCut;				
		   
			case 'i': // media copy
				//if (inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex] == 5)
				//{		
				//	TelnetPrinting_Begin(&inDataPtr->printing);
				//}
				escflg = 0;
				break;
			
			case 'J':
				My_VT100::eraseInDisplay(inDataPtr);
				goto ShortCut;
			
			case 'K':
				My_VT100::eraseInLine(inDataPtr);
				goto ShortCut;
			
			case 'm':
				// ANSI colors and other character attributes
				{
					SInt16		i = 0;
					
					
					while (i <= inDataPtr->emulator.parameterEndIndex)
					{
						SInt16		p = 0;
						
						
						if (inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex] < 0)
						{
							inDataPtr->emulator.parameterValues[inDataPtr->emulator.parameterEndIndex] = 0;
						}
						
						p = inDataPtr->emulator.parameterValues[i];
						
						// Note that a real VT100 will only understand 0-7 here.
						// Other values are basically recognized because they are
						// compatible with VT100 and are very often used (ANSI
						// colors in particular).
						if (p == 0) STYLE_REMOVE(inDataPtr->current.drawingAttributes, kAllStyleOrColorTerminalTextAttributes); // all style bits off
						else if (p < 9) STYLE_ADD(inDataPtr->current.drawingAttributes, styleOfVTParameter(p)); // set attribute
						else if (p == 10) { /* set normal font - unsupported */ }
						else if (p == 11) { /* set alternate font - unsupported */ }
						else if (p == 12) { /* set alternate font, shifting by 128 - unsupported */ }
						else if (p == 22) STYLE_REMOVE(inDataPtr->current.drawingAttributes, styleOfVTParameter(1)); // clear bold (oddball - 22, not 21)
						else if ((p > 22) && (p < 29)) STYLE_REMOVE(inDataPtr->current.drawingAttributes, styleOfVTParameter(p - 20)); // clear attribute
						else
						{
							if ((p >= 30) && (p <= 38))
							{
								STYLE_SET_FOREGROUND_INDEX(inDataPtr->current.drawingAttributes, p - 30);
							}
							else if ((p >= 40) && (p <= 48))
							{
								STYLE_SET_BACKGROUND_INDEX(inDataPtr->current.drawingAttributes, p - 40);
							}
						}
						++i;
					}
				}
				goto ShortCut;
			
			case 't':
				// XTerm window modification and state reporting
				{
					SInt16 const&	instruction = inDataPtr->emulator.parameterValues[0];
					
					
					switch (instruction)
					{
					case 1:
						// de-iconify window
						inDataPtr->windowMinimized = false;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowMinimization, inDataPtr->selfRef/* context */);
						break;
					
					case 2:
						// iconify window
						inDataPtr->windowMinimized = true;
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowMinimization, inDataPtr->selfRef/* context */);
						break;
					
					default:
						// SEE DISABLED BELOW
						break;
					}
				}
			#if 0 // TEMP
				{
					WindowRef		window = TerminalView_GetWindow(inDataPtr->view);
					SInt16 const&	instruction = inDataPtr->emulator.parameterValues[0];
					
					
					// determine which modifier was received
					switch (instruction)
					{
					//case 1 above
					//case 2 above
					
					case 3:
						// move window (X, Y coordinates are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->emulator.parameterEndIndex == 2)
						{
							SInt16		pixelLeft = inDataPtr->emulator.parameterValues[1];
							SInt16		pixelTop = inDataPtr->emulator.parameterValues[2];
							Rect		screenBounds;
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, use the window positioning boundaries as
							// defined by the Mac OS (excludes the Dock and menu bar,
							// etc.)
							RegionUtilities_GetPositioningBounds(window, &screenBounds);
							if (pixelLeft < screenBounds.left) pixelLeft = screenBounds.left;
							if (pixelLeft > screenBounds.right) pixelLeft = screenBounds.right;
							if (pixelTop < screenBounds.top) pixelTop = screenBounds.top;
							if (pixelTop > screenBounds.bottom) pixelTop = screenBounds.bottom;
							
							// now move the window
							MoveWindow(window, pixelLeft, pixelTop, false/* activate */);
						}
						break;
					
					case 4:
						// disabled due to instability
					#if 0
						// resize terminal area (width and height in *pixels* are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->emulator.parameterEndIndex == 2)
						{
							SInt16		pixelWidth = inDataPtr->emulator.parameterValues[1];
							SInt16		pixelHeight = inDataPtr->emulator.parameterValues[2];
							UInt16		columns = 0;
							UInt16		rows = 0;
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, use the window positioning boundaries as
							// defined by the Mac OS (excludes the Dock and menu bar,
							// etc.)
							{
								Rect	screenBounds;
								
								
								RegionUtilities_GetPositioningBounds(window, &screenBounds);
								if (pixelWidth < 10) pixelWidth = 32; // arbitrary
								if (pixelHeight < 10) pixelHeight = 32; // arbitrary
								if (pixelWidth > (screenBounds.right - screenBounds.left))
								{
									pixelWidth = (screenBounds.right - screenBounds.left);
								}
								if (pixelHeight > (screenBounds.bottom - screenBounds.top))
								{
									pixelHeight = (screenBounds.bottom - screenBounds.top);
								}
							}
							
							// determine the closest terminal size to the requested pixel width and height
							TerminalView_GetTheoreticalScreenDimensions(inDataPtr->view, pixelWidth, pixelHeight,
																		false/* include insets */, &columns, &rows);
							Terminal_SetVisibleScreenDimensions(inDataPtr->selfRef, columns, rows);
						}
					#endif
						break;
					
					case 5:
						// raise (activate) window
						EventLoop_SelectBehindDialogWindows(window);
						break;
					
					case 6:
						// lower window (become last in Z-order)
						SendBehind(window, kLastWindowOfGroup);
						break;
					
					case 7:
						// refresh (update) window
						RegionUtilities_RedrawWindowOnNextUpdate(window); // TEMPORARY; this changes too much (inefficient)
						break;
					
					case 8:
						// resize terminal area (width and height in *characters* are 2nd and 3rd parameters, respectively);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->emulator.parameterEndIndex == 2)
						{
							SInt16	columns = inDataPtr->emulator.parameterValues[1];
							SInt16  rows = inDataPtr->emulator.parameterValues[2];
							
							
							// first, constrain the values to something reasonable;
							// arbitrarily, each value must be at least 1
							if (columns < 1) columns = 1;
							if (columns > inDataPtr->text.visibleScreen.numberOfColumnsAllocated)
							{
								columns = inDataPtr->text.visibleScreen.numberOfColumnsAllocated;
							}
							if (rows < 1) rows = 1;
							if (rows > 64/* arbitrary */) rows = 64;
							
							// now resize the screen
							Terminal_SetVisibleScreenDimensions(inDataPtr->selfRef, columns, rows);
						}
						break;
					
					case 9:
						// zoom window (2nd parameter of 0 means return from maximized state, parameter of 1 means Maximize);
						// since parameters are expected, first assert that exactly the right number
						// of values was given (otherwise, the input data may be bogus)
						if (inDataPtr->emulator.parameterEndIndex == 1)
						{
							//Boolean		maximize = (inDataPtr->emulator.parameterValues[1] == 1);
							//WindowPartCode	inOrOut = (inDataPtr->emulator.parameterValues[1] == 0)
							//								? inZoomOut : inZoomIn;
							
							
							EventLoop_HandleZoomEvent(window);
						}
						break;
					
					case 11:
						// report window state; <CSI>1t if normal, <CSI>2t if iconified
						{
							SessionRef		session = returnListeningSession(inDataPtr);
							
							
							if (nullptr != session)
							{
								if (IsWindowCollapsed(window)) Session_SendData(session, "\033[2t", 4);
								else Session_SendData(session, "\033[1t", 4);
							}
						}
						break;
					
					case 10:
					case 12:
					case 13:
					case 14:
					case 15:
					case 16:
					case 17:
					case 18:
					case 19:
					case 20:
					case 21:
					case 22:
					case 23:
						// reporting function that is not currently handled
						break;
					
					default:
						// any number greater than or equal to 24 is a request to
						// change the number of terminal lines to that amount
						(Terminal_Result)setVisibleRowCount(inDataPtr, inDataPtr->emulator.parameterValues[0]);
						changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScreenSize, inDataPtr->selfRef/* context */);
						break;
					}
				}
			#endif // TEMP
				goto ShortCut;
			
			case 'q':
				My_VT100::loadLEDs(inDataPtr);
				goto ShortCut;
			
			case 'c':
				if (inDataPtr->emulator.primaryType == kTerminal_EmulatorVT220) My_VT220::deviceAttributes(inDataPtr);
				else if (inDataPtr->emulator.primaryType == kTerminal_EmulatorVT100) My_VT100::deviceAttributes(inDataPtr);
				goto ShortCut;
			
			case 'n':
				My_VT100::deviceStatusReport(inDataPtr);
				goto ShortCut;
			
			case 'L':
				{
					My_ScreenBufferLineList::iterator	cursorLineIterator;
					
					
					locateCursorLine(inDataPtr, cursorLineIterator);
					if (inDataPtr->emulator.parameterValues[0] < 1)
					{
						inDataPtr->emulator.parameterValues[0] = 1;
					}
					bufferInsertBlankLines(inDataPtr, inDataPtr->emulator.parameterValues[0], cursorLineIterator, kMy_AttributeRuleCopyLastLine);
				}
				goto ShortCut;				
			
			case 'M':
				{
					My_ScreenBufferLineList::iterator	cursorLineIterator;
					
					
					locateCursorLine(inDataPtr, cursorLineIterator);
					if (inDataPtr->emulator.parameterValues[0] < 1)
					{
						inDataPtr->emulator.parameterValues[0] = 1;
					}
					bufferRemoveLines(inDataPtr, inDataPtr->emulator.parameterValues[0], cursorLineIterator, kMy_AttributeRuleCopyLastLine);
				}
				goto ShortCut;
			
			case '@':
				if (inDataPtr->emulator.parameterValues[0] < 1)
					inDataPtr->emulator.parameterValues[0] = 1;
				bufferInsertBlanksAtCursorColumnWithoutUpdate(inDataPtr, inDataPtr->emulator.parameterValues[0]/* number of blank characters */);
				goto ShortCut;
			
			case 'P':
				if (inDataPtr->emulator.parameterValues[0] < 1)
					inDataPtr->emulator.parameterValues[0] = 1;
				bufferRemoveCharactersAtCursorColumn(inDataPtr, inDataPtr->emulator.parameterValues[0]);
				goto ShortCut;				
			
			case 'r':
				My_VT100::setTopAndBottomMargins(inDataPtr);
				goto ShortCut;
			
			case 'h':
			  	// set mode
				My_VT100::modeSetReset(inDataPtr, true/* set */);
				goto ShortCut;				
			
			case 'l':
				// reset mode
				My_VT100::modeSetReset(inDataPtr, false/* set */);
				goto ShortCut;				
			
			case 'g':
				if (inDataPtr->emulator.parameterValues[0] == 3)
				  /* clear all tabs */
					tabStopClearAll(inDataPtr);
				else if (inDataPtr->emulator.parameterValues[0] <= 0)
				  /* clear tab at current position */
					inDataPtr->tabSettings[inDataPtr->current.cursorX] = kMy_TabClear;
				goto ShortCut;				
			
			case '!':
			case '\'':
			case '\"':
				++escflg;					
				break;						
			
			default:			/* Dang blasted strays... */
				goto ShortCut;				
			}
			++c;
			--ctr;
		}/* while */

		while ((escflg == 3) && (ctr > 0))
		{
			// "#" handling
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case '3': // double width and height, top half
				
				goto ShortCut;
			
			case '4': // double width and height, bottom half
				
				goto ShortCut;
			
			case '5': // normal
				
				goto ShortCut;
			
			case '6': // double width, normal height
				
				goto ShortCut;
			
			case '8': // alignment display
				goto ShortCut;
			
			default:
				goto ShortCut;
			}
			++c;
			--ctr;
		}/* while */

		while ((escflg == 4) && (ctr > 0) && (characterSetInfoPtr != nullptr))
		{
			// "(" or ")" handling (respectively, selection of G0 or G1 character sets)
			switch (*c)
			{
			case 0x08:
				if (inDataPtr->current.cursorX > 0) moveCursorLeft(inDataPtr);
				else moveCursorLeftToEdge(inDataPtr);
				break;
			
			case 'A':
				// U.K. character set, normal ROM, no graphics
				characterSetInfoPtr->translationTable = kMy_CharacterSetVT100UnitedKingdom;
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
				goto ShortCut;
			
			case 'B':
				// U.S. character set, normal ROM, no graphics
				characterSetInfoPtr->translationTable = kMy_CharacterSetVT100UnitedStates;
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
				goto ShortCut;
			
			case '0':
				// normal ROM, graphics mode
				characterSetInfoPtr->source = kMy_CharacterROMNormal;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOn;
				STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // set graphics attribute
				goto ShortCut;
			
			case '1':
				// alternate ROM, no graphics
				characterSetInfoPtr->source = kMy_CharacterROMAlternate;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOff;
				STYLE_REMOVE(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // clear graphics attribute
				goto ShortCut;
			
			case '2':
				// alternate ROM, graphics mode
				characterSetInfoPtr->source = kMy_CharacterROMAlternate;
				characterSetInfoPtr->graphicsMode = kMy_GraphicsModeOn;
				STYLE_ADD(inDataPtr->current.drawingAttributes, kTerminalTextAttributeVTGraphics); // set graphics attribute
				goto ShortCut;				
			
			default:
				goto ShortCut;
			}
			++c;
			--ctr;
		}/* while */

        // Handle XTerm rename functions, code contributed by Bill Rausch
        // Modified by JMB to handle ESC]2; case as well.
        // 3.0 - handling many more sequences now
		if ((escflg >= 6) && (ctr > 0))
		{
			static char*	tmp = nullptr;
			static Str255	newname;
			Boolean			changeWindowTitle = false;
			Boolean			changeIconTitle = false;
			
			
			if ((escflg == 6) && ((*c == '0') || (*c == '1') || (*c == '2')))
			{
				if ((*c == '0') || (*c == '1')) changeIconTitle = true;
				if ((*c == '0') || (*c == '2')) changeWindowTitle = true;
				++escflg;
            	++c;
            	--ctr;
			}
			if ((escflg == 7) && (ctr > 0) && (*c == ';'))
			{
				--ctr;
				++c;
				++escflg;
				newname[0] = 0;
				tmp = (char*)&newname[1];
			}
			
			while ((escflg == 8) && (ctr > 0) && (*c != 07) && (*c != 033))
			{
				if (*newname < 255)
				{
            	 	*tmp++ = *c;
            		++(*newname);
            	}
            	++c;
            	--ctr;
			}
        	if ((escflg == 8) && (*c == 07 || *c == 033) && (ctr > 0))
			{
				CFStringRef		titleString = CFStringCreateWithPascalString(kCFAllocatorDefault, newname, CFStringGetSystemEncoding());
				
				
				if (changeWindowTitle)
				{
					inDataPtr->windowTitleCFString.setCFTypeRef(titleString);
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
				}
				if (changeIconTitle)
				{
					inDataPtr->iconTitleCFString.setCFTypeRef(titleString);
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowIconTitle, inDataPtr->selfRef/* context */);
				}
           		if (*c != 07)
            	{
            		// this will be undone after the "goto ShortCut" below
            		--c;
                	++ctr;
				}
				CFRelease(titleString);
            	goto ShortCut;
			}
		}/* if */
		
#if 0
		while ((escflg == 8) && (ctr > 0) && (*c != 07))
		{
			*tmp++ = *c++;
			--ctr;
			++(*newname);
		}
		if ((escflg == 8) && (*c == 07) && (ctr > 0))
		{
			CFStringRef		titleString = CFStringCreateWithPascalString(kCFAllocatorDefault, newname, CFStringGetSystemEncoding());
			
			
			if (nullptr != titleString)
			{
				inDataPtr->windowTitleCFString = titleString;
				changeNotifyForTerminal(inDataPtr, kTerminal_ChangeWindowFrameTitle, inDataPtr->selfRef/* context */);
				CFRelease(titleString);
			}
			goto ShortCut;
		}
#endif
		
		if ((escflg > 2) && (ctr > 0))
		{
ShortCut:
			escflg = 0;
			++c;
			--ctr;
		}
	}/* while (ctr > 0) */
	
	//inDataPtr->current.escapeSequence.level = escflg;
}// emulatorFrontEndOld


/*!
Erases the characters from the halfway point on the
screen to the right edge of the specified line,
clearing all attributes.  NO update events are sent.

This is required when switching to double-width
mode, which specifies that the right half of the
screen data on a double-width line be lost.

(3.0)
*/
void
eraseRightHalfOfLine	(My_ScreenBufferPtr		inDataPtr,
						 My_ScreenBufferLine&	inRow)
{
	SInt16								midColumn = INTEGER_HALVED(inDataPtr->text.visibleScreen.numberOfColumnsPermitted);
	My_TextIterator						textIterator = nullptr;
	My_TextAttributesList::iterator		attrIterator;
	
	
	// clear from halfway point to end of line
	textIterator = inRow.textVectorBegin;
	std::advance(textIterator, midColumn);
	attrIterator = inRow.attributeVector.begin();
	std::advance(attrIterator, midColumn);
	std::fill(textIterator, inRow.textVectorEnd, ' ');
	std::fill(attrIterator, inRow.attributeVector.end(), inRow.globalAttributes);
}// eraseRightHalfOfLine


/*!
Iterates over the given range of lines on the specified
terminal screen, executing a function on each of them.

The ranges are zero-based, but can be negative; 0 is the
first line of the main screen area, and -1 is the first
scrollback line.  The oldest scrollback row is a larger
negative number, the bottommost line of the visible screen
is one less than the number of rows high that the terminal
screen is.

The context is defined by you, and is passed directly to
the specified function each time it is invoked.

\retval kTerminal_ResultOK
if no error occurred

\retval kTerminal_ResultParameterError
if the screen line operation function, screen reference or
line iterator reference is invalid

\retval kTerminal_ResultNotEnoughMemory
if any line buffers are unexpectedly empty

(3.0)
*/
Terminal_Result
forEachLineDo	(TerminalScreenRef				inRef,
				 Terminal_LineRef				inStartRow,
				 UInt32							inNumberOfRowsToConsider,
				 My_ScreenLineOperationProcPtr	inDoWhat,
				 void*							inContextPtr)
{
	Terminal_Result			result = kTerminal_ResultOK;
	My_ScreenBufferPtr		screenPtr = getVirtualScreenData(inRef);
	My_LineIteratorPtr		iteratorPtr = getLineIterator(inStartRow);
	
	
	if ((inDoWhat == nullptr) || (screenPtr == nullptr) || (iteratorPtr == nullptr))
	{
		result = kTerminal_ResultParameterError;
	}
	else
	{
		UInt16		lineNumber = 0;
		Boolean		isEnd = false;
		
		
		// iterate over all lines in the range
		for (; ((lineNumber < inNumberOfRowsToConsider) && (false == isEnd));
				iteratorPtr->goToNextLine(isEnd), ++lineNumber)
		{
			invokeScreenLineOperationProc(inDoWhat, screenPtr, iteratorPtr->currentLine().textCFString.returnCFMutableStringRef(),
											lineNumber, inContextPtr);
		}
	}
	return result;
}// forEachLineDo


/*!
Given a character offset into a theoretically concatenated
buffer, returns the row and column that would refer to that
position.  Assumes each line is exactly the same number of
characters.

The "inEndOfLinePad" allows you to pretend each line ends
with zero or more characters (e.g. a new-line sequence), so
that if your buffer contains new-lines you can include them
when calculating the proper cell location.

The default results assume the main screen.  If your offset
is really for the scrollback, simply negate the row number
and subtract 1 (scrollback rows are represented as negative
numbers).

(3.1)
*/
void
getBufferOffsetCell		(My_ScreenBufferPtr		inDataPtr,
						 size_t					inBufferOffset,
						 UInt16					inEndOfLinePad,
						 UInt16&				outColumn,
						 SInt32&				outRow)
{
	outRow = inBufferOffset / (inDataPtr->text.visibleScreen.numberOfColumnsAllocated + inEndOfLinePad);
	outColumn = inBufferOffset % (inDataPtr->text.visibleScreen.numberOfColumnsAllocated + inEndOfLinePad);
}// getBufferOffsetCell


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
inline My_LineIteratorPtr
getLineIterator		(Terminal_LineRef	inIterator)
{
	return REINTERPRET_CAST(inIterator, My_LineIteratorPtr);
}// getLineIterator


/*!
Returns a pointer to the internal structure, given a
reference to it.

(3.0)
*/
inline My_ScreenBufferPtr
getVirtualScreenData	(TerminalScreenRef		inScreen)
{
	return REINTERPRET_CAST(inScreen, My_ScreenBufferPtr);
}// getVirtualScreenData


/*!
Turns on a specific terminal LED, or turns all LEDs
off (if 0 is given as the LED number).  The meaning of
an LED with a specific number is left to the caller.

Currently, only 4 LEDs are defined.  Providing an LED
number of 5 or greater will have no effect.  These 4
refer to the user-defined LEDs, so although VT100 (for
example) defines 3 other LEDs, you can only set the
user-defined ones with this routine.

Numbers less than zero are reserved.  Probably, they
will one day be used to turn off specific LEDs.

(3.0)
*/
void
highlightLED	(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inOneBasedLEDNumberOrZeroToTurnOffAllLEDs)
{
	switch (inOneBasedLEDNumberOrZeroToTurnOffAllLEDs)
	{
	case 0:
		inDataPtr->litLEDs = kMy_LEDBitsAllOff;
		break;
	
	case 1:
		inDataPtr->litLEDs |= kMy_LEDBitsLight1;
		break;
	
	case 2:
		inDataPtr->litLEDs |= kMy_LEDBitsLight2;
		break;
	
	case 3:
		inDataPtr->litLEDs |= kMy_LEDBitsLight3;
		break;
	
	case 4:
		inDataPtr->litLEDs |= kMy_LEDBitsLight4;
		break;
	
	default:
		// ???
		break;
	}
	
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeNewLEDState, inDataPtr->selfRef/* context */);
}// highlightLED


/*!
Discards all state history in the screen’s parser and creates
a single, initial state.

This is obviously done when a screen is first created, but
should also be done whenever the state determinant and state
transition routines are changed (e.g. to change the emulation
type).

(3.1)
*/
void
initializeParserStateStack	(My_EmulatorPtr		inDataPtr)
{
	inDataPtr->currentState = kMy_ParserStateInitial;
	inDataPtr->stringAccumulatorState = kMy_ParserStateInitial;
}// initializeParserStateStack


/*!
Locates the screen buffer line that the cursor is on,
providing an iterator into its list (which may be
past-the-end, that is, invalid).

This operation is linear with the size of the visible
screen area.  Using this routine instead of retaining
an iterator is recommended, since it is difficult to
properly synchronize an iterator with cursor movement,
(frequent) scroll activity and other list modifications.
Plus, the cursor line iterator only needs to be found
when data on the cursor line should actually be
manipulated.

(3.0)
*/
void
locateCursorLine	(My_ScreenBufferPtr						inDataPtr,
					 My_ScreenBufferLineList::iterator&		outCursorLine)
{
	My_ScreenBufferLineList::size_type const	kMaximumLines = inDataPtr->screenBuffer.size();
	
	
	//assert(inDataPtr->current.cursorY >= 0);
	assert(inDataPtr->current.cursorY <= kMaximumLines);
	if (inDataPtr->current.cursorY < INTEGER_HALVED(kMaximumLines))
	{
		// near the top; search from the beginning
		// (NOTE: linear search is still horrible, but this makes it less horrible)
		outCursorLine = inDataPtr->screenBuffer.begin();
		std::advance(outCursorLine, inDataPtr->current.cursorY/* zero-based */);
	}
	else
	{
		// near the bottom; search from the end
		// (NOTE: linear search is still horrible, but this makes it less horrible)
		SInt32 const	kDelta = inDataPtr->current.cursorY/* zero-based */ - kMaximumLines;
		
		
		outCursorLine = inDataPtr->screenBuffer.end();
		std::advance(outCursorLine, kDelta);
	}
}// locateCursorLine


/*!
Locates the screen buffer lines bounding the cursor and
scrolling activity (as defined by the terminal’s current
origin setting), providing iterators into the list (each
of which may be past-the-end, that is, invalid).  See
also locateScrollingRegionTop().

This operation is linear with the size of the visible
screen area.  Using this routine instead of retaining
iterators is recommended, since it is difficult to
properly synchronize an iterator with origin changes,
(frequent) scroll activity and other list modifications.
Plus, the scrolling region iterators only need to be
found rarely, in practice less frequently than most list
modifications that would invalidate retained iterators.

(3.0)
*/
void
locateScrollingRegion	(My_ScreenBufferPtr						inDataPtr,
						 My_ScreenBufferLineList::iterator&		outTopLine,
						 My_ScreenBufferLineList::iterator&		outPastBottomLine)
{
	My_ScreenBufferLineList::size_type const	kMaximumLines = inDataPtr->screenBuffer.size();
	
	
	//assert(inDataPtr->customScrollingRegion.firstRow >= 0);
	assertScrollingRegion(inDataPtr);
	
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->customScrollingRegion.firstRow/* zero-based */);
	
#if 0
	// slow, but definitely correct
	outPastBottomLine = inDataPtr->screenBuffer.begin();
	std::advance(outPastBottomLine, 1 + inDataPtr->customScrollingRegion.lastRow/* zero-based */);
#else
	{
		// note the region boundary is inclusive but past-the-end is exclusive;
		// also, for efficiency, assume this will be much closer to the end and
		// advance backwards to save some pointer iterations
		SInt32 const	kDelta = inDataPtr->customScrollingRegion.lastRow/* zero-based */ - kMaximumLines + 1/* past-end */;
		
		
		outPastBottomLine = inDataPtr->screenBuffer.end();
		std::advance(outPastBottomLine, kDelta);
	}
#endif
}// locateScrollingRegion


/*!
Locates the screen origin line, providing an iterator
into the list (which may be past-the-end, that is,
invalid).  See also locateScrollingRegion().

The origin is usually very close to the top of the
screen buffer, so although this search is technically
linear, in practice it will not be a slow operation.

(3.0)
*/
void
locateScrollingRegionTop	(My_ScreenBufferPtr						inDataPtr,
							 My_ScreenBufferLineList::iterator&		outTopLine)
{
	// find top line iterator in screen
	outTopLine = inDataPtr->screenBuffer.begin();
	std::advance(outTopLine, inDataPtr->customScrollingRegion.firstRow/* zero-based */);
}// locateScrollingRegionTop


/*!
Changes both the row and column of the cursor in
the specified terminal screen at the same time.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursor		(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inNewX,
				 My_ScreenRowIndex		inNewY)
{
	moveCursorX(inDataPtr, inNewX);
	moveCursorY(inDataPtr, inNewY);
	//Console_WriteValuePair("new cursor x, y", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
}// moveCursor


/*!
Moves the cursor to the row below its current row.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorDown		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->current.cursorY + 1);
}// moveCursorDown


/*!
Moves the cursor to the row below its current row,
or scrolls up one line if the cursor is already at
the bottom of the scrolling region.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
void
moveCursorDownOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->customScrollingRegion.lastRow)
	{
		screenScroll(inDataPtr, +1);
	}
	else if (inDataPtr->current.cursorY < (inDataPtr->screenBuffer.size() - 1))
	{
		moveCursorDown(inDataPtr);
	}
}// moveCursorDownOrScroll


/*!
Moves the cursor to the last row (constrained by
the current scrolling region).

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorDownToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->originRegionPtr->lastRow);
}// moveCursorDownToEdge


/*!
Moves the cursor to the column immediately preceding
its current column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeft		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX - 1);
}// moveCursorLeft


/*!
Moves the cursor to the first column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, 0);
}// moveCursorLeftToEdge


/*!
Moves the cursor to the middle column, but only if
this is possible by moving left.  In this way, the
cursor stays put unless it is in the right half of
the screen.  Useful for implementing double-width
text mode.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToHalf	(My_ScreenBufferPtr		inDataPtr)
{
	SInt16		halfway = INTEGER_HALVED(inDataPtr->text.visibleScreen.numberOfColumnsPermitted);
	
	
	if (inDataPtr->current.cursorX >= halfway)
	{
		moveCursorX(inDataPtr, halfway);
	}
}// moveCursorLeftToHalf


/*!
Moves the cursor in the opposite direction of a
normal tab, to find the stop that is behind it.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorLeftToNextTabStop	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX - tabStopGetDistanceFromCursor(inDataPtr, false/* forward */));
}// moveCursorLeftToNextTabStop


/*!
Moves the cursor to the column immediately following
its current column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRight		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX + 1);
}// moveCursorRight


/*!
Moves the cursor to the last column.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRightToEdge		(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.returnNumberOfColumnsPermitted() - 1);
}// moveCursorRightToEdge


/*!
Moves the cursor to the next column that has a
tab stop set for it.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorRightToNextTabStop	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorX(inDataPtr, inDataPtr->current.cursorX + tabStopGetDistanceFromCursor(inDataPtr, true/* forward */));
}// moveCursorRightToNextTabStop


/*!
Moves the cursor to the row above its current row.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorUp	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->current.cursorY - 1);
}// moveCursorUp


/*!
Moves the cursor to the row above its current row,
or scrolls down one line if the cursor is already
at the top of the scrolling region.

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
void
moveCursorUpOrScroll	(My_ScreenBufferPtr		inDataPtr)
{
	if (inDataPtr->current.cursorY == inDataPtr->customScrollingRegion.firstRow)
	{
		screenScroll(inDataPtr, -1);
	}
	else
	{
		moveCursorUp(inDataPtr);
	}
}// moveCursorUpOrScroll


/*!
Moves the cursor to the first row (constrained by
the current scrolling region).

IMPORTANT:	ALWAYS use moveCursor...() routines
			to set the cursor value; otherwise,
			cursor-dependent stuff could become
			out of sync.

(3.0)
*/
inline void
moveCursorUpToEdge	(My_ScreenBufferPtr		inDataPtr)
{
	moveCursorY(inDataPtr, inDataPtr->originRegionPtr->firstRow);
}// moveCursorUpToEdge


/*!
Changes the column of the cursor, within the boundaries of
the screen.  All cursor-dependent data is automatically
synchronized if necessary.

This has no effect if print controller mode (VT102 or later)
is active.

IMPORTANT:	ALWAYS use moveCursor...() routines to set the
			cursor value; otherwise, cursor-dependent stuff
			could become out of sync.

(3.0)
*/
inline void
moveCursorX		(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inNewX)
{
	if (0 == (inDataPtr->printingModes & kMy_PrintingModePrintController))
	{
		if ((inDataPtr->mayNeedToSaveToScrollback) && (inNewX != 0))
		{
			// once the cursor leaves the home position, there is no
			// chance of having to save the top line into scrollback
			inDataPtr->mayNeedToSaveToScrollback = false;
		}
		inDataPtr->current.cursorX = inNewX;
		if ((inDataPtr->current.cursorX == 0) && (inDataPtr->current.cursorY == 0))
		{
			// if the cursor moves into the home position, flag this
			// and prepare to possibly save the line contents if any
			// subsequent erase commands show up
			inDataPtr->mayNeedToSaveToScrollback = true;
		}
		
		// TEMPORARY - the inefficiency of this is ugly, but it
		// is definitely known to work
		{
			My_ScreenBufferLineList::iterator	cursorLineIterator;
			
			
			locateCursorLine(inDataPtr, cursorLineIterator);
			inDataPtr->current.cursorAttributes = cursorLineIterator->attributeVector[inDataPtr->current.cursorX];
		}
		
		// reset wrap flag, now that the cursor is moving
		inDataPtr->wrapPending = false;
		
		//if (inDataPtr->cursorVisible)
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
		}
		
	#if 0
		// DEBUG
		Console_WriteValuePair("cursor trace (dx)", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
	#endif
	}
}// moveCursorX


/*!
Changes the row of the cursor, within the boundaries of the
screen and any current margins.  All cursor-dependent data is
automatically synchronized if necessary.

This has no effect if print controller mode (VT102 or later)
is active.

IMPORTANT:	ALWAYS use moveCursor...() routines to set the
			cursor value; otherwise, cursor-dependent stuff
			could become out of sync.

(3.0)
*/
inline void
moveCursorY		(My_ScreenBufferPtr		inDataPtr,
				 My_ScreenRowIndex		inNewY)
{
	if (0 == (inDataPtr->printingModes & kMy_PrintingModePrintController))
	{
		My_ScreenBufferLineList::iterator	cursorLineIterator;
		
		
		locateCursorLine(inDataPtr, cursorLineIterator);
		
		if ((inDataPtr->mayNeedToSaveToScrollback) && (0 != inNewY))
		{
			// once the cursor leaves the home position, there is no
			// chance of having to save the top line into scrollback
			inDataPtr->mayNeedToSaveToScrollback = false;
		}
		STYLE_REMOVE(inDataPtr->current.drawingAttributes, cursorLineIterator->globalAttributes);
		
		// don’t allow the cursor to fall off the screen (in origin
		// mode, it cannot fall outside the scrolling region)
		{
			SInt16		newCursorY = inNewY;
			
			
			newCursorY = std::max< SInt16 >(newCursorY, inDataPtr->originRegionPtr->firstRow);
			newCursorY = std::min< SInt16 >(newCursorY, inDataPtr->originRegionPtr->lastRow);
			inDataPtr->current.cursorY = newCursorY;
		}
		
		// cursor has moved, so find the data for its new row
		locateCursorLine(inDataPtr, cursorLineIterator);
		
		STYLE_ADD(inDataPtr->current.drawingAttributes, cursorLineIterator->globalAttributes);
		if ((0 == inDataPtr->current.cursorY) && (0 == inDataPtr->current.cursorX))
		{
			// if the cursor moves into the home position, flag this
			// and prepare to possibly save the line contents if any
			// subsequent erase commands show up
			inDataPtr->mayNeedToSaveToScrollback = true;
		}
		
		inDataPtr->current.cursorAttributes = cursorLineIterator->attributeVector[inDataPtr->current.cursorX];
		
		// reset wrap flag, now that the cursor is moving
		inDataPtr->wrapPending = false;
		
		//if (inDataPtr->cursorVisible)
		{
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorLocation, inDataPtr->selfRef);
		}
		
	#if 0
		// DEBUG
		Console_WriteValuePair("cursor trace (dy)", inDataPtr->current.cursorX, inDataPtr->current.cursorY);
	#endif
	}
}// moveCursorY


/*!
Resets terminal modes to defaults, clears the screen
and notifies any listeners of reset events.

(2.6)
*/
void
resetTerminal   (My_ScreenBufferPtr  inDataPtr)
{
	inDataPtr->customScrollingRegion = inDataPtr->visibleBoundary.rows;
	assertScrollingRegion(inDataPtr);
	inDataPtr->emulator.parameterEndIndex = 0;
	My_VT100::ansiMode(inDataPtr);
	//inDataPtr->modeAutoWrap = false; // 3.0 - do not touch the auto-wrap setting
	inDataPtr->modeCursorKeysForApp = false;
	inDataPtr->modeApplicationKeys = false;
	inDataPtr->modeOriginRedefined = false; // also requires cursor homing (below), according to manual
	inDataPtr->originRegionPtr = &inDataPtr->visibleBoundary.rows;
	inDataPtr->previous.drawingAttributes = kInvalidTerminalTextAttributes;
	inDataPtr->current.drawingAttributes = kNoTerminalTextAttributes;
	inDataPtr->modeInsertNotReplace = false;
	inDataPtr->modeNewLineOption = false;
	moveCursor(inDataPtr, 0, 0);
	inDataPtr->current.characterSetInfoPtr = &inDataPtr->vtG0;
	inDataPtr->printingModes = 0;
	inDataPtr->printingEnd();
	bufferEraseVisibleScreenWithUpdate(inDataPtr);
	tabStopInitialize(inDataPtr);
	highlightLED(inDataPtr, 0/* zero means “turn off all LEDs” */);
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeReset, inDataPtr->selfRef/* context */);
}// resetTerminal


/*!
Returns the currently attached SessionRef, or nullptr
if none is attached.  This is necessary for a small
number of terminal features that need to report data
back to a requester (e.g. VT100 Device Attributes).

(3.1)
*/
SessionRef
returnListeningSession	(My_ScreenBufferPtr		inDataPtr)
{
	return inDataPtr->listeningSession;
}// returnListeningSession


/*!
Utility to extract a generic terminal type
from a more specific terminal emulation type.
For example, the VT family of terminals may
share many common emulation behaviors, so in
some cases you may only care that you are
currently using *some* kind of VT terminal,
you may not care which specific VT terminal
(e.g. VT220 versus VT100).

(3.0)
*/
inline Terminal_EmulatorType
returnTerminalType		(Terminal_Emulator	inEmulator)
{
	return ((inEmulator & kTerminal_EmulatorTypeMask) >> kTerminal_EmulatorTypeByteShift);
}// returnTerminalType


/*!
Utility to extract a generic terminal variant
from a more specific terminal emulation type.
For example, if you extracted the terminal type
and found you have a VT terminal, this routine
could tell you if the terminal is specifically
a VT100 or VT220.

(3.0)
*/
inline Terminal_EmulatorVariant
returnTerminalVariant		(Terminal_Emulator	inEmulator)
{
	return ((inEmulator & kTerminal_EmulatorVariantMask) >> kTerminal_EmulatorVariantByteShift);
}// returnTerminalVariant


/*!
Appends the visible screen to the scrollback buffer, usually in
preparation for then blanking the visible screen area.

Returns "true" only if successful.

(3.0)
*/
Boolean
screenCopyLinesToScrollback		(My_ScreenBufferPtr		inDataPtr)
{
	Boolean		result = true;
	
	
	if ((inDataPtr->text.scrollback.enabled) &&
		(inDataPtr->customScrollingRegion == inDataPtr->visibleBoundary.rows))
	{
		SInt16 const			kLineCount = inDataPtr->screenBuffer.size();
		My_ScreenBufferLine		templateLine;
		
		
		inDataPtr->scrollbackBuffer.insert(inDataPtr->scrollbackBuffer.begin(), kLineCount/* number of lines */, templateLine);
		inDataPtr->scrollbackBufferCachedSize += kLineCount;
		std::copy(inDataPtr->screenBuffer.rbegin(), inDataPtr->screenBuffer.rend(), inDataPtr->scrollbackBuffer.begin());
		
		if (inDataPtr->scrollbackBufferCachedSize > inDataPtr->text.scrollback.numberOfRowsPermitted)
		{
			inDataPtr->scrollbackBuffer.resize(inDataPtr->text.scrollback.numberOfRowsPermitted);
			inDataPtr->scrollbackBufferCachedSize = inDataPtr->text.scrollback.numberOfRowsPermitted;
		}
		
		if (result)
		{
			Terminal_ScrollDescription	scrollInfo;
			
			
			bzero(&scrollInfo, sizeof(scrollInfo));
			scrollInfo.screen = inDataPtr->selfRef;
			scrollInfo.rowDelta = -kLineCount;
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
		}
	}
	return result;
}// screenCopyLinesToScrollback


/*!
Modifies only the screen buffer, to include the specified
number of additional lines (at the end).  The new lines are
cleared.

This is rarely required, because scrolling will rotate the
oldest lines to the bottom.  However, if the screen buffer
becomes physically larger, or a screen is being created for
the first time, this can be useful.

The resultant lines may share a large memory block.  So, it
is usually better to invoke this routine once for the number
of lines you’ll ultimately need, than to invoke it many
times to add a single line.

Returns "true" only if successful.

(3.0)
*/
Boolean
screenInsertNewLines	(My_ScreenBufferPtr						inDataPtr,
						 My_ScreenBufferLineList::size_type		inNumberOfElements)
{
	Boolean		result = true;
	
	
	//Console_WriteValue("request to insert new lines", inNumberOfElements);
	if (0 != inNumberOfElements)
	{
		My_ScreenBufferLineList::size_type   kOldSize = inDataPtr->screenBuffer.size();
		
		
		// start by allocating more lines if necessary, or freeing unneeded lines
		inDataPtr->screenBuffer.resize(kOldSize + inNumberOfElements);
		
		// make sure the cursor line doesn’t fall off the end
		if (inDataPtr->current.cursorY >= inDataPtr->screenBuffer.size())
		{
			moveCursorY(inDataPtr, inDataPtr->screenBuffer.size() - 1);
		}
		
		// stop now if the resize failed
		if (inDataPtr->screenBuffer.size() != (kOldSize + inNumberOfElements))
		{
			// failed to resize!
			result = false;
		}
		else
		{
			//Console_WriteValue("append new lines", inNumberOfElements);
			//Console_WriteValue("final size", inDataPtr->screenBuffer.size());
		}
	}
	return result;
}// screenInsertNewLines


/*!
Removes lines from the top of the screen buffer, to the
adjacent part of the scrollback buffer, and replaces the
lost lines with blank ones at the bottom (so that the overall
screen buffer size is unchanged).

The data structures used for the “new” lines may actually
exist already, stripped from the oldest lines of the
scrollback buffer.

Returns "true" only if successful.

See also screenCopyLinesToScrollback().

(3.0)
*/
Boolean
screenMoveLinesToScrollback		(My_ScreenBufferPtr						inDataPtr,
								 My_ScreenBufferLineList::size_type		inNumberOfElements)
{
	Boolean		result = true;
	
	
	//Console_WriteValue("request to move lines to the scrollback", inNumberOfElements);
	if (0 != inNumberOfElements)
	{
		Boolean		recycleLines = false;
		
		
		// scrolling will be done; figure out whether or not to recycle old lines
		recycleLines = (!(inDataPtr->text.scrollback.enabled)) ||
						(!(inDataPtr->scrollbackBuffer.empty()) &&
							(inDataPtr->scrollbackBufferCachedSize >= inDataPtr->text.scrollback.numberOfRowsPermitted));
		
		// adjust screen and scrollback buffers appropriately; new lines
		// will either be rotated in from the oldest scrollback, or
		// allocated anew; in either case, they'll end up initialized
		if (recycleLines)
		{
			//Console_WriteValue("recycled lines", inNumberOfElements);
			
			// get the line destined for the new bottom of the main screen;
			// either extract it from elsewhere, or allocate a new line
			for (register My_ScreenBufferLineList::size_type i = 0; i < inNumberOfElements; ++i)
			{
				// extract the oldest line
				assert(!inDataPtr->screenBuffer.empty());
				if (inDataPtr->scrollbackBuffer.empty())
				{
					// make the oldest screen line the newest one
					inDataPtr->screenBuffer.splice(inDataPtr->screenBuffer.end()/* the next newest screen line */,
													inDataPtr->screenBuffer/* the list to move from */,
													inDataPtr->screenBuffer.begin()/* the line to move */);
				}
				else
				{
					My_ScreenBufferLineList::iterator	oldestScrollbackLine;
					
					
					// make the oldest screen line the newest scrollback line
					inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
														inDataPtr->screenBuffer/* the list to move from */,
														inDataPtr->screenBuffer.begin()/* the line to move */);
					++(inDataPtr->scrollbackBufferCachedSize);
					
					// end() points one past the end, so nudge it back
					oldestScrollbackLine = inDataPtr->scrollbackBuffer.end();
					std::advance(oldestScrollbackLine, -1);
					
					// make the oldest scrollback line the newest screen line
					inDataPtr->screenBuffer.splice(inDataPtr->screenBuffer.end()/* the next newest screen line */,
													inDataPtr->scrollbackBuffer/* the list to move from */,
													oldestScrollbackLine/* the line to move */);
					--(inDataPtr->scrollbackBufferCachedSize);
				}
				
				// the recycled line may have data in it, so clear it out
				inDataPtr->screenBuffer.back().structureInitialize();
			}
			
			//Console_WriteValue("post-recycle scrollback size (actual)", inDataPtr->scrollbackBuffer.size());
			//Console_WriteValue("post-recycle scrollback size (cached)", inDataPtr->scrollbackBufferCachedSize);
		}
		else
		{
			//Console_WriteValue("moved-and-reallocated lines", inNumberOfElements);
			
			My_ScreenBufferLineList::iterator	pastLastLineToScroll = inDataPtr->screenBuffer.begin();
			My_ScreenBufferLineList				movedLines;
			
			
			// find the last line that will remain in the screen buffer
			std::advance(pastLastLineToScroll, inNumberOfElements);
			
			// make the oldest screen lines the newest scrollback lines; since the
			// “front” scrollback line is adjacent to the “front” screen line, a
			// single splice is wrong; the lines have to insert in reverse order
			movedLines.splice(movedLines.begin()/* where to insert */,
								inDataPtr->screenBuffer/* the list to move from */,
								inDataPtr->screenBuffer.begin()/* the first line to move */,
								pastLastLineToScroll/* the first line that will not be moved */);
			movedLines.reverse();
			inDataPtr->scrollbackBuffer.splice(inDataPtr->scrollbackBuffer.begin()/* the next oldest scrollback line */,
												movedLines/* the list to move from */,
												movedLines.begin(), movedLines.end());
			inDataPtr->scrollbackBufferCachedSize += movedLines.size();
			
			if (inDataPtr->scrollbackBufferCachedSize > inDataPtr->text.scrollback.numberOfRowsPermitted)
			{
				inDataPtr->scrollbackBuffer.resize(inDataPtr->text.scrollback.numberOfRowsPermitted);
				inDataPtr->scrollbackBufferCachedSize = inDataPtr->text.scrollback.numberOfRowsPermitted;
			}
			
			//Console_WriteValue("post-move scrollback size (actual)", inDataPtr->scrollbackBuffer.size());
			//Console_WriteValue("post-move scrollback size (cached)", inDataPtr->scrollbackBufferCachedSize);
			
			// allocate new lines
			try
			{
				inDataPtr->screenBuffer.resize(inDataPtr->screenBuffer.size() + inNumberOfElements);
			}
			catch (std::bad_alloc)
			{
				// abort
				result = false;
			}
			
			//Console_WriteValue("post-allocation screen size", inDataPtr->screenBuffer.size());
		}
	}
	return result;
}// screenMoveLinesToScrollback


/*!
Removes the specified positive number of rows from the top of the
scrolling region, or the specified magnitude of rows from the
bottom when the value is negative.

When rows disappear from the top, blank lines appear at the
bottom.  Lines scrolled off the top are lost unless the terminal
is set to save them, in which case the lines are inserted into
the scrollback buffer.

When rows disappear from the bottom, blank lines appear at the
top.  Lines scrolled off the bottom are lost.

The display is updated.

(3.0)
*/
void
screenScroll	(My_ScreenBufferPtr		inDataPtr,
				 SInt16					inLineCount)
{
	if (inLineCount > 0)
	{
		// remove lines from the top of the scrolling region,
		// and possibly save them
		if (inDataPtr->current.cursorY < inDataPtr->screenBuffer.size())
		{
			if ((inDataPtr->text.scrollback.enabled) &&
				(inDataPtr->customScrollingRegion == inDataPtr->visibleBoundary.rows))
			{
				// scrolling region is entire screen, and lines are being saved off the top
				(Boolean)screenMoveLinesToScrollback(inDataPtr, inLineCount);
				
				// displaying right from top of scrollback buffer; topmost line being shown
				// has in fact vanished; update the display to show this
				//Console_WriteLine("text changed event: scroll terminal buffer");
				{
					Terminal_RangeDescription	range;
					
					
					range.screen = inDataPtr->selfRef;
					range.firstRow = 0;
					range.firstColumn = 0;
					range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
					range.rowCount = inDataPtr->visibleBoundary.rows.lastRow - inDataPtr->visibleBoundary.rows.firstRow + 1;
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextEdited, &range);
				}
				
				// notify about the scrolling amount
				{
					Terminal_ScrollDescription	scrollInfo;
					
					
					bzero(&scrollInfo, sizeof(scrollInfo));
					scrollInfo.screen = inDataPtr->selfRef;
					scrollInfo.rowDelta = -STATIC_CAST(inLineCount, SInt32);
					changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
				}
			}
			else
			{
				My_ScreenBufferLineList::iterator	scrollingRegionBegin;
				
				
				// region being scrolled is not entire screen; no saving of lines
				locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
				bufferRemoveLines(inDataPtr, inLineCount, scrollingRegionBegin);
			}
		}
	}
	else if (inLineCount < 0)
	{
		// remove lines from the bottom of the scrolling region (they are lost)
		// and add blank lines at the top
		My_ScreenBufferLineList::iterator	scrollingRegionBegin;
		
		
		locateScrollingRegionTop(inDataPtr, scrollingRegionBegin);
		bufferInsertBlankLines(inDataPtr, -inLineCount, scrollingRegionBegin/* insertion row */);
	}
}// screenScroll


/*!
Changes the logical cursor state.  Performed in a
function for consistency in case, for instance,
callbacks are invoked in the future whenever this
flag changes.

(3.0)
*/
inline void
setCursorVisible	(My_ScreenBufferPtr		inDataPtr,
					 Boolean				inIsVisible)
{
	inDataPtr->cursorVisible = inIsVisible;
	changeNotifyForTerminal(inDataPtr, kTerminal_ChangeCursorState, inDataPtr->selfRef);
}// setCursorVisible


/*!
Changes the maximum size of the scrollback buffer.  If the
current content is larger, its memory is truncated.

This triggers two events: "kTerminal_ChangeScrollActivity"
to indicate that data has been removed, and also
"kTerminal_ChangeTextRemoved" (given a range consisting of
all previous scrollback lines).

(4.0)
*/
void
setScrollbackSize	(My_ScreenBufferPtr		inDataPtr,
					 UInt32					inLineCount)
{
	My_ScreenBufferLineList::size_type const	kPreviousScrollbackCount = inDataPtr->scrollbackBufferCachedSize;
	
	
	inDataPtr->text.scrollback.numberOfRowsPermitted = inLineCount;
	inDataPtr->text.scrollback.enabled = (inDataPtr->text.scrollback.numberOfRowsPermitted > 0L);
	
	if (kPreviousScrollbackCount > inLineCount)
	{
		// notify listeners of the range of text that has gone away
		{
			Terminal_RangeDescription	range;
			
			
			range.screen = inDataPtr->selfRef;
			range.firstRow = -kPreviousScrollbackCount;
			range.firstColumn = 0;
			range.columnCount = inDataPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kPreviousScrollbackCount - inLineCount;
			
			changeNotifyForTerminal(inDataPtr, kTerminal_ChangeTextRemoved, &range/* context */);
		}
	}
	
	inDataPtr->scrollbackBuffer.resize(inLineCount);
	inDataPtr->scrollbackBufferCachedSize = inLineCount;
	
	// notify listeners that scroll activity has taken place,
	// though technically no remaining lines have been affected
	{
		Terminal_ScrollDescription	scrollInfo;
		
		
		bzero(&scrollInfo, sizeof(scrollInfo));
		scrollInfo.screen = inDataPtr->selfRef;
		scrollInfo.rowDelta = 0;
		changeNotifyForTerminal(inDataPtr, kTerminal_ChangeScrollActivity, &scrollInfo/* context */);
	}
}// setScrollbackSize


/*!
Changes the number of characters of text per line for a
screen buffer.  This operation is currently trivial
because all line arrays are allocated to maximum size
when a buffer is created, and this simply adjusts the
percentage of the total that should be usable.  Although,
this also forces the cursor into the new region, if
necessary.

IMPORTANT:	This is a low-level routine for internal use;
			send a "kTerminal_ChangeScreenSize" notification
			after all size changes are complete.  Note that
			Terminal_SetVisibleScreenDimensions() will set
			dimensions and automatically notify listeners.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of columns is too small or too large

\retval kTerminal_ResultNotEnoughMemory
not currently returned because this routine does no memory
reallocation; however a future implementation might decide
to reallocate, and if such reallocation fails, this error
should be returned

(2.6)
*/
Terminal_Result
setVisibleColumnCount	(My_ScreenBufferPtr		inPtr,
						 UInt16					inNewNumberOfCharactersWide)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	if (nullptr == inPtr) result = kTerminal_ResultInvalidID;
	else
	{
		// move cursor, if necessary
		if (inNewNumberOfCharactersWide <= inPtr->current.cursorX)
		{
			moveCursorX(inPtr, inNewNumberOfCharactersWide - 1);
		}
		
		if (inNewNumberOfCharactersWide > kMy_NumberOfCharactersPerLineMaximum)
		{
			// flag an error, but set a reasonable value anyway
			result = kTerminal_ResultParameterError;
			inNewNumberOfCharactersWide = kMy_NumberOfCharactersPerLineMaximum;
		}
		inPtr->text.visibleScreen.numberOfColumnsPermitted = inNewNumberOfCharactersWide;
	}
	return result;
}// setVisibleColumnCount


/*!
Changes the number of lines of text, not including the
scrollback buffer, for a screen buffer.  This non-trivial
operation requires reallocating a series of arrays.

IMPORTANT:	This is a low-level routine for internal use;
			send a "kTerminal_ChangeScreenSize" notification
			after all size changes are complete.  Note that
			Terminal_SetVisibleScreenDimensions() will set
			dimensions and automatically notify listeners.

\retval kTerminal_ResultOK
if the terminal is resized without errors

\retval kTerminal_ResultInvalidID
if the given terminal screen reference is invalid

\retval kTerminal_ResultParameterError
if the given number of rows is too small or too large

\retval kTerminal_ResultNotEnoughMemory
if it is not possible to allocate the requested number of rows

(2.6)
*/
Terminal_Result
setVisibleRowCount	(My_ScreenBufferPtr		inPtr,
					 UInt16					inNewNumberOfLinesHigh)
{
	Terminal_Result		result = kTerminal_ResultOK;
	
	
	//Console_WriteValue("requested new number of lines", inNewNumberOfLinesHigh);
	if (inNewNumberOfLinesHigh > 200)
	{
		Console_WriteLine("refusing to resize on account of ridiculous line size");
		result = kTerminal_ResultParameterError;
	}
	else if (nullptr == inPtr) result = kTerminal_ResultInvalidID;
	else if (inPtr->screenBuffer.size() != inNewNumberOfLinesHigh)
	{
		// then the requested number of lines is different than the current number; resize!
		SInt16 const	kOriginalNumberOfLines = inPtr->screenBuffer.size();
		SInt16 const	kLineDelta = (inNewNumberOfLinesHigh - kOriginalNumberOfLines);
		
		
		//Console_WriteValue("window line size", kOriginalNumberOfLines);
		//Console_WriteValue("window size change", kLineDelta);
		
		// force view to the top of the bottommost screenful
		// UNIMPLEMENTED
		
		if (kLineDelta > 0)
		{
			// if more lines are in the screen buffer than before,
			// allocate space for them (but don’t scroll them off!)
			Boolean		insertOK = screenInsertNewLines(inPtr, kLineDelta);
			
			
			unless (insertOK) result = kTerminal_ResultNotEnoughMemory;
		}
		else
		{
		#if 0
			// this will move the entire screen buffer into the scrollback
			// beforehand, if desired (having the effect of CLEARING the
			// main screen, which is basically undesirable most of the time)
			if (inPtr->text.scrollback.enabled)
			{
				(Boolean)screenCopyLinesToScrollback(inPtr);
			}
		#endif
			
			// now make sure the cursor line doesn’t fall off the end
			if (inPtr->current.cursorY >= (inPtr->screenBuffer.size() + kLineDelta))
			{
				moveCursorY(inPtr, inPtr->screenBuffer.size() + kLineDelta - 1);
			}
			
			// finally, shrink the buffer
			inPtr->screenBuffer.resize(inPtr->screenBuffer.size() + kLineDelta);
		}
		
		// reset visible region; if the custom margin is also at the bottom,
		// keep it at the new bottom (otherwise, assume there was a good
		// reason that it was not using all the lines, and leave it alone!)
		if (inPtr->customScrollingRegion.lastRow >= inPtr->visibleBoundary.rows.lastRow)
		{
			inPtr->customScrollingRegion.lastRow = inNewNumberOfLinesHigh - 1;
			if (inPtr->customScrollingRegion.firstRow > inPtr->customScrollingRegion.lastRow)
			{
				inPtr->customScrollingRegion.firstRow = inPtr->customScrollingRegion.lastRow - 1;
				assertScrollingRegion(inPtr);
			}
		}
		inPtr->visibleBoundary.rows.firstRow = 0;
		inPtr->visibleBoundary.rows.lastRow = inNewNumberOfLinesHigh - 1;
		
		// add new screen rows to the text-change region; this should trigger
		// things like Terminal View updates
		if (kLineDelta > 0)
		{
			// when the screen is becoming bigger, it is only necessary to
			// refresh the new region; when the screen is becoming smaller,
			// no refresh is necessary at all!
			Terminal_RangeDescription	range;
			
			
			//Console_WriteLine("text changed event: set visible row count");
			range.screen = inPtr->selfRef;
			range.firstRow = kOriginalNumberOfLines;
			range.firstColumn = 0;
			range.columnCount = inPtr->text.visibleScreen.numberOfColumnsPermitted;
			range.rowCount = kLineDelta;
			changeNotifyForTerminal(inPtr, kTerminal_ChangeTextEdited, &range);
		}
	}
	
	return result;
}// setVisibleRowCount


/*!
Removes all tab stops.  See also tabStopInitialize(),
which sets tabs to reasonable default values.

(3.0)
*/
void
tabStopClearAll		(My_ScreenBufferPtr		inDataPtr)
{
	My_TabStopList::iterator	tabStopIterator;
	
	
	for (tabStopIterator = inDataPtr->tabSettings.begin(); tabStopIterator != inDataPtr->tabSettings.end(); ++tabStopIterator)
	{
		*tabStopIterator = kMy_TabClear;
	}
}// tabStopClearAll


/*!
Returns the number of spaces until the next tab stop, based on
the current cursor position of the specified screen.

If "inForwardDirection" is true, the distance is returned
relative to the next stop to the right of the cursor location;
otherwise, the distance refers to the next stop to the left
(that is, a backwards tab).

(2.6)
*/
UInt16
tabStopGetDistanceFromCursor	(My_ScreenBufferConstPtr	inDataPtr,
								 Boolean					inForwardDirection)
{
	UInt16		result = 0;
	
	
	if (inForwardDirection)
	{
		if (inDataPtr->current.cursorX < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1))
		{
			result = inDataPtr->current.cursorX + 1;
			while ((inDataPtr->tabSettings[result] != kMy_TabSet) &&
					(result < (inDataPtr->current.returnNumberOfColumnsPermitted() - 1)))
			{
				++result;
			}
			result = (result - inDataPtr->current.cursorX);
		}
	}
	else
	{
		if (inDataPtr->current.cursorX > 0)
		{
			result = inDataPtr->current.cursorX - 1;
			while ((inDataPtr->tabSettings[result] != kMy_TabSet) && (result > 0))
			{
				--result;
			}
			result = (inDataPtr->current.cursorX - result);
		}
	}
	return result;
}// tabStopGetDistanceFromCursor


/*!
Reset tabs to default stops (one every "kMy_TabStop"
columns starting at the first column, and one at the
last column).

(3.0)
*/
void
tabStopInitialize	(My_ScreenBufferPtr		inDataPtr)
{
	My_TabStopList::iterator	tabStopIterator;
	
	
	tabStopClearAll(inDataPtr);
	
	// IMPORTANT: This works only because the list is initialized to be a multiple
	//            of the tab stop distance.  If this were not true, std::advance()
	//            would jump the iterator to an invalid value, causing this to crash.
	for (tabStopIterator = inDataPtr->tabSettings.begin(); tabStopIterator != inDataPtr->tabSettings.end();
			std::advance(tabStopIterator, STATIC_CAST(kMy_TabStop, My_TabStopList::difference_type)/* tab distance */))
	{
		*tabStopIterator = kMy_TabSet;
	}
	assert(!inDataPtr->tabSettings.empty());
	inDataPtr->tabSettings.back() = kMy_TabSet; // also make last column a tab
}// tabStopInitialize


/*!
Translates the given character code using the rules
of the current character set of the specified screen
(character sets G0 or G1, for VT terminals).  The
new code is returned, which may be unchanged.

The attributes are now provided too, which allows
the internal storage of an original character to be
more appropriate; e.g. an ASCII-encoded graphics
character may be stored as the Unicode character
that has the intended glyph.

If the given character needs its attributes changed
(typically, because it is actually graphical), those
new attributes are returned.  The new attributes
should ONLY apply to the given character; they do
not indicate a new default for the character stream.

(4.0)
*/
inline UniChar
translateCharacter	(My_ScreenBufferPtr			inDataPtr,
					 UniChar					inCharacter,
					 TerminalTextAttributes		inAttributes,
					 TerminalTextAttributes&	outNewAttributes)
{
	UniChar		result = inCharacter;
	
	
	outNewAttributes = inAttributes; // initially...
	switch (inDataPtr->current.characterSetInfoPtr->translationTable)
	{
	case kMy_CharacterSetVT100UnitedStates:
		// this is the default; do nothing
		break;
	
	case kMy_CharacterSetVT100UnitedKingdom:
		// the only difference between ASCII and U.K. is that
		// the pound sign (#) is a British currency symbol (£)
		if (inCharacter == '#') result = 0x00A3;
		break;
	
	default:
		// ???
		break;
	}
	
	// TEMPORARY - the renderer does not handle most Unicode characters,
	// but programs sometimes choose “unnecessarily exotic” variations
	// of characters that would lead to unknown-character renderings when
	// it is pretty easy to choose sensible ASCII equivalents...
	switch (inCharacter)
	{
	case 0x2212: // minus sign
	case 0x2010: // hyphen
		result = '-';
		break;
	
	default:
		break;
	}
	
	if (STYLE_USE_VT_GRAPHICS(inAttributes))
	{
		Boolean const	kIsBold = STYLE_BOLD(inAttributes);
		Boolean const	kVT52 = (false == inDataPtr->modeANSIEnabled);
		
		
		// if text was originally encoded with graphics attributes, internally
		// store the equivalent Unicode character so that cool stuff like
		// copy and paste of text will do the right thing (the renderer may
		// still choose not to rely on Unicode fonts for rendering them);
		// INCOMPLETE: this might need terminal-emulator-specific code
		// IMPORTANT: old drawing code currently requires that non-graphical
		// symbols below (e.g. degrees, plus-minus, etc.) successfully
		// translate to the Mac Roman encoding
		switch (inCharacter)
		{
		case '`':
			result = 0x25CA; // filled diamond; using hollow (lozenge) for now, Unicode 0x2666 is better
			break;
		
		case 'a':
			result = 0x2593; // checkerboard
			break;
		
		case 'b':
			result = 0x21E5; // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
			break;
		
		case 'c':
			result = 0x21DF; // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
			break;
		
		case 'd':
			result = 0x2190; // carriage return (international symbol is an arrow pointing right to left)
			break;
		
		case 'e':
			result = 0x2193; // line feed (international symbol is an arrow pointing top to bottom)
			break;
		
		case 'f':
			result = 0x00B0; // degrees (same in VT52)
			break;
		
		case 'g':
			result = 0x00B1; // plus or minus (same in VT52)
			break;
		
		case 'h':
			result = 0x21B5; // new line (international symbol is an arrow that hooks from mid-top to mid-left)
			break;
		
		case 'i':
			result = 0x2913; // vertical tab (international symbol is a down-pointing arrow with a terminating line)
			break;
		
		case 'j':
			if (kVT52)
			{
				result = 0x00F7; // division
			}
			else
			{
				result = (kIsBold) ? 0x251B : 0x2518; // hook mid-top to mid-left
			}
			break;
		
		case 'k':
			result = (kIsBold) ? 0x2513 : 0x2510; // hook mid-left to mid-bottom
			break;
		
		case 'l':
			result = (kIsBold) ? 0x250F : 0x250C; // hook mid-right to mid-bottom
			break;
		
		case 'm':
			result = (kIsBold) ? 0x2517 : 0x2514; // hook mid-top to mid-right
			break;
		
		case 'n':
			result = (kIsBold) ? 0x254B : 0x253C; // cross
			break;
		
		case 'o':
			result = 0x23BA; // top line
			break;
		
		case 'p':
			result = 0x23BB; // line between top and middle regions
			break;
		
		case 'q':
			result = (kIsBold) ? 0x2501 : 0x2500; // middle line
			break;
		
		case 'r':
			result = 0x23BC; // line between middle and bottom regions
			break;
		
		case 's':
			result = 0x23BD; // bottom line
			break;
		
		case 't':
			result = (kIsBold) ? 0x2523 : 0x251C; // cross minus the left piece
			break;
		
		case 'u':
			result = (kIsBold) ? 0x252B : 0x2524; // cross minus the right piece
			break;
		
		case 'v':
			result = (kIsBold) ? 0x253B : 0x2534; // cross minus the bottom piece
			break;
		
		case 'w':
			result = (kIsBold) ? 0x2533 : 0x252C; // cross minus the top piece
			break;
		
		case 'x':
			result = (kIsBold) ? 0x2503 : 0x2502; // vertical line
			break;
		
		case 'y':
			result = 0x2264; // less than or equal to
			break;
		
		case 'z':
			result = 0x2265; // greater than or equal to
			break;
		
		case '{':
			result = 0x03C0; // pi
			break;
		
		case '|':
			result = 0x2260; // not equal to
			break;
		
		case '}':
			result = 0x00A3; // British pounds (currency) symbol
			break;
		
		case '~':
			if (kVT52)
			{
				result = 0x00B6; // pilcrow (paragraph) sign
			}
			else
			{
				result = 0x2027; // centered dot
			}
			break;
		
		case 159:
			result = 0x0192; // small 'f' with hook
			break;
		
		case 224:
			result = 0x03B1; // alpha
			break;
		
		case 225:
			result = 0x00DF; // beta
			break;
		
		case 226:
			result = 0x0393; // capital gamma
			break;
		
		case 227:
			result = 0x03C0; // pi
			break;
		
		case 228:
			result = 0x03A3; // capital sigma
			break;
		
		case 229:
			result = 0x03C3; // sigma
			break;
		
		case 230:
			result = 0x00B5; // mu
			break;
		
		case 231:
			result = 0x03C4; // tau
			break;
		
		case 232:
			result = 0x03A6; // capital phi
			break;
		
		case 233:
			result = 0x0398; // capital theta
			break;
		
		case 234:
			result = 0x03A9; // capital omega
			break;
		
		case 235:
			result = 0x03B4; // delta
			break;
		
		case 237:
			result = 0x03C6; // phi
			break;
		
		case 238:
			result = 0x03B5; // epsilon
			break;
		
		case 251:
			result = 0x221A; // square root left edge
			break;
		
		default:
			break;
		}
	}
	else
	{
		// the original character was not explicitly identified as graphical,
		// but it may still be best to *tag* it as such, so that a more
		// advanced rendering can be done (default font renderings for
		// graphics are not always as nice)
		switch (inCharacter)
		{
		// this list should generally match the set of Unicode characters that
		// are handled by the drawVTGraphicsGlyph() internal method in the
		// Terminal View module
		case 0x2591: // light gray pattern
		case 0x2592: // medium gray pattern
		case 0x2593: // heavy gray pattern or checkerboard
		case 0x21E5: // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
		case 0x21DF: // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
		case 0x2190: // carriage return (international symbol is an arrow pointing right to left)
		case 0x2193: // line feed (international symbol is an arrow pointing top to bottom)
		case 0x21B5: // new line (international symbol is an arrow that hooks from mid-top to mid-left)
		case 0x2913: // vertical tab (international symbol is a down-pointing arrow with a terminating line)
		case 0x2518: // hook mid-top to mid-left
		case 0x251B: // hook mid-top to mid-left, bold version
		case 0x255B: // hook mid-top to mid-left, double-horizontal-only version
		case 0x255C: // hook mid-top to mid-left, double-vertical-only version
		case 0x255D: // hook mid-top to mid-left, double-line version
		case 0x2510: // hook mid-left to mid-bottom
		case 0x2513: // hook mid-left to mid-bottom, bold version
		case 0x2555: // hook mid-left to mid-bottom, double-horizontal-only version
		case 0x2556: // hook mid-left to mid-bottom, double-vertical-only version
		case 0x2557: // hook mid-left to mid-bottom, double-line version
		case 0x250C: // hook mid-right to mid-bottom
		case 0x250F: // hook mid-right to mid-bottom, bold version
		case 0x2552: // hook mid-right to mid-bottom, double-horizontal-only version
		case 0x2553: // hook mid-right to mid-bottom, double-vertical-only version
		case 0x2554: // hook mid-right to mid-bottom, double-line version
		case 0x2514: // hook mid-top to mid-right
		case 0x2517: // hook mid-top to mid-right, bold version
		case 0x2558: // hook mid-top to mid-right, double-horizontal-only version
		case 0x2559: // hook mid-top to mid-right, double-vertical-only version
		case 0x255A: // hook mid-top to mid-right, double-line version
		case 0x253C: // cross
		case 0x254B: // cross, bold version
		case 0x256A: // cross, double-horizontal-only version
		case 0x256B: // cross, double-vertical-only version
		case 0x256C: // cross, double-line version
		case 0x23BA: // top line
		case 0x23BB: // line between top and middle regions
		case 0x2500: // middle line
		case 0x2501: // middle line, bold version
		case 0x2550: // middle line, double-line version
		case 0x23BC: // line between middle and bottom regions
		case 0x2261: // equivalent to
		case 0x23BD: // bottom line
		case 0x251C: // cross minus the left piece
		case 0x2523: // cross minus the left piece, bold version
		case 0x255E: // cross minus the left piece, double-horizontal-only version
		case 0x255F: // cross minus the left piece, double-vertical-only version
		case 0x2560: // cross minus the left piece, double-line version
		case 0x2524: // cross minus the right piece
		case 0x252B: // cross minus the right piece, bold version
		case 0x2561: // cross minus the right piece, double-horizontal-only version
		case 0x2562: // cross minus the right piece, double-vertical-only version
		case 0x2563: // cross minus the right piece, double-line version
		case 0x2534: // cross minus the bottom piece
		case 0x253B: // cross minus the bottom piece, bold version
		case 0x2567: // cross minus the bottom piece, double-horizontal-only version
		case 0x2568: // cross minus the bottom piece, double-vertical-only version
		case 0x2569: // cross minus the bottom piece, double-line version
		case 0x252C: // cross minus the top piece
		case 0x2533: // cross minus the top piece, bold version
		case 0x2564: // cross minus the top piece, double-horizontal-only version
		case 0x2565: // cross minus the top piece, double-vertical-only version
		case 0x2566: // cross minus the top piece, double-line version
		case 0x2502: // vertical line
		case 0x2503: // vertical line, bold version
		case 0x2551: // vertical line, double-line version
		case 0x2588: // solid block
		case 0x2584: // lower-half block
		case 0x258C: // left-half block
		case 0x2590: // right-half block
		case 0x2580: // top-half block
		case 0x2027: // centered dot
		case 0x00B7: // centered dot (alternate?)
		case 0x2022: // bullet
		case 0x2219: // bullet operator
		case 0x25A0: // black square
		case 0x2320: // integral sign (elongated S), top
		case 0x2321: // integral sign (elongated S), bottom
		case 0x0192: // small 'f' with hook
		case 0x03B1: // alpha
		case 0x00DF: // beta
		case 0x0393: // capital gamma
		case 0x03C0: // pi
		case 0x03A3: // capital sigma
		case 0x03C3: // sigma
		case 0x00B5: // mu
		case 0x03C4: // tau
		case 0x03A6: // capital phi
		case 0x0398: // capital theta
		case 0x03A9: // capital omega
		case 0x03B4: // delta
		case 0x03C6: // phi
		case 0x03B5: // epsilon
		case 0x221A: // square root left edge
			STYLE_ADD(outNewAttributes, kTerminalTextAttributeVTGraphics);
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// translateCharacter


/*!
Copies the specified number of characters from the
source to the destination, dropping trailing blanks.
If table is nonzero, then every series of this many
consecutive spaces will be replaced with a single
tab character.

Returns an iterator just past the end of the copied
data in the output buffer.  This allows you to use
the iterator to insert a character immediately after
the text that this routine copies.

Also, the actual number of bytes copied is provided.
It follows that if you were to advance the original
buffer iterator by this amount, the result should be
the returned iterator.

NOTE:   This routine is generic.  It therefore works
		with something as simple as an array of
		characters, or as complex as an STL container
		of characters - your choice!

(3.0)
*/
template < typename src_char_seq_const_iter, typename src_char_seq_size_t,
			typename dest_char_seq_iter, typename dest_char_seq_size_t >
dest_char_seq_iter
whitespaceSensitiveCopy		(src_char_seq_const_iter	inSourceBuffer,
							 src_char_seq_size_t		inSourceBufferLength,
							 dest_char_seq_iter			outDestinationBuffer,
							 dest_char_seq_size_t		inDestinationBufferLength,
							 dest_char_seq_size_t*		outDestinationCopiedLengthPtr,
							 src_char_seq_size_t		inNumberOfSpacesPerTabOrZeroForNoSubstitution)
{
	dest_char_seq_iter  result = outDestinationBuffer;
	
	
	if (inSourceBufferLength > 0)
	{
		src_char_seq_const_iter		kPastSourceStartIterator;
		src_char_seq_const_iter		pastSourceEndIterator;
		src_char_seq_const_iter		sourceEndIterator;
		src_char_seq_size_t			sourceLength = inSourceBufferLength;
		
		
		// initialize “constant” past-source-start iterator
		kPastSourceStartIterator = inSourceBuffer;
		std::advance(kPastSourceStartIterator, -1);
		
		// initialize past-source-end iterator
		pastSourceEndIterator = inSourceBuffer;
		std::advance(pastSourceEndIterator, inSourceBufferLength);
		
		// initialize source-end iterator
		sourceEndIterator = inSourceBuffer;
		std::advance(sourceEndIterator, inSourceBufferLength - 1);
		
		// skip trailing blanks
		while ((*sourceEndIterator == ' ') && (sourceEndIterator != kPastSourceStartIterator))
		{
			--sourceEndIterator;
			--pastSourceEndIterator;
			--sourceLength;
		}
		
		// if the string contains non-blanks, continue
		if (sourceEndIterator != kPastSourceStartIterator)
		{
			if (inNumberOfSpacesPerTabOrZeroForNoSubstitution == 0)
			{
				// straight character copy
				src_char_seq_size_t		copiedLength = INTEGER_MINIMUM(sourceLength, inDestinationBufferLength);
				
				
				__gnu_cxx::copy_n(inSourceBuffer, copiedLength, outDestinationBuffer);
				result = outDestinationBuffer;
				std::advance(result, copiedLength);
			}
			else
			{
				src_char_seq_const_iter		srcIter = inSourceBuffer;
				dest_char_seq_iter			destIter = outDestinationBuffer;
				dest_char_seq_iter			kPastDestinationEndIterator;
				
				
				// initialize “constant” past-destination-end iterator
				kPastDestinationEndIterator = outDestinationBuffer;
				std::advance(kPastDestinationEndIterator, inDestinationBufferLength);
				
				// tab-replacement copy
				while (srcIter != pastSourceEndIterator)
				{
					// direct copy for non-whitespace
					while ((*srcIter != ' ') && (srcIter != pastSourceEndIterator) && (destIter != kPastDestinationEndIterator))
					{
						*destIter++ = *srcIter++;
					}
					
					// for whitespace, replace with tabs when appropriate
					if (srcIter != pastSourceEndIterator)
					{
						src_char_seq_size_t		numberOfConsecutiveSpaces = 0;
						
						
						while ((*srcIter == ' ') && (srcIter != pastSourceEndIterator) && (destIter != kPastDestinationEndIterator))
						{
							*destIter++ = *srcIter++;
							++numberOfConsecutiveSpaces;
							if (numberOfConsecutiveSpaces == inNumberOfSpacesPerTabOrZeroForNoSubstitution)
							{
								// replace series of spaces with a tab
								std::advance(destIter, -inNumberOfSpacesPerTabOrZeroForNoSubstitution);
								*destIter++ = '\011';
								numberOfConsecutiveSpaces = 0;
							}
						}
					}
				}
				result = destIter;
			}
		}
	}
	
	*outDestinationCopiedLengthPtr = std::distance(outDestinationBuffer, result);
	return result;
}// whitespaceSensitiveCopy

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE

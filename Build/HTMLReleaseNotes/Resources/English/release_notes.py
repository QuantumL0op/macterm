#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Notes on changes in each MacTelnet release.

Import this module to access its variables, or run it as a script to see the
sorted, formatted release notes as text (see to_rst()).
"""
__author__ = "Kevin Grant <kevin@ieee.org>"
__date__ = "28 September 2006"

# IMPORTANT: Released versions of MacTelnet will always point to
# a specific version page, so don't ever remove old versions from
# this list (that is, even ancient versions pages should be
# generated).  Also, the order of this list matters; the newest
# release should be inserted at the beginning.
version_lineage = [
    '3.1.0',
    '3.0.1',
]

daily_build_lineage = [
    '20080602',
    '20080530',
    '20080529',
    '20080527',
    '20080524',
    '20080523',
    '20080522',
    '20080520',
    '20080517',
    '20080515',
    '20080514',
    '20080513',
    '20080511',
    '20080510',
    '20080506',
    '20080505',
    '20080504',
    '20080501',
    '20080429',
    '20080426',
    '20080424',
    '20080422',
    '20080421',
    '20080418',
    '20080417',
    '20080416',
    '20080413',
    '20080412',
    '20080411',
    '20080410',
    '20080409',
    '20080408',
    '20080406',
    '20080405',
    '20080404',
    '20080403',
    '20080401',
    '20080330',
    '20080329',
    '20080328',
    '20080327',
    '20080326',
    '20080325',
    '20080324',
    '20080323',
    '20080322',
    '20080321',
    '20080320',
    '20080319',
    '20080318',
    '20080317',
    '20080316',
    '20080315',
    '20080314',
    '20080313',
    '20080312',
    '20080311',
    '20080310',
    '20080308',
    '20080307',
    '20080306',
    '20080305',
    '20080303',
    '20080302',
    '20080301',
    '20080229',
    '20080228',
    '20080227',
    '20080226',
    '20080224',
    '20080223',
    '20080222',
    '20080220',
    '20080216',
    '20080212',
    '20080210',
    '20080205',
    '20080203',
    '20080202',
    '20080130',
    '20080129',
    '20080128',
    '20080126',
    '20080121',
    '20080120',
    '20080115',
    '20080111',
    '20080103',
    '20080101',
    '20071231',
    '20071230',
    '20071229',
    '20071227',
    '20071225',
    '20071221',
    '20071219',
    '20071204',
    '20071119',
    '20071104',
    '20071103',
    '20071023',
    '20071022',
    '20071019',
    '20071015',
    '20071013',
    '20071011',
    '20071010',
    '20071006',
    '20070929',
    '20070918',
    '20070908',
    '20070906',
    '20070827',
    '20070424',
    '20070224',
    '20070220',
    '20070212',
    '20070207',
    '20070124',
    '20070109',
    '20070108',
    '20070107',
    '20070103',
    '20061227',
    '20061224',
    '20061127',
    '20061113',
    '20061110',
    '20061107',
    '20061105',
    '20061101',
    '20061030',
    '20061029',
    '20061028',
    '20061027',
    '20061026',
    '20061025',
    '20061024',
    '20061023',
    '20061022',
]

notes_by_version = {
	'20080602': [
		'The Growl framework is now used for background notifications, when it is available.',
	],
	'20080530': [
		'Preferences window Terminals pane Emulation tab now has an option for fixing the line wrap bug of a standard VT100.',
	],
	'20080529': [
		'The floating command line is now a Cocoa window, which fixes numerous problems this window has had in the past.',
	],
	'20080527': [
		'Fixed various contextual menu commands to better-match their menu bar equivalents.',
		'The keypads, Full Screen mode off-switch and "IP Addresses of This Mac" no longer steal keyboard focus when displayed.',
	],
	'20080524': [
		'The off-switch window in Full Screen mode is now a Cocoa window.',
		'The off-switch window in Full Screen mode now remembers its position.',
	],
	'20080523': [
		'Preferences window Terminals pane Emulation tab now has the intended list of checkboxes for terminal tweaks.',
	],
	'20080522': [
		'Fixed the sample terminal display in places like the Preferences window Formats pane.',
		'The command line displayed as the default window title no longer has a trailing space.',
		'MacTelnet Help now contains some basic information on how to use Automator with MacTelnet.',
	],
	'20080520': [
		'The Open dialog is now implemented using Cocoa, which is an improvement on older versions of Mac OS X.',
	],
	'20080517': [
		'Fixed a possible crash if a session is still trying to process data at the time it is terminated.',
		'Fixed startup errors, such as type -2703, that could appear on certain computers.',
		'Added Prefs.TRANSLATION to Quills, allowing Python functions to refer to this type of collection.',
	],
	'20080515': [
		'The "IP Addresses of This Mac" command now matches its window title, and is also in sync with the Dock menu.',
	],
	'20080514': [
		'Preferences window Sessions pane Keyboard tab has been refined further.',
	],
	'20080513': [
		'The "Show IP Addresses..." command is now in the Window menu, and has been renamed "IP Addresses".',
		'The IP Addresses window is now implemented using Cocoa, which made it trivial to support drags.',
		'"Send IP Address" was removed due to ambiguity; now, just use drag-and-drop from the IP Addresses window.',
	],
	'20080511': [
		'The Preferences window Translations pane now displays localized names for all text encodings (character sets).',
	],
	'20080510': [
		'Key palettes are now implemented using Cocoa, which makes them easier to use (for example, command-W works on them).',
	],
	'20080506': [
		'The Special Key Sequences dialog now reuses the floating Control Keys palette to make changes, so the dialog is much smaller.',
	],
	'20080505': [
		'Fixed a conflict in the Preferences window where using the Default command in the File menu would affect the collections list.',
		'Fixed name generation in the Preferences window collections drawer (for "+", and duplication) so the result is always unique.',
		'Terminal window tabs can now display much longer titles.',
		'Terminal window tabs now have a small side button for the "new workspace" behavior, instead of a huge button.',
	],
	'20080504': [
		'Fixed the Preferences window Sessions pane Resource tab, so it is possible to properly save favorite commands or servers.',
	],
	'20080501': [
		'Fixed the Preferences window Sessions pane Graphics tab.',
	],
	'20080429': [
		'Dragging text into a background terminal window will now automatically bring the window to the front after a short delay.',
	],
	'20080426': [
		'Fixed arrow key sequences in certain modes, noticeable in applications such as the "vim" text editor.',
		'Fixed command-option-click to once again send arrow key sequences to move the cursor to the clicked location.',
		'Fixed "Move cursor to text drop location" behavior.',
	],
	'20080424': [
		'Added even more to the Low-Level Settings section in MacTelnet Help.',
	],
	'20080422': [
		'Added more to the Low-Level Settings section in MacTelnet Help.',
	],
	'20080421': [
		'Fixed the Preferences window Translations pane.',
		'Fixed the Map menu to display all Translation collections.',
	],
	'20080418': [
		'Fixed some parts of the Preferences window Terminals pane Screen tab.',
	],
	'20080417': [
		'Fixed background window text selections to allow immediate drags, as their enabled states imply.',
		'Changed the style of MacTelnet Help somewhat, to better fit the monospaced layout that the content generates.',
	],
	'20080416': [
		'Started a Low-Level Settings section in MacTelnet Help to document preferences hidden from the main user interface.',
	],
	'20080413': [
		'Fixed font selection in the Preferences window Translations pane.',
	],
	'20080412': [
		'Redesigned parts of the Preferences window Translations pane to offer more useful rendering preferences.',
	],
	'20080411': [
		'The About box now uses the Cocoa standard implementation.',
		'Fixed certain cases on Panther where user interface panels could become "unclickable".',
	],
	'20080410': [
		'Fixed some glitches in the display of items in the Window menu.',
		'Fixed a possible crash at quitting time if the Command Line was ever displayed.',
	],
	'20080409': [
		'Fixed random actions to be more random; used in such things as the splash screen and the random terminal format setting.',
	],
	'20080408': [
		'The foreground and background colors used by TEK windows are now defined by Default Format preferences.',
		'The 6 other colors used by TEK windows are now defined by the normal ANSI Colors from Default Format preferences.',
		'Minor internal optimizations to TEK windows.',
	],
	'20080406': [
		'Fixed a case where renaming a single window could propagate the change to every open terminal window.',
	],
	'20080405': [
		'Fixed color boxes on Panther to no longer use the floating Color Panel, because it is too buggy on that system.',
		'TEK-related menu commands can now be used when a vector graphics window is frontmost.',
		'The Rename menu command now works with TEK windows.',
	],
	'20080404': [
		'Fixed bug (recently introduced) with Copy command in TEK windows.',
		'Various minor layout improvements in the Preferences window.',
	],
	'20080403': [
		'Fixed all known stability problems when using multiple TEK windows.',
		'Since MacTelnet cannot currently input text directly to TEK windows, it no longer puts them in front when they open.',
	],
	'20080401': [
		'Fixed dynamic resize of TEK graphics, so they once again scale as the window is resized.',
		'Internal changes to improve vector graphics handling.',
	],
	'20080330': [
		'Fixed bug (recently introduced) where closing a vector graphics window would not restore terminal input.',
		'Preferences window Terminals pane has been refined further.',
	],
	'20080329': [
		'Improved overall terminal performance by adjusting the session loop to process data at a faster rate.',
	],
	'20080328': [
		'Internal changes to improve vector graphics handling.',
	],
	'20080327': [
		'Various minor tweaks to user interface text, such as alert messages.',
	],
	'20080326': [
		'Internal changes to improve how process spawns are handled, and how process attributes are saved.',
	],
	'20080325': [
		'Fixed a problem where icon-changing toolbar items (such as LEDs) may stop working after a toolbar is customized.',
		'Added a hidden preference (accessible through the "defaults" program) to randomize the Format of every new terminal window.',
	],
	'20080324': [
		'Various minor changes to the menu bar layout, including the removal of the Action menu.',
		'MacTelnet Help updated with additional preferences information, and a few minor corrections.',
	],
	'20080323': [
		'Fixed Preferences window Sessions pane Resource tab to properly handle text field entries.',
	],
	'20080322': [
		'Selecting the name of a Format from the View menu will now transform the active terminal window to use those fonts/colors.',
		'It is now possible to override the state of an LED toolbar item just by clicking on it.  (Can also be set by the terminal.)',
		'Fixed user interfaces to consult default preferences when a required setting is not actually defined by a chosen collection.',
		'Now using a slightly more correct control-key symbol in the Control Keys palette and various other user interface elements.',
		'Preferences window Sessions pane (and Special Key Sequences dialog) now using segmented views instead of menus in some places.',
		'Preferences window General pane Special tab now using a segmented view for cursor shape preferences.',
	],
	'20080321': [
		'Fixed significant persistence problems in preference collections.',
		'Using Show Help Tags on the Control Keys palette now displays the common abbreviations and meanings of each control key.',
		'Internal changes to restructure preferences, renaming some keys and placing collections into their own domains.',
	],
	'20080320': [
		'Preferences window Sessions pane Keyboard tab has been refined further.',
		'Session Info window now has a Device column, showing the pseudo-terminal connected to a process.',
		'Fixed setup of window title and command line display when creating certain kinds of sessions.',
	],
	'20080319': [
		'Fixed Custom Format sheet to show the actual terminal font size.',
		'Minor fix to definition of text selection regions, visible through their outline shape in inactive windows.',
	],
	'20080318': [
		'Preferences window General pane now allows command-N to be used for creating log-in shells.',
		'Preferences window Sessions pane (and Special Key Sequences dialog) slightly redesigned to collect keyboard-related settings.',
	],
	'20080317': [
		'Fixed terminal windows to respond to changes in the font or font size.',
		'Terminal view matte now renders with precisely the chosen color, not a tinted version of it.',
		'Terminal view now renders extra space between the focus ring and text of a view, in the default background color; this is known as padding.',
		'Terminal views now read terminal margin preferences (hidden, but accessible through the "defaults" program) when setting matte thickness.',
		'Terminal views now interpret terminal padding preferences as the size of the new interior space, not the thickness of the matte.',
		'MacTelnet Help updated with additional preferences information, and a few minor corrections.',
	],
	'20080316': [
		'Preferences window Formats pane and other color box interfaces now use a floating color panel.',
		'Fixed various keyboard focus quirks.',
	],
	'20080315': [
		'Preferences window Macros pane redesigned because of far too many bugs in the Apple implementation of pop-up menus in lists.',
		'Fixed help tags in the Preferences window collections drawer.',
	],
	'20080314': [
		'Fixed calculation of ideal terminal view size, which affected the dimensions chosen by commands such as Make Text Bigger.',
	],
	'20080313': [
		'Fixed calculations that set terminal view size based on screen dimensions, to no longer lose a row or column in some cases.',
		'Terminal views now read terminal padding preferences (hidden, but accessible through the "defaults" program) when setting matte thickness.',
	],
	'20080312': [
		'Fixed seemingly random display glitches, traced to a corner case in processing CSI parameters.',
	],
	'20080311': [
		'Preferences window Terminals pane is now visible, though incomplete.',
		'Fixed blink rate to not change as more windows are opened.',
		'Fixed blink to not include a random color (usually black) at the end.',
		'Fixed crash when clicking the close box of the command line window.',
	],
	'20080310': [
		'Terminal views now render blinking text with a quadratic-delay pulse effect.',
		'Terminal view renderer has been further optimized in minor ways.',
	],
	'20080308': [
		'Custom Format dialog now correctly affects a single window and not global preferences.',
		'Preferences window Formats pane now sets the sample correctly, but due to a display bug this can only be seen after resizing the window.',
	],
	'20080307': [
		'Terminal view renderer has had several internal improvements, now only partially redrawing the screen in many cases (which is faster).',
	],
	'20080306': [
		'Finally on Leopard, the application menu is no longer named "Python", and preferences will no longer be saved in the Python domain.',
		'Fixed bug with files not opening correctly after the application first launched.',
		'The preferences converter no longer displays graphical alerts, although it will print a status line to the console.',
		'Find dialog search highlighting now has a unique appearance that does not interfere with normal text selection.',
		'Find dialog now allows blank queries, so it is possible to remove all previous search highlighting when the dialog is closed.',
	],
	'20080305': [
		'New Login Shell (hidden command requiring Option key in File menu) now runs "/usr/bin/login -p -f user" so there is no password prompt.',
	],
	'20080303': [
		'Reverted the default emulator to VT100 (from VT102), pending some important accuracy fixes in VT102.',
		'Floating command line appearance improvements.',
	],
	'20080302': [
		'Preferences window Formats pane now correctly updates font and color preferences.',
		'Preferences window Formats pane now uses the system font panel; though only for font name and size settings.',
		'Any font can now be chosen, but MacTelnet takes a performance hit from forcing monospaced layout on proportional fonts.',
		'A warning is now displayed in the Format pane if the user chooses a font that will be slow.',
		'Terminal inactivity notification now supports an additional reaction, "keep alive", which sends text to the server after 10 minutes.',
		'Added Session.set_keep_alive_transmission(str) to Quills, allowing Python to override what is sent after a keep-alive timer expires.',
		'Added Session.keep_alive_transmission() to Quills, to determine what string is sent to sessions when keep-alive timers expire.',
	],
	'20080301': [
		'Preferences window Formats pane is now as wide as many other panels, which avoids some resizing and truncation of toolbar icons.',
		'Major internal changes to improve handling of clipboard data, and rendering in the Clipboard window.',
	],
	'20080229': [
		'Fixed "Automatically Copy selected text" preference to also Copy selections from double-clicks, triple-clicks and keyboard selections.',
		'Fixed the rendering of text selection outlines for such things as inactive windows and drags.',
	],
	'20080228': [
		'Terminal view text selections now support shift-down-arrow and shift-up-arrow to manipulate by one line vertically.',
		'Terminal view text selections now support shift-left-arrow and shift-right-arrow to manipulate by one character horizontally (with wrap).',
		'Terminal view text selections now support shift-command-left-arrow to jump to the beginning of the line.',
		'Terminal view text selections now support shift-command-right-arrow to jump to the end of the line.',
		'Terminal view rectangular text selections are also changed intelligently when these new keyboard short-cuts are used.',
		'The cursor location or cursor line can now be selected simply by using a new extension short-cut while no text selection exists.',
	],
	'20080227': [
		'Even more tweaks to the Find dialog; it is now horizontally much narrower by default, but still resizable.',
		'Find dialog history menu no longer saves empty or all-whitespace searches.',
		'Fixed help tags in the Find dialog.',
	],
	'20080226': [
		'Further tweaks to the Find dialog; it is now about as vertically small as it can be, to show as many results as possible.',
	],
	'20080224': [
		'Significantly changed the layout of the Find dialog, to increase visibility of the terminal text underneath.',
		'Menus now correctly display the names of various Preferences collections as they are added.',
		'Preferences window once again has a help button; but a footer frame was added to give the button a logical place to be.',
		'Preferences window General pane Options tab now has correct keyboard focus ordering.',
		'Added accessibility descriptions for color boxes and the add/remove buttons in the Preferences window.',
		'Added accessibility relationships between certain labels and views (useful with VoiceOver, for instance) in the Preferences window.',
	],
	'20080223': [
		'The mouse pointer shape is now reset when selecting another window, to prevent (for instance) a persistent I-beam.',
		'Very minor tweaks to accessibility descriptions, affecting such things as speech when VoiceOver is on.',
	],
	'20080222': [
		'Custom Format dialog and Preferences window Formats pane now display proper colors.',
	],
	'20080220': [
		'Fixed presentation of Custom Format dialog.',
		'MacTelnet Help has received several minor corrections and other edits.',
	],
	'20080216': [
		'Terminal bell sound can once again be arbitrary.  See Preferences window, General pane, Notification tab.',
		'The Print Screen command is no longer instantaneous, it displays a dialog (allowing export to PDF, among other print options).',
	],
	'20080212': [
		'Fixed a possible crash if an item being renamed was deleted with the "-" button in the Preferences window collections drawer.',
	],
	'20080210': [
		'Fixed a possible crash when quitting.',
		'Fixed item highlighting in Preferences window collections drawer, so that something is always selected.',
		'Preferences window collections drawer now contains a contextual menu button with commands for duplicating and renaming items.',
		'Preferences window collections drawer "-" button is no longer enabled for items that cannot be deleted.',
		'Terminal backing store can now use Unicode, which will enable better text and rendering support.',
	],
	'20080205': [
		'Preferences window Translations pane now actually shows lists for the base character set and exceptions.',
	],
	'20080203': [
		'Fixed truncation of the last line of text when copying or dragging rectangular selections.',
		'Fixed drag highlighting to not reveal text that is marked as "concealed".',
		'Fixed Preferences window General pane to properly save preferences in text fields (like window stacking origin).',
	],
	'20080202': [
		'Find will now properly highlight matching text.',
		'Find will now highlight *every* match, anywhere in your terminal screen or scrollback, instead of just the first one.',
		'Find dialog now searches live, as you type or change search options!',
		'Find dialog now displays the number of matching terms during live search.',
		'Find dialog now disappears immediately when the Go button finds a match; sheet animation is bypassed.',
		'Find dialog search history menu no longer has a fixed size.',
		'Fixed a possible crash after several uses of the Find dialog.',
	],
	'20080130': [
		'Save Selected Text interface has been modernized, displaying a sheet and using Unicode.',
	],
	'20080129': [
		'The Quit warning is no longer displayed if every terminal window was recently opened.',
	],
	'20080128': [
		'Window slide-animation during the review for Quit is now turned off for recently opened sessions, since they do not display alerts.',
		'Fixed file-opens for extensions that are not scripts, namely macros and session files!',
		'Added Terminal.dumb_strings_init(func) to Quills, allowing Python functions to define how a dumb terminal renders each character.',
	],
	'20080126': [
		'Terminal view rendering speed now improved slightly in general, and noticeably during text selection.',
		'Fixed a possible crash in debug mode when setting the scroll region.',
		'Fixed Dock menu.',
	],
	'20080121': [
		'Robustness improvements to the focus-follows-mouse feature, particularly with sheets and non-terminal windows.',
	],
	'20080120': [
		'Implemented the special editing modes of the VT102 (delete character, insert line, delete line).',
		'Improved some rendering in the Clipboard window.',
		'MacTelnet Help updated with some terminal emulator information.',
	],
	'20080115': [
		'Fixed drag and drop of text into terminal windows.',
		'Fixed print dialog display when Media Copy (line printing) sequences are sent by applications in VT102 terminals.',
		'The TERM variable is now properly initialized to match answerback preferences, instead of always using "vt100".',
	],
	'20080111': [
		'Added Session.on_fileopen_call(func, extension) to Quills, allowing Python functions to respond to file open requests by type.',
		'Added Session.stop_fileopen_call(func, extension) to Quills, to mirror Session.on_fileopen_call().',
		'Now any common scripting extension (like ".py" and ".sh") can be opened by MacTelnet.',
		'Preferences window Formats pane now has correctly sized tab content.',
		'Preferences window Translations pane is now visible, though incomplete.',
	],
	'20080103': [
		'Terminal window tabs forced to the bottom edge by the system (window too close to menu bar) are now corrected when you move the window.',
	],
	'20080101': [
		'MacTelnet Help has received several minor corrections and other edits.',
	],
	'20071231': [
		'Fixed window review on Quit to automatically show hidden sessions instead of ignoring them.',
	],
	'20071230': [
		'Terminal views now support a focus-follows-mouse General preference.',
		'Fixed initialization of certain settings when creating a brand new preferences file.',
	],
	'20071229': [
		'Fixed terminal activity notification, allowing you to watch for new data arriving in inactive windows.',
		'Terminal inactivity notification is now available, allowing you to watch for sessions that become idle.',
		'Session Info window icons are now updated when activity or inactivity notifications occur.',
		'Notifications now display a new style of modeless alert window similar to those used by file copies in the Finder.',
	],
	'20071227': [
		'Preferences command now correctly responds to its key equivalent even if the mouse has never hit the application menu.',
	],
	'20071225': [
		'Internal improvements to sessions to set a foundation for better text translation.',
		'Fixed Paste, for both 8-bit and 16-bit Unicode sources (that can be translated).',
		'Using Paste with multi-line Clipboard text now displays a warning with an option to form one line before proceeding.',
	],
	'20071221': [
		'Fixed ANSI color rendering.',
		'Preferences window resize box now has a transparent look on most panels.',
	],
	'20071219': [
		'This build should once again support Panther, Tiger and Leopard.',
	],
	'20071204': [
		'Fixed scroll bars in terminal windows to allow the indicator to be dragged.',
	],
	'20071119': [
		'Added an optional argument to set the working directory of new Sessions in the Quills API.',
		'Added a box to set the matte color in the Format preferences panel.',
		'Preferences window Macros pane now has an option to display the active macro set in a menu.',
	],
	'20071104': [
		'Internal changes to make MacTelnet run properly on Leopard.',
	],
	'20071103': [
		'Added support for the file URL type.  The default behavior is to run "emacs" in file browser mode.',
	],
	'20071023': [
		'Show IP Addresses command now displays a more sophisticated dialog with a proper list view for addresses.',
		'Show IP Addresses dialog now allows addresses to be copied to the Clipboard individually.',
	],
	'20071022': [
		'Terminal window tabs now recognize drags, automatically switching tabs when the mouse moves over them.',
	],
	'20071019': [
		'Terminal window toolbars now have a Bell item option.  Clicking it will enable *or* disable the terminal bell.',
		'Renamed the "Disable Bell" command to simply "Bell", which also inverts the state of its checkmark.',
		'Preferences window collections drawer further tweaked to align in an aesthetically pleasing way with tabs.',
		'Custom Screen Size dialog arrows now increment and decrement by 4 for columns, and 10 for rows.',
		'Changed View menu items showing columns-by-rows, to use the Unicode X-like symbol for times instead of an X.',
	],
	'20071015': [
		'Fixed the Interrupt Process command to send the interrupt character to the active session.',
		'Preferences window General pane now correctly displays cursor shapes.',
		'Preferences window General pane now uses Unicode text for cursor shapes instead of icon images.',
	],
	'20071013': [
		'Changed the artwork for the main application icon and other icons showing terminals.',
	],
	'20071011': [
		'Added an icon for the Translations preference category.',
	],
	'20071010': [
		'Fixed line truncation during operations such as Copy and drag-and-drop.',
		'Fixed word and line highlighting for double-clicks and triple-clicks.',
	],
	'20071006': [
		'Internal improvements to the code (removing some compile warnings, etc.).',
	],
	'20070929': [
		'Fixed cases where size-change commands were ineffective in one terminal resize mode but not the other.',
	],
	'20070918': [
		'Terminal view backgrounds now have a user interface for changing the color, via menu or contextual menu.',
		'Session Info window toolbar items rearranged in customization sheet, so that Customize is not hidden.',
	],
	'20070908': [
		'Fixed a few possible crash conditions.',
		'Redesigned the Find icon on the VT220 keypad.',
		'Key palettes now have a 5 pixel padding between buttons and the window edge.',
	],
	'20070906': [
		'Preferences window collections drawer is now below the toolbar to show that it affects only one category.',
		'Preferences window General pane now has correctly sized tab content.',
		'Preferences window Sessions pane now has correctly sized tab content.',
		'Terminal views and backgrounds now support a more complete set of accessibility attributes.',
		'Terminal view backgrounds now have an accessibility role of "matte".',
		'Terminal view drag highlight now uses Core Graphics for a smoother display.',
	],
	'20070827': [
		'Terminal windows now support tabs, accessible through a new General preference.',
		'Added Move to New Workspace command (available only when using tabs) to move tabs into separate groups.',
	],
	'20070424': [
		'Session Info window list selections can now be the target of session-related menu bar commands.',
	],
	'20070224': [
		'New, high-quality icons for the VT220 keypad window.',
	],
	'20070220': [
		'Keys menu renamed to Map, which is more accurate; also renamed some of the items in the menu.',
		'Session Info window toolbars may now have "Arrange All Windows in Front".',
	],
	'20070212': [
		'Added placeholder Fix Character command.',
	],
	'20070207': [
		'A new, simpler look for MacTelnet Help.',
		'Internal changes to improve the MacTelnet Help build system.',
	],
	'20070124': [
		'Fixed a case where terminal windows could open with toolbar focus instead of terminal keyboard focus.',
	],
	'20070109': [
		'Fixed VT52-mode cursor positioning.',
	],
	'20070108': [
		'Preferences window resizes now remembered per category, so choosing a panel will restore its window size.',
		'Interrupt now correctly resets the Suspend Network flag.',
		'Interrupt now displays a help tag above the cursor instead of writing text into the terminal.',
		'Suspend now displays a help tag above the cursor instead of writing text into the terminal.',
		'Resume now hides the Interrupt or Suspend help tag instead of writing text into the terminal.',
		'Internal changes to old QuickDraw code based on Apple recommendations for Intel compatibility.',
	],
	'20070107': [
		'Compiled as a Universal Application (for Intel Macs).  The Intel version has NOT been tested yet!',
	],
	'20070103': [
		'Identified a possible crash when switching the Window Resize Affects preference.  No fix is available yet.',
		'Corrected possible parsing problem when entering URLs containing whitespace into the command line.',
		'Internal changes to separate URL parsing code from handling code, which also simplifies tests.',
		'Internal changes to make MacTelnet Python files have very unique names, avoiding risk of import collisions.',
		'Internal changes to unit testing code in RunMacTelnet.py, to make it cleaner and easier to filter modules.',
	],
	'20061227': [
		'Updated color box buttons to use Core Graphics natively.',
		'Drag-and-drop highlight now respects the Graphite appearance setting, if applicable.',
	],
	'20061224': [
		'Open Session dialog no longer crashes if Cancel is chosen.',
		'Open Session dialog now supports more than one file at the same time.',
		'Tall (80 x 48) pre-defined size added to the View menu.',
		'Color boxes in the Format preferences panel now properly display a color.',
		'Color boxes in the Format preferences panel (ANSI Colors tab) can now change the color when clicked.',
		'Internal changes to avoid some memory copies in a few instances, for efficiency.',
	],
	'20061127': [
		'Internally, removed a lot of legacy and transitional cruft for handling the menu bar.',
		'Menus are now implemented in the normal way as a single NIB file.',
		'The Network menu has been removed; various menu items moved to different menus.',
		'New "Action" menu, a redundant menu interface to make commands easy to find by task!',
	],
	'20061113': [
		'Terminal is now properly focused for text input when opened via the New Session sheet.',
		'Fixed a possible crash condition when selecting text.',
		'Internal changes to prevent needless reconstruction of the Python interface during builds.',
	],
	'20061110': [
		'Fixed visual synchronization of changes in New Session dialog with the command line field.',
		'More section names in Preferences window now use a bold font (like Keynote and Pages do).',
		'Redesigned the Format preferences panel.',
		'Minor renaming of items in the Full Screen preferences panel.',
		'Arbitrarily reduced the default window size of Session Info.',
	],
	'20061107': [
		'Terminal view cursor blinking works once again.',
		'Fixed a problem with the mouse cursor changing when pointing just outside a selection.',
		'Internal changes to allow ANSI-BBS terminal type to be supported in the future.',
	],
	'20061105': [
		'Terminal views now hide the cursor whenever scrollback lines are displayed.',
		'Window hiding no longer fails for windows that were redisplayed under certain conditions.',
		'Window hiding animation has been sped up, because it is still synchronous.',
		'Window hiding seems to cause a session to be ignored during quitting time reviews; there is no fix yet.',
		'Preferences panel for Sessions now has functional DNS lookup on the Resource tab.',
	],
	'20061101': [
		'Some problems with scrollback function and display are now fixed.',
		'Section names in dialogs and in the Preferences window now use a bold font (like Keynote and Pages do).',
		'Various bits of internal code cleanup.',
	],
	'20061030': [
		'Fixed problems with renaming items in the collections drawer of the Preferences window.',
		'Fixed problems with adding and removing items in the collections drawer of the Preferences window.',
	],
	'20061029': [
		'Once again supporting rlogin URLs.',
		'Updated property list so the Finder, etc. realizes MacTelnet can handle a number of different types of URLs.',
		'Once again printing random biline text on the splash screen.  However, it is now localizable.',
		'Internal changes to put the MacTelnet core and generated Python API into a framework called Quills.framework.',
		'Internal changes to put Python-based portions of MacTelnet into a framework called PyMacTelnet.framework.',
		'Minor corrections to MacTelnet Help.',
	],
	'20061028': [
		'More than any other release so far, this build shows off the true power of the new Quills interface in Python!',
		'Added Session.on_urlopen_call(func, schema) to Quills, allowing Python functions to respond to URL requests.',
		'Added Session.stop_urlopen_call(func, schema) to Quills, to mirror Session.on_urlopen_call().',
		'Reimplemented every URL handler as Python code, greatly simplifying both implementation and maintenance.',
		'Added a series of doctest testcases to validate all URL handling code.',
		'Renamed Quills API Session.on_new_ignore() to Session.stop_new_call().',
	],
	'20061027': [
		'Added support for the x-man-page URL type.  For example, "x-man-page://ls" or "x-man-page://3/printf".',
		'Cursor shape preferences are once again respected.',
		'Corrected glitch in rendering lower right edges of inactive selection outlines.',
	],
	'20061026': [
		'Fixed position of inactive text selection outlines in terminal views.',
		'Internal change to fix to cursor positioning code in renderer.',
	],
	'20061025': [
		'MacTelnet Help has received several minor corrections and other edits.',
		'Jump Scrolling menu item now has a help tag.',
		'Internal changes to add icon identifiers to a central registry.',
		'Internal changes to allow a terminal view to not render a focus ring, if a special flag is set.',
	],
	'20061024': [
		'Terminal window toolbars now have a Full Screen item option.  Clicking it will start *or* end Full Screen.',
		'Zoom effect for hiding windows now more accurately approximates the location of the Window menu title.',
	],
	'20061023': [
		'Added keys for the 5 Full Screen preferences to DefaultPreferences.plist.',
		'Updated the Preferences window interface to properly save and restore the Full Screen checkboxes.',
		'Full Screen now respects the menu bar visibility preference.',
		'Full Screen menu bar checkbox label changed to more accurately show its effect.',
		'Full Screen now respects the scroll bar visibility preference.',
		'Full Screen now respects the Force Quit availability preference.',
		'Full Screen now respects the off-switch visibility preference.',
		'Full Screen is now usually disabled automatically if the terminal window closes (some quirks remain here).',
	],
	'20061022': [
		'Most of the Preferences window is broken due to recent internal changes.  Please do not submit bugs on it.',
		'Terminal window text selections do not work properly.  Please do not submit bugs on this.',
		'Terminal scrollback cannot be displayed.  Please do not submit bugs on this.',
	],
    '3.1.0': [
        'Decision was made to require Mac OS X 10.3.9 minimum.',
        'IPv6 addresses can now be used anywhere hosts are normally used.',
        'Get IP Address command now returns IPv6 format if possible.',
        'New Session sheet now supports SSH-1, SSH-2, SFTP, FTP and TELNET.',
        'All user interface elements reimplemented as NIBs.',
        'Full Keyboard Access now works throughout (because of NIBs change).',
        'VoiceOver now works throughout (because of NIBs change).',
        'Minor accessibility additions to allow VoiceOver to work with custom interfaces.',
        'User interface style (layout, etc.) revamped based on Mac OS X guidelines.',
		'The Network menu has been removed; various menu items moved to different menus.',
		'New "Action" menu, a redundant menu interface to make commands easy to find by task!',
        'Session Info window now fully functional.',
        'Session Info window allows you to select a window by double-clicking its list item.',
        'Session Info window allows you to change window titles simply by clicking and typing.',
        "Session Info window displays icon and text version of each session's status.",
        'Session Info window now has a customizable toolbar, with the "unified" appearance.',
        'Session Info window columns can be moved or resized, ordering is implicitly saved.',
        'Terminal emulation core rewritten from scratch!',
		'Terminal maximum width increased to 256 columns.',
		'Terminal bell sound can once again be arbitrary.  See Preferences window, General pane, Notification tab.',
		'Terminal inactivity notification is now available, allowing you to watch for sessions that become idle.',
        'Terminal window toolbars now use the standard Mac OS X implementation.',
        'Terminal window toolbar LED icon artwork has been significantly improved.',
        'Terminal window toolbars now have Suspend (Scroll Lock), Hide, Full Screen, Bell, and Print.',
        'Terminal windows now support live feedback during resize.',
		'Terminal windows now support tabs, accessible through a new General preference.',
        'Terminal window resizes can now change font *or* dimensions.',
        'Terminal window resize behavior can be inverted with the Option key.',
        'Terminal window resize behavior can now be set in Preferences.',
        'Terminal views are now implemented as HIViews, which simplifies some code.',
        'Terminal views now automatically switch to a contrasting color if an ANSI color would make text invisible.',
		'Terminal views now support a focus-follows-mouse General preference.',
        'Terminal text with blinking style now animates, with a gentle pulse.',
		'Find will now highlight *every* match, anywhere in your terminal screen or scrollback, instead of just the first one.',
		'Find dialog now searches live, as you type or change search options.',
        'Floating command line window now has a pop-up menu for history as well.',
        'Floating command line window now recognizes up-arrow for history rotation.',
        'Floating command line window now properly respects tab focusing.',
        'Key palettes have been reimplemented as Aqua windows with regular buttons.',
        'Key palette buttons that are entirely iconic now have Help Tags to describe them.',
        'Key palettes now have keyboard equivalents to show and hide their windows.',
        'TEK graphics windows now support live resize.',
        'TEK graphics can once again be copied to the clipboard.',
        'Clipboard window once again updates itself when the clipboard contents are changed.',
        'Clipboard window can now display Unicode text.',
		'Using Paste with multi-line Clipboard text now displays a warning with an option to form one line before proceeding.',
        'Drag highlight effect is much improved (similar to Apple Mail on Tiger).',
        'Windows can now be given titles containing Unicode characters.',
        'Most file interfaces now support Unicode names.',
        'Most file interfaces now use sheets.',
        'Printing dialogs now use sheets.',
        'Preferences have been reimplemented as com.mactelnet.MacTelnet.plist.',
        'New preferences are now initialized exclusively from DefaultPreferences.plist.',
        'Converter utility now automatically imports older preferences files.',
        'Preferences window now has a toolbar like in other Mac OS X applications.',
        'Preferences window now supports live editing of settings in collections!',
        'Preferences window now displays a drawer with a list of Favorites when appropriate.',
        'Preferences window now fully integrates macro editing.',
		'Preferences window Macros pane now has an option to display the active macro set in a menu.',
        'Preferences for fonts and colors are now separate, in Format Favorites.',
        'Preferences for sessions now properly support local Unix command lines.',
        'Preferences for Full Screen are now a panel instead of a modal dialog.',
        "Preferences for Scripts no longer offer event mapping (which didn't work anyway).",
		'Show IP Addresses command now displays a more sophisticated dialog with a proper list view for addresses.',
		'Show IP Addresses dialog now allows addresses to be copied to the Clipboard individually.',
        'Significant AppleScript interfaces are broken.  They will be removed in the future.',
        'MacTelnet core reimplemented as a framework loaded into the Python interpreter!!!',
        'Python API called "Quills" is now available, allowing Python scripts to call MacTelnet!',
        'Python functions can also be called *by* MacTelnet, allowing simple extensibility!',
        'Python API is very minimal right now, the plan is for this to become much bigger.',
        'Text translation is currently broken.  However, Quills will make this much easier.',
        'Apple ".command" files can now be opened by MacTelnet.',
        'Apple ".term" files can now be opened by MacTelnet, but some settings are ignored.',
		'Added support for the file URL type. The default behavior is to run "emacs" in file browser mode.',
		'Added support for the x-man-page URL type.  For example, "x-man-page://ls" or "x-man-page://3/printf".',
        'Several new application menu commands, including Check for Updates.',
        'The Special Characters command is now available in the Edit menu.',
        'Help tags do not appear automatically; turn them on with the Help menu.',
    ],
    '3.0.1': [
        'Fixes for dialog boxes that stopped accepting input in Mac OS X 10.3.',
    ],
}

def to_basic_html(lineage=version_lineage):
    """to_basic_html(lineage) -> list of string
    
    Return lines rendering the release notes as basic HTML.
    
    No fancy formatting is applied, nor is the document complete
    (e.g. it doesn't start with <html>).
    
    By default all versions are returned; you can override lineage.
    """
    result = []
    for version in lineage:
        result.append("<h1>%s</h1>" % version)
        result.append("<ul>")
        for note in notes_by_version[version]:
            result.append("<li>%s</li>" % note)
        result.append("</ul>")
    return result

def to_plain_text(lineage=version_lineage):
    """to_plain_text(lineage) -> string
    
    Return lines rendering the release notes as a continuous string,
    with minimal formatting delimiting each note.  This is necessary
    for a MacPAD file, among other things.
    
    By default all versions are returned; you can override lineage.
    """
    result = ''
    for version in lineage:
        result += ('[%s] ' % version)
        for note in notes_by_version[version]:
            result += ("* %s   " % note)
    return result

def to_rst(lineage=version_lineage):
    """to_rst(lineage) -> list of string
    
    Return lines rendering the release notes as reStructuredText.
    
    By default all versions are returned; you can override lineage.
    """
    result = []
    for version in lineage:
        result.append(''.join(['-' for x in version]))
        result.append(version)
        result.append(''.join(['-' for x in version]))
        result.append('')
        for note in notes_by_version[version]:
            result.append("- %s" % note)
        result.append('')
    return result

if __name__ == '__main__':
    import sys
    # by default, all versions are returned; or, specify which ones
    format = sys.argv[1]
    if len(sys.argv) > 2: lineage = sys.argv[2:]
    else: lineage = version_lineage
    if format == "plain_text": print to_plain_text(lineage)
    elif format == "rst": print "\n".join(to_rst(lineage))
    else: print "\n".join(to_basic_html(lineage))


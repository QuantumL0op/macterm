/*!	\file CocoaExtensions.mm
	\brief Methods added to standard classes.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.6
	© 2008-2011 by Kevin Grant
	
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

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Public Methods

@implementation NSObject (CocoaExtensions_NSObject)


/*!
Given a property method name, such as fooBar, returns the
conventional name for a selector to indicate whether or not
changes to that key will automatically notify observers; in
this case, autoNotifyOnChangeToFooBar.

The idea is to keep property information close to the
accessors for those properties, instead of making the
"automaticallyNotifiesObserversForKey:" method big and ugly.

Related: NSObject(NSKeyValueObservingCustomization).

(4.0)
*/
+ (NSString*)
selectorNameForKeyChangeAutoNotifyFlag:(NSString*)	aPropertySelectorName
{
	NSString*			result = nil;
	NSMutableString*	nameOfAccessor = [[[NSMutableString alloc] initWithString:aPropertySelectorName]
											autorelease];
	NSString*			accessorFirstChar = [nameOfAccessor substringToIndex:1];
	
	
	[nameOfAccessor replaceCharactersInRange:NSMakeRange(0, 1/* length */)
												withString:[accessorFirstChar uppercaseString]];
	result = [@"autoNotifyOnChangeTo" stringByAppendingString:nameOfAccessor];
	return result;
}// selectorNameForKeyChangeAutoNotifyFlag:


/*!
Given a selector, such as @selector(fooBar), returns the
conventional selector for a method that would determine whether
or not property changes will automatically notify any observers;
in this case, @selector(autoNotifyOnChangeToFooBar).

The signature of the validator is expected to be:
+ (id) autoNotifyOnChangeToFooBar;
Due to limitations in performSelector:, the result is not a
BOOL, but rather an object of type NSNumber, whose "boolValue"
method is called.  Validators are encouraged to use the method
[NSNumber numberWithBool:] when returning their results.

See selectorNameForKeyChangeAutoNotifyFlag: and
automaticallyNotifiesObserversForKey:.

Related: NSObject(NSKeyValueObservingCustomization).

(4.0)
*/
+ (SEL)
selectorToReturnKeyChangeAutoNotifyFlag:(SEL)	anAccessor
{
	SEL		result = NSSelectorFromString([[self class] selectorNameForKeyChangeAutoNotifyFlag:NSStringFromSelector(anAccessor)]);
	
	
	return result;
}// selectorToReturnKeyChangeAutoNotifyFlag:


@end // NSObject(CocoaExtensions_NSObject)

// BELOW IS REQUIRED NEWLINE TO END FILE
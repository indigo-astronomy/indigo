/*
 * Copyright (c) 2007 Dave Dribin
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#import "DDHidKeyboard.h"
#import "DDHidElement.h"
#import "DDHidUsage.h"
#import "DDHidQueue.h"
#import "DDHidEvent.h"
#include <IOKit/hid/IOHIDUsageTables.h>

@interface DDHidKeyboard (DDHidKeyboardDelegate)

- (void) ddhidKeyboard: (DDHidKeyboard *) keyboard
               keyDown: (unsigned) usageId;

- (void) ddhidKeyboard: (DDHidKeyboard *) keyboard
                 keyUp: (unsigned) usageId;

@end

@interface DDHidKeyboard (Private)

- (void) initKeyboardElements: (NSArray *) elements;
- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;

@end

@implementation DDHidKeyboard

+ (NSArray *) allKeyboards;
{
    return
        [DDHidDevice allDevicesMatchingUsagePage: kHIDPage_GenericDesktop
                                         usageId: kHIDUsage_GD_Keyboard
                                       withClass: self
                               skipZeroLocations: YES];
}

- (id) initWithDevice: (io_object_t) device error: (NSError **) error_;
{
    self = [super initWithDevice: device error: error_];
    if (self == nil)
        return nil;
    
    mKeyElements = [[NSMutableArray alloc] init];
    [self initKeyboardElements: [self elements]];
    
    return self;
}

//=========================================================== 
// dealloc
//=========================================================== 
- (void) dealloc
{
    
    mKeyElements = nil;
}

#pragma mark -
#pragma mark Keyboards Elements

- (NSArray *) keyElements;
{
    return mKeyElements;
}

- (unsigned) numberOfKeys;
{
    return [mKeyElements count];
}

- (void) addElementsToQueue: (DDHidQueue *) queue;
{
    [queue addElements: mKeyElements];
}

#pragma mark -
#pragma mark Asynchronous Notification

- (void) setDelegate: (id) delegate;
{
    mDelegate = delegate;
}

- (void) addElementsToDefaultQueue;
{
    [self addElementsToQueue: mDefaultQueue];
}

@end

@implementation DDHidKeyboard (DDHidKeyboardDelegate)

- (void) ddhidKeyboard: (DDHidKeyboard *) keyboard
               keyDown: (unsigned) usageId;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidKeyboard: keyboard keyDown: usageId];
}

- (void) ddhidKeyboard: (DDHidKeyboard *) keyboard
                 keyUp: (unsigned) usageId;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidKeyboard: keyboard keyUp: usageId];
}

@end

@implementation DDHidKeyboard (Private)

- (void) initKeyboardElements: (NSArray *) elements;
{
    NSEnumerator * e = [elements objectEnumerator];
    DDHidElement * element;
    while (element = [e nextObject])
    {
        unsigned usagePage = [[element usage] usagePage];
        unsigned usageId = [[element usage] usageId];
        if (usagePage == kHIDPage_KeyboardOrKeypad)
        {
            if (((usageId >= 0x04) && (usageId <= 0xA4)) ||
                ((usageId >= 0xE0) && (usageId <= 0xE7)))
            {
                [mKeyElements addObject: element];
            }
        }
        NSArray * subElements = [element elements];
        if (subElements != nil)
            [self initKeyboardElements: subElements];
    }
}

- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;
{
    DDHidEvent * event;
    while ((event = [hidQueue nextEvent]))
    {
        DDHidElement * element = [self elementForCookie: [event elementCookie]];
        unsigned usageId = [[element usage] usageId];
        SInt32 value = [event value];
        if (value == 1)
            [self ddhidKeyboard: self keyDown: usageId];
        else
            [self ddhidKeyboard: self keyUp: usageId];
    }
}

@end

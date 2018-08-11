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

#import "DDHidMouse.h"
#import "DDHidUsage.h"
#import "DDHidElement.h"
#import "DDHidQueue.h"
#import "DDHidEvent.h"
#include <IOKit/hid/IOHIDUsageTables.h>


// Implement our own delegate methods

@interface DDHidMouse (DDHidMouseDelegate)

- (void) ddhidMouse: (DDHidMouse *) mouse xChanged: (SInt32) deltaX;
- (void) ddhidMouse: (DDHidMouse *) mouse yChanged: (SInt32) deltaY;
- (void) ddhidMouse: (DDHidMouse *) mouse wheelChanged: (SInt32) deltaWheel;
- (void) ddhidMouse: (DDHidMouse *) mouse buttonDown: (unsigned) buttonNumber;
- (void) ddhidMouse: (DDHidMouse *) mouse buttonUp: (unsigned) buttonNumber;

@end

@interface DDHidMouse (Private)

- (void) initMouseElements: (NSArray *) elements;
- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;

@end

@implementation DDHidMouse

+ (NSArray *) allMice;
{
    return
        [DDHidDevice allDevicesMatchingUsagePage: kHIDPage_GenericDesktop
                                         usageId: kHIDUsage_GD_Mouse
                                       withClass: self
                               skipZeroLocations: YES];
}

- (id) initWithDevice: (io_object_t) device error: (NSError **) error_;
{
    self = [super initWithDevice: device error: error_];
    if (self == nil)
        return nil;
    
    mXElement = mYElement = mWheelElement = nil;
    mButtonElements = [[NSMutableArray alloc] init];
    [self initMouseElements: [self elements]];
    
    return self;
}

//=========================================================== 
// dealloc
//=========================================================== 
- (void) dealloc
{
    
    mXElement = nil;
    mYElement = nil;
    mWheelElement = nil;
    mButtonElements = nil;
}

#pragma mark -
#pragma mark Mouse Elements

//=========================================================== 
// - xElement
//=========================================================== 
- (DDHidElement *) xElement
{
    return mXElement; 
}

//=========================================================== 
// - yElement
//=========================================================== 
- (DDHidElement *) yElement
{
    return mYElement; 
}

- (DDHidElement *) wheelElement;
{
    return mWheelElement;
}

//=========================================================== 
// - buttonElements
//=========================================================== 
- (NSArray *) buttonElements;
{
    return mButtonElements; 
}

- (unsigned) numberOfButtons;
{
    return [mButtonElements count];
}

- (void) addElementsToQueue: (DDHidQueue *) queue;
{
    [queue addElement: mXElement];
    [queue addElement: mYElement];
    [queue addElement: mWheelElement];
    [queue addElements: mButtonElements];
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

@implementation DDHidMouse (DDHidMouseDelegate)

- (void) ddhidMouse: (DDHidMouse *) mouse xChanged: (SInt32) deltaX;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidMouse: mouse xChanged: deltaX];
}

- (void) ddhidMouse: (DDHidMouse *) mouse yChanged: (SInt32) deltaY;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidMouse: mouse yChanged: deltaY];
}

- (void) ddhidMouse: (DDHidMouse *) mouse wheelChanged: (SInt32) deltaWheel;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidMouse: mouse wheelChanged: deltaWheel];
}

- (void) ddhidMouse: (DDHidMouse *) mouse buttonDown: (unsigned) buttonNumber;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidMouse: mouse buttonDown: buttonNumber];
}

- (void) ddhidMouse: (DDHidMouse *) mouse buttonUp: (unsigned) buttonNumber;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidMouse: mouse buttonUp: buttonNumber];
}

@end

@implementation DDHidMouse (Private)

- (void) initMouseElements: (NSArray *) elements;
{
    NSEnumerator * e = [elements objectEnumerator];
    DDHidElement * element;
    while (element = [e nextObject])
    {
        unsigned usagePage = [[element usage] usagePage];
        unsigned usageId = [[element usage] usageId];
        if ((usagePage == kHIDPage_GenericDesktop) &&
            (usageId == kHIDUsage_GD_X))
        {
            mXElement = element;
        }
        else if ((usagePage == kHIDPage_GenericDesktop) &&
                 (usageId == kHIDUsage_GD_Y))
        {
            mYElement = element;
        }
        else if ((usagePage == kHIDPage_GenericDesktop) &&
                 (usageId == kHIDUsage_GD_Wheel))
        {
            mWheelElement = element;
        }
        else if ((usagePage == kHIDPage_Button) &&
                 (usageId > 0))
        {
            [mButtonElements addObject: element];
        }
        NSArray * subElements = [element elements];
        if (subElements != nil)
            [self initMouseElements: subElements];
    }
}

- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;
{
    DDHidEvent * event;
    while ((event = [hidQueue nextEvent]))
    {
        IOHIDElementCookie cookie = [event elementCookie];
        SInt32 value = [event value];
        if (cookie == [[self xElement] cookie])
        {
            if (value != 0)
            {
                [self ddhidMouse: self xChanged: value];
            }
        }
        else if (cookie == [[self yElement] cookie])
        {
            if (value != 0)
            {
                [self ddhidMouse: self yChanged: value];
            }
        }
        else if (cookie == [[self wheelElement] cookie])
        {
            if (value != 0)
            {
                [self ddhidMouse: self wheelChanged: value];
            }
        }
        else
        {
            unsigned i = 0;
            for (i = 0; i < [[self buttonElements] count]; i++)
            {
                if (cookie == [[[self buttonElements] objectAtIndex: i] cookie])
                    break;
            }
            
            if (value == 1)
            {
                [self ddhidMouse: self buttonDown: i];
            }
            else if (value == 0)
            {
                [self ddhidMouse: self buttonUp: i];
            }
            else
            {
                DDHidElement * element = [self elementForCookie: [event elementCookie]];
                NSLog(@"Element: %@, value: %d", [[element usage] usageName], [event value]);
            }
        }
    }
}

@end

